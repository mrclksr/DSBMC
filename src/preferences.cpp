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

#include "preferences.h"
#include "qt-helper/qt-helper.h"

Preferences::Preferences(dsbcfg_t *cfg, QWidget *parent) : 
    QDialog(parent) {
	this->cfg	    = cfg;
	QIcon winIcon	    = qh_loadIcon("preferences-system", 0);
	QIcon okIcon	    = qh_loadStockIcon(QStyle::SP_DialogOkButton, 0);
	QIcon cancelIcon    = qh_loadStockIcon(QStyle::SP_DialogCancelButton,
	    NULL);
	QPushButton *ok	    = new QPushButton(okIcon, tr("&Ok"));
	QPushButton *cancel = new QPushButton(cancelIcon, tr("&Cancel"));
	QVBoxLayout *layout = new QVBoxLayout(this);
	QHBoxLayout *bbox   = new QHBoxLayout;
	QFormLayout *form   = new QFormLayout;
	QString toolTip     = tr("Use %d and %m as placeholders for the "    \
				 "device and mount point, respectively");
	setWindowIcon(winIcon);
	setWindowTitle(tr("Preferences"));

	form->setVerticalSpacing(0);
	
	fm_edit   = new QLineEdit(QString(dsbcfg_getval(cfg,
				CFG_FILEMANAGER).string));
	dvd_edit  = new QLineEdit(QString(dsbcfg_getval(cfg,
				CFG_PLAY_DVD).string));
	vcd_edit  = new QLineEdit(QString(dsbcfg_getval(cfg,
				CFG_PLAY_VCD).string));
	svcd_edit = new QLineEdit(QString(dsbcfg_getval(cfg,
				CFG_PLAY_SVCD).string));
	cdda_edit = new QLineEdit(QString(dsbcfg_getval(cfg,
				CFG_PLAY_CDDA).string));
	ignore_edit = new QLineEdit;
	ignore_edit->setToolTip(tr("Comma-separated list of devices, mount " \
				   "points, and volume IDs to ignore.\n"     \
				   "Example: /dev/da0s1, EFISYS, "	     \
				   "/var/run/user/1001/gvfs"));
	fm_edit->setToolTip(toolTip);
	dvd_edit->setToolTip(toolTip);
	vcd_edit->setToolTip(toolTip);
	cdda_edit->setToolTip(toolTip);
	svcd_edit->setToolTip(toolTip);
	
	QString ignoreList;
	for (char **v = dsbcfg_getval(cfg, CFG_HIDE).strings;
	    v != NULL && *v != NULL; v++) {
		if (ignoreList != "")
			ignoreList.append(", ");
		ignoreList.append(quoteString(*v));
	}
	ignore_edit->setText(ignoreList);
	
	QWidget *container = new QWidget;
	QHBoxLayout *hbox  = new QHBoxLayout(container);
	hbox->addWidget(fm_edit);
	form->addRow(tr("Filemanager:"), container);

	container	   = new QWidget;
	hbox		   = new QHBoxLayout(container);
	dvd_autoplay       = new QCheckBox(tr("Autoplay"));
	dvd_autoplay->setCheckState(dsbcfg_getval(cfg,
		CFG_DVD_AUTO).boolean ? Qt::Checked : Qt::Unchecked);
	hbox->addWidget(dvd_edit);
	hbox->addWidget(dvd_autoplay);
	form->addRow(tr("Play DVDs with:"), container);
	
	container	    = new QWidget;
	hbox		    = new QHBoxLayout(container);
	cdda_autoplay       = new QCheckBox(tr("Autoplay"));
	cdda_autoplay->setCheckState(dsbcfg_getval(cfg,
		CFG_CDDA_AUTO).boolean ? Qt::Checked : Qt::Unchecked);
	hbox->addWidget(cdda_edit);
	hbox->addWidget(cdda_autoplay);
	form->addRow(tr("Play Audio CDs with:"), container);

	container	    = new QWidget;
	hbox		    = new QHBoxLayout(container);
	vcd_autoplay        = new QCheckBox(tr("Autoplay"));
	vcd_autoplay->setCheckState(dsbcfg_getval(cfg,
		CFG_VCD_AUTO).boolean ? Qt::Checked : Qt::Unchecked);
	hbox->addWidget(vcd_edit);
	hbox->addWidget(vcd_autoplay);
	form->addRow(tr("Play VCDs with:"), container);

	container	    = new QWidget;
	hbox		    = new QHBoxLayout(container);
	svcd_autoplay       = new QCheckBox(tr("Autoplay"));
	svcd_autoplay->setCheckState(dsbcfg_getval(cfg,
		CFG_SVCD_AUTO).boolean ? Qt::Checked : Qt::Unchecked);
	hbox->addWidget(svcd_edit);
	hbox->addWidget(svcd_autoplay);
	form->addRow(tr("Play SVCDs with:"), container);

	container = new QWidget;
	hbox      = new QHBoxLayout(container);
	hbox->addWidget(ignore_edit);
	form->addRow(tr("Ignore:"), container);

	bbox->addWidget(ok, 1, Qt::AlignRight);
	bbox->addWidget(cancel, 0, Qt::AlignRight);

	layout->addLayout(form);
	layout->addLayout(bbox);

	connect(ok, SIGNAL(clicked()), this, SLOT(acceptSlot()));
	connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void Preferences::storeList(QString str)
{
	int	     i, j;
	bool	     esc, quote;
	QString	     buf;
	QStringList  list;
	dsbcfg_val_t val;
	
	quote = esc = false;
	for (i = 0, j = 0; i < str.size(); i++) {
		if (str[i] == '"') {
			if (!esc && !quote) {
				quote = true;
				continue;
			}
			if (esc) {
				buf[j++] = '"';
				esc = false;
			} else
				quote = false;
		} else if (str[i] == '\\') {
			if (!esc)
				esc = true;
			else {
				buf[j++] = '\\';
				esc = false;
			}
		} else if (str[i] == ',' || str[i] == ' ') {
			if (!esc && !quote) {
				if (str[i] == ',') {
					buf.resize(j); j = 0;
					list.append(buf);
				}
				continue;
			}
			buf[j++] = str[i];
			if (esc)
				esc = false;
		} else {
			buf[j++] = str[i];
			buf.resize(j);
		}
	}
	if (j > 0) {
		buf.resize(j);
		list.append(buf);
	}
	if (list.count() == 0)
		val.strings = NULL;
	else {
		val.strings = (char **)malloc((list.count() + 1) * sizeof (char *));
		if (val.strings == NULL)
			qh_err(this, 1, "malloc()");
		for (int i = 0; i < list.count(); i++) {
			val.strings[i] = strdup(list.at(i).toLatin1().data());
			if (val.strings[i] == NULL)
				qh_err(this, 1, "strdup()");
		}
		val.strings[list.count()] = NULL;
	}
	dsbcfg_setval(cfg, CFG_HIDE, val);
}

QString Preferences::quoteString(char *str)
{
	int	i;
	QString	buf = "\"";

	for (i = 1; str != NULL && *str != '\0'; str++) {
		if (*str == '"' || *str == '\\')
			buf[i++] = '\\'; buf[i++] = *str;
	}
	buf[i++] = '"'; buf.resize(i);

	return (buf);
}


void Preferences::acceptSlot()
{
	dsbcfg_val_t val;

	val.string = fm_edit->text().toLatin1().data();
	dsbcfg_setval(cfg, CFG_FILEMANAGER, val);

	val.string = dvd_edit->text().toLatin1().data();
	dsbcfg_setval(cfg, CFG_PLAY_DVD, val);

	val.string = cdda_edit->text().toLatin1().data();
	dsbcfg_setval(cfg, CFG_PLAY_CDDA, val);
	
	val.string = vcd_edit->text().toLatin1().data();
	dsbcfg_setval(cfg, CFG_PLAY_VCD, val);
	
	val.string = svcd_edit->text().toLatin1().data();
	dsbcfg_setval(cfg, CFG_PLAY_SVCD, val);
	
	val.boolean = dvd_autoplay->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_DVD_AUTO, val);

	val.boolean = cdda_autoplay->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_CDDA_AUTO, val);

	val.boolean = vcd_autoplay->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_VCD_AUTO, val);

	val.boolean = svcd_autoplay->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_SVCD_AUTO, val);

	storeList(ignore_edit->text());
	
	dsbcfg_write(PROGRAM, "config", cfg);
	accept();
}
