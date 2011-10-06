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

#include "dancetagprovider.h"

#include <glib.h>
#include <glib-object.h>
#include <QUrl>
#include <QSettings>

#include "core/logging.h"
#include "core/scopedgobject.h"
#include "core/song.h"

const char* DanceTagProvider::kSettingsGroup = "DanceTag";

DanceTagProvider::DanceTagProvider(QObject* parent)
  : QObject(parent)
{
  // Search for the dancetag library
  libdt_ = new QLibrary("dancetag", DanceTagProvider::DANCETAG_API_VERSION, this);
  available_ = libdt_->load();
  data_provider_.reset_without_add(new_dataprovider());
  
  reloadSettings();
}

void* DanceTagProvider::getFunc(const QString& name)
{
  void* res;
  QString nfo = "dancetag_" + name;
  res = libdt_->resolve(nfo.toLatin1());
  return res;
}

bool DanceTagProvider::setDataProviderApiKey(GObject* dt)
{
  typedef GObject* (*DataProviderSetKey)(GObject*, const gchar*);
  DataProviderSetKey _dt_set_key = (DataProviderSetKey) getFunc("data_provider_set_api_key");

  _dt_set_key(dt, apikey_.toUtf8());
  
  return true;
}

GObject* DanceTagProvider::new_dataprovider ()
{
  typedef GObject* (*NewDataProvider)();
  NewDataProvider _new_dt = (NewDataProvider) getFunc("data_provider_new");
  
  typedef GObject* (*DataProviderSetAgent)(GObject*, const gchar*);
  DataProviderSetAgent _dt_set_agent = (DataProviderSetAgent) getFunc("data_provider_set_useragent");

  GObject* dt = _new_dt();
  _dt_set_agent(dt, "Clementine");
  setDataProviderApiKey(dt);

  return dt;
}

bool DanceTagProvider::available() const
{
  return available_;
}

bool DanceTagProvider::ready() const
{
  if ((available()) && (!apikey_.isEmpty())
   && (data_provider_.get()) && (enabled_))
    return true;
  else
    return false;
}

GObject* DanceTagProvider::new_dtsongfile(const gchar* fname)
{
   typedef GObject* (*NewSongFile)(const gchar*, GObject*);
   NewSongFile _new_dtsong = (NewSongFile) getFunc("song_file_new");
   if (QString::fromUtf8(fname).isEmpty())
     return NULL;
   return _new_dtsong (fname, data_provider_.get());
}

void DanceTagProvider::reloadSettings()
{
  QSettings s;
  s.beginGroup(DanceTagProvider::kSettingsGroup);

  enabled_ = s.value("enabled").toBool();
  apikey_ = s.value("api_key").toString();
  writeTags_ = s.value("write_tags_file").toBool();
  overrideTags_ = s.value("override_tags").toBool();
  
  setDataProviderApiKey(data_provider_.get());
}

QString DanceTagProvider::dancesFromFile(const char* fname, bool allowWebDB)
{
  if (!available())
    return QString();

  ScopedGObject<GObject> dtFSong;
  dtFSong.reset_without_add(new_dtsongfile(fname));
  if (!dtFSong.get())
    return QString();

  typedef GObject* (*GetSong)(GObject*);
  GetSong _dt_get_song = (GetSong) getFunc("song_file_get_song");
  ScopedGObject<GObject> dtSong;
  dtSong.reset_without_add(_dt_get_song (dtFSong.get()));
  
  typedef const gchar* (*GetDancesStr)(GObject*);
  GetDancesStr _dt_get_dancestr = (GetDancesStr) getFunc("song_get_dances_str");
  
  QString dances = QString(_dt_get_dancestr(dtSong.get()));

  if ((allowWebDB) && (dances.isEmpty())) {
    qLog(Debug) << "Searching the web for dances...";
    // Search the web for dances which match this song
    typedef bool (*UpdateDancesFromWeb)(GObject*, bool, bool);
    UpdateDancesFromWeb _dt_file_update_dances = (UpdateDancesFromWeb) getFunc("song_file_query_web_database");
    // Load dance info from web and write tag to file if setting set
    bool success = _dt_file_update_dances(dtFSong.get(), writeTags_, overrideTags_);
    if (success) {
      // Reload the song object
      dtSong.reset_without_add(_dt_get_song (dtFSong.get()));
      // Reload dances
      dances = QString(_dt_get_dancestr(dtSong.get()));
    } else
      qLog(Debug) << "Unable to fetch dance-tag for" << fname << "from web.";
  }

  return dances;
}

void DanceTagProvider::fetchDanceTag(const Song& song, bool allowWebDB)
{
  if (song.url().scheme() != "file")
    return;

  QString dances = dancesFromFile(song.url().toLocalFile().toLocal8Bit().constData(), allowWebDB);

  SongList songs;
  Song newSong = song;
  qLog(Debug) << "!DanceList: " << dances;
  newSong.set_dances (dances);
  songs.append(newSong);

  emit songsMetadataChanged(songs);
  emit songMetadataChanged(newSong);
}

void DanceTagProvider::fetchDanceTags(const SongList& songs)
{
  foreach (const Song& song, songs) {
    fetchDanceTag(song);
  }
}

DanceTagProvider* get_dtProvider(QObject* parent) {
  if (dtProv == 0)
    dtProv = new DanceTagProvider(parent);
  return dtProv;
}