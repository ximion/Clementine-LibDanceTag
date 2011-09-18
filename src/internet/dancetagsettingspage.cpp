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

#include "core/network.h"
#include "magnatuneservice.h"
#include "internetmodel.h"
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

  //connect(ui_->api_key, SIGNAL(currentIndexChanged(int)), SLOT(MembershipChanged(int)));

}

DanceTagSettingsPage::~DanceTagSettingsPage() {
  delete ui_;
}

void DanceTagSettingsPage::Load() {
#if 0
  QSettings s;
  s.beginGroup(DanceTagService::kSettingsGroup);

  ui_->membership->setCurrentIndex(s.value("membership", DanceTagService::Membership_None).toInt());
  ui_->username->setText(s.value("username").toString());
  ui_->password->setText(s.value("password").toString());
  ui_->format->setCurrentIndex(s.value("format", DanceTagService::Format_Ogg).toInt());
  logged_in_ = s.value("logged_in",
      !ui_->username->text().isEmpty() &&
      !ui_->password->text().isEmpty()).toBool();

  UpdateLoginState();
#endif
}

void DanceTagSettingsPage::Save() {
#if 0
  QSettings s;
  s.beginGroup(DanceTagService::kSettingsGroup);

  s.setValue("membership", ui_->membership->currentIndex());
  s.setValue("username", ui_->username->text());
  s.setValue("password", ui_->password->text());
  s.setValue("format", ui_->format->currentIndex());
  s.setValue("logged_in", logged_in_);

  InternetModel::Service<DanceTagService>()->ReloadSettings();
#endif
}
