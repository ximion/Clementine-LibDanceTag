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
typedef struct _GCancellable GCancellable;

class DanceTagProvider : public QObject {
  Q_OBJECT

public:
  DanceTagProvider(QObject* parent = 0);

  static const char* kSettingsGroup;

  bool ready() const;
  bool available() const;

  void setApiKey(const QString& key) { apikey_ = key; }
  QString apiKey() const { return apikey_; }

  void queryDancesFromFile(const char* fname, bool allowWebDB = false);
  QString getDancesFromFile(const char* fname);

  void reloadSettings();

  static DanceTagProvider *getInstance(QObject* parent = 0);
  static void deleteInstance();

  // Please don't use these functions outside of DanceTagProvider!
  void* getFunc(const QString& name);
  Song currentSong() const { return currentSong_; }
  void setCurrentSong(const Song& s) { currentSong_ = s; }
  void emitSongDataChanged (const Song& s);

public slots:
  void fetchDanceTag(const Song& song, bool allowWebDB = false);
  void fetchDanceTagAllowWeb(const Song& song) { fetchDanceTag(song, true); }

signals:
  void songsMetadataChanged(const SongList& songs);
  void songMetadataChanged(const Song& song);

private:
  const static int DANCETAG_API_VERSION = 0;
  static bool dancetag_available_;
  static DanceTagProvider* instance_;

  QLibrary *libdt_;
  ScopedGObject<GObject> data_provider_;

  QString apikey_;
  bool enabled_;
  bool writeTags_;
  bool overrideTags_;

  Song currentSong_;
  GCancellable *currentCancellable_;

  bool setDataProviderApiKey(GObject* dt);
  GObject* new_dataprovider();
  GObject* new_dtsongfile(const gchar* fname);
};

#endif // DANCETAGPROVIDER_H
