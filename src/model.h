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
#include <QVariant>
#include <QList>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QStringList>
#include <QAbstractListModel>
#include <QSocketNotifier>
#include <QMessageBox>

#include "thread.h"
#include "libdsbmc.h"
#include "dsbcfg.h"
#include "config.h"

struct MenuAction {
	int action;
	int devid;
};

Q_DECLARE_METATYPE(MenuAction);

class Model : public QAbstractListModel
{
	Q_OBJECT
public:
	Model(dsbcfg_t *cfg, QObject *parent = nullptr);

	int	 rowCount(const QModelIndex &parent) const override;
	int	 rowCount(void) const;
	int	 querySize(const dsbmc_dev_t *, uint64_t *, uint64_t *);
	int	 mount(const dsbmc_dev_t *dev);
	int	 unmount(const dsbmc_dev_t *dev, bool force = false);
	int	 eject(const dsbmc_dev_t *dev, bool force = false);
	int	 open(const dsbmc_dev_t *dev, QString program);
	int	 play(const dsbmc_dev_t *dev, QString program);
	int	 speed(const dsbmc_dev_t *dev, int speed);
	void	 init(void);
	void	 setIconSize(QSize size);
	void	 redraw(void);
	QMenu	 *getContextMenu(int row);
	QString  errcodeToStr(int code);
	QString	 bytesToUnits(uint64_t bytes);
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role)
			    const override;
	const dsbmc_dev_t *devFromRow(int row);
	const dsbmc_dev_t *devFromId(int id);
signals:
	void	 noDevices(void);
	void	 deviceAdded(const dsbmc_dev_t *);
	void	 actionFinished(int action, const dsbmc_dev_t *, int code);
	void	 actionFinished(int action, const dsbmc_dev_t *, int code,
				QString program);
private slots:
	void	 fetchEvents(void);
	void	 checkCommandReturn(int command, const dsbmc_dev_t *dev,
				    int code);
	void	 checkCommandReturn(int command, const dsbmc_dev_t *dev,
				    int code, QString program);
private:
	int	 devRow(const dsbmc_dev_t *dev);
	int	 addDevice(const dsbmc_dev_t *, int row = -1);
	int	 execCommand(int command, const dsbmc_dev_t *dev, bool force, int speed);
	int	 execCommand(int command, const dsbmc_dev_t *dev,
			     QString program = "");
	bool	 hideDev(const dsbmc_dev_t *dev);
	void	 loadIcons(void);
	void	 removeDevice(const dsbmc_dev_t *);
	void	 updateDev(const dsbmc_dev_t *dev);
	void	 startThread(Thread *thr);
	void	 translateErrors(void);
	QIcon	 getDevIcon(int type);
	QAction	 *menuAction(const dsbmc_dev_t *dev, int command);

	struct DSBMCDev {
		QIcon icon;
		const dsbmc_dev_t *dp;
	};
	QIcon		icon_dir;
	QIcon		icon_cd;
	QIcon		icon_cdda;
	QIcon		icon_dvd;
	QIcon		icon_hdd;
	QIcon		icon_mmc;
	QIcon		icon_mtp; 
	QIcon		icon_ptp;
	QIcon		icon_vcd;
	QIcon		icon_svcd;
	QIcon		icon_usb;
	QIcon		icon_play;
	QIcon		icon_open;
	QIcon		icon_eject;
	QIcon		icon_mount;
	QIcon		icon_unmount;
	QSize		iconSize = QSize(48, 48);
	QMutex		*mutex;
	dsbmc_t		*dh;
	dsbcfg_t	*cfg;
	QStringList	errors;
	QList<DSBMCDev> devices;
	QSocketNotifier *swatcher;
};
