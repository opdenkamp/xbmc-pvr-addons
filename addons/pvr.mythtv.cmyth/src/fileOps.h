#pragma once
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "cppmyth/MythStorageGroupFile.h"

#include <platform/util/StdString.h>
#include <platform/threads/threads.h>

#include <vector>
#include <map>
#include <list>

class MythConnection;

class FileOps : public PLATFORM::CThread, public PLATFORM::CMutex
{
public:
  enum FileType
  {
    FileTypeCoverart,
    FileTypeFanart,
    FileTypeChannelIcon,
    FileTypeBanner,
    FileTypeScreenshot,
    FileTypePoster,
    FileTypeBackcover,
    FileTypeInsidecover,
    FileTypeCDImage,
    FileTypeThumbnail
  };

  static std::vector<FileType> GetFileTypes()
  {
    std::vector<FileType> ret;
    ret.push_back(FileTypeCoverart);
    ret.push_back(FileTypeFanart);
    ret.push_back(FileTypeChannelIcon);
    ret.push_back(FileTypeBanner);
    ret.push_back(FileTypeScreenshot);
    ret.push_back(FileTypePoster);
    ret.push_back(FileTypeBackcover);
    ret.push_back(FileTypeInsidecover);
    ret.push_back(FileTypeCDImage);
    ret.push_back(FileTypeThumbnail);
    return ret;
  }

  static const char *GetFolderNameByFileType(FileType fileType)
  {
    switch(fileType) {
    case FileTypeCoverart: return "coverart";
    case FileTypeFanart: return "fanart";
    case FileTypeChannelIcon: return "ChannelIcons";
    case FileTypeBanner: return "banner";
    case FileTypeScreenshot: return "screenshot";
    case FileTypePoster: return "poster";
    case FileTypeBackcover: return "backcover";
    case FileTypeInsidecover: return "insidecover";
    case FileTypeCDImage: return "cdimage";
      // FileTypeThumbnail: Thumbnails are located within their recording folders
    default: return "";
    }
  }

  static const int c_timeoutProcess       = 10;       // Wake the thread every 10s
  static const int c_timeoutCacheCleaning = 60*60*24; // Clean the cache every 24h

  FileOps(MythConnection &mythConnection);
  virtual ~FileOps();

  CStdString GetArtworkPath(const CStdString &title, FileType fileType);
  CStdString GetChannelIconPath(const CStdString &remotePath);
  CStdString GetPreviewIconPath(const CStdString &remotePath);

  void Suspend();
  void Resume();

protected:
  void* Process();

  bool CacheFile(const CStdString &destination, MythFile &source);
  void CleanCache();

  static CStdString GetFileName(const CStdString &path, char separator = PATH_SEPARATOR_CHAR);
  static CStdString GetDirectoryName(const CStdString &path, char separator = PATH_SEPARATOR_CHAR);

  std::map<FileType, StorageGroupFileList> m_StorageGroupFileList;
  std::map<FileType, std::time_t> m_StorageGroupFileListLastUpdated;
  std::map<CStdString, CStdString> m_icons;
  std::map<CStdString, CStdString> m_preview;
  MythConnection m_con;

  CStdString m_localBasePath;

  struct JobItem {
    JobItem(const CStdString &localFilename, const CStdString &remoteFilename, const CStdString &storageGroup)
      : m_localFilename(localFilename)
      , m_remoteFilename(remoteFilename)
      , m_storageGroup(storageGroup)
    {
    }

    CStdString m_localFilename;
    CStdString m_remoteFilename;
    CStdString m_storageGroup;
  };

  PLATFORM::CEvent m_queueContent;
  std::list<FileOps::JobItem> m_jobQueue;
};
