

#include "N7Xml.h"
#include "tinyxml/tinyxml.h"
#include "tinyxml/XMLUtils.h"
#include <curl/curl.h>

using namespace ADDON;

/* partially copied from http://curl.haxx.se/libcurl/c/getinmemory.html */
struct MemoryStruct
{
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory)
  {
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
  }

  return realsize;
}

bool CCurlFile::Get(const std::string &strURL, std::string &strResult, unsigned iTimeoutSeconds /* = 3 */)
{
  CURL *curl_handle;

  struct MemoryStruct chunk;
  chunk.memory = (char*)malloc(1);
  chunk.size   = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  curl_handle = curl_easy_init();
  if (!curl_handle)
    return false;

  curl_easy_setopt(curl_handle, CURLOPT_URL, strURL.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, iTimeoutSeconds);

  curl_easy_perform(curl_handle);
  curl_easy_cleanup(curl_handle);

  bool bReturn(false);
  if(chunk.memory)
  {
    strResult = chunk.memory;
    free(chunk.memory);
    bReturn = true;
  }

  curl_global_cleanup();

  return bReturn;
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
  if(!http.Get(strUrl, strXML, 5))
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

