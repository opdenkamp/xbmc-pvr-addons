/*
 *      Copyright (C) 2005-2014 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Tvheadend.h"

#include "platform/util/util.h"

extern "C" {
#include "platform/util/atomic.h"
#include "libhts/htsmsg_binary.h"
}

#define UPDATE(x, y)\
if ((x) != (y))\
{\
  (x) = (y);\
  update = true;\
}

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

CTvheadend::CTvheadend()
  : m_dmx(m_conn), m_vfs(m_conn), m_asyncState(ASYNC_NONE),
    m_asyncComplete(false)
{
}

CTvheadend::~CTvheadend()
{
}

/* **************************************************************************
 * Miscellaneous
 * *************************************************************************/

PVR_ERROR CTvheadend::GetDriveSpace ( long long *total, long long *used )
{
  int64_t s64;
  CLockObject lock(m_conn.Mutex());

  htsmsg_t *m = htsmsg_create_map();
  m = m_conn.SendAndWait("getDiskSpace", m);
  if (m == NULL)
  {
    tvherror("failed to get disk space");
    return PVR_ERROR_SERVER_ERROR;
  }

  if (htsmsg_get_s64(m, "totaldiskspace", &s64))
    goto error;
  *total = s64 / 1024;

  if (htsmsg_get_s64(m, "freediskspace", &s64))
    goto error;
  *used  = *total - (s64 / 1024);

  return PVR_ERROR_NO_ERROR;

error:
  tvherror("malformed getDiskSpace response");
  return PVR_ERROR_SERVER_ERROR;
}

CStdString CTvheadend::GetImageURL ( const char *str )
{
  if (*str != '/')
    return str;
  else
  {
    return m_conn.GetWebURL("%s", str);
  }
}

/* **************************************************************************
 * Tags
 * *************************************************************************/

int CTvheadend::GetTagCount ( void )
{
  CLockObject lock(m_mutex);
  return m_tags.size();
}

PVR_ERROR CTvheadend::GetTags ( ADDON_HANDLE handle, bool _unused(radio) )
{
  CLockObject lock(m_mutex);
  STags::iterator it;
  for (it = m_tags.begin(); it != m_tags.end(); it++)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(tag));
    
    tag.bIsRadio = false;
    strncpy(tag.strGroupName, it->second.name.c_str(),
            sizeof(tag.strGroupName));

    PVR->TransferChannelGroup(handle, &tag);
  }
  
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CTvheadend::GetTagMembers
  ( ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group )
{
  CLockObject lock(m_mutex);
  vector<uint32_t>::iterator it;
  SChannels::iterator cit;
  STags::iterator     tit = m_tags.begin();
  while (tit != m_tags.end())
  {
    if (tit->second.name == group.strGroupName)
    {
      for (it = tit->second.channels.begin();
           it != tit->second.channels.end(); it++)
      {
        if ((cit = m_channels.find(*it)) != m_channels.end())
        {
          PVR_CHANNEL_GROUP_MEMBER gm;
          memset(&gm, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
          gm.iChannelUniqueId = cit->second.id;
          gm.iChannelNumber   = cit->second.num;
          PVR->TransferChannelGroupMember(handle, &gm);
        }
      }
      break;
    }
    tit++;
  }

  return PVR_ERROR_NO_ERROR;
}

/* **************************************************************************
 * Channels
 * *************************************************************************/

int CTvheadend::GetChannelCount ( void )
{
  CLockObject lock(m_mutex);
  return m_channels.size();
}

PVR_ERROR CTvheadend::GetChannels ( ADDON_HANDLE handle, bool radio )
{
  CLockObject lock(m_mutex);
  SChannels::iterator it;
  for (it = m_channels.begin(); it != m_channels.end(); it++)
  {
    if (radio != it->second.radio)
      continue;

    PVR_CHANNEL chn;
    memset(&chn, 0 , sizeof(PVR_CHANNEL));

    chn.iUniqueId         = it->second.id;
    chn.bIsRadio          = it->second.radio;
    chn.iChannelNumber    = it->second.num;
    chn.iEncryptionSystem = it->second.caid;
    chn.bIsHidden         = false;
    strncpy(chn.strChannelName, it->second.name.c_str(),
            sizeof(chn.strChannelName) - 1);
    strncpy(chn.strIconPath, it->second.icon.c_str(),
            sizeof(chn.strIconPath) - 1);

    PVR->TransferChannelEntry(handle, &chn);
  }

  return PVR_ERROR_NO_ERROR;
}

/* **************************************************************************
 * Recordings
 * *************************************************************************/

PVR_ERROR CTvheadend::SendDvrDelete ( uint32_t id, const char *method )
{
  const char *str;
  uint32_t u32;

  CLockObject lock(m_conn.Mutex());

  /* Build message */
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_u32(m, "id", id);

  /* Send and wait a bit longer than usual */
  if ((m = m_conn.SendAndWait(method, m, 30)) == NULL)
  {
    tvherror("failed to cancel/delete DVR entry");
    return PVR_ERROR_SERVER_ERROR;
  }

  /* Check for error */
  if ((str = htsmsg_get_str(m, "error")) != NULL)
  {
    tvherror("failed to cancel/delete DVR entry [%s]", str);
  }
  else if (htsmsg_get_u32(m, "success", &u32))
  {
    tvherror("failed to parse cancelDvrEntry response");
  }
  htsmsg_destroy(m);

  return u32 > 0  ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}

PVR_ERROR CTvheadend::SendDvrUpdate
  ( uint32_t id, const CStdString &title, time_t start, time_t stop )
{
  const char *str;
  uint32_t u32;

  /* Build message */
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_u32(m, "id", id);
  htsmsg_add_str(m, "title", title.c_str());
  if (start)
    htsmsg_add_s64(m, "start", start);
  if (stop)
    htsmsg_add_s64(m, "stop",  stop);

  /* Send and Wait */
  if ((m = m_conn.SendAndWait("updateDvrEntry", m)) == NULL)
  {
    tvherror("failed to update DVR entry");
    return PVR_ERROR_SERVER_ERROR;
  }

  /* Check for error */
  if ((str = htsmsg_get_str(m, "error")) != NULL)
  {
    tvherror("failed to update DVR entry [%s]", str);
  }
  else if (htsmsg_get_u32(m, "success", &u32))
  {
    tvherror("failed to parse updateDvrEntry response");
  }
  htsmsg_destroy(m);

  return u32 > 0  ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}

int CTvheadend::GetRecordingCount ( void )
{
  int ret = 0;
  SRecordings::const_iterator rit;
  CLockObject lock(m_mutex);
  for (rit = m_recordings.begin(); rit != m_recordings.end(); rit++)
    if (rit->second.IsRecording())
      ret++;
  return ret;
}

PVR_ERROR CTvheadend::GetRecordings ( ADDON_HANDLE handle )
{
  CLockObject lock(m_mutex);
  SRecordings::const_iterator rit;
  SChannels::const_iterator cit;
  char buf[128];

  for (rit = m_recordings.begin(); rit != m_recordings.end(); rit++)
  {
    if (!rit->second.IsRecording()) continue;

    /* Setup entry */
    PVR_RECORDING rec;
    memset(&rec, 0, sizeof(rec));

    /* Channel name and icon */
    if ((cit = m_channels.find(rit->second.channel)) != m_channels.end())
    {
      strncpy(rec.strChannelName, cit->second.name.c_str(),
              sizeof(rec.strChannelName));
      
      strncpy(rec.strIconPath, cit->second.icon.c_str(),
              sizeof(rec.strIconPath));
    }

    /* URL ( HTSP < v7 ) */
    // TODO: do I care!

    /* ID */
    snprintf(buf, sizeof(buf), "%i", rit->second.id);
    strncpy(rec.strRecordingId, buf, sizeof(rec.strRecordingId));
    
    /* Title */
    strncpy(rec.strTitle, rit->second.title.c_str(), sizeof(rec.strTitle));

    /* Description */
    strncpy(rec.strPlot, rit->second.description.c_str(), sizeof(rec.strPlot));
  
    /* Time/Duration */
    rec.recordingTime = (time_t)rit->second.start;
    rec.iDuration     = (time_t)(rit->second.stop - rit->second.start);

    /* Directory */
    if (rit->second.path != "")
    {
      size_t idx = rit->second.path.rfind("/");
      if (idx == 0 || idx == string::npos)
        strncpy(rec.strDirectory, "/", sizeof(rec.strDirectory));
      else
      {
        CStdString d = rit->second.path.substr(0, idx);
        if (d[0] != '/')
          d = "/" + d;
        strncpy(rec.strDirectory, d.c_str(), sizeof(rec.strDirectory));  
      }
    }

    /* Transfer */
    PVR->TransferRecordingEntry(handle, &rec);
  }

  return PVR_ERROR_NO_ERROR;
}

#ifndef OPENELEC_32
PVR_ERROR CTvheadend::GetRecordingEdl
  ( const PVR_RECORDING &rec, PVR_EDL_ENTRY edl[], int *num )
{
  /* Not supported */
  if (m_conn.GetProtocol() < 12)
    return PVR_ERROR_NOT_IMPLEMENTED;
  
  CLockObject lock(m_mutex);
  htsmsg_t *list;
  htsmsg_field_t *f;
  int idx;
  
  /* Build request */
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_u32(m, "id", atoi(rec.strRecordingId));
  
  tvhdebug("dvr get cutpoints id=%s", rec.strRecordingId);

  /* Send and Wait */
  if ((m = m_conn.SendAndWait("getDvrCutpoints", m)) == NULL)
  {
    tvherror("failed to update DVR entry");
    return PVR_ERROR_SERVER_ERROR;
  }

  /* Validate */
  if (!(list = htsmsg_get_list(m, "cutpoints")))
  {
    tvherror("malformed getDvrCutpoints response");
    return PVR_ERROR_FAILED;
  }

  /* Process */
  idx = 0;
  HTSMSG_FOREACH(f, list)
  {
    uint32_t start, end, type;

    if (f->hmf_type != HMF_MAP)
      continue;
  
    /* Full */
    if (idx >= *num)
      break;

    /* Get fields */
    if (htsmsg_get_u32(&f->hmf_msg, "start", &start) ||
        htsmsg_get_u32(&f->hmf_msg, "end",   &end)   ||
        htsmsg_get_u32(&f->hmf_msg, "type",  &type))
    {
      tvherror("malformed EDL entry, will ignore");
      continue;
    }

    /* Build entry */
    edl[idx].start = start;
    edl[idx].end   = end;
    switch (type)
    {
      case DVR_ACTION_TYPE_CUT:
        edl[idx].type = PVR_EDL_TYPE_CUT;
        break;
      case DVR_ACTION_TYPE_MUTE:
        edl[idx].type = PVR_EDL_TYPE_MUTE;
        break;
      case DVR_ACTION_TYPE_SCENE:
        edl[idx].type = PVR_EDL_TYPE_SCENE;
        break;
      case DVR_ACTION_TYPE_COMBREAK:
      default:
        edl[idx].type = PVR_EDL_TYPE_COMBREAK;
        break;
    }
    idx++;
      
    tvhdebug("edl start:%d end:%d action:%d", start, end, type);
  }
  
  *num = idx;
  return PVR_ERROR_NO_ERROR;
}
#endif

PVR_ERROR CTvheadend::DeleteRecording ( const PVR_RECORDING &rec )
{
  return SendDvrDelete(atoi(rec.strRecordingId), "deleteDvrEntry");
}

PVR_ERROR CTvheadend::RenameRecording ( const PVR_RECORDING &rec )
{
  return SendDvrUpdate(atoi(rec.strRecordingId), rec.strTitle, 0, 0);
}

int CTvheadend::GetTimerCount ( void )
{
  int ret = 0;
  SRecordings::const_iterator rit;
  CLockObject lock(m_mutex);
  for (rit = m_recordings.begin(); rit != m_recordings.end(); rit++)
    if (rit->second.IsTimer())
      ret++;
  return ret;
}

PVR_ERROR CTvheadend::GetTimers ( ADDON_HANDLE handle )
{
  CLockObject lock(m_mutex);
  SRecordings::const_iterator rit;
  SChannels::const_iterator cit;
  CStdString strfmt;

  for (rit = m_recordings.begin(); rit != m_recordings.end(); rit++)
  {
    if (!rit->second.IsTimer()) continue;

    /* Setup entry */
    PVR_TIMER tmr;
    memset(&tmr, 0, sizeof(tmr));

    tmr.iClientIndex      = rit->second.id;
    tmr.iClientChannelUid = rit->second.channel;
    tmr.startTime         = (time_t)rit->second.start;
    tmr.endTime           = (time_t)rit->second.stop;
    strncpy(tmr.strTitle, rit->second.title.c_str(), 
            sizeof(tmr.strTitle) - 1);
    strncpy(tmr.strSummary, rit->second.description.c_str(),
            sizeof(tmr.strSummary) - 1);
    tmr.state             = rit->second.state;
    tmr.iPriority         = 0;     // unused
    tmr.iLifetime         = 0;     // unused
    tmr.bIsRepeating      = false; // unused
    tmr.firstDay          = 0;     // unused
    tmr.iWeekdays         = 0;     // unused
    tmr.iEpgUid           = 0;     // unused
    tmr.iMarginStart      = 0;     // unused
    tmr.iMarginEnd        = 0;     // unused
    tmr.iGenreType        = 0;     // unused
    tmr.iGenreSubType     = 0;     // unused

    PVR->TransferTimerEntry(handle, &tmr);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CTvheadend::AddTimer ( const PVR_TIMER &timer )
{
  const char *str;
  uint32_t u32;
  dvr_prio_t prio;

  CLockObject lock(m_conn.Mutex());

  /* Build message */
  htsmsg_t *m = htsmsg_create_map();
  if (timer.iEpgUid > 0)
  {
    htsmsg_add_u32(m, "eventId",    timer.iEpgUid);
    htsmsg_add_s64(m, "startExtra", timer.iMarginStart);
    htsmsg_add_s64(m, "stopExtra",  timer.iMarginEnd);
  }
  else
  {
    htsmsg_add_str(m, "title",        timer.strTitle);
    htsmsg_add_s64(m, "start",        timer.startTime);
    htsmsg_add_s64(m, "stop",         timer.endTime);
    htsmsg_add_u32(m, "channelId",    timer.iClientChannelUid);
    htsmsg_add_str(m, "description",  timer.strSummary);
  }

  /* Priority */
  if (timer.iPriority > 80)
    prio = DVR_PRIO_IMPORTANT;
  else if (timer.iPriority > 60)
    prio = DVR_PRIO_HIGH;
  else if (timer.iPriority > 40)
    prio = DVR_PRIO_NORMAL;
  else if (timer.iPriority > 20)
    prio = DVR_PRIO_LOW;
  else
    prio = DVR_PRIO_UNIMPORTANT;
  htsmsg_add_u32(m, "priority", (int)prio);
  
  /* Send and Wait */
  if ((m = m_conn.SendAndWait("addDvrEntry", m)) == NULL)
  {
    tvherror("failed to add DVR entry");
    return PVR_ERROR_SERVER_ERROR;
  }

  /* Check for error */
  if ((str = htsmsg_get_str(m, "error")) != NULL)
  {
    tvherror("failed to add DVR entry [%s]", str);
  }
  else if (htsmsg_get_u32(m, "success", &u32))
  {
    tvherror("failed to parse addDvrEntry response");
  }
  htsmsg_destroy(m);

  return u32 > 0  ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}

PVR_ERROR CTvheadend::DeleteTimer
  ( const PVR_TIMER &timer, bool _unused(force) )
{
  return SendDvrDelete(timer.iClientIndex, "cancelDvrEntry");
}

PVR_ERROR CTvheadend::UpdateTimer ( const PVR_TIMER &timer )
{
  return SendDvrUpdate(timer.iClientIndex, timer.strTitle,
                       timer.startTime, timer.endTime);
}

/* **************************************************************************
 * EPG
 * *************************************************************************/

/* Transfer schedule to XBMC */
void CTvheadend::TransferEvent
  ( ADDON_HANDLE handle, const SEvent &event )
{
  /* Build */
  EPG_TAG epg;
  memset(&epg, 0, sizeof(EPG_TAG));
  epg.iUniqueBroadcastId  = event.id;
  epg.strTitle            = event.title.c_str();
  epg.iChannelNumber      = event.channel;
  epg.startTime           = event.start;
  epg.endTime             = event.stop;
  epg.strPlotOutline      = event.summary.c_str();
  epg.strPlot             = event.desc.c_str();
  epg.iGenreType          = event.content & 0xF0;
  epg.iGenreSubType       = event.content & 0x0F;
  epg.firstAired          = event.aired;
  epg.iSeriesNumber       = event.season;
  epg.iEpisodeNumber      = event.episode;
  epg.iEpisodePartNumber  = event.part;

  PVR->TransferEpgEntry(handle, &epg);
}

PVR_ERROR CTvheadend::GetEpg
  ( ADDON_HANDLE handle, const PVR_CHANNEL &chn, time_t start, time_t end )
{
  SSchedules::const_iterator sit;
  SEvents::const_iterator eit;
  htsmsg_t *l;
  htsmsg_field_t *f;

  /* Async transfer */
  if (g_bAsyncEpg)
  {
    CLockObject lock(m_mutex);
    if (!m_asyncCond.Wait(m_mutex, m_asyncComplete, 5000))
      return PVR_ERROR_NO_ERROR;

    sit = m_schedules.find(chn.iUniqueId);
    if (sit != m_schedules.end())
    {
      for (eit = sit->second.events.begin();
          eit != sit->second.events.end(); eit++)
      {
        if (eit->second.start    > end)   continue;
        if (eit->second.stop     < start) continue;
        TransferEvent(handle, eit->second);
      }
    }

  /* Synchronous transfer */
  }
  else
  {
    CLockObject lock(m_conn.Mutex());

    /* Build message */
    htsmsg_t *msg = htsmsg_create_map();
    htsmsg_add_u32(msg, "channelId", chn.iUniqueId);
    htsmsg_add_s64(msg, "maxTime",   end);

    /* Send and Wait */
    if ((msg = m_conn.SendAndWait0("getEvents", msg)) == NULL)
    {
      tvherror("failed to request epg");
      return PVR_ERROR_SERVER_ERROR;
    }

    /* Process */
    if (!(l = htsmsg_get_list(msg, "events")))
    {
      htsmsg_destroy(msg);
      tvherror("malformed getEvents response");
      return PVR_ERROR_SERVER_ERROR;
    }
    HTSMSG_FOREACH(f, l)
    {
      SEvent event;
      if (f->hmf_type == HMF_MAP)
      {
        if (ParseEvent(&f->hmf_msg, event))
          TransferEvent(handle, event);
      }
    }
    htsmsg_destroy(msg);
  }

  return PVR_ERROR_NO_ERROR;
}

/* **************************************************************************
 * Connection
 * *************************************************************************/

void CTvheadend::Disconnected ( void )
{
  CLockObject lock(m_mutex);
  m_asyncComplete = false;
  m_asyncState    = ASYNC_NONE;
}

bool CTvheadend::Connected ( void )
{
  htsmsg_t *msg;
  STags::iterator tit;
  SChannels::iterator cit;

  /* Rebuild state */
  m_dmx.Connected();
  m_vfs.Connected();

  /* Flag all async fields in case they've been deleted */
  for (cit = m_channels.begin(); cit != m_channels.end(); cit++)
    cit->second.del = true;
  for (tit = m_tags.begin(); tit != m_tags.end(); tit++)
    tit->second.del = true;

  /* Request Async data */
  m_asyncComplete = false;
  m_asyncState    = ASYNC_NONE;
  msg = htsmsg_create_map();
  htsmsg_add_u32(msg, "epg",        g_bAsyncEpg);
  //htsmsg_add_u32(msg, "epgMaxTime", 0);
  //htsmsg_add_s64(msg, "lastUpdate", 0);
  if ((msg = m_conn.SendAndWait0("enableAsyncMetadata", msg)) == NULL)
  {
    tvherror("failed to request async updates");
    return false;
  }
  htsmsg_destroy(msg);
  tvhdebug("async updates requested");

  return true;
}

/* **************************************************************************
 * Message handling
 * *************************************************************************/

bool CTvheadend::ProcessMessage ( const char *method, htsmsg_t *msg )
{
  /* Demuxer */
  if (m_dmx.ProcessMessage(method, msg))
    return true;

  /* VFS */
  if (m_vfs.ProcessMessage(method, msg))
    return true;

  /* Lock */
  CLockObject lock(m_mutex);

  /* Channels */
  if (!strcmp("channelAdd", method) || !strcmp("channelUpdate", method))
    ParseChannelUpdate(msg);
  else if (!strcmp("channelDelete", method))
    ParseChannelDelete(msg);

  /* Tags */
  else if (!strcmp("tagAdd", method) || !strcmp("tagUpdate", method))
    ParseTagUpdate(msg);
  else if (!strcmp("tagDelete", method))
    ParseTagDelete(msg);

  /* Recordings */
  else if (!strcmp("dvrEntryAdd", method) || !strcmp("dvrEntryUpdate", method))
    ParseRecordingUpdate(msg);
  else if (!strcmp("dvrEntryDelete", method))
    ParseRecordingDelete(msg);

  /* EPG */
  else if (!strcmp("eventAdd", method) || !strcmp("eventUpdate", method))
    ParseEventUpdate(msg);
  else if (!strcmp("eventDelete", method))
    ParseEventDelete(msg);

  /* ASync complete */
  else if (!strcmp("initialSyncCompleted", method))
    SyncCompleted();

  /* Unknown */
  else  
    tvhdebug("unhandled message [%s]", method);

  /* Local */
  return true;
}

void CTvheadend::SyncCompleted ( void )
{
  /* The complete calls are probably redundant, but its a safety feature */
  SyncChannelsCompleted();
  SyncDvrCompleted();
  SyncEpgCompleted();
  m_asyncComplete = true;
  m_asyncState    = ASYNC_DONE;
  m_asyncCond.Broadcast();
}

void CTvheadend::SyncChannelsCompleted ( void )
{
  /* Already done */
  if (m_asyncComplete || m_asyncState > ASYNC_CHN)
    return;

  bool update;
  SChannels::iterator   cit = m_channels.begin();
  STags::iterator       tit = m_tags.begin();

  /* Tags */
  update = false;
  while (tit != m_tags.end())
  {
    if (tit->second.del)
    {
      update = true;
      m_tags.erase(tit);
    }
    tit++;
  }
  PVR->TriggerChannelGroupsUpdate();
  if (update)
    tvhinfo("tags updated");

  /* Channels */
  update = false;
  while (cit != m_channels.end())
  {
    if (cit->second.del)
    {
      update = true;
      m_channels.erase(cit);
    }
    cit++;
  }
  PVR->TriggerChannelUpdate();
  if (update)
    tvhinfo("channels updated");
  
  /* Next */
  m_asyncState = ASYNC_DVR;
}

void CTvheadend::SyncDvrCompleted ( void )
{
  /* Done */
  if (m_asyncComplete || m_asyncState > ASYNC_DVR)
    return;

  bool update;
  SRecordings::iterator rit = m_recordings.begin();

  /* Recordings */
  update = false;
  while (rit != m_recordings.end())
  {
    if (rit->second.del)
    {
      update = true;
      m_recordings.erase(rit);
    }
    rit++;
  }
  PVR->TriggerRecordingUpdate();
  PVR->TriggerTimerUpdate();
  if (update)
    tvhinfo("recordings updated");

  /* Next */
  m_asyncState = ASYNC_EPG;
}

void CTvheadend::SyncEpgCompleted ( void )
{
  /* Done */
  if (!g_bAsyncEpg || m_asyncComplete || m_asyncState > ASYNC_EPG)
    return;
  
  bool update;
  SSchedules::iterator  sit = m_schedules.begin();
  SEvents::iterator     eit;

  /* Events */
  update = false;
  while (sit != m_schedules.end())
  {
    if (sit->second.del)
    {
      update = true;
      m_schedules.erase(sit);
    }
    else
    {
      eit = sit->second.events.begin();
      while (eit != sit->second.events.end())
      {
        if (eit->second.del)
        {
          update = true;
          sit->second.events.erase(eit);
        }
        eit++;
      }
    }
    sit++;
  }
#if TODO_FIXME
  PVR->TriggerEventUpdate();
#endif
  if (update)
    tvhinfo("epg updated");
}

void CTvheadend::ParseTagUpdate ( htsmsg_t *msg )
{
  bool update = false;
  uint32_t u32;
  const char *str;
  htsmsg_t *list;

  /* Validate */
  if (htsmsg_get_u32(msg, "tagId", &u32))
  {
    tvherror("malformed tagUpdate");
    return;
  }

  /* Locate object */
  STag &tag = m_tags[u32];
  tag.id    = u32;
  tag.del   = false;

  /* Name */
  if ((str = htsmsg_get_str(msg, "tagName")) != NULL)
    UPDATE(tag.name, str);

  /* Icon */
  if ((str = htsmsg_get_str(msg, "tagIcon")) != NULL)
  {
    CStdString url = GetImageURL(str);
    UPDATE(tag.icon, url);
  }

  /* Members */
  if ((list = htsmsg_get_list(msg, "members")) != NULL)
  {
    htsmsg_field_t *f;
    tag.channels.clear();
    HTSMSG_FOREACH(f, list)
    {
      if (f->hmf_type != HMF_S64) continue;
      tag.channels.push_back((int)f->hmf_s64);
    }
    update = true; // TODO: could do detection here as well!
  }

  /* Update */
  if (update)
  {
    tvhdebug("tag updated id:%u, name:%s",
              tag.id, tag.name.c_str());
    if (m_asyncState > ASYNC_CHN)
      PVR->TriggerChannelGroupsUpdate();
  }
}

void CTvheadend::ParseTagDelete ( htsmsg_t *msg )
{
  uint32_t u32;

  /* Validate */
  if (htsmsg_get_u32(msg, "tagId", &u32))
  {
    tvherror("malformed tagDelete");
    return;
  }
  tvhdebug("delete tag %u", u32);
  
  /* Erase */
  m_tags.erase(u32);
  PVR->TriggerChannelGroupsUpdate();
}

void CTvheadend::ParseChannelUpdate ( htsmsg_t *msg )
{
  bool update = false;
  uint32_t u32;
  const char *str;
  htsmsg_t *list;

  /* Validate */
  if (htsmsg_get_u32(msg, "channelId", &u32))
  {
    tvherror("malformed channelUpdate");
    return;
  }

  /* Locate channel object */
  SChannel &channel = m_channels[u32];
  channel.id  = u32;
  channel.del = false;

  /* Channel name */
  if ((str = htsmsg_get_str(msg, "channelName")) != NULL)
    UPDATE(channel.name, str);

  /* Channel number */
  if (!htsmsg_get_u32(msg, "channelNumber", &u32))
  {
    if (!u32) u32 = UNNUMBERED_CHANNEL;
    UPDATE(channel.num, u32);
  }
  else if (!channel.num)
  {
    UPDATE(channel.num, UNNUMBERED_CHANNEL);
  }

  /* Channel icon */
  if ((str = htsmsg_get_str(msg, "channelIcon")) != NULL)
  {
    CStdString url = GetImageURL(str);
    UPDATE(channel.icon, url);
  }

  /* EPG info */
  if (!htsmsg_get_u32(msg, "eventId", &u32))
    UPDATE(channel.now,  u32);
  if (!htsmsg_get_u32(msg, "nextEventId", &u32))
    UPDATE(channel.next, u32);

  /* Services */
  if ((list = htsmsg_get_list(msg, "services")) != NULL)
  {
    htsmsg_field_t *f;
    uint32_t caid  = 0;
    bool     radio = false;
    HTSMSG_FOREACH(f, list)
    {
      if (f->hmf_type != HMF_MAP)
        continue;

      /* Radio? */
      if ((str = htsmsg_get_str(&f->hmf_msg, "type")) != NULL)
      {
        if (!strcmp(str, "Radio"))
          radio = true;
      }

      /* CAID */
      if (caid == 0)
        htsmsg_get_u32(&f->hmf_msg, "caid", &caid);
    }
    UPDATE(channel.radio, radio);
    UPDATE(channel.caid,  caid);
  }
  

  /* Update XBMC */
  if (update) {
    tvhdebug("channel update id:%u, name:%s",
              channel.id, channel.name.c_str());
    if (m_asyncState > ASYNC_CHN)
      PVR->TriggerChannelUpdate();
  }
}

void CTvheadend::ParseChannelDelete ( htsmsg_t *msg )
{
  uint32_t u32;

  /* Validate */
  if (htsmsg_get_u32(msg, "channelId", &u32))
  {
    tvherror("malformed channelUpdate");
    return;
  }
  tvhdebug("delete channel %u", u32);
  
  /* Erase */
  m_channels.erase(u32);
  PVR->TriggerChannelUpdate();
}

void CTvheadend::ParseRecordingUpdate ( htsmsg_t *msg )
{
  bool update = false;
  const char *state, *str;
  uint32_t id, channel;
  int64_t start, stop;

  /* Channels must be complete */
  SyncChannelsCompleted();

  /* Validate */
  if (htsmsg_get_u32(msg, "id",      &id)      ||
      htsmsg_get_u32(msg, "channel", &channel) ||
      htsmsg_get_s64(msg, "start",   &start)   ||
      htsmsg_get_s64(msg, "stop",    &stop)    ||
      ((state = htsmsg_get_str(msg, "state")) == NULL))
  {
    tvherror("malformed dvrEntryUpdate");
    return;
  }

  /* Get entry */
  SRecording &rec = m_recordings[id];
  rec.id  = id;
  rec.del = false;
  UPDATE(rec.channel, channel);
  UPDATE(rec.start,   start);
  UPDATE(rec.stop,    stop);

  /* Parse state */
  if      (strstr(state, "scheduled") != NULL)
  {
    UPDATE(rec.state, PVR_TIMER_STATE_SCHEDULED);
  }
  else if (strstr(state, "recording") != NULL)
  {
    UPDATE(rec.state, PVR_TIMER_STATE_RECORDING);
  }
  else if (strstr(state, "completed") != NULL)
  {
    UPDATE(rec.state, PVR_TIMER_STATE_COMPLETED);
  }
  else if (strstr(state, "invalid")   != NULL)
  {
    UPDATE(rec.state, PVR_TIMER_STATE_ERROR);
  }

  /* Info */
  if ((str = htsmsg_get_str(msg, "title")) != NULL)
    UPDATE(rec.title, str);
  if ((str = htsmsg_get_str(msg, "path")) != NULL)
    UPDATE(rec.path, str);
  if ((str = htsmsg_get_str(msg, "description")) != NULL)
  {
    UPDATE(rec.description, str);
  }
  else if ((str = htsmsg_get_str(msg, "summary")) != NULL)
  {
    UPDATE(rec.description, str);
  }

  /* Error */
  if ((str = htsmsg_get_str(msg, "error")) != NULL)
  {
    if (!strcmp(str, "300"))
    {
      UPDATE(rec.state, PVR_TIMER_STATE_ABORTED);
    }
    else if (strstr(str, "missing") != NULL)
    {
      UPDATE(rec.state, PVR_TIMER_STATE_ERROR);
    }
    else
    {
      UPDATE(rec.error, str);
    }
  }
  
  /* Update */
  if (update)
  {
    tvhdebug("recording id:%d, state:%s, title:%s, desc:%s, error:%s",
             rec.id, state, rec.title.c_str(), rec.description.c_str(),
             rec.error.c_str());

    if (m_asyncState > ASYNC_DVR)
    {
      PVR->TriggerTimerUpdate();
      if (rec.state == PVR_TIMER_STATE_RECORDING)
        PVR->TriggerRecordingUpdate();
    }
  }
}

void CTvheadend::ParseRecordingDelete ( htsmsg_t *msg )
{
  uint32_t u32;

  /* Validate */
  if (htsmsg_get_u32(msg, "id", &u32))
  {
    tvherror("malformed dvrEntryDelete");
    return;
  }
  tvhdebug("delete recording %u", u32);
  
  /* Erase */
  m_recordings.erase(u32);

  /* Update */
  PVR->TriggerTimerUpdate();
  PVR->TriggerRecordingUpdate();
}

bool CTvheadend::ParseEvent ( htsmsg_t *msg, SEvent &evt )
{
  const char *str;
  uint32_t u32, id, channel;
  int64_t s64, start, stop;

  /* Recordings complete */
  SyncDvrCompleted();

  /* Validate */
  if (htsmsg_get_u32(msg, "eventId",   &id)      ||
      htsmsg_get_u32(msg, "channelId", &channel) ||
      htsmsg_get_s64(msg, "start",     &start)   ||
      htsmsg_get_s64(msg, "stop",      &stop)    ||
      (str = htsmsg_get_str(msg, "title")) == NULL)
  {
    tvherror("malformed eventUpdate message");
    return false;
  }

  evt.id      = id;
  evt.channel = channel;
  evt.start   = (time_t)start;
  evt.stop    = (time_t)stop;
  evt.title   = str ? str : "";

  if ((str = htsmsg_get_str(msg, "summary")) != NULL)
    evt.summary  = str;
  if ((str = htsmsg_get_str(msg, "description")) != NULL)
    evt.desc     = str;
  if ((str = htsmsg_get_str(msg, "subtitle")) != NULL)
    evt.subtitle = str;
#if TODO_FIXME
  if ((str = htsmsg_get_str(msg, "image")) != NULL)
    evt.image   = str;
#endif
  if (!htsmsg_get_u32(msg, "nextEventId", &u32))
    evt.next    = u32;
  if (!htsmsg_get_u32(msg, "contentType", &u32))
    evt.content = u32;
  if (!htsmsg_get_u32(msg, "starRating", &u32))
    evt.stars   = u32;
  if (!htsmsg_get_u32(msg, "ageRating", &u32))
    evt.age     = u32;
  if (!htsmsg_get_s64(msg, "firstAired", &s64))
    evt.aired   = (time_t)s64;

  return true;
}

void CTvheadend::ParseEventUpdate ( htsmsg_t *msg )
{
  bool update = false;
  SEvent tmp;
  tmp.Clear();

  /* Parse */
  if (!ParseEvent(msg, tmp))
    return;

  /* Get event handle */
  SEvent &evt = m_schedules[tmp.channel].events[tmp.id];
  evt.id      = tmp.id;
  evt.del     = false;
  
  /* Store */
  UPDATE(evt.title,    tmp.title);
  UPDATE(evt.start,    tmp.start);
  UPDATE(evt.stop,     tmp.stop);
  UPDATE(evt.channel,  tmp.channel);
  UPDATE(evt.summary,  tmp.summary);
  UPDATE(evt.desc,     tmp.desc);
  UPDATE(evt.subtitle, tmp.subtitle);
  UPDATE(evt.image,    tmp.image);
  UPDATE(evt.next,     tmp.next);
  UPDATE(evt.content,  tmp.content);
  UPDATE(evt.stars,    tmp.stars);
  UPDATE(evt.age,      tmp.age);
  UPDATE(evt.aired,    tmp.aired);

  /* Update */
  if (update)
  {
    tvhtrace("event id:%d channel:%d start:%d stop:%d title:%s desc:%s",
             evt.id, evt.channel, (int)evt.start, (int)evt.stop,
             evt.title.c_str(), evt.desc.c_str());

#if TODO_FIXME
    if (m_asyncState > ASYNC_EPG)
      PVR->TriggerEventUpdate();
#endif
  }
}

void CTvheadend::ParseEventDelete ( htsmsg_t *msg )
{
  uint32_t u32;
  
  /* Validate */
  if (htsmsg_get_u32(msg, "eventId", &u32))
  {
    tvherror("malformed eventDelete");
    return;
  }
  tvhtrace("delete event %u", u32);
  
  /* Erase */
  // TODO: a bit nasty we don't actually know the channelId!
  SSchedules::iterator sit;
  for (sit = m_schedules.begin(); sit != m_schedules.end(); sit++)
    sit->second.events.erase(u32);

  /* Update */
#if TODO_FIXME
  PVR->TriggerEventUpdate();
#endif
}
