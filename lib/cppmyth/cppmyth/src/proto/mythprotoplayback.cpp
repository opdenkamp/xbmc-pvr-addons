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

#include "mythprotoplayback.h"
#include "../mythdebug.h"
#include "../private/builtin.h"
#include "../private/mythsocket.h"
#include "../private/platform/threads/mutex.h"

#include <limits>
#include <cstdio>

using namespace Myth;

///////////////////////////////////////////////////////////////////////////////
////
//// Protocol connection to control playback
////

ProtoPlayback::ProtoPlayback(const std::string& server, unsigned port)
: ProtoBase(server, port)
{
}

bool ProtoPlayback::Open()
{
  bool ok = false;

  if (!OpenConnection(PROTO_PLAYBACK_RCVBUF))
    return false;

  if (m_protoVersion >= 75)
    ok = Announce75();

  if (ok)
    return true;
  this->Close();
  return false;
}

bool ProtoPlayback::Announce75()
{
  PLATFORM::CLockObject lock(*m_mutex);

  std::string cmd("ANN Playback ");
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

void ProtoPlayback::TransferDone75(ProtoTransfer& transfer)
{
  char buf[32];

  PLATFORM::CLockObject lock(*m_mutex);
  if (!transfer.IsOpen())
    return;
  std::string cmd("QUERY_FILETRANSFER ");
  uint32str(transfer.GetFileId(), buf);
  cmd.append(buf).append(PROTO_STR_SEPARATOR).append("DONE");
  if (SendCommand(cmd.c_str()))
  {
    std::string field;
    if (!ReadField(field) || !IsMessageOK(field))
      FlushMessage();
  }
}

bool ProtoPlayback::TransferIsOpen75(ProtoTransfer& transfer)
{
  char buf[32];
  std::string field;
  int8_t status = 0;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!IsOpen())
    return false;
  std::string cmd("QUERY_FILETRANSFER ");
  uint32str(transfer.GetFileId(), buf);
  cmd.append(buf);
  cmd.append(PROTO_STR_SEPARATOR);
  cmd.append("IS_OPEN");

  if (!SendCommand(cmd.c_str()))
    return false;
  if (!ReadField(field) || 0 != str2int8(field.c_str(), &status))
  {
      FlushMessage();
      return false;
  }
  if (status == 0)
    return false;
  return true;
}

int ProtoPlayback::TransferRequestBlock75(ProtoTransfer& transfer, unsigned n)
{
  char buf[32];
  int32_t rlen = 0;
  std::string field;

  // Max size is RCVBUF size
  if (n > PROTO_TRANSFER_RCVBUF)
    n = PROTO_TRANSFER_RCVBUF;

  PLATFORM::CLockObject lock(*m_mutex);
  if (!transfer.IsOpen())
    return -1;
  std::string cmd("QUERY_FILETRANSFER ");
  uint32str(transfer.GetFileId(), buf);
  cmd.append(buf);
  cmd.append(PROTO_STR_SEPARATOR);
  cmd.append("REQUEST_BLOCK");
  cmd.append(PROTO_STR_SEPARATOR);
  uint32str(n, buf);
  cmd.append(buf);

  if (!SendCommand(cmd.c_str()))
    return -1;
  if (!ReadField(field) || 0 != str2int32(field.c_str(), &rlen))
  {
      FlushMessage();
      return -1;
  }
  transfer.filePosition += rlen;
  return (int)rlen;
}

int64_t ProtoPlayback::TransferSeek75(ProtoTransfer& transfer, int64_t offset, WHENCE_t whence)
{
  char buf[32];
  int64_t position = 0;
  std::string field;

  // Check offset
  switch (whence)
  {
    case WHENCE_CUR:
      if (offset == 0)
        return transfer.filePosition;
      position = transfer.filePosition + offset;
      if (position < 0 || position > transfer.fileSize)
        return -1;
      break;
    case WHENCE_SET:
      if (offset == transfer.filePosition)
        return transfer.filePosition;
      if (offset < 0 || offset > transfer.fileSize)
        return -1;
      break;
    case WHENCE_END:
      position = transfer.fileSize - offset;
      if (position < 0 || position > transfer.fileSize)
        return -1;
      break;
    default:
      return -1;
  }

  PLATFORM::CLockObject lock(*m_mutex);
  if (!transfer.IsOpen())
    return -1;
  std::string cmd("QUERY_FILETRANSFER ");
  uint32str(transfer.GetFileId(), buf);
  cmd.append(buf);
  cmd.append(PROTO_STR_SEPARATOR);
  cmd.append("SEEK");
  cmd.append(PROTO_STR_SEPARATOR);
  int64str(offset, buf);
  cmd.append(buf);
  cmd.append(PROTO_STR_SEPARATOR);
  int8str(whence, buf);
  cmd.append(buf);
  cmd.append(PROTO_STR_SEPARATOR);
  int64str(transfer.filePosition, buf);
  cmd.append(buf);

  if (!SendCommand(cmd.c_str()))
    return -1;
  if (!ReadField(field) || 0 != str2int64(field.c_str(), &position))
  {
      FlushMessage();
      return -1;
  }
  transfer.filePosition = position;
  return position;
}
