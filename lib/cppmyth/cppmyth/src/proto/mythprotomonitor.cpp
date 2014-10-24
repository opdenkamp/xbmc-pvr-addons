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

#include "mythprotomonitor.h"
#include "mythprotorecorder.h"
#include "../mythdebug.h"
#include "../private/builtin.h"
#include "../private/mythsocket.h"
#include "../private/platform/threads/mutex.h"

#include <limits>
#include <cstdio>

using namespace Myth;

///////////////////////////////////////////////////////////////////////////////
////
//// Protocol connection to monitor DVR
////

ProtoMonitor::ProtoMonitor(const std::string& server, unsigned port)
: ProtoBase(server, port)
, m_blockShutdown(false)
{
}

ProtoMonitor::ProtoMonitor(const std::string& server, unsigned port, bool blockShutdown)
: ProtoBase(server, port)
, m_blockShutdown(blockShutdown)
{
}

bool ProtoMonitor::Open()
{
  bool ok = false;

  if (!OpenConnection(PROTO_MONITOR_RCVBUF))
    return false;

  switch (m_protoVersion)
  {
    case 75:
    default:
      ok = Announce75();
  }
  if (ok)
  {
    if (m_blockShutdown)
      BlockShutdown();
    return true;
  }
  Close();
  return false;
}

void ProtoMonitor::Close()
{
  ProtoBase::Close();
  // Clean hanging and disable retry
  m_tainted = m_hang = false;
}

bool ProtoMonitor::IsOpen()
{
  // Try reconnect
  if (m_hang)
    return ProtoMonitor::Open();
  return ProtoBase::IsOpen();
}

bool ProtoMonitor::Announce75()
{
  PLATFORM::CLockObject lock(*m_mutex);

  std::string cmd("ANN Monitor ");
  cmd.append(m_socket->GetMyHostName()).append(" 0");
  if (!SendCommand(cmd.c_str()))
    return false;

  std::string field;
  if (!ReadField(field) || !IsMessageOK(field))
    goto out;
  return true;

out:
  FlushMessage();
  return false;
}

ProtoRecorderPtr ProtoMonitor::GetNextFreeRecorder75(int rnum)
{
  char buf[32];
  std::string field;
  ProtoRecorderPtr recorder;
  int nextnum = -1;
  std::string hostname;
  uint16_t port;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return recorder;
  std::string cmd("GET_NEXT_FREE_RECORDER");
  cmd.append(PROTO_STR_SEPARATOR);
  int32str((int32_t)rnum, buf);
  cmd.append(buf);

  if (!SendCommand(cmd.c_str()))
    return recorder;

  if (!ReadField(field) || str2int32(field.c_str(), &nextnum) || nextnum < 1)
    goto out;
  if (!ReadField(hostname) || hostname == "nohost")
    goto out;
  if (!ReadField(field) || str2uint16(field.c_str(), &port))
    goto out;
  FlushMessage();
  DBG(MYTH_DBG_DEBUG, "%s: open recorder %d (%s:%u)\n", __FUNCTION__, (int)nextnum, hostname.c_str(), (unsigned)port);
  recorder.reset(new ProtoRecorder(nextnum, hostname, port));
  return recorder;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return recorder;
}

ProtoRecorderPtr ProtoMonitor::GetRecorderFromNum75(int rnum)
{
  char buf[32];
  std::string field;
  ProtoRecorderPtr recorder;
  std::string hostname;
  uint16_t port;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return recorder;
  std::string cmd("GET_RECORDER_FROM_NUM");
  cmd.append(PROTO_STR_SEPARATOR);
  int32str((int32_t)rnum, buf);
  cmd.append(buf);

  if (!SendCommand(cmd.c_str()))
    return recorder;

  if (!ReadField(hostname) || hostname == "nohost")
    goto out;
  if (!ReadField(field) || str2uint16(field.c_str(), &port))
    goto out;
  FlushMessage();
  DBG(MYTH_DBG_DEBUG, "%s: open recorder %d (%s:%u)\n", __FUNCTION__, (int)rnum, hostname.c_str(), (unsigned)port);
  recorder.reset(new ProtoRecorder(rnum, hostname, port));
  return recorder;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return recorder;
}

bool ProtoMonitor::QueryFreeSpaceSummary75(int64_t *total, int64_t *used)
{
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("QUERY_FREE_SPACE_SUMMARY");

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || str2int64(field.c_str(), total))
    goto out;
  if (!ReadField(field) || str2int64(field.c_str(), used))
    goto out;
  FlushMessage();
  return true;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

std::string ProtoMonitor::GetSetting75(const std::string& hostname, const std::string& setting)
{
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return field;
  std::string cmd("QUERY_SETTING ");
  cmd.append(hostname).append(" ").append(setting);

  if (!SendCommand(cmd.c_str()))
    return field;

  if (!ReadField(field))
    goto out;
  FlushMessage();
  return field;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  field.clear();
  return field;
}

bool ProtoMonitor::SetSetting75(const std::string& hostname, const std::string& setting, const std::string& value)
{
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("SET_SETTING ");
  cmd.append(hostname).append(" ").append(setting).append(" ").append(value);

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || !IsMessageOK(field))
    goto out;
  FlushMessage();
  return true;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

bool ProtoMonitor::QueryGenpixmap75(const Program& program)
{
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("QUERY_GENPIXMAP2");
  cmd.append(PROTO_STR_SEPARATOR).append("do_not_care").append(PROTO_STR_SEPARATOR);
  MakeProgramInfo(program, field);
  cmd.append(field);

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || !IsMessageOK(field))
    goto out;
  FlushMessage();
  return true;
out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

bool ProtoMonitor::DeleteRecording75(const Program& program, bool force, bool forget)
{
  char buf[32];
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("DELETE_RECORDING ");
  uint32str(program.channel.chanId, buf);
  cmd.append(buf).append(" ");
  time2iso8601utc(program.recording.startTs, buf);
  cmd.append(buf).append(" ");
  if (force)
    cmd.append("FORCE ");
  else
    cmd.append("NO_FORCE ");
  if (forget)
    cmd.append("FORGET");
  else
    cmd.append("NO_FORGET");

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field))
    goto out;
  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, program.fileName.c_str());
  return true;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

bool ProtoMonitor::UndeleteRecording75(const Program& program)
{
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("UNDELETE_RECORDING");
  cmd.append(PROTO_STR_SEPARATOR);
  MakeProgramInfo(program, field);
  cmd.append(field);

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || field != "0")
    goto out;
  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, program.fileName.c_str());
  return true;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

bool ProtoMonitor::StopRecording75(const Program& program)
{
  std::string field;
  int32_t num;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("STOP_RECORDING");
  cmd.append(PROTO_STR_SEPARATOR);
  MakeProgramInfo(program, field);
  cmd.append(field);

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || str2int32(field.c_str(), &num) || num < 0)
    goto out;
  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, program.fileName.c_str());
  return true;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

bool ProtoMonitor::CancelNextRecording75(int rnum, bool cancel)
{
  char buf[32];
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("QUERY_RECORDER ");
  int32str(rnum, buf);
  cmd.append(buf).append(PROTO_STR_SEPARATOR);
  cmd.append("CANCEL_NEXT_RECORDING").append(PROTO_STR_SEPARATOR);
  cmd.append((cancel ? "1" : "0"));

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || !IsMessageOK(field))
    goto out;
  DBG(MYTH_DBG_DEBUG, "%s: succeeded\n", __FUNCTION__);
  return true;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

StorageGroupFilePtr ProtoMonitor::QuerySGFile75(const std::string& hostname, const std::string& sgname, const std::string& filename)
{
  std::string field;
  int64_t tmpi;
  StorageGroupFilePtr sgfile;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return sgfile;
  std::string cmd("QUERY_SG_FILEQUERY");
  cmd.append(PROTO_STR_SEPARATOR);
  cmd.append(hostname).append(PROTO_STR_SEPARATOR);
  cmd.append(sgname).append(PROTO_STR_SEPARATOR);
  cmd.append(filename);

  if (!SendCommand(cmd.c_str()))
    return sgfile;
  sgfile.reset(new StorageGroupFile());

  if (!ReadField(sgfile->fileName))
    goto out;
  if (!ReadField(field) || str2int64(field.c_str(), &tmpi))
    goto out;
  sgfile->lastModified = (time_t)tmpi;
  if (!ReadField(field) || str2int64(field.c_str(), &(sgfile->size)))
    goto out;
  sgfile->hostName = hostname;
  sgfile->storageGroup = sgname;

  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, sgfile->fileName.c_str());
  return sgfile;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  sgfile.reset();
  return sgfile;
}

MarkListPtr ProtoMonitor::GetCutList75(const Program& program, int unit)
{
  char buf[32];
  std::string field;
  int32_t nb;
  MarkListPtr list(new MarkList);

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen() || unit > 0)
    return list;
  std::string cmd("QUERY_CUTLIST ");
  uint32str(program.channel.chanId, buf);
  cmd.append(buf).append(" ");
  int64str(program.recording.startTs, buf);
  cmd.append(buf);

  if (!SendCommand(cmd.c_str()))
    return list;

  if (!ReadField(field) || str2int32(field.c_str(), &nb))
    goto out;
  if (nb > 0)
  {
    list->reserve(nb);
    do
    {
      MarkPtr mark = MarkPtr(new Mark());
      if (!ReadField(field) || str2int8(field.c_str(), (int8_t*)&(mark->markType)))
        break;
      if (!ReadField(field) || str2int64(field.c_str(), &(mark->markValue)))
        break;
      list->push_back(mark);
    }
    while (--nb > 0);
  }
  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, program.fileName.c_str());
  return list;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return list;
}

MarkListPtr ProtoMonitor::GetCutList__(const Program& program, int unit)
{
  char buf[32];
  std::string field;
  int32_t nb;
  MarkListPtr list(new MarkList);

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return list;
  std::string cmd("QUERY_CUTLIST ");
  uint32str(program.channel.chanId, buf);
  cmd.append(buf).append(" ");
  int64str(program.recording.startTs, buf);
  cmd.append(buf);
  if (unit == 1)
    cmd.append(" Position");
  else if (unit == 2)
    cmd.append(" Duration");

  if (!SendCommand(cmd.c_str()))
    return list;

  if (!ReadField(field) || str2int32(field.c_str(), &nb))
    goto out;
  if (nb > 0)
  {
    list->reserve(nb);
    do
    {
      MarkPtr mark = MarkPtr(new Mark());
      if (!ReadField(field) || str2int8(field.c_str(), (int8_t*)&(mark->markType)))
        break;
      if (!ReadField(field) || str2int64(field.c_str(), &(mark->markValue)))
        break;
      list->push_back(mark);
    }
    while (--nb > 0);
  }
  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, program.fileName.c_str());
  return list;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return list;
}

MarkListPtr ProtoMonitor::GetCommBreakList75(const Program& program, int unit)
{
  char buf[32];
  std::string field;
  int32_t nb;
  MarkListPtr list(new MarkList);

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen() || unit > 0)
    return list;
  std::string cmd("QUERY_COMMBREAK ");
  uint32str(program.channel.chanId, buf);
  cmd.append(buf).append(" ");
  int64str(program.recording.startTs, buf);
  cmd.append(buf);

  if (!SendCommand(cmd.c_str()))
    return list;

  if (!ReadField(field) || str2int32(field.c_str(), &nb))
    goto out;
  if (nb > 0)
  {
    list->reserve(nb);
    do
    {
      MarkPtr mark = MarkPtr(new Mark());
      if (!ReadField(field) || str2int8(field.c_str(), (int8_t*)&(mark->markType)))
        break;
      if (!ReadField(field) || str2int64(field.c_str(), &(mark->markValue)))
        break;
      list->push_back(mark);
    }
    while (--nb > 0);
  }
  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, program.fileName.c_str());
  return list;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return list;
}

MarkListPtr ProtoMonitor::GetCommBreakList__(const Program& program, int unit)
{
  char buf[32];
  std::string field;
  int32_t nb;
  MarkListPtr list(new MarkList);

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return list;
  std::string cmd("QUERY_COMMBREAK ");
  uint32str(program.channel.chanId, buf);
  cmd.append(buf).append(" ");
  int64str(program.recording.startTs, buf);
  cmd.append(buf);
  if (unit == 1)
    cmd.append(" Position");
  else if (unit == 2)
    cmd.append(" Duration");

  if (!SendCommand(cmd.c_str()))
    return list;

  if (!ReadField(field) || str2int32(field.c_str(), &nb))
    goto out;
  if (nb > 0)
  {
    list->reserve(nb);
    do
    {
      MarkPtr mark = MarkPtr(new Mark());
      if (!ReadField(field) || str2int8(field.c_str(), (int8_t*)&(mark->markType)))
        break;
      if (!ReadField(field) || str2int64(field.c_str(), &(mark->markValue)))
        break;
      list->push_back(mark);
    }
    while (--nb > 0);
  }
  DBG(MYTH_DBG_DEBUG, "%s: succeeded (%s)\n", __FUNCTION__, program.fileName.c_str());
  return list;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return list;
}

bool ProtoMonitor::BlockShutdown75()
{
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("BLOCK_SHUTDOWN");

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || !IsMessageOK(field))
    goto out;
  DBG(MYTH_DBG_DEBUG, "%s: succeeded\n", __FUNCTION__);
  return true;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}

bool ProtoMonitor::AllowShutdown75()
{
  std::string field;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("ALLOW_SHUTDOWN");

  if (!SendCommand(cmd.c_str()))
    return false;

  if (!ReadField(field) || !IsMessageOK(field))
    goto out;
  DBG(MYTH_DBG_DEBUG, "%s: succeeded\n", __FUNCTION__);
  return true;
  out:
  DBG(MYTH_DBG_ERROR, "%s: failed\n", __FUNCTION__);
  FlushMessage();
  return false;
}
