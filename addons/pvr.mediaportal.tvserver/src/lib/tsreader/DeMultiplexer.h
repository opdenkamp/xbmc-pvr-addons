/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *************************************************************************
 *  This file is a modified version from Team MediaPortal's
 *  TsReader DirectShow filter
 *  MediaPortal is a GPL'ed HTPC-Application
 *  Copyright (C) 2005-2012 Team MediaPortal
 *  http://www.team-mediaportal.com
 *
 * Changes compared to Team MediaPortal's version:
 * - Code cleanup for PVR addon usage
 * - Code refactoring for cross platform usage
 *************************************************************************
 *  This file originates from TSFileSource, a GPL directshow push
 *  source filter that provides an MPEG transport stream output.
 *  Copyright (C) 2005-2006 nate, bear
 *  http://forums.dvbowners.com/
 */
#pragma once

#include "MultiFileReader.h"
#include "PacketSync.h"
#include "TSHeader.h"
#include "PatParser.h"
#include "platform/threads/mutex.h"

using namespace std;
class CTsReader;

class CDeMultiplexer : public CPacketSync, public IPatParserCallback
{
public:
  CDeMultiplexer(CTsReader& filter);
  virtual ~CDeMultiplexer(void);

  void       Start();
  void       OnTsPacket(byte* tsPacket);
  void       OnNewChannel(CChannelInfo& info);
  void       SetFileReader(FileReader* reader);
  void RequestNewPat(void);
  int ReadFromFile();

private:
  unsigned long m_LastDataFromRtsp;
  bool m_bEndOfFile;
  PLATFORM::CMutex m_sectionRead;
  FileReader* m_reader;
  CPatParser m_patParser;
  CTsReader& m_filter;

  int m_iPatVersion;
  int m_ReqPatVersion;
  int m_WaitNewPatTmo;
  int m_receivedPackets;

  bool m_bStarting;

  bool m_bAudioAtEof;
  bool m_bVideoAtEof;

  // XBMC specific
  bool m_bGotNewChannel;
};
