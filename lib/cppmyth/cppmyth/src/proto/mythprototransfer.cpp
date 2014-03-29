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

#include "mythprototransfer.h"
#include "../mythdebug.h"
#include "../private/builtin.h"
#include "../private/mythsocket.h"
#include "../private/platform/threads/mutex.h"

#include <limits>
#include <cstdio>

using namespace Myth;

///////////////////////////////////////////////////////////////////////////////
////
//// Protocol connection to transfer file
////

ProtoTransfer::ProtoTransfer(const std::string& server, unsigned port)
: ProtoBase(server, port)
, fileSize(0)
, filePosition(0)
, m_fileId(0)
, m_pathName()
, m_storageGroupName()
{
}

void ProtoTransfer::Close()
{
  ProtoBase::Close();
  filePosition = fileSize = 0;
  m_fileId = 0;
}

bool ProtoTransfer::Open(const std::string& pathname, const std::string& sgname)
{
  bool ok = false;

  if (!OpenConnection(PROTO_TRANSFER_RCVBUF))
    return false;

  if (m_protoVersion >= 75)
    ok = Announce75(pathname, sgname);

  if (ok)
  {
    m_pathName = pathname;
    m_storageGroupName = sgname;
    return true;
  }
  this->Close();
  return false;
}

bool ProtoTransfer::Open()
{
  return false;
}

bool ProtoTransfer::Announce75(const std::string& pathname, const std::string& sgname)
{
  PLATFORM::CLockObject lock(*m_mutex);
  filePosition = fileSize = 0;
  std::string cmd("ANN FileTransfer ");
  cmd.append(m_socket->GetMyHostName());
  cmd.append(" 0 0 1000" PROTO_STR_SEPARATOR);
  cmd.append(pathname).append(PROTO_STR_SEPARATOR);
  cmd.append(sgname);
  if (!SendCommand(cmd.c_str()))
    return false;

  std::string field;
  if (!ReadField(field) || !IsMessageOK(field))
    goto out;
  if (!ReadField(field) || 0 != str2uint32(field.c_str(), &m_fileId))
    goto out;
  if (!ReadField(field) || 0 != str2int64(field.c_str(), &fileSize))
    goto out;
  return true;

out:
  FlushMessage();
  return false;
}

uint32_t ProtoTransfer::GetFileId() const
{
  return m_fileId;
}

std::string ProtoTransfer::GetPathName() const
{
  return m_pathName;
}

std::string ProtoTransfer::GetStorageGroupName() const
{
  return m_storageGroupName;
}

size_t ProtoTransfer::ReadData(void *buffer, size_t n)
{
  return m_socket->ReadResponse(buffer, n);
}
