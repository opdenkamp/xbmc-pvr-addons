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

#include "mythrecordingplayback.h"
#include "mythdebug.h"
#include "private/builtin.h"
#include "private/platform/threads/mutex.h"

#include <limits>
#include <cstdio>

using namespace Myth;

///////////////////////////////////////////////////////////////////////////////
////
//// Protocol connection to control playback
////

RecordingPlayback::RecordingPlayback(EventHandler& handler)
: ProtoPlayback(handler.GetServer(), handler.GetPort()), EventSubscriber()
, m_eventHandler(handler)
, m_eventSubscriberId(0)
, m_transfer(NULL)
, m_recording(NULL)
, m_readAhead(false)
{
  m_eventSubscriberId = m_eventHandler.CreateSubscription(this);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_UPDATE_FILE_SIZE);
  Open();
}

RecordingPlayback::RecordingPlayback(const std::string& server, unsigned port)
: ProtoPlayback(server, port), EventSubscriber()
, m_eventHandler(server, port)
, m_eventSubscriberId(0)
, m_transfer(NULL)
, m_recording(NULL)
, m_readAhead(false)
{
  // Private handler will be stopped and closed by destructor.
  m_eventSubscriberId = m_eventHandler.CreateSubscription(this);
  m_eventHandler.SubscribeForEvent(m_eventSubscriberId, EVENT_UPDATE_FILE_SIZE);
  Open();
}

RecordingPlayback::~RecordingPlayback()
{
  Close();
  if (m_eventSubscriberId)
    m_eventHandler.RevokeSubscription(m_eventSubscriberId);
}

bool RecordingPlayback::Open()
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  if (ProtoPlayback::IsOpen())
    return true;
  if (ProtoPlayback::Open())
  {
    if (!m_eventHandler.IsRunning())
      m_eventHandler.Start();
    return true;
  }
  return false;
}

void RecordingPlayback::Close()
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  CloseTransfer();
  ProtoPlayback::Close();
}

bool RecordingPlayback::OpenTransfer(ProgramPtr recording)
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  if (!ProtoPlayback::IsOpen())
    return false;
  CloseTransfer();
  if (recording)
  {
    m_transfer.reset(new ProtoTransfer(m_server, m_port, recording->fileName, recording->recording.storageGroup));
    if (m_transfer->Open())
    {
      m_recording.swap(recording);
      m_recording->fileSize = m_transfer->fileSize;
      return true;
    }
    m_transfer.reset();
  }
  return false;
}

void RecordingPlayback::CloseTransfer()
{
  // Begin critical section
  PLATFORM::CLockObject lock(*m_mutex);
  m_recording.reset();
  if (m_transfer)
  {
    TransferDone(*m_transfer);
    m_transfer->Close();
    m_transfer.reset();
  }
}

bool RecordingPlayback::TransferIsOpen()
{
  ProtoTransferPtr transfer(m_transfer);
  if (transfer)
    return ProtoPlayback::TransferIsOpen(*transfer);
  return false;
}

int64_t RecordingPlayback::GetSize() const
{
  ProtoTransferPtr transfer(m_transfer);
  if (transfer)
    return transfer->fileSize;
  return 0;
}

int RecordingPlayback::Read(void *buffer, unsigned n)
{
  ProtoTransferPtr transfer(m_transfer);
  if (transfer)
  {
    if (!m_readAhead)
    {
      int64_t s = transfer->fileSize - transfer->filePosition; // Acceptable block size
      if (s > 0)
      {
        if (s < (int64_t)n)
          n = (unsigned)s;
        // Request block data from transfer socket
        return TransferRequestBlock(*transfer, buffer, n);
      }
      return 0;
    }
    else
    {
      // Request block data from transfer socket
      return TransferRequestBlock(*transfer, buffer, n);
    }
  }
  return -1;
}

int64_t RecordingPlayback::Seek(int64_t offset, WHENCE_t whence)
{
  ProtoTransferPtr transfer(m_transfer);
  if (transfer)
    return TransferSeek(*transfer, offset, whence);
  return -1;
}

int64_t RecordingPlayback::GetPosition() const
{
  ProtoTransferPtr transfer(m_transfer);
  if (transfer)
    return transfer->filePosition;
  return 0;
}

void RecordingPlayback::HandleBackendMessage(const EventMessage& msg)
{
  // First of all i hold shared resources using copies
  ProgramPtr recording(m_recording);
  ProtoTransferPtr transfer(m_transfer);
  switch (msg.event)
  {
    case EVENT_UPDATE_FILE_SIZE:
      if (msg.subject.size() >= 4 && recording && transfer)
      {
        uint32_t chanid;
        time_t startts;
        if (str2uint32(msg.subject[1].c_str(), &chanid) || str2time(msg.subject[2].c_str(), &startts))
          break;
        if (recording->channel.chanId == chanid && recording->recording.startTs == startts)
        {
          int64_t newsize;
          if (0 == str2int64(msg.subject[3].c_str(), &newsize))
          {
            // The file grows. Allow reading ahead
            m_readAhead = true;
            recording->fileSize = transfer->fileSize = newsize;
            DBG(MYTH_DBG_DEBUG, "%s: (%d) %s %" PRIi64 "\n", __FUNCTION__,
                    msg.event, recording->fileName.c_str(), newsize);
          }
        }
      }
      break;
    //case EVENT_HANDLER_STATUS:
    //  if (msg.subject[0] == EVENTHANDLER_DISCONNECTED)
    //    closeTransfer();
    //  break;
    default:
      break;
  }
}
