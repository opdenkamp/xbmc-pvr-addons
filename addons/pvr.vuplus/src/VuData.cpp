#include "VuData.h"

#include <curl/curl.h>
#include "client.h" 
#include <iostream> 
#include <fstream> 

using namespace ADDON;
using namespace PLATFORM;

std::string& Vu::Escape(std::string &s, std::string from, std::string to)
{ 
  int pos = -1;
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

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return false;
  }

  XMLNode xNode = xMainNode.getChildNode("e2locations");
  int n = xNode.nChildNode("e2location");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("e2location", i);

    CStdString strTmp;

    strTmp = xTmp.getText();

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

  for (unsigned int j=0;j<newtimer.size(); j++) {
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

bool Vu::CheckForChannelUpdate() 
{
  if (!g_bCheckForChannelUpdates)
    return false;

  std::vector<VuChannel> oldchannels = m_channels;

  LoadChannels();

  for(unsigned int i=0; i< oldchannels.size(); i++)
    oldchannels[i].iChannelState = VU_UPDATE_STATE_NONE;

  for (unsigned int j=0; j<m_channels.size(); j++)
  {
    for (unsigned int i=0; i<oldchannels.size(); i++)
    {
      if (!oldchannels[i].strServiceReference.compare(m_channels[j].strServiceReference))
      {
        if(oldchannels[i] == m_channels[j])
        {
          m_channels[j].iChannelState = VU_UPDATE_STATE_FOUND;
          oldchannels[i].iChannelState = VU_UPDATE_STATE_FOUND;
        }
        else
        {
          oldchannels[i].iChannelState = VU_UPDATE_STATE_UPDATED;
          m_channels[j].iChannelState = VU_UPDATE_STATE_UPDATED;
        }
      }
    }
  }
  
  int iNewChannels = 0; 
  for (unsigned int i=0; i<m_channels.size(); i++) 
  {
    if (m_channels[i].iChannelState == VU_UPDATE_STATE_NEW)
      iNewChannels++;
  }

  int iRemovedChannels = 0;
  int iNotUpdatedChannels = 0;
  int iUpdatedChannels = 0;
  for (unsigned int i=0; i<oldchannels.size(); i++) 
  {
    if(oldchannels[i].iChannelState == VU_UPDATE_STATE_NONE)
      iRemovedChannels++;
    
    if(oldchannels[i].iChannelState == VU_UPDATE_STATE_FOUND)
      iNotUpdatedChannels++;  

    if(oldchannels[i].iChannelState == VU_UPDATE_STATE_UPDATED)
      iUpdatedChannels++;
  }

  XBMC->Log(LOG_INFO, "%s No of channels: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemovedChannels, iNotUpdatedChannels, iUpdatedChannels, iNewChannels); 

  if ((iRemovedChannels > 0) || (iUpdatedChannels > 0) || (iNewChannels > 0))
  {
    //Channels have been changed, so return "true"
    return true;
  }
  else 
  {
    m_channels = oldchannels;
    return false;
  }
}

bool Vu::CheckForGroupUpdate() {
  if (!g_bCheckForGroupUpdates)
    return false;

  std::vector<VuChannelGroup> m_oldgroups = m_groups;

  m_groups.clear();
  LoadChannelGroups();

  for (unsigned int i=0; i<m_oldgroups.size(); i++)
    m_oldgroups[i].iGroupState = VU_UPDATE_STATE_NONE;

  // Now compare the old group with the new one
  for (unsigned int j=0; j<m_groups.size(); j++) 
  {
    for(unsigned int i=0;i<m_oldgroups.size(); i++) 
    {
      // we find the same service reference for the just fetched
      // groups in the oldgroups, therefore this is either an name 
      // update or just a persisting group
      if (!m_oldgroups[i].strServiceReference.compare(m_groups[j].strServiceReference)) 
      {
        if (m_oldgroups[i] == m_groups[j]) 
        {
          m_groups[j].iGroupState = VU_UPDATE_STATE_FOUND;
          m_oldgroups[i].iGroupState = VU_UPDATE_STATE_FOUND;
        }
        else 
        {
          m_oldgroups[i].iGroupState = VU_UPDATE_STATE_UPDATED;
          m_groups[j].iGroupState = VU_UPDATE_STATE_UPDATED;
        }
      }
    }
  }

  int iNewGroups = 0; 
  for (unsigned int i=0; i<m_groups.size(); i++) 
  {
    if (m_groups[i].iGroupState == VU_UPDATE_STATE_NEW)
      iNewGroups++;
  }

  int iRemovedGroups = 0;
  int iNotUpdatedGroups = 0;
  int iUpdatedGroups = 0;
  for (unsigned int i=0; i<m_oldgroups.size(); i++) 
  {
    if(m_oldgroups[i].iGroupState == VU_UPDATE_STATE_NONE)
      iRemovedGroups++;
    
    if(m_oldgroups[i].iGroupState == VU_UPDATE_STATE_FOUND)
      iNotUpdatedGroups++;  

    if(m_oldgroups[i].iGroupState == VU_UPDATE_STATE_UPDATED)
      iUpdatedGroups++;
  }

  XBMC->Log(LOG_INFO, "%s No of groups: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemovedGroups, iNotUpdatedGroups, iUpdatedGroups, iNewGroups); 

  if ((iRemovedGroups > 0) || (iUpdatedGroups > 0) || (iNewGroups > 0))
  {
    // groups have been changed, so return "true"
    return true;
  }
  else 
  {
    m_groups = m_oldgroups;
    return false;
  }
}

void Vu::LoadChannelData()
{
  XBMC->Log(LOG_DEBUG, "%s Load channel data from file: '%schanneldata.xml'", __FUNCTION__, g_strChannelDataPath.c_str());

  XMLResults pResults;

  CStdString strFileName;
  strFileName.Format("%schanneldata.xml", g_strChannelDataPath.c_str());
  XMLNode xMainNode = XMLNode::parseFile(strFileName.c_str(), "channeldata", &pResults);

  if (pResults.error != 0) {
    XBMC->Log(LOG_ERROR, "%s error parsing channeldata!", __FUNCTION__);
    return;
  }

  int iVersion;
  if (!GetInt(xMainNode, "version", iVersion))
  {
    XBMC->Log(LOG_NOTICE, "%s No channeldata version string found, abort loading data from HDD!", __FUNCTION__);
    return;
  } 

  XBMC->Log(LOG_DEBUG, "%s Found channeldata version: '%d', current channeldata version: '%d'", __FUNCTION__, iVersion, CHANNELDATAVERSION);

  if (iVersion != CHANNELDATAVERSION) {
    XBMC->Log(LOG_NOTICE, "%s The channeldata versions do not match, we will abort loading the data from the HDD.", __FUNCTION__);
    return;
  }
  
  XMLNode xNode = xMainNode.getChildNode("grouplist");
  int n = xNode.nChildNode("group");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("group", i);

    CStdString strTmp;

    VuChannelGroup group; 
    
    if (!GetString(xTmp, "servicereference", strTmp))
      continue;

    group.strServiceReference = strTmp.c_str();
    
    if (!GetString(xTmp, "groupname", strTmp))
      continue;

    group.strGroupName = strTmp.c_str();


    m_groups.push_back(group);

    XBMC->Log(LOG_DEBUG, "%s Loaded group '%s' from HDD", __FUNCTION__, group.strGroupName.c_str());
  }

  xNode = xMainNode.getChildNode("channellist");
  n = xNode.nChildNode("channel");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("channel", i);

    CStdString strTmp;
    bool bTmp;
    int iTmp;

    VuChannel channel; 
    
    if (GetBoolean(xTmp, "radio", bTmp)) {
      channel.bRadio = bTmp;
    }

    if (!GetInt(xTmp, "id", iTmp))
      continue;
    channel.iUniqueId = iTmp;

    if (!GetInt(xTmp, "channelnumber", iTmp))
      continue;
    channel.iChannelNumber = iTmp;

    if (!GetString(xTmp, "groupname", strTmp))
      continue;
    channel.strGroupName = strTmp.c_str();

    if (!GetString(xTmp, "channelname", strTmp))
      continue;
    channel.strChannelName = strTmp.c_str();

    if (!GetString(xTmp, "servicereference", strTmp))
      continue;

    channel.strServiceReference = strTmp.c_str();
     
    if (!GetString(xTmp, "streamurl", strTmp))
      continue;
    channel.strStreamURL = strTmp.c_str();

    if (!GetString(xTmp, "iconpath", strTmp))
      continue;
    channel.strIconPath = strTmp.c_str();

    m_channels.push_back(channel);

    XBMC->Log(LOG_DEBUG, "%s Loaded channel '%s' from HDD", __FUNCTION__, channel.strChannelName.c_str());
  }

}

void Vu::StoreChannelData()
{
  XBMC->Log(LOG_DEBUG, "%s Store channel data into file: '%schanneldata.xml'", __FUNCTION__, g_strChannelDataPath.c_str());

  std::ofstream stream;
  
  CStdString strFileName;
  strFileName.Format("%schanneldata.xml", g_strChannelDataPath.c_str());
  stream.open(strFileName.c_str());

  if(stream.fail())
    XBMC->Log(LOG_ERROR, "%s Could not open channeldata file for writing!", __FUNCTION__);


  stream << "<channeldata>\n";
  stream << "\t<version>\n" << CHANNELDATAVERSION;
  stream << "\t</version>\n";
  stream << "\t<grouplist>\n";
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    VuChannelGroup &group = m_groups.at(iGroupPtr);
    stream << "\t\t<group>\n";

    CStdString strTmp = group.strServiceReference;
    Escape(strTmp, "&", "&amp;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<servicereference>" << strTmp;
    stream << "</servicereference>\n";
    
    strTmp = group.strGroupName;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<groupname>" << strTmp;
    stream << "</groupname>\n";
    stream << "\t\t</group>\n";
  }

  stream << "\t</grouplist>\n";
    
  stream << "\t<channellist>\n";
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    stream << "\t\t<channel>\n";
    VuChannel &channel = m_channels.at(iChannelPtr);

    // store channel properties
    stream << "\t\t\t<radio>";
    if (channel.bRadio)
      stream << "true";
    else
      stream << "false";
    stream << "</radio>\n";

    stream << "\t\t\t<id>" << channel.iUniqueId;
    stream << "</id>\n";
    stream << "\t\t\t<channelnumber>" << channel.iChannelNumber;
    stream << "</channelnumber>\n";
    
    CStdString strTmp = channel.strGroupName;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<groupname>" << strTmp;
    stream << "</groupname>\n";
    
    strTmp = channel.strChannelName;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<channelname>" << strTmp;
    stream << "</channelname>\n";

    strTmp = channel.strServiceReference;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<servicereference>" << strTmp;
    stream << "</servicereference>\n";

    strTmp =  channel.strStreamURL;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<streamurl>" << strTmp;
    stream << "</streamurl>\n";

    strTmp = channel.strIconPath;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");


    stream << "\t\t\t<iconpath>" << strTmp;
    stream << "</iconpath>\n";
 
    stream << "\t\t</channel>\n";

  }
  stream << "\t</channellist>\n";
  stream << "</channeldata>\n";
  stream.close();
}

Vu::Vu() 
{
  m_bIsConnected = false;
  m_strServerName = "Vu";
  CStdString strURL = "";

  // simply add user@pass in front of the URL if username/password is set
  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
    strURL.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  strURL.Format("http://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
  m_strURL = strURL.c_str();
  m_iNumRecordings = 0;
  m_iNumChannelGroups = 0;
  m_iCurrentChannel = -1;
  m_iClientIndexCounter = 1;
  m_bInitial = false;

  m_iUpdateTimer = 0;
}

// Curl callback
int Vu::VuWebResponseCallback(void *contents, int iLength, int iSize, void *memPtr)
{
  int iRealSize = iSize * iLength;
  struct VuWebResponse *resp = (struct VuWebResponse*) memPtr;

  resp->response = (char*) realloc(resp->response, resp->iSize + iRealSize + 1);

  if (resp->response == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s Could not allocate memeory!", __FUNCTION__);
    return 0;
  }

  memcpy(&(resp->response[resp->iSize]), contents, iRealSize);
  resp->iSize += iRealSize;
  resp->response[resp->iSize] = 0;

  return iRealSize;
}

bool Vu::Open()
{
  CLockObject lock(m_mutex);

  XBMC->Log(LOG_NOTICE, "%s - VU+ Addon Configuration options", __FUNCTION__);
  XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
  XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, g_iPortWeb);
  XBMC->Log(LOG_NOTICE, "%s - StreamPort: '%d'", __FUNCTION__, g_iPortStream);
  
  m_bIsConnected = GetDeviceInfo();

  LoadLocations();

  LoadChannelData();
  if (m_channels.size() == 0) {
    XBMC->Log(LOG_DEBUG, "%s No stored channels found, fetch from webapi", __FUNCTION__);
    // Load the TV channels - close connection if no channels are found
    if (!LoadChannelGroups())
      return false;

    if (!LoadChannels())
      return false;

    m_bInitial = true;
    StoreChannelData();
  }
  TimerUpdates();

  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread(); 
  
  return IsRunning(); 
}

void  *Vu::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  while(!IsStopped())
  {

    Sleep(5 * 1000);
    m_iUpdateTimer += 5;

    if (((int)m_iUpdateTimer > (g_iUpdateInterval * 60)) || (m_bInitial == false))
    {
      m_iUpdateTimer = 0;
 
      if (!m_bInitial)
      {
        // Load the TV channels - close connection if no channels are found
        bool bTriggerGroupsUpdate = CheckForGroupUpdate();
        bool bTriggerChannelsUpdate = CheckForChannelUpdate();

        m_bInitial = true;

        if (bTriggerGroupsUpdate) 
        {
          PVR->TriggerChannelGroupsUpdate();
          bTriggerChannelsUpdate = true;
        }

        if (bTriggerChannelsUpdate) 
        {
          PVR->TriggerChannelUpdate();
          // Store the channel data on HDD
          StoreChannelData();
        }
      }
    
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

  CLockObject lock(m_mutex);
  m_started.Broadcast();
  //XBMC->Log(LOG_DEBUG, "%s - exiting", __FUNCTION__);

  return NULL;
}


bool Vu::LoadChannels() 
{
    m_channels.clear();
    // Load Channels
    for (int i = 0;i<m_iNumChannelGroups;  i++) 
    {
      VuChannelGroup &myGroup = m_groups.at(i);
      if (!LoadChannels(myGroup.strServiceReference, myGroup.strGroupName))
      {
        return false;
      }
    }

    // Load the radio channels - continue if no channels are found 
    CStdString strTmp;
    strTmp.Format("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.favourites.radio\" ORDER BY bouquet");
    if (!LoadChannels(strTmp, "radio"))
      return false;

    return true;
}

bool Vu::LoadChannelGroups() 
{
  CStdString strTmp; 

  strTmp.Format("%sweb/getservices", m_strURL.c_str());

  CStdString strXML = GetHttpXML(strTmp);  

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);  

  if(xe.error != 0)  {    
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));    
    return false;  
  }  

  m_groups.clear();
  m_iNumChannelGroups = 0;

  XMLNode xNode = xMainNode.getChildNode("e2servicelist");
  int n = xNode.nChildNode("e2service");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("e2service", i);

    CStdString strTmp;
    
    if (!GetString(xTmp, "e2servicereference", strTmp))
      continue;
    
    // Check whether the current element is not just a label
    if (strTmp.compare(0,5,"1:64:") == 0)
      continue;

    VuChannelGroup newGroup;
    newGroup.strServiceReference = strTmp;

    if (!GetString(xTmp, "e2servicename", strTmp)) 
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

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);  

  if(xe.error != 0)  {    
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));    
    return false;  
  }  

  XMLNode xNode = xMainNode.getChildNode("e2servicelist");
  int n = xNode.nChildNode("e2service");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);
  bool bRadio;

  bRadio = !strGroupName.compare("radio");

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("e2service", i);
    CStdString strTmp;
    
    if (!GetString(xTmp, "e2servicereference", strTmp))
      continue;
    
    // Check whether the current element is not just a label
    if (strTmp.compare(0,5,"1:64:") == 0)
      continue;

    VuChannel newChannel;
    newChannel.bRadio = bRadio;
    newChannel.strGroupName = strGroupName;
    newChannel.iUniqueId = m_channels.size()+1;
    newChannel.iChannelNumber = m_channels.size()+1;
    newChannel.strServiceReference = strTmp;

    if (!GetString(xTmp, "e2servicename", strTmp)) 
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

    std::replace(strIcon.begin(), strIcon.end(), ':','_');
    strIcon = g_strIconPath.c_str() + strIcon + ".png";

    newChannel.strIconPath = strIcon;

    strTmp.Format("");

    if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
      strTmp.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());

    strTmp.Format("http://%s%s:%d/%s", strTmp.c_str(), g_strHostname, g_iPortStream, strTmp2.c_str());
    newChannel.strStreamURL = strTmp;

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
  CLockObject lock(m_mutex);
  CURL* curl_handle;

  XBMC->Log(LOG_INFO, "%s Open webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  struct VuWebResponse response;

  response.response = (char*) malloc(1);
  response.iSize = 0;

  // retrieve the webpage and store it in memory
  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &VuWebResponseCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "vuplus-pvraddon-agent/1.0");
  curl_easy_perform(curl_handle);

  if (response.iSize == 0)
  {
    XBMC->Log(LOG_INFO, "%s Could not open webAPI", __FUNCTION__);
    return "";
  }

  CStdString strTmp;
  strTmp.Format("%s", response.response);

  XBMC->Log(LOG_INFO, "%s Got result. Length: %u", __FUNCTION__, strTmp.length());
  
  free(response.response);
  curl_easy_cleanup(curl_handle);

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
      strncpy(xbmcChannel.strInputFormat, "", sizeof(xbmcChannel.strInputFormat)); // unused

      CStdString strStream;
      strStream.Format("pvr://stream/tv/%i.ts", channel.iUniqueId);
      strncpy(xbmcChannel.strStreamURL, strStream.c_str(), sizeof(xbmcChannel.strStreamURL)); //channel.strStreamURL.c_str();
      xbmcChannel.iEncryptionSystem = 0;
      
      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath));
      xbmcChannel.bIsHidden         = false;

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

Vu::~Vu() 
{
  StopThread();
  
  m_channels.clear();  
  m_timers.clear();
  m_recordings.clear();
  m_groups.clear();
  m_bIsConnected = false;
}

PVR_ERROR Vu::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  VuChannel &myChannel = m_channels.at(channel.iUniqueId-1);

  CStdString url;
  url.Format("%s%s%s",  m_strURL.c_str(), "web/epgservice?sRef=",  URLEncodeInline(myChannel.strServiceReference)); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  int iNumEPG = 0;

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return PVR_ERROR_SERVER_ERROR;
  }

  XMLNode xNode = xMainNode.getChildNode("e2eventlist");
  int n = xNode.nChildNode("e2event");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("e2event", i);

    CStdString strTmp;
    int iTmpStart;
    int iTmp;

    // check and set event starttime and endtimes
    if (!GetInt(xTmp, "e2eventstart", iTmpStart)) 
      continue;
 
    if (!GetInt(xTmp, "e2eventduration", iTmp))
      continue;

    if ((iEnd > 1) && (iEnd < (iTmpStart + iTmp)))
       continue;
    
    VuEPGEntry entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpStart + iTmp;

    if (!GetInt(xTmp, "e2eventid", entry.iEventId))  
      continue;

    entry.iChannelId = channel.iUniqueId;
    
    if(!GetString(xTmp, "e2eventtitle", strTmp))
      continue;

    entry.strTitle = strTmp;
    
    entry.strServiceReference = myChannel.strServiceReference;

    if (GetString(xTmp, "e2eventdescriptionextended", strTmp))
      entry.strPlot = strTmp;

    if (GetString(xTmp, "e2eventdescription", strTmp))
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

    XBMC->Log(LOG_INFO, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.iChannelId, entry.startTime, entry.endTime);
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

PVR_ERROR Vu::GetTimers(ADDON_HANDLE handle)
{

  XBMC->Log(LOG_INFO, "%s - timers available '%d'", __FUNCTION__, m_timers.size());
  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    VuTimer &timer = m_timers.at(i);
	XBMC->Log(LOG_INFO, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.iClientIndex);
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
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), "web/timerlist"); 

  CStdString strXML;
  strXML = GetHttpXML(url);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);
  
  std::vector<VuTimer> timers;

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return timers;
  }

  XMLNode xNode = xMainNode.getChildNode("e2timerlist");
  int n = xNode.nChildNode("e2timer");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);
  

  while(n>0)
  {
    int i = n-1;
    n--;
    XMLNode xTmp = xNode.getChildNode("e2timer", i);

    CStdString strTmp;
    int iTmp;
    bool bTmp;
    int iDisabled;
    
    if (GetString(xTmp, "e2name", strTmp)) 
      XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());
 
    if (!GetInt(xTmp, "e2state", iTmp)) 
      continue;

    if (!GetInt(xTmp, "e2disabled", iDisabled))
      continue;

    VuTimer timer;
    
    timer.strTitle          = strTmp;

    if (GetString(xTmp, "e2servicereference", strTmp))
      timer.iChannelId = GetChannelNumber(strTmp.c_str());

    if (!GetInt(xTmp, "e2timebegin", iTmp)) 
      continue; 
 
    timer.startTime         = iTmp;
    
    if (!GetInt(xTmp, "e2timeend", iTmp)) 
      continue; 
 
    timer.endTime           = iTmp;
    
    if (GetString(xTmp, "e2description", strTmp))
      timer.strPlot        = strTmp.c_str();
 
    if (GetInt(xTmp, "e2repeated", iTmp))
      timer.iWeekdays         = iTmp;
    else 
      timer.iWeekdays = 0;

    if (timer.iWeekdays != 0)
      timer.bRepeating      = true; 
    else
      timer.bRepeating = false;
    
    if (GetInt(xTmp, "e2eit", iTmp))
      timer.iEpgID = iTmp;
    else 
      timer.iEpgID = 0;

    timer.state = PVR_TIMER_STATE_NEW;

    if (!GetInt(xTmp, "e2state", iTmp))
      continue;

    XBMC->Log(LOG_DEBUG, "%s e2state is: %d ", __FUNCTION__, iTmp);
  
    if (iTmp == 0) {
      timer.state = PVR_TIMER_STATE_SCHEDULED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: SCHEDULED", __FUNCTION__);
    }
    
    if (iTmp == 2) {
      timer.state = PVR_TIMER_STATE_RECORDING;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: RECORDING", __FUNCTION__);
    }
    
    if (iTmp == 3 && iDisabled == 0) {
      timer.state = PVR_TIMER_STATE_COMPLETED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: COMPLETED", __FUNCTION__);
    }

    if (GetBoolean(xTmp, "e2cancled", bTmp)) {
      if (bTmp)  {
        timer.state = PVR_TIMER_STATE_ABORTED;
        XBMC->Log(LOG_DEBUG, "%s Timer state is: ABORTED", __FUNCTION__);
      }
    }

	if (iDisabled == 1) {
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

CStdString Vu::URLEncodeInline(const CStdString& strData)
{
  CStdString buffer = strData;
  CURL* handle = curl_easy_init();
  char* encodedURL = curl_easy_escape(handle, strData.c_str(), strlen(strData.c_str()));

  buffer.Format("%s", encodedURL);
  curl_free(encodedURL);
  curl_easy_cleanup(handle);

  return buffer;
}

bool Vu::SendSimpleCommand(const CStdString& strCommandURL, CStdString& strResultText, bool bIgnoreResult)
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), strCommandURL.c_str()); 

  CStdString strXML;
  strXML = GetHttpXML(url);

  if (!bIgnoreResult)
  {
    XMLResults xe;
    XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

    if(xe.error != 0)  {
      XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
      return false;
    }

    XMLNode xNode = xMainNode.getChildNode("e2simplexmlresult");

    bool bTmp;

    if (!GetBoolean(xNode, "e2state", bTmp)) {
      XBMC->Log(LOG_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      strResultText.Format("Could not parse e2state!");
      return false;
    }

    if (!GetString(xNode, "e2statetext", strResultText)) {
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

  // check if we got a event id
  //if (timer.iEpgUid > 0) 
  //  strTmp.Format("web/timeraddbyeventid?sRef=%s&eventid=%d", strServiceReference, timer.iEpgUid);
  //else
  if (!g_strRecordingPath.compare(""))
    strTmp.Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d&dirname=&s", strServiceReference, timer.iWeekdays, timer.startTime, timer.endTime, URLEncodeInline(timer.strTitle), URLEncodeInline(timer.strSummary),timer.iEpgUid, URLEncodeInline(g_strRecordingPath));
  else
    strTmp.Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d", strServiceReference, timer.iWeekdays, timer.startTime, timer.endTime, URLEncodeInline(timer.strTitle), URLEncodeInline(timer.strSummary),timer.iEpgUid);

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

  strTmp.Format("web/timerdelete?sRef=%s&begin=%d&end=%d", URLEncodeInline(strServiceReference), timer.startTime, timer.endTime);

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
  m_iNumRecordings = 0;
  m_recordings.clear();

  for (unsigned int i=0; i<m_locations.size(); i++)
  {
    if (!GetRecordingFromLocation(handle, m_locations[i]))
    {
      XBMC->Log(LOG_ERROR, "%s Error fetching lists for folder: '%s'", __FUNCTION__, m_locations[i].c_str());
      return PVR_ERROR_SERVER_ERROR;
    }
  }

  return PVR_ERROR_NO_ERROR;
}

bool Vu::GetRecordingFromLocation(ADDON_HANDLE handle, CStdString strRecordingFolder)
{
  CStdString url;

  if (!strRecordingFolder.compare("default"))
    url.Format("%s%s", m_strURL.c_str(), "web/movielist"); 
  else 
    url.Format("%s%s?dirname=%s", m_strURL.c_str(), "web/movielist", URLEncodeInline(strRecordingFolder.c_str())); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return false;
  }

  XMLNode xNode = xMainNode.getChildNode("e2movielist");
  int n = xNode.nChildNode("e2movie");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);
 
  int iNumRecording = 0; 

  while(n>0)
  {
    int i = n-1;
    n--;
    XMLNode xTmp = xNode.getChildNode("e2movie", i);
    CStdString strTmp;
    int iTmp;

    VuRecording recording;
    if (GetString(xTmp, "e2servicereference", strTmp))
      recording.strRecordingId = strTmp;

    if (GetString(xTmp, "e2title", strTmp))
      recording.strTitle = strTmp;
    
    if (GetString(xTmp, "e2description", strTmp))
      recording.strPlotOutline = strTmp;

    if (GetString(xTmp, "e2descriptionextended", strTmp))
      recording.strPlot = strTmp;
    
    if (GetString(xTmp, "e2servicename", strTmp))
      recording.strChannelName = strTmp;

    if (GetInt(xTmp, "e2time", iTmp)) 
      recording.startTime = iTmp;

    if (GetString(xTmp, "e2length", strTmp)) {
      iTmp = TimeStringToSeconds(strTmp.c_str());
      recording.iDuration = iTmp;
    }
    else
      recording.iDuration = 0;

    if (GetString(xTmp, "e2filename", strTmp)) {
      strTmp.Format("%sfile?file=%s", m_strURL.c_str(), URLEncodeInline(strTmp.c_str()));
      recording.strStreamURL = strTmp;
    }
    
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    strncpy(tag.strRecordingId, recording.strRecordingId.c_str(), sizeof(tag.strRecordingId));
    strncpy(tag.strTitle, recording.strTitle.c_str(), sizeof(tag.strTitle));
    strncpy(tag.strStreamURL, recording.strStreamURL.c_str(), sizeof(tag.strStreamURL));
    strncpy(tag.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(tag.strPlotOutline));
    strncpy(tag.strPlot, recording.strPlot.c_str(), sizeof(tag.strPlot));
    strncpy(tag.strChannelName, recording.strChannelName.c_str(), sizeof(tag.strChannelName));
    tag.recordingTime     = recording.startTime;
    tag.iDuration         = recording.iDuration;
    strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused

    PVR->TransferRecordingEntry(handle, &tag);

    m_iNumRecordings++; 
    iNumRecording++;
    m_recordings.push_back(recording);

    XBMC->Log(LOG_INFO, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, tag.strTitle, recording.startTime, recording.iDuration);
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

bool Vu::GetInt(XMLNode xRootNode, const char* strTag, int& iIntValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (xNode.isEmpty())
     return false;
  iIntValue = atoi(xNode.getText());
  return true;
}

bool Vu::GetBoolean(XMLNode xRootNode, const char* strTag, bool& bBoolValue)
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

bool Vu::GetString(XMLNode xRootNode, const char* strTag, CStdString& strStringValue)
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


unsigned int Vu::GetNumChannelGroups() {
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

  return SwitchChannel(channelinfo);
}

void Vu::CloseLiveStream(void) 
{
  m_iCurrentChannel = -1;
}

bool Vu::SwitchChannel(const PVR_CHANNEL &channel)
{
  if ((int)channel.iUniqueId == m_iCurrentChannel)
    return true;

  if (!g_bZap)
    return true;

  // Zapping is set to true, so send the zapping command to the PVR box 
  CStdString strServiceReference = m_channels.at(channel.iUniqueId-1).strServiceReference.c_str();

  CStdString strTmp;
  strTmp.Format("web/zap?sRef=%s", URLEncodeInline(strServiceReference));

  CStdString strResult;
  if(!SendSimpleCommand(strTmp, strResult))
    return false;

  return true;
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

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);
  
  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return false;
  }

  XMLNode xNode = xMainNode.getChildNode("e2deviceinfo");

  CStdString strTmp;;

  XBMC->Log(LOG_NOTICE, "%s - DeiveInfo", __FUNCTION__);

  // Get EnigmaVersion
  if (!GetString(xNode, "e2enigmaversion", strTmp)) {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2enigmaversion from result!", __FUNCTION__);
    return false;
  }
  m_strEnigmaVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2EnigmaVersion: %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  // Get ImageVersion
  if (!GetString(xNode, "e2imageversion", strTmp)) {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2imageversion from result!", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2ImageVersion: %s", __FUNCTION__, m_strImageVersion.c_str());

  // Get WebIfVersion
  if (!GetString(xNode, "e2webifversion", strTmp)) {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2webifversion from result!", __FUNCTION__);
    return false;
  }
  m_strWebIfVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2WebIfVersion: %s", __FUNCTION__, m_strWebIfVersion.c_str());

  // Get DeviceName
  if (!GetString(xNode, "e2devicename", strTmp)) {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2devicename from result!", __FUNCTION__);
    return false;
  }
  m_strServerName = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2DeviceName: %s", __FUNCTION__, m_strServerName.c_str());

  return true;
}
