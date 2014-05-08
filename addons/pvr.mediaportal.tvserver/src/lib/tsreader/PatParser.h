/* 
 *  Copyright (C) 2006 Team MediaPortal
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "SectionDecoder.h"
#include "PmtParser.h"
#include "ChannelInfo.h"
#include <vector>
using namespace std;

class IPatParserCallback
{
public:
  virtual void OnNewChannel(CChannelInfo& info)=0;
};

class CPatParser : public CSectionDecoder
{
public:
  enum PatState
  {
    Idle,
    Parsing,
  };
  CPatParser(void);
  virtual ~CPatParser(void);
  void        SkipPacketsAtStart(int64_t packets);
  void        OnTsPacket(byte* tsPacket);
  void        Reset();
  void        OnNewSection(CSection& section);
  int         Count();
  bool        GetChannel(int index, CChannelInfo& info);
  void        Dump();
  void        SetCallBack(IPatParserCallback* callback);
private:
  void        CleanUp();
  IPatParserCallback* m_pCallback;
  vector<CPmtParser*> m_pmtParsers;
  int64_t     m_packetsReceived;
  int64_t     m_packetsToSkip;
  int          m_iPatTableVersion;
  PatState    m_iState;
};
