#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  Parts of this file originate from Team MediaPortal's
 *  TsReader DirectShow filter
 *  MediaPortal is a GPL'ed HTPC-Application
 *  Copyright (C) 2005-2012 Team MediaPortal
 *  http://www.team-mediaportal.com
 *
 * Changes compared to Team MediaPortal's version:
 * - Code cleanup for PVR addon usage
 * - Code refactoring for cross platform usage
 *************************************************************************/

#include "client.h"
#include "FileReader.h"
#include "platform/util/StdString.h"

class CTsReader
{
public:
  CTsReader();
  ~CTsReader(void) {};
  long Open(const char* pszFileName);
  long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes);
  void Close();
  int64_t SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod);
  int64_t GetFileSize();
  int64_t GetFilePointer();
  void OnZap(void);
#if defined(TARGET_WINDOWS)
  long long sigmaTime();
  long long  sigmaCount();
#endif

private:
  bool            m_bTimeShifting;
  bool            m_bRecording;
  bool            m_bLiveTv;
  CStdString      m_fileName;
  FileReader*     m_fileReader;
#if defined(TARGET_WINDOWS)
  LARGE_INTEGER   liDelta; 
  LARGE_INTEGER   liCount; 
#endif
};
