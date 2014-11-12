/*
 *      Copyright (C) 2014 Jean-Luc Barriere
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
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "mythlivetvplayback.h"
#include "mythdebug.h"
#include "private/builtin.h"
#include "private/mythsocket.h"
#include "private/platform/threads/mutex.h"

#include <limits>
#include <cstdio>
#include <stdlib.h>

#define MIN_TUNE_DELAY        5
#define MAX_TUNE_DELAY        60
#define TICK_USEC             100000  // valid range: 10000 - 999999
#define STARTING_DELAY        1

using namespace Myth;

///////////////////////////////////////////////////////////////////////////////
////
//// Protocol connection to control LiveTV playback
////

LiveTVPlayback::LiveTVPlayback(EventHandler& handler)
: ProtoMonitor(handler.GetServer(), handler.GetPort()), EventSubscriber()
, m_eventHandler(handler)
, m_eventSubscriberId(0)
, m_tuneDelay(MIN_TUNE_DELAY)
, m_recorder()
, m_signal()
, m_chain()
{
  m_eventSubscriberId = m_eventHandler.CreateSubscription(this);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_SIGNAL);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_LIVETV_CHAIN);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_LIVETV_WATCH);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_DONE_RECORDING);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_UPDATE_FILE_SIZE);
  Open();
}

LiveTVPlayback::LiveTVPlayback(const std::string& server, unsigned port)
: ProtoMonitor(server, port), EventSubscriber()
, m_eventHandler(server, port)
, m_eventSubscriberId(0)
, m_tuneDelay(MIN_TUNE_DELAY)
, m_recorder()
, m_signal()
, m_chain()
{
  // Private handler will be stopped and closed by destructor.
  m_eventSubscriberId = m_eventHandler.CreateSubscription(this);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_SIGNAL);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_LIVETV_CHAIN);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_LIVETV_WATCH);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_DONE_RECORDING);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_UPDATE_FILE_SIZE);
  Open();
}

LiveTVPlayback::~LiveTVPlayback()
{
  if (m_eventSubscriberId)
    m_eventHandler.RevokeSubscription(m_eventSubscriberId);
  Close();
}

bool LiveTVPlayback::Open()
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  if (ProtoMonitor::IsOpen())
    return true;
  if (ProtoMonitor::Open())
  {
    if (!m_eventHandler.IsRunning())
    {
      uint32_t timer = 0, delay = STARTING_DELAY * 1000000;
      m_eventHandler.Start();
      while (!m_eventHandler.IsConnected() && timer < delay)
      {
        usleep(TICK_USEC);
        timer += TICK_USEC;
      }
      if (!m_eventHandler.IsConnected())
        DBG(MYTH_DBG_WARN, "%s: event handler is not connected in time (%" PRIu32 "ms)\n", __FUNCTION__, (timer / 1000));
      else
        DBG(MYTH_DBG_DEBUG, "%s: event handler is connected (%" PRIu32 "ms)\n", __FUNCTION__, (timer / 1000));
    }
    return true;
  }
  return false;
}

void LiveTVPlayback::Close()
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  m_recorder.reset();
  ProtoMonitor::Close();
}

void LiveTVPlayback::SetTuneDelay(unsigned delay)
{
  if (delay < MIN_TUNE_DELAY)
    m_tuneDelay = MIN_TUNE_DELAY;
  else if (delay > MAX_TUNE_DELAY)
    m_tuneDelay = MAX_TUNE_DELAY;
  else
    m_tuneDelay = delay;
}

bool LiveTVPlayback::SpawnLiveTV(const Channel& channel, uint32_t prefcardid)
{
  int rnum = 0; // first selected recorder num
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  if (!ProtoMonitor::IsOpen() || !m_eventHandler.IsConnected())
  {
    DBG(MYTH_DBG_ERROR, "%s: not connected\n", __FUNCTION__);
    return false;
  }

  StopLiveTV();
  // if i have'nt yet recorder then choose one
  if (!m_recorder)
  {
    // Start with my prefered card else get next free recorder
    if (!prefcardid || !(m_recorder = GetRecorderFromNum((int)prefcardid)))
      m_recorder = GetNextFreeRecorder(-1);
  }
  if (m_recorder)
  {
    InitChain(); // Setup chain
    rnum = m_recorder->GetNum(); // keep track of my first recorder
    do
    {
      DBG(MYTH_DBG_DEBUG, "%s: checking recorder num (%d)\n", __FUNCTION__, m_recorder->GetNum());
      if (m_recorder->IsTunable(channel))
      {
        // Setup the chain
        m_chain.switchOnCreate = true;
        m_chain.watch = true;
        if (m_recorder->SpawnLiveTV(m_chain.UID, channel.chanNum))
        {
          // Wait chain update until time limit
          uint32_t timer = 0, delay = m_tuneDelay * 1000000;
          do
          {
            lock.Unlock();  // Release the latch to allow chain update
            usleep(TICK_USEC);
            timer += TICK_USEC;
            lock.Lock();
            if (!m_chain.switchOnCreate)
            {
              DBG(MYTH_DBG_DEBUG, "%s: tune delay (%" PRIu32 "ms)\n", __FUNCTION__, (timer / 1000));
              return true;
            }
          }
          while (timer < delay);
          DBG(MYTH_DBG_ERROR, "%s: tune delay exceeded (%" PRIu32 "ms)\n", __FUNCTION__, (timer / 1000));
          m_recorder->StopLiveTV();
        }
        // Not retry next recorder
        // return false;
      }
      m_recorder = GetNextFreeRecorder(m_recorder->GetNum());
    }
    while (m_recorder && m_recorder->GetNum() != rnum);
    ClearChain();
  }
  return false;
}

void LiveTVPlayback::StopLiveTV()
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  if (m_recorder && m_recorder->IsPlaying())
  {
    m_recorder->StopLiveTV();
    // If recorder is keeping recording then release it to clear my instance status.
    // Otherwise next program would be considered as preserved.
    if (m_recorder->IsLiveRecording())
      m_recorder.reset();
  }
}

void LiveTVPlayback::InitChain()
{
  char buf[32];
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  time2iso8601(time(NULL), buf);
  m_chain.UID = m_socket->GetMyHostName();
  m_chain.UID.append("-").append(buf);
  m_chain.currentSequence = 0;
  m_chain.lastSequence = 0;
  m_chain.watch = false;
  m_chain.switchOnCreate = true;
  m_chain.chained.clear();
  m_chain.currentTransfer.reset();
}

void LiveTVPlayback::ClearChain()
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  m_chain.currentSequence = 0;
  m_chain.lastSequence = 0;
  m_chain.watch = false;
  m_chain.switchOnCreate = false;
  m_chain.chained.clear();
  m_chain.currentTransfer.reset();
}

int LiveTVPlayback::GetRecorderNum()
{
  ProtoRecorderPtr recorder(m_recorder);
  return (recorder ? recorder->GetNum() : 0);
}

bool LiveTVPlayback::IsChained(const Program& program)
{
  for (chained_t::const_iterator it = m_chain.chained.begin(); it != m_chain.chained.end(); ++it)
  {
    if (it->first && it->first->GetPathName() == program.fileName)
      return true;
  }
  return false;
}

void LiveTVPlayback::HandleChainUpdate()
{
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  ProtoRecorderPtr recorder(m_recorder);
  if (!recorder)
    return;
  ProgramPtr prog = recorder->GetCurrentRecording();
  /*
   * If program file doesn't exist in the recorder chain then create a new
   * transfer and add it to the chain.
   */
  if (prog && !prog->fileName.empty() && !IsChained(*prog))
  {
    DBG(MYTH_DBG_DEBUG, "%s: liveTV (%s): adding new transfer %s\n", __FUNCTION__,
            m_chain.UID.c_str(), prog->fileName.c_str());
    ProtoTransferPtr transfer(new ProtoTransfer(recorder->GetServer(), recorder->GetPort(), prog->fileName, prog->recording.storageGroup));
    // Pop previous dummy file if exists then add the new into the chain
    if (m_chain.lastSequence && m_chain.chained[m_chain.lastSequence - 1].first->fileSize == 0)
    {
      --m_chain.lastSequence;
      m_chain.chained.pop_back();
    }
    m_chain.chained.push_back(std::make_pair(transfer, prog));
    m_chain.lastSequence = m_chain.chained.size();
    /*
     * If switchOnCreate flag and file is filled then switch immediatly.
     * Else we will switch later on the next event 'UPDATE_FILE_SIZE'
     */
    if (m_chain.switchOnCreate && transfer->fileSize > 0 && SwitchChainLast())
      m_chain.switchOnCreate = false;
    m_chain.watch = false; // Chain update done. Restore watch flag
    DBG(MYTH_DBG_DEBUG, "%s: liveTV (%s): chain last (%u), watching (%u)\n", __FUNCTION__,
            m_chain.UID.c_str(), m_chain.lastSequence, m_chain.currentSequence);
  }
}

bool LiveTVPlayback::SwitchChain(unsigned sequence)
{
  PLATFORM::CLockObject lock(*m_mutex);
  // Check for out of range
  if (sequence < 1 || sequence > m_chain.lastSequence)
    return false;
  // If closed then try to open
  if (!m_chain.chained[sequence - 1].first->IsOpen() && !m_chain.chained[sequence - 1].first->Open())
    return false;
  m_chain.currentTransfer = m_chain.chained[sequence - 1].first;
  m_chain.currentSequence = sequence;
  DBG(MYTH_DBG_DEBUG, "%s: switch to file (%u) %s\n", __FUNCTION__,
          (unsigned)m_chain.currentTransfer->GetFileId(), m_chain.currentTransfer->GetPathName().c_str());
  return true;
}

bool LiveTVPlayback::SwitchChainLast()
{
  if (SwitchChain(m_chain.lastSequence))
  {
    ProtoRecorderPtr recorder(m_recorder);
    ProtoTransferPtr transfer(m_chain.currentTransfer);
    if (recorder && transfer && recorder->TransferSeek(*transfer, 0, WHENCE_SET) == 0)
      return true;
  }
  return false;
}

void LiveTVPlayback::HandleBackendMessage(const EventMessage& msg)
{
  ProtoRecorderPtr recorder(m_recorder);
  if (!recorder || !recorder->IsPlaying())
    return;
  switch (msg.event)
  {
    /*
     * Event: LIVETV_CHAIN UPDATE
     *
     * Called in response to the backend's notification of a chain update.
     * The recorder is supplied and will be queried for the current recording
     * to determine if a new file needs to be added to the chain of files
     * in the live tv instance.
     */
    case EVENT_LIVETV_CHAIN:
      if (msg.subject.size() >= 3)
      {
        if (msg.subject[1] == "UPDATE" && msg.subject[2] == m_chain.UID)
          HandleChainUpdate();
      }
      break;
    /*
     * Event: LIVETV_WATCH
     *
     * Called in response to the backend's notification of a livetv watch.
     * The recorder is supplied and will be updated for the watch signal.
     * This event is used to manage program breaks while watching live tv.
     * When the guide data marks the end of one show and the beginning of
     * the next, which will be recorded to a new file, this instructs the
     * frontend to terminate the existing playback, and change channel to
     * the new file. Before updating livetv chain and switching to new file
     * we must to wait for event DONE_RECORDING that informs the current
     * show is completed. Then we will call livetv chain update to get
     * current program info. Watch signal will be down during this period.
     */
    case EVENT_LIVETV_WATCH:
      if (msg.subject.size() >= 3)
      {
        int32_t rnum;
        int8_t flag;
        if (str2int32(msg.subject[1].c_str(), &rnum) == 0 && str2int8(msg.subject[2].c_str(), &flag) == 0)
        {
          if (recorder->GetNum() == (int)rnum)
          {
            PLATFORM::CLockObject lock(*m_mutex); // Lock chain
            m_chain.watch = true;
          }
        }
      }
      break;
    /*
     * Event: DONE_RECORDING
     *
     * Indicates that an active recording has completed on the specified
     * recorder. used to manage program breaks while watching live tv.
     * When receive event for recorder, we force an update of livetv chain
     * to get current program info when chain is not yet updated.
     * Watch signal is used when up, to mark the break period and
     * queuing the frontend for reading file buffer.
     */
    case EVENT_DONE_RECORDING:
      if (msg.subject.size() >= 2)
      {
        int32_t rnum;
        if (str2int32(msg.subject[1].c_str(), &rnum) == 0 && recorder->GetNum() == (int)rnum)
        {
          // Recorder is not subscriber. So callback event to it
          recorder->DoneRecordingCallback();
          // Manage program break
          if (m_chain.watch)
          {
            /*
             * Last recording is now completed but watch signal is ON.
             * Then force live tv chain update for the new current
             * program. We will retry 3 times before returning.
             */
            for (int i = 0; i < 3; ++i)
            {
              HandleChainUpdate();
              if (!m_chain.watch)
                break;
              usleep(100000); // waiting 100 ms
            }
          }
        }
      }
      break;
    case EVENT_UPDATE_FILE_SIZE:
      if (msg.subject.size() >= 4)
      {
        PLATFORM::CLockObject lock(*m_mutex); // Lock chain
        if (m_chain.lastSequence > 0)
        {
          int64_t newsize;
          uint32_t chanid;
          time_t startts;
          if (str2uint32(msg.subject[1].c_str(), &chanid) == 0
                  && str2time(msg.subject[2].c_str(), &startts) == 0
                  && m_chain.chained[m_chain.lastSequence -1].second->channel.chanId == chanid
                  && m_chain.chained[m_chain.lastSequence -1].second->recording.startTs == startts
                  && str2int64(msg.subject[3].c_str(), &newsize) == 0
                  && m_chain.chained[m_chain.lastSequence - 1].first->fileSize < newsize)
          {
            // Update transfer file size
            m_chain.chained[m_chain.lastSequence - 1].first->fileSize = newsize;
            // Is wait the filling before switching ?
            if (m_chain.switchOnCreate && SwitchChainLast())
              m_chain.switchOnCreate = false;
            DBG(MYTH_DBG_DEBUG, "%s: liveTV (%s): chain last (%u) filesize %" PRIi64 "\n", __FUNCTION__,
                    m_chain.UID.c_str(), m_chain.lastSequence, newsize);
          }
        }
      }
      break;
    case EVENT_SIGNAL:
      if (msg.subject.size() >= 2)
      {
        int32_t rnum;
        if (str2int32(msg.subject[1].c_str(), &rnum) == 0 && recorder->GetNum() == (int)rnum)
          m_signal = msg.signal;
      }
      break;
    //case EVENT_HANDLER_STATUS:
    //  if (msg.subject[0] == EVENTHANDLER_DISCONNECTED)
    //    closeTransfer();
    //  break;
    default:
      break;
  }
}

int64_t LiveTVPlayback::GetSize() const
{
  int64_t size = 0;
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  for (chained_t::const_iterator it = m_chain.chained.begin(); it != m_chain.chained.end(); ++it)
    size += it->first->fileSize;
  return size;
}

int LiveTVPlayback::Read(void* buffer, unsigned n)
{
  int r = 0;
  bool retry;
  int64_t s, fs, rp;

  // Begin critical section
  // First of all i hold my shared resources using copies
  ProtoRecorderPtr recorder(m_recorder);
  if (!m_chain.currentTransfer || !recorder)
    return -1;

  do
  {
    retry = false;
    fs = m_chain.currentTransfer->fileSize;  // Current known fileSize
    s = fs - m_chain.currentTransfer->filePosition; // Acceptable block size
    if (s == 0)
    {
      // Reading ahead
      if (m_chain.currentSequence == m_chain.lastSequence)
      {
        if ((rp = recorder->GetFilePosition()) > fs)
        {
          PLATFORM::CLockObject lock(*m_mutex); // Lock chain
          m_chain.currentTransfer->fileSize = rp;
          retry = true;
        }
        else
        {
          DBG(MYTH_DBG_WARN, "%s: read position is ahead (%" PRIi64 ")\n", __FUNCTION__, fs);
          usleep(100000); // timeshift +100ms
          return 0;
        }
      }
      // Switch next file transfer is required to continue
      else
      {
        if (!SwitchChain(m_chain.currentSequence + 1))
          return -1;
        if (m_chain.currentTransfer->filePosition != 0)
          recorder->TransferSeek(*(m_chain.currentTransfer), 0, WHENCE_SET);
        retry = true;
        DBG(MYTH_DBG_DEBUG, "%s: liveTV (%s): chain last (%u), watching (%u)\n", __FUNCTION__,
              m_chain.UID.c_str(), m_chain.lastSequence, m_chain.currentSequence);
      }
    }
    else if (s < 0)
      return -1;
  }
  while (retry);

  if (s < (int64_t)n)
    n = (unsigned)s ;

  r = recorder->TransferRequestBlock(*(m_chain.currentTransfer), buffer, n);
  return r;
}

int64_t LiveTVPlayback::Seek(int64_t offset, WHENCE_t whence)
{
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  if (!m_recorder || !m_chain.currentSequence)
    return -1;

  unsigned ci = m_chain.currentSequence - 1; // current sequence index
  int64_t size = GetSize(); // total stream size
  int64_t position = GetPosition(); // absolute position in stream
  int64_t p = 0;
  switch (whence)
  {
  case WHENCE_SET:
    p = offset;
    break;
  case WHENCE_END:
    p = size + offset;
    break;
  case WHENCE_CUR:
    p = position + offset;
    break;
  default:
    return -1;
  }
  if (p > size || p < 0)
  {
    DBG(MYTH_DBG_WARN, "%s: invalid seek (%" PRId64 ")\n", __FUNCTION__, p);
    return -1;
  }
  if (p > position)
  {
    for (;;)
    {
      if (position - m_chain.chained[ci].first->filePosition + m_chain.chained[ci].first->fileSize >= p)
      {
        // Try seek file to desired position. On success switch chain
        if (m_recorder->TransferSeek(*(m_chain.chained[ci].first), p - position, WHENCE_CUR) < 0 ||
                !SwitchChain(++ci))
          return -1;
        return p;
      }
      position += m_chain.chained[ci].first->fileSize - m_chain.chained[ci].first->filePosition;
      ++ci; // switch next
      if (ci < m_chain.lastSequence)
        position += m_chain.chained[ci].first->filePosition;
      else
        return -1;
    }
  }
  if (p < position)
  {
    for (;;)
    {
      if (position - m_chain.chained[ci].first->filePosition <= p)
      {
        // Try seek file to desired position. On success switch chain
        if (m_recorder->TransferSeek(*(m_chain.chained[ci].first), p - position, WHENCE_CUR) < 0 ||
                !SwitchChain(++ci))
          return -1;
        return p;
      }
      position -= m_chain.chained[ci].first->filePosition;
      if (ci > 0)
      {
        --ci; // switch previous
        position -= m_chain.chained[ci].first->fileSize - m_chain.chained[ci].first->filePosition;
      }
      else
        return -1;
    }
  }
  // p == position
  return p;
}

int64_t LiveTVPlayback::GetPosition() const
{
  int64_t pos = 0;
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  if (m_chain.currentSequence)
  {
    unsigned s = m_chain.currentSequence - 1;
    for (unsigned i = 0; i < s; ++i)
      pos += m_chain.chained[i].first->fileSize;
    pos += m_chain.currentTransfer->filePosition;
  }
  return pos;
}

bool LiveTVPlayback::IsPlaying() const
{
  ProtoRecorderPtr recorder(m_recorder);
  return (recorder ? recorder->IsPlaying() : false);
}

bool LiveTVPlayback::IsLiveRecording() const
{
  ProtoRecorderPtr recorder(m_recorder);
  return (recorder ? recorder->IsLiveRecording() : false);
}

bool LiveTVPlayback::KeepLiveRecording(bool keep)
{
  ProtoRecorderPtr recorder(m_recorder);
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  if (recorder && recorder->IsPlaying())
  {
    ProgramPtr prog = recorder->GetCurrentRecording();
    if (prog)
    {
      if (keep)
      {
        if (UndeleteRecording(*prog) && recorder->SetLiveRecording(keep))
        {
          QueryGenpixmap(*prog);
          return true;
        }
      }
      else
      {
        if (recorder->SetLiveRecording(keep) && recorder->FinishRecording())
          return true;
      }
    }
  }
  return false;
}

ProgramPtr LiveTVPlayback::GetPlayedProgram() const
{
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  if (m_chain.currentSequence > 0)
    return m_chain.chained[m_chain.currentSequence - 1].second;
  return ProgramPtr();
}

time_t LiveTVPlayback::GetLiveTimeStart() const
{
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  if (m_chain.lastSequence)
    return m_chain.chained[0].second->recording.startTs;
  return (time_t)(-1);
}

unsigned LiveTVPlayback::GetChainedCount() const
{
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  return m_chain.lastSequence;
}

ProgramPtr LiveTVPlayback::GetChainedProgram(unsigned sequence) const
{
  PLATFORM::CLockObject lock(*m_mutex); // Lock chain
  if (sequence > 0 && sequence <= m_chain.lastSequence)
    return m_chain.chained[sequence - 1].second;
  return ProgramPtr();
}

uint32_t LiveTVPlayback::GetCardId() const
{
  ProtoRecorderPtr recorder(m_recorder);
  return (recorder ? recorder->GetNum() : 0);
}

SignalStatusPtr LiveTVPlayback::GetSignal() const
{
  return (m_recorder ? m_signal : SignalStatusPtr());
}
