/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2007 Chris Tallon
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 * This code is taken from VOMP for VDR plugin.
 */

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include <asm/byteorder.h>

#include "responsepacket.h"
#include "vnsicommand.h"
#include "config.h"

/* Packet format for an RR channel response:

4 bytes = channel ID = 1 (request/response channel)
4 bytes = request ID (from serialNumber)
4 bytes = length of the rest of the packet
? bytes = rest of packet. depends on packet
*/

cResponsePacket::cResponsePacket()
{
  buffer = NULL;
  bufSize = 0;
  bufUsed = 0;
}

cResponsePacket::~cResponsePacket()
{
  if (buffer) free(buffer);
}

void cResponsePacket::initBuffers()
{
  if (buffer == NULL) {
    bufSize = 512;
    buffer = (uint8_t*)malloc(bufSize);
  }
}

bool cResponsePacket::init(uint32_t requestID)
{
  initBuffers();

  uint32_t ul;

  ul = htonl(VNSI_CHANNEL_REQUEST_RESPONSE);                     // RR channel
  memcpy(&buffer[0], &ul, sizeof(uint32_t));
  ul = htonl(requestID);
  memcpy(&buffer[4], &ul, sizeof(uint32_t));
  ul = 0;
  memcpy(&buffer[userDataLenPos], &ul, sizeof(uint32_t));

  bufUsed = headerLength;

  return true;
}

bool cResponsePacket::initScan(uint32_t opCode)
{
  initBuffers();

  uint32_t ul;

  ul = htonl(VNSI_CHANNEL_SCAN);                     // RR channel
  memcpy(&buffer[0], &ul, sizeof(uint32_t));
  ul = htonl(opCode);
  memcpy(&buffer[4], &ul, sizeof(uint32_t));
  ul = 0;
  memcpy(&buffer[userDataLenPos], &ul, sizeof(uint32_t));

  bufUsed = headerLength;

  return true;
}

bool cResponsePacket::initStatus(uint32_t opCode)
{
  initBuffers();

  uint32_t ul;

  ul = htonl(VNSI_CHANNEL_STATUS);                     // RR channel
  memcpy(&buffer[0], &ul, sizeof(uint32_t));
  ul = htonl(opCode);
  memcpy(&buffer[4], &ul, sizeof(uint32_t));
  ul = 0;
  memcpy(&buffer[userDataLenPos], &ul, sizeof(uint32_t));

  bufUsed = headerLength;

  return true;
}

bool cResponsePacket::initStream(uint32_t opCode, uint32_t streamID, uint32_t duration, int64_t pts, int64_t dts)
{
  initBuffers();

  uint32_t ul;
  uint64_t ull;

  ul =  htonl(VNSI_CHANNEL_STREAM);            // stream channel
  memcpy(&buffer[0], &ul, sizeof(uint32_t));
  ul = htonl(opCode);                          // Stream packet operation code
  memcpy(&buffer[4], &ul, sizeof(uint32_t));
  ul = htonl(streamID);                        // Stream ID
  memcpy(&buffer[8], &ul, sizeof(uint32_t));
  ul = htonl(duration);                        // Duration
  memcpy(&buffer[12], &ul, sizeof(uint32_t));
  ull = __cpu_to_be64(pts);                    // PTS
  memcpy(&buffer[16], &ull, sizeof(uint64_t));
  ull = __cpu_to_be64(dts);                    // DTS
  memcpy(&buffer[24], &ull, sizeof(uint64_t));
  ul = 0;
  memcpy(&buffer[userDataLenPosStream], &ul, sizeof(uint32_t));

  bufUsed = headerLengthStream;

  return true;
}

void cResponsePacket::finalise()
{
  uint32_t ul = htonl(bufUsed - headerLength);
  memcpy(&buffer[userDataLenPos], &ul, sizeof(uint32_t));
}

void cResponsePacket::finaliseStream()
{
  uint32_t ul = htonl(bufUsed - headerLengthStream);
  memcpy(&buffer[userDataLenPosStream], &ul, sizeof(uint32_t));
}


bool cResponsePacket::copyin(const uint8_t* src, uint32_t len)
{
  if (!checkExtend(len)) return false;
  memcpy(buffer + bufUsed, src, len);
  bufUsed += len;
  return true;
}

uint8_t* cResponsePacket::reserve(uint32_t len) {
  if (!checkExtend(len)) return false;
  uint8_t* result = buffer + bufUsed;
  bufUsed += len;
  return result;
}

bool cResponsePacket::unreserve(uint32_t len) {
  if(bufUsed < len) return false;
  bufUsed -= len;
  return true;
}

bool cResponsePacket::add_String(const char* string)
{
  uint32_t len = strlen(string) + 1;
  if (!checkExtend(len)) return false;
  memcpy(buffer + bufUsed, string, len);
  bufUsed += len;
  return true;
}

bool cResponsePacket::add_U32(uint32_t ul)
{
  if (!checkExtend(sizeof(uint32_t))) return false;
  uint32_t tmp = htonl(ul);
  memcpy(&buffer[bufUsed], &tmp, sizeof(uint32_t));
  bufUsed += sizeof(uint32_t);
  return true;
}

bool cResponsePacket::add_U8(uint8_t c)
{
  if (!checkExtend(sizeof(uint8_t))) return false;
  buffer[bufUsed] = c;
  bufUsed += sizeof(uint8_t);
  return true;
}

bool cResponsePacket::add_S32(int32_t l)
{
  if (!checkExtend(sizeof(int32_t))) return false;
  int32_t tmp = htonl(l);
  memcpy(&buffer[bufUsed], &tmp, sizeof(int32_t));
  bufUsed += sizeof(int32_t);
  return true;
}

bool cResponsePacket::add_U64(uint64_t ull)
{
  if (!checkExtend(sizeof(uint64_t))) return false;
  uint64_t tmp = __cpu_to_be64(ull);
  memcpy(&buffer[bufUsed], &tmp, sizeof(uint64_t));
  bufUsed += sizeof(uint64_t);
  return true;
}

bool cResponsePacket::add_double(double d)
{
  if (!checkExtend(sizeof(double))) return false;
  uint64_t ull;
  memcpy(&ull, &d, sizeof(double));
  ull = __cpu_to_be64(ull);
  memcpy(&buffer[bufUsed], &ull, sizeof(uint64_t));
  bufUsed += sizeof(uint64_t);
  return true;
}


bool cResponsePacket::checkExtend(uint32_t by)
{
  if ((bufUsed + by) < bufSize) return true;
  if (512 > by) by = 512;
  uint8_t* newBuf = (uint8_t*)realloc(buffer, bufSize + by);
  if (!newBuf) return false;
  buffer = newBuf;
  bufSize += by;
  return true;
}
