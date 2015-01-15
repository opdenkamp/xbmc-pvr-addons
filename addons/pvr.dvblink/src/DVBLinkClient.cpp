/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 
 *      Copyright (C) 2012 Palle Ehmsen(Barcode Madness)
 *      http://www.barcodemadness.com
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DVBLinkClient.h"
#include "platform/util/StdString.h"
#include "DialogRecordPref.h"
#include "DialogDeleteTimer.h"

using namespace dvblinkremote;
using namespace dvblinkremotehttp;
using namespace ADDON;

std::string DVBLinkClient::GetBuildInRecorderObjectID()
{
  std::string result = "";
  DVBLinkRemoteStatusCode status;
  GetPlaybackObjectRequest getPlaybackObjectRequest(m_hostname.c_str(), "");
  getPlaybackObjectRequest.RequestedObjectType = GetPlaybackObjectRequest::REQUESTED_OBJECT_TYPE_ALL;
  getPlaybackObjectRequest.RequestedItemType = GetPlaybackObjectRequest::REQUESTED_ITEM_TYPE_ALL;
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;
  if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse)) == DVBLINK_REMOTE_STATUS_OK)
  {
    for (std::vector<PlaybackContainer*>::iterator it = getPlaybackObjectResponse.GetPlaybackContainers().begin(); it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
    {
      PlaybackContainer * container = (PlaybackContainer *) *it;
      if (strcmp(container->SourceID.c_str(), DVBLINK_BUILD_IN_RECORDER_SOURCE_ID) == 0)
      {
        result = container->GetObjectID();
        break;
      }

    }
  }
  return result;
}

DVBLinkClient::DVBLinkClient(CHelper_libXBMC_addon* xbmc, CHelper_libXBMC_pvr* pvr, CHelper_libXBMC_gui* gui, std::string clientname, std::string hostname, 
    long port, bool showinfomsg, std::string username, std::string password, bool add_episode_to_rec_title, bool group_recordings_by_series)
{
  PVR = pvr;
  XBMC = xbmc;
  GUI = gui;
  m_clientname = clientname;
  m_hostname = hostname;
  m_connected = false;
  m_currentChannelId = 0;
  m_showinfomsg = showinfomsg;
  m_add_episode_to_rec_title = add_episode_to_rec_title;
  m_group_recordings_by_series = group_recordings_by_series;

  m_httpClient = new HttpPostClient(XBMC,hostname, port, username, password);
  m_dvblinkRemoteCommunication = DVBLinkRemote::Connect((HttpClient&)*m_httpClient, m_hostname.c_str(), port, username.c_str(), password.c_str());

  DVBLinkRemoteStatusCode status;
  m_timerCount = -1;
  m_recordingCount = -1;

  GetChannelsRequest request;
  m_channels = new ChannelList();
  m_stream = new Stream();
  m_live_streamer = NULL;

  if ((status = m_dvblinkRemoteCommunication->GetChannels(request, *m_channels)) == DVBLINK_REMOTE_STATUS_OK)
  {
    int iChannelUnique = 0;
    for (std::vector<Channel*>::iterator it = m_channels->begin(); it < m_channels->end(); it++) 
    {
      Channel* channel = (*it);
      m_channelMap[++iChannelUnique] = channel;
    }
    m_connected = true;
    
    XBMC->Log(LOG_INFO, "Connected to DVBLink Server '%s'",  m_hostname.c_str());
    if (m_showinfomsg)
    {
      XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32001), m_hostname.c_str());
      XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32002), m_channelMap.size());
    }

    m_recordingsid = GetBuildInRecorderObjectID();

    m_recordingsid_by_date = m_recordingsid;
    m_recordingsid_by_date.append(DVBLINK_RECODINGS_BY_DATA_ID);

    m_recordingsid_by_series = m_recordingsid;
    m_recordingsid_by_series.append(DVBLINK_RECODINGS_BY_SERIES_ID);

    m_updating = true;
    CreateThread();
  }
  else
  {
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32003), m_hostname.c_str(), (int)status);
    std::string error;
    m_dvblinkRemoteCommunication->GetLastError(error);
    XBMC->Log(LOG_ERROR, "Could not connect to DVBLink Server '%s' on port '%i' with username '%s' (Error code : %d Description : %s)", hostname.c_str(), port, username.c_str(), (int)status,error.c_str());
  }
}

void *DVBLinkClient::Process()
{
  XBMC->Log(LOG_DEBUG, "DVBLinkUpdateProcess:: thread started");
  unsigned int counter = 0;
  while (m_updating)
  {
    if (counter >= 300000)
    {
      counter = 0;
      PVR->TriggerTimerUpdate();
      Sleep(5000);
      PVR->TriggerRecordingUpdate();
    }
    counter += 1000;
    Sleep(1000);
  }
  XBMC->Log(LOG_DEBUG, "DVBLinkUpdateProcess:: thread stopped");
  return NULL;
}


bool DVBLinkClient::GetStatus()
{
  return m_connected;
}

int DVBLinkClient::GetChannelsAmount()
{
  return m_channelMap.size();
}

PVR_ERROR DVBLinkClient::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  XBMC->Log(LOG_INFO, "Getting channels (%d channels on server)", m_channelMap.size());
  for (std::map<int,Channel*>::iterator it=m_channelMap.begin(); it!=m_channelMap.end(); ++it)
  {
    Channel* channel = (*it).second;
    
    bool isRadio = (channel->GetChannelType() == Channel::CHANNEL_TYPE_RADIO);

    if (isRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));
      xbmcChannel.bIsRadio = isRadio;
      xbmcChannel.iChannelNumber =channel->Number;
      xbmcChannel.iEncryptionSystem = 0;
      xbmcChannel.iUniqueId = (*it).first;

      PVR_STRCPY(xbmcChannel.strChannelName,channel->GetName().c_str());
      CStdString stream;

      //PVR_STRCPY(xbmcChannel.strIconPath, "special://userdata/addon_data/pvr.dvblink/channel.png");
      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

int DVBLinkClient::GetTimersAmount()
{
  return m_timerCount;
}


int DVBLinkClient::GetInternalUniqueIdFromChannelId(const std::string& channelId)
{
  for (std::map<int,Channel*>::iterator it=m_channelMap.begin(); it!=m_channelMap.end(); ++it)
  {
    Channel * channel = (*it).second;
    int id = (*it).first;
    if (channelId.compare(channel->GetID()) == 0)
    {
      return id;
    }
  }
  return 0;
}

std::string DVBLinkClient::make_timer_hash(const std::string& timer_id, const std::string& schedule_id)
{
    std::string res = schedule_id + "#" + timer_id;
    return res;
}

bool DVBLinkClient::parse_timer_hash(const char* timer_hash, std::string& timer_id, std::string& schedule_id)
{
    bool ret_val = false;

    std::string timer = timer_hash;
    size_t pos = timer.find('#');
    if (pos != std::string::npos)
    {
        timer_id = timer.c_str() + pos + 1;
        schedule_id = timer.substr(0, pos);
        ret_val = true;
    }

    return ret_val;
}


PVR_ERROR DVBLinkClient::GetTimers(ADDON_HANDLE handle)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  PLATFORM::CLockObject critsec(m_mutex);

  GetRecordingsRequest recordingsRequest;
  RecordingList recordings;

  DVBLinkRemoteStatusCode status;
  if ((status = m_dvblinkRemoteCommunication->GetRecordings(recordingsRequest, recordings)) != DVBLINK_REMOTE_STATUS_OK)
  {
    std::string error;
    m_dvblinkRemoteCommunication->GetLastError(error);
    XBMC->Log(LOG_ERROR,  "Could not get timers (Error code : %d Description : %s)", (int)status, error.c_str());
    return result;
  }

  XBMC->Log(LOG_INFO, "Found %d timers", recordings.size());
  
  if (m_showinfomsg)
  {
    XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32007), recordings.size());
  }

  for (size_t i=0; i < recordings.size(); i++)
  {
    Recording* rec = recordings[i];

    PVR_TIMER xbmcTimer;
    memset(&xbmcTimer, 0, sizeof(PVR_TIMER));
    //fake index
    xbmcTimer.iClientIndex = i;
    //misuse strDirectory to keep id of the timer
    std::string timer_hash = make_timer_hash(rec->GetID(), rec->GetScheduleID());
    PVR_STRCPY(xbmcTimer.strDirectory, timer_hash.c_str());
      
    xbmcTimer.iClientChannelUid = GetInternalUniqueIdFromChannelId(rec->GetChannelID());
    xbmcTimer.state = PVR_TIMER_STATE_SCHEDULED;
    if (rec->IsActive)
      xbmcTimer.state = PVR_TIMER_STATE_RECORDING;
    if (rec->IsConflict)
      xbmcTimer.state = PVR_TIMER_STATE_CONFLICT_NOK;
    if (!rec->GetProgram().IsRecord)
      xbmcTimer.state = PVR_TIMER_STATE_CANCELLED;

    xbmcTimer.bIsRepeating = rec->GetProgram().IsRepeatRecord;

    xbmcTimer.iEpgUid = rec->GetProgram().GetStartTime();
    
    xbmcTimer.startTime =rec->GetProgram().GetStartTime();
    xbmcTimer.endTime = rec->GetProgram().GetStartTime() + rec->GetProgram().GetDuration();
    PVR_STRCPY(xbmcTimer.strTitle, rec->GetProgram().GetTitle().c_str());
    PVR_STRCPY(xbmcTimer.strSummary, rec->GetProgram().ShortDescription.c_str());

    int genre_type, genre_subtype;
    SetEPGGenre(rec->GetProgram(), genre_type, genre_subtype);
    if (genre_type == EPG_GENRE_USE_STRING)
    {
      xbmcTimer.iGenreType = EPG_EVENT_CONTENTMASK_UNDEFINED;
    } else
    {
      xbmcTimer.iGenreType = genre_type;
      xbmcTimer.iGenreSubType = genre_subtype;
    }

    PVR->TransferTimerEntry(handle, &xbmcTimer);
    XBMC->Log(LOG_INFO, "Added EPG timer : %s", rec->GetProgram().GetTitle().c_str());
  }

  m_timerCount = recordings.size();
  result = PVR_ERROR_NO_ERROR;
  return result;
}

bool DVBLinkClient::get_dvblink_program_id(std::string& channelId, int start_time, std::string& dvblink_program_id)
{
    bool ret_val = false;

    EpgSearchResult epgSearchResult;
    if (DoEPGSearch(epgSearchResult, channelId, start_time, start_time))
    {
        if (epgSearchResult.size() > 0 && epgSearchResult.at(0)->GetEpgData().size() > 0)
        {
            dvblink_program_id = epgSearchResult.at(0)->GetEpgData().at(0)->GetID();
            ret_val = true;
        }
    }

    return ret_val;
}

static bool is_bit_set(int bit_num, unsigned char bit_field)
{
    unsigned char mask = (0x01 << bit_num);
    return (bit_field & mask) != 0;
}

PVR_ERROR DVBLinkClient::AddTimer(const PVR_TIMER &timer)
{
    PVR_ERROR result = PVR_ERROR_FAILED;
    PLATFORM::CLockObject critsec(m_mutex);
    DVBLinkRemoteStatusCode status;
    AddScheduleRequest * addScheduleRequest = NULL;
    std::string channelId = m_channelMap[timer.iClientChannelUid]->GetID();
    if (timer.iEpgUid != -1)
    {
        bool record_series = false;
        //do not ask if record is pressed during watching
        if (timer.startTime != 0)
        {
            // ask how to record this program
            CDialogRecordPref vWindow(XBMC, GUI, record_series);
            int dlg_res = vWindow.DoModal();
            if (dlg_res == 1)
            {
                record_series = vWindow.RecSeries;
            } else 
            {
                if (dlg_res == 0)
                    return PVR_ERROR_NO_ERROR;
            }
        }

        std::string dvblink_program_id;
        if (get_dvblink_program_id(channelId, timer.iEpgUid, dvblink_program_id))
        {
            addScheduleRequest = new AddScheduleByEpgRequest(channelId, dvblink_program_id, record_series);
        }
        else
        {
            return PVR_ERROR_FAILED;
        }
    }
    else
    {
        time_t start_time = timer.startTime;
        time_t duration = timer.endTime - timer.startTime;
        long day_mask = 0;
        if (timer.bIsRepeating)
        {
            //change day mask to DVBLink server format (Sun - first day)
            bool bcarry = (timer.iWeekdays & 0x40) == 0x40;
            day_mask = (timer.iWeekdays << 1) & 0x7F;
            if (bcarry)
                day_mask |= 0x01;
            //find first now/future time, which matches the day mask
            start_time = timer.startTime > timer.firstDay ? timer.startTime : timer.firstDay;
            for (size_t i = 0; i < 7; i++)
            {
                tm* local_start_time = localtime(&start_time);
                if (is_bit_set(local_start_time->tm_wday, (unsigned char)day_mask))
                    break;
                start_time += time_t(24 * 3600);
            }
        }
        addScheduleRequest = new AddManualScheduleRequest(channelId, start_time, duration, day_mask, timer.strTitle);
    }
  
    if ((status = m_dvblinkRemoteCommunication->AddSchedule(*addScheduleRequest)) == DVBLINK_REMOTE_STATUS_OK)
    {
        XBMC->Log(LOG_INFO, "Timer added");
        PVR->TriggerTimerUpdate();
        result = PVR_ERROR_NO_ERROR;
    }
    else
    {
        result = PVR_ERROR_FAILED;
        std::string error;
        m_dvblinkRemoteCommunication->GetLastError(error);
        XBMC->Log(LOG_ERROR, "Could not add timer (Error code : %d Description : %s)", (int)status, error.c_str());
    }
    SAFE_DELETE(addScheduleRequest);
    return result;
}

PVR_ERROR DVBLinkClient::DeleteTimer(const PVR_TIMER &timer)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  PLATFORM::CLockObject critsec(m_mutex);
  DVBLinkRemoteStatusCode status;
  
  //timer id hash is kept in strDirectory!
  std::string timer_id;
  std::string schedule_id;
  parse_timer_hash(timer.strDirectory, timer_id, schedule_id);

  bool cancel_series = true;
  if (timer.bIsRepeating)
  {
      //show dialog and ask: cancel series or just this episode
      CDialogDeleteTimer vWindow(XBMC, GUI, cancel_series);
      int dlg_res = vWindow.DoModal();
      if (dlg_res == 1)
      {
          cancel_series = vWindow.DeleteSeries;
      } else 
      {
          if (dlg_res == 0)
              return PVR_ERROR_NO_ERROR;
      }
  }

  if (cancel_series)
  {
      RemoveScheduleRequest removeSchedule(schedule_id);
      status = m_dvblinkRemoteCommunication->RemoveSchedule(removeSchedule);
  }
  else
  {
      RemoveRecordingRequest removeRecording(timer_id);
      status = m_dvblinkRemoteCommunication->RemoveRecording(removeRecording);
  }

  if (status == DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_INFO, "Timer(s) deleted");
    PVR->TriggerTimerUpdate();
    result = PVR_ERROR_NO_ERROR;
  }
  else
  {
    std::string error;
    m_dvblinkRemoteCommunication->GetLastError(error);
    XBMC->Log(LOG_ERROR, "Timer could not be deleted (Error code : %d Description : %s)", (int)status, error.c_str());
  }
  return result;
}

PVR_ERROR DVBLinkClient::UpdateTimer(const PVR_TIMER &timer)
{
  //DVBLink serverdoes not support timer update (e.g. delete/re-create)
  return PVR_ERROR_FAILED;
}

int DVBLinkClient::GetRecordingsAmount()
{

  return m_recordingCount;
}

std::string DVBLinkClient::GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID)
{
  std::string result = "";
  DVBLinkRemoteStatusCode status;

  GetPlaybackObjectRequest getPlaybackObjectRequest(m_hostname.c_str(), buildInRecoderObjectID);
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;


  if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse)) == DVBLINK_REMOTE_STATUS_OK)
  {
    for (std::vector<PlaybackContainer*>::iterator it = getPlaybackObjectResponse.GetPlaybackContainers().begin(); it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
    {
      PlaybackContainer * container = (PlaybackContainer *) *it;
      
     
      if (container->GetObjectID().find("F6F08949-2A07-4074-9E9D-423D877270BB") != std::string::npos)
      {
        result = container->GetObjectID();
        break;
      }
    }
  }
  return result;

}

PVR_ERROR DVBLinkClient::DeleteRecording(const PVR_RECORDING& recording)
{
    PLATFORM::CLockObject critsec(m_mutex);
  PVR_ERROR result = PVR_ERROR_FAILED;
  DVBLinkRemoteStatusCode status;
  RemovePlaybackObjectRequest remoteObj(recording.strRecordingId);
  
  if ((status = m_dvblinkRemoteCommunication->RemovePlaybackObject(remoteObj)) != DVBLINK_REMOTE_STATUS_OK)
{
    std::string error;
    m_dvblinkRemoteCommunication->GetLastError(error);
    XBMC->Log(LOG_ERROR, "Recording %s could not be deleted (Error code: %d Description : %s)", recording.strTitle, (int)status, error.c_str());
    return result;
  }

  XBMC->Log(LOG_INFO, "Recording %s deleted", recording.strTitle);
  PVR->TriggerRecordingUpdate();
  result = PVR_ERROR_NO_ERROR;
  return result;
}

static std::string get_subtitle(int season, int episode, const std::string& episode_name, int year)
{
    std::string se_str;
    char buf[1024];

    if (season > 0 || episode > 0)
    {
        se_str += "(";
        if (season > 0)
        {
            sprintf(buf, "S%02d", season);
            se_str += buf;
        }
        if (episode > 0)
        {
            sprintf(buf, "E%02d", episode);
            se_str += buf;
        }
        se_str += ")";
    }

    if (year > 0)
    {
        if (se_str.size() > 0)
            se_str += " ";

        se_str += "[";
        sprintf(buf, "%04d", year);
        se_str += buf;
        se_str += "]";
    }

    if (episode_name.size() > 0)
    {
        if (se_str.size() > 0)
            se_str += " - ";

        se_str += episode_name;
    }

    return se_str;
}

static void get_timer_id_from_recording_id(const std::string& recording_id, std::string& timer_id)
{
    //find last /
    std::string::size_type pos = recording_id.rfind("/");
    if (pos != std::string::npos)
    {
        timer_id = recording_id.substr(pos + 1);
    }
    else
    {
        timer_id = recording_id;
    }
}

bool DVBLinkClient::build_recording_series_map(std::map<std::string, std::string>& rec_id_to_series_name)
{
    //get all series containers
    PVR_ERROR result = PVR_ERROR_FAILED;
    DVBLinkRemoteStatusCode status;

    GetPlaybackObjectRequest getPlaybackObjectRequest(m_hostname.c_str(), m_recordingsid_by_series);
    getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
    GetPlaybackObjectResponse getPlaybackObjectResponse;

    if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse)) != DVBLINK_REMOTE_STATUS_OK)
    {
        std::string error;
        m_dvblinkRemoteCommunication->GetLastError(error);
        XBMC->Log(LOG_ERROR, "Could not get recording series container (Error code : %d Description : %s)", (int)status, error.c_str());
        return false;
    }

    PlaybackContainerList& series_containers = getPlaybackObjectResponse.GetPlaybackContainers();
    for (size_t i = 0; i<series_containers.size(); i++)
    {
        //ask for the items of this container
        GetPlaybackObjectRequest container_items_req(m_hostname.c_str(), series_containers.at(i)->GetObjectID());
        container_items_req.IncludeChildrenObjectsForRequestedObject = true;
        GetPlaybackObjectResponse container_items_resp;

        if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(container_items_req, container_items_resp)) == DVBLINK_REMOTE_STATUS_OK)
        {
            PlaybackItemList& playback_items = container_items_resp.GetPlaybackItems();
            for (size_t j = 0; j<playback_items.size(); j++)
            {
                std::string timer_id;
                get_timer_id_from_recording_id(playback_items.at(j)->GetObjectID(), timer_id);
                rec_id_to_series_name[timer_id] = series_containers.at(i)->GetName();
            }
        }
        else
        {
            std::string error;
            m_dvblinkRemoteCommunication->GetLastError(error);
            XBMC->Log(LOG_ERROR, "Could not get recording items for container %s (Error code : %d Description : %s)", series_containers.at(i)->GetObjectID().c_str(), (int)status, error.c_str());
        }
    }

    return true;
}

PVR_ERROR DVBLinkClient::GetRecordings(ADDON_HANDLE handle)
{
    PLATFORM::CLockObject critsec(m_mutex);
    PVR_ERROR result = PVR_ERROR_FAILED;
    DVBLinkRemoteStatusCode status;
    m_recording_id_to_url_map.clear();

    GetPlaybackObjectRequest getPlaybackObjectRequest(m_hostname.c_str(), m_recordingsid_by_date);
    getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
    GetPlaybackObjectResponse getPlaybackObjectResponse;

    if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse)) != DVBLINK_REMOTE_STATUS_OK)
    {
        std::string error;
        m_dvblinkRemoteCommunication->GetLastError(error);
        XBMC->Log(LOG_ERROR, "Could not get recordings (Error code : %d Description : %s)", (int)status, error.c_str());
        //XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32004), (int)status);
        return result;
    }

    XBMC->Log(LOG_INFO, "Found %d recordings", getPlaybackObjectResponse.GetPlaybackItems().size());

    if (m_showinfomsg)
    {
        XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32009), getPlaybackObjectResponse.GetPlaybackItems().size());
    }

    //get a map of recording per series
    std::map<std::string, std::string> rec_id_to_series_name;

    if (m_group_recordings_by_series)
        build_recording_series_map(rec_id_to_series_name);

    for (std::vector<PlaybackItem*>::iterator it = getPlaybackObjectResponse.GetPlaybackItems().begin(); it < getPlaybackObjectResponse.GetPlaybackItems().end(); it++)
    {
        RecordedTvItem * tvitem = (RecordedTvItem *)*it;
        PVR_RECORDING xbmcRecording;
        memset(&xbmcRecording, 0, sizeof(PVR_RECORDING));

        PVR_STRCPY(xbmcRecording.strRecordingId, tvitem->GetObjectID().c_str());

        std::string title = tvitem->GetMetadata().GetTitle();
        if (m_add_episode_to_rec_title)
        {
            //form a title as "name - (SxxExx) subtitle" because XBMC does not display episode/season information almost anywhere
            std::string se_str = get_subtitle(tvitem->GetMetadata().SeasonNumber, tvitem->GetMetadata().EpisodeNumber, tvitem->GetMetadata().SubTitle, (int)tvitem->GetMetadata().Year);
            if (se_str.size() > 0)
                title += " - " + se_str;
        }
        PVR_STRCPY(xbmcRecording.strTitle, title.c_str());

        xbmcRecording.recordingTime = tvitem->GetMetadata().GetStartTime();
        PVR_STRCPY(xbmcRecording.strPlot, tvitem->GetMetadata().ShortDescription.c_str());
        PVR_STRCPY(xbmcRecording.strPlotOutline, tvitem->GetMetadata().SubTitle.c_str());
        //    PVR_STRCPY(xbmcRecording.strStreamURL, tvitem->GetPlaybackUrl().c_str());
        m_recording_id_to_url_map[xbmcRecording.strRecordingId] = tvitem->GetPlaybackUrl();
        xbmcRecording.iDuration = tvitem->GetMetadata().GetDuration();
        PVR_STRCPY(xbmcRecording.strChannelName, tvitem->ChannelName.c_str());
        PVR_STRCPY(xbmcRecording.strThumbnailPath, tvitem->GetThumbnailUrl().c_str());
        int genre_type, genre_subtype;
        SetEPGGenre(tvitem->GetMetadata(), genre_type, genre_subtype);
        if (genre_type == EPG_GENRE_USE_STRING)
        {
            xbmcRecording.iGenreType = EPG_EVENT_CONTENTMASK_UNDEFINED;
        }
        else
        {
            xbmcRecording.iGenreType = genre_type;
            xbmcRecording.iGenreSubType = genre_subtype;
        }

        if (m_group_recordings_by_series)
        {
            //compare timer_ids
            std::string timer_id;
            get_timer_id_from_recording_id(tvitem->GetObjectID(), timer_id);

            if (rec_id_to_series_name.find(timer_id) != rec_id_to_series_name.end())
            {
                PVR_STRCPY(xbmcRecording.strDirectory, rec_id_to_series_name[timer_id].c_str());
            }
        }

        PVR->TransferRecordingEntry(handle, &xbmcRecording);

    }
    m_recordingCount = getPlaybackObjectResponse.GetPlaybackItems().size();
    result = PVR_ERROR_NO_ERROR;
    return result;
}

bool DVBLinkClient::GetRecordingURL(const char* recording_id, std::string& url)
{
    bool ret_val = false;
    if (m_recording_id_to_url_map.find(recording_id) != m_recording_id_to_url_map.end())
    {
        url = m_recording_id_to_url_map[recording_id];
        ret_val = true;
    }
    else
    {
        XBMC->Log(LOG_ERROR, "Could not get playback url for recording %s)", recording_id);
    }
    return ret_val;
}

void DVBLinkClient::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  PLATFORM::CLockObject critsec(m_mutex);
  GetRecordingSettingsRequest recordingsettingsrequest;
  *iTotal = 0;
  *iUsed = 0;
  RecordingSettings settings;
  DVBLinkRemoteStatusCode status;
  if ((status = m_dvblinkRemoteCommunication->GetRecordingSettings(recordingsettingsrequest, settings)) == DVBLINK_REMOTE_STATUS_OK)
  {
    *iTotal = settings.TotalSpace;
    *iUsed = (settings.TotalSpace - settings.AvailableSpace);
  }
}


int DVBLinkClient::GetCurrentChannelId()
{
  return m_currentChannelId;
}

bool DVBLinkClient::StartStreaming(const PVR_CHANNEL &channel, StreamRequest* streamRequest, std::string& stream_url)
{
  DVBLinkRemoteStatusCode status;
  if ((status = m_dvblinkRemoteCommunication->PlayChannel(*streamRequest, *m_stream)) != DVBLINK_REMOTE_STATUS_OK)
  {
    std::string error;
    m_dvblinkRemoteCommunication->GetLastError(error);
    XBMC->Log(LOG_ERROR, "Could not start streaming for channel %i (Error code : %d)", channel.iUniqueId, (int)status,error.c_str());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32010), channel.strChannelName, (int)status);
    return false;
  }

  m_currentChannelId = channel.iUniqueId;
  stream_url = m_stream->GetUrl();
  return true;
}

bool DVBLinkClient::OpenLiveStream(const PVR_CHANNEL &channel, bool use_timeshift, bool use_transcoder, int width, int height, int bitrate, std::string audiotrack)
{
    bool ret_val = false;

    PLATFORM::CLockObject critsec(m_mutex);

    if (m_live_streamer)
    {
        SAFE_DELETE(m_live_streamer);
    }
	//time-shifted playback always uses raw http stream type - no transcoding option is possible now
    if (use_timeshift)
        m_live_streamer = new TimeShiftBuffer(XBMC);
    else
        m_live_streamer = new LiveTVStreamer(XBMC);

    //adjust transcoded height and width if needed
    int w = width == 0 ? GUI->GetScreenWidth() : width;
    int h = height == 0 ? GUI->GetScreenHeight() : height;

    Channel * c = m_channelMap[channel.iUniqueId];

    StreamRequest* sr = m_live_streamer->GetStreamRequest(c->GetDvbLinkID(), m_clientname, m_hostname, use_transcoder, w, h, bitrate, audiotrack);
    if (sr != NULL)
    {
        std::string url;
        if (StartStreaming(channel, sr, url))
        {
            m_live_streamer->Start(url);
            ret_val = true;
        }
        else
        {
            delete m_live_streamer;
            m_live_streamer = NULL;
        }
        delete sr;
    }
    else
    {
        XBMC->Log(LOG_ERROR, "m_live_streamer->GetStreamRequest returned NULL. (channel %i)", channel.iUniqueId);
        delete m_live_streamer;
        m_live_streamer = NULL;
    }
    return ret_val;
}

int DVBLinkClient::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
    if (m_live_streamer)
        return m_live_streamer->ReadData(pBuffer, iBufferSize);
  return 0;
}

long long DVBLinkClient::SeekLiveStream(long long iPosition, int iWhence)
{
    if (m_live_streamer)
        return m_live_streamer->Seek(iPosition, iWhence);
  return 0;
}

long long DVBLinkClient::PositionLiveStream(void)
{
    if (m_live_streamer)
        return m_live_streamer->Position();
  return 0;
}
long long DVBLinkClient::LengthLiveStream(void)
{
    if (m_live_streamer)
        return m_live_streamer->Length();
  return 0;
}

time_t DVBLinkClient::GetPlayingTime()
{
    if (m_live_streamer)
        return m_live_streamer->GetPlayingTime();
  return 0;
}

time_t DVBLinkClient::GetBufferTimeStart()
{
    if (m_live_streamer)
        return m_live_streamer->GetBufferTimeStart();
  return 0;
}

time_t DVBLinkClient::GetBufferTimeEnd()
{
    if (m_live_streamer)
        return m_live_streamer->GetBufferTimeEnd();
  return 0;
}

void DVBLinkClient::StopStreaming(bool bUseChlHandle)
{
  PLATFORM::CLockObject critsec(m_mutex);
  StopStreamRequest * request;
 
  if (m_live_streamer != NULL)
  {
      m_live_streamer->Stop();
      SAFE_DELETE(m_live_streamer);
      m_live_streamer = NULL;
  }

  if (bUseChlHandle)
  {
    request = new StopStreamRequest(m_stream->GetChannelHandle());
  }
  else
  {
    request = new StopStreamRequest(m_clientname);
  }

  DVBLinkRemoteStatusCode status;
  if ((status = m_dvblinkRemoteCommunication->StopChannel(*request)) != DVBLINK_REMOTE_STATUS_OK)
  {
    std::string error;
    m_dvblinkRemoteCommunication->GetLastError(error);
    XBMC->Log(LOG_ERROR, "Could not stop stream (Error code : %d Description : %s)", (int)status, error.c_str());
  }


  SAFE_DELETE(request);
}

void DVBLinkClient::SetEPGGenre(dvblinkremote::ItemMetadata& metadata, int& genre_type, int& genre_subtype)
{
  genre_type = EPG_GENRE_USE_STRING;
  genre_subtype = 0x00;

  if (metadata.IsCatNews)
  {
    genre_type = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
    genre_subtype = 0x00;
  }

  if (metadata.IsCatDocumentary)
  {
    genre_type = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
    genre_subtype = 0x03;
  }


  if (metadata.IsCatEducational)
  {
    genre_type = EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE;
  }

  if (metadata.IsCatSports)
  {
    genre_type = EPG_EVENT_CONTENTMASK_SPORTS;
  }

  if (metadata.IsCatMovie)
  {
    genre_type = EPG_EVENT_CONTENTMASK_MOVIEDRAMA;
    genre_subtype =metadata.IsCatThriller ? 0x01 : metadata.IsCatScifi ? 0x03 :metadata.IsCatHorror ? 0x03 : metadata.IsCatComedy ? 0x04 : metadata.IsCatSoap ? 0x05 : metadata.IsCatRomance ? 0x06 :metadata.IsCatDrama ? 0x08 : 0;
  }

  if (metadata.IsCatKids)
  {
    genre_type = EPG_EVENT_CONTENTMASK_CHILDRENYOUTH;
  }

  if (metadata.IsCatMusic)
  {
    genre_type = EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE;
  }

  if (metadata.IsCatSpecial)
  {
    genre_type = EPG_EVENT_CONTENTMASK_SPECIAL;
  }
}

bool  DVBLinkClient::DoEPGSearch(EpgSearchResult& epgSearchResult, const std::string& channelId, const long startTime, const long endTime, const std::string& programId)
{
  PLATFORM::CLockObject critsec(m_mutex);
  EpgSearchRequest epgSearchRequest(channelId, startTime, endTime);
  if (programId.compare("") != 0)
  {
    epgSearchRequest.ProgramID = programId;
  }

  DVBLinkRemoteStatusCode status;

  if ((status = m_dvblinkRemoteCommunication->SearchEpg(epgSearchRequest, epgSearchResult)) == DVBLINK_REMOTE_STATUS_OK)
  {
    return true;
  }
  return false;
}

PVR_ERROR DVBLinkClient::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  PLATFORM::CLockObject critsec(m_mutex);
  Channel * c = m_channelMap[channel.iUniqueId];
  EpgSearchResult epgSearchResult;

  if (DoEPGSearch(epgSearchResult,c->GetID(), iStart, iEnd))
  {
    for (std::vector<ChannelEpgData*>::iterator it = epgSearchResult.begin(); it < epgSearchResult.end(); it++) 
    {
      ChannelEpgData* channelEpgData = (ChannelEpgData*)*it;
      EpgData& epgData = channelEpgData->GetEpgData();
      for (std::vector<Program*>::iterator pIt = epgData.begin(); pIt < epgData.end(); pIt++) 
      {
        Program* p = (Program*)*pIt;
        EPG_TAG broadcast;
        memset(&broadcast, 0, sizeof(EPG_TAG));

        broadcast.iUniqueBroadcastId = p->GetStartTime();
        broadcast.strTitle = p->GetTitle().c_str();
        broadcast.iChannelNumber      = channel.iChannelNumber;
        broadcast.startTime           = p->GetStartTime();
        broadcast.endTime             = p->GetStartTime() + p->GetDuration();
        broadcast.strPlotOutline      = p->SubTitle.c_str();
        broadcast.strPlot             = p->ShortDescription.c_str();
        
        broadcast.strIconPath         = p->Image.c_str();
        broadcast.iGenreType          = 0;
        broadcast.iGenreSubType       = 0;
        broadcast.strGenreDescription = "";
        broadcast.firstAired          = 0;
        broadcast.iParentalRating     = 0;
        broadcast.iStarRating         = p->Rating;
        broadcast.bNotify             = false;
        broadcast.iSeriesNumber       = p->SeasonNumber;
        broadcast.iEpisodeNumber      = p->EpisodeNumber;
        broadcast.iEpisodePartNumber  = 0;
        broadcast.strEpisodeName      = p->SubTitle.c_str();

        int genre_type, genre_subtype;
        SetEPGGenre(*p, genre_type, genre_subtype);
        broadcast.iGenreType = genre_type;
        if (genre_type == EPG_GENRE_USE_STRING)
          broadcast.strGenreDescription = p->Keywords.c_str();
        else
          broadcast.iGenreSubType = genre_subtype;

        PVR->TransferEpgEntry(handle, &broadcast);
      }
    }
    result = PVR_ERROR_NO_ERROR;
  }
  else
  {
    XBMC->Log(LOG_NOTICE, "Not EPG data found for channel : %s with id : %i", channel.strChannelName, channel.iUniqueId);
  }
  return result;
}

DVBLinkClient::~DVBLinkClient(void)
{
  m_updating = false;
  if (IsRunning())
  {
    StopThread();
  }
  
  SAFE_DELETE(m_dvblinkRemoteCommunication);
  SAFE_DELETE(m_httpClient);
  SAFE_DELETE(m_channels);
  SAFE_DELETE(m_stream);
  if (m_live_streamer)
  {
      m_live_streamer->Stop();
      SAFE_DELETE(m_live_streamer);
  }
}
