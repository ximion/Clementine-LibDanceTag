/* This file is part of Clementine.
   Copyright 2011, Matthias Klumpp <matthias@tenstral.net>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dancetagsettingspage.h"

#include "internet/dancetagprovider.h"
#include "ui_dancetagsettingspage.h"

#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QtDebug>

DanceTagSettingsPage::DanceTagSettingsPage(SettingsDialog* dialog)
  : SettingsPage(dialog),
    ui_(new Ui_DanceTagSettingsPage),
    logged_in_(false)
{
  ui_->setupUi(this);
  setWindowIcon(QIcon(":/providers/dancetag.png"));
  
  connect(ui_->dt_enabled, SIGNAL(toggled(bool)), this, SLOT(enableDanceTag(bool)));
}

DanceTagSettingsPage::~DanceTagSettingsPage() {
  delete ui_;
}

void DanceTagSettingsPage::Load() {
  QSettings s;
  s.beginGroup(DanceTagProvider::kSettingsGroup);

  ui_->dt_enabled->setChecked(s.value("enabled").toBool());
  ui_->api_key->setText(s.value("api_key").toString());
  ui_->write_tags->setChecked(s.value("write_tags_file", true).toBool());
  ui_->override_tags->setChecked(s.value("override_tags").toBool());
}

void DanceTagSettingsPage::Save() {
  QSettings s;
  s.beginGroup(DanceTagProvider::kSettingsGroup);

  s.setValue("enabled", ui_->dt_enabled->isChecked());
  s.setValue("api_key", ui_->api_key->text());
  s.setValue("write_tags_file", ui_->write_tags->isChecked());
  s.setValue("override_tags", ui_->override_tags->isChecked());

  get_dtProvider()->reloadSettings();
}

void DanceTagSettingsPage::enableDanceTag(bool enabled)
{
  ui_->login_container->setEnabled(enabled);
  ui_->options_box->setEnabled(enabled);
}
