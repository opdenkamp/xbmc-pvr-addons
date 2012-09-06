/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 */

/*
 *  This file originates from TSFileSource, a GPL directshow push
 *  source filter that provides an MPEG transport stream output.
 *  Copyright (C) 2005      nate
 *  Copyright (C) 2006      bear
 *
 *  nate can be reached on the forums at
 *    http://forums.dvbowners.com/
 */

#if defined TSREADER

#include "FileReader.h"
#include "client.h" //for XBMC->Log
#include "os-dependent.h"
#include <algorithm> //std::min, std::max
#include "utils.h"
#include "platform/util/timeutils.h" // for usleep

using namespace ADDON;
using namespace PLATFORM;

FileReader::FileReader() :
#if defined(TARGET_WINDOWS)
  m_hFile(INVALID_HANDLE_VALUE),
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  m_hFile(),
#else
#error Implement initialisation of file for your OS
#endif
  m_pFileName(0),
  m_bReadOnly(false),
  m_fileSize(0),
  m_fileStartPos(0),
  m_bDebugOutput(false)
{
}

FileReader::~FileReader()
{
  CloseFile();
  if (m_pFileName)
    delete m_pFileName;
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
    delete[] m_pFileName;
    m_pFileName = NULL;
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
#if defined(TARGET_WINDOWS)
    // do not try to open a tsbuffer file without SHARE_WRITE so skip this try if we have a buffer file
    CStdStringW strWFile = UTF8Util::ConvertUTF8ToUTF16(m_pFileName);
    if (strstr(m_pFileName, ".ts.tsbuffer") == NULL) 
    {
      // Try to open the file
      m_hFile = ::CreateFileW(strWFile,      // The filename
             (DWORD) GENERIC_READ,             // File access
             (DWORD) FILE_SHARE_READ,          // Share access
             NULL,                             // Security
             (DWORD) OPEN_EXISTING,            // Open flags
             (DWORD) 0,                        // More flags
             NULL);                            // Template

      m_bReadOnly = FALSE;
      if (!IsFileInvalid()) break;
    }

    //Test incase file is being recorded to
    m_hFile = ::CreateFileW(strWFile,         // The filename
              (DWORD) GENERIC_READ,             // File access
              (DWORD) (FILE_SHARE_READ |
              FILE_SHARE_WRITE),                // Share access
              NULL,                             // Security
              (DWORD) OPEN_EXISTING,            // Open flags
//              (DWORD) 0,
              (DWORD) FILE_ATTRIBUTE_NORMAL,    // More flags
//              FILE_ATTRIBUTE_NORMAL |
//              FILE_FLAG_RANDOM_ACCESS,        // More flags
//              FILE_FLAG_SEQUENTIAL_SCAN,      // More flags
              NULL);                            // Template
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
    // Try to open the file
    XBMC->Log(LOG_INFO, "FileReader::OpenFile() %s %s.", m_pFileName, CFile::Exists(m_pFileName) ? "exists" : "not found");
    m_hFile.Open(m_pFileName, READ_CHUNKED);        // Open in readonly mode with this filename
#else
#error FIXME: Add an OpenFile() implementation for your OS
#endif
    m_bReadOnly = true;
    if (!IsFileInvalid()) break;

    usleep(20000) ;
  }
  while(--Tmo);

  if (Tmo)
  {
    if (Tmo<4) // 1 failed + 1 succeded is quasi-normal, more is a bit suspicious ( disk drive too slow or problem ? )
      XBMC->Log(LOG_DEBUG, "FileReader::OpenFile(), %d tries to succeed opening %ws.", 6-Tmo, m_pFileName);
  }
  else
  {
#ifdef TARGET_WINDOWS
    DWORD dwErr = GetLastError();
    XBMC->Log(LOG_ERROR, "FileReader::OpenFile(), open file %s failed. Error code %d", m_pFileName, dwErr);
    return HRESULT_FROM_WIN32(dwErr);
#else
    XBMC->Log(LOG_ERROR, "FileReader::OpenFile(), open file %s failed. Error %d: %s", m_pFileName, errno, strerror(errno));
    return S_FALSE;
#endif
  }

#if defined(TARGET_WINDOWS)
  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() %s handle %i %s", m_bReadOnly ? "read-only" : "read/write", m_hFile, m_pFileName );
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() %s %s", m_bReadOnly ? "read-only" : "read/write", m_pFileName );
#else
#error FIXME: Add an debug log implementation for your OS
#endif

  //SetFilePointer(0, FILE_BEGIN);

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

  //XBMC->Log(LOG_DEBUG, "FileReader::CloseFile() handle %i %ws", m_hFile, m_pFileName);

//  BoostThread Boost;

#if defined(TARGET_WINDOWS)
  ::CloseHandle(m_hFile);
  m_hFile = INVALID_HANDLE_VALUE; // Invalidate the file
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  m_hFile.Close();
#else
#error FIXME: Add a CloseFile() implementation for your OS
#endif

  return S_OK;
} // CloseFile


inline bool FileReader::IsFileInvalid()
{
#if defined(TARGET_WINDOWS)
  return (m_hFile == INVALID_HANDLE_VALUE);
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  return (m_hFile.IsInvalid());
#else
#error FIXME: Add an IsFileInvalid implementation for your OS
#endif
}

unsigned long FileReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  //XBMC->Log(LOG_DEBUG, "%s: distance %d method %d.", __FUNCTION__, llDistanceToMove, dwMoveMethod);
#if defined(TARGET_WINDOWS)
  LARGE_INTEGER li, fileposition;

  li.QuadPart = llDistanceToMove;

  if (::SetFilePointerEx(m_hFile, li, &fileposition, dwMoveMethod) == 0)
  {
    // Error
    XBMC->Log(LOG_ERROR, "FileReader::GetFilePointer() ::SetFilePointerEx failed");
    return -1;
  }
  return (unsigned long) fileposition.QuadPart;

#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  // Stupid but simple movement transform
  if (dwMoveMethod == FILE_BEGIN) dwMoveMethod = SEEK_SET;
  else if (dwMoveMethod == FILE_CURRENT) dwMoveMethod = SEEK_CUR;
  else if (dwMoveMethod == FILE_END) dwMoveMethod = SEEK_END;
  else
  {
    XBMC->Log(LOG_ERROR, "%s: SetFilePointer invalid MoveMethod(%d)", __FUNCTION__, dwMoveMethod);
    dwMoveMethod = SEEK_SET;
  }
  int64_t rc = m_hFile.Seek(llDistanceToMove, dwMoveMethod);
  //XBMC->Log(LOG_DEBUG, "%s: distance %d method %d returns %d.", __FUNCTION__, llDistanceToMove, dwMoveMethod, rc);
  return rc;
#else
  //lseek (or fseek for fopen)
#error FIXME: Add a SetFilePointer() implementation for your OS
  return 0;
#endif
}


int64_t FileReader::GetFilePointer()
{
#if defined(TARGET_WINDOWS)
  LARGE_INTEGER li, seekoffset;
  seekoffset.QuadPart = 0;
  if (::SetFilePointerEx(m_hFile, seekoffset, &li, FILE_CURRENT) == 0)
  {
    // Error
    XBMC->Log(LOG_ERROR, "FileReader::GetFilePointer() ::SetFilePointerEx failed");
    return -1;
  }

  return li.QuadPart;
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  off64_t myOffset;
  myOffset = m_hFile.Seek(0, SEEK_CUR);

  //XBMC->Log(LOG_DEBUG, "%s: returns %d GetPosition(%d).", __FUNCTION__, myOffset, m_hFile.GetPosition());
  return myOffset;
#else
#error FIXME: Add a GetFilePointer() implementation for your OS
  return 0;
#endif
}

long FileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  // If the file has already been closed, don't continue
  if (IsFileInvalid())
  {
    XBMC->Log(LOG_ERROR, "FileReader::Read() no open file");
    return E_FAIL;
  }
//  BoostThread Boost;

#ifdef TARGET_WINDOWS
  long hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)lDataLength, dwReadBytes, NULL);//Read file data into buffer

  if (!hr)
  {
    XBMC->Log(LOG_ERROR, "FileReader::Read() read failed - error = %d",  HRESULT_FROM_WIN32(GetLastError()));
    return E_FAIL;
  }

  if (*dwReadBytes < (unsigned long)lDataLength)
  {
    XBMC->Log(LOG_DEBUG, "FileReader::Read() read to less bytes");
    return S_FALSE;
  }
  return S_OK;
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  *dwReadBytes = m_hFile.Read((void*)pbData, lDataLength);//Read file data into buffer
  //XBMC->Log(LOG_DEBUG, "%s: requested read length %d actually read %d.", __FUNCTION__, lDataLength, *dwReadBytes);

  if (*dwReadBytes < (unsigned long)lDataLength)
  {
    XBMC->Log(LOG_DEBUG, "FileReader::Read() read too less bytes");
    return S_FALSE;
  }
  return S_OK;
#else
#error FIXME: Add a Read() implementation for your OS
  return S_FALSE;
#endif
}

void FileReader::SetDebugOutput(bool bDebugOutput)
{
  m_bDebugOutput = bDebugOutput;
}

int64_t FileReader::GetFileSize()
{
#if defined(TARGET_WINDOWS)
  //Do not get file size if static file or first time 
  if (m_bReadOnly || !m_fileSize) {
    LARGE_INTEGER li;
    if (::GetFileSizeEx(m_hFile, &li) == 0)
    {
      // Error
      XBMC->Log(LOG_ERROR, "FileReader::GetFileSize() ::GetFileSizeEx failed");
      return -1;
    }
    m_fileSize = li.QuadPart;
  }
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN)
  m_fileSize = m_hFile.GetLength();
#else
#error FIXME: Add a GetFileSize() implementation for your OS
#endif
  //XBMC->Log(LOG_DEBUG, "%s: returns %d.", __FUNCTION__, m_fileSize);
  return m_fileSize;
}

void FileReader::OnZap(void)
{
  SetFilePointer(0, FILE_END);
}
#endif //TSREADER
