/*-
 * Copyright (c) 2019 Marcel Kaiser. All rights reserved.
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

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>

#include "mainwin.h"
#include "qt-helper/qt-helper.h"

#define PATH_LOCK ".dsbmc.lock"

static char path_lock[PATH_MAX] = { 0 };

void
usage()
{
	(void)printf("Usage: %s [-ih] [<disk image> ...]\n" \
		     "   -i: Start %s as tray icon\n", PROGRAM, PROGRAM);
	exit(EXIT_FAILURE);
}

void
create_mddev(int npaths, char **paths)
{
	static dsbmc_t *dh = NULL;

	if (dh == NULL) {
		if ((dh = dsbmc_alloc_handle()) == NULL)
			qh_err(0, EXIT_FAILURE, "dsbmc_alloc_handle()");
		if (dsbmc_connect(dh) == -1)
			qh_errx(0, EXIT_FAILURE, "%s", dsbmc_errstr(dh));
	}
	while (npaths-- > 0) {
		if (dsbmc_mdattach(dh, *paths) == -1) {
			qh_warnx(0, "dsbmc_mdattach(%s): %s",
			    *paths, dsbmc_errstr(dh));
		}
		paths++;
	}
	dsbmc_disconnect(dh);
	dsbmc_free_handle(dh);
	dh = NULL;
}

static void
cleanup(int /* unused */)
{
	static sig_atomic_t block = 0;

	if (block == 1)
		return;
	block = 1;
	(void)unlink(path_lock);
	exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[])
{
	int	      ch, lockfd, error;
	bool	      iflag;
	struct passwd *pw;

	iflag = false;
	while ((ch = getopt(argc, argv, "ih")) != -1) {
		switch (ch) {
		case 'i':
			/* Start as tray icon. */
			iflag = true;
                        break;
		case '?':
		case 'h':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	QApplication app(argc, argv);
	QTranslator translator;

	if (translator.load(QLocale(), QLatin1String(PROGRAM),
	    QLatin1String("_"), QLatin1String(LOCALE_PATH)))
		app.installTranslator(&translator);

	if ((pw = getpwuid(getuid())) == NULL)
		qh_errx(0, EXIT_FAILURE, "getpwuid()");
	/* Check if another instance is already running. */
	(void)snprintf(path_lock, sizeof(path_lock), "%s/%s", pw->pw_dir,
	    PATH_LOCK);
	endpwent();
	if ((lockfd = open(path_lock, O_WRONLY | O_CREAT, 0600)) == -1)
		qh_errx(0, EXIT_FAILURE, "open(%s)", path_lock);
	if (flock(lockfd, LOCK_EX | LOCK_NB) == -1) {
		if (errno == EWOULDBLOCK) {
			(void)create_mddev(argc, argv);
			exit(EXIT_SUCCESS);
		}
		qh_errx(0, EXIT_FAILURE, "flock(%s)", path_lock);
	}
	(void)create_mddev(argc, argv);
	(void)signal(SIGINT, cleanup);
	(void)signal(SIGTERM, cleanup);
	(void)signal(SIGQUIT, cleanup);
	(void)signal(SIGHUP, cleanup);

	MainWin win;
	if (!iflag)
		win.show();
	error = app.exec();
	cleanup(0);
	return (error);
}
