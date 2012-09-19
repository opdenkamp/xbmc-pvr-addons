

#include "N7Xml.h"
#include "tinyxml/tinyxml.h"
#include "tinyxml/XMLUtils.h"

using namespace ADDON;

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

N7Xml::N7Xml(void) :
        m_connected(false)
{
  list_channels();
}

N7Xml::~N7Xml(void)
{
  m_channels.clear();
}

int N7Xml::getChannelsAmount()
{ 
  return m_channels.size();
}

void N7Xml::list_channels()
{
  CStdString strUrl;
  strUrl.Format("http://%s:%i/n7channel_nt.xml", g_strHostname.c_str(), g_iPort);
  CStdString strXML;

  CCurlFile http;
  if(!http.Get(strUrl, strXML))
  {
    XBMC->Log(LOG_DEBUG, "N7Xml - Could not open connection to N7 backend.");
  }
  else
  {
    TiXmlDocument xml;
    xml.Parse(strXML.c_str());
    TiXmlElement* rootXmlNode = xml.RootElement();
    if (rootXmlNode == NULL)
      return;
    TiXmlElement* channelsNode = rootXmlNode->FirstChildElement("channel");
    if (channelsNode)
    {
      XBMC->Log(LOG_DEBUG, "N7Xml - Connected to N7 backend.");
      m_connected = true;
      int iUniqueChannelId = 0;
      TiXmlNode *pChannelNode = NULL;
      while ((pChannelNode = channelsNode->IterateChildren(pChannelNode)) != NULL)
      {
        CStdString strTmp;
        PVRChannel channel;

        /* unique ID */
        channel.iUniqueId = ++iUniqueChannelId;

        /* channel number */
        if (!XMLUtils::GetInt(pChannelNode, "number", channel.iChannelNumber))
          channel.iChannelNumber = channel.iUniqueId;

        /* channel name */
        if (!XMLUtils::GetString(pChannelNode, "title", strTmp))
          continue;
        channel.strChannelName = strTmp;

        /* icon path */
        const TiXmlElement* pElement = pChannelNode->FirstChildElement("media:thumbnail");
        channel.strIconPath = pElement->Attribute("url");

        /* channel url */
        if (!XMLUtils::GetString(pChannelNode, "guid", strTmp))
          channel.strStreamURL = "";
        else
          channel.strStreamURL = strTmp;

        m_channels.push_back(channel);
      }
    }
  }
}


PVR_ERROR N7Xml::requestChannelList(ADDON_HANDLE handle, bool bRadio)
{
  if (m_connected)
  {
    std::vector<PVRChannel>::const_iterator item;
    PVR_CHANNEL tag;
    for( item = m_channels.begin(); item != m_channels.end(); ++item)
    {
      const PVRChannel& channel = *item;
      memset(&tag, 0 , sizeof(tag));

      tag.iUniqueId       = channel.iUniqueId;
      tag.iChannelNumber  = channel.iChannelNumber;
      strncpy(tag.strChannelName, channel.strChannelName.c_str(), sizeof(tag.strChannelName) - 1);
      strncpy(tag.strStreamURL, channel.strStreamURL.c_str(), sizeof(tag.strStreamURL) - 1);
      strncpy(tag.strIconPath, channel.strIconPath.c_str(), sizeof(tag.strIconPath) - 1);

      XBMC->Log(LOG_DEBUG, "N7Xml - Loaded channel - %s.", tag.strChannelName);
      PVR->TransferChannelEntry(handle, &tag);
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "N7Xml - no channels loaded");
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR N7Xml::requestEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR N7Xml::getSignal(PVR_SIGNAL_STATUS &qualityinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

