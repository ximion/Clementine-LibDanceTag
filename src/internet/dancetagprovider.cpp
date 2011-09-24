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
  // For testing
  apikey_ = "toapi11";
  data_provider_.reset_without_add(new_dataprovider());
}

void* DanceTagProvider::getFunc(const QString& name)
{
  void* res;
  QString nfo = "dancetag_" + name;
  res = libdt_->resolve(nfo.toLatin1());
  return res;
}

GObject* DanceTagProvider::new_dataprovider ()
{
  typedef GObject* (*NewDataProvider)();
  NewDataProvider _new_dt = (NewDataProvider) getFunc("data_provider_new");
  
  typedef GObject* (*DataProviderSetKey)(GObject*, const gchar*);
  DataProviderSetKey _dt_set_key = (DataProviderSetKey) getFunc("data_provider_set_api_key");
  
  typedef GObject* (*DataProviderSetAgent)(GObject*, const gchar*);
  DataProviderSetAgent _dt_set_agent = (DataProviderSetKey) getFunc("data_provider_set_useragent");

  GObject* dt = _new_dt();
  _dt_set_agent(dt, "Clementine");
  _dt_set_key(dt, apikey_.toUtf8());

  return dt;
}

bool DanceTagProvider::available() const
{
  return available_;
}

bool DanceTagProvider::ready() const
{
  if ((available()) && (!apikey_.isEmpty())
   && (data_provider_.get()))
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
  // TODO
}

QString DanceTagProvider::dancesFromFile(const char* fname, bool allowWebDB)
{
  if (!available())
    return QString();

  ScopedGObject<GObject> dtFSong;
  dtFSong.reset_without_add(new_dtsongfile(fname));
  if (!dtFSong.get())
    return QString();

  if (allowWebDB) {
    qLog(Debug) << "Searching the web for dances...";
    // Search the web for dances which match this song
    typedef bool (*UpdateDancesFromWeb)(GObject*, bool, bool);
    UpdateDancesFromWeb _dt_file_update_dances = (UpdateDancesFromWeb) getFunc("song_file_query_web_database");
    // Load dance info from web and write tag to file, do not override existing (true, false)
    bool success = _dt_file_update_dances(dtFSong.get(), true, false);
    if (!success)
      qLog(Debug) << "Unable to fetch dance tag for" << fname << "from web.";
  }

  typedef GObject* (*GetSong)(GObject*);
  GetSong _dt_get_song = (GetSong) getFunc("song_file_get_song");
  ScopedGObject<GObject> dtSong;
  dtSong.reset_without_add(_dt_get_song (dtFSong.get()));
  
  typedef const gchar* (*GetDancesStr)(GObject*);
  GetDancesStr _dt_get_dancestr = (GetDancesStr) getFunc("song_get_dances_str");
  
  QString dances = QString(_dt_get_dancestr(dtSong.get()));

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

void DanceTagProvider::_test()
{
  if (!ready())
    return;
  
  
  typedef GPtrArray* (*SearchSongs)(void*, const gchar*, const gchar*, GError**);
  SearchSongs _dt_search = (SearchSongs) getFunc("data_provider_search_songs");
  // Search the dance DB for a song
  GPtrArray* songList = _dt_search (data_provider_.get(), "Robbie Williams", "Feel", NULL);


  typedef gchar* (*SongToStr)(void*, bool);
  SongToStr _song_tostr = (SongToStr) getFunc("song_to_string");

  if (!songList) {
    // TODO
    qLog(Debug) << "ERROR!";
    return;
  }

  for (uint i = 0; i < songList->len; i++) {
    void* item = g_ptr_array_index(songList, i);
    qLog(Debug) << _song_tostr (item, true);
  }

  g_ptr_array_unref(songList);
  //g_object_unref (data_provider);
}
