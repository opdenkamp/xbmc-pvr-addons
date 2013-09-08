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

#include "MythRecorder.h"
#include "MythProgramInfo.h"
#include "MythChannel.h"
#include "MythPointer.h"
#include "../client.h"

using namespace ADDON;

MythRecorder::MythRecorder()
  : m_recorder_t(new MythPointerThreadSafe<cmyth_recorder_t>())
  , m_liveChainUpdated(new int(0))
  , m_conn()
{
}

MythRecorder::MythRecorder(cmyth_recorder_t cmyth_recorder, const MythConnection &conn)
  : m_recorder_t(new MythPointerThreadSafe<cmyth_recorder_t>())
  , m_liveChainUpdated(new int(0))
  , m_conn(conn)
{
  *m_recorder_t = cmyth_recorder;
}

bool MythRecorder::IsNull() const
{
  if (m_recorder_t == NULL)
    return true;
  return *m_recorder_t == NULL;
}

void MythRecorder::Lock()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "Recorder Lock %u", m_recorder_t.get());
  m_recorder_t->Lock();
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "Recorder Lock acquired %u", m_recorder_t.get());
}

void MythRecorder::Unlock()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "Recorder Unlock %u", m_recorder_t.get());
  m_recorder_t->Unlock();
}

unsigned int MythRecorder::ID()
{
  unsigned int retval = 0;
  Lock();
  retval = cmyth_recorder_get_recorder_id(*m_recorder_t);
  Unlock();
  return retval;
}

MythProgramInfo MythRecorder::GetCurrentProgram()
{
  cmyth_proginfo_t proginfo = NULL;
  Lock();
  proginfo = cmyth_recorder_get_cur_proginfo(*m_recorder_t);
  Unlock();
  return MythProgramInfo(proginfo);
}

bool MythRecorder::IsRecording()
{
  int retval = 0;
  Lock();
  retval = cmyth_recorder_is_recording(*m_recorder_t);
  Unlock();
  return retval == 1;
}

bool MythRecorder::CancelNextRecording(bool cancel)
{
  int retval = 0;
  Lock();
  retval = cmyth_recorder_cancel_next_recording(*m_recorder_t, cancel ? 1 : 0);
  Unlock();
  return retval == 1;
}

bool MythRecorder::IsTunable(MythChannel &channel)
{
  Lock();

  XBMC->Log(LOG_DEBUG, "%s: called for recorder %u, channel %u", __FUNCTION__, ID(), channel.ID());

  cmyth_inputlist_t inputlist = cmyth_get_free_inputlist(*m_recorder_t);

  bool ret = false;
  for (int i = 0; i < inputlist->input_count; ++i)
  {
    cmyth_input_t input = inputlist->input_list[i];
    if (input->sourceid != channel.SourceID())
    {
      XBMC->Log(LOG_DEBUG, "%s: skip input, source id differs (channel: %u, input: %u)", __FUNCTION__, channel.SourceID(), input->sourceid);
      continue;
    }

    if (input->multiplexid && input->multiplexid != channel.MultiplexID())
    {
      XBMC->Log(LOG_DEBUG, "%s: skip input, multiplex id id differs (channel: %u, input: %u)", __FUNCTION__, channel.MultiplexID(), input->multiplexid);
      continue;
    }

    XBMC->Log(LOG_DEBUG,"%s: using recorder, input is tunable: source id: %u, multiplex id: channel: %u, input: %u)", __FUNCTION__, channel.SourceID(), channel.MultiplexID(), input->multiplexid);

    ret = true;
    break;
  }

  ref_release(inputlist);
  Unlock();

  if (!ret)
  {
    XBMC->Log(LOG_DEBUG,"%s: recorder is not tunable", __FUNCTION__);
  }
  return ret;
}

bool MythRecorder::CheckChannel(MythChannel &channel)
{
  int retval = 0;
  Lock();
  retval = cmyth_recorder_check_channel(*m_recorder_t, const_cast<char*>(channel.Number().c_str()));
  Unlock();
  return retval == 1;
}

bool MythRecorder::SpawnLiveTV(MythChannel &channel)
{
  bool ret = false;
  char* pErr = NULL;

  Lock();

  // Check channel
  *m_liveChainUpdated = 0;
  cmyth_recorder_t recorder = NULL;
  recorder = cmyth_spawn_live_tv(*m_recorder_t, RCV_BUF_DATA_SIZE, TCP_RCV_BUF_DATA_SIZE, MythRecorder::prog_update_callback, &pErr, const_cast<char*>(channel.Number().c_str()));

  if (recorder && pErr == NULL) {
    *m_recorder_t = recorder;
    // Wait for chain update for 30s before break
    int i = 0;
    while (*m_liveChainUpdated == 0 && i < 30000) {
      // Release the latch to allow chain update
      Unlock();
      usleep(100000);
      // Gets the latch before read chain status
      Lock();
      i += 100;
      XBMC->Log(LOG_DEBUG, "%s: Delay channel switch: %d", __FUNCTION__, i);
    }
    if (*m_liveChainUpdated == 0)
    {
      XBMC->Log(LOG_ERROR,"%s - Chain update failed", __FUNCTION__);
      XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30304)); // No response from MythTV backend
    }
    else
      ret = true;
  }
  else
  {
    // Dereference recorder to cancel callback
    *m_recorder_t = NULL;
    // Chain setup fails. Stop existing recorder
    ref_release(recorder);
    if (pErr)
      XBMC->Log(LOG_ERROR,"%s - %s", __FUNCTION__, pErr);
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30306)); // Recorder unavailable
  }

  Unlock();

  return ret;
}

bool MythRecorder::SetChannel(MythChannel &channel)
{
  Lock();

  CStdString channelNum = channel.Number();

  if (cmyth_recorder_pause(*m_recorder_t) != 0)
  {
    XBMC->Log(LOG_ERROR, "%s: Failed to pause recorder %u", __FUNCTION__, ID());
    Unlock();
    return false;
  }

  *m_liveChainUpdated = 0;

  if (cmyth_recorder_set_channel(*m_recorder_t,channelNum.GetBuffer()) != 0)
  {
    XBMC->Log(LOG_ERROR, "%s: Failed to change recorder %u to channel %s", __FUNCTION__, ID(), channel.Name().c_str());
    Unlock();
    return false;
  }

  // Wait for chain update for 30s before break
  int i = 0;
  while (*m_liveChainUpdated == 0 && i < 30000) {
    // Release the latch to allow chain update
    Unlock();
    usleep(100000);
    // Get the latch before reading chain status
    Lock();
    i += 100;
    XBMC->Log(LOG_DEBUG, "%s: Delay channel switch: %d", __FUNCTION__, i);
  }
  if (*m_liveChainUpdated == 0)
  {
    XBMC->Log(LOG_ERROR,"%s - Chain update failed", __FUNCTION__);
    Unlock();
    return false;
  }

  Unlock();

  return true;
}

/* JLB
* Manage program breaks.
* Trigger on event LIVETV_WATCH
*/
bool MythRecorder::LiveTVWatch(const CStdString &msg)
{
  int retval = 0;
  Lock();
  retval = cmyth_livetv_watch(*m_recorder_t, const_cast<char*>(msg.c_str()));
  Unlock();
  if (retval != 0)
    XBMC->Log(LOG_ERROR, "LiveTVWatch failed: %s", msg.c_str());
  return retval >= 0;
}

/* JLB
* Manage program breaks.
* Trigger on event DONE_RECORDING
*/
bool MythRecorder::LiveTVDoneRecording(const CStdString &msg)
{
  int retval = 0;
  Lock();
  retval = cmyth_livetv_done_recording(*m_recorder_t, const_cast<char*>(msg.c_str()));
  Unlock();
  if (retval != 0)
    XBMC->Log(LOG_ERROR, "LiveTVDoneRecording failed: %s", msg.c_str());
  return retval >= 0;
}

bool MythRecorder::LiveTVChainUpdate(const CStdString &chainid)
{
  int retval = 0;
  /* Gets latch to update the chain */
  Lock();
  retval = cmyth_livetv_chain_update(*m_recorder_t, const_cast<char*>(chainid.c_str()));
  /* Releases latch */
  Unlock();
  if (retval != 0)
    XBMC->Log(LOG_ERROR,"LiveTVChainUpdate failed on chainID: %s", chainid.c_str());
  else
    *m_liveChainUpdated = 1;
  return retval >= 0;
}

int MythRecorder::ReadLiveTV(void *buffer, unsigned int length)
{
  /* Unlocked: LiveTV stream has its own control connection */
  int bytesRead = 0;
  bytesRead = cmyth_livetv_read(*m_recorder_t, static_cast<char*>(buffer), length);
  return bytesRead;
}

long long MythRecorder::LiveTVSeek(long long offset, int whence)
{
  /* Unlocked: LiveTV stream has its own control connection */
  long long retval = 0;
  retval = cmyth_livetv_seek(*m_recorder_t, offset, whence);
  return retval;
}

long long MythRecorder::LiveTVDuration()
{
  long long retval = 0;
  Lock();
  retval = cmyth_livetv_chain_duration(*m_recorder_t);
  Unlock();
  return retval;
}

bool MythRecorder::Stop()
{
  int retval = 0;
  Lock();
  retval = cmyth_recorder_stop_livetv(*m_recorder_t);
  Unlock();
  return retval >= 0;
}

void MythRecorder::prog_update_callback(cmyth_proginfo_t prog)
{
  (void)prog;
  XBMC->Log(LOG_DEBUG,"prog_update_callback");
}
