/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "HTSPData.h"
#include "HTSPDemux.h"
#include "platform/util/util.h"

extern "C" {
#include "platform/util/atomic.h"
#include "libhts/htsmsg.h"
#include "libhts/htsmsg_binary.h"
}

typedef enum {
  DVR_PRIO_IMPORTANT,
  DVR_PRIO_HIGH,
  DVR_PRIO_NORMAL,
  DVR_PRIO_LOW,
  DVR_PRIO_UNIMPORTANT,
} dvr_prio_t;

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

CHTSPData::CHTSPData()
{
  m_session                     = NULL;
  m_bDisconnectWarningDisplayed = false;
  m_bIsStarted                  = false;
  m_recordingId                 = 0;
  m_demux                       = NULL;
  m_recordingBuf.alloc(1000000);
}

CHTSPData::~CHTSPData()
{
  Close();
  delete m_session;
}

bool CHTSPData::Open()
{
  if (!m_session)
    m_session = new CHTSPConnection(this);

  CLockObject lock(m_mutex);
  if(!m_session->Connect())
  {
    /* failed to connect */
    return false;
  }

  if (!m_demux)
    m_demux = new CHTSPDemux(m_session);

  if(!SendEnableAsync())
  {
    XBMC->Log(LOG_ERROR, "%s - couldn't send EnableAsync().", __FUNCTION__);
    return false;
  }

  return m_started.Wait(m_mutex, m_bIsStarted, g_iConnectTimeout * 1000);
}

void CHTSPData::Close()
{
  CLockObject lock(m_mutex);
  m_bIsStarted = false;
  m_started.Broadcast();
  SAFE_DELETE(m_demux);
  SAFE_DELETE(m_session);
}

void CHTSPData::ReadResult(htsmsg_t *m, CHTSResult &result)
{
  if (!m_session || !m_session->IsConnected())
  {
    htsmsg_destroy(m);
    result.status = PVR_ERROR_SERVER_ERROR;
    return;
  }

  return m_session->ReadResult(m, result);
}

bool CHTSPData::GetDriveSpace(long long *total, long long *used)
{
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getDiskSpace");

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to get getDiskSpace", __FUNCTION__);
    return false;
  }

  int64_t freespace;
  if (htsmsg_get_s64(result.message, "freediskspace", &freespace) != 0)
    return false;

  int64_t totalspace;
  if (htsmsg_get_s64(result.message, "totaldiskspace", &totalspace) != 0)
    return false;

  *total = totalspace / 1024;
  *used  = (totalspace - freespace) / 1024;
  return true;
}

bool CHTSPData::GetBackendTime(time_t *utcTime, int *gmtOffset)
{
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getSysTime");

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_ERROR, "%s - failed to get sysTime", __FUNCTION__);
    return false;
  }

  unsigned int secs;
  if (htsmsg_get_u32(result.message, "time", &secs) != 0)
    return false;

  int offset;
  if (htsmsg_get_s32(result.message, "timezone", &offset) != 0)
    return false;

  XBMC->Log(LOG_DEBUG, "%s - tvheadend reported time=%u, timezone=%d, correction=%d"
      , __FUNCTION__, secs, offset);

  *utcTime = secs;
  *gmtOffset = offset;

  return true;
}


CodecVector CHTSPData::GetTranscodingCodecs(void)
{
  htsmsg_t *msg;
  htsmsg_field_t *f;
  CodecVector v;
  CHTSResult result;

  msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getCodecs");

  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to getCodecs", __FUNCTION__);
    return v;
  }

  msg = htsmsg_get_list(result.message, "encoders");
  if(!msg)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to get encoders", __FUNCTION__);
    return v;
  }

  HTSMSG_FOREACH(f, msg) {
    if (f->hmf_type != HMF_STR)
      continue;

    if (!strcmp("AAC", f->hmf_str))
      v.push_back(CODEC_ID_AAC);

    else if (!strcmp("MPEG2AUDIO", f->hmf_str))
      v.push_back(CODEC_ID_MP2);

    else if (!strcmp("AC3", f->hmf_str))
      v.push_back(CODEC_ID_AC3);

    else if (!strcmp("VORBIS", f->hmf_str))
      v.push_back(CODEC_ID_VORBIS);

    else if (!strcmp("MPEG2VIDEO", f->hmf_str))
      v.push_back(CODEC_ID_MPEG2VIDEO);

    else if (!strcmp("H264", f->hmf_str))
      v.push_back(CODEC_ID_H264);

    else if (!strcmp("VP8", f->hmf_str))
      v.push_back(CODEC_ID_VP8);

    else if (!strcmp("MPEG4VIDEO", f->hmf_str))
      v.push_back(CODEC_ID_MPEG4);
  }

  return v;
}

unsigned int CHTSPData::GetNumChannels()
{
  return GetChannels().size();
}

PVR_ERROR CHTSPData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  SChannels channels = GetChannels();
  for(SChannels::iterator it = channels.begin(); it != channels.end(); ++it)
  {
    SChannel& channel = it->second;
    if(bRadio != channel.radio)
      continue;

    PVR_CHANNEL tag;
    memset(&tag, 0 , sizeof(PVR_CHANNEL));

    tag.iUniqueId         = channel.id;
    tag.bIsRadio          = channel.radio;
    tag.iChannelNumber    = channel.num;
    strncpy(tag.strChannelName, channel.name.c_str(), sizeof(tag.strChannelName) - 1);
    tag.iEncryptionSystem = channel.caid;
    strncpy(tag.strIconPath, channel.icon.c_str(), sizeof(tag.strIconPath) - 1);
    tag.bIsHidden         = false;

    PVR->TransferChannelEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CHTSPData::GetEpg(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  PVR_ERROR retVal = PVR_ERROR_NO_ERROR;
  SChannels channels = GetChannels();

  if (channels.find(channel.iUniqueId) != channels.end())
  {

    /* Full channel update */
    if (GetProtocol() >= 6)
    {
      retVal = GetEvents(handle, channel.iUniqueId, iEnd);
    }
    /* Event at a time */
    else
    {
      uint32_t eventId = channels[channel.iUniqueId].event;
      if (eventId != 0)
      {
        do
        {
          retVal = GetEvent(handle, &eventId, iEnd);
        } while(eventId && retVal == PVR_ERROR_NO_ERROR);
      }
    }
  }

  return retVal;
}

SRecordings CHTSPData::GetDVREntries(bool recorded, bool scheduled)
{
  CLockObject lock(m_mutex);
  SRecordings recordings;

  for(SRecordings::const_iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    SRecording recording = it->second;

    if ((recorded && (recording.state == ST_COMPLETED || recording.state == ST_ABORTED || recording.state == ST_RECORDING)) ||
        (scheduled && (recording.state == ST_SCHEDULED || recording.state == ST_RECORDING)))
      recordings[recording.id] = recording;
  }

  return recordings;
}

unsigned int CHTSPData::GetNumRecordings()
{
  SRecordings recordings = GetDVREntries(true, false);
  return recordings.size();
}

PVR_ERROR CHTSPData::GetRecordings(ADDON_HANDLE handle)
{
  SRecordings recordings = GetDVREntries(true, false);

  for(SRecordings::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    SRecording recording = it->second;
    CStdString strStreamURL;
    CStdString strRecordingId;
    CStdString strDirectory = "/";
    std::string strChannelName = "";

    /* lock */
    {
      CLockObject lock(m_mutex);
      SChannels::const_iterator itr = m_channels.find(recording.channel);
      if (itr != m_channels.end())
        strChannelName = itr->second.name.c_str();

      /* HTSPv7+ - use HTSP */
      if (GetProtocol() >= 7)
        strStreamURL = "";

      /* HTSPv6- - use HTTP */
      else
        strStreamURL = m_session->GetWebURL("/dvrfile/%i", recording.id);
    }

    strRecordingId.Format("%i", recording.id);

    if (recording.path != "")
    {
      size_t i, idx = recording.path.rfind("/");
      if (idx == 0 || idx == std::string::npos) {
        strDirectory = "/";
      } else {
        strDirectory = recording.path.substr(0, idx);
        if (strDirectory[0] != '/')
          strDirectory = "/" + strDirectory;
      }
    }

    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));

    strncpy(tag.strRecordingId, strRecordingId.c_str(), sizeof(tag.strRecordingId) - 1);
    strncpy(tag.strTitle, recording.title.c_str(), sizeof(tag.strTitle) - 1);
    strncpy(tag.strStreamURL, strStreamURL.c_str(), sizeof(tag.strStreamURL) - 1);
    strncpy(tag.strDirectory, strDirectory.c_str(), sizeof(tag.strDirectory) - 1);
    strncpy(tag.strPlot, recording.description.c_str(), sizeof(tag.strPlot) - 1);
    strncpy(tag.strChannelName, strChannelName.c_str(), sizeof(tag.strChannelName) - 1);
    tag.recordingTime  = recording.start;
    tag.iDuration      = recording.stop - recording.start;

    PVR->TransferRecordingEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CHTSPData::DeleteRecording(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "deleteDvrEntry");
  htsmsg_add_u32(msg, "id", atoi(recording.strRecordingId));

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get deleteDvrEntry", __FUNCTION__);
    return result.status;
  }

  unsigned int success;
  if (htsmsg_get_u32(result.message, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}

unsigned int CHTSPData::GetNumTimers()
{
  SRecordings recordings = GetDVREntries(false, true);
  return recordings.size();
}

unsigned int CHTSPData::GetNumChannelGroups(void)
{
  return m_tags.size();
}

PVR_ERROR CHTSPData::GetChannelGroups(ADDON_HANDLE handle)
{
  for(unsigned int iTagPtr = 0; iTagPtr < m_tags.size(); iTagPtr++)
  {
    if (!m_tags[iTagPtr].name.empty())
    {
      PVR_CHANNEL_GROUP tag;
      memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

      tag.bIsRadio     = false;
      strncpy(tag.strGroupName, m_tags[iTagPtr].name.c_str(), sizeof(tag.strGroupName) - 1);

      PVR->TransferChannelGroup(handle, &tag);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CHTSPData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  XBMC->Log(LOG_DEBUG, "%s - group '%s'", __FUNCTION__, group.strGroupName);

  for(unsigned int iTagPtr = 0; iTagPtr < m_tags.size(); iTagPtr++)
  {
    if (m_tags[iTagPtr].name != group.strGroupName)
      continue;

    SChannels channels = GetChannels(m_tags[iTagPtr].id);

    for(SChannels::iterator it = channels.begin(); it != channels.end(); ++it)
    {
      SChannel& channel = it->second;
      if (channel.radio != group.bIsRadio)
        continue;

      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName) - 1);
      tag.iChannelUniqueId = channel.id;
      tag.iChannelNumber   = channel.num;

#if HTSP_DEBUGGING
      XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
          __FUNCTION__, channel.name.c_str(), tag.iChannelUniqueId, group.strGroupName, channel.num);
#endif

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CHTSPData::GetTimers(ADDON_HANDLE handle)
{
  SRecordings recordings = GetDVREntries(false, true);

  for(SRecordings::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    SRecording recording = it->second;

    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    tag.iClientIndex      = recording.id;
    tag.iClientChannelUid = recording.channel;
    tag.startTime         = recording.start;
    tag.endTime           = recording.stop;
    strncpy(tag.strTitle, recording.title.c_str(), sizeof(tag.strTitle) - 1);
    strncpy(tag.strSummary, recording.description.c_str(), sizeof(tag.strSummary) - 1);
    tag.state             = (PVR_TIMER_STATE) recording.state;
    tag.iPriority         = 0;     // unused
    tag.iLifetime         = 0;     // unused
    tag.bIsRepeating      = false; // unused
    tag.firstDay          = 0;     // unused
    tag.iWeekdays         = 0;     // unused
    tag.iEpgUid           = 0;     // unused
    tag.iMarginStart      = 0;     // unused
    tag.iMarginEnd        = 0;     // unused
    tag.iGenreType        = 0;     // unused
    tag.iGenreSubType     = 0;     // unused

    PVR->TransferTimerEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CHTSPData::DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "cancelDvrEntry");
  htsmsg_add_u32(msg, "id", timer.iClientIndex);

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get cancelDvrEntry", __FUNCTION__);
    return result.status;
  }

  const char *strError = NULL;
  if ((strError = htsmsg_get_str(result.message, "error")))
  {
    XBMC->Log(LOG_DEBUG, "%s - Error deleting timer: '%s'", __FUNCTION__, strError);
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(result.message, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}

PVR_ERROR CHTSPData::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  time_t startTime = timer.startTime;
  if (startTime <= 0)
  {
    int iGmtOffset;
    GetBackendTime(&startTime, &iGmtOffset);
  }

  dvr_prio_t prio = DVR_PRIO_UNIMPORTANT;
  if (timer.iPriority <= 20)
    prio = DVR_PRIO_UNIMPORTANT;
  else if (timer.iPriority <= 40)
    prio =  DVR_PRIO_LOW;
  else if (timer.iPriority <= 60)
    prio =  DVR_PRIO_NORMAL;
  else if (timer.iPriority <= 80)
    prio =  DVR_PRIO_HIGH;
  else
    prio = DVR_PRIO_IMPORTANT;

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method",      "addDvrEntry");
  if ((GetProtocol() >= 6) && timer.iEpgUid > 0)
  {
    htsmsg_add_u32(msg, "eventId",     timer.iEpgUid);
    htsmsg_add_s64(msg, "startExtra",  timer.iMarginStart);
    htsmsg_add_s64(msg, "stopExtra",   timer.iMarginEnd);
  }
  else
  {
    htsmsg_add_str(msg, "title",       timer.strTitle);
    htsmsg_add_u32(msg, "start",       startTime);
    htsmsg_add_u32(msg, "stop",        timer.endTime);
    htsmsg_add_u32(msg, "channelId",   timer.iClientChannelUid);
    htsmsg_add_str(msg, "description", timer.strSummary);
    htsmsg_add_u32(msg, "eventId",     -1);
  }
  htsmsg_add_u32(msg, "priority",    prio);
  htsmsg_add_str(msg, "creator",     "XBMC");

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get addDvrEntry", __FUNCTION__);
    return result.status;
  }

  const char *strError = NULL;
  if ((strError = htsmsg_get_str(result.message, "error")))
  {
    XBMC->Log(LOG_DEBUG, "%s - Error adding timer: '%s'", __FUNCTION__, strError);
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned int success;
  if (htsmsg_get_u32(result.message, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}

PVR_ERROR CHTSPData::UpdateTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "updateDvrEntry");
  htsmsg_add_u32(msg, "id",     timer.iClientIndex);
  htsmsg_add_str(msg, "title",  timer.strTitle);
  htsmsg_add_u32(msg, "start",  timer.startTime);
  htsmsg_add_u32(msg, "stop",   timer.endTime);

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get updateDvrEntry", __FUNCTION__);
    return result.status;
  }

  unsigned int success;
  if (htsmsg_get_u32(result.message, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}

PVR_ERROR CHTSPData::RenameRecording(const PVR_RECORDING &recording, const char *strNewName)
{
  XBMC->Log(LOG_DEBUG, "%s - id=%s", __FUNCTION__, recording.strRecordingId);

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "updateDvrEntry");
  htsmsg_add_u32(msg, "id",     atoi(recording.strRecordingId));
  htsmsg_add_str(msg, "title",  recording.strTitle);

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to get updateDvrEntry", __FUNCTION__);
    return result.status;
  }

  unsigned int success;
  if (htsmsg_get_u32(result.message, "success", &success) != 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Failed to parse param", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (success > 0)
    PVR->TriggerRecordingUpdate();

  return success > 0 ? PVR_ERROR_NO_ERROR : PVR_ERROR_FAILED;
}


bool CHTSPData::ProcessMessage(htsmsg* msg)
{
  const char* method;
  if((method = htsmsg_get_str(msg, "method")) == NULL)
    return true;

  CLockObject lock(m_mutex);
  if      (m_demux && m_demux->ProcessMessage(msg))
  {
    // demux packet
  }
  else if(strstr(method, "channelAdd"))
    ParseChannelUpdate(msg);
  else if(strstr(method, "channelUpdate"))
    ParseChannelUpdate(msg);
  else if(strstr(method, "channelDelete"))
    ParseChannelRemove(msg);
  else if(strstr(method, "tagAdd"))
    ParseTagUpdate(msg);
  else if(strstr(method, "tagUpdate"))
    ParseTagUpdate(msg);
  else if(strstr(method, "tagDelete"))
    ParseTagRemove(msg);
  else if(strstr(method, "initialSyncCompleted"))
  {
    m_bIsStarted = true;
    m_started.Broadcast();
  }
  else if(strstr(method, "dvrEntryAdd"))
    ParseDVREntryUpdate(msg);
  else if(strstr(method, "dvrEntryUpdate"))
    ParseDVREntryUpdate(msg);
  else if(strstr(method, "dvrEntryDelete"))
    ParseDVREntryDelete(msg);
  else
    XBMC->Log(LOG_DEBUG, "%s - Unmapped action recieved '%s'", __FUNCTION__, method);
  return true;
}

SChannels CHTSPData::GetChannels()
{
  return GetChannels(0);
}

SChannels CHTSPData::GetChannels(int tag)
{
  CLockObject lock(m_mutex);
  if(tag == 0)
    return m_channels;

  STags::iterator it = m_tags.find(tag);
  if(it == m_tags.end())
  {
    SChannels channels;
    return channels;
  }
  return GetChannels(it->second);
}

SChannels CHTSPData::GetChannels(STag& tag)
{
  CLockObject lock(m_mutex);
  SChannels channels;

  std::vector<int>::iterator it;
  for(it = tag.channels.begin(); it != tag.channels.end(); it++)
  {
    SChannels::iterator it2 = m_channels.find(*it);
    if(it2 == m_channels.end())
    {
      XBMC->Log(LOG_ERROR, "%s - tag points to unknown channel %d", __FUNCTION__, *it);
      continue;
    }
    channels[*it] = it2->second;
  }
  return channels;
}

STags CHTSPData::GetTags()
{
  CLockObject lock(m_mutex);
  return m_tags;
}

PVR_ERROR CHTSPData::GetEvent(ADDON_HANDLE handle, uint32_t *id, time_t stop)
{
  if(*id == 0)
  {
    return PVR_ERROR_UNKNOWN;
  }

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", *id);

  CHTSResult result;
  ReadResult(msg, result);
  if(result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to get event %d", __FUNCTION__, id);
    return result.status;
  }

  if (ParseEvent(handle, result.message, id, stop))
  {
    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR CHTSPData::GetEvents(ADDON_HANDLE handle, uint32_t cid, time_t stop)
{
  PVR_ERROR retVal = PVR_ERROR_NO_ERROR;

  if (cid == 0)
  {
    return PVR_ERROR_UNKNOWN;
  }

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvents");
  htsmsg_add_u32(msg, "channelId", cid);
  htsmsg_add_s64(msg, "maxTime", stop);

  CHTSResult result;
  ReadResult(msg, result);
  if(result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to get events for %d", __FUNCTION__, cid);
    return result.status;
  }

  if (!(msg = htsmsg_get_list(result.message, "events"))) {
    XBMC->Log(LOG_DEBUG, "%s - failed to get events for %d", __FUNCTION__, cid);
    return PVR_ERROR_UNKNOWN;
  }

  htsmsg_t *e;
  htsmsg_field_t *f;

  unsigned int failedEvents = 0;
  unsigned int goodEvents = 0;

  HTSMSG_FOREACH(f, msg)
  {
    if ((e = htsmsg_get_map_by_field(f)))
    {
      if (ParseEvent(handle, e, NULL, stop))
        goodEvents++;
      else
        failedEvents++;
    }
  }

  if (goodEvents == 0 && failedEvents > 0)
    retVal = PVR_ERROR_SERVER_ERROR;

  return retVal;
}

bool CHTSPData::SendEnableAsync()
{
  htsmsg_t *m = htsmsg_create_map();
  htsmsg_add_str(m, "method", "enableAsyncMetadata");
  return m_session->ReadSuccess(m, "enableAsyncMetadata");
}

void CHTSPData::ParseChannelRemove(htsmsg_t* msg)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "channelId", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }
  XBMC->Log(LOG_DEBUG, "%s - id:%u", __FUNCTION__, id);

  m_channels.erase(id);

  PVR->TriggerChannelUpdate();
}

void CHTSPData::ParseChannelUpdate(htsmsg_t* msg)
{
  bool bChannelChanged(false), bTagsChanged(false);
  uint32_t iChannelId, iEventId = 0, iChannelNumber = 0, iCaid = 0;
  const char *strName, *strIconPath;
  if(htsmsg_get_u32(msg, "channelId", &iChannelId))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }

  SChannel &channel = m_channels[iChannelId];
  channel.id = iChannelId;

  if(htsmsg_get_u32(msg, "eventId", &iEventId) == 0)
    channel.event = iEventId;

  if((strName = htsmsg_get_str(msg, "channelName")))
  {
    if (channel.name != strName)
    {
      bChannelChanged = true;
      channel.name = strName;
    }
  }

  if((strIconPath = htsmsg_get_str(msg, "channelIcon")))
  {
    CStdString strIconURL;

    if (strIconPath[0] != '/' || strIconPath[0] == '\0')
      strIconURL = strIconPath;
    else
      strIconURL = m_session->GetWebURL("%s", strIconPath);

    if (channel.icon != strIconURL)
    {
      bChannelChanged = true;
      channel.icon = strIconURL;
    }
  }

  if(htsmsg_get_u32(msg, "channelNumber", &iChannelNumber) == 0)
  {
    int iNewChannelNumber = (iChannelNumber == 0) ? iChannelId + 1000 : iChannelNumber;
    if (channel.num != iNewChannelNumber)
    {
      bChannelChanged = true;
      channel.num = iNewChannelNumber;
    }
  }

  htsmsg_t *tags;

  if((tags = htsmsg_get_list(msg, "tags")))
  {
    std::vector<int> newTags;
    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, tags)
    {
      if(f->hmf_type != HMF_S64)
        continue;
      newTags.push_back((int)f->hmf_s64);
    }

    for (std::vector<int>::const_iterator it = newTags.begin(); it != newTags.end(); it++)
    {
      if (std::find(channel.tags.begin(), channel.tags.end(), *it) == channel.tags.end())
        bTagsChanged = true;
    }
    for (std::vector<int>::const_iterator it = channel.tags.begin(); it != channel.tags.end(); it++)
    {
      if (std::find(newTags.begin(), newTags.end(), *it) == newTags.end())
        bTagsChanged = true;
    }
    if (bTagsChanged)
      channel.tags = newTags;
  }

  htsmsg_t *services;
  bool bIsRadio = channel.radio;
  if((services = htsmsg_get_list(msg, "services")))
  {
    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, services)
    {
      if(f->hmf_type != HMF_MAP)
        continue;

      htsmsg_t *service = &f->hmf_msg;
      const char *service_type = htsmsg_get_str(service, "type");
      if(service_type != NULL)
        bIsRadio = !strcmp(service_type, "Radio");

      if(!htsmsg_get_u32(service, "caid", &iCaid))
      {
        if ((channel.caid != (int)iCaid))
        {
          bChannelChanged = true;
          channel.caid = (int) iCaid;
        }
      }
    }
  }
  if (channel.radio != bIsRadio)
  {
    bChannelChanged = true;
    channel.radio = bIsRadio;
  }

#if HTSP_DEBUGGING
  XBMC->Log(LOG_DEBUG, "%s - id:%u, name:'%s', icon:'%s', event:%u",
      __FUNCTION__, iChannelId, strName ? strName : "(null)", strIconPath ? strIconPath : "(null)", iEventId);
#endif

  if (bChannelChanged)
    PVR->TriggerChannelUpdate();
  if (bTagsChanged)
    PVR->TriggerChannelGroupsUpdate();
}

void CHTSPData::ParseDVREntryDelete(htsmsg_t* msg)
{
  uint32_t id;

  if(htsmsg_get_u32(msg, "id", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }

  XBMC->Log(LOG_DEBUG, "%s - Recording %i was deleted", __FUNCTION__, id);

  m_recordings.erase(id);

  PVR->TriggerTimerUpdate();
  PVR->TriggerRecordingUpdate();
}

void CHTSPData::ParseDVREntryUpdate(htsmsg_t* msg)
{
  SRecording recording;
  const char *state;

  if(htsmsg_get_u32(msg, "id",      &recording.id)
  || htsmsg_get_u32(msg, "channel", &recording.channel)
  || htsmsg_get_u32(msg, "start",   &recording.start)
  || htsmsg_get_u32(msg, "stop",    &recording.stop)
  || (state = htsmsg_get_str(msg, "state")) == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }

  /* parse the dvr entry's state */
  if     (strstr(state, "scheduled"))
    recording.state = ST_SCHEDULED;
  else if(strstr(state, "recording"))
    recording.state = ST_RECORDING;
  else if(strstr(state, "completed"))
    recording.state = ST_COMPLETED;
  else if(strstr(state, "invalid"))
    recording.state = ST_INVALID;

  const char* str;
  if((str = htsmsg_get_str(msg, "title")) == NULL)
    recording.title = "";
  else
    recording.title = str;

  if((str = htsmsg_get_str(msg, "path")) == NULL)
    recording.path = "";
  else
    recording.path = str;

  if((str = htsmsg_get_str(msg, "description")) == NULL)
    recording.description = "";
  else
    recording.description = str;

  if((str = htsmsg_get_str(msg, "error")) == NULL)
    recording.error = "";
  else
    recording.error = str;

  // if the user has aborted the recording then the recording.error will be set to 300 by tvheadend
  if (recording.error == "300")
  {
    recording.state = ST_ABORTED;
    recording.error.clear();
  }

  // Missing file (hide)
  else if (recording.error == "File missing")
  {
    recording.state = ST_INVALID;
    recording.error.clear();
  }


#if HTSP_DEBUGGING
  XBMC->Log(LOG_DEBUG, "%s - id:%u, state:'%s', title:'%s', description: '%s', error:'%s'"
      , __FUNCTION__, recording.id, state, recording.title.c_str()
      , recording.description.c_str(), recording.error.c_str());
#endif

  m_recordings[recording.id] = recording;

  PVR->TriggerTimerUpdate();

  if (recording.state == ST_RECORDING)
   PVR->TriggerRecordingUpdate();
}

bool CHTSPData::ParseEvent(ADDON_HANDLE handle, htsmsg_t* msg, uint32_t *id, time_t end)
{
  uint32_t eventId, channelId, content, nextId, stars, age, start, stop;
  int64_t aired;
  const char *title, *subtitle, *desc, *summary, *image;

  /* Required fields */
  if(         htsmsg_get_u32(msg, "eventId",   &eventId)
  ||          htsmsg_get_u32(msg, "channelId", &channelId)
  ||          htsmsg_get_u32(msg, "start",     &start)
  ||          htsmsg_get_u32(msg, "stop" ,     &stop)
  || (title = htsmsg_get_str(msg, "title")) == NULL
  || (id && (*id != eventId)))
  {
    XBMC->Log(LOG_DEBUG, "%s - malformed event", __FUNCTION__);
    htsmsg_print(msg);
    return false;
  }

  /* Optional fields */
  summary  = htsmsg_get_str(msg, "summary");
  subtitle = htsmsg_get_str(msg, "subtitle");
  desc     = htsmsg_get_str(msg, "description");
  image    = htsmsg_get_str(msg, "image");
  content  = htsmsg_get_u32_or_default(msg, "contentType", 0);
  nextId   = htsmsg_get_u32_or_default(msg, "nextEventId", 0);
  stars    = htsmsg_get_u32_or_default(msg, "starRating", 0);
  age      = htsmsg_get_u32_or_default(msg, "ageRating", 0);
  htsmsg_get_s64(msg, "firstAired", &aired);

  /* Fix old genre spec */
  if (GetProtocol() < 6)
    content = content << 4;

#if HTSP_DEBUGGING
  XBMC->Log(LOG_DEBUG, "%s - id:%u, chan_id:%u, title:'%s', genre_type:%u, genre_sub_type:%u, desc:'%s', start:%u, stop:%u, next:%u"
                    , __FUNCTION__
                    , eventId
                    , channelId
                    , title
                    , content & 0xF0
                    , content & 0x0F
                    , desc
                    , start
                    , stop
                    , nextId);
#endif

  /* Broadcast */
  EPG_TAG broadcast;
  memset(&broadcast, 0, sizeof(EPG_TAG));

  broadcast.iUniqueBroadcastId  = eventId;
  broadcast.strTitle            = title;
  broadcast.iChannelNumber      = channelId;
  broadcast.startTime           = start;
  broadcast.endTime             = stop;
  broadcast.strPlotOutline      = summary ? summary : "";
  broadcast.strPlot             = desc ? desc : "";
  broadcast.strIconPath         = image ? image : "";
  broadcast.iGenreType          = content & 0xF0;
  broadcast.iGenreSubType       = content & 0x0F;
  broadcast.strGenreDescription = ""; // unused
  broadcast.firstAired          = (time_t) aired;
  broadcast.iParentalRating     = age;
  broadcast.iStarRating         = stars;
  broadcast.bNotify             = false;
  broadcast.iSeriesNumber       = htsmsg_get_u32_or_default(msg, "seasonNumber", 0);
  broadcast.iEpisodeNumber      = htsmsg_get_u32_or_default(msg, "episodeNumber", 0);
  broadcast.iEpisodePartNumber  = htsmsg_get_u32_or_default(msg, "partNumber", 0);
  broadcast.strEpisodeName      = subtitle ? subtitle : "";

  /* Post to PVR */
  PVR->TransferEpgEntry(handle, &broadcast);

  /* Update next */
  if (id && ((time_t)stop < end))
    *id = nextId;
  else if (id)
    *id = 0;

  return true;
}

void CHTSPData::ParseTagRemove(htsmsg_t* msg)
{
  uint32_t id;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }
  XBMC->Log(LOG_DEBUG, "%s - id:%u", __FUNCTION__, id);

  m_tags.erase(id);

  PVR->TriggerChannelGroupsUpdate();
}

void CHTSPData::ParseTagUpdate(htsmsg_t* msg)
{
  uint32_t id;
  const char *name, *icon;
  if(htsmsg_get_u32(msg, "tagId", &id))
  {
    XBMC->Log(LOG_ERROR, "%s - malformed message received", __FUNCTION__);
    htsmsg_print(msg);
    return;
  }
  STag &tag = m_tags[id];
  tag.id = id;

  if((icon = htsmsg_get_str(msg, "tagIcon")))
    tag.icon  = icon;

  if((name = htsmsg_get_str(msg, "tagName")))
    tag.name  = name;

  htsmsg_t *channels;

  if((channels = htsmsg_get_list(msg, "members")))
  {
    tag.channels.clear();

    htsmsg_field_t *f;
    HTSMSG_FOREACH(f, channels)
    {
      if(f->hmf_type != HMF_S64)
        continue;
      tag.channels.push_back((int)f->hmf_s64);
    }
  }

#if HTSP_DEBUGGING
  XBMC->Log(LOG_DEBUG, "%s - id:%u, name:'%s', icon:'%s'"
      , __FUNCTION__, id, name ? name : "(null)", icon ? icon : "(null)");
#endif

  PVR->TriggerChannelGroupsUpdate();
}

bool CHTSPData::OpenRecordedStream(const PVR_RECORDING &recording)
{
  if (GetProtocol() < 7) return false;

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "fileOpen");

  CStdString strDvrPath;
  strDvrPath.Format("dvr/%s", recording.strRecordingId);
  htsmsg_add_str(msg, "file", strDvrPath.c_str());

  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to fileOpen", __FUNCTION__);
    return false;
  }

  uint32_t id;
  if (htsmsg_get_u32(result.message, "id", &id))
    return false;
  m_recordingId  = id;
  m_recordingOff = 0;
  m_recordingBuf.reset();

  return true;
}

void CHTSPData::CloseRecordedStream(void)
{
  if (GetProtocol() < 7) return;
  if (!m_recordingId) return;

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "fileClose");
  htsmsg_add_u32(msg, "id", m_recordingId);
  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to fileClose", __FUNCTION__);
  }
  m_recordingId = 0;
}

int CHTSPData::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  ssize_t     ret;
  if (GetProtocol() < 7) return 0;
  if (!m_recordingId) return -1;

  /* Fetch data */
  if (m_recordingBuf.avail() <= iBufferSize)
  {
    const void *buf;
    size_t      len;
    htsmsg_t *msg = htsmsg_create_map();
    htsmsg_add_str(msg, "method", "fileRead");
    htsmsg_add_u32(msg, "id", m_recordingId);
    htsmsg_add_s64(msg, "size", m_recordingBuf.free());
    CHTSResult result;
    ReadResult(msg, result);
    if (result.status != PVR_ERROR_NO_ERROR)
    {
      XBMC->Log(LOG_DEBUG, "%s - failed to fileRead", __FUNCTION__);
      return -1;
    }
    if (htsmsg_get_bin(result.message, "data", &buf, &len)) {
      XBMC->Log(LOG_DEBUG, "%s - failed fileRead no buffer", __FUNCTION__);
      return -1;
    }
    ret = m_recordingBuf.write((unsigned char*)buf, len);
    if (ret != (ssize_t)len)
    {
      XBMC->Log(LOG_ERROR, "%s - CircBuffer::write() partial %ld != %ld", __FUNCTION__, ret, len);
      return -1;
    }
  }

  /* Read */
  ret = m_recordingBuf.read(pBuffer, iBufferSize);
  m_recordingOff += ret;
  return (int)ret;
}

long long CHTSPData::SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (GetProtocol() < 7) return 0;
  if (!m_recordingId)    return -1;
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "fileSeek");
  htsmsg_add_u32(msg, "id",     m_recordingId);
  htsmsg_add_s64(msg, "offset", iPosition);
  if (iWhence == SEEK_CUR)
    htsmsg_add_str(msg, "whence", "SEEK_CUR");
  else if (iWhence == SEEK_END)
    htsmsg_add_str(msg, "whence", "SEEK_END");
  //else
  //  htsmsg_add_str(msg, "whence", SEEK_SET");
  // Note: last is default so no need to send
  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to fileSeek", __FUNCTION__);
    return -1;
  }
  int64_t off;
  if (htsmsg_get_s64(result.message, "offset", &off)) {
    XBMC->Log(LOG_DEBUG, "%s - failed to fileSeek no offset", __FUNCTION__);
    return -1;
  }
  m_recordingOff = off;
  m_recordingBuf.reset();
  return m_recordingOff;
}

long long CHTSPData::PositionRecordedStream(void)
{
  if (GetProtocol() < 7) return 0;
  return m_recordingOff;
}

long long CHTSPData::LengthRecordedStream(void)
{
  if (GetProtocol() < 7) return 0;
  if (!m_recordingOff) return -1;
  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "fileStat");
  htsmsg_add_u32(msg, "id",     m_recordingId);
  CHTSResult result;
  ReadResult(msg, result);
  if (result.status != PVR_ERROR_NO_ERROR)
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to fileStat", __FUNCTION__);
    return -1;
  }
  int64_t size;
  if (htsmsg_get_s64(result.message, "size", &size))
  {
    XBMC->Log(LOG_DEBUG, "%s - failed to fileStat no size", __FUNCTION__);
    return -1;
  }
  return size;
}

bool CHTSPData::OnConnectionDropped(void)
{
  if (m_demux)
    m_demux->OnConnectionDropped();
  m_bIsStarted = false;
  if (m_connectionWarningTimeout.TimeLeft() == 0)
  {
    // don't show the warning more than once every 5 seconds
    m_connectionWarningTimeout.Init(5000);

    CStdString strNotification(XBMC->GetLocalizedString(30500));
    XBMC->QueueNotification(QUEUE_ERROR, strNotification, GetServerName());
  }
  return true;
}

bool CHTSPData::OnConnectionRestored(void)
{
  if(!SendEnableAsync())
    return false;

  {
    CLockObject lock(m_mutex);
    if (!m_started.Wait(m_mutex, m_bIsStarted, g_iConnectTimeout * 1000))
      return false;
  }

  if (m_demux)
    m_demux->OnConnectionRestored();

  CStdString strNotification(XBMC->GetLocalizedString(30501));
  XBMC->QueueNotification(QUEUE_INFO, strNotification, GetServerName());
  return true;
}

bool CHTSPData::OpenLiveStream(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  if (!IsConnected() || !m_demux)
    return false;

  return m_demux->Open(channel);
}

void CHTSPData::CloseLiveStream(void)
{
  if (m_demux)
    m_demux->Close();
}

int CHTSPData::GetCurrentClientChannel(void)
{
  return m_demux ?
      m_demux->CurrentChannel() :
      -1;
}

bool CHTSPData::SwitchChannel(const PVR_CHANNEL &channel)
{
  return m_demux ?
      m_demux->SwitchChannel(channel) :
      false;
}

PVR_ERROR CHTSPData::GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  return m_demux && m_demux->GetStreamProperties(pProperties) ?
      PVR_ERROR_NO_ERROR :
      PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR CHTSPData::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  return m_demux && m_demux->GetSignalStatus(signalStatus) ?
      PVR_ERROR_NO_ERROR :
      PVR_ERROR_SERVER_ERROR;
}

void CHTSPData::DemuxAbort(void)
{
  if (m_demux)
    m_demux->Abort();
}

void CHTSPData::DemuxFlush(void)
{
  if (m_demux)
    m_demux->Flush();
}

DemuxPacket* CHTSPData::DemuxRead(void)
{
  return m_demux ?
      m_demux->Read() :
      NULL;
}

bool CHTSPData::SeekTime(int time,bool backward,double *startpts)
{
  return m_demux && CanSeekLiveStream() ?
      m_demux->SeekTime(time, backward, startpts) :
      false;
}

void CHTSPData::SetSpeed(int speed)
{
  if (m_demux && CanTimeshift())
      m_demux->SetSpeed(speed);
}
