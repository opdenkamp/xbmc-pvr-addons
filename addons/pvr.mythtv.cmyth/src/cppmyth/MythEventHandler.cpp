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
#include "MythProgramInfo.h"
#include "MythTimestamp.h"
#include "../client.h"

#include <list>

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

  void HandleAskRecording(const CStdString &buffer, MythProgramInfo &programInfo);

  void HandleUpdateSignal(const CStdString &buffer);

  void SetRecordingEventListener(const CStdString &recordid, const MythFile &file);
  void HandleUpdateFileSize(const CStdString &buffer);

  void RetryConnect();
  bool TryReconnect();
  void RecordingListChange();

  // Data
  CStdString m_server;
  unsigned short m_port;

  boost::shared_ptr<MythPointerThreadSafe<cmyth_conn_t> > m_conn_t;

  MythEventObserver *m_observer;

  MythRecorder m_recorder;
  MythSignal m_signal;

  bool m_playback;
  bool m_hang;
  CStdString m_currentRecordID;
  MythFile m_currentFile;

  std::list<MythEventHandler::RecordingChangeEvent> m_recordingChangeEventList;
};

MythEventHandler::MythEventHandlerPrivate::MythEventHandlerPrivate(const CStdString &server, unsigned short port)
  : CThread()
  , CMutex()
  , m_server(server)
  , m_port(port)
  , m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>())
  , m_observer(NULL)
  , m_recorder(MythRecorder())
  , m_signal()
  , m_playback(false)
  , m_hang(false)
  , m_recordingChangeEventList()
{
  *m_conn_t = cmyth_conn_connect_event(const_cast<char*>(m_server.c_str()), port, RCV_BUF_CONTROL_SIZE, TCP_RCV_BUF_CONTROL_SIZE);
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
  bool triggerRecordingUpdate = false;
  unsigned int recordingChangeCount = 0;
  timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;

  while (!IsStopped())
  {
    bool recordingChange = false;
    int select = 0;
    m_conn_t->Lock();
    select = cmyth_event_select(*m_conn_t, &timeout);
    m_conn_t->Unlock();

    if (select > 0)
    {
      cmyth_proginfo_t proginfo = NULL;
      m_conn_t->Lock();
      myth_event = cmyth_event_get_message(*m_conn_t, databuf, 2048, &proginfo);
      m_conn_t->Unlock();

      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s - EVENT ID: %s, EVENT databuf: %s", __FUNCTION__, events[myth_event], databuf);

      if (myth_event == CMYTH_EVENT_UPDATE_FILE_SIZE)
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE,"%s - Event file size update: %s", __FUNCTION__, databuf);
        HandleUpdateFileSize(databuf);
      }

      else if (myth_event == CMYTH_EVENT_LIVETV_CHAIN_UPDATE)
      {
        Lock();
        if (!m_recorder.IsNull())
        {
          bool retval = m_recorder.LiveTVChainUpdate(databuf);
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s - Event chain update: %s", __FUNCTION__, (retval ? "true" : "false"));
        }
        else
          if (g_bExtraDebug)
            XBMC->Log(LOG_NOTICE, "%s - Event chain update: No recorder", __FUNCTION__);
        Unlock();
      }

      else if (myth_event == CMYTH_EVENT_LIVETV_WATCH)
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

      else if(myth_event == CMYTH_EVENT_DONE_RECORDING)
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

      else if (myth_event == CMYTH_EVENT_ASK_RECORDING)
      {
        MythProgramInfo prog(proginfo);
        HandleAskRecording(databuf, prog);
      }

      else if (myth_event == CMYTH_EVENT_SIGNAL)
      {
        HandleUpdateSignal(databuf);
      }

      if (myth_event == CMYTH_EVENT_SCHEDULE_CHANGE)
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE, "%s - Event schedule change", __FUNCTION__);
        PVR->TriggerTimerUpdate();
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD)
      {
        //Event data: "4121 2010-03-06T01:06:43[]:[]empty"
        unsigned int chanid;
        char recstartts[20];
        if (strlen(databuf)>=24 && sscanf(databuf, "%u %19s", &chanid, recstartts) == 2) {
          Lock();
          m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_ADD, chanid, recstartts));
          Unlock();
          if (g_bExtraDebug)
            XBMC->Log(LOG_DEBUG,"%s - Event recording list add: CHANID=%u TS=%s", __FUNCTION__, chanid, recstartts);
          recordingChange = true;
        }
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE)
      {
        //Event data: Updated 'proginfo' is returned
        MythProgramInfo prog = MythProgramInfo(proginfo);
        if (!prog.IsNull())
        {
          Lock();
          m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_UPDATE, prog));
          Unlock();
          if (g_bExtraDebug)
            XBMC->Log(LOG_DEBUG,"%s - Event recording list update: UID=%s", __FUNCTION__, prog.UID().c_str());
          recordingChange = true;
        }
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE)
      {
        //Event data: "4121 2010-03-06T01:06:43[]:[]empty"
        unsigned int chanid;
        char recstartts[20];
        if (strlen(databuf)>=24 && sscanf(databuf, "%u %19s", &chanid, recstartts) == 2) {
          Lock();
          m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_DELETE, chanid, recstartts));
          Unlock();
          if (g_bExtraDebug)
            XBMC->Log(LOG_DEBUG,"%s - Event recording list delete: CHANID=%u TS=%s", __FUNCTION__, chanid, recstartts);
          recordingChange = true;
        }
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE)
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_NOTICE, "%s - Event recording list change", __FUNCTION__);
        RecordingListChange();
      }

      else if (myth_event == CMYTH_EVENT_UNKNOWN)
      {
        XBMC->Log(LOG_NOTICE, "%s - Event unknown, databuf: %s", __FUNCTION__, databuf);
      }

      else if (myth_event == CMYTH_EVENT_CLOSE)
      {
        XBMC->Log(LOG_NOTICE, "%s - Event client connection closed", __FUNCTION__);
        RetryConnect();
      }

      databuf[0] = 0;
    }
    else if (select < 0)
    {
      XBMC->Log(LOG_ERROR, "%s Event client connection error", __FUNCTION__);
      RetryConnect();
    }
    else if (cmyth_conn_hung(*m_conn_t))
    {
      XBMC->Log(LOG_NOTICE, "%s - Connection hung - reconnect event client connection", __FUNCTION__);
      if (!m_conn_t || !TryReconnect())
        RetryConnect();
    }

    //Accumulate recording change events before triggering PVR event
    //First timeout is 0.5 sec and next 2 secs
    if (recordingChange)
    {
      if (recordingChangeCount == 0)
      {
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
      }
      else
      {
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
      }
      recordingChangeCount++;
      triggerRecordingUpdate = true;
    }
    else
    {
      //Restore timeout
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      //Need PVR update ?
      if (triggerRecordingUpdate)
      {
        XBMC->Log(LOG_DEBUG, "%s - Trigger PVR recording update: %lu recording(s)", __FUNCTION__, recordingChangeCount);
        PVR->TriggerRecordingUpdate();
        triggerRecordingUpdate = false;
      }
      //Reset counter
      recordingChangeCount = 0;
    }
  }
  // Free recording change event
  m_recordingChangeEventList.clear();
  return NULL;
}

void MythEventHandler::MythEventHandlerPrivate::SetRecordingEventListener(const CStdString &recordid, const MythFile &file)
{
  m_currentFile = file;
  char recordIDBuffer[20];
  sscanf(recordid.c_str(),"/%4s_%14s", recordIDBuffer, recordIDBuffer + 4);
  m_currentRecordID = recordIDBuffer;
}

void MythEventHandler::MythEventHandlerPrivate::HandleAskRecording(const CStdString &databuf, MythProgramInfo &programInfo)
{
  // ASK_RECORDING <card id> <time until> <has rec> <has later>[]:[]<program info>
  // Example: ASK_RECORDING 9 29 0 1[]:[]<program>

  // The scheduled recording will hang in MythTV if ASK_RECORDING is just ignored.
  // - Stop recorder (and blocked for time until seconds)
  // - Skip the recording by sending CANCEL_NEXT_RECORDING(true)

  unsigned int cardid;
  int timeuntil, hasrec, haslater;
  if (sscanf(databuf.c_str(), "%d %d %d %d", &cardid, &timeuntil, &hasrec, &haslater) == 4)
    XBMC->Log(LOG_NOTICE, "%s: Event ASK_RECORDING: rec=%d timeuntil=%d hasrec=%d haslater=%d", __FUNCTION__, cardid, timeuntil, hasrec, haslater);
  else
    XBMC->Log(LOG_ERROR, "%s: Incorrect ASK_RECORDING event: rec=%d timeuntil=%d hasrec=%d haslater=%d", __FUNCTION__, cardid, timeuntil, hasrec, haslater);

  CStdString title;
  if (!programInfo.IsNull())
    title = programInfo.Title();
  XBMC->Log(LOG_NOTICE, "%s: Event ASK_RECORDING: title=%s", __FUNCTION__, title.c_str());

  if (timeuntil >= 0 && !m_recorder.IsNull() && m_recorder.ID() == cardid)
  {
    if (g_iLiveTVConflictStrategy == LIVETV_CONFLICT_STRATEGY_CANCELREC ||
      (g_iLiveTVConflictStrategy == LIVETV_CONFLICT_STRATEGY_HASLATER && haslater))
    {
      XBMC->QueueNotification(QUEUE_WARNING, XBMC->GetLocalizedString(30307), title.c_str()); // Canceling conflicting recording: %s
      m_recorder.CancelNextRecording(true);
    }
    else // LIVETV_CONFLICT_STRATEGY_STOPTV
    {
      XBMC->QueueNotification(QUEUE_WARNING, XBMC->GetLocalizedString(30308), title.c_str()); // Stopping Live TV due to conflicting recording: %s
      if (m_observer)
        m_observer->CloseLiveStream();
    }
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleUpdateSignal(const CStdString &buffer)
{
  // SIGNAL <card id> On known multiplex... or <tuner status list>
  // Example: SIGNAL 1[]:[]Channel Tuned[]:[]tuned 1 3 0 3 0 1 1[]:[]Signal Lock[]:[]slock 0 1 0 1 0 1 0[]:[]Signal Power[]:[]signal 0 45 0 100 0 1 0
  // Example: SIGNAL 1[]:[]Channel Tuned[]:[]tuned 3 3 0 3 0 1 1[]:[]Signal Lock[]:[]slock 0 1 0 1 0 1 1[]:[]Signal Power[]:[]signal 67 45 0 100 0 1 1[]:[]Seen PAT[]:[]seen_pat 0 1 0 1 0 1 1[]:[]Matching PAT[]:[]matching_pat 0 1 0 1 0 1 1[]:[]Seen PMT[]:[]seen_pmt 0 1 0 1 0 1 1[]:[]Matching PMT[]:[]matching_pmt 0 1 0 1 0 1 1

  std::vector<std::string> tokenList;
  tokenize<std::vector<std::string> >(buffer, tokenList, ";");
  for (std::vector<std::string>::iterator it = tokenList.begin(); it != tokenList.end(); ++it)
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
        m_signal.m_BER = std::atol(tokenList2[1].c_str());
      else if (tokenList2[0] == "ucb")
        m_signal.m_UNC = std::atol(tokenList2[1].c_str());
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
  sscanf(buffer.c_str(),"%4s %4s-%2s-%2sT%2s:%2s:%2s %lld", recordIDBuffer, recordIDBuffer + 4, recordIDBuffer + 8, recordIDBuffer + 10, recordIDBuffer + 12, recordIDBuffer + 14, recordIDBuffer + 16, &length);
  CStdString recordID = recordIDBuffer;

  if (m_currentRecordID.compare(recordID) == 0) {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"EVENT: %s, --UPDATING CURRENT RECORDING LENGTH-- EVENT msg: %s %lld", __FUNCTION__, recordID.c_str(), length);
    m_currentFile.UpdateLength(length);
  }
}

void MythEventHandler::MythEventHandlerPrivate::RetryConnect()
{
  XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30302)); // MythTV backend unavailable
  m_hang = true;
  while (!IsStopped())
  {
    usleep(999999);
    ref_release(*m_conn_t);
    *m_conn_t = NULL;
    *m_conn_t = cmyth_conn_connect_event(const_cast<char*>(m_server.c_str()), m_port, RCV_BUF_CONTROL_SIZE, TCP_RCV_BUF_CONTROL_SIZE);

    if (*m_conn_t == NULL)
      XBMC->Log(LOG_NOTICE, "%s - Could not connect client to event socket", __FUNCTION__);
    else
    {
      XBMC->Log(LOG_NOTICE, "%s - Connected client to event socket", __FUNCTION__);
      XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(30303)); // MythTV backend available
      m_hang = false;
      RecordingListChange();
      break;
    }
  }
}

bool MythEventHandler::MythEventHandlerPrivate::TryReconnect()
{
  int retval = 0;
  XBMC->Log(LOG_DEBUG, "%s - Trying to reconnect", __FUNCTION__);
  m_conn_t->Lock();
  retval = cmyth_conn_reconnect_event(*m_conn_t);
  m_conn_t->Unlock();
  if (retval == 1)
    XBMC->Log(LOG_NOTICE, "%s - Connected client to event socket", __FUNCTION__);
  else
    XBMC->Log(LOG_NOTICE, "%s - Could not connect client to event socket", __FUNCTION__);
  return retval == 1;
}

void MythEventHandler::MythEventHandlerPrivate::RecordingListChange()
{
  Lock();
  m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_ALL));
  Unlock();
  PVR->TriggerRecordingUpdate();
}

MythEventHandler::MythEventHandler(const CStdString &server, unsigned short port)
  : m_imp(new MythEventHandlerPrivate(server, port))
{
  m_imp->CreateThread();
}

void MythEventHandler::RegisterObserver(MythEventObserver *observer)
{
  m_imp->m_observer = observer;
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

bool MythEventHandler::IsListening() const
{
  return (!m_imp->m_hang);
}

void MythEventHandler::SetRecordingListener(const CStdString &recordid, const MythFile &file)
{
  m_imp->Lock();
  m_imp->SetRecordingEventListener(recordid, file);
  m_imp->Unlock();
}

bool MythEventHandler::HasRecordingChangeEvent() const
{
  return !m_imp->m_recordingChangeEventList.empty();
}

MythEventHandler::RecordingChangeEvent MythEventHandler::NextRecordingChangeEvent()
{
  m_imp->Lock();
  RecordingChangeEvent event = m_imp->m_recordingChangeEventList.front();
  m_imp->m_recordingChangeEventList.pop_front();
  m_imp->Unlock();
  return event;
}

void MythEventHandler::ClearRecordingChangeEvents()
{
  m_imp->Lock();
  m_imp->m_recordingChangeEventList.clear();
  m_imp->Unlock();
}
