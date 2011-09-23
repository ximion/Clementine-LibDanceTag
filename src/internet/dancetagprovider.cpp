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
#include "core/song.h"

#define DANCETAG_API_VERSION 0

const char* DanceTagProvider::kSettingsGroup = "DanceTag";

DanceTagProvider::DanceTagProvider(QObject* parent)
  : QObject(parent)
{
  // Search for the dancetag library
  m_libdt = new QLibrary("dancetag", DANCETAG_API_VERSION, this);
  dt_available = m_libdt->load();
  apikey = "";
  apikey = "toapi11";
  data_provider = new_dataprovider();
}

void* DanceTagProvider::getFunc(QString name)
{
  void* res;
  QString nfo = "dancetag_" + name;
  res = m_libdt->resolve(nfo.toLatin1());
  return res;
}

void* DanceTagProvider::new_dataprovider ()
{
  typedef void* (*NewDataProvider)();
  NewDataProvider _new_dt = (NewDataProvider) getFunc("data_provider_new");
  
  typedef void* (*DataProviderSetKey)(void*, const gchar*);
  DataProviderSetKey _dt_set_key = (DataProviderSetKey) getFunc("data_provider_set_api_key");

  void* dt = _new_dt();
  _dt_set_key(dt, apikey.toLatin1());

  return dt;
}

bool DanceTagProvider::available() const
{
  return dt_available;
}

bool DanceTagProvider::ready() const
{
  if ((available()) && (apikey != "")
   && (data_provider))
    return true;
  else
    return false;
}

void* DanceTagProvider::new_dtsongfile(const gchar* fname)
{
   typedef void* (*NewSongFile)(const gchar*, void*);
   NewSongFile _new_dtsong = (NewSongFile) getFunc("song_file_new");
   if (QString::fromLatin1(fname) == "")
     return NULL;
   return _new_dtsong (fname, data_provider);
}

void DanceTagProvider::reloadSettings()
{
  // TODO
}

QString DanceTagProvider::dancesFromFile(const char* fname, bool allowWebDB)
{
  if (!available())
    return "";

  void* dtSong = new_dtsongfile(fname);
  if (!dtSong)
    return "";

  if (allowWebDB) {
    qDebug() << "Searching the web for dances...";
    // Search the web for dances which match this song
    typedef bool (*UpdateDancesFromWeb)(void*);
    UpdateDancesFromWeb _dt_file_update_dances = (UpdateDancesFromWeb) getFunc("song_file_update_dance_information_from_web");
    bool success = _dt_file_update_dances(dtSong);
    if (!success)
      qLog(Debug) << "Unable to fetch dance tag for" << fname << "from web.";
  }

  typedef GPtrArray* (*GetDances)(void*);
  GetDances _dt_get_dances = (GetDances) getFunc("song_file_get_dances");
  GPtrArray* danceList = _dt_get_dances (dtSong);
  
  QString dances = "";
  if (danceList) {
      for (uint i = 0; i < danceList->len; i++) {
	const gchar* dance = (const gchar*) g_ptr_array_index(danceList, i);
	if ((dance == NULL) || (QString(dance) == ""))
	  continue;

	qDebug() << dance;
	if (dances != "")
	  dances += " / ";
	dances += QString::fromLatin1(dance);
      }

    g_ptr_array_unref(danceList);
  }
  g_object_unref(dtSong);
  return dances;
}

void DanceTagProvider::fetchDanceTag(const Song& song, bool allowWebDB)
{
  QString dances = dancesFromFile(song.url().toString(QUrl::RemoveScheme).toLatin1(), allowWebDB);

  SongList songs;
  Song newSong = song;
  qDebug() << "!!!!!!!!!!!!!! " << dances;
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
  GPtrArray* songList = _dt_search (data_provider, "Robbie Williams", "Feel", NULL);


  typedef gchar* (*SongToStr)(void*, bool);
  SongToStr _song_tostr = (SongToStr) getFunc("song_to_string");

  if (!songList) {
    qDebug() << "ERROR!";
    return;
  }

  for (uint i = 0; i < songList->len; i++) {
    void* item = g_ptr_array_index(songList, i);
    qDebug() << _song_tostr (item, true);
  }

  g_ptr_array_unref(songList);
  //g_object_unref (data_provider);
}
