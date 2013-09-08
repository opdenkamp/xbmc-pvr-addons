/* 
 *  Copyright (C) 2006-2008 Team MediaPortal
 *  http://www.team-mediaportal.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "os-dependent.h"
#include "client.h" //XBMC->Log
#include "TSHeader.h" 

#define PAYLOADONLY             1
#define ADAPTIONFIELDONLY       2
#define ADAPTIONFIELDANDPAYLOAD 3

using namespace ADDON;

CTsHeader::CTsHeader(): SyncByte(0),
  TransportError(false),
  PayloadUnitStart(false),
  TransportPriority(false),
  Pid(0),
  TScrambling(0),
  AdaptionControl(0),
  ContinuityCounter(0),
  AdaptionFieldLength(0),
  PayLoadStart(0),
  HasAdaptionField(false),
  HasPayload(false),
  m_packet(NULL)
{
}

CTsHeader::CTsHeader(byte* tsPacket)
{
  Decode(tsPacket);
}

CTsHeader::~CTsHeader(void)
{
}

bool CTsHeader::PayLoadOnly()
{
  return (AdaptionControl == 1);
}

bool CTsHeader::AdaptionFieldOnly()
{
  return (AdaptionControl == 2);
}

bool CTsHeader::AdaptionFieldAndPayLoad()
{
  return (AdaptionControl == 3);
}

void CTsHeader::Decode(byte *data)
{
  m_packet = data;
  //47 40 d2 10
  //                              bits  byteNo    mask
  //SyncByte                      :  8      0        0xff  11111111
  //TransportError                :  1      1        0x80  10000000
  //PayloadUnitStart              :  1      1        0x40  01000000
  //TransportPriority             :  1      1        0x20  00100000
  //Pid                           : 13    1&2              00011111 11111111
  //Transport Scrambling Control  :  2      3        0xc0  11000000
  //Adaption Field Control        :  2      3        0x30  00110000
  //ContinuityCounter             :  4      3        0xf   00001111

  //Two adaption field control bits which may take four values:
  // 1. 01 � no adaptation field, payload only         0x10    1
  // 2. 10 � adaptation field only, no payload         0x20    2
  // 3. 11 � adaptation field followed by payload      0x30    3
  // 4. 00 - RESERVED for future use                   0x00

  SyncByte = data[0];

  if (SyncByte != 0x47)
  {
    TransportError = true;
    return;
  }

  TransportError = (data[1] & 0x80) > 0 ? true : false;
  PayloadUnitStart = (data[1] & 0x40) > 0 ? true : false;
  TransportPriority = (data[1] & 0x20) > 0 ? true : false;
  Pid = ((data[1] & 0x1F) <<8) + data[2];
  TScrambling = data[3] & 0x80;
  AdaptionControl = (data[3]>>4) & 0x3;
  HasAdaptionField = ((data[3] & 0x20) == 0x20);
  HasPayload = ((data[3] & 0x10) == 0x10);
  ContinuityCounter = data[3] & 0x0F;
  AdaptionFieldLength = 0;
  PayLoadStart = 4;

  if (HasAdaptionField)
  {
    AdaptionFieldLength = data[4];

    // Only set payload start if it starts in this packet
    if((5 + AdaptionFieldLength) < 188)
      PayLoadStart=5+AdaptionFieldLength;
  }
  if (PayloadUnitStart && !HasPayload)
    PayloadUnitStart = false;
}

void CTsHeader::LogHeader()
{
  XBMC->Log(LOG_DEBUG, "tsheader:%02.2x%02.2x%02.2x%02.2x%02.2x%02.2x%02.2x%02.2x%02.2x%02.2x",
          m_packet[0],m_packet[1],m_packet[2],m_packet[3],m_packet[4],m_packet[5],m_packet[6],m_packet[7],m_packet[8],m_packet[9]);
  XBMC->Log(LOG_DEBUG, "  SyncByte           :%x", SyncByte);
  XBMC->Log(LOG_DEBUG, "  TransportError     :%x", TransportError);
  XBMC->Log(LOG_DEBUG, "  PayloadUnitStart   :%d", PayloadUnitStart);
  XBMC->Log(LOG_DEBUG, "  TransportPriority  :%x", TransportPriority);
  XBMC->Log(LOG_DEBUG, "  Pid                :%x", Pid);
  XBMC->Log(LOG_DEBUG, "  TScrambling        :%x", TScrambling);
  XBMC->Log(LOG_DEBUG, "  AdaptionControl    :%x", AdaptionControl);
  XBMC->Log(LOG_DEBUG, "  ContinuityCounter  :%x", ContinuityCounter);
  XBMC->Log(LOG_DEBUG, "  AdaptionFieldLength:%d", AdaptionFieldLength);
  XBMC->Log(LOG_DEBUG, "  PayLoadStart       :%d", PayLoadStart);
  XBMC->Log(LOG_DEBUG, "  PayLoadOnly            :%d", PayLoadOnly());
  XBMC->Log(LOG_DEBUG, "  AdaptionFieldOnly      :%d", AdaptionFieldOnly());
  XBMC->Log(LOG_DEBUG, "  AdaptionFieldAndPayLoad:%d", AdaptionFieldAndPayLoad());
}
