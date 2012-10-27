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

#include "MythEventHandler.h"
#include "MythRecorder.h"
#include "MythSignal.h"
#include "MythPointer.h"
#include "MythFile.h"
#include "../client.h"

using namespace ADDON;

template <class ContainerT>
void tokenize(const std::string& str, ContainerT& tokens,
              const std::string& delimiters = " ", const bool trimEmpty = false)
{
  std::string::size_type pos, lastPos = 0;
  while (true)
  {
    pos = str.find_first_of(delimiters, lastPos);
    if (pos == std::string::npos)
    {
      pos = str.length();

      if (pos != lastPos || !trimEmpty)
        tokens.push_back(typename ContainerT::value_type(str.data() + lastPos, pos - lastPos));
      break;
    }
    else
    {
      if (pos != lastPos || !trimEmpty)
        tokens.push_back(typename ContainerT::value_type(str.data() + lastPos, pos - lastPos));
    }
    lastPos = pos + 1;
  }
};

class MythEventHandler::MythEventHandlerPrivate : public CThread, public CMutex
{
public:
  friend class MythEventHandler;

  MythEventHandlerPrivate(const CStdString &server, unsigned short port);
  virtual ~MythEventHandlerPrivate();

  virtual void* Process(void);

  void HandleUpdateSignal(const CStdString &buffer);

  void SetRecordingEventListener(const CStdString &recordid, const MythFile &file);
  void HandleUpdateFileSize(const CStdString &buffer);

  // Data
  CStdString m_server;
  unsigned short m_port;

  boost::shared_ptr<MythPointerThreadSafe<cmyth_conn_t> > m_conn_t;

  MythRecorder m_recorder;
  MythSignal m_signal;

  bool m_playback;
  CStdString m_currentRecordID;
  MythFile m_currentFile;
};

MythEventHandler::MythEventHandlerPrivate::MythEventHandlerPrivate(const CStdString &server, unsigned short port)
  : CThread()
  , CMutex()
  , m_server(server)
  , m_port(port)
  , m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>())
  , m_recorder(MythRecorder())
  , m_signal()
  , m_playback(false)
{
  *m_conn_t = cmyth_conn_connect_event(const_cast<char*>(m_server.c_str()), port, 64 * 1024, 16 * 1024);
}

MythEventHandler::MythEventHandlerPrivate::~MythEventHandlerPrivate()
{
  StopThread();
  ref_release(*m_conn_t);
  *m_conn_t = NULL;
}

void *MythEventHandler::MythEventHandlerPrivate::Process()
{
  const char *events[] = {
    "CMYTH_EVENT_UNKNOWN",
    "CMYTH_EVENT_CLOSE",
    "CMYTH_EVENT_RECORDING_LIST_CHANGE",
    "CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD",
    "CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE",
    "CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE",
    "CMYTH_EVENT_SCHEDULE_CHANGE",
    "CMYTH_EVENT_DONE_RECORDING",
    "CMYTH_EVENT_QUIT_LIVETV",
    "CMYTH_EVENT_LIVETV_WATCH",
    "CMYTH_EVENT_LIVETV_CHAIN_UPDATE",
    "CMYTH_EVENT_SIGNAL",
    "CMYTH_EVENT_ASK_RECORDING",
    "CMYTH_EVENT_SYSTEM_EVENT",
    "CMYTH_EVENT_UPDATE_FILE_SIZE",
    "CMYTH_EVENT_GENERATED_PIXMAP",
    "CMYTH_EVENT_CLEAR_SETTINGS_CACHE"
  };
  cmyth_event_t myth_event;
  char databuf[2049];
  databuf[0] = 0;
  timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;

  while (!IsStopped())
  {
    int select = 0;
    m_conn_t->Lock();
    select = cmyth_event_select(*m_conn_t, &timeout);
    m_conn_t->Unlock();

    if (select > 0)
    {
      m_conn_t->Lock();
      myth_event = cmyth_event_get(*m_conn_t, databuf, 2048);
      m_conn_t->Unlock();

      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s - EVENT ID: %s, EVENT databuf: %s", __FUNCTION__, events[myth_event], databuf);

      if (myth_event == CMYTH_EVENT_UPDATE_FILE_SIZE)
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE,"%s - Event file size update: %i", __FUNCTION__, databuf);
        HandleUpdateFileSize(databuf);
      }

      if (myth_event == CMYTH_EVENT_LIVETV_CHAIN_UPDATE)
      {
        Lock();
        if (!m_recorder.IsNull())
        {
          bool retval = m_recorder.LiveTVChainUpdate(databuf);
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s - Event chain update: %i", __FUNCTION__, retval);
        }
        else
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s - Event chain update: No recorder", __FUNCTION__);
        Unlock();
      }

      if (myth_event == CMYTH_EVENT_LIVETV_WATCH)
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE,"%s: Event LIVETV_WATCH: recoder %s", __FUNCTION__, databuf);

        Lock();
        if (!m_recorder.IsNull())
        {
          bool retval = m_recorder.LiveTVWatch(databuf);
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s: Event LIVETV_WATCH: %s", __FUNCTION__, (retval) ? "true " : "false");
        }
        else
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s: Event LIVETV_WATCH: No recorder", __FUNCTION__);
        Unlock();
      }

      if(myth_event == CMYTH_EVENT_DONE_RECORDING)
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE, "%s: Event DONE_RECORDING: recorder %s", __FUNCTION__, databuf);

        Lock();
        if (!m_recorder.IsNull())
        {
          bool retval = m_recorder.LiveTVDoneRecording(databuf);
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s: Event DONE_RECORDING: %s", __FUNCTION__, (retval) ? "true" : "false");
        }
        else
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s: Event DONE_RECORDING: No recorder", __FUNCTION__);
        Unlock();
      }

      if (myth_event == CMYTH_EVENT_SIGNAL)
      {
        HandleUpdateSignal(databuf);
      }

      if (myth_event == CMYTH_EVENT_SCHEDULE_CHANGE)
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE, "%s - Event schedule change", __FUNCTION__);
        PVR->TriggerTimerUpdate();
        PVR->TriggerRecordingUpdate();
      }

      if (!m_playback && (
          myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD ||
          myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE ||
          myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE ||
          myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE))
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE, "%s - Event recording list change", __FUNCTION__);
        PVR->TriggerRecordingUpdate();
      }

      if (myth_event == CMYTH_EVENT_CLOSE ||
          myth_event == CMYTH_EVENT_UNKNOWN)
      {
        XBMC->Log(LOG_NOTICE, "%s - Event client connection closed", __FUNCTION__);
      }

      databuf[0] = 0;
    }
    else if (select < 0 || cmyth_conn_hung(*m_conn_t))
    {
      XBMC->Log( LOG_NOTICE, "%s - Select returned error; reconnect event client connection", __FUNCTION__);

      if (!m_conn_t)
        break;

      m_conn_t->Lock();
      int retval = cmyth_conn_reconnect_event(*m_conn_t);
      m_conn_t->Unlock();
      if (retval == 1)
        XBMC->Log(LOG_NOTICE, "%s - Connected client to event socket", __FUNCTION__);
      else
        XBMC->Log(LOG_NOTICE, "%s - Could not connect client to event socket", __FUNCTION__);
    }

    //Restore timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
  }
  return NULL;
}

void MythEventHandler::MythEventHandlerPrivate::SetRecordingEventListener(const CStdString &recordid, const MythFile &file)
{
  m_currentFile = file;
  char recordIDBuffer[20];
  sscanf(recordid.c_str(),"/%4s_%14s", recordIDBuffer, recordIDBuffer + 4);
  m_currentRecordID = recordIDBuffer;
}

void MythEventHandler::MythEventHandlerPrivate::HandleUpdateSignal(const CStdString &buffer)
{
  // SIGNAL <card id> On known multiplex... or <tuner status list>
  // Example: SIGNAL 1[]:[]Channel Tuned[]:[]tuned 1 3 0 3 0 1 1[]:[]Signal Lock[]:[]slock 0 1 0 1 0 1 0[]:[]Signal Power[]:[]signal 0 45 0 100 0 1 0
  // Example: SIGNAL 1[]:[]Channel Tuned[]:[]tuned 3 3 0 3 0 1 1[]:[]Signal Lock[]:[]slock 0 1 0 1 0 1 1[]:[]Signal Power[]:[]signal 67 45 0 100 0 1 1[]:[]Seen PAT[]:[]seen_pat 0 1 0 1 0 1 1[]:[]Matching PAT[]:[]matching_pat 0 1 0 1 0 1 1[]:[]Seen PMT[]:[]seen_pmt 0 1 0 1 0 1 1[]:[]Matching PMT[]:[]matching_pmt 0 1 0 1 0 1 1

  std::vector<std::string> tokenList;
  tokenize<std::vector<std::string> >(buffer, tokenList, ";");
  for (std::vector<std::string>::iterator it = tokenList.begin(); it != tokenList.end(); it++)
  {
    std::vector<std::string> tokenList2;
    tokenize< std::vector<std::string> >(*it, tokenList2, " ");
    if (tokenList2.size() >= 2)
    {
      if (tokenList2[0] == "slock")
        m_signal.m_AdapterStatus = tokenList2[1] == "1" ? "Locked" : "No lock";
      else if (tokenList2[0] == "signal")
        m_signal.m_Signal = atoi(tokenList2[1].c_str());
      else if (tokenList2[0] == "snr")
        m_signal.m_SNR = std::atoi(tokenList2[1].c_str());
      else if (tokenList2[0] == "ber")
        m_signal.m_BER = std::atoi(tokenList2[1].c_str());
      else if (tokenList2[0] == "ucb")
        m_signal.m_UNC = std::atoi(tokenList2[1].c_str());
      else if (tokenList2[0] == "cardid")
        m_signal.m_ID = std::atoi(tokenList2[1].c_str());
    }
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleUpdateFileSize(const CStdString &buffer)
{
  // UPDATE_FILE_SIZE <chanid> <starttime> <filesize>
  // Example: 4092 2009-09-29T02:58:42 290586314

  long long length;
  char recordIDBuffer[20];
  sscanf(buffer.c_str(),"%4s %4s-%2s-%2sT%2s:%2s:%2s %lli", recordIDBuffer, recordIDBuffer + 4, recordIDBuffer + 8, recordIDBuffer + 10, recordIDBuffer + 12, recordIDBuffer + 14, recordIDBuffer + 16, &length);
  CStdString recordID = recordIDBuffer;

  if (m_currentRecordID.compare(recordID) == 0) {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"EVENT: %s, --UPDATING CURRENT RECORDING LENGTH-- EVENT msg: %s %ll", __FUNCTION__, recordID.c_str(), length);
    m_currentFile.UpdateLength(length);
  }
}

MythEventHandler::MythEventHandler()
  : m_imp()
{
}

MythEventHandler::MythEventHandler(const CStdString &server, unsigned short port)
  : m_imp(new MythEventHandlerPrivate(server, port))
{
  m_imp->CreateThread();
}


bool MythEventHandler::TryReconnect()
{
  int retval = 0;
  if (m_retryCount < 10)
  {
    XBMC->Log(LOG_DEBUG, "%s - Trying to reconnect (retry count: %d)", __FUNCTION__, m_retryCount);
    m_retryCount++;

    m_imp->m_conn_t->Lock();
    retval = cmyth_conn_reconnect_event(*(m_imp->m_conn_t));
    m_imp->m_conn_t->Unlock();

    if (retval == 1)
      m_retryCount = 0;
  }
  if (g_bExtraDebug && retval == 0)
    XBMC->Log(LOG_DEBUG, "%s - Unable to reconnect (retry count: %d)", __FUNCTION__, m_retryCount);

  return retval == 1;
}

void MythEventHandler::PreventLiveChainUpdate()
{
  m_imp->Lock();
}

void MythEventHandler::AllowLiveChainUpdate()
{
  m_imp->Unlock();
}

MythSignal MythEventHandler::GetSignal()
{
  return m_imp->m_signal;
}

void MythEventHandler::SetRecorder(const MythRecorder &recorder)
{
  m_imp->Lock();
  m_imp->m_recorder = recorder;
  m_imp->Unlock();
}

void MythEventHandler::EnablePlayback()
{
  m_imp->Lock();
  m_imp->m_playback = true;
  m_imp->Unlock();
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
}

void MythEventHandler::DisablePlayback()
{
  m_imp->Lock();
  m_imp->m_playback = false;
  m_imp->Unlock();
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
}

bool MythEventHandler::IsPlaybackActive() const
{
  return m_imp->m_playback;
}

void MythEventHandler::SetRecordingListener(const CStdString &recordid, const MythFile &file)
{
  m_imp->Lock();
  m_imp->SetRecordingEventListener(recordid, file);
  m_imp->Unlock();
}
