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

#include <QProcess>
#include <QDebug>
#include <errno.h>

#include "thread.h"

void
Thread::run()
{
	int	 code;
	QProcess proc;
	
	mutex->lock();
	switch (command) {
	case DSBMC_CMD_OPEN:
		if (!dev->mounted) {
			if ((code = dsbmc_mount(handle, dev)) != 0)
				break;
		}
	case DSBMC_CMD_PLAY:
		program.replace("%d", dev->dev);
		program.replace("%m", dev->mntpt != NULL ? dev->mntpt : "");
		mutex->unlock();
		proc.start(QString(program));
		(void)proc.waitForStarted(-1);
		if (proc.state() == QProcess::NotRunning)
			code = errno;
		else
			code = 0;
		emit commandReturned(command, dev, code, program);
		proc.waitForFinished(-1);
		quit();
		return;
	case DSBMC_CMD_MOUNT:
		code = dsbmc_mount(handle, dev);
		break;
	case DSBMC_CMD_UNMOUNT:
		code = dsbmc_unmount(handle, dev, force);
		break;
	case DSBMC_CMD_EJECT:
		code = dsbmc_eject(handle, dev, force);
		break;
	case DSBMC_CMD_SPEED:
		code = dsbmc_set_speed(handle, dev, speed);
		break;
	}
	mutex->unlock();
	emit commandReturned(command, dev, code);
	quit();
}

Thread::Thread(QMutex *mutex, int command, dsbmc_t *handle,
	const dsbmc_dev_t *dev, bool force, int speed, QObject *parent)
	: QThread(parent)
{
	this->command = command;
	this->handle  = handle;
	this->dev     = dev;
	this->force   = force;
	this->speed   = speed;
	this->mutex   = mutex;
}

Thread::Thread(QMutex *mutex, int command, dsbmc_t *handle,
	const dsbmc_dev_t *dev, QString program, QObject *parent)
	: QThread(parent)
{
	this->command = command;
	this->handle  = handle;
	this->dev     = dev;
	this->program = program;
	this->mutex   = mutex;
}
