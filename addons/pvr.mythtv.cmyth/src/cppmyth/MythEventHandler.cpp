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
using namespace PLATFORM;

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

class MythEventHandler::MythEventHandlerPrivate : public CThread
{
public:
  friend class MythEventHandler;

  MythEventHandlerPrivate(const CStdString &server, unsigned short port);
  virtual ~MythEventHandlerPrivate();

  virtual void* Process(void);

  void HandleAskRecording(const CStdString &buffer, MythProgramInfo &programInfo);

  void HandleUpdateSignal(const CStdString &buffer);

  void HandleScheduleChange(const CStdString &buffer);

  void SetRecordingEventListener(const CStdString &recordid, const MythFile &file);
  void HandleUpdateFileSize(const CStdString &buffer);
  void HandleLiveTVWatch(const CStdString &buffer);
  void HandleUpdateLiveTVChain(const CStdString &buffer);
  void HandleDoneRecording(const CStdString &buffer);

  void SignalRecordingListEvent(bool changeEvent);
  void HandleRecordingListChangeAdd(const CStdString &buffer);
  void HandleRecordingListChangeUpdate(const CStdString &buffer, MythProgramInfo &programInfo);
  void HandleRecordingListChangeDelete(const CStdString &buffer);
  void HandleRecordingListChange();

  void RetryConnect();
  bool TryReconnect();

  // Data
  CStdString m_server;
  unsigned short m_port;

  boost::shared_ptr<MythPointerThreadSafe<cmyth_conn_t> > m_conn_t;
  timeval m_timeout;

  MythEventObserver *m_observer;

  MythRecorder m_recorder;
  MythSignal m_signal;

  bool m_playback;
  bool m_hang;
  bool m_sendWOL;
  CStdString m_currentRecordID;
  MythFile m_currentFile;

  std::list<MythEventHandler::RecordingChangeEvent> m_recordingChangeEventList;
  int m_recordingChangePinCount;

  PLATFORM::CMutex m_lock;
};

MythEventHandler::MythEventHandlerPrivate::MythEventHandlerPrivate(const CStdString &server, unsigned short port)
  : CThread()
  , m_server(server)
  , m_port(port)
  , m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>())
  , m_timeout()
  , m_observer(NULL)
  , m_recorder(MythRecorder())
  , m_signal()
  , m_playback(false)
  , m_hang(false)
  , m_sendWOL(false)
  , m_recordingChangeEventList()
  , m_recordingChangePinCount(0)
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
  m_timeout.tv_sec = 0;
  m_timeout.tv_usec = 100000;

  while (!IsStopped())
  {
    bool recordingChangeEvent = false;
    int select = 0;
    m_conn_t->Lock();
    select = cmyth_event_select(*m_conn_t, &m_timeout);
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
        HandleUpdateFileSize(databuf);
      }

      else if (myth_event == CMYTH_EVENT_LIVETV_CHAIN_UPDATE)
      {
        HandleUpdateLiveTVChain(databuf);
      }

      else if (myth_event == CMYTH_EVENT_LIVETV_WATCH)
      {
        HandleLiveTVWatch(databuf);
      }

      else if(myth_event == CMYTH_EVENT_DONE_RECORDING)
      {
        HandleDoneRecording(databuf);
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
        HandleScheduleChange(databuf);
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD)
      {
        HandleRecordingListChangeAdd(databuf);
        recordingChangeEvent = true;
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE)
      {
        MythProgramInfo prog(proginfo);
        HandleRecordingListChangeUpdate(databuf, prog);
        recordingChangeEvent = true;
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE)
      {
        HandleRecordingListChangeDelete(databuf);
        recordingChangeEvent = true;
      }

      else if (myth_event == CMYTH_EVENT_RECORDING_LIST_CHANGE)
      {
        HandleRecordingListChange();
        recordingChangeEvent = true;
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

    // Adjust timeout to accumulate more changes or trigger observer as needed
    SignalRecordingListEvent(recordingChangeEvent);
  }
  // Free recording change event
  m_recordingChangeEventList.clear();
  return NULL;
}

void MythEventHandler::MythEventHandlerPrivate::SetRecordingEventListener(const CStdString &recordid, const MythFile &file)
{
  m_currentFile = file;
  m_currentRecordID = recordid;
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

void MythEventHandler::MythEventHandlerPrivate::HandleScheduleChange(const CStdString& buffer)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_NOTICE, "%s: Event SCHEDULE_CHANGE: %s", __FUNCTION__, buffer.c_str());
  m_observer->UpdateSchedules();
}

void MythEventHandler::MythEventHandlerPrivate::HandleUpdateFileSize(const CStdString &buffer)
{
  // UPDATE_FILE_SIZE <chanid> <recstartts> <filesize>
  // Example local time: 4092 2009-09-29T02:58:42 290586314
  // Example UTC time  : 4092 2009-09-29T02:58:42Z 290586314

  unsigned int chanid;
  char recstartts[21];
  long long length;
  if (g_bExtraDebug)
    XBMC->Log(LOG_NOTICE,"%s: Event UPDATE_FILE_SIZE: %s", __FUNCTION__, buffer.c_str());
  if (sscanf(buffer.c_str(), "%u %20s %lld", &chanid, recstartts, &length) == 3)
  {
    // Make UID from recstartts
    MythTimestamp ts = MythTimestamp(recstartts);
    CStdString UID = MythProgramInfo::MakeUID(chanid, ts);
    if (m_currentRecordID.compare(UID) == 0) {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG,"%s: Event UPDATE_FILE_SIZE: UID=%s length=%lld", __FUNCTION__, UID.c_str(), length);
      m_currentFile.UpdateLength(length);
    }
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleLiveTVWatch(const CStdString &buffer)
{
  CLockObject lock(m_lock);
  if (!m_recorder.IsNull())
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_NOTICE,"%s: Event LIVETV_WATCH: recoder %s", __FUNCTION__, buffer.c_str());
    bool retval = m_recorder.LiveTVWatch(buffer);
    if (g_bExtraDebug)
      XBMC->Log(LOG_NOTICE, "%s: Event LIVETV_WATCH: %s", __FUNCTION__, (retval) ? "true " : "false");
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleUpdateLiveTVChain(const CStdString &buffer)
{
  CLockObject lock(m_lock);
  if (!m_recorder.IsNull())
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_NOTICE, "%s: Event LIVETV_CHAIN_UPDATE: %s", __FUNCTION__, buffer.c_str());
    bool retval = m_recorder.LiveTVChainUpdate(buffer);
    if (g_bExtraDebug)
      XBMC->Log(LOG_NOTICE, "%s: Event LIVETV_CHAIN_UPDATE: %s", __FUNCTION__, (retval ? "true" : "false"));
    // On success then set recording event listener
    if (retval)
    {
      int last = m_recorder.GetLiveTVChainLast();
      MythProgramInfo prog = m_recorder.GetLiveTVChainProgram(last);
      if (m_currentRecordID.compare(prog.UID()) != 0)
      {
        this->SetRecordingEventListener(prog.UID(), m_recorder.GetLiveTVChainFile(last));
        if (g_bExtraDebug)
          XBMC->Log(LOG_DEBUG,"%s: Event LIVETV_CHAIN_UPDATE: UID=%s Length=%llu", __FUNCTION__, m_currentRecordID.c_str(), m_currentFile.Length());
      }
    }
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleDoneRecording(const CStdString &buffer)
{
  CLockObject lock(m_lock);
  if (!m_recorder.IsNull())
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_NOTICE, "%s: Event DONE_RECORDING: recorder %s", __FUNCTION__, buffer.c_str());
    bool retval = m_recorder.LiveTVDoneRecording(buffer);
    if (g_bExtraDebug)
      XBMC->Log(LOG_NOTICE, "%s: Event DONE_RECORDING: %s", __FUNCTION__, (retval) ? "true" : "false");
    if (retval)
    {
      // In critical case the chain could be updated here.
      // So on success then set recording event listener. Cf CMYTH_EVENT_LIVETV_CHAIN_UPDATE
      int last = m_recorder.GetLiveTVChainLast();
      MythProgramInfo prog = m_recorder.GetLiveTVChainProgram(last);
      if (m_currentRecordID.compare(prog.UID()) != 0)
        this->SetRecordingEventListener(prog.UID(), m_recorder.GetLiveTVChainFile(last));
    }
  }
}

void MythEventHandler::MythEventHandlerPrivate::SignalRecordingListEvent(bool newChangeEvent)
{
  if (!newChangeEvent || m_recordingChangePinCount < 0)
  {
    //Restore timeout
    m_timeout.tv_sec = 0;
    m_timeout.tv_usec = 100000;
    //Need PVR update ?
    if (m_recordingChangePinCount != 0)
    {
      XBMC->Log(LOG_DEBUG, "%s - Trigger PVR recording update: %d", __FUNCTION__, m_recordingChangePinCount);
      m_observer->UpdateRecordings();
      m_recordingChangePinCount = 0;
    }
  }
  else
  {
    // Accumulate recording change events before triggering observer
    // First timeout is 0.5 sec and next 2 secs
    if (m_recordingChangePinCount < 2)
    {
      m_timeout.tv_sec = 0;
      m_timeout.tv_usec = 500000;
    }
    else
    {
      m_timeout.tv_sec = 2;
      m_timeout.tv_usec = 0;
    }
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleRecordingListChangeAdd(const CStdString& buffer)
{
  //Event data: "4121 2010-03-06T01:06:43[]:[]empty"
  unsigned int chanid;
  char recstartts[21];
  if (buffer.length() >= 24 && sscanf(buffer.c_str(), "%u %20s", &chanid, recstartts) == 2)
  {
    {
      CLockObject lock(m_lock);
      m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_ADD, chanid, recstartts));
      m_recordingChangePinCount++;
    }
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s: Event RECORDING_LIST_CHANGE_ADD: CHANID=%u TS=%s", __FUNCTION__, chanid, recstartts);
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleRecordingListChangeUpdate(const CStdString& buffer, MythProgramInfo& programInfo)
{
  (void)buffer;
  if (!programInfo.IsNull())
  {
    {
      CLockObject lock(m_lock);
      m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_UPDATE, programInfo));
      m_recordingChangePinCount++;
    }
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s: Event RECORDING_LIST_CHANGE_UPDATE: UID=%s", __FUNCTION__, programInfo.UID().c_str());
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleRecordingListChangeDelete(const CStdString& buffer)
{
  //Event data: "4121 2010-03-06T01:06:43[]:[]empty"
  unsigned int chanid;
  char recstartts[21];
  if (buffer.length() >= 24 && sscanf(buffer.c_str(), "%u %20s", &chanid, recstartts) == 2)
  {
    {
      CLockObject lock(m_lock);
      m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_DELETE, chanid, recstartts));
      m_recordingChangePinCount++;
    }
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s: Event RECORDING_LIST_CHANGE_DELETE: CHANID=%u TS=%s", __FUNCTION__, chanid, recstartts);
  }
}

void MythEventHandler::MythEventHandlerPrivate::HandleRecordingListChange()
{
  CLockObject lock(m_lock);
  m_recordingChangeEventList.push_back(RecordingChangeEvent(CHANGE_ALL));
  m_recordingChangePinCount = -1; // Trigger observer anyway
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s: Event RECORDING_LIST_CHANGE", __FUNCTION__);
}

void MythEventHandler::MythEventHandlerPrivate::RetryConnect()
{
  XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30302)); // MythTV backend unavailable
  m_hang = true;
  while (!IsStopped())
  {
    // wake up the backend sending magic packet
    if (m_sendWOL && !g_szMythHostEther.IsEmpty())
      XBMC->WakeOnLan(g_szMythHostEther);

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
      m_sendWOL = false;
      HandleRecordingListChange(); // Reload all recordings
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

MythEventHandler::MythEventHandler(const CStdString &server, unsigned short port)
  : m_imp(new MythEventHandlerPrivate(server, port))
{
  m_imp->CreateThread();
}

void MythEventHandler::RegisterObserver(MythEventObserver *observer)
{
  m_imp->m_observer = observer;
}

void MythEventHandler::Suspend()
{
  if (m_imp->IsRunning())
  {
    m_imp->StopThread();
    // We must close the connection to be able to restart properly.
    // On resume the thread will retry to connect by RetryConnect().
    // So all recordings will be reloaded after restoring the connection.
    m_imp->m_hang = true;
    ref_release(*(m_imp->m_conn_t));
    *(m_imp->m_conn_t) = NULL;
  }
}

void MythEventHandler::Resume(bool sendWOL)
{
  if (m_imp->IsStopped())
  {
    m_imp->m_sendWOL = sendWOL;
    m_imp->m_lock.Clear();
    m_imp->CreateThread();
  }
}

void MythEventHandler::PreventLiveChainUpdate()
{
  m_imp->m_lock.Lock();
}

void MythEventHandler::AllowLiveChainUpdate()
{
  m_imp->m_lock.Unlock();
}

MythSignal MythEventHandler::GetSignal()
{
  return m_imp->m_signal;
}

void MythEventHandler::SetRecorder(const MythRecorder &recorder)
{
  CLockObject lock(m_imp->m_lock);
  m_imp->m_recorder = recorder;
}

void MythEventHandler::EnablePlayback()
{
  CLockObject lock(m_imp->m_lock);
  m_imp->m_playback = true;
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
}

void MythEventHandler::DisablePlayback()
{
  CLockObject lock(m_imp->m_lock);
  m_imp->m_playback = false;
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
  CLockObject lock(m_imp->m_lock);
  m_imp->SetRecordingEventListener(recordid, file);
}

bool MythEventHandler::HasRecordingChangeEvent() const
{
  return !m_imp->m_recordingChangeEventList.empty();
}

MythEventHandler::RecordingChangeEvent MythEventHandler::NextRecordingChangeEvent()
{
  CLockObject lock(m_imp->m_lock);
  RecordingChangeEvent event = m_imp->m_recordingChangeEventList.front();
  m_imp->m_recordingChangeEventList.pop_front();
  return event;
}

void MythEventHandler::ClearRecordingChangeEvents()
{
  CLockObject lock(m_imp->m_lock);
  m_imp->m_recordingChangeEventList.clear();
}
