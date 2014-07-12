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

#include "mythprotobase.h"
#include "../mythdebug.h"
#include "../private/builtin.h"
#include "../private/mythsocket.h"
#include "../private/platform/threads/mutex.h"
#include "../private/platform/util/util.h"

#include <limits>
#include <cstdio>

using namespace Myth;

typedef struct
{
  unsigned version;
  char token[14]; // up to 13 chars used in v74 + the terminating NULL character
} myth_protomap_t;

static myth_protomap_t protomap[] = {
  {75, "SweetRock"},
  {76, "FireWilde"},
  {77, "WindMark"},
  {79, "BasaltGiant"},
  {80, "TaDah!"},
  {81, "MultiRecDos"},
  {82, "IdIdO"},
  {0, ""}
};

ProtoBase::ProtoBase(const std::string& server, unsigned port)
: m_mutex(new PLATFORM::CMutex)
, m_socket(new TcpSocket())
, m_protoVersion(0)
, m_isOpen(false)
, m_server(server)
, m_port(port)
, m_hang(false)
, m_msgLength(0)
, m_msgConsumed(0)
{
}

ProtoBase::~ProtoBase()
{
  this->Close();
  SAFE_DELETE(m_socket);
  SAFE_DELETE(m_mutex);
}

void ProtoBase::HangException()
{
  DBG(MYTH_DBG_ERROR, "%s: connection hang with error %d\n", __FUNCTION__, m_socket->GetErrNo());
  m_hang = true;
  this->Close();
  //@TODO Try reconnect
}

bool ProtoBase::SendCommand(const char *cmd, bool feedback)
{
  char buf[9];
  size_t l = strlen(cmd);

  if (m_msgConsumed != m_msgLength)
  {
    DBG(MYTH_DBG_ERROR, "%s: did not consume everything\n", __FUNCTION__);
    FlushMessage();
  }

  if (l > 0 && l < PROTO_SENDMSG_MAXSIZE)
  {
    std::string msg;
    msg.reserve(l + 8);
    sprintf(buf, "%-8u", (unsigned)l);
    msg.append(buf).append(cmd);
    DBG(MYTH_DBG_PROTO, "%s: %s\n", __FUNCTION__, cmd);
    if (m_socket->SendMessage(msg.c_str(), msg.size()))
    {
      if (feedback)
        return RcvMessageLength();
      return true;
    }
  }
  DBG(MYTH_DBG_ERROR, "%s: failed (%d)\n", __FUNCTION__, m_socket->GetErrNo());
  return false;
}

size_t ProtoBase::GetMessageLength() const
{
  return m_msgLength;
}

/**
 * Read one field from the backend response
 * @param field
 * @return true : false
 */
bool ProtoBase::ReadField(std::string& field)
{
  const char *str_sep = PROTO_STR_SEPARATOR;
  size_t str_sep_len = PROTO_STR_SEPARATOR_LEN;
  char buf[PROTO_BUFFER_SIZE];
  size_t p = 0, p_ss = 0, l = m_msgLength, c = m_msgConsumed;

  field.clear();
  if ( c >= l)
    return false;

  for (;;)
  {
    if (l > c)
    {
      if (m_socket->ReadResponse(&buf[p], 1) < 1)
      {
        HangException();
        return false;
      }
      ++c;
      if (buf[p++] == str_sep[p_ss])
      {
        if (++p_ss >= str_sep_len)
        {
          // Append data until separator before exit
          buf[p - str_sep_len] = '\0';
          field.append(buf);
          break;
        }
      }
      else
      {
        p_ss = 0;
        if (p > (PROTO_BUFFER_SIZE - 2 - str_sep_len))
        {
          // Append data before flushing to refill the following
          buf[p] = '\0';
          field.append(buf);
          p = 0;
        }
      }
    }
    else
    {
      // All is consumed. Append rest of data before exit
      buf[p] = '\0';
      field.append(buf);
      break;
    }
  }
  // Renew consumed or reset when no more data
  if (l > c)
    m_msgConsumed = c;
  else
    m_msgConsumed = m_msgLength = 0;
  return true;
}

bool ProtoBase::IsMessageOK(const std::string& field) const
{
  if (field.size() == 2)
  {
    if ((field[0] == 'O' || field[0] == 'o') && (field[1] == 'K' || field[1] == 'k'))
      return true;
  }
  return false;
}

size_t ProtoBase::FlushMessage()
{
  char buf[PROTO_BUFFER_SIZE];
  size_t r, n = 0, f = m_msgLength - m_msgConsumed;

  while (f > 0)
  {
    r = (f > PROTO_BUFFER_SIZE ? PROTO_BUFFER_SIZE : f);
    if (m_socket->ReadResponse(buf, r) != r)
    {
      HangException();
      break;
    }
    f -= r;
    n += r;
  }
  m_msgLength = m_msgConsumed = 0;
  return n;
}

bool ProtoBase::RcvMessageLength()
{
  char buf[9];
  uint32_t val = 0;

  // If not placed on head of new response then break
  if (m_msgLength > 0)
    return false;

  if (m_socket->ReadResponse(buf, 8) == 8)
  {
    if (0 == str2uint32(buf, &val))
    {
      DBG(MYTH_DBG_PROTO, "%s: %" PRIu32 "\n", __FUNCTION__, val);
      m_msgLength = (size_t)val;
      m_msgConsumed = 0;
      return true;
    }
    DBG(MYTH_DBG_ERROR, "%s: failed ('%s')\n", __FUNCTION__, buf);
  }
  HangException();
  return false;
}

/**
 * Parse feedback of command MYTH_PROTO_VERSION and return protocol version
 * of backend
 * @param version
 * @return true : false
 */
bool ProtoBase::RcvVersion(unsigned *version)
{
  std::string field;
  uint32_t val = 0;

  /*
   * The string we just consumed was either "ACCEPT" or "REJECT".  In
   * either case, the number following it is the correct version, and
   * we use it as an unsigned.
   */
  if (!ReadField(field))
    goto out;
  if (!ReadField(field))
    goto out;
  if (FlushMessage())
  {
    DBG(MYTH_DBG_ERROR, "%s: did not consume everything\n", __FUNCTION__);
    return false;
  }
  if (0 != str2uint32(field.c_str(), &val))
    goto out;
  *version = (unsigned)val;
  return true;

out:
  DBG(MYTH_DBG_ERROR, "%s: failed ('%s')\n", __FUNCTION__, field.c_str());
  FlushMessage();
  return false;
}

bool ProtoBase::OpenConnection(int rcvbuf)
{
  static unsigned my_version = 0;
  char cmd[256];
  bool ok, retry, attempt = false;
  myth_protomap_t *map = protomap;
  unsigned tmp_ver;

  PLATFORM::CLockObject lock(*m_mutex);

  if (!my_version)
    tmp_ver = map->version;
  else
    tmp_ver = my_version;

  if (IsOpen())
    this->Close();
  do
  {
    retry = false;
    if (!(ok = m_socket->Connect(m_server.c_str(), m_port, rcvbuf)))
      continue;
    // Now socket is connected: Reset hang
    m_hang = false;

    while (map->version != 0 && map->version != tmp_ver)
      ++map;

    ok = false;
    if (map->version == 0)
    {
      DBG(MYTH_DBG_ERROR, "%s: failed to connect with any version\n", __FUNCTION__);
      continue;
    }

    sprintf(cmd, "MYTH_PROTO_VERSION %" PRIu32 " %s", map->version, map->token);

    if (!(ok = SendCommand(cmd)))
      continue;
    if (!(ok = RcvVersion(&tmp_ver)))
      continue;

    DBG(MYTH_DBG_DEBUG, "%s: asked for version %" PRIu32 ", got version %" PRIu32 "\n",
            __FUNCTION__, map->version, tmp_ver);

    if (map->version == tmp_ver)
      continue;

    if (!attempt)
    {
      m_socket->Disconnect();
      retry = true;
    }
    ok = false;
  }
  while (retry);

  if (!ok)
  {
    m_socket->Disconnect();
    m_isOpen = false;
    m_protoVersion = 0;
    return false;
  }
  DBG(MYTH_DBG_DEBUG, "%s: agreed on Version %u protocol\n", __FUNCTION__, tmp_ver);
  if (tmp_ver != my_version)
    my_version = tmp_ver; // Store agreed version for next time
  m_isOpen = true;
  m_protoVersion = tmp_ver;
  return true;
}

void ProtoBase::Close()
{
  const char *cmd = "DONE";

  PLATFORM::CLockObject lock(*m_mutex);

  if (m_socket->IsConnected())
  {
    // Close gracefully by sending DONE message before disconnect
    if (m_isOpen && !m_hang)
    {
      if (SendCommand(cmd, false))
        DBG(MYTH_DBG_PROTO, "%s: done\n", __FUNCTION__);
      else
        DBG(MYTH_DBG_WARN, "%s: gracefully failed (%d)\n", __FUNCTION__, m_socket->GetErrNo());
    }
    m_socket->Disconnect();
  }
  m_isOpen = false;
  m_msgLength = m_msgConsumed = 0;
}

bool ProtoBase::IsOpen() const
{
  return m_isOpen;
}

unsigned ProtoBase::GetProtoVersion() const
{
  if (m_isOpen)
    return m_protoVersion;
  return 0;
}

std::string ProtoBase::GetServer() const
{
  return m_server;
}

unsigned ProtoBase::GetPort() const
{
  return m_port;
}

int ProtoBase::GetSocketErrNo() const
{
  return m_socket->GetErrNo();
}

ProgramPtr ProtoBase::RcvProgramInfo75()
{
  int64_t tmpi;
  std::string field;
  ProgramPtr program(new Program());
  int i = 0;

  ++i;
  if (!ReadField(program->title))
    goto out;
  ++i;
  if (!ReadField(program->subTitle))
    goto out;
  ++i;
  if (!ReadField(program->description))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->season)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->episode)))
    goto out;
  ++i;
  if (!ReadField(program->category))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.chanId)))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanNum))
    goto out;
  ++i;
  if (!ReadField(program->channel.callSign))
    goto out;
  ++i;
  if (!ReadField(program->channel.channelName))
    goto out;
  ++i;
  if (!ReadField(program->fileName))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &(program->fileSize)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->startTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->endTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field)) // findid
    goto out;
  ++i;
  if (!ReadField(program->hostName))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.sourceId)))
    goto out;
  ++i;
  if (!ReadField(field)) // cardid
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.inputId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int32(field.c_str(), &(program->recording.priority)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int8(field.c_str(), &(program->recording.status)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->recording.recordId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.recType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupInType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupMethod)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.startTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.endTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->programFlags)))
    goto out;
  ++i;
  if (!ReadField(program->recording.recGroup))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanFilters))
    goto out;
  ++i;
  if (!ReadField(program->seriesId))
    goto out;
  ++i;
  if (!ReadField(program->programId))
    goto out;
  ++i;
  if (!ReadField(program->inetref))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->lastModified = (time_t)tmpi;
  ++i;
  if (!ReadField(program->stars))
    goto out;
  ++i;
  if (!ReadField(field) || str2time(field.c_str(), &(program->airdate)))
    goto out;
  ++i;
  if (!ReadField(program->recording.playGroup))
    goto out;
  ++i;
  if (!ReadField(field)) // recpriority2
    goto out;
  ++i;
  if (!ReadField(field)) // parentid
    goto out;
  ++i;
  if (!ReadField(program->recording.storageGroup))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->audioProps)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->videoProps)))
    goto out;
  return program;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed (%d) buf='%s'\n", __FUNCTION__, i, field.c_str());
  program.reset();
  return program;
}

ProgramPtr ProtoBase::RcvProgramInfo76()
{
  int64_t tmpi;
  std::string field;
  ProgramPtr program(new Program());
  int i = 0;

  ++i;
  if (!ReadField(program->title))
    goto out;
  ++i;
  if (!ReadField(program->subTitle))
    goto out;
  ++i;
  if (!ReadField(program->description))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->season)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->episode)))
    goto out;
  ++i;
  if (!ReadField(field)) // syndicated episode
    goto out;
  ++i;
  if (!ReadField(program->category))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.chanId)))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanNum))
    goto out;
  ++i;
  if (!ReadField(program->channel.callSign))
    goto out;
  ++i;
  if (!ReadField(program->channel.channelName))
    goto out;
  ++i;
  if (!ReadField(program->fileName))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &(program->fileSize)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->startTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->endTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field)) // findid
    goto out;
  ++i;
  if (!ReadField(program->hostName))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.sourceId)))
    goto out;
  ++i;
  if (!ReadField(field)) // cardid
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.inputId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int32(field.c_str(), &(program->recording.priority)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int8(field.c_str(), &(program->recording.status)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->recording.recordId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.recType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupInType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupMethod)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.startTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.endTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->programFlags)))
    goto out;
  ++i;
  if (!ReadField(program->recording.recGroup))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanFilters))
    goto out;
  ++i;
  if (!ReadField(program->seriesId))
    goto out;
  ++i;
  if (!ReadField(program->programId))
    goto out;
  ++i;
  if (!ReadField(program->inetref))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->lastModified = (time_t)tmpi;
  ++i;
  if (!ReadField(program->stars))
    goto out;
  ++i;
  if (!ReadField(field) || str2time(field.c_str(), &(program->airdate)))
    goto out;
  ++i;
  if (!ReadField(program->recording.playGroup))
    goto out;
  ++i;
  if (!ReadField(field)) // recpriority2
    goto out;
  ++i;
  if (!ReadField(field)) // parentid
    goto out;
  ++i;
  if (!ReadField(program->recording.storageGroup))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->audioProps)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->videoProps)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->subProps)))
    goto out;
  ++i;
  if (!ReadField(field)) // year
    goto out;
  ++i;
  if (!ReadField(field)) // part number
    goto out;
  ++i;
  if (!ReadField(field)) // part total
    goto out;
  return program;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed (%d) buf='%s'\n", __FUNCTION__, i, field.c_str());
  program.reset();
  return program;
}

ProgramPtr ProtoBase::RcvProgramInfo79()
{
  int64_t tmpi;
  std::string field;
  ProgramPtr program(new Program());
  int i = 0;

  ++i;
  if (!ReadField(program->title))
    goto out;
  ++i;
  if (!ReadField(program->subTitle))
    goto out;
  ++i;
  if (!ReadField(program->description))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->season)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->episode)))
    goto out;
  ++i;
  if (!ReadField(field)) // total episodes
    goto out;
  ++i;
  if (!ReadField(field)) // syndicated episode
    goto out;
  ++i;
  if (!ReadField(program->category))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.chanId)))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanNum))
    goto out;
  ++i;
  if (!ReadField(program->channel.callSign))
    goto out;
  ++i;
  if (!ReadField(program->channel.channelName))
    goto out;
  ++i;
  if (!ReadField(program->fileName))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &(program->fileSize)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->startTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->endTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field)) // findid
    goto out;
  ++i;
  if (!ReadField(program->hostName))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.sourceId)))
    goto out;
  ++i;
  if (!ReadField(field)) // cardid
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.inputId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int32(field.c_str(), &(program->recording.priority)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int8(field.c_str(), &(program->recording.status)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->recording.recordId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.recType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupInType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupMethod)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.startTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.endTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->programFlags)))
    goto out;
  ++i;
  if (!ReadField(program->recording.recGroup))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanFilters))
    goto out;
  ++i;
  if (!ReadField(program->seriesId))
    goto out;
  ++i;
  if (!ReadField(program->programId))
    goto out;
  ++i;
  if (!ReadField(program->inetref))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->lastModified = (time_t)tmpi;
  ++i;
  if (!ReadField(program->stars))
    goto out;
  ++i;
  if (!ReadField(field) || str2time(field.c_str(), &(program->airdate)))
    goto out;
  ++i;
  if (!ReadField(program->recording.playGroup))
    goto out;
  ++i;
  if (!ReadField(field)) // recpriority2
    goto out;
  ++i;
  if (!ReadField(field)) // parentid
    goto out;
  ++i;
  if (!ReadField(program->recording.storageGroup))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->audioProps)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->videoProps)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->subProps)))
    goto out;
  ++i;
  if (!ReadField(field)) // year
    goto out;
  ++i;
  if (!ReadField(field)) // part number
    goto out;
  ++i;
  if (!ReadField(field)) // part total
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->catType = CategoryTypeToString(m_protoVersion, CategoryTypeFromNum(m_protoVersion, (int)tmpi));
  return program;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed (%d) buf='%s'\n", __FUNCTION__, i, field.c_str());
  program.reset();
  return program;
}

ProgramPtr ProtoBase::RcvProgramInfo82()
{
  int64_t tmpi;
  std::string field;
  ProgramPtr program(new Program());
  int i = 0;

  ++i;
  if (!ReadField(program->title))
    goto out;
  ++i;
  if (!ReadField(program->subTitle))
    goto out;
  ++i;
  if (!ReadField(program->description))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->season)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->episode)))
    goto out;
  ++i;
  if (!ReadField(field)) // total episodes
    goto out;
  ++i;
  if (!ReadField(field)) // syndicated episode
    goto out;
  ++i;
  if (!ReadField(program->category))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.chanId)))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanNum))
    goto out;
  ++i;
  if (!ReadField(program->channel.callSign))
    goto out;
  ++i;
  if (!ReadField(program->channel.channelName))
    goto out;
  ++i;
  if (!ReadField(program->fileName))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &(program->fileSize)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->startTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->endTime = (time_t)tmpi;
  ++i;
  if (!ReadField(field)) // findid
    goto out;
  ++i;
  if (!ReadField(program->hostName))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.sourceId)))
    goto out;
  ++i;
  if (!ReadField(field)) // cardid
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->channel.inputId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int32(field.c_str(), &(program->recording.priority)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int8(field.c_str(), &(program->recording.status)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->recording.recordId)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.recType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupInType)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint8(field.c_str(), &(program->recording.dupMethod)))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.startTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->recording.endTs = (time_t)tmpi;
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->programFlags)))
    goto out;
  ++i;
  if (!ReadField(program->recording.recGroup))
    goto out;
  ++i;
  if (!ReadField(program->channel.chanFilters))
    goto out;
  ++i;
  if (!ReadField(program->seriesId))
    goto out;
  ++i;
  if (!ReadField(program->programId))
    goto out;
  ++i;
  if (!ReadField(program->inetref))
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->lastModified = (time_t)tmpi;
  ++i;
  if (!ReadField(program->stars))
    goto out;
  ++i;
  if (!ReadField(field) || str2time(field.c_str(), &(program->airdate)))
    goto out;
  ++i;
  if (!ReadField(program->recording.playGroup))
    goto out;
  ++i;
  if (!ReadField(field)) // recpriority2
    goto out;
  ++i;
  if (!ReadField(field)) // parentid
    goto out;
  ++i;
  if (!ReadField(program->recording.storageGroup))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->audioProps)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->videoProps)))
    goto out;
  ++i;
  if (!ReadField(field) || str2uint16(field.c_str(), &(program->subProps)))
    goto out;
  ++i;
  if (!ReadField(field)) // year
    goto out;
  ++i;
  if (!ReadField(field)) // part number
    goto out;
  ++i;
  if (!ReadField(field)) // part total
    goto out;
  ++i;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  program->catType = CategoryTypeToString(m_protoVersion, CategoryTypeFromNum(m_protoVersion, (int)tmpi));
  ++i;
  if (!ReadField(field) || str2uint32(field.c_str(), &(program->recording.recordedId)))
    goto out;
  return program;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed (%d) buf='%s'\n", __FUNCTION__, i, field.c_str());
  program.reset();
  return program;
}

void ProtoBase::MakeProgramInfo75(Program& program, std::string& msg)
{
  char buf[32];
  msg.clear();

  msg.append(program.title).append(PROTO_STR_SEPARATOR);
  msg.append(program.subTitle).append(PROTO_STR_SEPARATOR);
  msg.append(program.description).append(PROTO_STR_SEPARATOR);
  uint16str(program.season, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.episode, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.category).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.chanId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanNum).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.callSign).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.channelName).append(PROTO_STR_SEPARATOR);
  msg.append(program.fileName).append(PROTO_STR_SEPARATOR);
  int64str(program.fileSize, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.startTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.endTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // findid
  msg.append(program.hostName).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.sourceId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // cardid
  uint32str(program.channel.inputId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int32str(program.recording.priority, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int8str(program.recording.status, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.recording.recordId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.recType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupInType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupMethod, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.startTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.endTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.programFlags, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.recGroup).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanFilters).append(PROTO_STR_SEPARATOR);
  msg.append(program.seriesId).append(PROTO_STR_SEPARATOR);
  msg.append(program.programId).append(PROTO_STR_SEPARATOR);
  msg.append(program.inetref).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.lastModified, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.stars).append(PROTO_STR_SEPARATOR);
  time2isodate(program.airdate, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.playGroup).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // recpriority2
  msg.append("0").append(PROTO_STR_SEPARATOR); // parentid
  msg.append(program.recording.storageGroup).append(PROTO_STR_SEPARATOR);
  uint16str(program.audioProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.videoProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.subProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0"); // year
}

void ProtoBase::MakeProgramInfo76(Program& program, std::string& msg)
{
  char buf[32];
  msg.clear();

  msg.append(program.title).append(PROTO_STR_SEPARATOR);
  msg.append(program.subTitle).append(PROTO_STR_SEPARATOR);
  msg.append(program.description).append(PROTO_STR_SEPARATOR);
  uint16str(program.season, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.episode, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(PROTO_STR_SEPARATOR); // syndicated episode
  msg.append(program.category).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.chanId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanNum).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.callSign).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.channelName).append(PROTO_STR_SEPARATOR);
  msg.append(program.fileName).append(PROTO_STR_SEPARATOR);
  int64str(program.fileSize, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.startTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.endTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // findid
  msg.append(program.hostName).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.sourceId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // cardid
  uint32str(program.channel.inputId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int32str(program.recording.priority, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int8str(program.recording.status, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.recording.recordId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.recType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupInType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupMethod, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.startTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.endTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.programFlags, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.recGroup).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanFilters).append(PROTO_STR_SEPARATOR);
  msg.append(program.seriesId).append(PROTO_STR_SEPARATOR);
  msg.append(program.programId).append(PROTO_STR_SEPARATOR);
  msg.append(program.inetref).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.lastModified, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.stars).append(PROTO_STR_SEPARATOR);
  time2isodate(program.airdate, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.playGroup).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // recpriority2
  msg.append("0").append(PROTO_STR_SEPARATOR); // parentid
  msg.append(program.recording.storageGroup).append(PROTO_STR_SEPARATOR);
  uint16str(program.audioProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.videoProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.subProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // year
  msg.append("0").append(PROTO_STR_SEPARATOR); // part number
  msg.append("0"); // part total
}

void ProtoBase::MakeProgramInfo79(Program& program, std::string& msg)
{
  char buf[32];
  msg.clear();

  msg.append(program.title).append(PROTO_STR_SEPARATOR);
  msg.append(program.subTitle).append(PROTO_STR_SEPARATOR);
  msg.append(program.description).append(PROTO_STR_SEPARATOR);
  uint16str(program.season, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.episode, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // total episodes
  msg.append(PROTO_STR_SEPARATOR); // syndicated episode
  msg.append(program.category).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.chanId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanNum).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.callSign).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.channelName).append(PROTO_STR_SEPARATOR);
  msg.append(program.fileName).append(PROTO_STR_SEPARATOR);
  int64str(program.fileSize, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.startTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.endTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // findid
  msg.append(program.hostName).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.sourceId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // cardid
  uint32str(program.channel.inputId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int32str(program.recording.priority, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int8str(program.recording.status, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.recording.recordId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.recType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupInType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupMethod, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.startTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.endTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.programFlags, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.recGroup).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanFilters).append(PROTO_STR_SEPARATOR);
  msg.append(program.seriesId).append(PROTO_STR_SEPARATOR);
  msg.append(program.programId).append(PROTO_STR_SEPARATOR);
  msg.append(program.inetref).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.lastModified, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.stars).append(PROTO_STR_SEPARATOR);
  time2isodate(program.airdate, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.playGroup).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // recpriority2
  msg.append("0").append(PROTO_STR_SEPARATOR); // parentid
  msg.append(program.recording.storageGroup).append(PROTO_STR_SEPARATOR);
  uint16str(program.audioProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.videoProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.subProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // year
  msg.append("0").append(PROTO_STR_SEPARATOR); // part number
  msg.append("0").append(PROTO_STR_SEPARATOR); // part total
  uint8str((uint8_t)CategoryTypeToNum(m_protoVersion, CategoryTypeFromString(m_protoVersion, program.catType)), buf);
  msg.append(buf);
}

void ProtoBase::MakeProgramInfo82(Program& program, std::string& msg)
{
  char buf[32];
  msg.clear();

  msg.append(program.title).append(PROTO_STR_SEPARATOR);
  msg.append(program.subTitle).append(PROTO_STR_SEPARATOR);
  msg.append(program.description).append(PROTO_STR_SEPARATOR);
  uint16str(program.season, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.episode, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // total episodes
  msg.append(PROTO_STR_SEPARATOR); // syndicated episode
  msg.append(program.category).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.chanId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanNum).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.callSign).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.channelName).append(PROTO_STR_SEPARATOR);
  msg.append(program.fileName).append(PROTO_STR_SEPARATOR);
  int64str(program.fileSize, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.startTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.endTime, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // findid
  msg.append(program.hostName).append(PROTO_STR_SEPARATOR);
  uint32str(program.channel.sourceId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // cardid
  uint32str(program.channel.inputId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int32str(program.recording.priority, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int8str(program.recording.status, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.recording.recordId, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.recType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupInType, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint8str(program.recording.dupMethod, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.startTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.recording.endTs, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.programFlags, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.recGroup).append(PROTO_STR_SEPARATOR);
  msg.append(program.channel.chanFilters).append(PROTO_STR_SEPARATOR);
  msg.append(program.seriesId).append(PROTO_STR_SEPARATOR);
  msg.append(program.programId).append(PROTO_STR_SEPARATOR);
  msg.append(program.inetref).append(PROTO_STR_SEPARATOR);
  int64str((int64_t)program.lastModified, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.stars).append(PROTO_STR_SEPARATOR);
  time2isodate(program.airdate, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append(program.recording.playGroup).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // recpriority2
  msg.append("0").append(PROTO_STR_SEPARATOR); // parentid
  msg.append(program.recording.storageGroup).append(PROTO_STR_SEPARATOR);
  uint16str(program.audioProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.videoProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint16str(program.subProps, buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  msg.append("0").append(PROTO_STR_SEPARATOR); // year
  msg.append("0").append(PROTO_STR_SEPARATOR); // part number
  msg.append("0").append(PROTO_STR_SEPARATOR); // part total
  uint8str((uint8_t)CategoryTypeToNum(m_protoVersion, CategoryTypeFromString(m_protoVersion, program.catType)), buf);
  msg.append(buf).append(PROTO_STR_SEPARATOR);
  uint32str(program.recording.recordedId, buf);
  msg.append(buf);
}
