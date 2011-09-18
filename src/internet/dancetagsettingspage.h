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

#ifndef DANCETAGSETTINGSPAGE_H
#define DANCETAGSETTINGSPAGE_H

#include "ui/settingspage.h"

class QAuthenticator;
class QNetworkReply;

class NetworkAccessManager;
class Ui_DanceTagSettingsPage;

class DanceTagSettingsPage : public SettingsPage {
  Q_OBJECT
public:
  DanceTagSettingsPage(SettingsDialog* dialog);
  ~DanceTagSettingsPage();

  void Load();
  void Save();

private slots:


private:


private:
  Ui_DanceTagSettingsPage* ui_;

  bool logged_in_;
};

#endif // DANCETAGSETTINGSPAGE_H
