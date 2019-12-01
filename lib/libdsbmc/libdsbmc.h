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

#ifndef _LIB_DSBMC_H_
#define _LIB_DSBMC_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>

#define DSBMC_ERR_SYS			(1 << 10)
#define DSBMC_ERR_FATAL			(1 << 11)
#define DSBMC_ERR_LOST_CONNECTION	(1 << 12)
#define DSBMC_ERR_INVALID_DEVICE	(1 << 13)
#define DSBMC_ERR_CMDQ_BUSY		(1 << 14)
#define DSBMC_ERR_COMMAND_IN_PROGRESS	(1 << 15)
#define DSBMC_ERR_CONN_DENIED		(1 << 16)

#define DSBMC_ERR_ALREADY_MOUNTED	((1 << 8) + 0x01)
#define DSBMC_ERR_PERMISSION_DENIED	((1 << 8) + 0x02)
#define DSBMC_ERR_NOT_MOUNTED		((1 << 8) + 0x03)
#define DSBMC_ERR_DEVICE_BUSY		((1 << 8) + 0x04)
#define DSBMC_ERR_NO_SUCH_DEVICE	((1 << 8) + 0x05)
#define DSBMC_ERR_MAX_CONN_REACHED	((1 << 8) + 0x06)
#define DSBMC_ERR_NOT_EJECTABLE		((1 << 8) + 0x07)
#define DSBMC_ERR_UNKNOWN_COMMAND	((1 << 8) + 0x08)
#define DSBMC_ERR_UNKNOWN_OPTION	((1 << 8) + 0x09)
#define DSBMC_ERR_SYNTAX_ERROR		((1 << 8) + 0x0a)
#define DSBMC_ERR_NO_MEDIA		((1 << 8) + 0x0b)
#define DSBMC_ERR_UNKNOWN_FILESYSTEM	((1 << 8) + 0x0c)
#define DSBMC_ERR_UNKNOWN_ERROR		((1 << 8) + 0x0d)
#define DSBMC_ERR_MNTCMD_FAILED		((1 << 8) + 0x0e)
#define DSBMC_ERR_INVALID_ARGUMENT	((1 << 8) + 0x0f)
#define DSBMC_ERR_STRING_TOO_LONG	((1 << 8) + 0x10)
#define DSBMC_ERR_BAD_STRING		((1 << 8) + 0x11)
#define DSBMC_ERR_TIMEOUT		((1 << 8) + 0x12)
#define DSBMC_ERR_NOT_A_FILE		((1 << 8) + 0x13)

#define DSBMC_EVENT_SUCCESS_MSG		'O'
#define DSBMC_EVENT_WARNING_MSG		'W'
#define DSBMC_EVENT_ERROR_MSG		'E'
#define DSBMC_EVENT_INFO_MSG		'I'
#define DSBMC_EVENT_ADD_DEVICE		'+'
#define DSBMC_EVENT_DEL_DEVICE		'-'
#define DSBMC_EVENT_END_OF_LIST		'='
#define DSBMC_EVENT_MOUNT		'M'
#define DSBMC_EVENT_UNMOUNT		'U'
#define DSBMC_EVENT_SPEED		'V'
#define DSBMC_EVENT_SHUTDOWN		'S'

typedef struct dsbmc_dev_s {
	uint8_t cmds;			/* Supported commands. */
#define DSBMC_CMD_MOUNT			(1 << 0x00)
#define DSBMC_CMD_UNMOUNT		(1 << 0x01)
#define DSBMC_CMD_EJECT			(1 << 0x02)
#define DSBMC_CMD_PLAY			(1 << 0x03)
#define DSBMC_CMD_OPEN			(1 << 0x04)
#define DSBMC_CMD_SPEED			(1 << 0x05)
#define DSBMC_CMD_SIZE			(1 << 0x06)
#define DSBMC_CMD_MDATTACH		(1 << 0x07)
	uint8_t type;
#define DSBMC_DT_HDD			0x01
#define DSBMC_DT_USBDISK		0x02
#define DSBMC_DT_DATACD			0x03
#define	DSBMC_DT_AUDIOCD		0x04
#define	DSBMC_DT_RAWCD			0x05
#define	DSBMC_DT_DVD			0x06
#define	DSBMC_DT_VCD			0x07
#define	DSBMC_DT_SVCD			0x08
#define	DSBMC_DT_FLOPPY			0x09
#define DSBMC_DT_MMC			0x0a
#define DSBMC_DT_MTP			0x0b
#define DSBMC_DT_PTP			0x0c
	int	 id;
	char	 *dev;			/* Device name */
	char	 *volid;		/* Volume ID */
	char	 *mntpt;		/* Mount point */
	char	 *fsname;		/* Filesystem name */
	bool	 mounted;		/* Whether drive is mounted. */
	bool	 removed;		/* Whether device was removed */
	uint8_t	 speed;			/* CD/DVD reading speed */
	uint64_t mediasize;		/* For "size" command. */
	uint64_t free;			/* 	 ""	       */
	uint64_t used;			/* 	 ""	       */
} dsbmc_dev_t;

typedef struct dsbmc_event_s {
	int type;			/* Event type */
	int code;			/* Error code */
	dsbmc_dev_t *dev;		/* Pointer into dev list */
} dsbmc_event_t;

typedef struct dsbmc_sender_s {
	int	     id;	/* DSBMC_CMD_.. */
	int	     retcode;	/* Reply code from DSBMD */
	char	    *cmd;	/* Command string */
	dsbmc_dev_t *dev;
	void (*callback)(int retcode, const dsbmc_dev_t *dev);
} dsbmc_sender_t;

typedef struct dsbmc_eventq_s {
	size_t n;		/* # of events in queue */
	size_t i;		/* Current index */
#define DSBMC_MAXEQSZ 64
	char *ln[DSBMC_MAXEQSZ];
} dsbmc_eventq_t;

typedef struct dsbmd_event_s {
	char	    type;	/* Event type. */
	char	    *command;	/* In case of a reply, the executed command. */
	int	    mntcmderr;	/* Return code of external mount command. */
	int	    code;	/* The error code */
	uint64_t    mediasize;	/* For "size" command. */
	uint64_t    free;	/* 	 ""	       */
	uint64_t    used;	/* 	 ""	       */
	dsbmc_dev_t devinfo;	/* For Add/delete/mount/unmount message. */
} dsbmd_event_t;

typedef struct dsbmc_s {
	int	       socket;
	int	       error;
	char	       *lbuf;	/* Buffer for reading lines from socket */
	char	       *pbuf;	/* Parser buffer */
	char	       *sbuf;	/* Send buffer */
	char	       errormsg[_POSIX2_LINE_MAX];
	size_t	       rd;
	size_t	       lbufsz;
	size_t	       pbufsz;
	size_t	       sbufsz;
	size_t	       slen;
	size_t	       ndevs;
	size_t	       cmdqsz;
	dsbmc_dev_t    **devs;
	dsbmd_event_t  event;
	dsbmc_eventq_t evq;
#define DSBMC_CMDQMAXSZ 32
	dsbmc_sender_t sender[DSBMC_CMDQMAXSZ];
} dsbmc_t;

extern int  dsbmc_fetch_event(dsbmc_t *, dsbmc_event_t *ev);
extern int  dsbmc_get_devlist(dsbmc_t *, const dsbmc_dev_t ***);
extern int  dsbmc_mount(dsbmc_t *, const dsbmc_dev_t *d);
extern int  dsbmc_unmount(dsbmc_t *, const dsbmc_dev_t *d, bool force);
extern int  dsbmc_eject(dsbmc_t *, const dsbmc_dev_t *d, bool force);
extern int  dsbmc_set_speed(dsbmc_t *, const dsbmc_dev_t *d, int speed);
extern int  dsbmc_size(dsbmc_t *, const dsbmc_dev_t *d);
extern int  dsbmc_mdattach(dsbmc_t *, const char *);
extern int  dsbmc_set_speed_async(dsbmc_t *, const dsbmc_dev_t *d, int speed,
		void (*cb)(int, const dsbmc_dev_t *));
extern int  dsbmc_mount_async(dsbmc_t *, const dsbmc_dev_t *d,
		void (*cb)(int, const dsbmc_dev_t *));
extern int  dsbmc_unmount_async(dsbmc_t *, const dsbmc_dev_t *d, bool force,
		void (*cb)(int, const dsbmc_dev_t *));
extern int  dsbmc_eject_async(dsbmc_t *, const dsbmc_dev_t *d, bool force,
		void (*cb)(int, const dsbmc_dev_t *));
extern int  dsbmc_size_async(dsbmc_t *, const dsbmc_dev_t *d,
		void (*cb)(int, const dsbmc_dev_t *));
extern int  dsbmc_mdattach_async(dsbmc_t *, const char *,
		void (*cb)(int, const dsbmc_dev_t *));
extern int  dsbmc_connect(dsbmc_t *);
extern int  dsbmc_get_fd(dsbmc_t *);
extern int  dsbmc_get_err(dsbmc_t *, const char **);
extern void dsbmc_free_handle(dsbmc_t *);
extern void dsbmc_disconnect(dsbmc_t *);
extern void dsbmc_free_dev(dsbmc_t *, const dsbmc_dev_t *);
extern const char  *dsbmc_errstr(dsbmc_t *);
extern const char  *dsbmc_errcode_to_str(int);
extern dsbmc_t	   *dsbmc_alloc_handle(void);
extern dsbmc_dev_t *dsbmc_dev_from_id(dsbmc_t *, int);
extern dsbmc_dev_t *dsbmc_dev_from_name(dsbmc_t *, const char *);
extern dsbmc_dev_t *dsbmc_next_dev(dsbmc_t *, int *, bool);
#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* !_LIB_DSBMC_H_ */

