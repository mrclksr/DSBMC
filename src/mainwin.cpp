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

#include <QScreen>
#include <QDebug>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <errno.h>

#include "mainwin.h"
#include "qt-helper.h"

MainWin::MainWin(QWidget *parent) : QMainWindow(parent)
{
	quitIcon      = qh_loadIcon("application-exit", 0);
	prefsIcon     = qh_loadIcon("preferences-system", 0);
	statusLabel   = new QLabel(this);
	trayTimer     = new QTimer(this);
	list	      = new QListView(this);
	QIcon winIcon = qh_loadIcon("drive-removable-media-usb-pendrive",
				    "drive-removable-media-usb",
				    "drive-harddisk-usb",
				    "drive-removable-media",
				    "drive-harddisk", 0);
	cfg = dsbcfg_read(PROGRAM, "config", vardefs, CFG_NVARS);
	if (cfg == NULL && errno == ENOENT) {
		cfg = dsbcfg_new(NULL, vardefs, CFG_NVARS);
		if (cfg == NULL)
			qh_errx(0, EXIT_FAILURE, "%s", dsbcfg_strerror());
	} else if (cfg == NULL)
		qh_errx(0, EXIT_FAILURE, "%s", dsbcfg_strerror());
	posX	= &dsbcfg_getval(cfg, CFG_POS_X).integer;
	posY	= &dsbcfg_getval(cfg, CFG_POS_Y).integer;
	wWidth	= &dsbcfg_getval(cfg, CFG_WIDTH).integer;
	hHeight	= &dsbcfg_getval(cfg, CFG_HEIGHT).integer;
	model   = new Model(cfg, this);

	QItemSelectionModel *selections = new QItemSelectionModel(model);
	statusBar()->addPermanentWidget(statusLabel);
	
	list->setContentsMargins(15, 15, 15, 15);
	model->init();
	list->setModel(model);
	list->setSelectionModel(selections);
	list->setViewMode(QListView::IconMode);
	list->setResizeMode(QListView::Adjust);
	list->setUniformItemSizes(true);
	list->setSpacing(20);
	list->setWordWrap(true);
	list->setTextElideMode(Qt::ElideNone);
	list->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	setIconSize(QSize(48, 48));
	setCentralWidget(list);
	
	createMainMenu();

	qApp->installEventFilter(this);
	qApp->setQuitOnLastWindowClosed(false);
	
	setWindowIcon(winIcon);
	setWindowTitle(tr("DSBMC - DSBMD Client"));

	resize(*wWidth, *hHeight);      
	move(*posX, *posY);
	trayTimer->start(500);

	connect(QGuiApplication::primaryScreen(),
	    SIGNAL(geometryChanged(const QRect &)), this,
	    SLOT(scrGeomChanged(const QRect &)));
	connect(trayTimer, SIGNAL(timeout()), this, SLOT(checkForSysTray()));
	connect(list->selectionModel(),
	    SIGNAL(selectionChanged(const QItemSelection &,
	        const QItemSelection &)), this,
	    SLOT(catchSelectionChanged(const QItemSelection &,
	        const QItemSelection &)));
	connect(model, SIGNAL(actionFinished(int, const dsbmc_dev_t *, int)),
	    this, SLOT(checkReply(int, const dsbmc_dev_t *, int)));
	connect(model,
	    SIGNAL(actionFinished(int, const dsbmc_dev_t *, int, QString)),
	    this, SLOT(checkReply(int, const dsbmc_dev_t *, int, QString)));
	connect(model,
	    SIGNAL(deviceAdded(const dsbmc_dev_t *)), this,
	    SLOT(catchDeviceAdded(const dsbmc_dev_t *)));
}

void MainWin::checkForSysTray()
{
	static int tries = 60;

	if (trayIcon != 0) {
		tries = 60;
		delete trayIcon;
		trayIcon = 0;
	}
	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		trayTimer->stop();
		createTrayIcon();
	} else if (tries-- <= 0) {
		trayTimer->stop();
		show();
	}
}

void MainWin::loadTrayIconPic()
{
	char *fname = dsbcfg_getval(cfg, CFG_TRAY_ICON).string;

	if (fname != NULL && *fname != '\0')
		trayIconPic = QIcon(fname);
	if (trayIconPic.isNull() || fname == NULL || *fname == '\0') {
		trayIconPic = qh_loadIcon("drive-removable-media-usb-pendrive",
					  "drive-removable-media-usb-panel",
					  "drive-harddisk-usb-panel",
					  "drive-removable-media-panel",
					  "drive-harddisk-panel",
					  "drive-removable-media-usb",
					  "drive-harddisk-usb",
					  "drive-removable-media",
					  "drive-harddisk", 0);
	}
}

void MainWin::createTrayIcon()
{
	if (trayIcon != 0)
		return;
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;
	loadTrayIconPic();

	QMenu	*menu	     = new QMenu(this);
	QAction *quitAction  = new QAction(quitIcon, tr("&Quit"), this);
	QAction *prefsAction = new QAction(prefsIcon, tr("&Preferences"), this);
	
	connect(quitAction, &QAction::triggered, this, &MainWin::quit);
	connect(prefsAction, &QAction::triggered, this,
		&MainWin::showConfigMenu);

	menu->addAction(prefsAction);
	menu->addAction(quitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setIcon(trayIconPic);
	trayIcon->setContextMenu(menu);
	trayIcon->show();
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	    this, SLOT(trayClicked(QSystemTrayIcon::ActivationReason)));
	connect(trayIcon, &QSystemTrayIcon::messageClicked,
	    this, &MainWin::show);
}

void MainWin::trayClicked(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || 
	    reason == QSystemTrayIcon::DoubleClick) {
		if (isVisible())
			hide();
		else
			show();
	}
}

void MainWin::setIconSize(QSize size)
{
	list->setIconSize(size);
	model->setIconSize(size);
}

void MainWin::startBusyMessage(QString &msg)
{
	list->setEnabled(false);
	if (spinnerTimer == nullptr) {
		spinnerTimer = new QTimer(this);
		spinnerTimer->setInterval(400);
		connect(spinnerTimer, &QTimer::timeout, this,
		    &MainWin::updateBusyMessage);
	}
	spinnerTimer->start();
	spinnerMsg = msg;
	updateBusyMessage();
}

void MainWin::scrGeomChanged(const QRect &g)
{
	Q_UNUSED(g);
	trayTimer->start(500);
}

void MainWin::updateBusyMessage()
{
	spinnerCounter %= spinnerPhase.count();
	statusLabel->setText(QString("%1 %2").arg(spinnerMsg)
		.arg(spinnerPhase.at(spinnerCounter++)));
}

void MainWin::stopBusyMessage()
{
	if (spinnerTimer != nullptr)
		spinnerTimer->stop();
	statusLabel->setText("");
	list->setEnabled(true);
}

void MainWin::showDevAddedMsg(const char *devname)
{
	static int     secs = 10;
	static time_t  t0 = 0, t1;
	static QString msg;

	if (!trayIcon || !trayIcon->supportsMessages())
		return;
	QString str = QString(tr("Storage device %1 added")).arg(devname);

	t1 = time(NULL);
	if (t1 - t0 < secs)
		msg.append("\n").append(str);
	else
		msg = str;
	t0 = t1;
	trayIcon->showMessage(QString(tr("Storage device")), msg,
			      QSystemTrayIcon::Information, secs * 1000);
}

void MainWin::autoplay(const dsbmc_dev_t *dev)
{
	QString cmd = playCommand(dev);
	if (cmd == "")
		return;
	model->execCommand(DSBMC_CMD_PLAY, dev, playCommand(dev));
}

void MainWin::catchDeviceAdded(const dsbmc_dev_t *dev)
{
	if (dsbcfg_getval(cfg, CFG_MSGWIN).boolean)
		showDevAddedMsg(dev->volid);
	if (dsbcfg_getval(cfg, CFG_POPUP).boolean)
		show();
	if ((dev->cmds & DSBMC_CMD_PLAY))
		autoplay(dev);
}

void MainWin::checkReply(int action, const dsbmc_dev_t *dev, int code)
{
	stopBusyMessage();
	
	if (action == DSBMC_CMD_MOUNT || action == DSBMC_CMD_UNMOUNT) {
		if (code == 0) {
			showSize(dev);
			return;
		}
		if (action == DSBMC_CMD_MOUNT) {
			errWin(tr("Couldn't mount %1: %2")
				.arg(dev->dev).arg(model->errcodeToStr(code)));
		} else if (action == DSBMC_CMD_UNMOUNT) {
			errWin(tr("Couldn't unmount %1: %2")
				.arg(dev->dev).arg(model->errcodeToStr(code)));
		}
	} else if (action == DSBMC_CMD_EJECT && code != 0) {
		errWin(tr("Couldn't eject %1: %2")
			.arg(dev->dev).arg(model->errcodeToStr(code)));
	}
}

void MainWin::checkReply(int action, const dsbmc_dev_t *dev, int code,
	QString program)
{
	stopBusyMessage();
	if (code == 0) {
		showSize(dev);
		return;
	}
	if (action == DSBMC_CMD_OPEN || action == DSBMC_CMD_PLAY) {
		QString errmsg = QString(tr("Couldn't start %1: %2")
					.arg(program).arg(strerror(code)));
		errWin(errmsg);
	} else
		qDebug() << "Unexpected reply code " << code;
}

void MainWin::showContextMenu(QPoint pos)
{
	QModelIndex idx = list->indexAt(pos);
	if (!idx.isValid())
		return;
	if (!list->isEnabled())
		return;
	QMenu *menu = model->getContextMenu(idx.row());
	connect(menu, &QMenu::triggered, this, &MainWin::execAction);
	connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);
	menu->popup(list->viewport()->mapToGlobal(pos));
}

QString MainWin::playCommand(const dsbmc_dev_t *dev)
{
	switch (dev->type) {
	case DSBMC_DT_AUDIOCD:
		return (QString(dsbcfg_getval(cfg, CFG_PLAY_CDDA).string));
	case DSBMC_DT_DVD:
		return (QString(dsbcfg_getval(cfg, CFG_PLAY_DVD).string));
	case DSBMC_DT_VCD:
		return (QString(dsbcfg_getval(cfg, CFG_PLAY_VCD).string));
	case DSBMC_DT_SVCD:
		return (QString(dsbcfg_getval(cfg, CFG_PLAY_SVCD).string));
	}
	return (QString(""));
}

void MainWin::execAction(QAction *action)
{
	QString		   msg, devname;
	MenuAction	   ma  = action->data().value<MenuAction>();
	const dsbmc_dev_t *dev = model->devFromId(ma.devid);
	
	if (dev == NULL)
		return;
	devname = model->getDevname(dev);
	switch (ma.action) {
	case DSBMC_CMD_OPEN:
		if (model->execCommand(DSBMC_CMD_OPEN, dev,
		    QString(dsbcfg_getval(cfg, CFG_FILEMANAGER).string)) == -1)
			return;
		msg = QString(tr("Opening %1. Please wait")).arg(devname);
		if (dsbcfg_getval(cfg, CFG_HIDE_ON_OPEN).boolean)
			hide();
		break;
	case DSBMC_CMD_PLAY:
		if (model->execCommand(DSBMC_CMD_PLAY, dev,
		    playCommand(dev)) == -1)
			return;
		if (dsbcfg_getval(cfg, CFG_HIDE_ON_OPEN).boolean)
			hide();
		return;
	case DSBMC_CMD_MOUNT:
		if (model->execCommand(DSBMC_CMD_MOUNT, dev) == -1)
			return;
		msg = QString(tr("Mounting %1. Please wait")).arg(devname);
		break;
	case DSBMC_CMD_UNMOUNT:
		if (model->execCommand(DSBMC_CMD_UNMOUNT, dev) == -1)
			return;
		msg = QString(tr("Unmounting %1. Please wait")).arg(devname);
		break;
	case DSBMC_CMD_EJECT:
		if (model->execCommand(DSBMC_CMD_EJECT, dev) == -1)
			return;
		msg = QString(tr("Ejecting %1. Please wait")).arg(devname);
		break;
	default:
		return;
	}
	startBusyMessage(msg);
}

void MainWin::saveGeometry()
{
	if (isVisible()) {
		*posX = this->x(); *posY = this->y();
		*wWidth = this->width(); *hHeight = this->height();
		dsbcfg_write(PROGRAM, "config", cfg);
	}
}

void MainWin::resizeEvent(QResizeEvent *event)
{
	saveGeometry();
	event->accept();
}

void MainWin::moveEvent(QMoveEvent *event)
{
	saveGeometry();
	event->accept();
}

bool MainWin::eventFilter(QObject *obj, QEvent *event)
{
	
	if (obj != list->viewport())
		return QObject::eventFilter(obj, event);
	QMouseEvent *ev = static_cast<QMouseEvent *>(event);

	if (event->type() == QEvent::MouseButtonPress &&
	    (ev->buttons() & Qt::RightButton)) {
		showContextMenu(ev->pos());
	} else if (event->type() == QEvent::MouseButtonDblClick &&
	    (ev->buttons() & Qt::LeftButton)) {
		int row = list->indexAt(ev->pos()).row();
		const dsbmc_dev_t *dev = model->devFromRow(row);
		if (dev != NULL) {
			if (model->execCommand(DSBMC_CMD_OPEN, dev,
			    QString(dsbcfg_getval(cfg,
			    CFG_FILEMANAGER).string)) == -1)
				return QObject::eventFilter(obj, event);
			QString msg = QString(tr("Opening %1. Please wait"))
					     .arg(dev->dev);
			if (dsbcfg_getval(cfg, CFG_HIDE_ON_OPEN).boolean)
				hide();
			startBusyMessage(msg);
		}
        }
	return QObject::eventFilter(obj, event);
}

void MainWin::quit()
{
	saveGeometry();
	dsbcfg_write(PROGRAM, "config", cfg);
	QApplication::quit();
}

void MainWin::showConfigMenu()
{
	Preferences prefs(cfg);
	if (prefs.exec() == QDialog::Accepted) {
		loadTrayIconPic();
		trayIcon->setIcon(trayIconPic);
		model->redraw();
	}
}

void MainWin::createMainMenu()
{
	QAction *quitAction  = new QAction(quitIcon, tr("&Quit"), this);
	QAction *prefsAction = new QAction(prefsIcon, tr("&Preferences"), this);
	connect(quitAction, &QAction::triggered, this, &MainWin::quit);
	connect(prefsAction, &QAction::triggered, this,
	    &MainWin::showConfigMenu);
	mainMenu = menuBar()->addMenu(tr("&File"));
	mainMenu->addAction(prefsAction);
	mainMenu->addAction(quitAction);
}

QFrame *MainWin::mkLine()
{
	QFrame *line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	return (line);
}

void MainWin::errWin(QString message)
{
	QMessageBox msgBox(this);
	msgBox.setWindowModality(Qt::WindowModal);
	msgBox.setSizeGripEnabled(true);
	msgBox.setText(tr("An error occured\n"));
	msgBox.setWindowTitle(tr("Error"));
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowIcon(msgBox.iconPixmap());
	msgBox.setText(message);
	msgBox.addButton(tr("Ok"), QMessageBox::ActionRole);
	msgBox.exec();
}

void MainWin::showSize(const dsbmc_dev_t *dev)
{
	uint64_t mediasize, bytesfree;
	
	if (model->querySize(dev, &mediasize, &bytesfree))
		return;
	QString status = QString("%1 Capacity: %2 Free: %3")
	    .arg(model->getDevname(dev))
	    .arg(model->bytesToUnits(mediasize))
	    .arg(model->bytesToUnits(bytesfree));
	statusLabel->setText(status);
}

void MainWin::catchSelectionChanged(const QItemSelection &selected,
	const QItemSelection &deselected)
{
	Q_UNUSED(selected);
	Q_UNUSED(deselected);
	showSize(model->devFromRow(list->currentIndex().row()));
	return;
}

void MainWin::closeEvent(QCloseEvent *event)
{
	hide();
	event->ignore();
}
