/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <list>
#include "parser.h"

struct sStreamPacket;
class cTSStream;
class cChannel;
class cPatPmtParser;
class cVideoBuffer;

struct sStreamInfo
{
  int pID;
  eStreamType type;
  eStreamContent content;
  char language[MAXLANGCODE2];
  int subtitlingType;
  int compositionPageId;
  int ancillaryPageId;
  void SetLanguage(const char* lang)
  {
    language[0] = lang[0];
    language[1] = lang[1];
    language[2] = lang[2];
    language[3] = 0;
  }
};

class cVNSIDemuxer
{
public:
  cVNSIDemuxer();
  virtual ~cVNSIDemuxer();
  int Read(sStreamPacket *packet);
  cTSStream *GetFirstStream();
  cTSStream *GetNextStream();
  void Open(const cChannel &channel, cVideoBuffer *videoBuffer);
  void Close();

protected:
  bool EnsureParsers();
  void SetChannelStreams(const cChannel *channel);
  void SetChannelPids(cChannel *channel, cPatPmtParser *patPmtParser);
  cTSStream *FindStream(int Pid);
  void AddStreamInfo(sStreamInfo &stream);
  std::list<cTSStream*> m_Streams;
  std::list<cTSStream*>::iterator m_StreamsIterator;
  std::list<sStreamInfo> m_StreamInfos;
  cChannel m_CurrentChannel;
  cPatPmtParser m_PatPmtParser;
  int m_OldPmtVersion;
  bool m_WaitIFrame;
  cVideoBuffer *m_VideoBuffer;
};
