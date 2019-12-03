/*-
 * Copyright (c) 2017 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include "libdsbmc.h"

#define BUFSZ		  64
#define ERR_SYS_FATAL	  (DSBMC_ERR_SYS|DSBMC_ERR_FATAL)
#define PATH_DSBMD_SOCKET "/var/run/dsbmd.socket"

#define ERROR(dh, ret, error, prepend, fmt, ...) do {		\
	set_error(dh, error, prepend, fmt, ##__VA_ARGS__);	\
	return (ret);						\
} while (0)

#define VALIDATE(dh, dev) do {					\
	if (dev == NULL || dev->removed)			\
		ERROR(dh, -1, DSBMC_ERR_INVALID_DEVICE, false,	\
		    "Invalid device");				\
} while (0)

#define LOOKUP_DEV(dh, arg, dev) do {				\
	VALIDATE(dh, arg);					\
	dev = dsbmc_dev_from_id(dh, arg->id);			\
	VALIDATE(dh, dev);					\
} while (0)

typedef enum kwtype_e {
	KWTYPE_CHAR = 1, KWTYPE_STRING, KWTYPE_COMMANDS, KWTYPE_INTEGER,
	KWTYPE_UINT64, KWTYPE_UINT8, KWTYPE_DSKTYPE
} kwtype_t;

typedef union val_u val_t;
struct dsbmdkeyword_s {
	const char *key;
	kwtype_t   type;
	union val_u {
		int	 *integer;
		char	 *character;
		char	 **string;
		uint8_t  *uint8;
		uint64_t *uint64;
	} val;
};

static const struct cmdtbl_s {
	char	*name;
	uint8_t cmd;
} cmdtbl[] = {
	{ "mount",    DSBMC_CMD_MOUNT    },
	{ "unmount",  DSBMC_CMD_UNMOUNT  },
	{ "eject",    DSBMC_CMD_EJECT    },
	{ "speed",    DSBMC_CMD_SPEED    },
	{ "size",     DSBMC_CMD_SIZE     },
	{ "mdattach", DSBMC_CMD_MDATTACH }
};
#define NCMDS (sizeof(cmdtbl) / sizeof(struct cmdtbl_s))

/*
 * Struct to assign disk type strings to disktype IDs.
 */
static const struct disktypetbl_s {
        char   *name;
        uint8_t type;
} disktypetbl[] = {
        { "AUDIOCD", DSBMC_DT_AUDIOCD },
        { "DATACD",  DSBMC_DT_DATACD  },
        { "RAWCD",   DSBMC_DT_RAWCD   },
        { "USBDISK", DSBMC_DT_USBDISK },
        { "FLOPPY",  DSBMC_DT_FLOPPY  },
        { "DVD",     DSBMC_DT_DVD     },
        { "VCD",     DSBMC_DT_VCD     },
        { "SVCD",    DSBMC_DT_SVCD    },
        { "HDD",     DSBMC_DT_HDD     },
	{ "MMC",     DSBMC_DT_MMC     },
	{ "MTP",     DSBMC_DT_MTP     },
	{ "PTP",     DSBMC_DT_PTP     }
};
#define NDSKTYPES (sizeof(disktypetbl) / sizeof(struct disktypetbl_s))

/*
 * Error code translation.
 */
static const struct errmsg_s {
	int  code;
	char *msg;
} errmsgs[] = {
	{ DSBMC_ERR_ALREADY_MOUNTED,	"Device already mounted"   },
	{ DSBMC_ERR_PERMISSION_DENIED,	"Permission denied"	   },
	{ DSBMC_ERR_NOT_MOUNTED,	"Device not mounted"	   },
	{ DSBMC_ERR_DEVICE_BUSY,	"Device busy"		   },
	{ DSBMC_ERR_NO_SUCH_DEVICE,	"No such device"	   },
	{ DSBMC_ERR_NOT_EJECTABLE,	"Device not ejectable"	   },
	{ DSBMC_ERR_UNKNOWN_COMMAND,	"Unknown command"	   },
	{ DSBMC_ERR_UNKNOWN_OPTION,	"Unknown option"	   },
	{ DSBMC_ERR_SYNTAX_ERROR,	"Syntax error"		   },
	{ DSBMC_ERR_NO_MEDIA,		"No media"		   },
	{ DSBMC_ERR_UNKNOWN_FILESYSTEM,	"Unknown filesystem"	   },
	{ DSBMC_ERR_UNKNOWN_ERROR,	"Unknown error"		   },
	{ DSBMC_ERR_MNTCMD_FAILED,	"Mount command failed"	   },
	{ DSBMC_ERR_INVALID_ARGUMENT,	"Invalid argument"	   },
	{ DSBMC_ERR_MAX_CONN_REACHED,	"Max. number of connections reached" },
	{ DSBMC_ERR_STRING_TOO_LONG,	"Command string too long"  },
	{ DSBMC_ERR_BAD_STRING,		"Invalid command string"   },
	{ DSBMC_ERR_TIMEOUT,		"Timeout"		   },
	{ DSBMC_ERR_NOT_A_FILE,		"Not a regular file"	   },
};
#define NERRMSGS (sizeof(errmsgs) / sizeof(struct errmsg_s))

static int	   uconnect(dsbmc_t *, const char *);
static int	   send_string(dsbmc_t *, const char *);
static int	   push_event(dsbmc_t *, const char *);
static int	   parse_event(dsbmc_t *, const char *);
static int	   process_event(dsbmc_t *, char *);
static int	   dsbmc_send(dsbmc_t *, const char *, ...);
static int	   dsbmc_send_async(dsbmc_t *, dsbmc_dev_t *,
			void (*cb)(int, const dsbmc_dev_t *), const char *, ...);
static void	   dsbmc_clearerr(dsbmc_t *);
static void	   set_error(dsbmc_t *, int, bool, const char *, ...);
static void	   del_device(dsbmc_t *, const char *);
static void	   cleanup(dsbmc_t *);
static char	   *readln(dsbmc_t *);
static char	   *read_event(dsbmc_t *, bool);
static char	   *pull_event(dsbmc_t *);
static dsbmc_dev_t *add_device(dsbmc_t *, const dsbmc_dev_t *);
static dsbmc_dev_t *lookup_device(dsbmc_t *, const char *);

int
dsbmc_mount(dsbmc_t *dh, const dsbmc_dev_t *d)
{
	int	     ret;
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	if ((ret = dsbmc_send(dh, "mount %s\n", dev->dev)) == 0) {
		if (dh->event.devinfo.mntpt == NULL) {
			ERROR(dh, -1, DSBMC_ERR_UNKNOWN_ERROR, false,
			    "mntpt == NULL");
		}
		dev->mounted = true; free(dev->mntpt);
		dev->mntpt = strdup(dh->event.devinfo.mntpt);
		if (dev->mntpt == NULL)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "strdup()");
	}
	return (ret);
}

int
dsbmc_unmount(dsbmc_t *dh, const dsbmc_dev_t *d, bool force)
{
	int ret;
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	if ((ret = dsbmc_send(dh, force ? "unmount -f %s\n" : "unmount %s\n",
	    d->dev)) == 0) {
		dev->mounted = false;
	}
	return (ret);
}

int
dsbmc_eject(dsbmc_t *dh, const dsbmc_dev_t *d, bool force)
{
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	return (dsbmc_send(dh, force ? "eject -f %s\n" : "eject %s\n", d->dev));
}

int
dsbmc_size(dsbmc_t *dh, const dsbmc_dev_t *d)
{
	int ret;
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	if ((ret = dsbmc_send(dh, "size %s\n", d->dev)) == 0) {
		dev->mediasize = dh->event.mediasize;
		dev->used = dh->event.used;
		dev->free = dh->event.free;
	}
	return (ret);
}

int
dsbmc_set_speed(dsbmc_t *dh, const dsbmc_dev_t *d, int speed)
{
	int ret;
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	if ((ret = dsbmc_send(dh, "speed %s %d\n", dev->dev, speed)) == 0)
		dev->speed = dh->event.devinfo.speed;
	return (ret);
}

int
dsbmc_mdattach(dsbmc_t *dh, const char *image)
{
	return (dsbmc_send(dh, "mdattach \"%s\"\n", image));
}

int
dsbmc_mount_async(dsbmc_t *dh, const dsbmc_dev_t *d,
	void (*cb)(int, const dsbmc_dev_t *))
{
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	return (dsbmc_send_async(dh, dev, cb, "mount %s\n", dev->dev));
}

int
dsbmc_unmount_async(dsbmc_t *dh, const dsbmc_dev_t *d, bool force,
	void (*cb)(int, const dsbmc_dev_t *))
{
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	return (dsbmc_send_async(dh, dev, cb, force ? "unmount -f %s\n" : \
	    "unmount %s\n", dev->dev));
}

int
dsbmc_eject_async(dsbmc_t *dh, const dsbmc_dev_t *d, bool force,
	void (*cb)(int, const dsbmc_dev_t *))
{
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	return (dsbmc_send_async(dh, dev, cb, force ? "eject -f %s\n" : \
	    "eject %s\n", dev->dev));
}

int
dsbmc_set_speed_async(dsbmc_t *dh, const dsbmc_dev_t *d, int speed,
	void (*cb)(int, const dsbmc_dev_t *))
{
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	return (dsbmc_send_async(dh, dev, cb, "speed %s %d\n", dev->dev, speed));
}

int
dsbmc_size_async(dsbmc_t *dh, const dsbmc_dev_t *d,
	void (*cb)(int, const dsbmc_dev_t *))
{
	dsbmc_dev_t *dev;

	LOOKUP_DEV(dh, d, dev);
	return (dsbmc_send_async(dh, dev, cb, "size %s\n", dev->dev));
}

int
dsbmc_mdattach_async(dsbmc_t *dh, const char *image,
	void (*cb)(int, const dsbmc_dev_t *))
{
	return (dsbmc_send_async(dh, NULL, cb, "mdattach \"%s\"\n", image));
}

void
dsbmc_disconnect(dsbmc_t *dh)
{
	(void)dsbmc_send(dh, "quit\n");
	(void)close(dh->socket);
}

void
dsbmc_free_dev(dsbmc_t *dh, const dsbmc_dev_t *dev)
{
	if (dev == NULL || !dev->removed)
		return;
	del_device(dh, dev->dev);
}

static void
cleanup(dsbmc_t *dh)
{
	size_t i;

	for (i = 0; i < dh->ndevs; i++) {
		dh->devs[i]->removed = true;
		del_device(dh, dh->devs[i]->dev);
	}
	for (i = 0; i < dh->evq.n; i++)
		free(dh->evq.ln[i]);
	while (dh->cmdqsz != 0 && --(dh->cmdqsz) > 0)
		free(dh->sender[dh->cmdqsz].cmd);
	free(dh->lbuf);
	free(dh->pbuf);
	free(dh->sbuf);
	free(dh);
}

int
dsbmc_get_err(dsbmc_t *dh, const char **errmsg)
{
	if (!dh->error)
		return (0);
	if (errmsg != NULL)
		*errmsg = dh->errormsg;
	return (dh->error);
}

int
dsbmc_get_fd(dsbmc_t *dh)
{
	return (dh->socket);
}

const char *
dsbmc_errstr(dsbmc_t *dh)
{
	return (dh->errormsg);
}

const char *
dsbmc_errcode_to_str(int code)
{
	size_t i;

	if (code < (1 << 8))
		return (strerror(code));
	for (i = 0; i < NERRMSGS; i++) {
		if (errmsgs[i].code == code)
			return (errmsgs[i].msg);
	}
	return (NULL);
}

int
dsbmc_fetch_event(dsbmc_t *dh, dsbmc_event_t *ev)
{
	int  error;
	char *e;

	dsbmc_clearerr(dh);
	while ((e = read_event(dh, false)) != NULL) {
		if (push_event(dh, e) == -1)
			ERROR(dh, -1, 0, true, "push_event()");
	}
	if (dsbmc_get_err(dh, NULL) != 0)
		ERROR(dh, -1, 0, true, "read_event()");
	if ((e = pull_event(dh)) == NULL)
		return (0);
	if ((error = process_event(dh, e)) == 1) {
		ev->type = dh->event.type;
		ev->code = dh->event.code;
		if (dh->event.devinfo.dev != NULL)
			ev->dev = lookup_device(dh, dh->event.devinfo.dev);
	}
	return (error);
}

dsbmc_t *
dsbmc_alloc_handle()
{
	dsbmc_t *dh;

	if ((dh = malloc(sizeof(dsbmc_t))) == NULL)
		return (NULL);
	bzero(dh, sizeof(dsbmc_t));

	return (dh);
}

void
dsbmc_free_handle(dsbmc_t *dh)
{
	cleanup(dh);
}

int
dsbmc_connect(dsbmc_t *dh)
{
	char *e;

	if ((dh->socket = uconnect(dh, PATH_DSBMD_SOCKET)) == -1) {
		ERROR(dh, -1, ERR_SYS_FATAL, true, "uconnect(%s)",
		    PATH_DSBMD_SOCKET);
	}
	/* Get device list */
	while ((e = read_event(dh, true)) != NULL) {
		if (process_event(dh, e) == -1)
			ERROR(dh, -1, 0, true, "parse_event()");
		if (dh->event.type == DSBMC_EVENT_ERROR_MSG &&
		    dh->event.code == DSBMC_ERR_PERMISSION_DENIED) {
			ERROR(dh, -1, DSBMC_ERR_FATAL | DSBMC_ERR_CONN_DENIED,
			    false, "Permission denied");
		}
		if (dh->event.type != DSBMC_EVENT_ADD_DEVICE &&
		    dh->event.type != DSBMC_EVENT_END_OF_LIST) {
			ERROR(dh, -1, ERR_SYS_FATAL, false,
			    "Unexpected event (%d) received", dh->event.type);
		} else if (dh->event.type == DSBMC_EVENT_END_OF_LIST)
			break;
	}
	return (0);
}

int
dsbmc_get_devlist(dsbmc_t *dh, const dsbmc_dev_t ***list)
{
	*list = (const dsbmc_dev_t **)dh->devs;

	return (dh->ndevs);
}

dsbmc_dev_t *
dsbmc_dev_from_id(dsbmc_t *dh, int id)
{
	int i;

	for (i = dh->ndevs - 1; i >= 0; i--) {
		if (dh->devs[i]->id == id)
			return (dh->devs[i]);
	}
	return (NULL);
}

dsbmc_dev_t *
dsbmc_dev_from_name(dsbmc_t *dh, const char *dev)
{
	return (lookup_device(dh, dev));
}

dsbmc_dev_t *
dsbmc_next_dev(dsbmc_t *dh, int *idx, bool removed)
{
	int n;

	if (*idx < 0 || (size_t)*idx >= dh->ndevs)
		return (NULL);
	for (n = dh->ndevs - 1; n - *idx >= 0; (*idx)++) {
		if (dh->devs[n - *idx]->removed) {
			if (removed)
				return (dh->devs[n - (*idx)++]);
		} else
			return (dh->devs[n - (*idx)++]);
	}
	return (NULL);
}

static void
dsbmc_clearerr(dsbmc_t *dh)
{
	dh->error = 0;
}

static void
set_error(dsbmc_t *dh, int error, bool prepend, const char *fmt, ...)
{
	int	_errno;
	char	errbuf[sizeof(dh->errormsg)];
	size_t  len;
	va_list ap;

	_errno = errno;

	va_start(ap, fmt);
	if (prepend) {
		dh->error |= error;
		if (error & DSBMC_ERR_FATAL) {
			if (strncmp(dh->errormsg, "Fatal error: ", 13) == 0) {
				memmove(dh->errormsg, dh->errormsg + 13,
				    strlen(dh->errormsg) - 12);
			}
			(void)strlcpy(errbuf, "Fatal error: ", sizeof(errbuf));
			len = strlen(errbuf);
		} else if (strncmp(dh->errormsg, "Error: ", 7) == 0) {
			memmove(dh->errormsg, dh->errormsg + 7,
			    strlen(dh->errormsg) - 6);
			(void)strlcpy(errbuf, "Error: ", sizeof(errbuf));
			len = strlen(errbuf);
 		} else
			len = 0;
		(void)vsnprintf(errbuf + len, sizeof(errbuf) - len, fmt, ap);

		len = strlen(errbuf);
		(void)snprintf(errbuf + len, sizeof(errbuf) - len, ": %s",
		    dh->errormsg);
		(void)strcpy(dh->errormsg, errbuf);
	} else {
		dh->error = error;
		(void)vsnprintf(dh->errormsg, sizeof(dh->errormsg), fmt, ap);
		if (error == DSBMC_ERR_FATAL) {
			(void)snprintf(errbuf, sizeof(errbuf),
			    "Fatal error: %s", dh->errormsg);
		} else {
			(void)snprintf(errbuf, sizeof(errbuf),
			    "Error: %s", dh->errormsg);
		}	
		(void)strcpy(dh->errormsg, errbuf);
	}
	if ((error & DSBMC_ERR_SYS) && _errno != 0) {
		len = strlen(dh->errormsg);
		(void)snprintf(dh->errormsg + len, sizeof(dh->errormsg) - len,
		    ": %s", strerror(_errno));
		errno = 0;
	}
}

static int
uconnect(dsbmc_t *dh, const char *path)
{
	int  s;
	struct sockaddr_un saddr;

	if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
		ERROR(dh, -1, ERR_SYS_FATAL, false, "socket()");
	(void)memset(&saddr, (unsigned char)0, sizeof(saddr));
	(void)snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", path);
	saddr.sun_family = AF_LOCAL;
	if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
		ERROR(dh, -1, ERR_SYS_FATAL, false, "connect(%s)", path);
	if (fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK) == -1)
		ERROR(dh, -1, ERR_SYS_FATAL, false, "setvbuf()/fcntl()");
	return (s);
}

static dsbmc_dev_t *
add_device(dsbmc_t *dh, const dsbmc_dev_t *d)
{
	static int   id = 1;
	dsbmc_dev_t *dp, **dl;

	if ((dp = lookup_device(dh, d->dev)) != NULL && !dp->removed)
		return (NULL);
	dl = realloc(dh->devs, sizeof(dsbmc_dev_t *) * (dh->ndevs + 1));
	if (dl == NULL)
		ERROR(dh, NULL, ERR_SYS_FATAL, false, "realloc()");
	dh->devs = dl;
	if ((dh->devs[dh->ndevs] = malloc(sizeof(dsbmc_dev_t))) == NULL)
		ERROR(dh, NULL, ERR_SYS_FATAL, false, "malloc()");
	dp = dh->devs[dh->ndevs];
	if ((dp->dev = strdup(d->dev)) == NULL)
		ERROR(dh, NULL, ERR_SYS_FATAL, false, "strdup()");
	if (d->volid != NULL) {
		if ((dp->volid = strdup(d->volid)) == NULL)
			ERROR(dh, NULL, ERR_SYS_FATAL, false, "strdup()");
	} else
		dp->volid = NULL;
	if (d->mntpt != NULL) {
		if ((dp->mntpt = strdup(d->mntpt)) == NULL)
			ERROR(dh, NULL, ERR_SYS_FATAL, false, "strdup()");
		dp->mounted = true;
	} else {
		dp->mntpt   = NULL;
		dp->mounted = false;
	}
	if (d->fsname != NULL) {
		if ((dp->fsname = strdup(d->fsname)) == NULL)
			ERROR(dh, NULL, ERR_SYS_FATAL, false, "strdup()");
	} else
		dp->fsname = NULL;
	dp->id	    = id++;
	dp->type    = d->type;
	dp->cmds    = d->cmds;
	dp->speed   = d->speed;
	dp->removed = false;

	/* Add our own commands to the device's command list, and set VolIDs. */
	switch (d->type) {
	case DSBMC_DT_AUDIOCD:
		dp->volid = strdup("Audio CD");
	case DSBMC_DT_DVD:
		if (dp->volid == NULL)
			dp->volid = strdup("DVD");
	case DSBMC_DT_SVCD:
		if (dp->volid == NULL)
			dp->volid = strdup("SVCD");
	case DSBMC_DT_VCD:
		if (dp->volid == NULL)
			dp->volid = strdup("VCD");
		/* Playable media. */
		dp->cmds |= DSBMC_CMD_PLAY;
	}
	if (dp->volid == NULL)
		dp->volid = strdup(d->dev);
	if (dp->volid == NULL)
		ERROR(dh, NULL, ERR_SYS_FATAL, false, "strdup()");
	if ((d->cmds & DSBMC_CMD_MOUNT)) {
		/* Device we can open in a filemanager. */
		dp->cmds |= DSBMC_CMD_OPEN;
	}
	return (dh->devs[dh->ndevs++]);
}

static void
del_device(dsbmc_t *dh, const char *dev)
{
	int i;

	for (i = (int)dh->ndevs - 1; i >= 0; i--) {
		if (!dh->devs[i]->removed)
			continue;
		if (strcmp(dh->devs[i]->dev, dev) == 0)
			break;
	}
	if (i < 0)
		return;
	free(dh->devs[i]->dev);
	free(dh->devs[i]->volid);
	free(dh->devs[i]->mntpt);
	free(dh->devs[i]);
	for (; i < (int)dh->ndevs - 1; i++)
		dh->devs[i] = dh->devs[i + 1];
	dh->ndevs--;
}

static void
set_removed(dsbmc_t *dh, const char *dev)
{
	dsbmc_dev_t *dp;

	if ((dp = lookup_device(dh, dev)) == NULL)
		return;
	dp->removed = true;
}

dsbmc_dev_t *
lookup_device(dsbmc_t *dh, const char *dev)
{
	int i;
	
	if (dev == NULL)
		return (NULL);
	for (i = dh->ndevs - 1; i >= 0; i--) {
		if (dh->devs[i]->removed)
			continue;
		if (strcmp(dh->devs[i]->dev, dev) == 0)
			return (dh->devs[i]);
	}
	for (i = dh->ndevs - 1; i >= 0; i--) {
		if (strcmp(dh->devs[i]->dev, dev) == 0)
			return (dh->devs[i]);
	
	}
	return (NULL);
}

char *
readln(dsbmc_t *dh)
{
	int    n;
	size_t i;

	if (dh->lbuf == NULL) {
		if ((dh->lbuf = malloc(BUFSZ)) == NULL)
			return (NULL);
		dh->lbufsz = BUFSZ;
	}
	n = 0;
	do {
		dh->rd += n;
		if (dh->slen > 0) {
			(void)memmove(dh->lbuf, dh->lbuf + dh->slen,
			    dh->rd - dh->slen);
		}
		dh->rd  -= dh->slen;
		dh->slen = 0;
		for (i = 0; i < dh->rd && dh->lbuf[i] != '\n'; i++)
			;
		if (i < dh->rd) {
			dh->lbuf[i] = '\0';
			dh->slen = i + 1;
			if (dh->slen >= dh->lbufsz)
				dh->slen = dh->rd = 0;
			return (dh->lbuf);
		}
		if (dh->rd >= dh->lbufsz) {
			dh->lbuf = realloc(dh->lbuf, dh->lbufsz + BUFSZ);
			if (dh->lbuf == NULL) {
				ERROR(dh, NULL, ERR_SYS_FATAL, false,
				    "realloc()");
			}
			dh->lbufsz += BUFSZ;
		}
	} while ((n = read(dh->socket, dh->lbuf + dh->rd,
	    dh->lbufsz - dh->rd)) > 0);

	if (n == 0) {
		dh->rd = dh->slen = 0;
		ERROR(dh, NULL, DSBMC_ERR_LOST_CONNECTION, false,
		    "Lost connection to DSBMD");
	} else if (n == -1) {
		dh->rd = dh->slen = 0;
		if (errno != EINTR && errno != EAGAIN)
			ERROR(dh, NULL, ERR_SYS_FATAL, false, "read()");
	}
	return (NULL);
}

static char *
read_event(dsbmc_t *dh, bool block)
{
	char   *ln;
	fd_set rset;

	if ((ln = readln(dh)) == NULL) {
		if ((dh->error & DSBMC_ERR_LOST_CONNECTION) || !block)
			return (NULL);
	} else
		return (ln);
	FD_ZERO(&rset); FD_SET(dh->socket, &rset);
	/* Block until data is available. */
	while (select(dh->socket + 1, &rset, NULL, NULL, NULL) == -1) {
		if (errno != EINTR)
			return (NULL);
		else
			ERROR(dh, NULL, ERR_SYS_FATAL, false, "select()");
	}
	return (readln(dh));
}

static int
push_event(dsbmc_t *dh, const char *e)
{
	if (dh->evq.n >= DSBMC_MAXEQSZ)
		ERROR(dh, -1, ERR_SYS_FATAL, false, "DSBMC_MAXEQSZ exceeded.");
	if ((dh->evq.ln[dh->evq.n] = strdup(e)) == NULL)
		ERROR(dh, -1, ERR_SYS_FATAL, false, "strdup()");
	dh->evq.n++;

	return (0);
}

static char *
pull_event(dsbmc_t *dh)
{
	size_t i;

	if (dh->evq.i >= dh->evq.n) {
		/* Reset queue */
		for (i = 0; i < dh->evq.n; i++) {
			free(dh->evq.ln[i]);
			dh->evq.ln[i] = NULL;
		}
		dh->evq.n = dh->evq.i = 0;
		return (NULL);
	}
	return (dh->evq.ln[dh->evq.i++]);
}

static int
parse_event(dsbmc_t *dh, const char *str)
{
	char   *p, *q, *tmp, *k, *c;
	size_t i, len;
	struct dsbmdkeyword_s kwords[] = {
	  { "+",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "-",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "E",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "O",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "M",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "U",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "V",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "S",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "=",	  KWTYPE_CHAR,	  (val_t)&dh->event.type	   },
	  { "command=",	  KWTYPE_STRING,  (val_t)&dh->event.command	   },
	  { "dev=",	  KWTYPE_STRING,  (val_t)&dh->event.devinfo.dev    },
	  { "fs=",	  KWTYPE_STRING,  (val_t)&dh->event.devinfo.fsname },
	  { "volid=",	  KWTYPE_STRING,  (val_t)&dh->event.devinfo.volid  },
	  { "mntpt=",	  KWTYPE_STRING,  (val_t)&dh->event.devinfo.mntpt  },
	  { "type=",	  KWTYPE_DSKTYPE, (val_t)&dh->event.devinfo.type   },
	  { "speed=",	  KWTYPE_UINT8,	  (val_t)&dh->event.devinfo.speed  },
	  { "code=",	  KWTYPE_INTEGER, (val_t)&dh->event.code	   },
	  { "cmds=",	  KWTYPE_COMMANDS,(val_t)(char *)0	           },
	  { "mntcmderr=", KWTYPE_INTEGER, (val_t)&dh->event.mntcmderr      },
	  { "mediasize=", KWTYPE_UINT64,  (val_t)&dh->event.mediasize      },
	  { "used=",	  KWTYPE_UINT64,  (val_t)&dh->event.used	   },
	  { "free=",	  KWTYPE_UINT64,  (val_t)&dh->event.free	   }
	};
	const size_t nkwords = sizeof(kwords) / sizeof(kwords[0]);

	/* Init */
	len = strlen(str) + 1;
	if (dh->pbufsz < len) {
		dh->pbuf = realloc(dh->pbuf, len);
		if (dh->pbuf == NULL)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "realloc()");
		dh->pbufsz = len;
	}
	bzero(&dh->event, sizeof(dh->event));
	(void)strlcpy(dh->pbuf, str, len);

	for (i = 0; i < nkwords; i++) {
		if (kwords[i].val.string != NULL)
			*kwords[i].val.string = NULL;
	}
	for (p = dh->pbuf; (k = strsep(&p, ":\n")) != NULL;) {
		for (i = 0; i < nkwords; i++) {
			len = strlen(kwords[i].key);
			if (strncmp(kwords[i].key, k, len) == 0)
				break;
		}
		if (i == nkwords) {
			warnx("Unknown keyword '%s'", k);
			continue;
		}
		switch (kwords[i].type) {
		case KWTYPE_STRING:
			*kwords[i].val.string = k + len;
			break;
		case KWTYPE_CHAR:
			*kwords[i].val.character = *k;
			break;
		case KWTYPE_UINT8:
			*kwords[i].val.uint8 =
			    (uint8_t)strtol(k + len, NULL, 10);
			break;
		case KWTYPE_INTEGER:
			*kwords[i].val.integer =
			    strtol(k + len, NULL, 10);
			break;
		case KWTYPE_UINT64:
			*kwords[i].val.uint64 =
			    (uint64_t)strtoll(k + len, NULL, 10);
			break;
		case KWTYPE_COMMANDS:
			dh->event.devinfo.cmds = 0;
			if ((q = tmp = c = strdup(k + len)) == NULL)
				ERROR(dh, -1, ERR_SYS_FATAL, false, "strdup()");
			while ((c = strsep(&q, ",")) != NULL) {
				for (i = 0; i < NCMDS; i++) {
					if (strcmp(cmdtbl[i].name, c) == 0) {
						dh->event.devinfo.cmds |=
						    cmdtbl[i].cmd;
					}
				}
			}
			free(tmp);
			break;
		case KWTYPE_DSKTYPE:
			for (i = 0; i < NDSKTYPES; i++) {
				if (strcmp(disktypetbl[i].name, k + len) == 0) {
					dh->event.devinfo.type =
					    disktypetbl[i].type;
					break;
                        	}
                	}
			break;
		}
	}
	return (0);
}

/*
 * Parses  a  string  read from the dsbmd socket. Depending on the event type,
 * process_event() adds or deletes drives from the list, or updates a drive's
 * status.
 */
static int
process_event(dsbmc_t *dh, char *buf)
{
	size_t	     i;
	dsbmc_dev_t *d;

	if (parse_event(dh, buf) != 0)
		ERROR(dh, -1, 0, true, "parse_event()");
	if (dh->event.type == DSBMC_EVENT_SUCCESS_MSG) {
		if (dh->cmdqsz <= 0)
			return (0);
		d = dh->sender[0].dev;
	} else {
		switch (dh->event.type) {
		case DSBMC_EVENT_MOUNT:
		case DSBMC_EVENT_UNMOUNT:
		case DSBMC_EVENT_SPEED:
			d = lookup_device(dh, dh->event.devinfo.dev);
			if (d == NULL) {
				warnx("Unknown device %s",
				    dh->event.devinfo.dev);
				return (-1);
			}
		}
	}
	switch (dh->event.type) {
	case DSBMC_EVENT_SUCCESS_MSG:
		if (dh->cmdqsz <= 0)
			return (0);
		if (dh->sender[0].id == DSBMC_CMD_MOUNT) {
			d->mounted = true; free(d->mntpt);
			d->mntpt = strdup(dh->event.devinfo.mntpt);
			if (d->mntpt == NULL)
				ERROR(dh, -1, ERR_SYS_FATAL, false, "strdup()");
		} else if (dh->sender[0].id == DSBMC_CMD_UNMOUNT) {
			d->mounted = false;
		} else if (dh->sender[0].id == DSBMC_CMD_SIZE) {
			d->mediasize = dh->event.mediasize;
			d->used = dh->event.used;
			d->free = dh->event.free;
		} else if (dh->sender[0].id == DSBMC_CMD_SPEED) {
			d->speed = dh->event.devinfo.speed;
		}
	case DSBMC_EVENT_ERROR_MSG:
		if (dh->cmdqsz <= 0)
			return (0);
		dh->sender[0].callback(dh->event.code, dh->sender[0].dev);
		free(dh->sender[0].cmd);
		for (i = 0; i < dh->cmdqsz - 1; i++) {
			dh->sender[i].dev = dh->sender[i + 1].dev;
			dh->sender[i].callback = dh->sender[i + 1].callback;
			dh->sender[i].cmd = dh->sender[i + 1].cmd;
			dh->sender[i].id = dh->sender[i + 1].id;
		}
		if (--(dh->cmdqsz) == 0)
			return (0);
		if (send_string(dh, dh->sender[0].cmd) == -1)
			ERROR(dh, -1, 0, true, "send_string()");
		return (0);
	case DSBMC_EVENT_ADD_DEVICE:
		if ((d = add_device(dh, &dh->event.devinfo)) == NULL)
			ERROR(dh, -1, 0, true, "add_device()");
		return (1);
	case DSBMC_EVENT_DEL_DEVICE:
		set_removed(dh, dh->event.devinfo.dev);
		return (1);
	case DSBMC_EVENT_END_OF_LIST:
		return (0);
	case DSBMC_EVENT_SHUTDOWN:
		return (1);
	case DSBMC_EVENT_MOUNT:
		d->mounted = true; free(d->mntpt);
		d->mntpt = strdup(dh->event.devinfo.mntpt);
		if (d->mntpt == NULL)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "strdup()");
		return (1);
	case DSBMC_EVENT_UNMOUNT:
		d->mounted = false;
		return (1);
	case DSBMC_EVENT_SPEED:
		d->speed = dh->event.devinfo.speed;
		return (1);
	default:
		warnx("Invalid event received.");
		return (-1);
	}
	return (0);
}

static int
dsbmc_send_async(dsbmc_t *dh, dsbmc_dev_t *dev,
	void (*cb)(int, const dsbmc_dev_t *), const char *cmd, ...)
{
	char	*p;
	size_t	i, len;
	va_list ap;
	
	dsbmc_clearerr(dh);
	if (dh->cmdqsz >= DSBMC_CMDQMAXSZ)
		ERROR(dh, -1, DSBMC_ERR_CMDQ_BUSY, false, "Command queue busy");
	if (dh->sbufsz == 0) {
		if ((dh->sbuf = malloc(BUFSZ)) == NULL)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "malloc()");
		dh->sbufsz = BUFSZ;
	}
	for (;;) {
		va_start(ap, cmd);
		if ((size_t)vsnprintf(dh->sbuf, dh->sbufsz, cmd, ap) < dh->sbufsz)
			break;
		/* Send buffer too small. */
		if ((p = realloc(dh->sbuf, BUFSZ + dh->sbufsz)) == NULL)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "realloc()");
		dh->sbuf    = p;
		dh->sbufsz += BUFSZ;
	}
	dh->sender[dh->cmdqsz].dev = dev;
	dh->sender[dh->cmdqsz].callback = cb;

	for (i = 0; i < NCMDS; i++) {
		len = strlen(cmdtbl[i].name);
		if (strncmp(cmdtbl[i].name, cmd, len) == 0) {
			dh->sender[dh->cmdqsz].id = cmdtbl[i].cmd;
			break;
		}
	}
	if ((dh->sender[dh->cmdqsz].cmd = strdup(dh->sbuf)) == NULL)
		ERROR(dh, -1, ERR_SYS_FATAL, false, "strdup()");
	if ((dh->cmdqsz)++ > 0)
		return (0);
	if (send_string(dh, dh->sbuf) == -1)
		ERROR(dh, -1, 0, true, "send_string()");
	return (0);
}

static int
dsbmc_send(dsbmc_t *dh, const char *cmd, ...)
{
	char	*e, *p;
	va_list	ap;

	dsbmc_clearerr(dh);

	if (dh->cmdqsz > 0) {
		ERROR(dh, -1, DSBMC_ERR_COMMAND_IN_PROGRESS, false,
		    "dsbmc_send(): Command already in progress");
	}
	if (dh->sbufsz == 0) {
		if ((dh->sbuf = malloc(BUFSZ)) == NULL)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "malloc()");
		dh->sbufsz = BUFSZ;
	}
	for (;;) {
		va_start(ap, cmd);
		if ((size_t)vsnprintf(dh->sbuf, dh->sbufsz, cmd, ap) < dh->sbufsz)
			break;
		/* Send buffer too small. */
		if ((p = realloc(dh->sbuf, BUFSZ + dh->sbufsz)) == NULL)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "realloc()");
		dh->sbuf    = p;
		dh->sbufsz += BUFSZ;
	}
	if (send_string(dh, dh->sbuf) == -1)
		ERROR(dh, -1, 0, true, "send_string()");
	while ((e = read_event(dh, true)) != NULL) {
		if (parse_event(dh, e) == -1)
			ERROR(dh, -1, 0, true, "parse_event()");
		if (dh->event.type == DSBMC_EVENT_SUCCESS_MSG)
			return (0);
		else if (dh->event.type == DSBMC_EVENT_ERROR_MSG)
			return (dh->event.code);
		else if (push_event(dh, e) == -1)
			ERROR(dh, -1, 0, true, "push_event()");
	}
	ERROR(dh, -1, 0, true, "read_event()");
}

static int
send_string(dsbmc_t *dh, const char *str)
{
	fd_set wrset;

	FD_ZERO(&wrset); FD_SET(dh->socket, &wrset);
	while (select(dh->socket + 1, 0, &wrset, 0, 0) == -1) {
		if (errno != EINTR)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "select()");
	}
	while (write(dh->socket, str, strlen(str)) == -1) {
		if (errno != EINTR)
			ERROR(dh, -1, ERR_SYS_FATAL, false, "write()");
	}		
	return (0);
}

