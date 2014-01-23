#include "VuData.h"
#include "client.h" 
#include <iostream> 
#include <fstream> 
#include "tinyxml/XMLUtils.h"


using namespace ADDON;
using namespace PLATFORM;

bool CCurlFile::Get(const std::string &strURL, std::string &strResult)
{
  void* fileHandle = XBMC->OpenFile(strURL.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (XBMC->ReadFileString(fileHandle, buffer, 1024))
      strResult.append(buffer);
    XBMC->CloseFile(fileHandle);
    return true;
  }
  return false;
}

std::string& Vu::Escape(std::string &s, std::string from, std::string to)
{ 
  std::string::size_type pos = -1;
  while ( (pos = s.find(from, pos+1) ) != std::string::npos)         
    s.erase(pos, from.length()).insert(pos, to);        

  return s;     
} 

bool Vu::LoadLocations() 
{
  CStdString url;
  if (g_bOnlyCurrentLocation)
    url.Format("%s%s",  m_strURL.c_str(), "web/getcurrlocation"); 
  else 
    url.Format("%s%s",  m_strURL.c_str(), "web/getlocations"); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  int iNumLocations = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2locations").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2locations> element");
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2location> element");
    return false;
  }

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2location"))
  {
    CStdString strTmp;
    strTmp = pNode->GetText();

    m_locations.push_back(strTmp);
    iNumLocations++;

    XBMC->Log(LOG_DEBUG, "%s Added '%s' as a recording location", __FUNCTION__, strTmp.c_str());
  }

  XBMC->Log(LOG_INFO, "%s Loded '%d' recording locations", __FUNCTION__, iNumLocations);

  return true;
}

void Vu::TimerUpdates()
{
  std::vector<VuTimer> newtimer = LoadTimers();

  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    m_timers[i].iUpdateState = VU_UPDATE_STATE_NONE;
  }

  unsigned int iUpdated=0;
  unsigned int iUnchanged=0; 

  for (unsigned int j=0;j<newtimer.size(); j++) 
  {
    for (unsigned int i=0; i<m_timers.size(); i++) 
    {
      if (m_timers[i].like(newtimer[j]))
      {
        if(m_timers[i] == newtimer[j])
        {
          m_timers[i].iUpdateState = VU_UPDATE_STATE_FOUND;
          newtimer[j].iUpdateState = VU_UPDATE_STATE_FOUND;
          iUnchanged++;
        }
        else
        {
          newtimer[j].iUpdateState = VU_UPDATE_STATE_UPDATED;
          m_timers[i].iUpdateState = VU_UPDATE_STATE_UPDATED;
          m_timers[i].strTitle = newtimer[j].strTitle;
          m_timers[i].strPlot = newtimer[j].strPlot;
          m_timers[i].iChannelId = newtimer[j].iChannelId;
          m_timers[i].startTime = newtimer[j].startTime;
          m_timers[i].endTime = newtimer[j].endTime;
          m_timers[i].bRepeating = newtimer[j].bRepeating;
          m_timers[i].iWeekdays = newtimer[j].iWeekdays;
          m_timers[i].iEpgID = newtimer[j].iEpgID;

          iUpdated++;
        }
      }
    }
  }

  unsigned int iRemoved = 0;

  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    if (m_timers.at(i).iUpdateState == VU_UPDATE_STATE_NONE)
    {
      XBMC->Log(LOG_INFO, "%s Removed timer: '%s', ClientIndex: '%d'", __FUNCTION__, m_timers.at(i).strTitle.c_str(), m_timers.at(i).iClientIndex);
      m_timers.erase(m_timers.begin()+i);
      i=0;
      iRemoved++;
    }
  }
  unsigned int iNew=0;

  for (unsigned int i=0; i<newtimer.size();i++)
  { 
    if(newtimer.at(i).iUpdateState == VU_UPDATE_STATE_NEW)
    {  
      VuTimer &timer = newtimer.at(i);
      timer.iClientIndex = m_iClientIndexCounter;
      XBMC->Log(LOG_INFO, "%s New timer: '%s', ClientIndex: '%d'", __FUNCTION__, timer.strTitle.c_str(), m_iClientIndexCounter);
      m_timers.push_back(timer);
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

Vu::Vu() 
{
  m_bIsConnected = false;
  m_strServerName = "Vu";
  CStdString strURL = "";

  // simply add user@pass in front of the URL if username/password is set
  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
    strURL.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  
  if (!g_bUseSecureHTTP)
    strURL.Format("http://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
  else
    strURL.Format("https://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
  
  m_strURL = strURL.c_str();

  m_iNumRecordings = 0;
  m_iNumChannelGroups = 0;
  m_iCurrentChannel = -1;
  m_iClientIndexCounter = 1;

  m_bUpdating = false;
  m_iUpdateTimer = 0;
  m_tsBuffer = NULL;
  m_bInitialEPG = true;

  std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
  m_writeHandle = XBMC->OpenFileForWrite(initialEPGReady.c_str(), true);
  XBMC->WriteFile(m_writeHandle, "Y", 1);
  XBMC->CloseFile(m_writeHandle);
  
}

bool Vu::Open()
{
  CLockObject lock(m_mutex);

  XBMC->Log(LOG_NOTICE, "%s - VU+ Addon Configuration options", __FUNCTION__);
  XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
  XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, g_iPortWeb);
  XBMC->Log(LOG_NOTICE, "%s - StreamPort: '%d'", __FUNCTION__, g_iPortStream);
  if (!g_bUseSecureHTTP)
    XBMC->Log(LOG_NOTICE, "%s Use HTTPS: 'false'", __FUNCTION__);
  else
    XBMC->Log(LOG_NOTICE, "%s Use HTTPS: 'true'", __FUNCTION__);
  
  m_bIsConnected = true; // GetDeviceInfo();

  if (!m_bIsConnected)
  {
    XBMC->Log(LOG_ERROR, "%s It seem's that the webinterface cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __FUNCTION__);
    return false;
  }

  LoadLocations();

  if (m_channels.size() == 0) 
  {
    // Load the TV channels - close connection if no channels are found
    if (!LoadChannelGroups())
      return false;

    if (!LoadChannels())
      return false;

  }
  TimerUpdates();

  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread(); 
  
  return IsRunning(); 
}

void  *Vu::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  // Wait for the initial EPG update to complete 
  bool bwait = true;
  int cycles = 0;

  while (bwait)
  {
    if (cycles == 30)
      bwait = false;

    cycles++;
    std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
    m_readHandle = XBMC->OpenFile(initialEPGReady.c_str(), 0);
    byte buf[1];
    XBMC->ReadFile(m_readHandle, buf, 1);
    XBMC->CloseFile(m_readHandle);
    char buf2[] = { "N" };
    if (buf[0] == buf2[0])
    {
      XBMC->Log(LOG_DEBUG, "%s - Intial EPG update COMPLETE!", __FUNCTION__);
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "%s - Intial EPG update not completed yet.", __FUNCTION__);
      Sleep(5 * 1000);
    }
  }

  // Trigger "Real" EPG updates 
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    XBMC->Log(LOG_DEBUG, "%s - Trigger EPG update for channel '%d'", __FUNCTION__, iChannelPtr);
    PVR->TriggerEpgUpdate(m_channels.at(iChannelPtr).iUniqueId);
  }

  while(!IsStopped())
  {
    Sleep(5 * 1000);
    m_iUpdateTimer += 5;

    if ((int)m_iUpdateTimer > (g_iUpdateInterval * 60)) 
    {
      m_iUpdateTimer = 0;
 
      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      XBMC->Log(LOG_INFO, "%s Perform Updates!", __FUNCTION__);

      if (g_bAutomaticTimerlistCleanup) 
      {
        CStdString strTmp;
        strTmp.Format("web/timercleanup?cleanup=true");
        CStdString strResult;
        if(!SendSimpleCommand(strTmp, strResult))
          XBMC->Log(LOG_ERROR, "%s - AutomaticTimerlistCleanup failed!", __FUNCTION__);
      }
      TimerUpdates();
      PVR->TriggerRecordingUpdate();
    }

  }

  //CLockObject lock(m_mutex);
  m_started.Broadcast();

  return NULL;
}

bool Vu::LoadChannels() 
{    
  bool bOk = false;

  m_channels.clear();
  // Load Channels
  for (int i = 0;i<m_iNumChannelGroups;  i++) 
  {
    VuChannelGroup &myGroup = m_groups.at(i);
    if (LoadChannels(myGroup.strServiceReference, myGroup.strGroupName))
      bOk = true;
  }

  // Load the radio channels - continue if no channels are found 
  CStdString strTmp;
  strTmp.Format("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.favourites.radio\" ORDER BY bouquet");
  LoadChannels(strTmp, "radio");

  return bOk;
}

bool Vu::LoadChannelGroups() 
{
  CStdString strTmp; 

  strTmp.Format("%sweb/getservices", m_strURL.c_str());

  CStdString strXML = GetHttpXML(strTmp);  

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2servicelist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2service> element", __FUNCTION__);
    return false;
  }

  m_groups.clear();
  m_iNumChannelGroups = 0;

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2service"))
  {
    CStdString strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      continue;
    
    // Check whether the current element is not just a label
    if (strTmp.compare(0,5,"1:64:") == 0)
      continue;

    VuChannelGroup newGroup;
    newGroup.strServiceReference = strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicename", strTmp)) 
      continue;

    newGroup.strGroupName = strTmp;

    if (g_bOnlyOneGroup && g_strOneGroup.compare(strTmp.c_str())) {
        XBMC->Log(LOG_INFO, "%s Only one group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, strTmp.c_str(), g_strOneGroup.c_str());
        continue;
    }
 
    m_groups.push_back(newGroup);

    XBMC->Log(LOG_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newGroup.strGroupName.c_str());
    m_iNumChannelGroups++; 
  }

  XBMC->Log(LOG_INFO, "%s Loaded %d Channelsgroups", __FUNCTION__, m_iNumChannelGroups);
  return true;
}

bool Vu::LoadChannels(CStdString strServiceReference, CStdString strGroupName) 
{
  XBMC->Log(LOG_INFO, "%s loading channel group: '%s'", __FUNCTION__, strGroupName.c_str());

  CStdString strTmp;
  strTmp.Format("%sweb/getservices?sRef=%s", m_strURL.c_str(), URLEncodeInline(strServiceReference.c_str()));

  CStdString strXML = GetHttpXML(strTmp);  
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2servicelist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2service> element");
    return false;
  }
  
  bool bRadio;

  bRadio = !strGroupName.compare("radio");

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2service"))
  {
    CStdString strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      continue;
    
    // Check whether the current element is not just a label
    if (strTmp.compare(0,5,"1:64:") == 0)
      continue;

    VuChannel newChannel;
    newChannel.bRadio = bRadio;
    newChannel.bInitialEPG = true;
    newChannel.strGroupName = strGroupName;
    newChannel.iUniqueId = m_channels.size()+1;
    newChannel.iChannelNumber = m_channels.size()+1;
    newChannel.strServiceReference = strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicename", strTmp)) 
      continue;

    newChannel.strChannelName = strTmp;
 
    std::string strIcon;
    strIcon = newChannel.strServiceReference.c_str();

    int j = 0;
    std::string::iterator it = strIcon.begin();

    while (j<10 && it != strIcon.end())
    {
      if (*it == ':')
        j++;

      it++;
    }
    std::string::size_type index = it-strIcon.begin();

    strIcon = strIcon.substr(0,index);

    it = strIcon.end() - 1;
    if (*it == ':')
    {
      strIcon.erase(it);
    }
    
    CStdString strTmp2;

    strTmp2.Format("%s", strIcon.c_str());

    std::replace(strIcon.begin(), strIcon.end(), ':', '_');
    strIcon = g_strIconPath.c_str() + strIcon + ".png";

    newChannel.strIconPath = strIcon;

    strTmp.Format("");

    if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
      strTmp.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
    
    if (!g_bUseSecureHTTP)
      strTmp.Format("http://%s%s:%d/%s", strTmp.c_str(), g_strHostname, g_iPortStream, strTmp2.c_str());
    else
      strTmp.Format("https://%s%s:%d/%s", strTmp.c_str(), g_strHostname, g_iPortStream, strTmp2.c_str());
    
    newChannel.strStreamURL = strTmp;

    if (g_bOnlinePicons == true)
    {
      std::replace(strTmp2.begin(), strTmp2.end(), ':', '_');
      strTmp.Format("%spicon/%s.png", m_strURL.c_str(), strTmp2.c_str());
      newChannel.strIconPath = strTmp;
    }

    m_channels.push_back(newChannel);
    XBMC->Log(LOG_INFO, "%s Loaded channel: %s, Icon: %s", __FUNCTION__, newChannel.strChannelName.c_str(), newChannel.strIconPath.c_str());
  }

  XBMC->Log(LOG_INFO, "%s Loaded %d Channels", __FUNCTION__, m_channels.size());
  return true;
}

bool Vu::IsConnected() 
{
  return m_bIsConnected;
}

CStdString Vu::GetHttpXML(CStdString& url) 
{
//  CLockObject lock(m_mutex);

  XBMC->Log(LOG_INFO, "%s Open webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  CStdString strTmp;

  CCurlFile http;
  if(!http.Get(url, strTmp))
  {
    XBMC->Log(LOG_DEBUG, "%s - Could not open webAPI.", __FUNCTION__);
    return "";
  }

  XBMC->Log(LOG_INFO, "%s Got result. Length: %u", __FUNCTION__, strTmp.length());
  

  return strTmp;
}

const char * Vu::GetServerName() 
{
  return m_strServerName.c_str();  
}

int Vu::GetChannelsAmount()
{
  return m_channels.size();
}

int Vu::GetTimersAmount()
{
  return m_timers.size();
}

unsigned int Vu::GetRecordingsAmount() {
  return m_iNumRecordings;
}

PVR_ERROR Vu::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    VuChannel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName));
      strncpy(xbmcChannel.strInputFormat, "", 0); // unused
      xbmcChannel.iEncryptionSystem = 0;
      xbmcChannel.bIsHidden         = false;
      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath));

      if (!g_bUseTimeshift)
      {
        CStdString strStream;
        strStream.Format("pvr://stream/tv/%i.ts", channel.iUniqueId);
        strncpy(xbmcChannel.strStreamURL, strStream.c_str(), sizeof(xbmcChannel.strStreamURL)); 
      }

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

Vu::~Vu() 
{
  CLockObject lock(m_mutex);
  XBMC->Log(LOG_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal channels list...", __FUNCTION__);
  m_channels.clear();  
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal timers list...", __FUNCTION__);
  m_timers.clear();
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal recordings list...", __FUNCTION__);
  m_recordings.clear();
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal group list...", __FUNCTION__);
  m_groups.clear();
  m_bIsConnected = false;
  if (m_tsBuffer)
  {
    XBMC->Log(LOG_DEBUG, "%s Removing internal tsBuffer...", __FUNCTION__);
    SAFE_DELETE(m_tsBuffer);
  }
}

bool Vu::GetInitialEPGForGroup(VuChannelGroup &group)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  CStdString url;
  url.Format("%s%s%s",  m_strURL.c_str(), "web/epgnownext?bRef=",  URLEncodeInline(group.strServiceReference.c_str())); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  int iNumEPG = 0;
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2eventlist").Element();
 
  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2event> element");
    // RETURN "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2event"))
  {
    CStdString strTmp;

    int iTmpStart;
    int iTmp;

    // check and set event starttime and endtimes
    if (!XMLUtils::GetInt(pNode, "e2eventstart", iTmpStart)) 
      continue;

    if (!XMLUtils::GetInt(pNode, "e2eventduration", iTmp))
      continue;

    VuEPGEntry entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpStart + iTmp;

    if (!XMLUtils::GetInt(pNode, "e2eventid", entry.iEventId))  
      continue;

    
    if(!XMLUtils::GetString(pNode, "e2eventtitle", strTmp))
      continue;

    entry.strTitle = strTmp;

    if(!XMLUtils::GetString(pNode, "e2eventservicereference", strTmp))
      continue;

    entry.strServiceReference = strTmp;
    
    entry.iChannelId = GetChannelNumber(entry.strServiceReference.c_str());

    if (XMLUtils::GetString(pNode, "e2eventdescriptionextended", strTmp))
      entry.strPlot = strTmp;

    if (XMLUtils::GetString(pNode, "e2eventdescription", strTmp))
       entry.strPlotOutline = strTmp;

    iNumEPG++; 
    
    group.initialEPG.push_back(entry);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u EPG Entries for group '%s'", __FUNCTION__, iNumEPG, group.strGroupName.c_str());
  return true;
}

PVR_ERROR Vu::GetInitialEPGForChannel(ADDON_HANDLE handle, const VuChannel &channel, time_t iStart, time_t iEnd)
{
  if (m_iNumChannelGroups < 1)
    return PVR_ERROR_SERVER_ERROR;

  XBMC->Log(LOG_DEBUG, "%s Fetch information for group '%s'", __FUNCTION__, channel.strGroupName.c_str());

  VuChannelGroup &myGroup = m_groups.at(0);
  for (int i = 0;i<m_iNumChannelGroups;  i++) 
  {
    myGroup = m_groups.at(i);
    if (!myGroup.strGroupName.compare(channel.strGroupName))
      if (myGroup.initialEPG.size() == 0)
      {
        GetInitialEPGForGroup(myGroup);
        break;
      }
  }

  XBMC->Log(LOG_DEBUG, "%s initialEPG size is now '%d'", __FUNCTION__, myGroup.initialEPG.size());
  
  for (unsigned int i = 0;i<myGroup.initialEPG.size();  i++) 
  {
    VuEPGEntry &entry = myGroup.initialEPG.at(i);
    if (!channel.strServiceReference.compare(entry.strServiceReference)) 
    {
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
    }
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  if (channel.iUniqueId-1 > m_channels.size())
  {
    XBMC->Log(LOG_ERROR, "%s Could not fetch cannel object - not fetching EPG for channel with UniqueID '%d'", __FUNCTION__, channel.iUniqueId);
    return PVR_ERROR_NO_ERROR;
  }

  VuChannel myChannel;
  myChannel = m_channels.at(channel.iUniqueId-1);

  // Check if the initial short import has already been done for this channel
  if (m_channels.at(channel.iUniqueId-1).bInitialEPG == true)
  {
    m_channels.at(channel.iUniqueId-1).bInitialEPG = false;
  
    // Check if all channels have completed the initial EPG import
    m_bInitialEPG = false;
    for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
    {
      if (m_channels.at(iChannelPtr).bInitialEPG == true) 
      {
        m_bInitialEPG = true;
      }
    }

    if (!m_bInitialEPG)
    {
      std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
      m_writeHandle = XBMC->OpenFileForWrite(initialEPGReady.c_str(), true);
      XBMC->WriteFile(m_writeHandle, "N", 1);
      XBMC->CloseFile(m_writeHandle);
    }
    return GetInitialEPGForChannel(handle, myChannel, iStart, iEnd);
  }

  CStdString url;
  url.Format("%s%s%s",  m_strURL.c_str(), "web/epgservice?sRef=",  URLEncodeInline(myChannel.strServiceReference.c_str())); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  int iNumEPG = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2eventlist").Element();
 
  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return PVR_ERROR_NO_ERROR;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2event> element");
    // RETURN "NO_ERROR" as the EPG could be empty for this channel
    return PVR_ERROR_SERVER_ERROR;
  }
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2event"))
  {
    CStdString strTmp;

    int iTmpStart;
    int iTmp;

    // check and set event starttime and endtimes
    if (!XMLUtils::GetInt(pNode, "e2eventstart", iTmpStart)) 
      continue;

    // Skip unneccessary events
    if (iStart > iTmpStart)
      continue;
 
    if (!XMLUtils::GetInt(pNode, "e2eventduration", iTmp))
      continue;

    if ((iEnd > 1) && (iEnd < (iTmpStart + iTmp)))
       continue;
    
    VuEPGEntry entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpStart + iTmp;

    if (!XMLUtils::GetInt(pNode, "e2eventid", entry.iEventId))  
      continue;

    entry.iChannelId = channel.iUniqueId;
    
    if(!XMLUtils::GetString(pNode, "e2eventtitle", strTmp))
      continue;

    entry.strTitle = strTmp;
    
    entry.strServiceReference = myChannel.strServiceReference.c_str();

    if (XMLUtils::GetString(pNode, "e2eventdescriptionextended", strTmp))
      entry.strPlot = strTmp;

    if (XMLUtils::GetString(pNode, "e2eventdescription", strTmp))
       entry.strPlotOutline = strTmp;

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

    XBMC->Log(LOG_DEBUG, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.iChannelId, entry.startTime, entry.endTime);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

int Vu::GetChannelNumber(CStdString strServiceReference)  
{
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    if (!strServiceReference.compare(m_channels[i].strServiceReference))
      return i+1;
  }
  return -1;
}

CStdString Vu::GetChannelIconPath(CStdString strChannelName)  
{
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    if (!strChannelName.compare(m_channels[i].strChannelName))
      return m_channels[i].strIconPath;
  }
  return "";
}

PVR_ERROR Vu::GetTimers(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  XBMC->Log(LOG_INFO, "%s - timers available '%d'", __FUNCTION__, m_timers.size());
  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    VuTimer &timer = m_timers.at(i);
    XBMC->Log(LOG_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.iClientIndex);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));
    tag.iClientChannelUid = timer.iChannelId;
    tag.startTime         = timer.startTime;
    tag.endTime           = timer.endTime;
    strncpy(tag.strTitle, timer.strTitle.c_str(), sizeof(tag.strTitle));
    strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
    strncpy(tag.strSummary, timer.strPlot.c_str(), sizeof(tag.strSummary));
    tag.state             = timer.state;
    tag.iPriority         = 0;     // unused
    tag.iLifetime         = 0;     // unused
    tag.bIsRepeating      = timer.bRepeating;
    tag.firstDay          = 0;     // unused
    tag.iWeekdays         = timer.iWeekdays;
    tag.iEpgUid           = timer.iEpgID;
    tag.iMarginStart      = 0;     // unused
    tag.iMarginEnd        = 0;     // unused
    tag.iGenreType        = 0;     // unused
    tag.iGenreSubType     = 0;     // unused
    tag.iClientIndex = timer.iClientIndex;

    PVR->TransferTimerEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

std::vector<VuTimer> Vu::LoadTimers()
{
  std::vector<VuTimer> timers;

  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), "web/timerlist"); 

  CStdString strXML;
  strXML = GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return timers;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2timerlist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2timerlist> element!", __FUNCTION__);
    return timers;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2timer").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2timer> element");
    return timers;
  }
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2timer"))
  {
    CStdString strTmp;

    int iTmp;
    bool bTmp;
    int iDisabled;
    
    if (XMLUtils::GetString(pNode, "e2name", strTmp)) 
      XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());
 
    if (!XMLUtils::GetInt(pNode, "e2state", iTmp)) 
      continue;

    if (!XMLUtils::GetInt(pNode, "e2disabled", iDisabled))
      continue;

    VuTimer timer;
    
    timer.strTitle          = strTmp;

    if (XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      timer.iChannelId = GetChannelNumber(strTmp.c_str());

    if (!XMLUtils::GetInt(pNode, "e2timebegin", iTmp)) 
      continue; 
 
    timer.startTime         = iTmp;
    
    if (!XMLUtils::GetInt(pNode, "e2timeend", iTmp)) 
      continue; 
 
    timer.endTime           = iTmp;
    
    if (XMLUtils::GetString(pNode, "e2description", strTmp))
      timer.strPlot        = strTmp.c_str();
 
    if (XMLUtils::GetInt(pNode, "e2repeated", iTmp))
      timer.iWeekdays         = iTmp;
    else 
      timer.iWeekdays = 0;

    if (timer.iWeekdays != 0)
      timer.bRepeating      = true; 
    else
      timer.bRepeating = false;
    
    if (XMLUtils::GetInt(pNode, "e2eit", iTmp))
      timer.iEpgID = iTmp;
    else 
      timer.iEpgID = 0;

    timer.state = PVR_TIMER_STATE_NEW;

    if (!XMLUtils::GetInt(pNode, "e2state", iTmp))
      continue;

    XBMC->Log(LOG_DEBUG, "%s e2state is: %d ", __FUNCTION__, iTmp);
  
    if (iTmp == 0) 
    {
      timer.state = PVR_TIMER_STATE_SCHEDULED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: SCHEDULED", __FUNCTION__);
    }
    
    if (iTmp == 2) 
    {
      timer.state = PVR_TIMER_STATE_RECORDING;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: RECORDING", __FUNCTION__);
    }
    
    if (iTmp == 3 && iDisabled == 0) 
    {
      timer.state = PVR_TIMER_STATE_COMPLETED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: COMPLETED", __FUNCTION__);
    }

    if (XMLUtils::GetBoolean(pNode, "e2cancled", bTmp)) 
    {
      if (bTmp)  
      {
        timer.state = PVR_TIMER_STATE_ABORTED;
        XBMC->Log(LOG_DEBUG, "%s Timer state is: ABORTED", __FUNCTION__);
      }
    }

    if (iDisabled == 1) 
    {
      timer.state = PVR_TIMER_STATE_CANCELLED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: Cancelled", __FUNCTION__);
    }

    if (timer.state == PVR_TIMER_STATE_NEW)
      XBMC->Log(LOG_DEBUG, "%s Timer state is: NEW", __FUNCTION__);

    timers.push_back(timer);

    XBMC->Log(LOG_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }

  XBMC->Log(LOG_INFO, "%s fetched %u Timer Entries", __FUNCTION__, timers.size());
  return timers; 
}

bool Vu::SendSimpleCommand(const CStdString& strCommandURL, CStdString& strResultText, bool bIgnoreResult)
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), strCommandURL.c_str()); 

  CStdString strXML;
  strXML = GetHttpXML(url);
  
  if (!bIgnoreResult)
  {
    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return false;
    }

    TiXmlHandle hDoc(&xmlDoc);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);

    pElem = hDoc.FirstChildElement("e2simplexmlresult").Element();

    if (!pElem)
    {
      XBMC->Log(LOG_DEBUG, "%s Could not find <e2simplexmlresult> element!", __FUNCTION__);
      return false;
    }

    bool bTmp;

    if (!XMLUtils::GetBoolean(pElem, "e2state", bTmp)) 
    {
      XBMC->Log(LOG_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      strResultText.Format("Could not parse e2state!");
      return false;
    }

    if (!XMLUtils::GetString(pElem, "e2statetext", strResultText)) 
    {
      XBMC->Log(LOG_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      return false;
    }

    if (!bTmp)
      XBMC->Log(LOG_ERROR, "%s Error message from backend: '%s'", __FUNCTION__, strResultText.c_str());

    return bTmp;
  }
  return true;
}


PVR_ERROR Vu::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  CStdString strTmp;
  CStdString strServiceReference = m_channels.at(timer.iClientChannelUid-1).strServiceReference.c_str();

  if (!g_strRecordingPath.compare(""))
    strTmp.Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d&dirname=&s", URLEncodeInline(strServiceReference), timer.iWeekdays, timer.startTime, timer.endTime, URLEncodeInline(timer.strTitle), URLEncodeInline(timer.strSummary),timer.iEpgUid, URLEncodeInline(g_strRecordingPath));
  else
    strTmp.Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d", URLEncodeInline(strServiceReference), timer.iWeekdays, timer.startTime, timer.endTime, URLEncodeInline(timer.strTitle), URLEncodeInline(timer.strSummary),timer.iEpgUid);

  CStdString strResult;
  if(!SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::DeleteTimer(const PVR_TIMER &timer) 
{
  CStdString strTmp;
  CStdString strServiceReference = m_channels.at(timer.iClientChannelUid-1).strServiceReference.c_str();

  strTmp.Format("web/timerdelete?sRef=%s&begin=%d&end=%d", URLEncodeInline(strServiceReference.c_str()), timer.startTime, timer.endTime);

  CStdString strResult;
  if(!SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::GetRecordings(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  m_iNumRecordings = 0;
  m_recordings.clear();

  for (unsigned int i=0; i<m_locations.size(); i++)
  {
    if (!GetRecordingFromLocation(m_locations[i]))
    {
      XBMC->Log(LOG_ERROR, "%s Error fetching lists for folder: '%s'", __FUNCTION__, m_locations[i].c_str());
    }
  }

  TransferRecordings(handle);

  return PVR_ERROR_NO_ERROR;
}

bool Vu::IsInRecordingFolder(CStdString strRecordingFolder)
{
  int iMatches = 0;
  for (unsigned int i = 0; i < m_recordings.size(); i++)
  {
    if (strRecordingFolder.compare(m_recordings.at(i).strTitle) == 0)
    {
      iMatches++;
      XBMC->Log(LOG_DEBUG, "%s Found Recording title '%s' in recordings vector!", __FUNCTION__, strRecordingFolder.c_str());
      if (iMatches > 1)
      {
        XBMC->Log(LOG_DEBUG, "%s Found Recording title twice '%s' in recordings vector!", __FUNCTION__, strRecordingFolder.c_str());
        return true;    
      }
    }
  }

  return false;
}

void Vu::TransferRecordings(ADDON_HANDLE handle)
{
  for (unsigned int i=0; i<m_recordings.size(); i++)
  {
    CStdString strTmp;
    VuRecording &recording = m_recordings.at(i);
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    strncpy(tag.strRecordingId, recording.strRecordingId.c_str(), sizeof(tag.strRecordingId));
    strncpy(tag.strTitle, recording.strTitle.c_str(), sizeof(tag.strTitle));
    strncpy(tag.strStreamURL, recording.strStreamURL.c_str(), sizeof(tag.strStreamURL));
    strncpy(tag.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(tag.strPlotOutline));
    strncpy(tag.strPlot, recording.strPlot.c_str(), sizeof(tag.strPlot));
    strncpy(tag.strChannelName, recording.strChannelName.c_str(), sizeof(tag.strChannelName));
    strncpy(tag.strIconPath, recording.strIconPath.c_str(), sizeof(tag.strIconPath));

    if(IsInRecordingFolder(recording.strTitle))
      strTmp.Format("/%s/", recording.strTitle.c_str());
    else
      strTmp.Format("/");

    recording.strDirectory = strTmp;
    strncpy(tag.strDirectory, recording.strDirectory.c_str(), sizeof(tag.strDirectory));
    tag.recordingTime     = recording.startTime;
    tag.iDuration         = recording.iDuration;

    PVR->TransferRecordingEntry(handle, &tag);
  }
}

bool Vu::GetRecordingFromLocation(CStdString strRecordingFolder)
{
  CStdString url;

  if (!strRecordingFolder.compare("default"))
    url.Format("%s%s", m_strURL.c_str(), "web/movielist"); 
  else 
    url.Format("%s%s?dirname=%s", m_strURL.c_str(), "web/movielist", URLEncodeInline(strRecordingFolder.c_str())); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2movielist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2movie> element");
    return false;
  }
  
  int iNumRecording = 0; 
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2movie"))
  {
    CStdString strTmp;
    int iTmp;

    VuRecording recording;

    recording.iLastPlayedPosition = 0;
    if (XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      recording.strRecordingId = strTmp;

    if (XMLUtils::GetString(pNode, "e2title", strTmp))
      recording.strTitle = strTmp;
    
    if (XMLUtils::GetString(pNode, "e2description", strTmp))
      recording.strPlotOutline = strTmp;

    if (XMLUtils::GetString(pNode, "e2descriptionextended", strTmp))
      recording.strPlot = strTmp;
    
    if (XMLUtils::GetString(pNode, "e2servicename", strTmp))
      recording.strChannelName = strTmp;

    recording.strIconPath = GetChannelIconPath(strTmp.c_str());

    if (XMLUtils::GetInt(pNode, "e2time", iTmp)) 
      recording.startTime = iTmp;

    if (XMLUtils::GetString(pNode, "e2length", strTmp)) 
    {
      iTmp = TimeStringToSeconds(strTmp.c_str());
      recording.iDuration = iTmp;
    }
    else
      recording.iDuration = 0;

    if (XMLUtils::GetString(pNode, "e2filename", strTmp)) 
    {
      strTmp.Format("%sfile?file=%s", m_strURL.c_str(), URLEncodeInline(strTmp.c_str()));
      recording.strStreamURL = strTmp;
    }

    m_iNumRecordings++; 
    iNumRecording++;

    m_recordings.push_back(recording);

    XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, recording.strTitle.c_str(), recording.startTime, recording.iDuration);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u Recording Entries from folder '%s'", __FUNCTION__, iNumRecording, strRecordingFolder.c_str());

  return true;
}

PVR_ERROR Vu::DeleteRecording(const PVR_RECORDING &recinfo) 
{
  CStdString strTmp;

  strTmp.Format("web/moviedelete?sRef=%s", URLEncodeInline(recinfo.strRecordingId));

  CStdString strResult;
  if(!SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_FAILED;

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::UpdateTimer(const PVR_TIMER &timer)
{

  XBMC->Log(LOG_DEBUG, "%s timer channelid '%d'", __FUNCTION__, timer.iClientChannelUid);

  CStdString strTmp;
  CStdString strServiceReference = m_channels.at(timer.iClientChannelUid-1).strServiceReference.c_str();  

  unsigned int i=0;

  while (i<m_timers.size())
  {
    if (m_timers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }

  VuTimer &oldTimer = m_timers.at(i);
  CStdString strOldServiceReference = m_channels.at(oldTimer.iChannelId-1).strServiceReference.c_str();  
  XBMC->Log(LOG_DEBUG, "%s old timer channelid '%d'", __FUNCTION__, oldTimer.iChannelId);

  int iDisabled = 0;
  if (timer.state == PVR_TIMER_STATE_CANCELLED)
    iDisabled = 1;

  strTmp.Format("web/timerchange?sRef=%s&begin=%d&end=%d&name=%s&eventID=&description=%s&tags=&afterevent=3&eit=0&disabled=%d&justplay=0&repeated=%d&channelOld=%s&beginOld=%d&endOld=%d&deleteOldOnSave=1", URLEncodeInline(strServiceReference.c_str()), timer.startTime, timer.endTime, URLEncodeInline(timer.strTitle), URLEncodeInline(timer.strSummary), iDisabled, timer.iWeekdays, URLEncodeInline(strOldServiceReference.c_str()), oldTimer.startTime, oldTimer.endTime  );
  
  CStdString strResult;
  if(!SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

long Vu::TimeStringToSeconds(const CStdString &timeString)
{
  CStdStringArray secs;
  SplitString(timeString, ":", secs);
  int timeInSecs = 0;
  for (unsigned int i = 0; i < secs.size(); i++)
  {
    timeInSecs *= 60;
    timeInSecs += atoi(secs[i]);
  }
  return timeInSecs;
}

int Vu::SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings)
{
  int iPos = -1;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  results.clear();
  std::vector<unsigned int> positions;

  newPos = input.Find (delimiter, 0);

  if ( newPos < 0 )
  {
    results.push_back(input);
    return 1;
  }

  while ( newPos > iPos )
  {
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.Find (delimiter, iPos + sizeS2);
  }

  // numFound is the number of delimeters which is one less
  // than the number of substrings
  unsigned int numFound = positions.size();
  if (iMaxStrings > 0 && numFound >= iMaxStrings)
    numFound = iMaxStrings - 1;

  for ( unsigned int i = 0; i <= numFound; i++ )
  {
    CStdString s;
    if ( i == 0 )
    {
      if ( i == numFound )
        s = input;
      else
        s = input.Mid( i, positions[i] );
    }
    else
    {
      int offset = positions[i - 1] + sizeS2;
      if ( offset < isize )
      {
        if ( i == numFound )
          s = input.Mid(offset);
        else if ( i > 0 )
          s = input.Mid( positions[i - 1] + sizeS2,
                         positions[i] - positions[i - 1] - sizeS2 );
      }
    }
    results.push_back(s);
  }
  // return the number of substrings
  return results.size();
}

PVR_ERROR Vu::GetChannelGroups(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  for(unsigned int iTagPtr = 0; iTagPtr < m_groups.size(); iTagPtr++)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

    tag.bIsRadio     = false;
    strncpy(tag.strGroupName, m_groups[iTagPtr].strGroupName.c_str(), sizeof(tag.strGroupName));

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

unsigned int Vu::GetNumChannelGroups() 
{
  return m_iNumChannelGroups;
}

CStdString Vu::GetGroupServiceReference(CStdString strGroupName)  
{
  for (int i = 0;i<m_iNumChannelGroups;  i++) 
  {
    VuChannelGroup &myGroup = m_groups.at(i);
    if (!strGroupName.compare(myGroup.strGroupName))
      return myGroup.strServiceReference;
  }
  return "error";
}

PVR_ERROR Vu::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  XBMC->Log(LOG_DEBUG, "%s - group '%s'", __FUNCTION__, group.strGroupName);
  CStdString strTmp = group.strGroupName;
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    VuChannel &myChannel = m_channels.at(i);
    if (!strTmp.compare(myChannel.strGroupName)) 
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
      tag.iChannelUniqueId = myChannel.iUniqueId;
      tag.iChannelNumber   = myChannel.iChannelNumber;

      XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
          __FUNCTION__, myChannel.strChannelName.c_str(), tag.iChannelUniqueId, group.strGroupName, myChannel.iChannelNumber);

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

int Vu::GetCurrentClientChannel(void) 
{
  return m_iCurrentChannel;
}

const char* Vu::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  SwitchChannel(channelinfo);

  return m_channels.at(channelinfo.iUniqueId-1).strStreamURL.c_str();
}

bool Vu::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_INFO, "%s channel '%u'", __FUNCTION__, channelinfo.iUniqueId);

  if ((int)channelinfo.iUniqueId == m_iCurrentChannel)
    return true;

  if (!g_bUseTimeshift)
    return SwitchChannel(channelinfo);

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);

  XBMC->Log(LOG_INFO, "%s ts buffer starts url=%s", __FUNCTION__, GetLiveStreamURL(channelinfo));
  m_tsBuffer = new TimeshiftBuffer(GetLiveStreamURL(channelinfo), g_strTimeshiftBufferPath);
  return m_tsBuffer->IsValid();
}

void Vu::CloseLiveStream(void) 
{
  m_iCurrentChannel = -1;
  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
}

int Vu::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->ReadData(pBuffer, iBufferSize);
}

long long Vu::SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->Seek(iPosition, iWhence);
}

long long Vu::PositionLiveStream(void)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->Position();
}

long long Vu::LengthLiveStream(void)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->Length();
}

bool Vu::SwitchChannel(const PVR_CHANNEL &channel)
{
  XBMC->Log(LOG_DEBUG, "%s Switching channels", __FUNCTION__);

  if ((int)channel.iUniqueId == m_iCurrentChannel)
    return true;

  m_iCurrentChannel = (int)channel.iUniqueId;

  if (g_bZap)
  {
    // Zapping is set to true, so send the zapping command to the PVR box 
    CStdString strServiceReference = m_channels.at(channel.iUniqueId-1).strServiceReference.c_str();

    CStdString strTmp;
    strTmp.Format("web/zap?sRef=%s", URLEncodeInline(strServiceReference));

    CStdString strResult;
    if(!SendSimpleCommand(strTmp, strResult))
      return false;
  
    if (!g_bUseTimeshift)
      return true;
  }

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);

  XBMC->Log(LOG_INFO, "%s ts buffer starts url=%s", __FUNCTION__, GetLiveStreamURL(channel));
  m_tsBuffer = new TimeshiftBuffer(GetLiveStreamURL(channel), g_strTimeshiftBufferPath);
  return m_tsBuffer->IsValid();
}

void Vu::SendPowerstate()
{
  if (!g_bSetPowerstate)
    return;
  
  CLockObject lock(m_mutex);
  CStdString strTmp;
  strTmp.Format("web/powerstate?newstate=1");

  CStdString strResult;
  SendSimpleCommand(strTmp, strResult, true); 
}

bool Vu::GetDeviceInfo()
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), "web/deviceinfo"); 

  CStdString strXML;
  strXML = GetHttpXML(url);
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_ERROR, "%s Could not find <e2deviceinfo> element!", __FUNCTION__);
    return false;
  }

  CStdString strTmp;;

  XBMC->Log(LOG_NOTICE, "%s - DeviceInfo", __FUNCTION__);

  // Get EnigmaVersion
  if (!XMLUtils::GetString(pElem, "e2enigmaversion", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2enigmaversion from result!", __FUNCTION__);
    return false;
  }
  m_strEnigmaVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2EnigmaVersion: %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  // Get ImageVersion
  if (!XMLUtils::GetString(pElem, "e2imageversion", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2imageversion from result!", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2ImageVersion: %s", __FUNCTION__, m_strImageVersion.c_str());

  // Get WebIfVersion
  if (!XMLUtils::GetString(pElem, "e2webifversion", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2webifversion from result!", __FUNCTION__);
    return false;
  }
  m_strWebIfVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2WebIfVersion: %s", __FUNCTION__, m_strWebIfVersion.c_str());

  // Get DeviceName
  if (!XMLUtils::GetString(pElem, "e2devicename", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2devicename from result!", __FUNCTION__);
    return false;
  }
  m_strServerName = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2DeviceName: %s", __FUNCTION__, m_strServerName.c_str());

  return true;
}

const char SAFE[256] =
{
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};


CStdString Vu::URLEncodeInline(const CStdString& sSrc) 
{
  const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
  const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
  const int SRC_LEN = sSrc.length();
  unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
  unsigned char * pEnd = pStart;
  const unsigned char * const SRC_END = pSrc + SRC_LEN;

  for (; pSrc < SRC_END; ++pSrc)
  {
    if (SAFE[*pSrc])
      *pEnd++ = *pSrc;
    else
    {
      // escape this char
      *pEnd++ = '%';
      *pEnd++ = DEC2HEX[*pSrc >> 4];
      *pEnd++ = DEC2HEX[*pSrc & 0x0F];
    }
  }

  std::string sResult((char *)pStart, (char *)pEnd);
  delete [] pStart;
  return sResult;
}

int Vu::GetRecordingIndex(CStdString strStreamURL)  
{
  for (unsigned int i = 0;i<m_recordings.size();  i++) 
  {
    if (!strStreamURL.compare(m_recordings[i].strStreamURL))
      return i;
  }
  return -1;
}

