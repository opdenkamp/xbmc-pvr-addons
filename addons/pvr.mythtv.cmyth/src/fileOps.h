#ifndef __FILEOPS_H
#define __FILEOPS_H
//TODO merge into MythConnection ??

#include "platform/util/StdString.h"
#include <vector>
#include <map>
#include <queue>
#include "cppmyth/MythStorageGroupFile.h"
#include <platform/threads/threads.h>
#define BOOST_FILESYSTEM_NO_DEPRECATED
// Use v3, if it's available.
#if defined(BOOST_FILESYSTEM_VERSION)
#define BOOST_FILESYSTEM_VERSION 3
#endif
#include <boost/filesystem.hpp>



class MythConnection;

typedef enum
  {
    FILE_OPS_GET_COVERART,
    FILE_OPS_GET_FANART, 
    FILE_OPS_GET_CHAN_ICONS, //TODO: channel icons will be available as a storage group in v.0.25
    FILE_OPS_GET_BANNER,
    FILE_OPS_GET_SCREENSHOT,
    FILE_OPS_GET_POSTER,
    FILE_OPS_GET_BACKCOVER,
    FILE_OPS_GET_INSIDECOVER,
    FILE_OPS_GET_CD_IMAGE,
    FILE_OPS_GET_THUMBNAIL
  } FILE_OPTIONS;
  

class FileOps : public PLATFORM::CThread, public PLATFORM::CMutex
{
public:
  FileOps(MythConnection &mythConnection);
   virtual ~FileOps();
  CStdString getArtworkPath(CStdString title,FILE_OPTIONS Get_What);
  CStdString getChannelIconPath(CStdString remotePath);
  CStdString getPreviewIconPath(CStdString remotePath);
protected:
  bool createDirectory(boost::filesystem::path dirPath, bool hasFilename = false);
  bool localFileExists(MythStorageGroupFile &remotefile, boost::filesystem::path dirPath);
  void* Process();
  bool writeFile(boost::filesystem::path destination, MythFile &source); 
  void cleanCache();
  std::map< FILE_OPTIONS, std::vector< MythStorageGroupFile > > m_SGFilelist;
  std::map< FILE_OPTIONS, int > m_lastSGupdate;
  std::map< CStdString, CStdString > m_icons;
  std::map< CStdString, CStdString > m_preview;
  MythConnection m_con;
  boost::filesystem::path m_localBasePath;
  std::map< FILE_OPTIONS, CStdString > m_sg_strings;
  PLATFORM::CEvent m_queue_content;
  struct jobItem {
    boost::filesystem::path localFilename;
    CStdString remoteFilename;
    CStdString storageGroup;
    jobItem(boost::filesystem::path localFilename,CStdString remoteFilename,CStdString storageGroup):localFilename(localFilename),remoteFilename(remoteFilename),storageGroup(storageGroup){}    
  };
  std::queue< FileOps::jobItem > m_jobqueue;

};

#endif
