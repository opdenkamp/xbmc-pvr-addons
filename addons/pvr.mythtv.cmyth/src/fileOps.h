#ifndef __FILEOPS_H
#define __FILEOPS_H
//TODO merge into MythConnection ??

#include "utils/StdString.h"
#include <vector>
#include <map>
#include <queue>
#include "cppmyth/MythSGFile.h"
#include "../../../lib/platform/threads/threads.h"
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
  

class fileOps2 : public CThread, public CMutex
{
public:
  fileOps2(MythConnection &mythConnection);
   virtual ~fileOps2();
  CStdString getArtworkPath(CStdString title,FILE_OPTIONS Get_What);
  CStdString getChannelIconPath(CStdString remotePath);
  CStdString getPreviewIconPath(CStdString remotePath);
protected:
  bool createDirectory(boost::filesystem::path dirPath, bool hasFilename = false);
  bool localFileExists(MythSGFile &remotefile, boost::filesystem::path dirPath);
  void* Process();
  bool writeFile(boost::filesystem::path destination, MythFile &source); 
  void cleanCache();
  std::map< FILE_OPTIONS, std::vector< MythSGFile > > m_SGFilelist;
  std::map< FILE_OPTIONS, int > m_lastSGupdate;
  std::map< CStdString, CStdString > m_icons;
  std::map< CStdString, CStdString > m_preview;
  boost::filesystem::path m_localBasePath;
  MythConnection m_con;
  std::map< FILE_OPTIONS, CStdString > m_sg_strings;
  CEvent m_queue_content;
  struct jobItem {
    boost::filesystem::path localFilename;
    CStdString remoteFilename;
    CStdString storageGroup;
    jobItem(boost::filesystem::path localFilename,CStdString remoteFilename,CStdString storageGroup):localFilename(localFilename),remoteFilename(remoteFilename),storageGroup(storageGroup){}    
  };
  std::queue< fileOps2::jobItem > m_jobqueue;

};

#endif
