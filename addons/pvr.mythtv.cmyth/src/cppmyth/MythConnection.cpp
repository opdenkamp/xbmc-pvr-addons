/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MythConnection.h"
#include "MythRecorder.h"
#include "MythFile.h"
#include "MythStorageGroupFile.h"
#include "MythProgramInfo.h"
#include "MythEventHandler.h"
#include "MythRecordingRule.h"
#include "MythPointer.h"
#include "../client.h"

using namespace ADDON;

/* Call 'call', then if 'cond' condition is true see if we're still
 * connected to the control socket and try to re-connect if not.
 * If reconnection is ok, call 'call' again. */
#define CMYTH_CONN_CALL(var, cond, call) Lock(); \
                                         var = call; \
                                         Unlock(); \
                                         if (cond) \
                                         { \
                                           if (!IsConnected() && TryReconnect()) \
                                           { \
                                             Lock(); \
                                             var = call; \
                                             Unlock(); \
                                           } \
                                         } \

/* Similar to CMYTH_CONN_CALL, but it will release 'var' if it was not NULL
 * right before calling 'call' again. */
#define CMYTH_CONN_CALL_REF(var, cond, call) Lock(); \
                                             var = call; \
                                             Unlock(); \
                                             if (cond) \
                                             { \
                                               if (!IsConnected() && TryReconnect()) \
                                               { \
                                                 Lock(); \
                                                 if (var != NULL) \
                                                   ref_release( var ); \
                                                 var = call; \
                                                 Unlock(); \
                                               } \
                                             } \

MythConnection::MythConnection()
  : m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>())
  , m_server("")
  , m_port(0)
  , m_playback(false)
  , m_pEventHandler(NULL)
{
}

MythConnection::MythConnection(const CStdString &server, unsigned short port, bool playback)
  : m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>)
  , m_server(server)
  , m_port(port)
  , m_playback(playback)
  , m_pEventHandler(NULL)
{
  cmyth_conn_t connection;
  if (m_playback)
    connection = cmyth_conn_connect_playback(const_cast<char*>(server.c_str()), port, RCV_BUF_CONTROL_SIZE, TCP_RCV_BUF_CONTROL_SIZE);
  else
    connection = cmyth_conn_connect_monitor(const_cast<char*>(server.c_str()), port, RCV_BUF_CONTROL_SIZE, TCP_RCV_BUF_CONTROL_SIZE);
  *m_conn_t = connection;
}

MythEventHandler *MythConnection::CreateEventHandler()
{
  if (m_pEventHandler)
  {
    delete(m_pEventHandler);
    m_pEventHandler = NULL;
  }
  m_pEventHandler = new MythEventHandler(m_server, m_port);
  return m_pEventHandler;
}

bool MythConnection::IsNull() const
{
  if (m_conn_t == NULL)
    return true;
  return *m_conn_t == NULL;
}

void MythConnection::Lock()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "Lock %u", m_conn_t.get());
  m_conn_t->Lock();
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "Lock acquired %u", m_conn_t.get());
}

void MythConnection::Unlock()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "Unlock %u", m_conn_t.get());
  m_conn_t->Unlock();
}

bool MythConnection::IsConnected()
{
  Lock();
  bool connected = *m_conn_t != 0 && !cmyth_conn_hung(*m_conn_t);
  Unlock();
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - %s", __FUNCTION__, (connected ? "true" : "false"));
  return connected;
}

bool MythConnection::TryReconnect()
{
  int retval;

  if (!g_szMythHostEther.IsEmpty())
      XBMC->WakeOnLan(g_szMythHostEther);

  Lock();
  if (m_playback)
    retval = cmyth_conn_reconnect_playback(*m_conn_t);
  else
    retval = cmyth_conn_reconnect_monitor(*m_conn_t);
  Unlock();
  if (retval == 0)
    XBMC->Log(LOG_DEBUG, "%s - Unable to reconnect", __FUNCTION__);
  return retval == 1;
}

CStdString MythConnection::GetBackendName() const
{
  return m_server;
}

CStdString MythConnection::GetHostname()
{
  char *hostname = NULL;
  CMYTH_CONN_CALL_REF(hostname, hostname == NULL, cmyth_conn_get_client_hostname(*m_conn_t));
  CStdString retval(hostname);
  ref_release(hostname);
  return retval;
}

CStdString MythConnection::GetBackendHostname()
{
  char *hostname = NULL;
  CMYTH_CONN_CALL_REF(hostname, hostname == NULL, cmyth_conn_get_backend_hostname(*m_conn_t));
  CStdString retval(hostname);
  ref_release(hostname);
  return retval;
}

int MythConnection::GetProtocolVersion()
{
  int retval = 0;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_conn_get_protocol_version(*m_conn_t));
  return retval;
}

bool MythConnection::GetDriveSpace(long long &total, long long &used)
{
  int retval = 0;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_conn_get_freespace(*m_conn_t, (int64_t*)&total, (int64_t*)&used));
  return retval >= 0;
}

CStdString MythConnection::GetSettingOnHost(const CStdString &setting, const CStdString &hostname)
{
  char *value = NULL;
  CMYTH_CONN_CALL_REF(value, value == NULL, cmyth_conn_get_setting(*m_conn_t, const_cast<char*>(hostname.c_str()), const_cast<char*>(setting.c_str())));
  CStdString retval(value);
  ref_release(value);
  return retval;
}

bool MythConnection::SetSetting(const CStdString &hostname, const CStdString &setting, const CStdString &value)
{
  int retval = 0;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_conn_set_setting(*m_conn_t, const_cast<char*>(hostname.c_str()), const_cast<char*>(setting.c_str()), const_cast<char*>(value.c_str())));
  return retval >= 0;
}

MythRecorder MythConnection::GetFreeRecorder()
{
  cmyth_recorder_t rec = NULL;
  CMYTH_CONN_CALL_REF(rec, rec == NULL, cmyth_conn_get_free_recorder(*m_conn_t));
  MythRecorder retval = MythRecorder(rec, *this);
  return retval;
}

MythRecorder MythConnection::GetRecorder(int n)
{
  cmyth_recorder_t rec = NULL;
  CMYTH_CONN_CALL_REF(rec, rec == NULL, cmyth_conn_get_recorder_from_num(*m_conn_t, n));
  MythRecorder retval = MythRecorder(rec, *this);
  return retval;
}

bool  MythConnection::DeleteRecording(MythProgramInfo &recording)
{
  int retval = 0;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_proginfo_delete_recording(*m_conn_t, *recording.m_proginfo_t, 0, 0));
  return retval >= 0;
}

bool  MythConnection::DeleteAndForgetRecording(MythProgramInfo &recording)
{
  int retval = 0;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_proginfo_delete_recording(*m_conn_t, *recording.m_proginfo_t, 0, 1));
  return retval >= 0;
}

ProgramInfoMap MythConnection::GetRecordedPrograms()
{
  Lock();

  ProgramInfoMap retval;
  cmyth_proglist_t proglist = NULL;
  CMYTH_CONN_CALL_REF(proglist, proglist == NULL, cmyth_proglist_get_all_recorded(*m_conn_t));
  int len = cmyth_proglist_get_count(proglist);
  for (int i = 0; i < len; i++)
  {
    MythProgramInfo prog = cmyth_proglist_get_item(proglist, i);
    if (!prog.IsNull()) {
      retval.insert(std::pair<CStdString, MythProgramInfo>(prog.UID().c_str(), prog));
    }
  }
  ref_release(proglist);

  Unlock();

  return retval;
}

MythProgramInfo MythConnection::GetRecordedProgram(const CStdString &basename)
{
  MythProgramInfo retval;
  cmyth_proginfo_t prog = NULL;
  if (!basename.IsEmpty())
  {
    CMYTH_CONN_CALL_REF(prog, prog == NULL, cmyth_proginfo_get_from_basename(*m_conn_t, const_cast<char*>(basename.c_str())));
    if (prog) {
      retval = MythProgramInfo(prog);
    }
  }

  return retval;
}

MythProgramInfo MythConnection::GetRecordedProgram(int chanid, const MythTimestamp &recstartts)
{
  MythProgramInfo retval;
  cmyth_proginfo_t prog = NULL;
  if (chanid > 0 && !recstartts.IsNull())
  {
    CMYTH_CONN_CALL_REF(prog, prog == NULL, cmyth_proginfo_get_from_timeslot(*m_conn_t, chanid, *recstartts.m_timestamp_t));
    if (prog) {
      retval = MythProgramInfo(prog);
    }
  }

  return retval;
}

bool MythConnection::GenerateRecordingPreview(MythProgramInfo &recording)
{
  int retval = 0;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_proginfo_generate_pixmap(*m_conn_t, *recording.m_proginfo_t));
  return retval >= 0;
}

ProgramInfoMap MythConnection::GetPendingPrograms()
{
  Lock();

  ProgramInfoMap retval;
  cmyth_proglist_t proglist = NULL;
  CMYTH_CONN_CALL_REF(proglist, proglist == NULL, cmyth_proglist_get_all_pending(*m_conn_t));
  int len = cmyth_proglist_get_count(proglist);
  for (int i = 0; i < len; i++)
  {
    MythProgramInfo prog = cmyth_proglist_get_item(proglist, i);
    if (!prog.IsNull()) {
      retval.insert(std::pair<CStdString, MythProgramInfo>(prog.UID().c_str(), prog));
    }
  }
  ref_release(proglist);

  Unlock();

  return retval;
}

ProgramInfoMap MythConnection::GetScheduledPrograms()
{
  Lock();

  ProgramInfoMap retval;
  cmyth_proglist_t proglist = NULL;
  CMYTH_CONN_CALL_REF(proglist, proglist == NULL, cmyth_proglist_get_all_scheduled(*m_conn_t));
  int len = cmyth_proglist_get_count(proglist);
  for (int i = 0; i < len; i++)
  {
    MythProgramInfo prog = cmyth_proglist_get_item(proglist, i);
    if (!prog.IsNull()) {
      retval.insert(std::pair<CStdString, MythProgramInfo>(prog.UID().c_str(), prog));
    }
  }
  ref_release(proglist);

  Unlock();

  return retval;
}

bool MythConnection::UpdateSchedules(int id)
{
  int retval = 0;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_conn_reschedule_recordings(*m_conn_t, id));
  return retval >= 0;
}

bool MythConnection::StopRecording(const MythProgramInfo &recording)
{
  int retval;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_proginfo_stop_recording(*m_conn_t, *recording.m_proginfo_t));
  return (retval >= 0);
}

MythStorageGroupFile MythConnection::GetStorageGroupFile(const CStdString &hostname, const CStdString &storageGroup, const CStdString &filename)
{
  cmyth_storagegroup_file_t file = NULL;
  CMYTH_CONN_CALL_REF(file, file == NULL, cmyth_storagegroup_get_fileinfo(*m_conn_t, const_cast<char*>(hostname.c_str()), const_cast<char*>(storageGroup.c_str()), const_cast<char*>(filename.c_str())));
  return MythStorageGroupFile(file);
}

MythFile MythConnection::ConnectFile(MythProgramInfo &recording)
{
  // When file is not NULL doesn't need to mean there is no more connection,
  // so always check after calling cmyth_conn_connect_file if still connected to control socket.
  cmyth_file_t file = NULL;
  CMYTH_CONN_CALL_REF(file, true, cmyth_conn_connect_file(*recording.m_proginfo_t, *m_conn_t, RCV_BUF_DATA_SIZE, TCP_RCV_BUF_DATA_SIZE));
  MythFile retval = MythFile(file);
  return retval;
}

MythFile MythConnection::ConnectPath(const CStdString &filename, const CStdString &storageGroup)
{
  cmyth_file_t file = NULL;
  CMYTH_CONN_CALL_REF(file, file == NULL, cmyth_conn_connect_path(const_cast<char*>(filename.c_str()), *m_conn_t, RCV_BUF_DATA_SIZE, TCP_RCV_BUF_DATA_SIZE, const_cast<char*>(storageGroup.c_str())));
  return MythFile(file);
}

bool MythConnection::SetBookmark(MythProgramInfo &recording, long long bookmark)
{
  int retval;
  CMYTH_CONN_CALL(retval, retval < 0, cmyth_set_bookmark(*m_conn_t, *recording.m_proginfo_t, bookmark));
  return retval >= 0;
}

long long MythConnection::GetBookmark(MythProgramInfo &recording)
{
  long long bookmark;
  CMYTH_CONN_CALL(bookmark, bookmark < 0, cmyth_get_bookmark(*m_conn_t, *recording.m_proginfo_t));
  return bookmark;
}

Edl MythConnection::GetCommbreakList(MythProgramInfo &recording)
{
  Edl retval;
  cmyth_commbreaklist_t list = NULL;
  CMYTH_CONN_CALL(list, list == NULL, cmyth_get_commbreaklist(*m_conn_t, *recording.m_proginfo_t));
  if (!list)
    return retval;

  retval.reserve(list->commbreak_count);
  for (int i = 0; i < list->commbreak_count; ++i)
  {
    cmyth_commbreak commbreak = *list->commbreak_list[i];
    retval.push_back(commbreak);
  }

  ref_release(list);
  return retval;
}

Edl MythConnection::GetCutList(MythProgramInfo &recording)
{
  Edl retval;
  cmyth_commbreaklist_t list = NULL;
  CMYTH_CONN_CALL(list, list == NULL, cmyth_get_cutlist(*m_conn_t, *recording.m_proginfo_t));
  if (!list)
    return retval;

  retval.reserve(list->commbreak_count);
  for (int i = 0; i < list->commbreak_count; ++i)
  {
    cmyth_commbreak commbreak = *list->commbreak_list[i];
    retval.push_back(commbreak);
  }

  ref_release(list);
  return retval;
}
