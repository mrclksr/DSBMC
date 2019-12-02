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

#pragma once
#include <QDialog>
#include <QLabel>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QTableWidget>

#include "lib/config.h"

class Preferences : public QDialog {
	Q_OBJECT
public:
	Preferences(dsbcfg_t *cfg, QWidget *parent = 0);
public slots:
	void	acceptSlot();
private:
	void	storeList(QString str);
	QString	quoteString(char *str);
	QWidget *commandsTab(void);
	QWidget *generalSettingsTab(void);
	QFrame	*mkLine(void);
private slots:
	void	openIcon(void);
private:
	dsbcfg_t  *cfg;
	QCheckBox *cdda_autoplay;
	QCheckBox *svcd_autoplay;
	QCheckBox *vcd_autoplay;
	QCheckBox *dvd_autoplay;
	QCheckBox *hideOnOpen;
	QCheckBox *notify;
	QCheckBox *popup;
	QLineEdit *fm_edit;
	QLineEdit *dvd_edit;
	QLineEdit *vcd_edit;
	QLineEdit *cdda_edit;
	QLineEdit *svcd_edit;
	QLineEdit *ignore_edit;
	QLineEdit *icon_edit;
};
