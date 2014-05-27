#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdio.h>
#include <stdint.h>

class cResponsePacket
{
  public:
    cResponsePacket();
    ~cResponsePacket();

    void setResponse(uint8_t* packet, uint32_t packetLength);
    void setStatus(uint8_t* packet, uint32_t packetLength);
    void setStream(uint8_t* packet, uint32_t packetLength);
    void setOSD(uint8_t* packet, uint32_t packetLength);

    void extractHeader();
    void extractStreamHeader();
    void extractOSDHeader();

    bool noResponse() { return (userData == NULL); };
    int  serverError();

    uint32_t  getUserDataLength() { return userDataLength; }
    uint32_t  getChannelID()      { return channelID; }
    uint32_t  getRequestID()      { return requestID; }
    uint32_t  getStreamID()       { return streamID; }
    uint32_t  getOpCodeID()       { return opcodeID; }
    uint32_t  getDuration()       { return duration; }
    int64_t   getDTS()            { return dts; }
    int64_t   getPTS()            { return pts; }
    uint32_t  getMuxSerial()      { return muxSerial; }
    void      getOSDData(uint32_t &wnd, uint32_t &color, uint32_t &x0, uint32_t &y0, uint32_t &x1, uint32_t &y1);

    uint32_t  getPacketPos()      { return packetPos; }

    char*     extract_String();
    uint8_t   extract_U8();
    uint32_t  extract_U32();
    uint64_t  extract_U64();
    int32_t   extract_S32();
    int64_t   extract_S64();
    double    extract_Double();

    bool      end();

    // If you call this, the memory becomes yours. Free with free()
    uint8_t* getUserData();

    uint8_t* getHeader() { return header; };
    unsigned int getStreamHeaderLength() { return 36; };
    unsigned int getHeaderLength() { return 8; };
    unsigned int getOSDHeaderLength() { return 32; } ;

  private:
    uint8_t  header[40];
    uint8_t* userData;
    uint32_t userDataLength;
    uint32_t packetPos;

    uint32_t channelID;

    uint32_t requestID;
    uint32_t streamID;
    uint32_t opcodeID;
    uint32_t duration;
    int64_t  dts;
    int64_t  pts;
    uint32_t muxSerial;

    int32_t osdWnd;
    int32_t osdColor;
    int32_t osdX0,osdY0,osdX1,osdY1;

    bool ownBlock;
};
