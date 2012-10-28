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

#include "platform/os.h"

class FileReader
{
  public:
    FileReader();
    virtual ~FileReader();

    // Open and write to the file
    virtual long GetFileName(char* *lpszFileName);
    virtual long SetFileName(const char* pszFileName);
    virtual long OpenFile();
    virtual long CloseFile();
    virtual long Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes);
    long GetStartPosition(int64_t *lpllpos);
    virtual bool IsFileInvalid();
    virtual int64_t SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod);
    virtual int64_t GetFilePointer();
    virtual int64_t GetFileSize();
    virtual bool IsBuffer(){return false;};
    virtual int64_t OnChannelChange(void);

  protected:
    void*    m_hFileHandle;        // Handle to file for streaming
    char*    m_pFileName;           // The filename where we stream
    int64_t  m_fileSize;
    int64_t  m_llBufferPointer;

    bool     m_bDebugOutput;
};
