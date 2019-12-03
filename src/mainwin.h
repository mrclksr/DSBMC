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

#pragma once
#include <QApplication>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMainWindow>
#include <QItemSelection>
#include <QListView>
#include <QTimer>
#include <QStringList>
#include <QSystemTrayIcon>

#include "preferences.h"
#include "model.h"


class MainWin : public QMainWindow {
        Q_OBJECT
public:
	MainWin(QWidget *parent = 0);
	void	 closeEvent(QCloseEvent *event);
	QMenu	 *menu();
protected:
	bool	 eventFilter(QObject *obj, QEvent *event);
public slots:
	void	 catchSelectionChanged(const QItemSelection &selected,
			const QItemSelection &deselected);
	void	 showConfigMenu(void);
	void	 showContextMenu(QPoint pos);
	void	 quit(void);
	void	 resizeEvent(QResizeEvent *event);
	void	 moveEvent(QMoveEvent *event);
	void	 checkReply(int action, const dsbmc_dev_t *dev, int code);
	void	 checkReply(int action, const dsbmc_dev_t *dev, int code,
		     QString program);
	void	 execAction(QAction *action);
private:
	int	 forceEjectWin(const char *dev);
	void	 errWin(QString message);
	void	 createMenuActions(void);
	void	 createMainMenu(void);
	void	 showSize(const dsbmc_dev_t *dev);
	void	 setStatus(QString &msg);
	void	 startBusyMessage(QString &msg);
	void	 stopBusyMessage(void);
	void	 setIconSize(QSize size);
	void	 saveGeometry(void);
	void	 createTrayIcon(void);
	void	 loadTrayIconPic(void);
	QString	 playCommand(const dsbmc_dev_t *dev);
private slots:
	void	 showDevAddedMsg(const char *devname);
	void	 catchDeviceAdded(const dsbmc_dev_t *dev);
	void	 checkForSysTray(void);
	void	 updateBusyMessage(void);
	void	 autoplay(const dsbmc_dev_t *dev);
	void	 scrGeomChanged(const QRect &g);
	void	 trayClicked(QSystemTrayIcon::ActivationReason reason);
private:
	int		spinnerCounter = 0;
        int		*posX;
        int		*posY;
        int		*wWidth;
        int		*hHeight;
	QIcon		quitIcon;
	QIcon		prefsIcon; 
	QIcon		trayIconPic;
	Model		*model;
	QMenu		*mainMenu;
	QLabel		*statusLabel;
	QTimer		*trayTimer;
	QTimer		*spinnerTimer = nullptr;
	QString		spinnerMsg;
	dsbcfg_t	*cfg;
	QListView	*list;
	QMessageBox	*msgBox;
	QStringList	spinnerPhase = { "   ", ".  ", ".. ", "..." };
	QSystemTrayIcon *trayIcon = 0;
};
