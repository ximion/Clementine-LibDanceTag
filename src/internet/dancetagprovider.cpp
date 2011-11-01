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

#ifdef signals
# undef signals
#endif

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <QUrl>
#include <QSettings>

#include "core/logging.h"
#include "core/scopedgobject.h"
#include "core/song.h"

const char* DanceTagProvider::kSettingsGroup = "DanceTag";
DanceTagProvider* DanceTagProvider::instance_ = 0;

// Define types for LibDanceTag Methods
typedef GObject* (*NewDataProvider)();
typedef GObject* (*DataProviderSetKey)(GObject*, const gchar*);
typedef GObject* (*DataProviderSetAgent)(GObject*, const gchar*);
typedef GObject* (*NewSongFile)(const gchar*, GObject*);
typedef const gchar* (*GetDancesStr)(GObject*);
typedef void (*QueryDancesFromWeb)(GObject*, bool, bool, GCancellable*, GAsyncReadyCallback, DanceTagProvider*);
typedef gboolean (*QueryDancesFromWeb_Finish)(GObject*, GAsyncResult*, GError**);

DanceTagProvider::DanceTagProvider(QObject* parent)
  : QObject(parent)
{
  // Search for the dancetag library
  libdt_ = new QLibrary("dancetag", DanceTagProvider::DANCETAG_API_VERSION, this);
  available_ = libdt_->load();
  data_provider_.reset_without_add(new_dataprovider());
  currentCancellable_ = NULL;

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
  DataProviderSetKey _dt_set_key = (DataProviderSetKey) getFunc("data_provider_set_api_key");

  _dt_set_key(dt, apikey_.toUtf8());

  return true;
}

GObject* DanceTagProvider::new_dataprovider ()
{
  NewDataProvider _new_dt = (NewDataProvider) getFunc("data_provider_new");
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
   && (data_provider_) && (enabled_))
    return true;
  else
    return false;
}

GObject* DanceTagProvider::new_dtsongfile(const gchar* fname)
{
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

static const gchar* songfile_get_dances_str (DanceTagProvider *self, GObject *dtfile) {
  GetDancesStr _dt_get_dancestr = (GetDancesStr) self->getFunc("song_file_get_song_dances_str");

  return _dt_get_dancestr(dtfile);
}

static void song_file_query_web_database_finish (GObject* dtfile, GAsyncResult* res, gpointer udata)
{
  GError *error = NULL;
  DanceTagProvider *self = static_cast<DanceTagProvider*>(udata);
  if (self == NULL) {
    qLog(Error) << "Received invalid DanceTagProvider* instance!";
    return;
  }

  QueryDancesFromWeb_Finish _dt_query_finish = (QueryDancesFromWeb_Finish) self->getFunc("song_file_query_web_database_finish");

  bool success = _dt_query_finish (dtfile, res, &error);

  if (error != NULL) {
    qLog(Error) << error->message;
    g_clear_error (&error);
    return;
  }

  // Get the current song data and abort if it's not defined.
  Song newSong = self->currentSong();
  if (!newSong.is_valid())
    return;

  QString dances = newSong.dances();
  if (success) {
      // Reload dances
      dances = QString::fromUtf8(songfile_get_dances_str(self, dtfile));
  } else
      qLog(Debug) << "Unable to fetch dance-tag for" << newSong.basefilename() << "from web.";

  qLog(Debug) << "!DanceList: " << dances;
  newSong.set_dances (dances);

  self->setCurrentSong(Song());
  self->emitSongDataChanged(newSong);
}

void DanceTagProvider::emitSongDataChanged(const Song& s)
{
  SongList songs;
  songs.append(s);

  emit songsMetadataChanged(songs);
  emit songMetadataChanged(s);
}


void DanceTagProvider::queryDancesFromFile(const char* fname, bool allowWebDB)
{
  if (!available())
    return;

  QueryDancesFromWeb _dt_file_query_db = (QueryDancesFromWeb) getFunc("song_file_query_web_database");

  ScopedGObject<GObject> dtFSong;
  dtFSong.reset_without_add(new_dtsongfile(fname));
  if (!dtFSong)
    return;

  QString dances = QString::fromUtf8(songfile_get_dances_str(this, dtFSong.get()));

  if ((allowWebDB) && (dances.isEmpty())) {
    qLog(Debug) << "Searching the web for dances...";

    if (currentCancellable_ != NULL) {
      // Cancel previous action
      g_cancellable_cancel (currentCancellable_);
      g_object_unref (currentCancellable_);
    }
    currentCancellable_ = g_cancellable_new ();

    // Search the web for dances which match this song
    // (Load dance info from web and write tag to file if setting set)
    _dt_file_query_db(dtFSong.get(), writeTags_, overrideTags_,
		      currentCancellable_, song_file_query_web_database_finish, this);
  }
}

QString DanceTagProvider::getDancesFromFile(const char* fname)
{
  if (!available())
    return QString();

  ScopedGObject<GObject> dtFSong;
  dtFSong.reset_without_add(new_dtsongfile(fname));
  if (!dtFSong)
    return QString();

  QString dances = QString::fromUtf8(songfile_get_dances_str(this, dtFSong.get()));

  return dances;
}

void DanceTagProvider::fetchDanceTag(const Song& song, bool allowWebDB)
{
  if (!ready())
    return;

  if (song.url().scheme() != "file")
    return;

  if (!song.dances().isEmpty())
    return;

  setCurrentSong(song);
  queryDancesFromFile(song.url().toLocalFile().toLocal8Bit().constData(), allowWebDB);
}

DanceTagProvider* DanceTagProvider::getInstance(QObject* parent) {
  if (instance_ == 0)
    instance_ = new DanceTagProvider(parent);
  return instance_;
}

void DanceTagProvider::deleteInstance()
{
  delete instance_;
  instance_ = 0;
}
