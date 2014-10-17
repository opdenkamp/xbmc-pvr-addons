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

#include "MultiFileReader.h"
#include "client.h" //for XBMC->Log
#include "TSDebug.h"
#include <string>
#include "utils.h"
#include <algorithm>
#include "platform/util/timeutils.h"
#include "platform/util/StdString.h"
#include "platform/threads/threads.h"
#include <inttypes.h>

using namespace ADDON;
using namespace PLATFORM;

//Maximum time in msec to wait for the buffer file to become available - Needed for DVB radio (this sometimes takes some time)
#define MAX_BUFFER_TIMEOUT 1500

MultiFileReader::MultiFileReader():
  m_TSBufferFile(),
  m_TSFile()
{
  m_startPosition = 0;
  m_endPosition = 0;
  m_currentPosition = 0;
  m_lastZapPosition = 0;
  m_filesAdded = 0;
  m_filesRemoved = 0;
  m_TSFileId = 0;
  m_bDelay = 0;
}

MultiFileReader::~MultiFileReader()
{
  //CloseFile called by ~FileReader
}


long MultiFileReader::SetFileName(const std::string& fileName)
{
  return m_TSBufferFile.SetFileName(fileName);
}

long MultiFileReader::OpenFile(const std::string& fileName)
{
  SetFileName(fileName);
  return OpenFile();
}

//
// OpenFile
//
long MultiFileReader::OpenFile()
{
  long hResult = m_TSBufferFile.OpenFile();
  XBMC->Log(LOG_DEBUG, "MultiFileReader: buffer file opened return code %d.", hResult);

  if (hResult != S_OK)
    return hResult;

  m_lastZapPosition = 0;
  m_currentFileStartOffset = 0;

  int retryCount = 0;

  while ((m_TSBufferFile.GetFileSize() == 0) && (retryCount < 50))
  {
    retryCount++;
    XBMC->Log(LOG_DEBUG, "MultiFileReader: buffer file has zero length, closing, waiting 100 ms and re-opening. Attempt: %d.", retryCount);
    m_TSBufferFile.CloseFile();
    usleep(100000);
    hResult = m_TSBufferFile.OpenFile();
    XBMC->Log(LOG_DEBUG, "MultiFileReader: buffer file opened return code %d.", hResult);
  }

  if (RefreshTSBufferFile() == S_FALSE)
  {
    // For radio the buffer sometimes needs some time to become available, so wait and try it more than once
    PLATFORM::CTimeout timeout(MAX_BUFFER_TIMEOUT);

    do
    {
      usleep(100000);
      if (timeout.TimeLeft() == 0)
      {
        XBMC->Log(LOG_ERROR, "MultiFileReader: timed out while waiting for buffer file to become available");
        XBMC->QueueNotification(QUEUE_ERROR, "Time out while waiting for buffer file");
        return S_FALSE;
      }
    } while (RefreshTSBufferFile() == S_FALSE);
  }

  m_currentPosition = 0;

  return hResult;
}

//
// CloseFile
//
long MultiFileReader::CloseFile()
{
  long hr;
  std::vector<MultiFileReaderFile *>::iterator it;

  m_TSBufferFile.CloseFile();
  hr = m_TSFile.CloseFile();

  for (it = m_tsFiles.begin(); it < m_tsFiles.end(); ++it)
  {
    delete (*it);
  }
  m_tsFiles.clear();

  m_TSFileId = 0;
  return hr;
}

bool MultiFileReader::IsFileInvalid()
{
  return m_TSBufferFile.IsFileInvalid();
}

int64_t MultiFileReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  RefreshTSBufferFile();

  if (dwMoveMethod == FILE_END)
  {
    m_currentPosition = m_endPosition + llDistanceToMove;
  }
  else if (dwMoveMethod == FILE_CURRENT)
  {
    m_currentPosition += llDistanceToMove;
  }
  else // if (dwMoveMethod == FILE_BEGIN)
  {
    m_currentPosition = m_startPosition + llDistanceToMove;
  }

  if (m_currentPosition < m_startPosition)
    m_currentPosition = m_startPosition;

  if (m_currentPosition > m_endPosition)
  {
    XBMC->Log(LOG_ERROR, "Seeking beyond the end position: %I64d > %I64d", m_currentPosition, m_endPosition);
    m_currentPosition = m_endPosition;
  }

  return m_currentPosition;
}

int64_t MultiFileReader::SetCurrentFilePointer(int64_t timeShiftBufferFilePos, long timeshiftBufferFileID)
{
  RefreshTSBufferFile();

  if (m_TSFileId != timeshiftBufferFileID)
  {
    // We have to switch to a different buffer file
    TSDEBUG(LOG_DEBUG, "Change buffer file from %i to %i", m_TSFileId, timeshiftBufferFileID);

    MultiFileReaderFile *file = NULL;
    std::vector<MultiFileReaderFile *>::iterator it = m_tsFiles.begin();
    for ( ; it < m_tsFiles.end(); ++it )
    {
      file = *it;
      if (file->filePositionId == timeshiftBufferFileID)
        break;
    };

    if(!file)
    {
      XBMC->Log(LOG_ERROR, "MultiFileReader::no buffer file with id=%i", timeshiftBufferFileID);
      XBMC->QueueNotification(QUEUE_ERROR, "No buffer file");
      return m_currentPosition;
    }

    if (m_currentPosition < (file->startPosition + timeShiftBufferFilePos))
    {
      m_TSFile.CloseFile();
      m_TSFile.SetFileName(file->filename.c_str());
      m_TSFile.OpenFile();

      m_TSFileId = file->filePositionId;
      m_currentFileStartOffset = file->startPosition;

      TSDEBUG(LOG_DEBUG, "MultiFileReader::Read() Current File Changed to %s TS file id=%i\n", file->filename.c_str(), m_TSFileId);
    }
  }

  // Reposition the read pointer within the current timeshift buffer file
  TSDEBUG(LOG_DEBUG, "Move read pointer within buffer file %i; %I64d -> %i64d", m_TSFileId, m_currentPosition, m_currentFileStartOffset + timeShiftBufferFilePos);
  m_currentPosition = m_currentFileStartOffset + timeShiftBufferFilePos;

  if (m_currentPosition > m_endPosition)
  {
    XBMC->Log(LOG_ERROR, "Seeking beyond the end position: %I64d > %I64d", m_currentPosition, m_endPosition);
    m_currentPosition = m_endPosition;
  }

  return m_currentPosition;
}

int64_t MultiFileReader::GetFilePointer()
{
  return m_currentPosition;
}

long MultiFileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  // If the file has already been closed, don't continue
  if (m_TSBufferFile.IsFileInvalid())
    return S_FALSE;

  RefreshTSBufferFile();

  if (m_currentPosition < m_startPosition)
  {
    XBMC->Log(LOG_INFO, "%s: current position adjusted from %%I64dd to %%I64dd.", __FUNCTION__, m_currentPosition, m_startPosition);
    m_currentPosition = m_startPosition;
  }

  // Find out which file the currentPosition is in.
  MultiFileReaderFile *file = NULL;
  std::vector<MultiFileReaderFile *>::iterator it = m_tsFiles.begin();
  for ( ; it < m_tsFiles.end(); ++it )
  {
    file = *it;
    if (m_currentPosition < (file->startPosition + file->length))
      break;
  };

  // XBMC->Log(LOG_DEBUG, "%s: reading %ld bytes. File %s, start %d, current %d, end %d.", __FUNCTION__, lDataLength, file->filename.c_str(), m_startPosition, m_currentPosition, m_endPosition);

  if(!file)
  {
    XBMC->Log(LOG_ERROR, "MultiFileReader::no file");
    XBMC->QueueNotification(QUEUE_ERROR, "No buffer file");
    return S_FALSE;
  }
  if (m_currentPosition < (file->startPosition + file->length))
  {
    if (m_TSFileId != file->filePositionId)
    {
      m_TSFile.CloseFile();
      m_TSFile.SetFileName(file->filename.c_str());
      if (m_TSFile.OpenFile() != S_OK)
      {
        XBMC->Log(LOG_ERROR, "MultiFileReader: can't open %s\n", file->filename.c_str());
        return S_FALSE;
      }

      m_TSFileId = file->filePositionId;
      m_currentFileStartOffset = file->startPosition;

      TSDEBUG(LOG_DEBUG, "MultiFileReader::Read() Current File Changed to %s TS file id=%i\n", file->filename.c_str(), m_TSFileId);
    }

    int64_t seekPosition = m_currentPosition - file->startPosition;

    m_TSFile.SetFilePointer(seekPosition, FILE_BEGIN);
    int64_t posSeeked = m_TSFile.GetFilePointer();
    if (posSeeked != seekPosition)
    {
      m_TSFile.SetFilePointer(seekPosition, FILE_BEGIN);
      posSeeked = m_TSFile.GetFilePointer();
      if (posSeeked != seekPosition)
      {
        XBMC->Log(LOG_ERROR, "SEEK FAILED");
        return S_FALSE;
      }
    }

    unsigned long bytesRead = 0;
    long hr;

    int64_t bytesToRead = file->length - seekPosition;
    if ((int64_t)lDataLength > bytesToRead)
    {
      // XBMC->Log(LOG_DEBUG, "%s: datalength %lu bytesToRead %lli.", __FUNCTION__, lDataLength, bytesToRead);
      hr = m_TSFile.Read(pbData, (unsigned long)bytesToRead, &bytesRead);
      if (FAILED(hr))
      {
        XBMC->Log(LOG_ERROR, "READ FAILED1");
        return S_FALSE;
      }
      m_currentPosition += bytesToRead;

      hr = this->Read(pbData + bytesToRead, lDataLength - (unsigned long)bytesToRead, dwReadBytes);
      if (FAILED(hr))
      {
        XBMC->Log(LOG_ERROR, "READ FAILED2");
      }
      *dwReadBytes += bytesRead;
    }
    else
    {
      hr = m_TSFile.Read(pbData, lDataLength, dwReadBytes);
      if (FAILED(hr))
      {
        XBMC->Log(LOG_ERROR, "READ FAILED3");
      }
      m_currentPosition += lDataLength;
    }
  }
  else
  {
    // The current position is past the end of the last file
    *dwReadBytes = 0;
  }

  // XBMC->Log(LOG_DEBUG, "%s: read %lu bytes. start %lli, current %lli, end %lli.", __FUNCTION__, *dwReadBytes, m_startPosition, m_currentPosition, m_endPosition);
  return S_OK;
}


long MultiFileReader::RefreshTSBufferFile()
{
  if (m_TSBufferFile.IsFileInvalid())
  {
    XBMC->Log(LOG_ERROR, "%s: buffer file is invalid.", __FUNCTION__);
    return S_FALSE;
  }

  unsigned long bytesRead;
  MultiFileReaderFile *file;

  int64_t currentPosition;
  int32_t filesAdded, filesRemoved;
  int32_t filesAdded2, filesRemoved2;
  long Error = 0;
  long Loop = 10;

  Wchar_t* pBuffer = NULL;
  do
  {
    Error = 0;
    currentPosition = -1;
    filesAdded = -1;
    filesRemoved = -1;
    filesAdded2 = -2;
    filesRemoved2 = -2;

    int64_t fileLength = m_TSBufferFile.GetFileSize();

    // Min file length is Header ( int64_t + int32_t + int32_t ) + filelist ( > 0 ) + Footer ( int32_t + int32_t )
    int64_t minimumlength = (int64_t)(sizeof(currentPosition) + sizeof(filesAdded) + sizeof(filesRemoved) + sizeof(Wchar_t) + sizeof(filesAdded2) + sizeof(filesRemoved2));
    if (fileLength <= minimumlength)
    {
      XBMC->Log(LOG_ERROR, "%s: TSBufferFile too short. Minimum length %ld, current length %ld", __FUNCTION__, minimumlength, fileLength);
      return S_FALSE;
    }

    m_TSBufferFile.SetFilePointer(0, FILE_BEGIN);

    uint32_t readLength = sizeof(currentPosition) + sizeof(filesAdded) + sizeof(filesRemoved);
    unsigned char* readBuffer = new unsigned char[readLength];

    long result = m_TSBufferFile.Read(readBuffer, readLength, &bytesRead);

    if (!SUCCEEDED(result) || bytesRead!=readLength)
      Error |= 0x02;

    if (Error == 0)
    {
      currentPosition = *((int64_t*)(readBuffer + 0));
      filesAdded = *((int32_t*)(readBuffer + sizeof(currentPosition)));
      filesRemoved = *((int32_t*)(readBuffer + sizeof(currentPosition) + sizeof(filesAdded)));
    }

    delete[] readBuffer;

    // If no files added or removed, break the loop !
    if ((m_filesAdded == filesAdded) && (m_filesRemoved == filesRemoved)) 
      break;

    int64_t remainingLength = fileLength - sizeof(currentPosition) - sizeof(filesAdded) - sizeof(filesRemoved) - sizeof(filesAdded2) - sizeof(filesRemoved2);

    // Above 100kb seems stupid and figure out a problem !!!
    if (remainingLength > 100000)
      Error |= 0x10;
  
    pBuffer = (Wchar_t*) new char[(unsigned int)remainingLength];

    result = m_TSBufferFile.Read((unsigned char*) pBuffer, (uint32_t) remainingLength, &bytesRead);
    if ( !SUCCEEDED(result) || (int64_t) bytesRead != remainingLength)
      Error |= 0x20;

    readLength = sizeof(filesAdded) + sizeof(filesRemoved);

    readBuffer = new unsigned char[readLength];

    result = m_TSBufferFile.Read(readBuffer, readLength, &bytesRead);

    if (!SUCCEEDED(result) || bytesRead != readLength) 
      Error |= 0x40;

    if (Error == 0)
    {
      filesAdded2 = *((int32_t*)(readBuffer + 0));
      filesRemoved2 = *((int32_t*)(readBuffer + sizeof(filesAdded2)));
    }

    delete[] readBuffer;

    if ((filesAdded2 != filesAdded) || (filesRemoved2 != filesRemoved))
    {
      Error |= 0x80;

      XBMC->Log(LOG_ERROR, "MultiFileReader has error 0x%x in Loop %d. Try to clear SMB Cache.", Error, 10-Loop);
      XBMC->Log(LOG_DEBUG, "%s: filesAdded %d, filesAdded2 %d, filesRemoved %d, filesRemoved2 %d.", __FUNCTION__, filesAdded, filesAdded2, filesRemoved, filesRemoved2);

      // try to clear local / remote SMB file cache. This should happen when we close the filehandle
      m_TSBufferFile.CloseFile();
      m_TSBufferFile.OpenFile();
      usleep(5000);
    }

    if (Error)
      delete[] pBuffer;

    Loop--;
  } while ( Error && Loop ); // If Error is set, try again...until Loop reaches 0.

  if (Loop < 8)
  {
    XBMC->Log(LOG_DEBUG, "MultiFileReader has waited %d times for TSbuffer integrity.", 10-Loop);

    if (Error)
    {
      XBMC->Log(LOG_ERROR, "MultiFileReader has failed for TSbuffer integrity. Error : %x", Error);
      return E_FAIL;
    }
  }

  if ((m_filesAdded != filesAdded) || (m_filesRemoved != filesRemoved))
  {
    int32_t filesToRemove = filesRemoved - m_filesRemoved;
    int32_t filesToAdd = filesAdded - m_filesAdded;
    int32_t fileID = filesRemoved;
    int64_t nextStartPosition = 0;

    TSDEBUG(LOG_DEBUG, "MultiFileReader: Files Added %i, Removed %i\n", filesToAdd, filesToRemove);

    // Removed files that aren't present anymore.
    while ((filesToRemove > 0) && (!m_tsFiles.empty()))
    {
      file = m_tsFiles.at(0);

      TSDEBUG(LOG_DEBUG, "MultiFileReader: Removing file %s\n", file->filename.c_str());
      
      SAFE_DELETE(file);
      m_tsFiles.erase(m_tsFiles.begin());

      filesToRemove--;
    }


    // Figure out what the start position of the next new file will be
    if (!m_tsFiles.empty())
    {
      file = m_tsFiles.back();

      if (filesToAdd > 0)
      {
        // If we're adding files the changes are the one at the back has a partial length
        // so we need update it.
        GetFileLength(file->filename.c_str(), file->length);
      }

      nextStartPosition = file->startPosition + file->length;
    }

    // Get the real path of the buffer file
    std::string sFilename;
    std::string path;

    m_TSBufferFile.GetFileName(sFilename);
    //size_t pos = sFilename.find_last_of(PATH_SEPARATOR_CHAR);
    size_t pos = sFilename.find_last_of('/');
    path = sFilename.substr(0, pos+1);

    // Create a list of files in the .tsbuffer file.
    std::vector<std::string> filenames;

    Wchar_t* pwCurrFile = pBuffer;    //Get a pointer to the first wchar filename string in pBuffer
    long length = WcsLen(pwCurrFile);

    //XBMC->Log(LOG_DEBUG, "%s: WcsLen(%d), sizeof(Wchar_t) == %d sizeof(wchar_t) == %d.", __FUNCTION__, length, sizeof(Wchar_t), sizeof(wchar_t));

    while (length > 0)
    {
      // Convert the current filename (wchar to normal char)
      char* wide2normal = new char[length + 1];
      WcsToMbs(wide2normal, pwCurrFile, length);
      wide2normal[length] = '\0';
      std::string sCurrFile = wide2normal;
      //XBMC->Log(LOG_DEBUG, "%s: filename %s (%s).", __FUNCTION__, wide2normal, sCurrFile.c_str());
      delete[] wide2normal;

      // Modify filename path here to include the real (local) path
      pos = sCurrFile.find_last_of(92);
      std::string name = sCurrFile.substr(pos+1);
      if (path.length()>0 && name.length()>0)
      {
        // Replace the original path with our local path
        filenames.push_back(path + name);
      }
      else
      {
        // Keep existing path
        filenames.push_back(sCurrFile);
      }
      
      // Move the wchar buffer pointer to the next wchar string
      pwCurrFile += (length + 1);
      length = WcsLen(pwCurrFile);
    }

    // Go through files
    std::vector<MultiFileReaderFile *>::iterator itFiles = m_tsFiles.begin();
    std::vector<std::string>::iterator itFilenames = filenames.begin();

    while (itFiles < m_tsFiles.end())
    {
      file = *itFiles;

      ++itFiles;
      fileID++;

      if (itFilenames < filenames.end())
      {
        // TODO: Check that the filenames match. ( Ambass : With buffer integrity check, probably no need to do this !)
        ++itFilenames;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "MultiFileReader: Missing files!!\n");
      }
    }

    while (itFilenames < filenames.end())
    {
      std::string pFilename = *itFilenames;

      TSDEBUG(LOG_DEBUG, "%s: Adding file %s (%" PRId64 ")\n", __FUNCTION__, pFilename.c_str(), nextStartPosition);

      file = new MultiFileReaderFile();
      file->filename = pFilename;
      file->startPosition = nextStartPosition;

      fileID++;
      file->filePositionId = fileID;

      GetFileLength(file->filename.c_str(), file->length);

      m_tsFiles.push_back(file);

      nextStartPosition = file->startPosition + file->length;

      ++itFilenames;
    }

    m_filesAdded = filesAdded;
    m_filesRemoved = filesRemoved;

    delete[] pBuffer;
  }

  if (!m_tsFiles.empty())
  {
    file = m_tsFiles.front();
    m_startPosition = file->startPosition;
    // Since the buffer file may be re-used when a channel is changed, we
    // want the start position to reflect the position in the file after the last
    // channel change, or the real start position, whichever is larger
    if (m_lastZapPosition > m_startPosition)
    {
      m_startPosition = m_lastZapPosition;
    }

    file = m_tsFiles.back();
    file->length = currentPosition;
    m_endPosition = file->startPosition + currentPosition;
  
    TSDEBUG(LOG_DEBUG, "StartPosition %lli, EndPosition %lli, CurrentPosition %lli\n", m_startPosition, m_endPosition, m_currentPosition);
  }
  else
  {
    m_startPosition = 0;
    m_endPosition = 0;
  }

  return S_OK;
}

long MultiFileReader::GetFileLength(const char* pFilename, int64_t &length)
{
  //USES_CONVERSION;

  length = 0;

  // Try to open the file
  void* hFile;
  if (((hFile = XBMC->OpenFile(pFilename, 0)) != NULL))
  {
    length = XBMC->GetFileLength(hFile);
    XBMC->CloseFile(hFile);
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Failed to open file %s : 0x%x(%s)\n", pFilename, errno, strerror(errno));
    XBMC->QueueNotification(QUEUE_ERROR, "Failed to open file %s", pFilename);
    return S_FALSE;
  }
  return S_OK;
}

int64_t MultiFileReader::GetFileSize()
{
  RefreshTSBufferFile();
  return m_endPosition - m_startPosition;
}

int64_t MultiFileReader::OnChannelChange(void)
{
  m_lastZapPosition = m_currentPosition;
  return m_currentPosition;
}
