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
    XBMC->Log(LOG_DEBUG,"%s: determined localFilename: %s", __FUNCTION__, localFilename.c_str());

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

CStdString FileOps::GetPreviewIconPath(const CStdString &remoteFilename)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: preview icon: %s", __FUNCTION__, remoteFilename.c_str());

  // Check local directory
  std::map<CStdString, CStdString>::iterator it = m_preview.find(remoteFilename);
  if (it != m_preview.end())
    return it->second;

  // Determine local filename
  CStdString localFilename = m_localBasePath + "preview" + PATH_SEPARATOR_CHAR + GetFileName(remoteFilename, '/');
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s: determined localFilename: %s", __FUNCTION__, localFilename.c_str());

  if (!XBMC->FileExists(localFilename, true))
  {
    Lock();
    FileOps::JobItem job(localFilename, remoteFilename, "Default");
    m_jobQueue.push_back(job);
    m_queueContent.Signal();
    Unlock();
  }

  m_preview[remoteFilename] = localFilename;
  return localFilename;
}

CStdString FileOps::GetArtworkPath(const CStdString &title, FileType fileType)
{
  // Update storage group filelist only once every 30s
  std::time_t now = 0;

  // Update the list of files in a storage group if the filelist is empty
  // or the validity timeout has been reached (30s)
  if (m_StorageGroupFileList.find(fileType) == m_StorageGroupFileList.end() ||
    std::difftime(now, m_StorageGroupFileListLastUpdated[fileType]) > 30)
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s Getting storage group file list for type: %s (%s - %s)", __FUNCTION__, GetFolderNameByFileType(fileType), std::asctime(std::localtime(&now)), std::asctime(std::localtime(&m_StorageGroupFileListLastUpdated[fileType])));

    m_StorageGroupFileList[fileType] = m_con.GetStorageGroupFileList(GetFolderNameByFileType(fileType));
    m_StorageGroupFileListLastUpdated[fileType] = now;

    // Clean the file lists of unwanted files
    StorageGroupFileList::iterator it = m_StorageGroupFileList[fileType].begin();
    while (it != m_StorageGroupFileList[fileType].end())
    {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s %s - %s", __FUNCTION__, GetFolderNameByFileType(fileType), it->Filename().c_str());

      // Check file extension, if it's not an image, remove the file from storage group
      size_t extensionPos = it->Filename().find_last_of('.');
      CStdString extension = it->Filename().substr(extensionPos + 1);
      if (extension != "jpg" && extension != "png" && extension != "bmp")
      {
        it = m_StorageGroupFileList[fileType].erase(it);
        continue;
      }

      ++it;
    }
  }

  // Search in the storage group file list
  StorageGroupFileList::iterator it;
  for (it = m_StorageGroupFileList[fileType].begin(); it != m_StorageGroupFileList[fileType].end(); ++it)
  {
    if (it->Filename().Left(title.GetLength()) == title)
      break;
  }
  if (it == m_StorageGroupFileList[fileType].end())
  {
    return ""; // Not found
  }

  // Determine local filename
  CStdString localFilename;
  localFilename.Format("%u_%s", it->LastModified(), it->Filename());
  localFilename = m_localBasePath + GetFolderNameByFileType(fileType) + PATH_SEPARATOR_CHAR + localFilename.c_str();

  if (!XBMC->FileExists(localFilename, true))
  {
    Lock();
    FileOps::JobItem job(localFilename, it->Filename(), GetFolderNameByFileType(fileType));
    m_jobQueue.push_back(job);
    m_queueContent.Signal();
    Unlock();
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

  std::time_t lastRunCacheClean = std::time(NULL);
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
      m_con.Lock();

      MythFile file = m_con.ConnectPath(job.m_remoteFilename, job.m_storageGroup);
      if (file.Length() > 0)
      {
        if (CacheFile(job.m_localFilename.c_str(), file))
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
        // File was empty (this happens usually for new recordings where the preview image hasn't been generated)
        XBMC->Log(LOG_DEBUG, "%s File is empty, delayed recache: local: %s, remote: %s, type: %s", __FUNCTION__, job.m_localFilename.c_str(), job.m_remoteFilename.c_str(), job.m_storageGroup.c_str());
        jobQueueDelayed.push_back(job);
      }

      m_con.Unlock();
    }

    // Try to recache the currently empty files
    Lock();
    m_jobQueue.insert(m_jobQueue.end(), jobQueueDelayed.begin(), jobQueueDelayed.end());
    jobQueueDelayed.clear();
    Unlock();

    // Clean the cache from time to time
    std::time_t now = std::time(NULL);
    if (std::difftime(now, lastRunCacheClean) > (double)c_timeoutCacheCleaning)
    {
      CleanCache();
      lastRunCacheClean = now;
    }
  }

  XBMC->Log(LOG_DEBUG, "%s FileOps Thread Stopped", __FUNCTION__);
  return NULL;
}

bool FileOps::CacheFile(const CStdString &localFilename, MythFile &source)
{
  if (source.IsNull())
  {
    XBMC->Log(LOG_ERROR,"%s: NULL file provided", __FUNCTION__);
    return false;
  }

  if (source.Length() == 0)
  {
    XBMC->Log(LOG_ERROR,"%s: Empty file provided", __FUNCTION__);
    return false;
  }

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
        return false;
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s: Failed to create cache directory: %s", __FUNCTION__, cacheDirectory.c_str());
      return false;
    }
  }

  unsigned long long totalLength = source.Length();
  unsigned long long totalRead = 0;
  unsigned long long totalWrite = 0;

  const long buffersize = 4096;
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
      int bytes_written = XBMC->WriteFile(file, p, bytes_read);
      if (bytes_written <= 0)
        break;

      bytes_read -= bytes_written;
      p += bytes_written;
      totalWrite += bytes_written;
    }
  }

  XBMC->CloseFile(file);
  delete buffer;

  if (totalRead < totalLength)
  {
    XBMC->Log(LOG_DEBUG, "%s: Failed to read all data: %s (%d/%d)", __FUNCTION__, localFilename.c_str(), totalRead, totalLength);
  }

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
  directories.push_back(m_localBasePath + "channels");
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
