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
#include "private/mythjsonbinder.h"
#include "private/janssonptr.h"
#include "private/mythwsrequest.h"
#include "private/mythwsresponse.h"
#include "private/platform/threads/mutex.h"
#include "private/platform/util/util.h"

#include <jansson.h>

#define BOOLSTR(a)  ((a) ? "true" : "false")

using namespace Myth;

JanssonPtr ParseResponseJSON(WSResponse& resp)
{
  // Read content response
  JanssonPtr root;
  size_t r, content_length = resp.GetContentLength();
  char *content = new char[content_length + 1];
  if ((r = resp.ReadContent(content, content_length)) == content_length)
  {
    json_error_t error;
    content[content_length] = '\0';
    DBG(MYTH_DBG_PROTO,"%s: %s\n", __FUNCTION__, content);
    // Parse JSON content
    root.reset(json_loads(content, 0, &error));
    if (!root.isValid())
      DBG(MYTH_DBG_ERROR, "%s: failed to parse: %d: %s\n", __FUNCTION__, error.line, error.text);
  }
  else
  {
    DBG(MYTH_DBG_ERROR, "%s: read error\n", __FUNCTION__);
  }
  delete[] content;
  return root;
}

WSAPI::WSAPI(const std::string& server, unsigned port)
: m_mutex(new PLATFORM::CMutex)
, m_server(server)
, m_port(port)
, m_checked(false)
, m_version()
, m_serverHostName()
{
  // Try it !
  CheckService();
}

WSAPI::~WSAPI()
{
  SAFE_DELETE(m_mutex);
}

bool WSAPI::CheckServerHostName()
{
  m_serverHostName.clear();

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetHostName");
  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  // Parse content response
  JanssonPtr root = ParseResponseJSON(resp);
  if (root.isValid() && json_is_object(root.get()))
  {
    const json_t *field = json_object_get(root.get(), "String");
    if (field != NULL)
    {
      const char *val = json_string_value(field);
      if (val != NULL)
      {
        m_serverHostName = val;
        return true;
      }
    }
  }
  return false;
}

bool WSAPI::CheckVersion()
{
  m_version.protocol = 0;
  m_version.schema = 0;
  m_version.version.clear();

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetConnectionInfo");
  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  // Parse content response
  JanssonPtr root = ParseResponseJSON(resp);
  if (root.isValid() && json_is_object(root.get()))
  {
    const json_t *con = json_object_get(root.get(), "ConnectionInfo");
    if (con != NULL)
    {
      const json_t *ver = json_object_get(con, "Version");
      MythJSON::BindObject(ver, &m_version, MythDTO::getVersionBindArray(0));
      if (m_version.protocol)
        return true;
    }
  }
  return false;
}

unsigned WSAPI::CheckService()
{
  PLATFORM::CLockObject lock(*m_mutex);
  if (m_checked)
    return (unsigned)m_version.protocol;

  if (CheckServerHostName() && CheckVersion())
  {
    DBG(MYTH_DBG_INFO, "%s: MythTV API service is available: %s:%d(%s) protocol(%d) schema(%d)\n",
            __FUNCTION__, m_serverHostName.c_str(), m_port, m_version.version.c_str(),
            (unsigned)m_version.protocol, (unsigned)m_version.schema);
    m_checked = true;
    return (unsigned)m_version.protocol;
  }
  DBG(MYTH_DBG_ERROR, "%s: MythTV API service is unavailable: %s:%d\n", __FUNCTION__, m_server.c_str(), m_port);
  return 0;
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
  Version *p = new Version();
  p->version = m_version.version;
  p->protocol = m_version.protocol;
  p->schema = m_version.schema;
  return VersionPtr(p);
}

///////////////////////////////////////////////////////////////////////////////
////
////  Service operations
////

SettingPtr WSAPI::GetSetting(const std::string& key, bool myhost)
{
  SettingPtr ret;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetSetting");
  std::string hostname;
  if (myhost)
    hostname = TcpSocket::GetMyHostName();
  req.SetContentParam("HostName", hostname);
  req.SetContentParam("Key", key);
  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

SettingMapPtr WSAPI::GetSettings(bool myhost)
{
  SettingMapPtr ret(new SettingMap);
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Myth/GetSetting");
  std::string hostname;
  if (myhost)
    hostname = TcpSocket::GetMyHostName();
  req.SetContentParam("HostName", hostname);
  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::PutSetting(const std::string& key, const std::string& value, bool myhost)
{
  unsigned proto;

  if (!(proto = CheckService()))
    return false;

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
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

ProgramListPtr WSAPI::GetRecordedList(unsigned n, bool descending)
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  uint32_t req_index = 0, req_count = 100, count = 0, total = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
    if (!resp.IsValid())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = ParseResponseJSON(resp);
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

ProgramPtr WSAPI::GetRecorded(uint32_t chanid, time_t recstartts)
{
  ProgramPtr ret;
  char buf[32];
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::UpdateRecordedWatchedStatus79(uint32_t chanid, time_t recstartts, bool watched)
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
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

CaptureCardListPtr WSAPI::GetCaptureCardList()
{
  CaptureCardListPtr ret(new CaptureCardList);
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

  // Get bindings for protocol version
  const bindings_t *bindcard = MythDTO::getCaptureCardBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Capture/GetCaptureCardList");
  req.SetContentParam("HostName", m_serverHostName.c_str());
  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

VideoSourceListPtr WSAPI::GetVideoSourceList()
{
  VideoSourceListPtr ret(new VideoSourceList);
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

  // Get bindings for protocol version
  const bindings_t *bindvsrc = MythDTO::getVideoSourceBindArray(proto);

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Channel/GetVideoSourceList");
  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

ChannelListPtr WSAPI::GetChannelList75(uint32_t sourceid, bool onlyVisible)
{
  ChannelListPtr ret(new ChannelList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
    if (!resp.IsValid())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = ParseResponseJSON(resp);
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
      if (!onlyVisible || channel->visible)
        ret->push_back(channel);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

ChannelListPtr WSAPI::GetChannelList82(uint32_t sourceid, bool onlyVisible)
{
  ChannelListPtr ret(new ChannelList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
    if (!resp.IsValid())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = ParseResponseJSON(resp);
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
      ret->push_back(channel);
    }
    DBG(MYTH_DBG_DEBUG, "%s: received count(%d)\n", __FUNCTION__, count);
    req_index += count; // Set next requested index
  }
  while (count == req_count);

  return ret;
}

ProgramMapPtr WSAPI::GetProgramGuide(uint32_t chanid, time_t starttime, time_t endtime)
{
  ProgramMapPtr ret(new ProgramMap);
  char buf[32];
  int32_t count = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

void WSAPI::ProcessRecordIN(unsigned proto, RecordSchedule& record)
{
  // Converting API codes to internal types
  record.type_t = RuleTypeFromString(proto, record.type);
  record.searchType_t = SearchTypeFromString(proto, record.searchType);
  record.dupMethod_t = DupMethodFromString(proto, record.dupMethod);
  record.dupIn_t = DupInFromString(proto, record.dupIn);
}

void WSAPI::ProcessRecordOUT(unsigned proto, RecordSchedule& record)
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

RecordScheduleListPtr WSAPI::GetRecordScheduleList()
{
  RecordScheduleListPtr ret(new RecordScheduleList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
    if (!resp.IsValid())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = ParseResponseJSON(resp);
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

RecordSchedulePtr WSAPI::GetRecordSchedule(uint32_t recordid)
{
  RecordSchedulePtr ret;
  char buf[32];
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

  // Get bindings for protocol version
  const bindings_t *bindrec = MythDTO::getRecordScheduleBindArray(proto);

  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/GetRecordSchedule");
  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);
  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return ret;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::AddRecordSchedule75(RecordSchedule& record)
{
  char buf[32];
  uint32_t recordid;

  ProcessRecordOUT(75, record);

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
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::AddRecordSchedule76(RecordSchedule& record)
{
  char buf[32];
  uint32_t recordid;

  ProcessRecordOUT(76, record);

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
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::UpdateRecordSchedule76(RecordSchedule& record)
{
  char buf[32];

  ProcessRecordOUT(76, record);

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
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::DisableRecordSchedule(uint32_t recordid)
{
  char buf[32];
  unsigned proto;

  if (!(proto = CheckService()))
    return false;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/DisableRecordSchedule", HRM_POST);

  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);

  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::EnableRecordSchedule(uint32_t recordid)
{
  char buf[32];
  unsigned proto;

  if (!(proto = CheckService()))
    return false;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/EnableRecordSchedule", HRM_POST);

  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);

  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

bool WSAPI::RemoveRecordSchedule(uint32_t recordid)
{
  char buf[32];
  unsigned proto;

  if (!(proto = CheckService()))
    return false;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestAccept(CT_JSON);
  req.RequestService("/Dvr/RemoveRecordSchedule", HRM_POST);

  uint32str(recordid, buf);
  req.SetContentParam("RecordId", buf);

  WSResponse resp(req);
  if (!resp.IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    return false;
  }
  JanssonPtr root = ParseResponseJSON(resp);
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

ProgramListPtr WSAPI::GetUpcomingList75()
{
  // Only for backward compatibility (0.27)
  ProgramListPtr ret = GetUpcomingList79();
  // Add being recorded (https://code.mythtv.org/trac/changeset/3084ebc/mythtv)
  ProgramListPtr recordings = GetRecordedList(20, true);
  for (Myth::ProgramList::iterator it = recordings->begin(); it != recordings->end(); ++it)
  {
    if ((*it)->recording.status == RS_RECORDING)
      ret->push_back(*it);
  }
  return ret;
}

ProgramListPtr WSAPI::GetUpcomingList79()
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
    if (!resp.IsValid())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = ParseResponseJSON(resp);
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

ProgramListPtr WSAPI::GetConflictList()
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
    if (!resp.IsValid())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = ParseResponseJSON(resp);
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

ProgramListPtr WSAPI::GetExpiringList()
{
  ProgramListPtr ret(new ProgramList);
  char buf[32];
  int32_t req_index = 0, req_count = 100, count = 0;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

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
    if (!resp.IsValid())
    {
      DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
      break;
    }
    JanssonPtr root = ParseResponseJSON(resp);
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

WSStreamPtr WSAPI::GetFile(const std::string& filename, const std::string& sgname)
{
  WSStreamPtr ret;
  unsigned proto;

  if (!(proto = CheckService()))
    return ret;

  // Initialize request header
  WSRequest req = WSRequest(m_server, m_port);
  req.RequestService("/Content/GetFile");
  req.SetContentParam("StorageGroup", sgname);
  req.SetContentParam("FileName", filename);
  WSResponse *resp = new WSResponse(req);
  if (!resp->IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}

WSStreamPtr WSAPI::GetChannelIcon(uint32_t chanid, unsigned width, unsigned height)
{
  WSStreamPtr ret;
  unsigned proto;
  char buf[32];

  if (!(proto = CheckService()))
    return ret;

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
  if (!resp->IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}

WSStreamPtr WSAPI::GetPreviewImage(uint32_t chanid, time_t recstartts, unsigned width, unsigned height)
{
  WSStreamPtr ret;
  unsigned proto;
  char buf[32];

  if (!(proto = CheckService()))
    return ret;

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
  if (!resp->IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}

WSStreamPtr WSAPI::GetRecordingArtwork(const std::string& type, const std::string& inetref, uint16_t season, unsigned width, unsigned height)
{
  WSStreamPtr ret;
  unsigned proto;
  char buf[32];

  if (!(proto = CheckService()))
    return ret;

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
  if (!resp->IsValid())
  {
    DBG(MYTH_DBG_ERROR, "%s: invalid response\n", __FUNCTION__);
    delete resp;
    return ret;
  }
  ret.reset(new WSStream(resp));
  return ret;
}
