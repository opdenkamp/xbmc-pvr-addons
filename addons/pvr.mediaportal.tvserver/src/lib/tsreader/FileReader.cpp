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

#include "FileReader.h"
#include "client.h" //for XBMC->Log
#include "platform/threads/threads.h"
#include <algorithm> //std::min, std::max
#include "platform/util/timeutils.h" // for usleep
#include "platform/util/util.h"

using namespace ADDON;

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED 0x01

/* indicate that that caller support read in the minimum defined chunk size, this disables internal cache then */
#define READ_CHUNKED   0x02

/* use cache to access this file */
#define READ_CACHED     0x04

/* open without caching. regardless to file type. */
#define READ_NO_CACHE  0x08

/* calcuate bitrate for file while reading */
#define READ_BITRATE   0x10

FileReader::FileReader() :
  m_hFileHandle(NULL),
  m_pFileName(0),
  m_fileSize(0),
  m_llBufferPointer(0),
  m_bDebugOutput(false)
{
}

FileReader::~FileReader()
{
  CloseFile();
  SAFE_DELETE_ARRAY(m_pFileName);
}


long FileReader::GetFileName(char* *lpszFileName)
{
  *lpszFileName = m_pFileName;
  return S_OK;
}


long FileReader::SetFileName(const char *pszFileName)
{
  if(strlen(pszFileName) > MAX_PATH)
    return ERROR_FILENAME_EXCED_RANGE;

  // Take a copy of the filename
  if (m_pFileName)
  {
    SAFE_DELETE_ARRAY(m_pFileName);
  }

  m_pFileName = new char[1 + strlen(pszFileName)];
  if (m_pFileName == NULL)
    return E_OUTOFMEMORY;

  strncpy(m_pFileName, pszFileName, strlen(pszFileName) + 1);

  return S_OK;
}

//
// OpenFile
//
// Opens the file ready for streaming
//
long FileReader::OpenFile()
{
  int Tmo = 25; //5 in MediaPortal

  // Is the file already opened
  if (!IsFileInvalid())
  {
    XBMC->Log(LOG_NOTICE, "FileReader::OpenFile() file already open");
    return S_OK;
  }

  // Has a filename been set yet
  if (m_pFileName == NULL) 
  {
    XBMC->Log(LOG_ERROR, "FileReader::OpenFile() no filename");
    return ERROR_INVALID_NAME;
  }

  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() Trying to open %s\n", m_pFileName);

  do
  {
    XBMC->Log(LOG_INFO, "FileReader::OpenFile() %s.", m_pFileName);
    // Open in readonly mode with this filename
    void* fileHandle = XBMC->OpenFile(m_pFileName, READ_CHUNKED);
    if (fileHandle)
    {
      m_hFileHandle = fileHandle;
      break;
    }

    usleep(20000);
  }
  while(--Tmo);

  if (Tmo)
  {
    if (Tmo<4) // 1 failed + 1 succeded is quasi-normal, more is a bit suspicious ( disk drive too slow or problem ? )
      XBMC->Log(LOG_DEBUG, "FileReader::OpenFile(), %d tries to succeed opening %ws.", 6-Tmo, m_pFileName);
  }
  else
  {
    XBMC->Log(LOG_ERROR, "FileReader::OpenFile(), open file %s failed.", m_pFileName);
    return S_FALSE;
  }

  XBMC->Log(LOG_DEBUG, "%s: OpenFile(%s) succeeded.", __FUNCTION__, m_pFileName );

  SetFilePointer(0, SEEK_SET);
  m_llBufferPointer = 0;

  return S_OK;

} // Open

//
// CloseFile
//
// Closes any dump file we have opened
//
long FileReader::CloseFile()
{
  if (IsFileInvalid())
  {
    return S_OK;
  }

  if (m_hFileHandle)
  {
    XBMC->CloseFile(m_hFileHandle);
    m_hFileHandle = NULL;
  }

  m_llBufferPointer = 0;
  return S_OK;
} // CloseFile


inline bool FileReader::IsFileInvalid()
{
  return m_hFileHandle == NULL;
}


long FileReader::GetStartPosition(int64_t *lpllpos)
{
  *lpllpos = 0;
  return S_OK;
}


int64_t FileReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  //XBMC->Log(LOG_DEBUG, "%s: distance %d method %d.", __FUNCTION__, llDistanceToMove, dwMoveMethod);
  int64_t rc = XBMC->SeekFile(m_hFileHandle, llDistanceToMove, dwMoveMethod);
  //XBMC->Log(LOG_DEBUG, "%s: distance %d method %d returns %d.", __FUNCTION__, llDistanceToMove, dwMoveMethod, rc);
  return rc;
}


int64_t FileReader::GetFilePointer()
{
  return XBMC->GetFilePosition(m_hFileHandle);
}


long FileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  *dwReadBytes = XBMC->ReadFile(m_hFileHandle, (void*)pbData, lDataLength);//Read file data into buffer
  //XBMC->Log(LOG_DEBUG, "%s: requested read length %d actually read %d.", __FUNCTION__, lDataLength, *dwReadBytes);

  if (*dwReadBytes < lDataLength)
  {
    XBMC->Log(LOG_NOTICE, "%s: requested %d bytes, read only %d bytes.", __FUNCTION__, lDataLength, *dwReadBytes);
    return S_FALSE;
  }
  return S_OK;
}


int64_t FileReader::GetFileSize()
{
  return XBMC->GetFileLength(m_hFileHandle);
}

int64_t FileReader::OnChannelChange(void)
{
  return XBMC->GetFilePosition(m_hFileHandle);
}
