#include "DvbData.h"

#include "client.h" 

using namespace ADDON;
using namespace PLATFORM;

void Dvb::TimerUpdates()
{
	std::deque<DvbTimer> newtimer = LoadTimers();

	std::deque<DvbTimer>::iterator iter;
	for (iter = m_timers.begin(); iter != m_timers.end(); iter++)
		(*iter).iUpdateState = DVB_UPDATE_STATE_NONE;

	unsigned int iUpdated=0;
	unsigned int iUnchanged=0; 

	if (m_timers.size()>0) // No need to doo an empty outer loop.
	{
		std::deque<DvbTimer>::iterator iternewtimer;
		for (iternewtimer = newtimer.begin(); iternewtimer != newtimer.end(); iternewtimer++)
		{
			std::deque<DvbTimer>::iterator iter;
			for (iter = m_timers.begin(); iter != m_timers.end(); iter++) 
			{
				if ((*iter).like((*iternewtimer)))
				{
					if((*iter) == (*iternewtimer))
					{
						(*iter).iUpdateState = DVB_UPDATE_STATE_FOUND;
						(*iternewtimer).iUpdateState = DVB_UPDATE_STATE_FOUND;
						iUnchanged++;
					}
					else
					{
						(*iternewtimer).iUpdateState = DVB_UPDATE_STATE_UPDATED;
						(*iter).iUpdateState = DVB_UPDATE_STATE_UPDATED;
						(*iter).strTitle = (*iternewtimer).strTitle;
						(*iter).strPlot = (*iternewtimer).strPlot;
						(*iter).iChannelId = (*iternewtimer).iChannelId;
						(*iter).startTime = (*iternewtimer).startTime;
						(*iter).endTime = (*iternewtimer).endTime;
						(*iter).bRepeating = (*iternewtimer).bRepeating;
						(*iter).iWeekdays = (*iternewtimer).iWeekdays;
						(*iter).iEpgID = (*iternewtimer).iEpgID;
						(*iter).iTimerID = (*iternewtimer).iTimerID;
						(*iter).iPriority = (*iternewtimer).iPriority;
						(*iter).iFirstDay = (*iternewtimer).iFirstDay;
						(*iter).state = (*iternewtimer).state;

						iUpdated++;
					}
				}
			}
		}
	}

	unsigned int iRemoved = 0;
	iter = m_timers.begin();
	while (iter != m_timers.end())
	{
		if ((*iter).iUpdateState == DVB_UPDATE_STATE_NONE)
		{
			XBMC->Log(LOG_INFO, "%s Removed timer: '%s', ClientIndex: '%d'", __FUNCTION__, (*iter).strTitle.c_str(), (*iter).iClientIndex);
			iter = m_timers.erase(iter);
			iRemoved++;
		}
		else
			iter++;
	}
	unsigned int iNew=0;

	for (iter = newtimer.begin(); iter != newtimer.end(); iter++) 
	{ 
		if((*iter).iUpdateState == DVB_UPDATE_STATE_NEW)
		{  
			(*iter).iClientIndex = m_iClientIndexCounter;
			XBMC->Log(LOG_INFO, "%s New timer: '%s', ClientIndex: '%d'", __FUNCTION__, (*iter).strTitle.c_str(), m_iClientIndexCounter);
			m_timers.push_back((*iter));
			m_iClientIndexCounter++;
			iNew++;
		} 
	}

	XBMC->Log(LOG_INFO, "%s No of timers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew); 

	if (iRemoved != 0 || iUpdated != 0 || iNew != 0) 
	{
		XBMC->Log(LOG_INFO, "%s Changes in timerlist detected, trigger an update!", __FUNCTION__);
		PVR->TriggerTimerUpdate();
	}
}

Dvb::Dvb() 
{
	m_bIsConnected = false;
	m_strServerName = "DVBViewer";
	CStdString strURL = "", strURLStream = "", strURLRecording = "";

	// simply add user@pass in front of the URL if username/password is set
	if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
		strURL.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
	strURL.Format("http://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
	m_strURL = strURL.c_str();

	//Not needed anymore. The URL comes with the recordings list.
	/* if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
	strURLRecording.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
	strURLRecording.Format("http://%s%s:%u/", strURLRecording.c_str(), g_strHostname.c_str(), g_iPortRecording);
	m_strURLRecording = strURLRecording.c_str();
	*/

	//The streaming server does not use authentification. It conflicts with UPnP clients.

	//if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
	//  strURLStream.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
	//strURLStream.Format("http://%s:%u/", g_strHostname.c_str(), g_iPortStream);
//	m_strURLStream = strURLStream.c_str();

	m_iNumChannelGroups = 0;
	m_iCurrentChannel = -1;
	m_iClientIndexCounter = 1;

	m_iUpdateTimer = 0;
	m_bUpdateTimers = false;
	m_bUpdateEPG = false;
}

bool Dvb::Open()
{
	CLockObject lock(m_mutex);

	XBMC->Log(LOG_NOTICE, "%s - DVBViewer Addon Configuration options", __FUNCTION__);
	XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
	XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, g_iPortWeb);

	m_bIsConnected = GetDeviceInfo();

	if (!m_bIsConnected)
		return false;

	if (!LoadChannels())
		return false;

	//GetPreferredLanguage(); // see GetTimeZone()
	GetTimeZone();

	TimerUpdates();

	XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
	CreateThread();

	return IsRunning(); 
}

void  *Dvb::Process()
{
	XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

	while(!IsStopped())
	{
		Sleep(1000);
		m_iUpdateTimer++;

		if (m_bUpdateEPG)
		{
			Sleep(8000); /* Sleep enough time to let the recording service grab the EPG data */
			PVR->TriggerEpgUpdate(m_iCurrentChannel);
			m_bUpdateEPG = false;
		}

		if ((m_iUpdateTimer > 60) || m_bUpdateTimers)
		{
			m_iUpdateTimer = 0;

			// Trigger Timer and Recording updates acording to the addon settings
			CLockObject lock(m_mutex);
			XBMC->Log(LOG_INFO, "%s Perform Updates!", __FUNCTION__);

			if (m_bUpdateTimers)
			{
				Sleep(500);
				m_bUpdateTimers = false;
			}
			TimerUpdates();
			PVR->TriggerRecordingUpdate();
		}
	}

	CLockObject lock(m_mutex);
	m_started.Broadcast();
	//XBMC->Log(LOG_DEBUG, "%s - exiting", __FUNCTION__);

	return NULL;
}

DvbChannel Dvb::ExtractChannelData(CStdString gName, XMLNode& xTmpChannel, int channel_pos)
{
	DvbChannel datChannel;
	CStdString strTmp;

	strTmp = xTmpChannel.getAttribute("EPGID");
	sscanf(strTmp, "%lld|", &datChannel.llEpgId);

	datChannel.strChannelName = xTmpChannel.getAttribute("name");

	strTmp = xTmpChannel.getAttribute("ID");
	sscanf(strTmp, "%lld|", &datChannel.iChannelId);

	strTmp = xTmpChannel.getAttribute("flags");

	if (!strTmp.IsEmpty()) {
		datChannel.bRadio = (atoi(strTmp.c_str()) & VIDEO_FLAG) == 0;    
		datChannel.Encrypted =  atoi(strTmp.c_str()) & ENCRYPTED_FLAG;    
	}
	else {
		datChannel.bRadio = false;
		datChannel.Encrypted = 0;
	}

	datChannel.strGroupName = gName.c_str();
	datChannel.iUniqueId = channel_pos;
	datChannel.iChannelNumber = channel_pos;

	if (!g_bUseRTSP) {
		int nr;
		strTmp = xTmpChannel.getAttribute("nr");
		sscanf(strTmp, "%d", &nr);
		strTmp.Format("%s%d.ts", m_strURLStream.c_str(), nr);
	}
	else {
		XMLNode xChannelRTSP = xTmpChannel.getChildNode("rtsp");
		strTmp.Format("%s%s", m_strURLStream.c_str(), xChannelRTSP.getText());
	}

	datChannel.strStreamURL = strTmp;

	XMLNode xChannelLogo = xTmpChannel.getChildNode("logo");
	strTmp.Format("%s%s", m_strURL.c_str(), xChannelLogo.getText()); 
	datChannel.strIconPath = strTmp;

	return datChannel;
}

bool Dvb::LoadChannels() 
{
	m_groups.clear();
	m_iNumChannelGroups = 0;
	m_strURLStream.clear();

	CStdString url;

	DvbChannel datChannel;
	DvbChannelGroup datGroup;

	// request whole channel xml if not favorites are used
	if (!g_bUseFavourites)
	{
		url.Format("%sapi/getchannelsxml.html?rtsp=1&upnp=1&logo=1", m_strURL.c_str()); 
		CStdString strXML;
		strXML = GetHttpXML(url);

		if(!strXML.size()) {
			XBMC->Log(LOG_ERROR, "%s Unable to load getchannelsxml.html.", __FUNCTION__);
			return false;
		}

		XMLResults xe;
		XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

		if(xe.error != 0)  {
			XBMC->Log(LOG_ERROR, "%s Unable to parse getchannelsxml.html. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
			return false;
		}

		XMLNode xNode = xMainNode.getChildNode("channels");

		if (!g_bUseRTSP)      
			xNode = xNode.getChildNode("upnpURL");
		else 
			xNode = xNode.getChildNode("rtspURL");

		m_strURLStream = xNode.getText();

		xNode = xMainNode.getChildNode("channels");
		int nroot = xNode.nChildNode("root");

		for (int i=0; i<nroot; i++)
		{
			XMLNode xTmpGroup = xNode.getChildNode("root", i);
			int ngroup = xTmpGroup.nChildNode("group");

			for (int x=0; x<ngroup; x++)
			{
				XMLNode xTmpChannels = xTmpGroup.getChildNode("group", x);
				int nchannels = xTmpChannels.nChildNode("channel");
				datGroup.strGroupName = xTmpChannels.getAttribute("name");
				m_groups.push_back(datGroup);
				m_iNumChannelGroups++;

				for (int y=0; y<nchannels; y++)
				{
					XMLNode xTmpChannel = xTmpChannels.getChildNode("channel", y);
					datChannel = ExtractChannelData(datGroup.strGroupName, xTmpChannel, m_channels.size() + 1); 
					m_channels.push_back(datChannel);
					
					/* Subchannels aren't usefull for channel handling
					int nsubchannels = xTmpChannel.nChildNode("subchannel");

					for (int c=0; c<nsubchannels; c++)
					{
						channel_pos++;
						XMLNode xSubTmpChannel = xTmpChannel.getChildNode("subchannel", c);
						if (!g_bUseRTSP) {
							datChannel = ExtractChannelData(datGroup.strGroupName, xSubTmpChannel, channel_pos, datChannel.strStreamURL.c_str()); 
						}
						else {
							datChannel = ExtractChannelData(datGroup.strGroupName, xSubTmpChannel, channel_pos); 
						}
						datChannel.iChannelNumber = m_channels.size() + 1;
						m_channels.push_back(datChannel);			
					}
					*/
				}
			}
		}
	}
	else
	{
		// get favorites.xml first
		CStdString urlFav = g_strFavouritesPath;
		if (!XBMC->FileExists(urlFav.c_str(), false))
			urlFav.Format("%sapi/getfavourites.html", m_strURL.c_str());

		CStdString strXML;
		strXML = GetHttpXML(urlFav);

		if(!strXML.size())
		{
			XBMC->Log(LOG_ERROR, "%s Unable to load favourites.xml.", __FUNCTION__);
			return false;
		}

		XMLResults xe;
		XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

		if(xe.error != 0)  {
			XBMC->Log(LOG_ERROR, "%s Unable to parse favourites.xml. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
			return false;
		}

		XMLNode xNode = xMainNode.getChildNode("settings");
		int m = xNode.nChildNode("section");

		for (int i = 0; i<m; i++)
		{
			CStdString groupName;
			XMLNode xTmp = xNode.getChildNode("section", i);
			int n = xTmp.nChildNode("entry");

			for (int j = 0; j<n; j++)
			{
				CStdString strTmp;
				XMLNode xTmp2 = xTmp.getChildNode("entry", j);
				strTmp = xTmp2.getText();
				uint64_t iChannelId;

				if (sscanf(strTmp, "%lld|", &iChannelId))
				{
					if (n == 1)
						groupName = "";

					url.Format("%sapi/getchannelsxml.html?rtsp=1&upnp=1&logo=1&id=%lld", m_strURL.c_str(), iChannelId); 
					CStdString strXML;
					strXML = GetHttpXML(url);

					if(!strXML.size()) {
						XBMC->Log(LOG_ERROR, "%s Unable to load getchannelsxml.html for favorites", __FUNCTION__);
						return false;
					}

					XMLResults xe;
					XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

					if(xe.error != 0)  {
						XBMC->Log(LOG_ERROR, "%s Unable to parse getchannelsxml.html. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
						return false;
					}

					XMLNode xNode = xMainNode.getChildNode("channels");

					if (m_strURLStream.size() == 0)
					{
						if (!g_bUseRTSP)      
							xNode = xNode.getChildNode("upnpURL");
						else
							xNode = xNode.getChildNode("rtspURL");

						m_strURLStream = xNode.getText();
					}

					XMLNode xTmpChannel =xMainNode.getChildNodeByPath("channels/root/group/channel");
					datChannel = ExtractChannelData(groupName, xTmpChannel, m_channels.size() + 1); 
					m_channels.push_back(datChannel);
				}
				else
				{
					groupName = strTmp;
					char* strGroupNameUtf8 = XBMC->UnknownToUTF8(groupName.c_str());
					datGroup.strGroupName = strGroupNameUtf8;
					m_groups.push_back(datGroup);
					XBMC->FreeString(strGroupNameUtf8);
				}
			}
		}
	}

	XBMC->Log(LOG_INFO, "%s Loaded %d Channelsgroups", __FUNCTION__, m_iNumChannelGroups);
	return true;
}

bool Dvb::IsConnected() 
{
  return m_bIsConnected;
}

CStdString Dvb::GetHttpXML(CStdString& url)
{
  CStdString strResult;
  void* fileHandle = XBMC->OpenFile(url.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      strResult.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }
  return strResult;
}

const char * Dvb::GetServerName() 
{
  return m_strServerName.c_str();  
}

int Dvb::GetChannelsAmount()
{
  return m_channels.size();
}

int Dvb::GetTimersAmount()
{
  return m_timers.size();
}

unsigned int Dvb::GetRecordingsAmount() {
  return m_iNumRecordings;
}

PVR_ERROR Dvb::GetChannels(ADDON_HANDLE handle, bool bRadio)
{

	std::deque<DvbChannel>::iterator iter;
	for (iter = m_channels.begin(); iter != m_channels.end(); iter++)
	{
		if ((*iter).bRadio == bRadio)
		{
			PVR_CHANNEL xbmcChannel;
			memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

			xbmcChannel.iUniqueId         = (*iter).iUniqueId;
			xbmcChannel.bIsRadio          = (*iter).bRadio;
			xbmcChannel.iChannelNumber    = (*iter).iChannelNumber;
			strncpy(xbmcChannel.strChannelName, (*iter).strChannelName.c_str(), sizeof(xbmcChannel.strChannelName));

			/*
			if (!g_bUseRTSP) {
			strncpy(xbmcChannel.strInputFormat, "video/x-mpegts", sizeof("video/x-mpegts"));       
			}
			else {
			strncpy(xbmcChannel.strInputFormat, "", sizeof(xbmcChannel.strInputFormat)); // unused
			}
			*/

			CStdString strStream;
			//strStream.Format("pvr://stream/tv/%i.ts", channel.iUniqueId);
			strStream.Format("%s", (*iter).strStreamURL);
			strncpy(xbmcChannel.strStreamURL, strStream.c_str(), sizeof(xbmcChannel.strStreamURL)); //channel.strStreamURL.c_str();
			xbmcChannel.iEncryptionSystem = (*iter).Encrypted;

			strncpy(xbmcChannel.strIconPath, (*iter).strIconPath.c_str(), sizeof(xbmcChannel.strIconPath));
			xbmcChannel.bIsHidden         = false;

			PVR->TransferChannelEntry(handle, &xbmcChannel);
		}
	}

	return PVR_ERROR_NO_ERROR;
}

Dvb::~Dvb() 
{
  StopThread();
  
  m_channels.clear();  
  m_timers.clear();
  m_recordings.clear();
  m_groups.clear();
  m_bIsConnected = false;
}

int Dvb::ParseDateTime(CStdString strDate, bool iDateFormat)
{
  struct tm timeinfo;

  memset(&timeinfo, 0, sizeof(tm));
  if (iDateFormat)
    sscanf(strDate, "%04d%02d%02d%02d%02d%02d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
  else
    sscanf(strDate, "%02d.%02d.%04d%02d:%02d:%02d", &timeinfo.tm_mday, &timeinfo.tm_mon, &timeinfo.tm_year, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
  timeinfo.tm_mon  -= 1;
  timeinfo.tm_year -= 1900;
  timeinfo.tm_isdst = -1;

  return mktime(&timeinfo);
}

PVR_ERROR Dvb::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
	DvbChannel &myChannel = m_channels[channel.iUniqueId-1];

	CStdString url;
	url.Format("%sapi/epg.html?lng=%s&channel=%lld&start=%f&end=%f", m_strURL.c_str(), m_strEPGLanguage.c_str(), myChannel.llEpgId, iStart/86400.0 + DELPHI_DATE, iEnd/86400.0 + DELPHI_DATE); 

	CStdString strXML;
	strXML = GetHttpXML(url);

	if (strXML.length()==0)
		return PVR_ERROR_NO_ERROR;

	int iNumEPG = 0;

	XMLResults xe;
	XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

	if(xe.error != 0)  {
		XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
		return PVR_ERROR_SERVER_ERROR;
	}

	XMLNode xNode = xMainNode.getChildNode("epg");
	int n = xNode.nChildNode("prg");

	XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

	for (int i = 0; i<n; i++)
	{
		CStdString strTmp;
		int iTmpStart;
		int iTmpEnd;

		XMLNode xTmp = xNode.getChildNode("prg", i);
		iTmpStart = ParseDateTime(xTmp.getAttribute("start"));
		iTmpEnd = ParseDateTime(xTmp.getAttribute("stop"));

		// isn't needed, it doesn't matter if end time is later than XBMC EPG time setting
		//if ((iEnd > 1) && (iEnd < iTmpEnd))
		//	continue;

		DvbEPGEntry entry;
		entry.startTime = iTmpStart;
		entry.endTime = iTmpEnd;

		entry.iEventId = atoi(xTmp.getAttribute("evid"));

		if (entry.iEventId==0)  
			continue;

		if(!GetString(xTmp,"title",strTmp))
			continue;

		entry.strTitle = strTmp;

		CStdString strTmp2;
		GetString(xTmp,"descr", strTmp);
		GetString(xTmp, "event", strTmp2);
		if (strTmp.size() > strTmp2.size()) {
			entry.strPlot = strTmp;
			entry.strPlotOutline = strTmp2;
		}
		else
			entry.strPlot = strTmp2;

		EPG_TAG broadcast;
		memset(&broadcast, 0, sizeof(EPG_TAG));

		broadcast.iUniqueBroadcastId  = entry.iEventId;
		broadcast.strTitle            = entry.strTitle.c_str();
		broadcast.iChannelNumber      = channel.iChannelNumber;
		broadcast.startTime           = entry.startTime;
		broadcast.endTime             = entry.endTime;
		broadcast.strPlotOutline      = entry.strPlotOutline.c_str();
		broadcast.strPlot             = entry.strPlot.c_str();
		broadcast.strIconPath         = ""; // unused
		broadcast.iGenreType          = 0; // unused
		broadcast.iGenreSubType       = 0; // unused
		broadcast.strGenreDescription = "";
		broadcast.firstAired          = 0;  // unused
		broadcast.iParentalRating     = 0;  // unused
		broadcast.iStarRating         = 0;  // unused
		broadcast.bNotify             = false;
		broadcast.iSeriesNumber       = 0;  // unused
		broadcast.iEpisodeNumber      = 0;  // unused
		broadcast.iEpisodePartNumber  = 0;  // unused
		broadcast.strEpisodeName      = ""; // unused

		PVR->TransferEpgEntry(handle, &broadcast);

		iNumEPG++; 

		XBMC->Log(LOG_INFO, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, channel.iChannelNumber, entry.startTime, entry.endTime);
	}

	XBMC->Log(LOG_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
	return PVR_ERROR_NO_ERROR;
}

int Dvb::GetChannelNumber(CStdString strChannelId)  
{
	uint64_t channel;
	sscanf(strChannelId.c_str(), "%lld|", &channel);	
	XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strChannelId);
	XBMC->Log(LOG_DEBUG, "%s Processing timer '%lld'", __FUNCTION__, channel);

	std::deque<DvbChannel>::iterator iter;
	int i;
	for (iter = m_channels.begin(), i = 1; iter != m_channels.end(); iter++, i++)
	{
		if ((*iter).iChannelId == channel)
			return i;
	}
	return PVR_ERROR_UNKNOWN;
}

PVR_ERROR Dvb::GetTimers(ADDON_HANDLE handle)
{
	XBMC->Log(LOG_INFO, "%s - timers available '%d'", __FUNCTION__, m_timers.size());
	
	std::deque<DvbTimer>::iterator iter;
	for (iter = m_timers.begin(); iter != m_timers.end(); iter++)
	{
		XBMC->Log(LOG_INFO, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, (*iter).strTitle.c_str(), (*iter).iClientIndex);
		PVR_TIMER tag;
		memset(&tag, 0, sizeof(PVR_TIMER));
		tag.iClientChannelUid	= (*iter).iChannelId;
		tag.startTime			= (*iter).startTime;
		tag.endTime				= (*iter).endTime;
		strncpy(tag.strTitle, (*iter).strTitle.c_str(), sizeof(tag.strTitle));
		strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
		strncpy(tag.strSummary, (*iter).strPlot.c_str(), sizeof(tag.strSummary));
		tag.state				= (*iter).state;
		tag.iPriority			= (*iter).iPriority;
		tag.iLifetime			= 0;     // unused
		tag.bIsRepeating		= (*iter).bRepeating;
		tag.firstDay			= (*iter).iFirstDay;
		tag.iWeekdays			= (*iter).iWeekdays;
		tag.iEpgUid				= (*iter).iEpgID;
		tag.iMarginStart		= 0;     // unused
		tag.iMarginEnd			= 0;     // unused
		tag.iGenreType			= 0;     // unused
		tag.iGenreSubType		= 0;     // unused
		tag.iClientIndex		= (*iter).iClientIndex;

		PVR->TransferTimerEntry(handle, &tag);
	}
	return PVR_ERROR_NO_ERROR;
}

std::deque<DvbTimer> Dvb::LoadTimers()
{
	CStdString url; 
	url.Format("%s%s", m_strURL.c_str(), "api/timerlist.html?utf8");

	CStdString strXML;
	strXML = GetHttpXML(url);

	XMLResults xe;
	XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

	std::deque<DvbTimer> timers;

	if(xe.error != 0)  {
		XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
		return timers;
	}

	XMLNode xNode = xMainNode.getChildNode("Timers");
	int n = xNode.nChildNode("Timer");

	XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

	for (int i = n-1; i>=0; i--)
	{
		XMLNode xTmp = xNode.getChildNode("Timer", i);

		CStdString strTmp;
		int iTmp;

		if (GetString(xTmp, "Descr", strTmp)) 
			XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());

		DvbTimer timer;

		timer.strTitle = strTmp;
		timer.iChannelId = GetChannelNumber(xTmp.getChildNode("Channel").getAttribute("ID"));
		timer.state = PVR_TIMER_STATE_SCHEDULED;

		CStdString DateTime;
		DateTime = xTmp.getAttribute("Date");
		DateTime.append(xTmp.getAttribute("Start"));
		timer.startTime = ParseDateTime(DateTime, false);
		timer.endTime = timer.startTime + atoi(xTmp.getAttribute("Dur"))*60;

		CStdString Weekdays;
		Weekdays = xTmp.getAttribute("Days");
		timer.iWeekdays = 0;
		for (unsigned int i = 0; i<Weekdays.size(); i++)
		{
			if (Weekdays.data()[i] != '-')
				timer.iWeekdays += (1 << i);
		}

		if (timer.iWeekdays != 0)
		{
			timer.iFirstDay = timer.startTime;
			timer.bRepeating = true; 
		}
		else
			timer.bRepeating = false;

		timer.iPriority = atoi(xTmp.getAttribute("Priority"));

		if (xTmp.getAttribute("EPGEventID"))
			timer.iEpgID = atoi(xTmp.getAttribute("EPGEventID"));

		if (xTmp.getAttribute("Enabled")[0] == '0')
			timer.state = PVR_TIMER_STATE_CANCELLED;

		if (GetInt(xTmp, "Recording", iTmp))
		{
			if (iTmp == -1) timer.state = PVR_TIMER_STATE_RECORDING;
		}

		if (GetInt(xTmp, "ID", iTmp))
			timer.iTimerID = iTmp;

		timers.push_back(timer);

		XBMC->Log(LOG_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
	}

	XBMC->Log(LOG_INFO, "%s fetched %u Timer Entries", __FUNCTION__, timers.size());
	return timers; 
}

int Dvb::GetTimerID(const PVR_TIMER &timer)
{
	std::deque<DvbTimer>::iterator iter;
	for (iter = m_timers.begin(); iter != m_timers.end(); iter++)
	{
		if ((*iter).iClientIndex == timer.iClientIndex)
			return (*iter).iTimerID;
	}
	return PVR_ERROR_UNKNOWN;
}

void Dvb::GenerateTimer(const PVR_TIMER &timer, bool bNewTimer)
{
	XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

	struct tm *timeinfo;
	time_t startTime = timer.startTime, endTime = timer.endTime;
	if (!startTime)
		time(&startTime);
	else
	{
		startTime -= timer.iMarginStart*60;
		endTime += timer.iMarginEnd*60;
	}

	int dor = ((startTime + m_iTimezone*60) / DAY_SECS) + DELPHI_DATE;
	timeinfo = localtime(&startTime);
	int start = timeinfo->tm_hour*60 + timeinfo->tm_min;
	timeinfo = localtime(&endTime);
	int stop = timeinfo->tm_hour*60 + timeinfo->tm_min;

	char strWeek[8] = "-------";
	for (int i = 0; i<7; i++)
	{
		if (timer.iWeekdays & (1 << i)) strWeek[i] = 'T';
	}

	uint64_t iChannelId = m_channels.at(timer.iClientChannelUid-1).iChannelId;
	CStdString strTmp;
	if (bNewTimer)
		strTmp.Format("api/timeradd.html?ch=%lld&dor=%d&enable=1&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255", iChannelId, dor, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
	else
	{
		int enabled = 1;
		if (timer.state == PVR_TIMER_STATE_CANCELLED)
			enabled = 0;
		strTmp.Format("api/timeredit.html?id=%d&ch=%lld&dor=%d&enable=%d&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255", GetTimerID(timer), iChannelId, dor, enabled, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
	}

	SendSimpleCommand(strTmp);
	m_bUpdateTimers = true;
}

PVR_ERROR Dvb::AddTimer(const PVR_TIMER &timer)
{
  GenerateTimer(timer);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::UpdateTimer(const PVR_TIMER &timer)
{
  GenerateTimer(timer, false);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::DeleteTimer(const PVR_TIMER &timer) 
{
  CStdString strTmp;
  strTmp.Format("api/timerdelete.html?id=%d", GetTimerID(timer));
  SendSimpleCommand(strTmp);

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  m_bUpdateTimers = true;

  return PVR_ERROR_NO_ERROR;
}

void Dvb::SetRecordingTag(PVR_RECORDING& tag, DvbRecording& recording)
{
	memset(&tag, 0, sizeof(PVR_RECORDING));
	strncpy(tag.strRecordingId, recording.strRecordingId.c_str(), sizeof(tag.strRecordingId));
	strncpy(tag.strTitle, recording.strTitle.c_str(), sizeof(tag.strTitle));
	strncpy(tag.strStreamURL, recording.strStreamURL.c_str(), sizeof(tag.strStreamURL));
	strncpy(tag.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(tag.strPlotOutline));
	strncpy(tag.strPlot, recording.strPlot.c_str(), sizeof(tag.strPlot));
	strncpy(tag.strChannelName, recording.strChannelName.c_str(), sizeof(tag.strChannelName));
	strncpy(tag.strThumbnailPath, recording.strThumbnailPath.c_str(), sizeof(tag.strThumbnailPath));
	tag.recordingTime     = recording.startTime;
	tag.iDuration         = recording.iDuration;
	strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
}

PVR_ERROR Dvb::GetRecordings(ADDON_HANDLE handle)
{
	//m_recordings.clear();

	CStdString url;
	url.Format("%s%s", m_strURL.c_str(), "api/recordings.html?images=1&nofilename=1");

	CStdString strXML;
	strXML = GetHttpXML(url);

	XMLResults xe;
	XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

	if(xe.error != 0)  {
		XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
		return PVR_ERROR_SERVER_ERROR;
	}

	XMLNode xNode = xMainNode.getChildNode("recordings");

	//Get the server base URLs for thumbnails and streaming
	CStdString strimgURL;
	GetString(xNode,"imageURL",strimgURL);

	CStdString strStreamURL;
	GetString(xNode,"serverURL",strStreamURL);

	int n = xNode.nChildNode("recording");

	// This variable makes sure we only compare already present entries (or none if the list was empty).
	// There is no use comparing new added entries against the same download.
	int iold_m_recsize;
	iold_m_recsize = m_recordings.size();

	XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

	// set available to false for later clean up
	for (uint16_t i = 0; i < iold_m_recsize; i++)
		m_recordings[i].bStillAvailable = false;

	for (int i = n-1; i>=0; i--)  // I like for loops better. A lot easier to read than the while construct...
	{
		XMLNode xTmp = xNode.getChildNode("recording", i);
		DvbRecording recording;
		CStdString strTmp;
		bool bAlreadyListed = false;

		PVR_RECORDING tag;		

		recording.strRecordingId = xTmp.getAttribute("id");

		// check if recording was already listed
		for (uint16_t x = 0; x < iold_m_recsize; x++)
		{
			// recording found already in list
			if (!strcmp(recording.strRecordingId.c_str(), m_recordings[x].strRecordingId.c_str()))
			{
				SetRecordingTag(tag, m_recordings[x]);
				PVR->TransferRecordingEntry(handle, &tag);

				bAlreadyListed = true;
				m_recordings[x].bStillAvailable = true;
				break;
			}
		}

		// new recording, collect data
		if (!bAlreadyListed)
		{		
			if (GetString(xTmp, "title", strTmp))
				recording.strTitle = strTmp;

			CStdString strTmp2;
			GetString(xTmp, "desc", strTmp);
			GetString(xTmp, "info", strTmp2);
			if (strTmp.size() > strTmp2.size()){
				recording.strPlot = strTmp;
				recording.strPlotOutline = strTmp2;
			}else
				recording.strPlot = strTmp2;

			if (GetString(xTmp, "channel", strTmp))
				recording.strChannelName = strTmp;

			if (GetString(xTmp, "image", strTmp))
				recording.strThumbnailPath = strimgURL + strTmp;

			strTmp = recording.strRecordingId;
			recording.strStreamURL = strStreamURL + strTmp + ".ts";

			recording.startTime = ParseDateTime(xTmp.getAttribute("start"));

			int hours, mins, secs;
			sscanf(xTmp.getAttribute("duration"), "%02d%02d%02d", &hours, &mins, &secs);
			recording.iDuration = hours*60*60 + mins*60 + secs;

			recording.bStillAvailable = true;
			
			SetRecordingTag(tag, recording);
			PVR->TransferRecordingEntry(handle, &tag);

			m_recordings.push_back(recording);
			XBMC->Log(LOG_INFO, "%s loaded Recording entry '%s', start '%d', length '%d', ID '%s'", __FUNCTION__, tag.strTitle, recording.startTime, recording.iDuration,tag.strRecordingId);
		}
	}
	
	// clean up recordings from list
	std::deque<DvbRecording>::iterator iter = m_recordings.begin();
	while (iter != m_recordings.end())
	{
		if (!iter->bStillAvailable)
			iter = m_recordings.erase(iter);
		else
			++iter;
	}

	XBMC->Log(LOG_INFO, "%s Loaded %u Recording Entries", __FUNCTION__, m_recordings.size());

	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::DeleteRecording(const PVR_RECORDING &recinfo) 
{
  CStdString strTmp;
  strTmp.Format("rec_list.html?aktion=delete_rec&recid=%s", recinfo.strRecordingId);
  SendSimpleCommand(strTmp);

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

void Dvb::SendSimpleCommand(const CStdString& strCommandURL)
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), strCommandURL.c_str()); 
  GetHttpXML(url);
}

CStdString Dvb::URLEncodeInline(const CStdString& strData)
{
  /* Copied from xbmc/URL.cpp */

  CStdString strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strData.length() * 2 );

  for (int i = 0; i < (int)strData.size(); ++i)
  {
    int kar = (unsigned char)strData[i];
    //if (kar == ' ') strResult += '+'; // obsolete
    if (isalnum(kar) || strchr("-_.!()" , kar) ) // Don't URL encode these according to RFC1738
    {
      strResult += kar;
    }
    else
    {
      CStdString strTmp;
      strTmp.Format("%%%02.2x", kar);
      strResult += strTmp;
    }
  }
  return strResult;
}

bool Dvb::GetInt(XMLNode xRootNode, const char* strTag, int& iIntValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (xNode.isEmpty())
     return false;
  iIntValue = atoi(xNode.getText());
  return true;
}

bool Dvb::GetBoolean(XMLNode xRootNode, const char* strTag, bool& bBoolValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (xNode.isEmpty()) 
    return false;

  CStdString strEnabled = xNode.getText();

  strEnabled.ToLower();
  if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" || strEnabled == "false" || strEnabled == "0" )
    bBoolValue = false;
  else
  {
    bBoolValue = true;
    if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" && strEnabled != "true")
      return false; // invalid bool switch - it's probably some other string.
  }
  return true;
}

bool Dvb::GetString(XMLNode xRootNode, const char* strTag, CStdString& strStringValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (!xNode.isEmpty())
  {
    strStringValue = xNode.getText();
    return true;
  }
  strStringValue.Empty();
  return false;
}

bool Dvb::GetStringLng(XMLNode xRootNode, const char* strTag, CStdString& strStringValue)
{
  XMLNode xNode;
  bool found = false;
  int n = xRootNode.nChildNode(strTag);

  if (n > 1)
  {
    for (int i = 0; i<n; i++)
    {
      xNode = xRootNode.getChildNode(strTag, i);
      CStdString strTmp;
      strTmp = xNode.getAttribute("lng");
      if (strTmp == m_strEPGLanguage)
      {
        found = true;
        break;
      }
    }
  }

  if (!found) xNode = xRootNode.getChildNode(strTag);
  if (!xNode.isEmpty())
  {
    strStringValue = xNode.getText();
    return true;
  }
  strStringValue.Empty();
  return false;
}

void Dvb::GetPreferredLanguage()
{
  CStdString strXML, url;
  url.Format("%s%s",  m_strURL.c_str(), "config.html?aktion=config"); 
  strXML = GetHttpXML(url);
  unsigned int iLanguagePos = strXML.find("EPGLanguage");
  iLanguagePos = strXML.find(" selected>", iLanguagePos);
  m_strEPGLanguage = strXML.substr(iLanguagePos-4, 3);
}

void Dvb::GetTimeZone()
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), "api/status.html"); 

  CStdString strXML;
  strXML = GetHttpXML(url);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);
  
  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
  }
  else {
    XMLNode xNode = xMainNode.getChildNode("status");
    GetInt(xNode, "timezone", m_iTimezone);
  }
}

void Dvb::RemoveNullChars(CStdString &String)
{
  /* favourites.xml and timers.xml sometimes have null chars that screw the xml */
  for (unsigned int i = 0; i<String.size()-1; i++)
  {
    if (String.data()[i] == '\0')
    {
      String.erase(i, 1);
      i--;
    }
  }
}

PVR_ERROR Dvb::GetChannelGroups(ADDON_HANDLE handle)
{
	std::deque<DvbChannelGroup>::iterator iter; m_groups.begin();
	for(iter = m_groups.begin(); iter != m_groups.end(); iter++)
	{
		PVR_CHANNEL_GROUP tag;
		memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

		tag.bIsRadio     = false;
		strncpy(tag.strGroupName, (*iter).strGroupName.c_str(), sizeof(tag.strGroupName));

		PVR->TransferChannelGroup(handle, &tag);
	}

	return PVR_ERROR_NO_ERROR;
}


unsigned int Dvb::GetNumChannelGroups() {
  return m_iNumChannelGroups;
}

PVR_ERROR Dvb::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
	XBMC->Log(LOG_DEBUG, "%s - group '%s'", __FUNCTION__, group.strGroupName);
	CStdString strTmp = group.strGroupName;

	std::deque<DvbChannel>::iterator iter;
	for (iter = m_channels.begin(); iter != m_channels.end(); iter++)
	{
		if (!strTmp.compare((*iter).strGroupName)) 
		{
			PVR_CHANNEL_GROUP_MEMBER tag;
			memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

			strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
			tag.iChannelUniqueId = (*iter).iUniqueId;
			tag.iChannelNumber   = (*iter).iChannelNumber;

			XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
				__FUNCTION__, (*iter).strChannelName.c_str(), tag.iChannelUniqueId, group.strGroupName, (*iter).iChannelNumber);

			PVR->TransferChannelGroupMember(handle, &tag);
		}
	}
	return PVR_ERROR_NO_ERROR;
}

int Dvb::GetCurrentClientChannel(void) 
{
  return m_iCurrentChannel;
}

const char* Dvb::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  SwitchChannel(channelinfo);

  return m_channels.at(channelinfo.iUniqueId-1).strStreamURL.c_str();
}

bool Dvb::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_INFO, "%s channel '%u'", __FUNCTION__, channelinfo.iUniqueId);

  if ((int)channelinfo.iUniqueId == m_iCurrentChannel)
    return true;

  return SwitchChannel(channelinfo);
}

void Dvb::CloseLiveStream(void) 
{
  m_iCurrentChannel = -1;
}

bool Dvb::SwitchChannel(const PVR_CHANNEL &channel)
{
  m_iCurrentChannel = channel.iUniqueId;
  m_bUpdateEPG = true;
  return true;
}

bool Dvb::GetDeviceInfo()
{
	CStdString url; 
	url.Format("%s%s", m_strURL.c_str(), "api/version.html"); 

	CStdString strXML;
	strXML = GetHttpXML(url);

	XMLResults xe;
	XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

	if(xe.error != 0)
	{
		XBMC->Log(LOG_ERROR, "%s Can't connect to the Recording Service", __FUNCTION__);
		XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30500));
		Sleep(10000);
		return false;
	}

	XMLNode xNode = xMainNode.getChildNode("version");

	CStdString strTmp;

	XBMC->Log(LOG_NOTICE, "%s - DeviceInfo", __FUNCTION__);

	// Get Version
	if (!xNode.getText()) {
		XBMC->Log(LOG_ERROR, "%s Could not parse version from result!", __FUNCTION__);
		return false;
	}
	m_strDVBViewerVersion = xNode.getText();
	CStdString strVersion = xNode.getAttribute("iver");

	//Using the integer version number is only allowed from 1.25.0.0 and higher.
	if (strVersion.length()==0 || atoi(strVersion) < RS_MIN_VERSION)
	{
		XBMC->Log(LOG_ERROR, "%s - Recording Service version %s or higher required", __FUNCTION__, cRS_MIN_VERSION);
		XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30501), cRS_MIN_VERSION);
		Sleep(10000);
		return false;
	}
	XBMC->Log(LOG_NOTICE, "%s - Version: %s", __FUNCTION__, m_strDVBViewerVersion.c_str());

	return true;
}

PVR_ERROR Dvb::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  CStdString strXML, url;
  url.Format("%s%s",  m_strURL.c_str(), "status.html?aktion=status"); 
  strXML = GetHttpXML(url);
  unsigned int iSignalStartPos, iSignalEndPos;
  iSignalEndPos = strXML.find("%</th>");

  unsigned int iAdapterStartPos, iAdapterEndPos;
  iAdapterStartPos = strXML.rfind("3\">", iSignalEndPos) + 3;
  iAdapterEndPos = strXML.find("<", iAdapterStartPos);
  strncpy(signalStatus.strAdapterName, strXML.substr(iAdapterStartPos, iAdapterEndPos - iAdapterStartPos).c_str(), sizeof(signalStatus.strAdapterName));

  iSignalStartPos = strXML.find_last_of(">", iSignalEndPos) + 1;
  if (iSignalEndPos < strXML.size())
    signalStatus.iSignal = (int)(atoi(strXML.substr(iSignalStartPos, iSignalEndPos - iSignalStartPos).c_str()) * 655.35);
  strncpy(signalStatus.strAdapterStatus, "OK", sizeof(signalStatus.strAdapterStatus));

  return PVR_ERROR_NO_ERROR;
}
