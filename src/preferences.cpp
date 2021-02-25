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

#include <QGridLayout>
#include <QFileDialog>
#include <QDirIterator>
#include <QGroupBox>

#include "preferences.h"
#include "qt-helper/qt-helper.h"

Preferences::Preferences(dsbcfg_t *cfg, QWidget *parent) : 
    QDialog(parent) {
	this->cfg	    = cfg;
	QIcon winIcon	    = qh_loadIcon("preferences-system", 0);
	QIcon okIcon	    = qh_loadStockIcon(QStyle::SP_DialogOkButton, 0);
	QIcon cancelIcon    = qh_loadStockIcon(QStyle::SP_DialogCancelButton,
	    NULL);
	QTabWidget  *tabs   = new QTabWidget(this);
	QPushButton *ok	    = new QPushButton(okIcon, tr("&Ok"));
	QPushButton *cancel = new QPushButton(cancelIcon, tr("&Cancel"));
	QVBoxLayout *layout = new QVBoxLayout(this);
	QHBoxLayout *bbox   = new QHBoxLayout;

	setWindowIcon(winIcon);
	setWindowTitle(tr("Preferences"));

	tabs->addTab(generalSettingsTab(), tr("General settings"));
	tabs->addTab(commandsTab(), tr("Commands"));
	layout->addWidget(tabs);

	bbox->addWidget(ok, 1, Qt::AlignRight);
	bbox->addWidget(cancel, 0, Qt::AlignRight);
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
			if (esc)
				esc = false;
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
			buf[i++] = '\\';
		buf[i++] = *str;
	}
	buf[i++] = '"'; buf.resize(i);

	return (buf);
}

void Preferences::createThemeComboBox()
{
	themeBox	  = new QComboBox;
	QString curTheme  = QIcon::themeName();
	QStringList paths = QIcon::themeSearchPaths();
	QStringList names;
	QString themeName(dsbcfg_getval(cfg, CFG_TRAY_THEME).string);

	if (themeName.isNull())
		themeName = curTheme;
	for (int i = 0; i < paths.size(); i++) {
		QDirIterator it(paths.at(i));
		while (it.hasNext()) {
			QString indexPath = QString("%1/index.theme").arg(it.next());
			if (!it.fileInfo().isDir())
				continue;
			QString name = it.fileName();
			if (name == "." || name == "..")
				continue;
			QFile indexFile(indexPath);
			if (!indexFile.exists())
				continue;
			indexFile.close();
			names.append(name);
		}
	}
	names.sort(Qt::CaseInsensitive);
	names.removeDuplicates();
	themeBox->addItems(names);

	int index = themeBox->findText(themeName, Qt::MatchExactly);
	if (index != -1)
		themeBox->setCurrentIndex(index);
}

QWidget *Preferences::generalSettingsTab()
{
	QWidget	    *tab    = new QWidget;
	QVBoxLayout *layout = new QVBoxLayout(tab);
	QVBoxLayout *bhvBox = new QVBoxLayout;
	QVBoxLayout *ignBox = new QVBoxLayout;
	QVBoxLayout *tryBox = new QVBoxLayout;
	QGroupBox   *tryGrp = new QGroupBox(tr("Tray Icon Theme"));
	QGroupBox   *ignGrp = new QGroupBox(tr("Ignore Devices"));
	QGroupBox   *bhvGrp = new QGroupBox(tr("Behavior"));
	ignore_edit	    = new QLineEdit;
	icon_edit	    = new QLineEdit;
	hideOnOpen	    = new QCheckBox(tr("Hide main window after "   \
					       "opening a device"));
	notify		    = new QCheckBox(tr("Show notification when a " \
					       "device was added"));
	popup		    = new QCheckBox(tr("Show main window when a "  \
					       "device was added"));
	automount	    = new QCheckBox(tr("Automatically mount devices"));

	createThemeComboBox();
	hideOnOpen->setCheckState(
	    dsbcfg_getval(cfg, CFG_HIDE_ON_OPEN).boolean ? Qt::Checked : \
		Qt::Unchecked);
	notify->setCheckState(
	    dsbcfg_getval(cfg, CFG_MSGWIN).boolean ? Qt::Checked : \
		Qt::Unchecked);
	popup->setCheckState(
	    dsbcfg_getval(cfg, CFG_POPUP).boolean ? Qt::Checked : \
		Qt::Unchecked);
	automount->setCheckState(
	    dsbcfg_getval(cfg, CFG_AUTOMOUNT).boolean ? Qt::Checked : \
		Qt::Unchecked);
	bhvBox->addWidget(hideOnOpen);
	bhvBox->addWidget(notify);
	bhvBox->addWidget(popup);
	bhvBox->addWidget(automount);
	bhvGrp->setLayout(bhvBox);

	tryBox->addWidget(themeBox);
	tryGrp->setLayout(tryBox);

	ignBox->addWidget(ignore_edit);
	ignBox->addWidget(new QLabel(tr("Example: <tt>/dev/da0s1, " \
				        "EFISYS, /var/run/user/1001/gvfs</tt>")));
	ignore_edit->setToolTip(tr("A comma-separated list of device names, " \
				   "mount points, and volume IDs to ignore"));
	ignGrp->setLayout(ignBox);

	QString ignoreList;
	for (char **v = dsbcfg_getval(cfg, CFG_HIDE).strings;
	    v != NULL && *v != NULL; v++) {
		if (ignoreList != "")
			ignoreList.append(", ");
		ignoreList.append(quoteString(*v));
	}
	ignore_edit->setText(ignoreList);

	layout->addWidget(bhvGrp);
	layout->addWidget(ignGrp);
	layout->addWidget(tryGrp);

	layout->addStretch(1);

	return (tab);
}

QWidget *Preferences::commandsTab()
{
	int	    i;
	QLineEdit   *edit;
	QCheckBox   *cb;
	QWidget	    *tab    = new QWidget;
	QGridLayout *layout = new QGridLayout(tab);
	QString toolTip     = tr("Use %d and %m as placeholders for the "    \
				 "device and mount point, respectively");

	struct {
		QString	  label;
		int	  cfg_id_prog;
		int	  cfg_id_autoplay;
		QLineEdit **edit;
		QCheckBox **cb;
	} settings[] = {
		{
		  tr("Filemanager:"),	      CFG_FILEMANAGER, 0,
		  &fm_edit,		      nullptr
		},
		{
		  tr("Play DVDs with:"),      CFG_PLAY_DVD,    CFG_DVD_AUTO,
		  &dvd_edit,		      &dvd_autoplay
		},
		{
		  tr("Play Audio CDs with:"), CFG_PLAY_CDDA,   CFG_CDDA_AUTO,
		  &cdda_edit,		      &cdda_autoplay
		},
		{
		  tr("Play VCDs with:"),      CFG_PLAY_VCD,    CFG_VCD_AUTO,
		  &vcd_edit,		      &vcd_autoplay
		},
		{
		  tr("Play SVCDs with:"),     CFG_PLAY_SVCD,   CFG_SVCD_AUTO,
		  &svcd_edit,		      &svcd_autoplay
		}
	};
	for (i = 0; i < 5; i++) {
		edit = new QLineEdit(QString(dsbcfg_getval(cfg,
				settings[i].cfg_id_prog).string));
		edit->setToolTip(toolTip);
		*settings[i].edit = edit;
		if (settings[i].cb != nullptr) {
			cb = new QCheckBox(tr("Autoplay"));
			cb->setCheckState(dsbcfg_getval(cfg,
				settings[i].cfg_id_autoplay).boolean ?\
					Qt::Checked : Qt::Unchecked);
			layout->addWidget(cb, i, 2);
			*settings[i].cb = cb;
		}
		layout->addWidget(new QLabel(settings[i].label), i, 0);
		layout->addWidget(*settings[i].edit, i, 1);
	}
	layout->setColumnStretch(1, 1);
	layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding,
	    QSizePolicy::Expanding), i + 1, 0);
	return (tab);
}

void Preferences::openIcon()
{
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Open Image"), "/",
		tr("Image Files (*.png *.svg *.xpm)"));
	if (filename.isEmpty())
		return;
	icon_edit->setText(filename);
}

QFrame *Preferences::mkLine()
{
	QFrame *line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	return (line);
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

	val.boolean = hideOnOpen->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_HIDE_ON_OPEN, val);

	val.boolean = notify->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_MSGWIN, val);

	val.boolean = popup->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_POPUP, val);

	val.boolean = automount->checkState() == Qt::Checked ? true : false;
	dsbcfg_setval(cfg, CFG_AUTOMOUNT, val);

	val.string = themeBox->currentText().toLatin1().data();
	dsbcfg_setval(cfg, CFG_TRAY_THEME, val);

	storeList(ignore_edit->text());

	dsbcfg_write(PROGRAM, "config", cfg);
	accept();
}
