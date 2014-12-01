/*
 *      Copyright (C) 2014 Jean-Luc Barriere
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
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "mythwsapi.h"
#include "mythdebug.h"
#include "private/builtin.h"
#include "private/mythsocket.h"
#include "private/mythwsrequest.h"
#include "private/mythwsresponse.h"
#include "private/janssonptr.h"
#include "private/mythjsonparser.h"
#include "private/mythjsonbinder.h"
#include "private/platform/threads/mutex.h"
#include "private/platform/util/util.h"
#include "private/uriparser.h"

#include <jansson.h>

#define BOOLSTR(a)  ((a) ? "true" : "false")

using namespace Myth;

#define WS_ROOT_MYTH          "/Myth"
#define WS_ROOT_CAPTURE       "/Capture"
#define WS_ROOT_CHANNEL       "/Channel"
#define WS_ROOT_GUIDE         "/Guide"
#define WS_ROOT_CONTENT       "/Content"
#define WS_ROOT_DVR           "/Dvr"

WSAPI::WSAPI(const std::string& server, unsigned port, const std::string& securityPin)
: m_mutex(new PLATFORM::CMutex)
, m_server(server)
, m_port(port)
, m_securityPin(securityPin)
, m_checked(false)
, m_version()
, m_serverHostName()
{
  m_checked = InitWSAPI();
}

WSAPI::~WSAPI()
{
  SAFE_DELETE(m_mutex);
}

bool WSAPI::InitWSAPI()
{
  bool status = false;
  // Reset array of version
  memset(m_serviceVersion, 0, sizeof(m_serviceVersion));
  // Check the core service Myth
  WSServiceVersion_t& mythwsv = m_serviceVersion[WS_Myth];
  if (!GetServiceVersion(WS_Myth, mythwsv))
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  if (mythwsv.ranking > MYTH_API_VERSION_MAX_RANKING) {}
  else if (mythwsv.ranking >= 0x00020000)
    status = CheckServerHostName2_0() & CheckVersion2_0();

  if (status)
  {
    GetServiceVersion(WS_Capture, m_serviceVersion[WS_Capture]);
    GetServiceVersion(WS_Channel, m_serviceVersion[WS_Channel]);
    GetServiceVersion(WS_Guide, m_serviceVersion[WS_Guide]);
    GetServiceVersion(WS_Content, m_serviceVersion[WS_Content]);
    GetServiceVersion(WS_Dvr, m_serviceVersion[WS_Dvr]);
    DBG(MYTH_DBG_INFO, "%s: MythTV API service is available: %s:%d(%s) protocol(%d) schema(%d)\n",
            __FUNCTION__, m_serverHostName.c_str(), m_port, m_version.version.c_str(),
            (unsigned)m_version.protocol, (unsigned)m_version.schema);
    return true;
  }
  DBG(MYTH_DBG_ERROR, "%s: MythTV API service is not supported or unavailable: %s:%d (%u.%u)\n",
          __FUNCTION__, m_server.c_str(), m_port, mythwsv.major, mythwsv.minor);
  return false;
}

bool WSAPI::GetServiceVersion(WSServiceId_t id, WSServiceVersion_t& wsv)
{
  static const char * WSServiceRoot[WS_INVALID + 1] =
  {
    WS_ROOT_MYTH,         ///< WS_Myth
    WS_ROOT_CAPTURE,      ///< WS_Capture
    WS_ROOT_CHANNEL,      ///< WS_Channel
    WS_ROOT_GUIDE,        ///< WS_Guide
    WS_ROOT_CONTENT,      ///< WS_Content
    WS_ROOT_DVR,          ///< WS_Dvr
    "/?",                 ///< WS_INVALID
  };
  std::string url(WSServiceRoot[id]);
  url.append("/version");
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService(url);
  WSResponse resp(req);
  if (resp.IsSuccessful())
  {
    // Parse content response
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (root.isValid() && json_is_object(root.get()))
    {
      const json_t *field = json_object_get(root.get(), "String");
      if (field != NULL)
      {
        const char *val = json_string_value(field);
        if (val != NULL)
        {
          if (sscanf(val, "%d.%d", &(wsv.major), &(wsv.minor)) == 2)
          {
            wsv.ranking = ((wsv.major & 0xFFFF) << 16) | (wsv.minor & 0xFFFF);
            return true;
          }
        }
      }
    }
  }
  wsv.major = 0;
  wsv.minor = 0;
  wsv.ranking = 0;
  return false;
}

bool WSAPI::CheckServerHostName2_0()
{
  m_serverHostName.clear();

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetHostName");
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  // Parse content response
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (root.isValid() && json_is_object(root.get()))
  {
    const json_t *field = json_object_get(root.get(), "String");
    if (field != NULL)
    {
      const char *val = json_string_value(field);
      if (val != NULL)
      {
        m_serverHostName = val;
        m_namedCache[val] = m_server;
        return true;
      }
    }
  }
  return false;
}

bool WSAPI::CheckVersion2_0()
{
  m_version.protocol = 0;
  m_version.schema = 0;
  m_version.version.clear();
  WSServiceVersion_t& wsv = m_serviceVersion[WS_Myth];

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetConnectionInfo");
  if (!m_securityPin.empty())
  {
    // Skip if null or empty
    req.SetContentParam("Pin", m_securityPin);
  }
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  // Parse content response
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (root.isValid() && json_is_object(root.get()))
  {
    const json_t *con = json_object_get(root.get(), "ConnectionInfo");
    if (con != NULL)
    {
      const json_t *ver = json_object_get(con, "Version");
      MythJSON::BindObject(ver, &m_version, MythDTO::getVersionBindArray(wsv.ranking));
      if (m_version.protocol)
        return true;
    }
  }
  return false;
}

unsigned WSAPI::CheckService()
{
  PLATFORM::CLockObject lock(*m_mutex);
  if (m_checked || (m_checked = InitWSAPI()))
    return (unsigned)m_version.protocol;
  return 0;
}

WSServiceVersion_t WSAPI::CheckService(WSServiceId_t id)
{
  PLATFORM::CLockObject lock(*m_mutex);
  if (m_checked || (m_checked = InitWSAPI()))
    return m_serviceVersion[id];
  return m_serviceVersion[WS_INVALID];
}

void WSAPI::InvalidateService()
{
  if (m_checked)
    m_checked = false;
}

std::string WSAPI::GetServerHostName()
{
  return m_serverHostName;
}

VersionPtr WSAPI::GetVersion()
{
  return VersionPtr(new Version(m_version));
}

std::string WSAPI::ResolveHostName(const std::string& hostname)
{
  PLATFORM::CLockObject lock(*m_mutex);
  std::map<std::string, std::string>::const_iterator it = m_namedCache.find(hostname);
  if (it != m_namedCache.end())
    return it->second;
  Myth::SettingPtr addr = this->GetSetting("BackendServerIP6", hostname);
  if (addr && !addr->value.empty() && addr->value != "::1")
  {
    std::string& ret = m_namedCache[hostname];
    ret.assign(addr->value);
    DBG(MYTH_DBG_DEBUG, "%s: resolving hostname %s as %s\n", __FUNCTION__, hostname.c_str(), ret.c_str());
    return ret;
  }
  addr = this->GetSetting("BackendServerIP", hostname);
  if (addr && !addr->value.empty())
  {
    std::string& ret = m_namedCache[hostname];
    ret.assign(addr->value);
    DBG(MYTH_DBG_DEBUG, "%s: resolving hostname %s as %s\n", __FUNCTION__, hostname.c_str(), ret.c_str());
    return ret;
  }
  DBG(MYTH_DBG_ERROR, "%s: unknown host (%s)\n", __FUNCTION__, hostname.c_str());
  return std::string();
}

///////////////////////////////////////////////////////////////////////////////
////
////  Service operations
////

SettingPtr WSAPI::GetSetting2_0(const std::string& key, const std::string& hostname)
{
  SettingPtr ret;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetSetting");
  req.SetContentParam("HostName", hostname);
  req.SetContentParam("Key", key);
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  // Object: SettingList
  const json_t *slist = json_object_get(root.get(), "SettingList");
  // Object: Settings
  json_t *sts = json_object_get(slist, "Settings");
  if (json_is_object(sts))
  {
    void *it = json_object_iter(sts);
    if (it != NULL)
    {
      const json_t *val = json_object_iter_value(it);
      if (val != NULL)
      {
        ret.reset(new Setting());  // Using default constructor
        ret->key = json_object_iter_key(it);
        ret->value = json_string_value(val);
      }
    }
  }
  return ret;
}

SettingPtr WSAPI::GetSetting(const std::string& key, bool myhost)
{
  std::string hostname;
  if (myhost)
    hostname = TcpSocket::GetMyHostName();
  return GetSetting(key, hostname);
}

SettingMapPtr WSAPI::GetSettings2_0(const std::string& hostname)
{
  SettingMapPtr ret(new SettingMap);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetSetting");
  req.SetContentParam("HostName", hostname);
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  // Object: SettingList
  const json_t *slist = json_object_get(root.get(), "SettingList");
  // Object: Settings
  json_t *sts = json_object_get(slist, "Settings");
  if (json_is_object(sts))
  {
    void *it = json_object_iter(sts);
    while (it)
    {
      const json_t *val = json_object_iter_value(it);
      if (val != NULL)
      {
        SettingPtr setting(new Setting());  // Using default constructor
        setting->key = json_object_iter_key(it);
        setting->value = json_string_value(val);
        ret->insert(SettingMap::value_type(setting->key, setting));
      }
      it = json_object_iter_next(sts, it);
    }
  }
  return ret;
}

SettingMapPtr WSAPI::GetSettings(bool myhost)
{
  std::string hostname;
  if (myhost)
    hostname = TcpSocket::GetMyHostName();
  return GetSettings(hostname);
}

bool WSAPI::PutSetting2_0(const std::string& key, const std::string& value, bool myhost)
{
  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/PutSetting", HRM_POST);
  std::string hostname;
  if (myhost)
    hostname = TcpSocket::GetMyHostName();
  req.SetContentParam("HostName", hostname);
  req.SetContentParam("Key", key);
  req.SetContentParam("Value", value);
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Capture service
////
CaptureCardListPtr WSAPI::GetCaptureCardList1_4()
{
  CaptureCardListPtr ret(new CaptureCardList);
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindcard = MythDTO::getCaptureCardBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Capture/GetCaptureCardList");
  req.SetContentParam("HostName", m_serverHostName.c_str());
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  // Object: CaptureCardList
  const json_t *clist = json_object_get(root.get(), "CaptureCardList");
  // Object: CaptureCards[]
  const json_t *cards = json_object_get(clist, "CaptureCards");
  // Iterates over the sequence elements.
  for (size_t ci = 0; ci < json_array_size(cards); ++ci)
  {
    const json_t *card = json_array_get(cards, ci);
    CaptureCardPtr captureCard(new CaptureCard());  // Using default constructor
    // Bind the new captureCard
    MythJSON::BindObject(card, captureCard.get(), bindcard);
    ret->push_back(captureCard);
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Channel Service
////
VideoSourceListPtr WSAPI::GetVideoSourceList1_2()
{
  VideoSourceListPtr ret(new VideoSourceList);
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindvsrc = MythDTO::getVideoSourceBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Channel/GetVideoSourceList");
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  // Object: VideoSourceList
  const json_t *slist = json_object_get(root.get(), "VideoSourceList");
  // Object: VideoSources[]
  const json_t *vsrcs = json_object_get(slist, "VideoSources");
  // Iterates over the sequence elements.
  for (size_t vi = 0; vi < json_array_size(vsrcs); ++vi)
  {
    const json_t *vsrc = json_array_get(vsrcs, vi);
    VideoSourcePtr videoSource(new VideoSource());  // Using default constructor
    // Bind the new videoSource
    MythJSON::BindObject(vsrc, videoSource.get(), bindvsrc);
    ret->push_back(videoSource);
  }
  return ret;
}

ChannelListPtr WSAPI::GetChannelList1_2(uint32_t sourceid, bool onlyVisible)
{
  ChannelListPtr ret(new ChannelList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Channel/GetChannelInfoList");

  do
  {
    req.ClearContent();
    uint32str(sourceid, buf);
    req.SetContentParam("SourceID", buf);
    int32str(req_index, buf);
    req.SetContentParam("StartIndex", buf);
    int32str(req_count, buf);
    req.SetContentParam("Count", buf);

    DBG(MYTH_DBG_DEBUG, "%s: request index(%d) count(%d)\n", __FUNCTION__, req_index, req_count);
    WSResponse resp(req);
    if (!resp.IsSuccessful())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (!root.isValid() || !json_is_object(root.get()))
    {
      DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
      break;
    }
    DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

    // Object: ChannelInfoList
    const json_t *clist = json_object_get(root.get(), "ChannelInfoList");
    ItemList list = ItemList(); // Using default constructor
    MythJSON::BindObject(clist, &list, bindlist);
    // List has ProtoVer. Check it or sound alarm
    if (list.protoVer != proto)
    {
      InvalidateService();
      break;
    }
    count = 0;
    // Object: ChannelInfos[]
    const json_t *chans = json_object_get(clist, "ChannelInfos");
    // Iterates over the sequence elements.
    for (size_t ci = 0; ci < json_array_size(chans); ++ci)
    {
      ++count;
      const json_t *chan = json_array_get(chans, ci);
      ChannelPtr channel(new Channel());  // Using default constructor
      // Bind the new channel
      MythJSON::BindObject(chan, channel.get(), bindchan);
      if (channel->chanId && (!onlyVisible || channel->visible))
        ret->push_back(channel);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

ChannelListPtr WSAPI::GetChannelList1_5(uint32_t sourceid, bool onlyVisible)
{
  ChannelListPtr ret(new ChannelList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Channel/GetChannelInfoList");

  do
  {
    req.ClearContent();
    req.SetContentParam("Details", "true");
    req.SetContentParam("OnlyVisible", BOOLSTR(onlyVisible));
    uint32str(sourceid, buf);
    req.SetContentParam("SourceID", buf);
    int32str(req_index, buf);
    req.SetContentParam("StartIndex", buf);
    int32str(req_count, buf);
    req.SetContentParam("Count", buf);

    DBG(MYTH_DBG_DEBUG, "%s: request index(%d) count(%d)\n", __FUNCTION__, req_index, req_count);
    WSResponse resp(req);
    if (!resp.IsSuccessful())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (!root.isValid() || !json_is_object(root.get()))
    {
      DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
      break;
    }
    DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

    // Object: ChannelInfoList
    const json_t *clist = json_object_get(root.get(), "ChannelInfoList");
    ItemList list = ItemList(); // Using default constructor
    MythJSON::BindObject(clist, &list, bindlist);
    // List has ProtoVer. Check it or sound alarm
    if (list.protoVer != proto)
    {
      InvalidateService();
      break;
    }
    count = 0;
    // Object: ChannelInfos[]
    const json_t *chans = json_object_get(clist, "ChannelInfos");
    // Iterates over the sequence elements.
    for (size_t ci = 0; ci < json_array_size(chans); ++ci)
    {
      ++count;
      const json_t *chan = json_array_get(chans, ci);
      ChannelPtr channel(new Channel());  // Using default constructor
      // Bind the new channel
      MythJSON::BindObject(chan, channel.get(), bindchan);
      if (channel->chanId)
        ret->push_back(channel);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Guide service
////
ProgramMapPtr WSAPI::GetProgramGuide1_0(uint32_t chanid, time_t starttime, time_t endtime)
{
  ProgramMapPtr ret(new ProgramMap);
  char buf[32];
  int32_t count = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);
  const bindings_t *bindprog = MythDTO::getProgramBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Guide/GetProgramGuide");
  uint32str(chanid, buf);
  req.SetContentParam("StartChanId", buf);
  req.SetContentParam("NumChannels", "1");
  time2iso8601utc(starttime, buf);
  req.SetContentParam("StartTime", buf);
  time2iso8601utc(endtime, buf);
  req.SetContentParam("EndTime", buf);
  req.SetContentParam("Details", "true");

  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  // Object: ProgramGuide
  const json_t *glist = json_object_get(root.get(), "ProgramGuide");
  ItemList list = ItemList(); // Using default constructor
  MythJSON::BindObject(glist, &list, bindlist);
  // List has ProtoVer. Check it or sound alarm
  if (list.protoVer != proto)
  {
    InvalidateService();
    return ret;
  }
  // Object: Channels[]
  const json_t *chans = json_object_get(glist, "Channels");
  // Iterates over the sequence elements.
  for (size_t ci = 0; ci < json_array_size(chans); ++ci)
  {
    const json_t *chan = json_array_get(chans, ci);
    Channel channel;
    MythJSON::BindObject(chan, &channel, bindchan);
    // Object: Programs[]
    const json_t *progs = json_object_get(chan, "Programs");
    // Iterates over the sequence elements.
    for (size_t pi = 0; pi < json_array_size(progs); ++pi)
    {
      ++count;
      const json_t *prog = json_array_get(progs, pi);
      ProgramPtr program(new Program());  // Using default constructor
      // Bind the new program
      MythJSON::BindObject(prog, program.get(), bindprog);
      program->channel = channel;
      ret->insert(std::make_pair(program->startTime, program));
    }
  }
  DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Dvr service
////
ProgramListPtr WSAPI::GetRecordedList1_5(unsigned n, bool descending)
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  uint32_t req_index = 0, req_count = 100, count = 0, total = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindprog = MythDTO::getProgramBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);
  const bindings_t *bindreco = MythDTO::getRecordingBindArray(proto);
  const bindings_t *bindartw = MythDTO::getArtworkBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetRecordedList");

  do
  {
    // Adjust the packet size
    if (n && req_count > (n - total))
      req_count = (n - total);

    req.ClearContent();
    uint32str(req_index, buf);
    req.SetContentParam("StartIndex", buf);
    uint32str(req_count, buf);
    req.SetContentParam("Count", buf);
    req.SetContentParam("Descending", BOOLSTR(descending));

    DBG(MYTH_DBG_DEBUG, "%s: request index(%d) count(%d)\n", __FUNCTION__, req_index, req_count);
    WSResponse resp(req);
    if (!resp.IsSuccessful())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (!root.isValid() || !json_is_object(root.get()))
    {
      DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
      break;
    }
    DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

    // Object: ProgramList
    const json_t *plist = json_object_get(root.get(), "ProgramList");
    ItemList list = ItemList(); // Using default constructor
    MythJSON::BindObject(plist, &list, bindlist);
    // List has ProtoVer. Check it or sound alarm
    if (list.protoVer != proto)
    {
      InvalidateService();
      break;
    }
    count = 0;
    // Object: Programs[]
    const json_t *progs = json_object_get(plist, "Programs");
    // Iterates over the sequence elements.
    for (size_t pi = 0; pi < json_array_size(progs); ++pi)
    {
      ++count;
      const json_t *prog = json_array_get(progs, pi);
      ProgramPtr program(new Program());  // Using default constructor
      // Bind the new program
      MythJSON::BindObject(prog, program.get(), bindprog);
      // Bind channel of program
      const json_t *chan = json_object_get(prog, "Channel");
      MythJSON::BindObject(chan, &(program->channel), bindchan);
      // Bind recording of program
      const json_t *reco = json_object_get(prog, "Recording");
      MythJSON::BindObject(reco, &(program->recording), bindreco);
      // Bind artwork list of program
      const json_t *arts = json_object_get(json_object_get(prog, "Artwork"), "ArtworkInfos");
      for (size_t pa = 0; pa < json_array_size(arts); ++pa)
      {
        const json_t *artw = json_array_get(arts, pa);
        Artwork artwork = Artwork();  // Using default constructor
        MythJSON::BindObject(artw, &artwork, bindartw);
        program->artwork.push_back(artwork);
      }
      ret->push_back(program);
      ++total;
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count && (!n || n > total));

  return ret;
}

ProgramPtr WSAPI::GetRecorded1_5(uint32_t chanid, time_t recstartts)
{
  ProgramPtr ret;
  char buf[32];
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindprog = MythDTO::getProgramBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);
  const bindings_t *bindreco = MythDTO::getRecordingBindArray(proto);
  const bindings_t *bindartw = MythDTO::getArtworkBindArray(proto);

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetRecorded");
  uint32str(chanid, buf);
  req.SetContentParam("ChanId", buf);
  time2iso8601utc(recstartts, buf);
  req.SetContentParam("StartTime", buf);
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *prog = json_object_get(root.get(), "Program");
  ProgramPtr program(new Program());  // Using default constructor
  // Bind the new program
  MythJSON::BindObject(prog, program.get(), bindprog);
  // Bind channel of program
  const json_t *chan = json_object_get(prog, "Channel");
  MythJSON::BindObject(chan, &(program->channel), bindchan);
  // Bind recording of program
  const json_t *reco = json_object_get(prog, "Recording");
  MythJSON::BindObject(reco, &(program->recording), bindreco);
  // Bind artwork list of program
  const json_t *arts = json_object_get(json_object_get(prog, "Artwork"), "ArtworkInfos");
  for (size_t pa = 0; pa < json_array_size(arts); ++pa)
  {
    const json_t *artw = json_array_get(arts, pa);
    Artwork artwork = Artwork();  // Using default constructor
    MythJSON::BindObject(artw, &artwork, bindartw);
    program->artwork.push_back(artwork);
  }
  // Return valid program
  if (program->recording.startTs != INVALID_TIME)
    ret = program;
  return ret;
}

bool WSAPI::DeleteRecording2_1(uint32_t chanid, time_t recstartts, bool forceDelete, bool allowRerecord)
{
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/DeleteRecording", HRM_POST);
  uint32str(chanid, buf);
  req.SetContentParam("ChanId", buf);
  time2iso8601utc(recstartts, buf);
  req.SetContentParam("StartTime", buf);
  req.SetContentParam("ForceDelete", BOOLSTR(forceDelete));
  req.SetContentParam("AllowRerecord", BOOLSTR(allowRerecord));
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

bool WSAPI::UnDeleteRecording2_1(uint32_t chanid, time_t recstartts)
{
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/UnDeleteRecording", HRM_POST);
  uint32str(chanid, buf);
  req.SetContentParam("ChanId", buf);
  time2iso8601utc(recstartts, buf);
  req.SetContentParam("StartTime", buf);
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

bool WSAPI::UpdateRecordedWatchedStatus4_5(uint32_t chanid, time_t recstartts, bool watched)
{
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/UpdateRecordedWatchedStatus", HRM_POST);
  uint32str(chanid, buf);
  req.SetContentParam("ChanId", buf);
  time2iso8601utc(recstartts, buf);
  req.SetContentParam("StartTime", buf);
  req.SetContentParam("Watched", BOOLSTR(watched));
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

static void ProcessRecordIN(unsigned proto, RecordSchedule& record)
{
  // Converting API codes to internal types
  record.type_t = RuleTypeFromString(proto, record.type);
  record.searchType_t = SearchTypeFromString(proto, record.searchType);
  record.dupMethod_t = DupMethodFromString(proto, record.dupMethod);
  record.dupIn_t = DupInFromString(proto, record.dupIn);
}

RecordScheduleListPtr WSAPI::GetRecordScheduleList1_5()
{
  RecordScheduleListPtr ret(new RecordScheduleList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindrec = MythDTO::getRecordScheduleBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetRecordScheduleList");

  do
  {
    req.ClearContent();
    int32str(req_index, buf);
    req.SetContentParam("StartIndex", buf);
    int32str(req_count, buf);
    req.SetContentParam("Count", buf);

    DBG(MYTH_DBG_DEBUG, "%s: request index(%d) count(%d)\n", __FUNCTION__, req_index, req_count);
    WSResponse resp(req);
    if (!resp.IsSuccessful())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (!root.isValid() || !json_is_object(root.get()))
    {
      DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
      break;
    }
    DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

    // Object: RecRuleList
    const json_t *clist = json_object_get(root.get(), "RecRuleList");
    ItemList list = ItemList(); // Using default constructor
    MythJSON::BindObject(clist, &list, bindlist);
    // List has ProtoVer. Check it or sound alarm
    if (list.protoVer != proto)
    {
      InvalidateService();
      break;
    }
    count = 0;
    // Object: RecRules[]
    const json_t *recs = json_object_get(clist, "RecRules");
    // Iterates over the sequence elements.
    for (size_t ci = 0; ci < json_array_size(recs); ++ci)
    {
      ++count;
      const json_t *rec = json_array_get(recs, ci);
      RecordSchedulePtr record(new RecordSchedule()); // Using default constructor
      // Bind the new record
      MythJSON::BindObject(rec, record.get(), bindrec);
      ProcessRecordIN(proto, *record);
      ret->push_back(record);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

RecordSchedulePtr WSAPI::GetRecordSchedule1_5(uint32_t recordid)
{
  RecordSchedulePtr ret;
  char buf[32];
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindrec = MythDTO::getRecordScheduleBindArray(proto);

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetRecordSchedule");
  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *rec = json_object_get(root.get(), "RecRule");
  RecordSchedulePtr record(new RecordSchedule()); // Using default constructor
  // Bind the new record
  MythJSON::BindObject(rec, record.get(), bindrec);
  // Return valid record
  if (record->recordId > 0)
  {
    ProcessRecordIN(proto, *record);
    ret = record;
  }
  return ret;
}

static void ProcessRecordOUT(unsigned proto, RecordSchedule& record)
{
  char buf[10];
  struct tm stm;
  time_t st = record.startTime;
  localtime_r(&st, &stm);
  // Set find time & day
  sprintf(buf, "%.2d:%.2d:%.2d", stm.tm_hour, stm.tm_min, stm.tm_sec);
  record.findTime = buf;
  record.findDay = stm.tm_wday;
  // Converting internal types to API codes
  record.type = RuleTypeToString(proto, record.type_t);
  record.searchType = SearchTypeToString(proto, record.searchType_t);
  record.dupMethod = DupMethodToString(proto, record.dupMethod_t);
  record.dupIn = DupInToString(proto, record.dupIn_t);
}

bool WSAPI::AddRecordSchedule1_5(RecordSchedule& record)
{
  char buf[32];
  uint32_t recordid;
  unsigned proto = (unsigned)m_version.protocol;

  ProcessRecordOUT(proto, record);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/AddRecordSchedule", HRM_POST);

  req.SetContentParam("Title", record.title);
  req.SetContentParam("Subtitle", record.subtitle);
  req.SetContentParam("Description", record.description);
  req.SetContentParam("Category", record.category);
  time2iso8601utc(record.startTime, buf);
  req.SetContentParam("StartTime", buf);
  time2iso8601utc(record.endTime, buf);
  req.SetContentParam("EndTime", buf);
  req.SetContentParam("SeriesId", record.seriesId);
  req.SetContentParam("ProgramId", record.programId);
  uint32str(record.chanId, buf);
  req.SetContentParam("ChanId", buf);
  uint32str(record.parentId, buf);
  req.SetContentParam("ParentId", buf);
  req.SetContentParam("Inactive", BOOLSTR(record.inactive));
  uint16str(record.season, buf);
  req.SetContentParam("Season", buf);
  uint16str(record.episode, buf);
  req.SetContentParam("Episode", buf);
  req.SetContentParam("Inetref", record.inetref);
  req.SetContentParam("Type", record.type);
  req.SetContentParam("SearchType", record.searchType);
  int8str(record.recPriority, buf);
  req.SetContentParam("RecPriority", buf);
  uint32str(record.preferredInput, buf);
  req.SetContentParam("PreferredInput", buf);
  uint8str(record.startOffset, buf);
  req.SetContentParam("StartOffset", buf);
  uint8str(record.endOffset, buf);
  req.SetContentParam("EndOffset", buf);
  req.SetContentParam("DupMethod", record.dupMethod);
  req.SetContentParam("DupIn", record.dupIn);
  uint32str(record.filter, buf);
  req.SetContentParam("Filter", buf);
  req.SetContentParam("RecProfile", record.recProfile);
  req.SetContentParam("RecGroup", record.recGroup);
  req.SetContentParam("StorageGroup", record.storageGroup);
  req.SetContentParam("PlayGroup", record.playGroup);
  req.SetContentParam("AutoExpire", BOOLSTR(record.autoExpire));
  uint32str(record.maxEpisodes, buf);
  req.SetContentParam("MaxEpisodes", buf);
  req.SetContentParam("MaxNewest", BOOLSTR(record.maxNewest));
  req.SetContentParam("AutoCommflag", BOOLSTR(record.autoCommflag));
  req.SetContentParam("AutoTranscode", BOOLSTR(record.autoTranscode));
  req.SetContentParam("AutoMetaLookup", BOOLSTR(record.autoMetaLookup));
  req.SetContentParam("AutoUserJob1", BOOLSTR(record.autoUserJob1));
  req.SetContentParam("AutoUserJob2", BOOLSTR(record.autoUserJob2));
  req.SetContentParam("AutoUserJob3", BOOLSTR(record.autoUserJob3));
  req.SetContentParam("AutoUserJob4", BOOLSTR(record.autoUserJob4));
  uint32str(record.transcoder, buf);
  req.SetContentParam("Transcoder", buf);

  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "int");
  if (!field || str2uint32(json_string_value(field), &recordid))
    return false;
  record.recordId = recordid;
  return true;
}

bool WSAPI::AddRecordSchedule1_7(RecordSchedule& record)
{
  char buf[32];
  uint32_t recordid;
  unsigned proto = (unsigned)m_version.protocol;

  ProcessRecordOUT(proto, record);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/AddRecordSchedule", HRM_POST);

  req.SetContentParam("Title", record.title);
  req.SetContentParam("Subtitle", record.subtitle);
  req.SetContentParam("Description", record.description);
  req.SetContentParam("Category", record.category);
  time2iso8601utc(record.startTime, buf);
  req.SetContentParam("StartTime", buf);
  time2iso8601utc(record.endTime, buf);
  req.SetContentParam("EndTime", buf);
  req.SetContentParam("SeriesId", record.seriesId);
  req.SetContentParam("ProgramId", record.programId);
  uint32str(record.chanId, buf);
  req.SetContentParam("ChanId", buf);
  req.SetContentParam("Station", record.callSign);
  int8str(record.findDay, buf);
  req.SetContentParam("FindDay", buf);
  req.SetContentParam("FindTime", record.findTime);
  uint32str(record.parentId, buf);
  req.SetContentParam("ParentId", buf);
  req.SetContentParam("Inactive", BOOLSTR(record.inactive));
  uint16str(record.season, buf);
  req.SetContentParam("Season", buf);
  uint16str(record.episode, buf);
  req.SetContentParam("Episode", buf);
  req.SetContentParam("Inetref", record.inetref);
  req.SetContentParam("Type", record.type);
  req.SetContentParam("SearchType", record.searchType);
  int8str(record.recPriority, buf);
  req.SetContentParam("RecPriority", buf);
  uint32str(record.preferredInput, buf);
  req.SetContentParam("PreferredInput", buf);
  uint8str(record.startOffset, buf);
  req.SetContentParam("StartOffset", buf);
  uint8str(record.endOffset, buf);
  req.SetContentParam("EndOffset", buf);
  req.SetContentParam("DupMethod", record.dupMethod);
  req.SetContentParam("DupIn", record.dupIn);
  uint32str(record.filter, buf);
  req.SetContentParam("Filter", buf);
  req.SetContentParam("RecProfile", record.recProfile);
  req.SetContentParam("RecGroup", record.recGroup);
  req.SetContentParam("StorageGroup", record.storageGroup);
  req.SetContentParam("PlayGroup", record.playGroup);
  req.SetContentParam("AutoExpire", BOOLSTR(record.autoExpire));
  uint32str(record.maxEpisodes, buf);
  req.SetContentParam("MaxEpisodes", buf);
  req.SetContentParam("MaxNewest", BOOLSTR(record.maxNewest));
  req.SetContentParam("AutoCommflag", BOOLSTR(record.autoCommflag));
  req.SetContentParam("AutoTranscode", BOOLSTR(record.autoTranscode));
  req.SetContentParam("AutoMetaLookup", BOOLSTR(record.autoMetaLookup));
  req.SetContentParam("AutoUserJob1", BOOLSTR(record.autoUserJob1));
  req.SetContentParam("AutoUserJob2", BOOLSTR(record.autoUserJob2));
  req.SetContentParam("AutoUserJob3", BOOLSTR(record.autoUserJob3));
  req.SetContentParam("AutoUserJob4", BOOLSTR(record.autoUserJob4));
  uint32str(record.transcoder, buf);
  req.SetContentParam("Transcoder", buf);

  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "uint");
  if (!field || str2uint32(json_string_value(field), &recordid))
    return false;
  record.recordId = recordid;
  return true;
}

bool WSAPI::UpdateRecordSchedule1_7(RecordSchedule& record)
{
  char buf[32];
  unsigned proto = (unsigned)m_version.protocol;

  ProcessRecordOUT(proto, record);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/UpdateRecordSchedule", HRM_POST);

  uint32str(record.recordId, buf);
  req.SetContentParam("RecordId", buf);
  req.SetContentParam("Title", record.title);
  req.SetContentParam("Subtitle", record.subtitle);
  req.SetContentParam("Description", record.description);
  req.SetContentParam("Category", record.category);
  time2iso8601utc(record.startTime, buf);
  req.SetContentParam("StartTime", buf);
  time2iso8601utc(record.endTime, buf);
  req.SetContentParam("EndTime", buf);
  req.SetContentParam("SeriesId", record.seriesId);
  req.SetContentParam("ProgramId", record.programId);
  uint32str(record.chanId, buf);
  req.SetContentParam("ChanId", buf);
  req.SetContentParam("Station", record.callSign);
  int8str(record.findDay, buf);
  req.SetContentParam("FindDay", buf);
  req.SetContentParam("FindTime", record.findTime);
  uint32str(record.parentId, buf);
  req.SetContentParam("ParentId", buf);
  req.SetContentParam("Inactive", BOOLSTR(record.inactive));
  uint16str(record.season, buf);
  req.SetContentParam("Season", buf);
  uint16str(record.episode, buf);
  req.SetContentParam("Episode", buf);
  req.SetContentParam("Inetref", record.inetref);
  req.SetContentParam("Type", record.type);
  req.SetContentParam("SearchType", record.searchType);
  int8str(record.recPriority, buf);
  req.SetContentParam("RecPriority", buf);
  uint32str(record.preferredInput, buf);
  req.SetContentParam("PreferredInput", buf);
  uint8str(record.startOffset, buf);
  req.SetContentParam("StartOffset", buf);
  uint8str(record.endOffset, buf);
  req.SetContentParam("EndOffset", buf);
  req.SetContentParam("DupMethod", record.dupMethod);
  req.SetContentParam("DupIn", record.dupIn);
  uint32str(record.filter, buf);
  req.SetContentParam("Filter", buf);
  req.SetContentParam("RecProfile", record.recProfile);
  req.SetContentParam("RecGroup", record.recGroup);
  req.SetContentParam("StorageGroup", record.storageGroup);
  req.SetContentParam("PlayGroup", record.playGroup);
  req.SetContentParam("AutoExpire", BOOLSTR(record.autoExpire));
  uint32str(record.maxEpisodes, buf);
  req.SetContentParam("MaxEpisodes", buf);
  req.SetContentParam("MaxNewest", BOOLSTR(record.maxNewest));
  req.SetContentParam("AutoCommflag", BOOLSTR(record.autoCommflag));
  req.SetContentParam("AutoTranscode", BOOLSTR(record.autoTranscode));
  req.SetContentParam("AutoMetaLookup", BOOLSTR(record.autoMetaLookup));
  req.SetContentParam("AutoUserJob1", BOOLSTR(record.autoUserJob1));
  req.SetContentParam("AutoUserJob2", BOOLSTR(record.autoUserJob2));
  req.SetContentParam("AutoUserJob3", BOOLSTR(record.autoUserJob3));
  req.SetContentParam("AutoUserJob4", BOOLSTR(record.autoUserJob4));
  uint32str(record.transcoder, buf);
  req.SetContentParam("Transcoder", buf);

  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

bool WSAPI::DisableRecordSchedule1_5(uint32_t recordid)
{
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/DisableRecordSchedule", HRM_POST);

  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);

  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

bool WSAPI::EnableRecordSchedule1_5(uint32_t recordid)
{
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/EnableRecordSchedule", HRM_POST);

  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);

  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

bool WSAPI::RemoveRecordSchedule1_5(uint32_t recordid)
{
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/RemoveRecordSchedule", HRM_POST);

  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);

  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *field = json_object_get(root.get(), "bool");
  if (!field || strcmp(json_string_value(field), "true"))
    return false;
  return true;
}

ProgramListPtr WSAPI::GetUpcomingList1_5()
{
  // Only for backward compatibility (0.27)
  ProgramListPtr ret = GetUpcomingList2_2();
  // Add being recorded (https://code.mythtv.org/trac/changeset/3084ebc/mythtv)
  ProgramListPtr recordings = GetRecordedList(20, true);
  for (Myth::ProgramList::iterator it = recordings->begin(); it != recordings->end(); ++it)
  {
    if ((*it)->recording.status == RS_RECORDING)
      ret->push_back(*it);
  }
  return ret;
}

ProgramListPtr WSAPI::GetUpcomingList2_2()
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindprog = MythDTO::getProgramBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);
  const bindings_t *bindreco = MythDTO::getRecordingBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetUpcomingList");

  do
  {
    req.ClearContent();
    int32str(req_index, buf);
    req.SetContentParam("StartIndex", buf);
    int32str(req_count, buf);
    req.SetContentParam("Count", buf);
    req.SetContentParam("ShowAll", "true");

    DBG(MYTH_DBG_DEBUG, "%s: request index(%d) count(%d)\n", __FUNCTION__, req_index, req_count);
    WSResponse resp(req);
    if (!resp.IsSuccessful())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (!root.isValid() || !json_is_object(root.get()))
    {
      DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
      break;
    }
    DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

    // Object: ProgramList
    const json_t *plist = json_object_get(root.get(), "ProgramList");
    ItemList list = ItemList(); // Using default constructor
    MythJSON::BindObject(plist, &list, bindlist);
    // List has ProtoVer. Check it or sound alarm
    if (list.protoVer != proto)
    {
      InvalidateService();
      break;
    }
    count = 0;
    // Object: Programs[]
    const json_t *progs = json_object_get(plist, "Programs");
    // Iterates over the sequence elements.
    for (size_t pi = 0; pi < json_array_size(progs); ++pi)
    {
      ++count;
      const json_t *prog = json_array_get(progs, pi);
      ProgramPtr program(new Program());  // Using default constructor
      // Bind the new program
      MythJSON::BindObject(prog, program.get(), bindprog);
      // Bind channel of program
      const json_t *chan = json_object_get(prog, "Channel");
      MythJSON::BindObject(chan, &(program->channel), bindchan);
      // Bind recording of program
      const json_t *reco = json_object_get(prog, "Recording");
      MythJSON::BindObject(reco, &(program->recording), bindreco);
      ret->push_back(program);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

ProgramListPtr WSAPI::GetConflictList1_5()
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindprog = MythDTO::getProgramBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);
  const bindings_t *bindreco = MythDTO::getRecordingBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetConflictList");

  do
  {
    req.ClearContent();
    int32str(req_index, buf);
    req.SetContentParam("StartIndex", buf);
    int32str(req_count, buf);
    req.SetContentParam("Count", buf);

    DBG(MYTH_DBG_DEBUG, "%s: request index(%d) count(%d)\n", __FUNCTION__, req_index, req_count);
    WSResponse resp(req);
    if (!resp.IsSuccessful())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (!root.isValid() || !json_is_object(root.get()))
    {
      DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
      break;
    }
    DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

    // Object: ProgramList
    const json_t *plist = json_object_get(root.get(), "ProgramList");
    ItemList list = ItemList(); // Using default constructor
    MythJSON::BindObject(plist, &list, bindlist);
    // List has ProtoVer. Check it or sound alarm
    if (list.protoVer != proto)
    {
      InvalidateService();
      break;
    }
    count = 0;
    // Object: Programs[]
    const json_t *progs = json_object_get(plist, "Programs");
    // Iterates over the sequence elements.
    for (size_t pi = 0; pi < json_array_size(progs); ++pi)
    {
      ++count;
      const json_t *prog = json_array_get(progs, pi);
      ProgramPtr program(new Program());  // Using default constructor
      // Bind the new program
      MythJSON::BindObject(prog, program.get(), bindprog);
      // Bind channel of program
      const json_t *chan = json_object_get(prog, "Channel");
      MythJSON::BindObject(chan, &(program->channel), bindchan);
      // Bind recording of program
      const json_t *reco = json_object_get(prog, "Recording");
      MythJSON::BindObject(reco, &(program->recording), bindreco);
      ret->push_back(program);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

ProgramListPtr WSAPI::GetExpiringList1_5()
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindlist = MythDTO::getListBindArray(proto);
  const bindings_t *bindprog = MythDTO::getProgramBindArray(proto);
  const bindings_t *bindchan = MythDTO::getChannelBindArray(proto);
  const bindings_t *bindreco = MythDTO::getRecordingBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetExpiringList");

  do
  {
    req.ClearContent();
    int32str(req_index, buf);
    req.SetContentParam("StartIndex", buf);
    int32str(req_count, buf);
    req.SetContentParam("Count", buf);

    DBG(MYTH_DBG_DEBUG, "%s: request index(%d) count(%d)\n", __FUNCTION__, req_index, req_count);
    WSResponse resp(req);
    if (!resp.IsSuccessful())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = MythJSON::ParseResponseJSON(resp);
    if (!root.isValid() || !json_is_object(root.get()))
    {
      DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
      break;
    }
    DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

    // Object: ProgramList
    const json_t *plist = json_object_get(root.get(), "ProgramList");
    ItemList list = ItemList(); // Using default constructor
    MythJSON::BindObject(plist, &list, bindlist);
    // List has ProtoVer. Check it or sound alarm
    if (list.protoVer != proto)
    {
      InvalidateService();
      break;
    }
    count = 0;
    // Object: Programs[]
    const json_t *progs = json_object_get(plist, "Programs");
    // Iterates over the sequence elements.
    for (size_t pi = 0; pi < json_array_size(progs); ++pi)
    {
      ++count;
      const json_t *prog = json_array_get(progs, pi);
      ProgramPtr program(new Program());  // Using default constructor
      // Bind the new program
      MythJSON::BindObject(prog, program.get(), bindprog);
      // Bind channel of program
      const json_t *chan = json_object_get(prog, "Channel");
      MythJSON::BindObject(chan, &(program->channel), bindchan);
      // Bind recording of program
      const json_t *reco = json_object_get(prog, "Recording");
      MythJSON::BindObject(reco, &(program->recording), bindreco);
      ret->push_back(program);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Content service
////
WSStreamPtr WSAPI::GetFile1_32(const std::string& filename, const std::string& sgname)
{
  WSStreamPtr ret;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestService("/Content/GetFile");
  req.SetContentParam("StorageGroup", sgname);
  req.SetContentParam("FileName", filename);
  WSResponse *resp = new WSResponse(req);
  if (!resp->IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}

WSStreamPtr WSAPI::GetChannelIcon1_32(uint32_t chanid, unsigned width, unsigned height)
{
  WSStreamPtr ret;
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestService("/Guide/GetChannelIcon");
  uint32str(chanid, buf);
  req.SetContentParam("ChanId", buf);
  if (width && height)
  {
    uint32str(width, buf);
    req.SetContentParam("Width", buf);
    uint32str(height, buf);
    req.SetContentParam("Height", buf);
  }
  WSResponse *resp = new WSResponse(req);
  if (!resp->IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}

WSStreamPtr WSAPI::GetPreviewImage1_32(uint32_t chanid, time_t recstartts, unsigned width, unsigned height)
{
  WSStreamPtr ret;
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestService("/Content/GetPreviewImage");
  uint32str(chanid, buf);
  req.SetContentParam("ChanId", buf);
  time2iso8601utc(recstartts, buf);
  req.SetContentParam("StartTime", buf);
  if (width && height)
  {
    uint32str(width, buf);
    req.SetContentParam("Width", buf);
    uint32str(height, buf);
    req.SetContentParam("Height", buf);
  }
  WSResponse *resp = new WSResponse(req);
  /* try redirection if any */
  if (resp->GetStatusCode() == 301 && !resp->Redirection().empty())
  {
    URIParser uri(resp->Redirection());
    WSRequest rreq(ResolveHostName(uri.Host()), uri.Port());
    rreq.RequestService(std::string("/").append(uri.Path()));
    delete resp;
    resp = new WSResponse(rreq);
  }
  if (!resp->IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}

WSStreamPtr WSAPI::GetRecordingArtwork1_32(const std::string& type, const std::string& inetref, uint16_t season, unsigned width, unsigned height)
{
  WSStreamPtr ret;
  char buf[32];

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestService("/Content/GetRecordingArtwork");
  req.SetContentParam("Type", type.c_str());
  req.SetContentParam("Inetref", inetref.c_str());
  uint16str(season, buf);
  req.SetContentParam("Season", buf);
  if (width && height)
  {
    uint32str(width, buf);
    req.SetContentParam("Width", buf);
    uint32str(height, buf);
    req.SetContentParam("Height", buf);
  }
  WSResponse *resp = new WSResponse(req);
  if (!resp->IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}

ArtworkListPtr WSAPI::GetRecordingArtworkList1_32(uint32_t chanid, time_t recstartts)
{
  ArtworkListPtr ret(new ArtworkList);
  char buf[32];
  unsigned proto = (unsigned)m_version.protocol;

  // Get bindings for protocol version
  const bindings_t *bindartw = MythDTO::getArtworkBindArray(proto);

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Content/GetRecordingArtworkList");
  uint32str(chanid, buf);
  req.SetContentParam("ChanId", buf);
  time2iso8601utc(recstartts, buf);
  req.SetContentParam("StartTime", buf);
  WSResponse resp(req);
  if (!resp.IsSuccessful())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = MythJSON::ParseResponseJSON(resp);
  if (!root.isValid() || !json_is_object(root.get()))
  {
    DBG(MYTH_DBG_ERROR, "%s: unexpected content\n", __FUNCTION__);
    return ret;
  }
  DBG(MYTH_DBG_DEBUG, "%s: content parsed\n", __FUNCTION__);

  const json_t *list = json_object_get(root.get(), "ArtworkInfoList");
  // Bind artwork list
  const json_t *arts = json_object_get(list, "ArtworkInfos");
  for (size_t pa = 0; pa < json_array_size(arts); ++pa)
  {
    const json_t *artw = json_array_get(arts, pa);
    ArtworkPtr artwork(new Artwork());  // Using default constructor
    MythJSON::BindObject(artw, artwork.get(), bindartw);
    ret->push_back(artwork);
  }
  return ret;
}
