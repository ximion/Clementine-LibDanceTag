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

#ifndef DANCETAGPROVIDER_H
#define DANCETAGPROVIDER_H

#include <QIcon>
#include <QMetaType>
#include <QObject>
#include <QLibrary>

#include "core/scopedgobject.h"
#include "core/song.h"

class MimeData;

class DanceTagProvider : public QObject {
  Q_OBJECT

public:
  DanceTagProvider(QObject* parent = 0);
  
  static const char* kSettingsGroup;

  bool ready() const;
  bool available() const;
  
  void setApiKey(const QString& key) { apikey_ = key; }
  QString apiKey() const { return apikey_; }
  
  QString dancesFromFile(const char* fname, bool allowWebDB = false);

  void reloadSettings();

  void _test();

public slots:
  void fetchDanceTag(const Song& song, bool allowWebDB = false);
  void fetchDanceTags(const SongList& songs);
  void fetchDanceTagAllowWeb(const Song& song) { fetchDanceTag(song, true); }

signals:
  void songsMetadataChanged(const SongList& songs);
  void songMetadataChanged(const Song& song);

private:
  const static int DANCETAG_API_VERSION = 0;

  QString apikey_;
  QLibrary *libdt_;
  ScopedGObject<GObject> data_provider_;
  
  bool available_;
  
  void* getFunc(const QString& name);
  GObject* new_dataprovider();
  GObject* new_dtsongfile(const gchar* fname);
};

Q_GLOBAL_STATIC(DanceTagProvider, get_dtProvider);

#endif // DANCETAGPROVIDER_H
