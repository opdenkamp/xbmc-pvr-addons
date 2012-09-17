#include <stdio.h>
#include <map>
#include <ctime>
#include <algorithm>

#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>
#include "cppmyth/MythFile.h"
#include "client.h"
#include "fileOps.h"
#include "stdio.h"

extern ADDON::CHelper_libXBMC_addon *XBMC;
using namespace ADDON;

fileOps2::fileOps2(MythConnection &mythConnection)
  :m_con(mythConnection),m_localBasePath(g_szUserPath.c_str()),m_sg_strings(),CThread(),CMutex(),m_queue_content(), m_jobqueue()
{
  m_localBasePath /= "cache";
  if(!createDirectory(m_localBasePath))
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    XBMC->Log(LOG_ERROR,"%s - Failed to create cache directory %s",__FUNCTION__,m_localBasePath.c_str());
#else
    XBMC->Log(LOG_ERROR,"%s - Failed to create cache directory %s",__FUNCTION__,m_localBasePath.native_file_string().c_str());
#endif
  m_sg_strings[FILE_OPS_GET_COVERART] = "coverart";
  m_sg_strings[FILE_OPS_GET_FANART] = "fanart";
  m_sg_strings[FILE_OPS_GET_BANNER] = "banner";
  m_sg_strings[FILE_OPS_GET_SCREENSHOT] = "screenshot";
  m_sg_strings[FILE_OPS_GET_POSTER] = "poster";
  m_sg_strings[FILE_OPS_GET_BACKCOVER] = "backcover";
  m_sg_strings[FILE_OPS_GET_INSIDECOVER] = "insidecover";
  m_sg_strings[FILE_OPS_GET_CD_IMAGE] = "cdimage";        
  m_sg_strings[FILE_OPS_GET_CHAN_ICONS] = "ChannelIcons";     
  CreateThread();
}

CStdString fileOps2::getChannelIconPath(CStdString remotePath)
{
  //Check local directory
  if(remotePath == "")
    return "";
  XBMC->Log(LOG_DEBUG,"%s: channelicon: %s",__FUNCTION__,remotePath.c_str());
  if(m_icons.count(remotePath)>0)
    return m_icons.at(remotePath);
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
  CStdString remoteFilename = boost::filesystem::path(remotePath.c_str()).filename().string();
#else
  CStdString remoteFilename = boost::filesystem::path(remotePath.c_str()).filename();
#endif
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
  boost::filesystem::path localFilePath = m_localBasePath / "channels" / remoteFilename.c_str();
#else
  CStdString localFilePathStr = m_localBasePath.native_file_string().c_str();
  localFilePathStr.append("/channels/");
  localFilePathStr.append(remoteFilename);
  boost::filesystem::path localFilePath = localFilePathStr;
#endif
  if(!boost::filesystem::exists(localFilePath))
  {      
    Lock();
    fileOps2::jobItem job(localFilePath, "/channels/"+remoteFilename,"");
    m_jobqueue.push(job);
    m_queue_content.Signal();
    Unlock();
  }
  m_icons[remotePath] = localFilePath.string();
  return localFilePath.string();

}

CStdString fileOps2::getPreviewIconPath(CStdString remotePath)
{
  //Check local directory
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s: preview icon: %s",__FUNCTION__,remotePath.c_str());
  if(m_preview.count(remotePath)>0)
    return m_preview.at(remotePath);
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
  CStdString remoteFilename = boost::filesystem::path(remotePath.c_str()).filename().string();
#else
  CStdString remoteFilename = boost::filesystem::path(remotePath.c_str()).filename();
#endif
  boost::filesystem::path localFilePath = m_localBasePath / "preview" / remoteFilename.c_str();
  if(!boost::filesystem::exists(localFilePath))
  {      
    Lock();
    fileOps2::jobItem job(localFilePath, remoteFilename,"Default");
    m_jobqueue.push(job);
    m_queue_content.Signal();
    Unlock();
  }
  m_preview[remotePath] = localFilePath.string();
  return localFilePath.string();
}

CStdString fileOps2::getArtworkPath(CStdString title,FILE_OPTIONS Get_What)
{
  CStdString retval;
  //update remote filelist
  time_t curTime;
  time(&curTime);
  if (m_SGFilelist.count(Get_What)==0||((int)curTime - m_lastSGupdate.at(Get_What)) > 30) { // Limit storage group updates to once every 30 seconds
    m_SGFilelist[Get_What] = m_con.GetStorageGroupFileList(m_sg_strings.at(Get_What));//=GetStorageGroupFiles(Get_What)
    //for(auto it=m_SGFilelist.at(Get_What).begin();it!=m_SGFilelist.at(Get_What).end();it++)XBMC->Log(LOG_DEBUG,"%s: Storagegroup %s, filename: %s",__FUNCTION__,m_sg_strings.at(Get_What).c_str(),it->Filename().c_str());
    m_lastSGupdate[Get_What] = curTime;
  }
  //check title against remote regex_match(title .*_storagegroup) 
  CStdString re_string;
  CStdString esctitle = boost::regex_replace(title,boost::regex("[\\.\\[\\{\\}\\(\\)\\\\\\*\\+\\?\\|\\^\\$]"),"\\\\$&");
  re_string.Format("%s.*_%s\\.(?:jpg|png|bmp)",esctitle,m_sg_strings.at(Get_What));    
  if(Get_What==FILE_OPS_GET_CHAN_ICONS)
  {
    boost::filesystem::path chanicon(title.c_str());
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
    re_string = chanicon.filename().string();
#else
    re_string = chanicon.filename();
#endif
  }
  boost::regex re(re_string.c_str());
  std::vector< MythSGFile >::iterator it = m_SGFilelist.at(Get_What).begin();
  for(;it!=m_SGFilelist.at(Get_What).end()&&!boost::regex_match(it->Filename(),re);it++);
  if(it==m_SGFilelist.at(Get_What).end())
  {
    //if no match return ""
    return "";
  }
  //check local file cache for up to date match
  CStdString localFilename;
  CStdString filename=it->Filename();
  localFilename.Format("%u_%s",it->LastModified(),filename);
  boost::filesystem::path localFilePath = m_localBasePath / m_sg_strings.at(Get_What).c_str() / localFilename.c_str();
  if(boost::filesystem::exists(localFilePath))
    return localFilePath.string();
  //else add to "files to fetch" and return expected path
  Lock();
  fileOps2::jobItem job(localFilePath, it->Filename(), m_sg_strings.at(Get_What));
  m_jobqueue.push(job);
  m_queue_content.Signal();
  Unlock();
  return localFilePath.string();
}

fileOps2::~fileOps2()
{
  cleanCache();
  StopThread(-1); //Set stopping. don't wait as we need to signal the thread first.
  m_queue_content.Signal();
  StopThread(); //wait for thread to stop;
}

void fileOps2::cleanCache()
{
  //Need to be redone at some time. Too much repeated code. But at least it works now.
  try{
  boost::regex re("^([[:digit:]]{1,20})_(.*)");
  boost::smatch match;
  for(std::map< FILE_OPTIONS, std::vector< MythSGFile > >::iterator it = m_SGFilelist.begin();it != m_SGFilelist.end(); it++)
  {
    boost::filesystem::path dirPath = m_localBasePath;
    dirPath /= m_sg_strings.at(it->first).c_str();     
    for(boost::filesystem::recursive_directory_iterator dit(dirPath);dit != boost::filesystem::recursive_directory_iterator();dit++)
    {
      bool deletefile = true;
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
      CStdString filename = dit->path().filename().string();    
#else
      CStdString filename = dit->path().filename();
#endif
      if(boost::regex_search(filename,match,re)&&match[0].matched)
      {
        std::string lastmodified = std::string(match[1].first,match[1].second);
        std::string title = std::string(match[2].first,match[2].second);
        for( std::vector< MythSGFile >::iterator mit = it->second.begin();mit != it->second.end(); mit++ )
        {
          CStdString mfilename = mit->Filename();
          unsigned int mlm = mit->LastModified();
          if(!mit->Filename().CompareNoCase(title.c_str())&&mit->LastModified()==atoi(lastmodified.c_str()))
          {
            if(boost::filesystem::file_size(dit->path())>0)
              deletefile = false;
            break;
          }
        }
        if(deletefile)
          boost::filesystem::remove(dit->path());
      }
    }
  }
  if(boost::filesystem::exists( m_localBasePath / "channels" ))
    for(boost::filesystem::recursive_directory_iterator dit( m_localBasePath / "channels");dit != boost::filesystem::recursive_directory_iterator();dit++)
    {
      bool deletefile = true;
      for(std::map< CStdString, CStdString >::iterator it = m_icons.begin(); it != m_icons.end(); it++ )
        if( !it->second.CompareNoCase(dit->path().string().c_str()))
        {
          if(boost::filesystem::file_size(dit->path())>0)
            deletefile = false;
          break;
        }
        if(deletefile)
          boost::filesystem::remove(dit->path());
    }
    if(boost::filesystem::exists( m_localBasePath / "preview" ))
      for(boost::filesystem::recursive_directory_iterator dit( m_localBasePath / "preview");dit != boost::filesystem::recursive_directory_iterator();dit++)
      {
        bool deletefile = true;
        for(std::map< CStdString, CStdString >::iterator it = m_preview.begin(); it != m_preview.end(); it++ )
          if( !it->second.CompareNoCase(dit->path().string().c_str()))
          {
            if(boost::filesystem::file_size(dit->path())>0)
              deletefile = false;
            break;
          }
          if(deletefile)
            boost::filesystem::remove(dit->path());
      }
  }
  catch(...)
  {
    XBMC->Log(LOG_ERROR,"%s, caught exception during cache cleaning",__FUNCTION__);
  }
}

void* fileOps2::Process()
{
  time_t curTime;
  time_t lastCacheClean;
  time(&lastCacheClean);

  while(!IsStopped())
  {
    m_queue_content.Wait(60*1000);
    while(!m_jobqueue.empty()&&!IsStopped())
    {
      Lock();
      fileOps2::jobItem job = m_jobqueue.front();
      m_jobqueue.pop();
      Unlock();
      try
      {
        XBMC->Log(LOG_DEBUG,"%s Job fetched: local: %s, remote: %s, storagegroup: %s",__FUNCTION__,job.localFilename.string().c_str(),job.remoteFilename.c_str(),job.storageGroup.c_str());
        m_con.Lock();
        MythFile file = m_con.ConnectPath(job.remoteFilename,job.storageGroup);
        if(!writeFile(job.localFilename,file))
          if(boost::filesystem::exists(job.localFilename))
            boost::filesystem::remove(job.localFilename);
        m_con.Unlock();
      }
      catch(...)
      {
        XBMC->Log(LOG_ERROR,"%s Error executing job: local: %s, remote: %s, storagegroup: %s",__FUNCTION__,job.localFilename.string().c_str(),job.remoteFilename.c_str(),job.storageGroup.c_str());
        m_con.Unlock();
      }
    }
    time(&curTime);
    if(curTime>lastCacheClean+60*60*24)
    {
      cleanCache();
      time(&lastCacheClean);
    }
  }
  return NULL;
}

bool fileOps2::writeFile(boost::filesystem::path destination, MythFile &source)
{
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
  CStdString parentPath = destination.parent_path().c_str();
#else
  CStdString parentPath = destination.parent_path().native_file_string().c_str();
#endif
  if(!createDirectory(destination,true))
  {
    XBMC->Log(LOG_ERROR,"%s - Failed to create destination directory: %s",
      __FUNCTION__,parentPath.c_str());
    return false;
  }
  if(source.IsNull())
  {
    XBMC->Log(LOG_ERROR,"%s - NULL file provided.",
      __FUNCTION__,parentPath.c_str());
    return false;
  }
  unsigned long long length = source.Duration(); 
  
  FILE*  writeFile = fopen(destination.string().c_str(),"w");
  //std::ofstream writeFile(destination.string(),std::ios_base::binary /*| std::ios_base::trunc*/);
  
  if (writeFile)
  {
    //char* theFileBuff = new char[theFilesLength];
    unsigned long long  totalRead = 0;
    unsigned int buffersize = 4096;
    char* buffer = new char[buffersize];
    long long  readsize = 1024;
    while (totalRead < length)
    {

      int readData = source.Read(buffer,readsize);
      if (readData <= 0)
      {
        break;
      }
      //writeFile.write(buffer,readData);
      fwrite(buffer,1,readData,writeFile);
      if(readsize == readData)
      {
        readsize <<=1;
        if(readsize > buffersize)
        {
          buffersize <<=1;
          delete buffer;
          buffer = new char[buffersize];
        }
      }
      totalRead += readData;
    }
    //writeFile.close();
    fclose(writeFile);
    writeFile = 0;
    delete buffer;
    if (totalRead < length) 
    {
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
      CStdString destinationStr = destination.c_str();
#else
      CStdString destinationStr = destination.native_file_string().c_str();
#endif
      XBMC->Log(LOG_DEBUG,"%s - Did not Read all data - %s - %d - %d",
        __FUNCTION__,destinationStr.c_str(),totalRead,length);
    }
    return true;
  }
  else 
  {
    XBMC->Log(LOG_ERROR,"%s - Failed to create destination file: %s",
      __FUNCTION__,destination.filename().c_str()); 
    return false;
  }
}

bool fileOps2::createDirectory(boost::filesystem::path dirPath, bool hasFilename/* = false */)
{
  if(hasFilename)
    return boost::filesystem::is_directory(dirPath.parent_path())?true:boost::filesystem::create_directory(dirPath.parent_path());
  else
    return boost::filesystem::is_directory(dirPath)?true:boost::filesystem::create_directory(dirPath);
}

