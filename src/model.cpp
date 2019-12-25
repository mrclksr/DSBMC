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

#include <QLabel>
#include <QFontMetrics>
#include <QApplication>
#include <paths.h>

#include "model.h"
#include "qt-helper.h"


Model::Model(dsbcfg_t *cfg, QObject *parent) : QAbstractListModel(parent)
{
	this->cfg = cfg;
}

int Model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return (devices.count());
}

int Model::rowCount() const
{
	return (devices.count());
}

QVariant Model::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role == Qt::DisplayRole)
		return (QString(devices.at(index.row()).dp->volid));
	if (role == Qt::DecorationRole) {
		return (devices.at(index.row()).dp->mounted ? \
		    icon_dir : devices.at(index.row()).icon);
	}
	if (role == Qt::SizeHintRole) {
		QLabel l("M");
		QFontMetrics fm(l.font());

		return QSize(fm.horizontalAdvance("M") * 12,
		    2 * fm.height() + iconSize.height());
	}
	return QVariant();
}

QVariant Model::headerData(int section, Qt::Orientation orientation,
	int role) const
{
	Q_UNUSED(section);
	Q_UNUSED(orientation);
	Q_UNUSED(role);
	return QVariant();
}

void Model::setIconSize(QSize size)
{
	iconSize = size;
}

bool Model::hideDev(const dsbmc_dev_t *dev)
{
	bool hide;
	char **v;
	
	for (hide = false, v = dsbcfg_getval(cfg, CFG_HIDE).strings;
	    !hide && v != NULL && *v != NULL; v++) {
		if (strncmp(*v, _PATH_DEV, sizeof(_PATH_DEV) - 1) != 0) {
			char *name = dev->dev;
			if (strncmp(name, _PATH_DEV, sizeof(_PATH_DEV) - 1) == 0)
				name += sizeof(_PATH_DEV) - 1;
			if (strcmp(name, *v) == 0)
				return (true);
		}
		if (strcmp(dev->dev, *v) == 0)
			return (true);
		else if (dev->mounted && strcmp(dev->mntpt, *v) == 0)
			return (true);
		else if (strcmp(dev->volid, *v) == 0)
			return (true);
	}
	return (false);
}

void Model::redraw()
{
	const dsbmc_dev_t *dev;

	while (devices.count())
		removeDevice(devFromRow(0));
	for (int i = 0; (dev = dsbmc_next_dev(dh, &i, false)) != NULL;) {
		if (hideDev(dev))
			continue;
		addDevice(dev);
	}
}

void Model::init()
{
	const dsbmc_dev_t *dev;

	loadIcons();
	translateErrors();
	if ((dh = dsbmc_alloc_handle()) == NULL)
		qh_err(0, EXIT_FAILURE, "dsbmc_alloc_handle()");
	if (dsbmc_connect(dh) == -1)
		qh_errx(0, EXIT_FAILURE, "%s", dsbmc_errstr(dh));
	for (int i = 0; (dev = dsbmc_next_dev(dh, &i, false)) != NULL;) {
		if (hideDev(dev))
			continue;
		addDevice(dev);
	}
	mutex	 = new QMutex;
	swatcher = new QSocketNotifier(dsbmc_get_fd(dh),
				       QSocketNotifier::Read, this);
	connect(swatcher, &QSocketNotifier::activated, this,
	    &Model::fetchEvents);
}

int Model::devRow(const dsbmc_dev_t *dev)
{
	for (int row = 0; row < devices.count(); row++) {
		if (dev->id == devices.at(row).dp->id)
			return (row);
	}
	return (-1);
}

const dsbmc_dev_t *Model::devFromRow(int row)
{
	QModelIndex idx = this->index(row);
	if (!idx.isValid())
		return (NULL);
	return (devices.at(row).dp);
}

const dsbmc_dev_t *Model::devFromId(int id)
{
	for (int i = 0; i < devices.count(); i++) {
		if (devices.at(i).dp->id == id)
			return (devices.at(i).dp);
	}
	return (NULL);
}

int Model::addDevice(const dsbmc_dev_t *dp, int row)
{
	DSBMCDev dev;

	if (row < 0 || row > devices.count())
		row = devices.count();
	QModelIndex idx = this->index(row);
	dev.dp   = dp;
	dev.icon = getDevIcon(dp->type);
	beginInsertRows(idx, row, row);
	insertRows(row, 1);
	devices.insert(row, dev);
	endInsertRows();
	return (row);
}

void Model::removeDevice(const dsbmc_dev_t *dev)
{
	int row = devRow(dev);
	QModelIndex idx = this->index(row);
	if (!idx.isValid())
		return;
	beginRemoveRows(idx, row, row);
	removeRow(row);
	endRemoveRows();
	devices.removeAt(row);
	dsbmc_free_dev(dh, dev);
}

QIcon Model::getDevIcon(int type)
{
	switch (type) {
	case DSBMC_DT_HDD:
		return (icon_hdd);
	case DSBMC_DT_USBDISK:
		return (icon_usb);
	case DSBMC_DT_DATACD:
		return (icon_cd);
	case DSBMC_DT_AUDIOCD:
		return (icon_cdda);
	case DSBMC_DT_RAWCD:
		return (icon_cd);
	case DSBMC_DT_DVD:
		return (icon_dvd);
	case DSBMC_DT_VCD:
		return (icon_vcd);
	case DSBMC_DT_SVCD:
		return (icon_svcd);
	case DSBMC_DT_MMC:
		return (icon_mmc);
	case DSBMC_DT_MTP:
		return (icon_mtp);
	case DSBMC_DT_PTP:
		return (icon_ptp);
	}
	return (QIcon());
}

void Model::updateDev(const dsbmc_dev_t *dev)
{
	int row = devRow(dev);
	QModelIndex idx = this->index(row);
	if (!idx.isValid())
		return;
	emit dataChanged(idx, idx, {Qt::DisplayRole});
}

void Model::loadIcons()
{
	icon_dir     = qh_loadIcon("folder", 0);
	icon_cd      = qh_loadIcon("media-optical-cd", "drive-optical", 0);
	icon_cdda    = qh_loadIcon("media-optical-audio", "drive-optical", 0);
	icon_dvd     = qh_loadIcon("media-optical-dvd", "drive-optical", 0);
	icon_hdd     = qh_loadIcon("drive-harddisk", "harddrive", 0);
	icon_mmc     = qh_loadIcon("media-flash-sd-mmc", "media-flash", 0);
	icon_mtp     = qh_loadIcon("multimedia-player", "drive-harddisk-usb", 0);
	icon_ptp     = qh_loadIcon("camera-photo", "drive-harddisk-usb", 0);
	icon_vcd     = qh_loadIcon("media-optical-cd", "drive-optical", 0);
	icon_svcd    = qh_loadIcon("media-optical-cd", "drive-optical", 0);
	icon_usb     = qh_loadIcon("drive-removable-media-usb",
				   "drive-harddisk-usb",
				   "drive-removable-media",
				   "drive-harddisk", 0);
	icon_play    = qh_loadIcon("media-playback-start", 0);
	icon_open    = qh_loadIcon("document-open", 0);
	icon_mount   = qh_loadIcon("emblem-mounted", "go-up", 0);
	icon_unmount = qh_loadIcon("emblem-unmounted", "go-down", 0);
	icon_eject   = qh_loadIcon("media-eject", 0);
}

void Model::translateErrors()
{
	errors	<< tr("Device already mounted")
		<< tr("Permission denied")
		<< tr("Device not mounted")
		<< tr("Device busy")
		<< tr("No such device")
		<< tr("Device not ejectable")
		<< tr("Unknown command")
		<< tr("Unknown option")
		<< tr("Syntax error")
		<< tr("No media")
		<< tr("Unknown filesystem")
		<< tr("Unknown error")
		<< tr("Mount command failed")
		<< tr("Invalid argument")
		<< tr("Max. number of connections reached")
		<< tr("Command string too long")
		<< tr("Invalid command string")
		<< tr("Timeout")
		<< tr("Not a regular file");
}

QString Model::errcodeToStr(int code)
{
	return (tr(dsbmc_errcode_to_str(code)));
}

QAction *Model::menuAction(const dsbmc_dev_t *dev, int command)
{
	bool	enabled = true;
	QIcon	icon;
	QString	name;
	
	switch (command) {
	case DSBMC_CMD_OPEN:
		icon = icon_open;
		name = tr("Open");
		break;
	case DSBMC_CMD_PLAY:
		icon = icon_play;
		name = tr("Play");
		break;
	case DSBMC_CMD_MOUNT:
		if (dev->mounted)
			enabled = false;
		icon = icon_mount;
		name = tr("Mount");
		break;
	case DSBMC_CMD_UNMOUNT:
		if (!dev->mounted)
			enabled = false;
		icon = icon_unmount;
		name = tr("Unmount");
		break;
	case DSBMC_CMD_EJECT:
		icon = icon_eject;
		name = tr("Eject");
		break;
	case DSBMC_CMD_SPEED:
		icon = icon_cd;
		name = tr("Set reading speed");
	}
	QAction	   *action = new QAction(icon, name, this);
	MenuAction ma;

	action->setEnabled(enabled);
	ma.action = command;
	ma.devid  = dev->id;
	action->setData(QVariant::fromValue(ma));

	return (action);
}

QMenu *Model::getContextMenu(int row)
{
	QMenu *menu = new QMenu;
	const dsbmc_dev_t *dev = devices.at(row).dp;

	if (dev == NULL)
		return (menu);
	if ((dev->cmds & DSBMC_CMD_OPEN))
		menu->addAction(menuAction(dev, DSBMC_CMD_OPEN));
	if ((dev->cmds & DSBMC_CMD_PLAY))
		menu->addAction(menuAction(dev, DSBMC_CMD_PLAY));
	if ((dev->cmds & DSBMC_CMD_MOUNT))
		menu->addAction(menuAction(dev, DSBMC_CMD_MOUNT));
	if ((dev->cmds & DSBMC_CMD_UNMOUNT))
		menu->addAction(menuAction(dev, DSBMC_CMD_UNMOUNT));
	if ((dev->cmds & DSBMC_CMD_SPEED))
		menu->addAction(menuAction(dev, DSBMC_CMD_SPEED));
	if ((dev->cmds & DSBMC_CMD_EJECT))
		menu->addAction(menuAction(dev, DSBMC_CMD_EJECT));
	return (menu);
}

void Model::checkCommandReturn(int command, const dsbmc_dev_t *dev, int code)
{
	int row		= devRow(dev);
	QModelIndex idx	= this->index(row);

	mutex->unlock();

	emit actionFinished(command, dev, code);
	if (!idx.isValid())
		return;
	if (command == DSBMC_CMD_MOUNT || command == DSBMC_CMD_UNMOUNT) {
		if (code == 0)
			emit dataChanged(idx, idx, {Qt::DisplayRole});
	}
	fetchEvents();
}

void Model::checkCommandReturn(int command, const dsbmc_dev_t *dev, int code,
	QString program)
{
	int row		= devRow(dev);
	QModelIndex idx	= this->index(row);

	mutex->unlock();

	emit actionFinished(command, dev, code, program);
	if (!idx.isValid())
		return;
	if (command == DSBMC_CMD_MOUNT || command == DSBMC_CMD_UNMOUNT) {
		if (code == 0)
			emit dataChanged(idx, idx, {Qt::DisplayRole});
	}
	fetchEvents();
}

int Model::querySize(const dsbmc_dev_t *dev, uint64_t *mediasize, uint64_t *free)
{
	if (dev == NULL)
		return (-1);
	if (!mutex->try_lock())
		return (-1);
	if (dsbmc_size(dh, dev) != 0) {
		mutex->unlock();
		return (-1);
	}
	*mediasize = dev->mediasize; *free = dev->free; 
	mutex->unlock();

	return (0);
}

QString Model::bytesToUnits(uint64_t bytes)
{
	double	units;
	QString	str;

	if (bytes >= (1 << 30)) {
		units = bytes / (double)(1 << 30);
		str = QString("%1 GB").arg(str.setNum(units, 'f', 2));
	} else if (bytes >= (1 << 20)) {
		units = bytes / (double)(1 << 20);
		str = QString("%1 MB").arg(str.setNum(units, 'f', 2));
	} else if (bytes >= (1 << 10)) {
		units = bytes / (double)(1 << 10);
		str = QString("%1 KB").arg(str.setNum(units, 'f', 2));
	} else {
		str = QString("%1 Bytes").arg(bytes);
	}
	return (str);
}

void Model::fetchEvents()
{
	QList<int>	  devids;
	dsbmc_event_t	  e;
	const dsbmc_dev_t *dev;
	
	if (!mutex->try_lock())
		return;
	while (dsbmc_fetch_event(dh, &e) > 0) {
		switch (e.type) {
		case DSBMC_EVENT_ADD_DEVICE:
			if (!hideDev(e.dev)) {
				addDevice(e.dev);
				devids.append(e.dev->id);
			}
			break;
		case DSBMC_EVENT_DEL_DEVICE:
			if (e.dev != NULL)
				removeDevice(e.dev);
			break;
		case DSBMC_EVENT_MOUNT:
		case DSBMC_EVENT_UNMOUNT:
			updateDev(e.dev);
			break;
		case DSBMC_EVENT_SHUTDOWN:
			emit dsbmdShutdown();
			break;
		}
	}
	if ((dsbmc_get_err(dh, NULL) & DSBMC_ERR_LOST_CONNECTION))
		emit dsbmdLostConnection();
	mutex->unlock();
	for (int i = 0; i < devids.count(); i++) {
		if ((dev = devFromId(devids.at(i))) != NULL)
			emit deviceAdded(dev);
	}
	if (devices.count() == 0)
		emit noDevices();
}

void Model::startThread(Thread *thr)
{
	connect(thr, &QThread::finished, thr, &QObject::deleteLater);
	connect(thr, SIGNAL(commandReturned(int, const dsbmc_dev_t *, int)),
		this,
		SLOT(checkCommandReturn(int, const dsbmc_dev_t *, int)));
	connect(thr, SIGNAL(commandReturned(int, const dsbmc_dev_t *, int, QString)),
		this,
		SLOT(checkCommandReturn(int, const dsbmc_dev_t *, int, QString)));
	thr->start();
}

int Model::mount(const dsbmc_dev_t *dev)
{
	return (execCommand(DSBMC_CMD_MOUNT, dev, false, -1));
}

int Model::unmount(const dsbmc_dev_t *dev, bool force)
{
	return (execCommand(DSBMC_CMD_UNMOUNT, dev, force, -1));
}

int Model::eject(const dsbmc_dev_t *dev, bool force)
{
	return (execCommand(DSBMC_CMD_EJECT, dev, force, -1));
}

int Model::open(const dsbmc_dev_t *dev, QString program)
{
	return (execCommand(DSBMC_CMD_OPEN, dev, program));
}

int Model::play(const dsbmc_dev_t *dev, QString program)
{
	return (execCommand(DSBMC_CMD_PLAY, dev, program));
}

int Model::speed(const dsbmc_dev_t *dev, int speed)
{
	return (execCommand(DSBMC_CMD_SPEED, dev, false, speed));
}

int Model::execCommand(int command, const dsbmc_dev_t *dev, QString program)
{
	Thread *thr;

	if (dev == NULL)
		return (-1);
	if (!mutex->try_lock()) {
		// A command is already running
		return (-1);
	}
	if (command == DSBMC_CMD_OPEN || command == DSBMC_CMD_PLAY)
		thr = new Thread(mutex, command, dh, dev, program);
	else
		thr = new Thread(mutex, command, dh, dev);
	startThread(thr);

	return (0);
}

int Model::execCommand(int command, const dsbmc_dev_t *dev, bool force, int speed)
{
	Thread *thr;

	if (dev == NULL)
		return (-1);
	if (!mutex->try_lock()) {
		// A command is already running
		return (-1);
	}
	thr = new Thread(mutex, command, dh, dev, force, speed);
	startThread(thr);

	return (0);
}
