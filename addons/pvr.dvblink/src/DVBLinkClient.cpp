#include "DVBLinkClient.h"
#include "platform/util/StdString.h"

bool DVBLinkClient::LoadChannelsFromFile()
{

	CStdString channelfile("special://userdata/addon_data/pvr.dvblink/channels");

	// do we already have the icon file?
	if (!XBMC->FileExists(channelfile, false))
	{        
		return false;    
	} 


	CStdString strResult;
	void* fileHandle = XBMC->OpenFile(channelfile, 0);
	if (fileHandle)
	{
		char buffer[1024];
		while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
			strResult.append(buffer, bytesRead);
		XBMC->CloseFile(fileHandle);
	}


	return true;
}


DVBLinkClient::DVBLinkClient(CHelper_libXBMC_addon  *XBMC, CHelper_libXBMC_pvr *PVR,std::string clientname, std::string hostname, long port, std::string username, std::string password)
{
	this->PVR = PVR;
	this->XBMC = XBMC;
	this->clientname = clientname;
	this->hostname = hostname;
	connected = false;
	currentChannelId = 0;
	httpClient = new HttpPostClient(XBMC,hostname,port, username, password);
	dvblinkRemoteCommunication = DVBLinkRemote::Connect((HttpClient&)*httpClient, hostname.c_str(), 8080, username.c_str(), password.c_str());

	DVBLinkRemoteStatusCode status;
	channels = NULL;
	timerCount = -1;
	recordingCount = -1;

	stream = new Stream();

	GetChannelsRequest* request = new GetChannelsRequest();
	channels = new ChannelList();

	if ((status = dvblinkRemoteCommunication->GetChannels(*request, *channels)) == DVBLINK_REMOTE_STATUS_OK) {
		int iChannelUnique = 0;
		for (std::vector<Channel*>::iterator it = channels->begin(); it < channels->end(); it++) 
		{
			Channel* channel = (*it);
			channelMap[++iChannelUnique] = channel;
		}
		connected = true;
		XBMC->Log(LOG_INFO, "Connected to DVBLink Server");
	}else
	{
		XBMC->Log(LOG_ERROR, "Could not get channels from DVBLink Server '%s' on port '%i'", hostname.c_str(),port);
	}
	SAFE_DELETE(request);
}

bool DVBLinkClient::GetStatus()
{
	return connected;
}

int DVBLinkClient::GetChannelsAmount()
{
	return channelMap.size();
}

PVR_ERROR DVBLinkClient::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
	for( std::map<int,Channel*>::iterator it=channelMap.begin(); it!=channelMap.end(); ++it)
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
			if(isRadio)
				stream.Format("pvr://stream/radio/%i.ts", channel->GetDvbLinkID());
			else
				stream.Format("pvr://stream/tv/%i.ts", channel->GetDvbLinkID());

			PVR_STRCPY(xbmcChannel.strStreamURL, stream.c_str());
			//pvrchannel.strInputFormat = "";
			//pvrchannel.strIconPath = "";
			PVR->TransferChannelEntry(handle, &xbmcChannel);
		}
	}
	return PVR_ERROR_NO_ERROR;
}

int DVBLinkClient::GetTimersAmount()
{
	return timerCount;
}


int DVBLinkClient::GetInternalUniqueIdFromChannelId(const std::string& channelId)
{

	for( std::map<int,Channel*>::iterator it=channelMap.begin(); it!=channelMap.end(); ++it)
	{
		Channel * channel = (*it).second;
		int id = (*it).first;
		if (channelId.compare(channel->GetID())==0)
		{
			return id;
		}
	}
	return 0;
}

PVR_ERROR DVBLinkClient::GetTimers(ADDON_HANDLE handle)
{
	PVR_ERROR result = PVR_ERROR_FAILED;
	PLATFORM::CLockObject critsec(m_mutex);

	GetSchedulesRequest getSchedulesRequest;

	StoredSchedules sschedules;

	DVBLinkRemoteStatusCode status;
	int count = 0;
	if ((status = dvblinkRemoteCommunication->GetSchedules(getSchedulesRequest, sschedules)) == DVBLINK_REMOTE_STATUS_OK) {

		for (std::vector<StoredEpgSchedule*>::iterator it = sschedules.GetEpgSchedules().begin(); it < sschedules.GetEpgSchedules().end(); it++) 
		{
			StoredEpgSchedule* schedule = (StoredEpgSchedule*)*it;
			PVR_TIMER xbmcTimer;
			memset(&xbmcTimer, 0, sizeof(PVR_TIMER));
			PVR_STR2INT(xbmcTimer.iClientIndex, schedule->GetID().c_str());
			
			xbmcTimer.iClientChannelUid = GetInternalUniqueIdFromChannelId(schedule->GetChannelID());
			xbmcTimer.state = PVR_TIMER_STATE_SCHEDULED;
			xbmcTimer.bIsRepeating = schedule->Repeat;
			PVR_STR2INT(xbmcTimer.iEpgUid,schedule->GetProgramID().c_str());

			EpgSearchResult epgSearchResult;
			if (DoEPGSearch(epgSearchResult,schedule->GetChannelID(), -1, -1,schedule->GetProgramID())) {
				ChannelEpgData * channelepgdata = epgSearchResult[0];
				
				Program * program = channelepgdata->GetEpgData()[0];
		
				xbmcTimer.startTime =program->GetStartTime();
				xbmcTimer.endTime = program->GetStartTime() + program->GetDuration();
				PVR_STRCPY(xbmcTimer.strTitle,program->GetTitle().c_str());
				PVR_STRCPY(xbmcTimer.strSummary,program->ShortDescription.c_str());
				
			}			
			PVR->TransferTimerEntry(handle, &xbmcTimer);
			count++;
		}
		for (std::vector<StoredManualSchedule*>::iterator it = sschedules.GetManualSchedules().begin(); it < sschedules.GetManualSchedules().end(); it++) 
		{
			StoredManualSchedule* schedule = (StoredManualSchedule*)*it;
			PVR_TIMER xbmcTimer;
			memset(&xbmcTimer, 0, sizeof(PVR_TIMER));
			PVR_STR2INT(xbmcTimer.iClientIndex, schedule->GetID().c_str());
			
			xbmcTimer.iClientChannelUid = GetInternalUniqueIdFromChannelId(schedule->GetChannelID());


			xbmcTimer.state = PVR_TIMER_STATE_SCHEDULED;
			xbmcTimer.startTime = schedule->GetStartTime();
			xbmcTimer.endTime =  schedule->GetStartTime() + schedule->GetDuration();
			PVR_STRCPY(xbmcTimer.strTitle,schedule->Title.c_str());
			//TODO: PAE: Add weekdays
			PVR->TransferTimerEntry(handle, &xbmcTimer);
			count++;
		}

		timerCount = count;
		result = PVR_ERROR_NO_ERROR;
	}
	return result;
}

PVR_ERROR DVBLinkClient::AddTimer(const PVR_TIMER &timer)
{
	PVR_ERROR result = PVR_ERROR_FAILED;
	PLATFORM::CLockObject critsec(m_mutex);
	DVBLinkRemoteStatusCode status;
	AddScheduleRequest * addScheduleRequest = NULL;
	std::string channelId = channelMap[timer.iClientChannelUid]->GetID();
	if (timer.iEpgUid != 0)
	{
		char programId [33];
		PVR_INT2STR(programId,timer.iEpgUid);
		addScheduleRequest = new AddScheduleByEpgRequest(channelId,programId,timer.bIsRepeating);
	}else{
		//TODO: Fix day mask
		addScheduleRequest = new AddManualScheduleRequest(channelId, timer.startTime, timer.endTime - timer.startTime,-1,timer.strTitle);
	}
	if ( (status = dvblinkRemoteCommunication->AddSchedule(*addScheduleRequest)) == DVBLINK_REMOTE_STATUS_OK)
	{
		PVR->TriggerTimerUpdate();
		result = PVR_ERROR_NO_ERROR;
	}else{
		result = PVR_ERROR_FAILED;
		XBMC->Log(LOG_ERROR,"Could not add timer");
	}
	SAFE_DELETE(addScheduleRequest);
	return result;
}

PVR_ERROR DVBLinkClient::DeleteTimer(const PVR_TIMER &timer)
{
	PVR_ERROR result = PVR_ERROR_FAILED;
	PLATFORM::CLockObject critsec(m_mutex);
	DVBLinkRemoteStatusCode status;
	char scheduleId [33];
	PVR_INT2STR(scheduleId,timer.iClientIndex);
	
	RemoveScheduleRequest removeSchedule(scheduleId);


	if ((status = dvblinkRemoteCommunication->RemoveSchedule(removeSchedule)) == DVBLINK_REMOTE_STATUS_OK) {

		PVR->TriggerTimerUpdate();
		result = PVR_ERROR_NO_ERROR;
	}
	return result;
}

PVR_ERROR DVBLinkClient::UpdateTimer(const PVR_TIMER &timer)
{
	PVR_ERROR deleteResult = DeleteTimer(timer);
	if (deleteResult == PVR_ERROR_NO_ERROR)
	{
		return AddTimer(timer);
	}
	return deleteResult;
}

int DVBLinkClient::GetRecordingsAmount()
{

	return recordingCount;
}

std::string DVBLinkClient::GetBuildInRecorderObjectID()
{
	std::string result = "";
	DVBLinkRemoteStatusCode status;
	GetPlaybackObjectRequest getPlaybackObjectRequest(hostname.c_str(),"");
	getPlaybackObjectRequest.RequestedObjectType = GetPlaybackObjectRequest::REQUESTED_OBJECT_TYPE_ALL;
	getPlaybackObjectRequest.RequestedItemType = GetPlaybackObjectRequest::REQUESTED_ITEM_TYPE_ALL;
	getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
	GetPlaybackObjectResponse getPlaybackObjectResponse;
	if ((status = dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse)) == DVBLINK_REMOTE_STATUS_OK) {
		for(std::vector<PlaybackContainer*>::iterator it = getPlaybackObjectResponse.GetPlaybackContainers().begin(); it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
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


std::string DVBLinkClient::GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID)
{
	std::string result = "";
	DVBLinkRemoteStatusCode status;

	GetPlaybackObjectRequest getPlaybackObjectRequest(hostname.c_str(),buildInRecoderObjectID);
	getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
	GetPlaybackObjectResponse getPlaybackObjectResponse;


	if ((status = dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse)) == DVBLINK_REMOTE_STATUS_OK) {
		for(std::vector<PlaybackContainer*>::iterator it = getPlaybackObjectResponse.GetPlaybackContainers().begin(); it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
		{
			PlaybackContainer * container = (PlaybackContainer *) *it;
			if (strcmp(container->GetName().c_str(), "By Date") == 0)
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
	
	if ((status = dvblinkRemoteCommunication->RemovePlaybackObject(remoteObj)) == DVBLINK_REMOTE_STATUS_OK) {

		PVR->TriggerRecordingUpdate();
		 result = PVR_ERROR_NO_ERROR;
	}
	return result;
}


PVR_ERROR DVBLinkClient::GetRecordings(ADDON_HANDLE handle)
{
	PLATFORM::CLockObject critsec(m_mutex);
	PVR_ERROR result = PVR_ERROR_FAILED;
	DVBLinkRemoteStatusCode status;

	std::string recoderObjectId = GetBuildInRecorderObjectID();
	std::string recordingsByDateObjectId = GetRecordedTVByDateObjectID(recoderObjectId);


	GetPlaybackObjectRequest getPlaybackObjectRequest(hostname.c_str(),recordingsByDateObjectId);
	getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
	GetPlaybackObjectResponse getPlaybackObjectResponse;

	if ((status = dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse)) == DVBLINK_REMOTE_STATUS_OK) {
		
		
		for(std::vector<PlaybackItem*>::iterator it = getPlaybackObjectResponse.GetPlaybackItems().begin(); it < getPlaybackObjectResponse.GetPlaybackItems().end(); it++)
		{		

			RecordedTvItem * tvitem = (RecordedTvItem *) *it;
			PVR_RECORDING xbmcRecording;
			memset(&xbmcRecording, 0, sizeof(PVR_RECORDING));
			
			PVR_STRCPY(xbmcRecording.strRecordingId,tvitem->GetObjectID().c_str());
			
			PVR_STRCPY(xbmcRecording.strTitle,tvitem->GetMetadata().GetTitle().c_str());
			
			xbmcRecording.recordingTime = tvitem->GetMetadata().GetStartTime();
			PVR_STRCPY(xbmcRecording.strPlot, tvitem->GetMetadata().ShortDescription.c_str());
			PVR_STRCPY(xbmcRecording.strStreamURL, tvitem->GetPlaybackUrl().c_str());
			xbmcRecording.iDuration =  tvitem->GetMetadata().GetDuration();
			PVR_STRCPY(xbmcRecording.strChannelName,tvitem->ChannelName.c_str());
			PVR_STRCPY(xbmcRecording.strThumbnailPath, tvitem->GetThumbnailUrl().c_str());
			PVR->TransferRecordingEntry(handle, &xbmcRecording);

		}
		recordingCount = getPlaybackObjectResponse.GetPlaybackItems().size();
		result = PVR_ERROR_NO_ERROR;
	}
	return result;
}

long DVBLinkClient::GetFreeDiskSpace()
{
	
	PLATFORM::CLockObject critsec(m_mutex);
	GetRecordingSettingsRequest recordingsettingsrequest;

	RecordingSettings settings;
	DVBLinkRemoteStatusCode status;
	if ((status = dvblinkRemoteCommunication->GetRecordingSettings(recordingsettingsrequest, settings)) != DVBLINK_REMOTE_STATUS_OK) {
		return (settings.TotalSpace - settings.AvailableSpace) * 1024;
	}
	
	return 0;
}

long DVBLinkClient::GetTotalDiskSpace()
{
	PLATFORM::CLockObject critsec(m_mutex);
	GetRecordingSettingsRequest recordingsettingsrequest;

	RecordingSettings settings;
	DVBLinkRemoteStatusCode status;
	if ((status = dvblinkRemoteCommunication->GetRecordingSettings(recordingsettingsrequest, settings)) != DVBLINK_REMOTE_STATUS_OK) {
		return settings.TotalSpace * 1024;
	}
	return 0;
}

int DVBLinkClient::GetCurrentChannelId()
{
	return currentChannelId;
}

const char * DVBLinkClient::GetLiveStreamURL(const PVR_CHANNEL &channel, DVBLINK_STREAMTYPE streamtype, int width, int height, int bitrate, std::string audiotrack)
{
	PLATFORM::CLockObject critsec(m_mutex);
	StreamRequest* streamRequest = NULL;
	TranscodingOptions options(width, height);
	options.SetBitrate(bitrate);
	options.SetAudioTrack(audiotrack);
	Channel * c = channelMap[channel.iUniqueId];
	DVBLinkRemoteStatusCode status;
 	switch(streamtype)
	{
	case HTTP:
		streamRequest = new RawHttpStreamRequest(hostname.c_str(), c->GetDvbLinkID(), clientname.c_str());
		break;
	case RTP:
		streamRequest = new RealTimeTransportProtocolStreamRequest(hostname.c_str(), c->GetDvbLinkID(), clientname.c_str(), options);
		break;
	case HLS:
		streamRequest = new HttpLiveStreamRequest(hostname.c_str(), c->GetDvbLinkID(), clientname.c_str(), options);
		break;
	case ASF:
		streamRequest = new WindowsMediaStreamRequest(hostname.c_str(), c->GetDvbLinkID(), clientname.c_str(), options);
		break;
	}

	if ((status = dvblinkRemoteCommunication->PlayChannel(*streamRequest, *stream)) != DVBLINK_REMOTE_STATUS_OK) {
		XBMC->Log(LOG_ERROR,"Could not get stream for channel %i", channel.iUniqueId);
		SAFE_DELETE(streamRequest);
		return "";
	}
	else
	{
		currentChannelId = channel.iUniqueId;
		SAFE_DELETE(streamRequest);
		return stream->GetUrl().c_str();
	}
}

void DVBLinkClient::StopStreaming(bool bUseChlHandle)
{
	PLATFORM::CLockObject critsec(m_mutex);
	StopStreamRequest * request;

	if (bUseChlHandle) {
		request = new StopStreamRequest(stream->GetChannelHandle());
	}
	else {
		request = new StopStreamRequest(clientname);
	}

	DVBLinkRemoteStatusCode status;
	if ((status = dvblinkRemoteCommunication->StopChannel(*request)) != DVBLINK_REMOTE_STATUS_OK) {
		XBMC->Log(LOG_ERROR, "Could not stop stream");
	}
	SAFE_DELETE(request);

}

void DVBLinkClient::SetEPGGenre(Program *program, EPG_TAG *tag)
{

	if (program->IsCatNews)
	{
		tag->iGenreType = 0x20;
		tag->iGenreSubType = 0x02;
	}
	if (program->IsCatDocumentary)
	{
		tag->iGenreType = 0x20;
		tag->iGenreSubType = 0x03;
	}

	if (program->IsCatEducational)
	{
		tag->iGenreType = 0x90;
	}

	if (program->IsCatSports)
	{
		tag->iGenreType = 0x40;
	}

	if (program->IsCatMovie)
	{
		tag->iGenreType = 0x10;
		tag->iGenreSubType =program->IsCatComedy ? 0x04 : 0;
		tag->iGenreSubType = program->IsCatRomance ? 0x06 : 0;
		tag->iGenreSubType = program->IsCatDrama ? 0x08 : 0;
		tag->iGenreSubType = program->IsCatThriller ? 0x01 : 0;
		tag->iGenreSubType = program->IsCatScifi ? 0x03 : 0;
		tag->iGenreSubType = program->IsCatSoap ? 0x05 : 0;
		tag->iGenreSubType = program->IsCatHorror ? 0x03 : 0;
	}

	if (program->IsCatKids)
	{
		tag->iGenreType = 0x50;
	}

	if (program->IsCatMusic)
	{
		tag->iGenreType = 0x60;
	}

	if (program->IsCatSpecial)
	{
		tag->iGenreType = 0xB0;
	}

}

bool  DVBLinkClient::DoEPGSearch(EpgSearchResult& epgSearchResult, const std::string& channelId, const long startTime, const long endTime, const std::string& programId)
{
	PLATFORM::CLockObject critsec(m_mutex);
	EpgSearchRequest epgSearchRequest(channelId, startTime, endTime);
	if (programId.compare("")!= 0)
	{
		epgSearchRequest.ProgramID = programId;
	}

	DVBLinkRemoteStatusCode status;

	if ((status = dvblinkRemoteCommunication->SearchEpg(epgSearchRequest, epgSearchResult)) == DVBLINK_REMOTE_STATUS_OK) {
	
		return true;
	}

	return false;
}

PVR_ERROR DVBLinkClient::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
	PVR_ERROR result = PVR_ERROR_FAILED;
	PLATFORM::CLockObject critsec(m_mutex);
	Channel * c = channelMap[channel.iUniqueId];
	EpgSearchResult epgSearchResult;

	

	if (DoEPGSearch(epgSearchResult,c->GetID(), iStart, iEnd)) {
		for (std::vector<ChannelEpgData*>::iterator it = epgSearchResult.begin(); it < epgSearchResult.end(); it++) 
		{
			ChannelEpgData* channelEpgData = (ChannelEpgData*)*it;
			EpgData& epgData = channelEpgData->GetEpgData();
			for (std::vector<Program*>::iterator pIt = epgData.begin(); pIt < epgData.end(); pIt++) 
			{

				Program* p = (Program*)*pIt;
				EPG_TAG broadcast;
				memset(&broadcast, 0, sizeof(EPG_TAG));

				
				PVR_STR2INT(broadcast.iUniqueBroadcastId,p->GetID().c_str() );
	
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
				broadcast.iSeriesNumber       = 0;
				broadcast.iEpisodeNumber      = p->EpisodeNumber;
				broadcast.iEpisodePartNumber  = 0;
				broadcast.strEpisodeName      = "";
				
				SetEPGGenre(p, &broadcast);

				PVR->TransferEpgEntry(handle, &broadcast);
			}
		}
		result = PVR_ERROR_NO_ERROR;
	}else
	{
		XBMC->Log(LOG_ERROR, "Not EPG data found for channel : %s with id : %i", channel.strChannelName, channel.iUniqueId);

	}
	return result;
}

DVBLinkClient::~DVBLinkClient(void)
{
	SAFE_DELETE(dvblinkRemoteCommunication);
	SAFE_DELETE(httpClient);
	SAFE_DELETE(channels);
	SAFE_DELETE(stream);
}
