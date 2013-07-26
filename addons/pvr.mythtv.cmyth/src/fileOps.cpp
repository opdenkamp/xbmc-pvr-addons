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

#include "fileOps.h"
#include "client.h"

#include "cppmyth/MythFile.h"

#include <stdio.h>
#include <map>
#include <ctime>
#include <algorithm>

using namespace ADDON;

FileOps::FileOps(MythConnection &mythConnection)
  : CThread()
  , CMutex()
  , m_con(mythConnection)
  , m_localBasePath(g_szUserPath.c_str())
  , m_queueContent()
  , m_jobQueue()
{
  m_localBasePath = m_localBasePath + "cache" + PATH_SEPARATOR_CHAR;

  // Create the cache directories if it does not exist
  if (!XBMC->DirectoryExists(m_localBasePath) && !XBMC->CreateDirectory(m_localBasePath))
  {
    XBMC->Log(LOG_ERROR,"%s - Failed to create cache directory %s", __FUNCTION__, m_localBasePath.c_str());
  }

  CreateThread();
}

FileOps::~FileOps()
{
  CleanCache();
  StopThread(-1); // Set stopping. don't wait as we need to signal the thread first
  m_queueContent.Signal();
  StopThread(); // Wait for thread to stop
}

CStdString FileOps::GetChannelIconPath(const CStdString &remoteFilename)
{
  if (remoteFilename == "")
    return "";

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: channel icon: %s", __FUNCTION__, remoteFilename.c_str());

  // Check local directory
  std::map<CStdString, CStdString>::iterator it = m_icons.find(remoteFilename);
  if (it != m_icons.end())
    return it->second;

  // Determine filename
  CStdString localFilename = m_localBasePath + "channels" + PATH_SEPARATOR_CHAR + GetFileName(remoteFilename, '/');
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: determined localFilename: %s", __FUNCTION__, localFilename.c_str());

  if (!XBMC->FileExists(localFilename, true))
  {
    Lock();
    FileOps::JobItem job(localFilename, remoteFilename, "");
    m_jobQueue.push_back(job);
    m_queueContent.Signal();
    Unlock();
  }

  m_icons[remoteFilename] = localFilename;
  return localFilename;
}

CStdString FileOps::GetPreviewIconPath(const CStdString &remoteFilename, const CStdString &storageGroup)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: preview icon: %s", __FUNCTION__, remoteFilename.c_str());

  // Check local directory
  std::map<CStdString, CStdString>::iterator it = m_preview.find(remoteFilename);
  if (it != m_preview.end())
    return it->second;

  // Check file exists in storage group
  MythStorageGroupFile sgfile = m_con.GetStorageGroupFile(storageGroup, remoteFilename);

  // Determine local filename
  CStdString localFilename;
  if (!sgfile.IsNull())
  {
    localFilename = m_localBasePath + "preview" + PATH_SEPARATOR_CHAR + GetFileName(remoteFilename, '/');
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s: determined localFilename: %s", __FUNCTION__, localFilename.c_str());

    if (!XBMC->FileExists(localFilename, true))
    {
      Lock();
      FileOps::JobItem job(localFilename, remoteFilename, "Default");
      m_jobQueue.push_back(job);
      m_queueContent.Signal();
      Unlock();
    }

    m_preview[remoteFilename] = localFilename;
  }
  return localFilename;
}

CStdString FileOps::GetArtworkPath(const CStdString &remoteFilename, FileType fileType)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: filename: %s, fileType: %s", __FUNCTION__, remoteFilename.c_str(), GetFolderNameByFileType(fileType));

  // Check local directory
  std::pair<FileType, CStdString> key = std::make_pair(fileType, remoteFilename);
  std::map<std::pair<FileType, CStdString>, CStdString>::iterator iter = m_artworks.find(key);
  if (iter != m_artworks.end())
    return iter->second;

  // Check file exists in storage group
  MythStorageGroupFile sgfile = m_con.GetStorageGroupFile(GetFolderNameByFileType(fileType), remoteFilename);

  // Determine local filename
  CStdString localFilename;
  if (!sgfile.IsNull())
  {
    localFilename = m_localBasePath + GetFolderNameByFileType(fileType) + PATH_SEPARATOR_CHAR + remoteFilename.c_str();

    if (!XBMC->FileExists(localFilename, true))
    {
      Lock();
        FileOps::JobItem job(localFilename, remoteFilename, GetFolderNameByFileType(fileType));
        m_jobQueue.push_back(job);
        m_queueContent.Signal();
      Unlock();
    }
    m_artworks[key] = localFilename;
  }

  return localFilename;
}

void FileOps::Suspend()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  if (IsRunning())
  {
    XBMC->Log(LOG_DEBUG, "%s Stopping Thread", __FUNCTION__);
    StopThread(-1); // Set stopping. don't wait as we need to signal the thread first
    m_queueContent.Signal();
    StopThread(); // Wait for thread to stop
  }
}

void FileOps::Resume()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  if (IsStopped())
  {
    XBMC->Log(LOG_DEBUG, "%s Resuming Thread", __FUNCTION__);
    Clear();
    CreateThread();
  }
}

void* FileOps::Process()
{
  XBMC->Log(LOG_DEBUG, "%s FileOps Thread Started", __FUNCTION__);

  std::list<FileOps::JobItem> jobQueueDelayed;

  while (!IsStopped())
  {
    // Wake this thread from time to time to clean the cache and recache empty files (delayed queue)
    // For caching new files, the tread is woken up by m_queueContent.Signal();
    m_queueContent.Wait(c_timeoutProcess * 1000);

    while (!m_jobQueue.empty() && !IsStopped())
    {
      Lock();
      FileOps::JobItem job = m_jobQueue.front();
      m_jobQueue.pop_front();
      Unlock();

      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG,"%s Job fetched: local: %s, remote: %s, storagegroup: %s", __FUNCTION__, job.m_localFilename.c_str(), job.m_remoteFilename.c_str(), job.m_storageGroup.c_str());

      // Try to open the destination file
      void *localFile = OpenFile(job.m_localFilename.c_str());
      if (!localFile)
        continue;

      // Connect to the file and cache it to the local addon cache
      MythFile remoteFile = m_con.ConnectPath(job.m_remoteFilename, job.m_storageGroup);
      if (!remoteFile.IsNull() && remoteFile.Length() > 0)
      {
        if (CacheFile(localFile, remoteFile))
        {
          if (g_bExtraDebug)
            XBMC->Log(LOG_DEBUG, "%s File Cached: local: %s, remote: %s, type: %s", __FUNCTION__, job.m_localFilename.c_str(), job.m_remoteFilename.c_str(), job.m_storageGroup.c_str());
        }
        else
        {
          XBMC->Log(LOG_DEBUG, "%s Caching file failed: local: %s, remote: %s, type: %s", __FUNCTION__, job.m_localFilename.c_str(), job.m_remoteFilename.c_str(), job.m_storageGroup.c_str());
          if (XBMC->FileExists(job.m_localFilename.c_str(), true))
          {
            XBMC->DeleteFile(job.m_localFilename.c_str());
          }
        }
      }
      else
      {
        // Failed to open file for reading. Unfortunately it cannot be determined if this is a permanent or a temporary problem (new recording's preview hasn't been generated yet).
        // Increase the error count and retry to cache the file a few times
        if (remoteFile.IsNull())
        {
          XBMC->Log(LOG_ERROR, "%s Failed to read file: local: %s, remote: %s, type: %s", __FUNCTION__, job.m_localFilename.c_str(), job.m_remoteFilename.c_str(), job.m_storageGroup.c_str());
          job.m_errorCount += 1;
        }

        // File was empty (this happens usually for new recordings where the preview image hasn't been generated yet)
        // This is not an error, always try to recache the file
        else if (remoteFile.Length() == 0)
        {
          XBMC->Log(LOG_DEBUG, "%s File is empty: local: %s, remote: %s, type: %s", __FUNCTION__, job.m_localFilename.c_str(), job.m_remoteFilename.c_str(), job.m_storageGroup.c_str());
        }

        // Recache the file if it hasn't exceeded the maximum number of allowed attempts
        if (job.m_errorCount <= c_maximumAttemptsOnReadError)
        {
          XBMC->Log(LOG_DEBUG, "%s Delayed recache file: local: %s, remote: %s, type: %s", __FUNCTION__, job.m_localFilename.c_str(), job.m_remoteFilename.c_str(), job.m_storageGroup.c_str());
          jobQueueDelayed.push_back(job);
        }
      }
    }

    // Try to recache the currently empty files
    Lock();
    m_jobQueue.insert(m_jobQueue.end(), jobQueueDelayed.begin(), jobQueueDelayed.end());
    jobQueueDelayed.clear();
    Unlock();
  }

  XBMC->Log(LOG_DEBUG, "%s FileOps Thread Stopped", __FUNCTION__);
  return NULL;
}

void *FileOps::OpenFile(const CStdString &localFilename)
{
  // Try to open the file. If it fails, check if we need to create the directory first.
  // This way we avoid checking if the directory exists every time.
  void *file;
  if (!(file = XBMC->OpenFileForWrite(localFilename.c_str(), true)))
  {
    CStdString cacheDirectory = GetDirectoryName(localFilename);
    if (XBMC->DirectoryExists(cacheDirectory.c_str()) || XBMC->CreateDirectory(cacheDirectory.c_str()))
    {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s: Created cache directory: %s", __FUNCTION__, cacheDirectory.c_str());

      if (!(file = XBMC->OpenFileForWrite(localFilename.c_str(), true)))
      {
        XBMC->Log(LOG_ERROR, "%s: Failed to create cache file: %s", __FUNCTION__, localFilename.c_str());
        return NULL;
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s: Failed to create cache directory: %s", __FUNCTION__, cacheDirectory.c_str());
      return NULL;
    }
  }
  return file;
}

bool FileOps::CacheFile(void* destination, MythFile &source)
{
  unsigned long long totalLength = source.Length();
  unsigned long long totalRead = 0;

  const long buffersize = RCV_BUF_IMAGE_SIZE;
  char* buffer = new char[buffersize];

  while (totalRead < totalLength)
  {
    int bytes_read = source.Read(buffer, buffersize);
    if (bytes_read <= 0)
      break;

    totalRead += bytes_read;

    char *p = buffer;
    while (bytes_read > 0)
    {
      int bytes_written = XBMC->WriteFile(destination, p, bytes_read);
      if (bytes_written <= 0)
        break;

      bytes_read -= bytes_written;
      p += bytes_written;
    }
  }

  XBMC->CloseFile(destination);
  delete[] buffer;

  if (totalRead < totalLength)
    XBMC->Log(LOG_DEBUG, "%s: Failed to read all data: (%d/%d)", __FUNCTION__, totalRead, totalLength);

  return true;
}

void FileOps::CleanCache()
{
  // Currently XBMC's addon lib doesn't provide a way to list files in a directory.
  // Therefore we currently can't clean only leftover files.

  XBMC->Log(LOG_DEBUG, "%s Cleaning cache %s", __FUNCTION__, m_localBasePath.c_str());

  Lock();

  // Remove cache sub directories
  std::vector<FileType>::const_iterator it;
  std::vector<FileType> fileTypes = GetFileTypes();
  std::vector<CStdString> directories;
  for (it = fileTypes.begin(); it != fileTypes.end(); ++it)
  {
    CStdString directory(GetFolderNameByFileType(*it));
    if (!directory.IsEmpty())
      directories.push_back(m_localBasePath + directory);
  }
  directories.push_back(m_localBasePath + "preview");
  std::vector<CStdString>::const_iterator it2;
  for (it2 = directories.begin(); it2 != directories.end(); ++it2)
  {
    if (XBMC->DirectoryExists(it2->c_str()) && !XBMC->RemoveDirectory(it2->c_str()))
    {
      XBMC->Log(LOG_ERROR,"%s - Failed to remove cache sub directory %s", __FUNCTION__, it2->c_str());
    }
  }

  // Clear the cached local filenames so that new cache jobs get generated
  m_icons.clear();
  m_preview.clear();

  Unlock();

  XBMC->Log(LOG_DEBUG, "%s Cleaned cache %s", __FUNCTION__, m_localBasePath.c_str());
}

CStdString FileOps::GetFileName(const CStdString &path, char separator)
{
  size_t pos = path.find_last_of(separator);
  return path.substr(pos + 1);
}

CStdString FileOps::GetDirectoryName(const CStdString &path, char separator)
{
  size_t pos = path.find_last_of(separator);
  return path.substr(0, pos);
}
