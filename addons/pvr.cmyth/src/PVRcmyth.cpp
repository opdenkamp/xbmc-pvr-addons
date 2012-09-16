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

#include "PVRcmyth.h"
#include "tools.h"
#include <cmyth/cmyth.h>

using namespace std;
using namespace ADDON;

PVRcmyth::PVRcmyth(void)
  :m_con(),m_pEventHandler(NULL),m_db(),m_protocolVersion(""),m_connectionString(""),m_EPGstart(0),m_EPGend(0),m_groups(),m_categoryMap(),m_fOps2_client(0)
{
  m_strDefaultIcon =  "http://www.royalty-free.tv/news/wp-content/uploads/2011/06/cc-logo1.jpg";
  m_strDefaultMovie = "";

  //LoadDemoData();
  LoadCategoryMap();
}

PVRcmyth::~PVRcmyth(void)
{
  m_channels.clear();
  m_groups.clear();
  
  //NEW
  if(m_fOps2_client)
  {
    delete m_fOps2_client;
    m_fOps2_client = 0;
  }
  if ( m_pEventHandler )
  {
    m_pEventHandler->Stop();
    delete ( m_pEventHandler );
    m_pEventHandler = NULL;
  } 
}

CStdString PVRcmyth::GetArtWork(FILE_OPTIONS storageGroup, CStdString shwTitle) {
  if ((storageGroup == FILE_OPS_GET_COVERART) || 
    (storageGroup == FILE_OPS_GET_FANART))
  {
    return m_fOps2_client->getArtworkPath(shwTitle,storageGroup);
  } 
  else if(storageGroup == FILE_OPS_GET_CHAN_ICONS)
  {
    return m_fOps2_client->getChannelIconPath(shwTitle);
  }
  else 
  {
    XBMC->Log(LOG_DEBUG,"%s - ## Not a valid storageGroup ##",__FUNCTION__);
    return "";
  }

}

int PVRcmyth::Genre(CStdString g)
{
  int retval=0;
  //XBMC->Log(LOG_DEBUG,"- %s - ## Genre ## - %s -",__FUNCTION__,g.c_str());
  try{
    if(m_categoryMap.by< mythcat >().count(g))
      retval=m_categoryMap.by< mythcat >().at(g);

  }
  catch(std::out_of_range){}
  //XBMC->Log(LOG_DEBUG,"- %s - ## Genre ## - ret: %d -",__FUNCTION__,retval);
  return retval;
}

CStdString PVRcmyth::Genre(int g)
{
  CStdString retval="";
  try{
    if(m_categoryMap.by< pvrcat >().count(g))
      retval=m_categoryMap.by< pvrcat >().at(g);
  }
  catch(std::out_of_range){}
  return retval;
}

void Log(int l,char* msg)
{

  if(msg&&l!=CMYTH_DBG_NONE)
  {
    bool doLog=g_bExtraDebug;
    addon_log_t loglevel=LOG_DEBUG;
    switch( l)
    {
    case CMYTH_DBG_ERROR:
      loglevel=LOG_ERROR;
      doLog=true;
      break;
    case CMYTH_DBG_WARN:
    case CMYTH_DBG_INFO:
      loglevel=LOG_INFO;
      break;
    case CMYTH_DBG_DETAIL:
    case CMYTH_DBG_DEBUG:
    case CMYTH_DBG_PROTO:
    case CMYTH_DBG_ALL: 
      loglevel=LOG_DEBUG;
      break;
    }    
    if(XBMC&&doLog)
      XBMC->Log(loglevel,"LibCMyth: %s",  msg);
  }
}

bool PVRcmyth::Connect()
{
  if(g_bExtraDebug)
    cmyth_dbg_all();
  else
    cmyth_dbg_level(CMYTH_DBG_ERROR);
  cmyth_set_dbg_msgcallback(Log);
  m_con=MythConnection(g_strHostname,g_iMythPort);
  if(!m_con.IsConnected())
  {
    XBMC->QueueNotification(QUEUE_ERROR,"%s: Failed to connect to MythTV backend %s:%i",__FUNCTION__,g_strHostname.c_str(),g_iMythPort);
    return false;
  }
  m_pEventHandler=m_con.CreateEventHandler();
  if ( !m_pEventHandler )
  {
    XBMC->QueueNotification( QUEUE_ERROR, "Failed to create MythTV Event Handler" );
    return false;
  }
  m_protocolVersion.Format("%i",m_con.GetProtocolVersion());
  m_connectionString.Format("%s:%i",g_strHostname,g_iMythPort);
  m_fOps2_client = new fileOps2(m_con);
  m_db=MythDatabase(g_strHostname,g_strMythDBname,g_strMythDBuser,g_strMythDBpassword);
  if(m_db.IsNull())
  {
    XBMC->QueueNotification(QUEUE_ERROR,"Failed to connect to MythTV MySQL database %s@%s %s/%s",g_strMythDBname.c_str(),g_strHostname.c_str(),g_strMythDBuser.c_str(),g_strMythDBpassword.c_str());
    if ( m_pEventHandler )
    {
      m_pEventHandler->Stop();
    }
    return false;
  }
  CStdString db_test;
  if(!m_db.TestConnection(db_test))
  {
    XBMC->QueueNotification(QUEUE_ERROR,"Failed to connect to MythTV MySQL database %s@%s %s/%s \n %s",g_strMythDBname.c_str(),g_strHostname.c_str(),g_strMythDBuser.c_str(),g_strMythDBpassword.c_str(),db_test.c_str());
    if ( m_pEventHandler )
    {
      m_pEventHandler->Stop();
    }
    return false;
  }
  m_channels=m_db.ChannelList();
  if(m_channels.size()==0)
    XBMC->Log(LOG_INFO,"%s: Empty channellist",__FUNCTION__);
  m_sources=m_db.SourceList();
  if(m_sources.size()==0)
    XBMC->Log(LOG_INFO,"%s: Empty source list",__FUNCTION__);

  m_groups=m_db.GetChannelGroups();
  if(m_groups.size()==0)
    XBMC->Log(LOG_INFO,"%s: No channelgroups",__FUNCTION__);

  std::vector<MythRecordingProfile > rp = m_db.GetRecordingProfiles();

  return true;
}

bool PVRcmyth::LoadCategoryMap(void)
{
  m_categoryMap.insert(catbimap::value_type("Movie",0x10));
  m_categoryMap.insert(catbimap::value_type("Movie", 0x10));
  m_categoryMap.insert(catbimap::value_type("Movie - Detective/Thriller", 0x11));
  m_categoryMap.insert(catbimap::value_type("Movie - Adventure/Western/War", 0x12));
  m_categoryMap.insert(catbimap::value_type("Movie - Science Fiction/Fantasy/Horror", 0x13));
  m_categoryMap.insert(catbimap::value_type("Movie - Comedy", 0x14));
  m_categoryMap.insert(catbimap::value_type("Movie - Soap/melodrama/folkloric", 0x15));
  m_categoryMap.insert(catbimap::value_type("Movie - Romance", 0x16));
  m_categoryMap.insert(catbimap::value_type("Movie - Serious/Classical/Religious/Historical Movie/Drama", 0x17));
  m_categoryMap.insert(catbimap::value_type("Movie - Adult   ", 0x18));
  m_categoryMap.insert(catbimap::value_type("Drama", 0x1F));//MythTV use 0xF0 but xbmc doesn't implement this.
  m_categoryMap.insert(catbimap::value_type("News", 0x20));
  m_categoryMap.insert(catbimap::value_type("News/weather report", 0x21));
  m_categoryMap.insert(catbimap::value_type("News magazine", 0x22));
  m_categoryMap.insert(catbimap::value_type("Documentary", 0x23));
  m_categoryMap.insert(catbimap::value_type("Intelligent Programmes", 0x24));
  m_categoryMap.insert(catbimap::value_type("Entertainment", 0x30));
  m_categoryMap.insert(catbimap::value_type("Game Show", 0x31));
  m_categoryMap.insert(catbimap::value_type("Variety Show", 0x32));
  m_categoryMap.insert(catbimap::value_type("Talk Show", 0x33));
  m_categoryMap.insert(catbimap::value_type("Sports", 0x40));
  m_categoryMap.insert(catbimap::value_type("Special Events (World Cup, World Series, etc)", 0x41));
  m_categoryMap.insert(catbimap::value_type("Sports Magazines", 0x42));
  m_categoryMap.insert(catbimap::value_type("Football (Soccer)", 0x43));
  m_categoryMap.insert(catbimap::value_type("Tennis/Squash", 0x44));
  m_categoryMap.insert(catbimap::value_type("Misc. Team Sports", 0x45));
  m_categoryMap.insert(catbimap::value_type("Athletics", 0x46));
  m_categoryMap.insert(catbimap::value_type("Motor Sport", 0x47));
  m_categoryMap.insert(catbimap::value_type("Water Sport", 0x48));
  m_categoryMap.insert(catbimap::value_type("Winter Sports", 0x49));
  m_categoryMap.insert(catbimap::value_type("Equestrian", 0x4A));
  m_categoryMap.insert(catbimap::value_type("Martial Sports", 0x4B));
  m_categoryMap.insert(catbimap::value_type("Kids", 0x50));
  m_categoryMap.insert(catbimap::value_type("Pre-School Children's Programmes", 0x51));
  m_categoryMap.insert(catbimap::value_type("Entertainment Programmes for 6 to 14", 0x52));
  m_categoryMap.insert(catbimap::value_type("Entertainment Programmes for 10 to 16", 0x53));
  m_categoryMap.insert(catbimap::value_type("Informational/Educational", 0x54));
  m_categoryMap.insert(catbimap::value_type("Cartoons/Puppets", 0x55));
  m_categoryMap.insert(catbimap::value_type("Music/Ballet/Dance", 0x60));
  m_categoryMap.insert(catbimap::value_type("Rock/Pop", 0x61));
  m_categoryMap.insert(catbimap::value_type("Classical Music", 0x62));
  m_categoryMap.insert(catbimap::value_type("Folk Music", 0x63));
  m_categoryMap.insert(catbimap::value_type("Jazz", 0x64));
  m_categoryMap.insert(catbimap::value_type("Musical/Opera", 0x65));
  m_categoryMap.insert(catbimap::value_type("Ballet", 0x66));
  m_categoryMap.insert(catbimap::value_type("Arts/Culture", 0x70));
  m_categoryMap.insert(catbimap::value_type("Performing Arts", 0x71));
  m_categoryMap.insert(catbimap::value_type("Fine Arts", 0x72));
  m_categoryMap.insert(catbimap::value_type("Religion", 0x73));
  m_categoryMap.insert(catbimap::value_type("Popular Culture/Traditional Arts", 0x74));
  m_categoryMap.insert(catbimap::value_type("Literature", 0x75));
  m_categoryMap.insert(catbimap::value_type("Film/Cinema", 0x76));
  m_categoryMap.insert(catbimap::value_type("Experimental Film/Video", 0x77));
  m_categoryMap.insert(catbimap::value_type("Broadcasting/Press", 0x78));
  m_categoryMap.insert(catbimap::value_type("New Media", 0x79));
  m_categoryMap.insert(catbimap::value_type("Arts/Culture Magazines", 0x7A));
  m_categoryMap.insert(catbimap::value_type("Fashion", 0x7B));
  m_categoryMap.insert(catbimap::value_type("Social/Policical/Economics", 0x80));
  m_categoryMap.insert(catbimap::value_type("Magazines/Reports/Documentary", 0x81));
  m_categoryMap.insert(catbimap::value_type("Economics/Social Advisory", 0x82));
  m_categoryMap.insert(catbimap::value_type("Remarkable People", 0x83));
  m_categoryMap.insert(catbimap::value_type("Education/Science/Factual", 0x90));
  m_categoryMap.insert(catbimap::value_type("Nature/animals/Environment", 0x91));
  m_categoryMap.insert(catbimap::value_type("Technology/Natural Sciences", 0x92));
  m_categoryMap.insert(catbimap::value_type("Medicine/Physiology/Psychology", 0x93));
  m_categoryMap.insert(catbimap::value_type("Foreign Countries/Expeditions", 0x94));
  m_categoryMap.insert(catbimap::value_type("Social/Spiritual Sciences", 0x95));
  m_categoryMap.insert(catbimap::value_type("Further Education", 0x96));
  m_categoryMap.insert(catbimap::value_type("Languages", 0x97));
  m_categoryMap.insert(catbimap::value_type("Leisure/Hobbies", 0xA0));
  m_categoryMap.insert(catbimap::value_type("Tourism/Travel", 0xA1));
  m_categoryMap.insert(catbimap::value_type("Handicraft", 0xA2));
  m_categoryMap.insert(catbimap::value_type("Motoring", 0xA3));
  m_categoryMap.insert(catbimap::value_type("Fitness & Health", 0xA4));
  m_categoryMap.insert(catbimap::value_type("Cooking", 0xA5));
  m_categoryMap.insert(catbimap::value_type("Advertizement/Shopping", 0xA6));
  m_categoryMap.insert(catbimap::value_type("Gardening", 0xA7));
  m_categoryMap.insert(catbimap::value_type("Original Language", 0xB0));
  m_categoryMap.insert(catbimap::value_type("Black & White", 0xB1));
  m_categoryMap.insert(catbimap::value_type("\"Unpublished\" Programmes", 0xB2));
  m_categoryMap.insert(catbimap::value_type("Live Broadcast", 0xB3));

  m_categoryMap.insert(catbimap::value_type("Community", 0));
  m_categoryMap.insert(catbimap::value_type("Fundraiser", 0));
  m_categoryMap.insert(catbimap::value_type("Bus./financial", 0));
  m_categoryMap.insert(catbimap::value_type("Variety", 0));
  m_categoryMap.insert(catbimap::value_type("Romance-comedy", 0xC6));
  m_categoryMap.insert(catbimap::value_type("Sports event", 0x40));
  m_categoryMap.insert(catbimap::value_type("Sports talk", 0x40));
  m_categoryMap.insert(catbimap::value_type("Computers", 0x92));
  m_categoryMap.insert(catbimap::value_type("How-to", 0xA2));
  m_categoryMap.insert(catbimap::value_type("Religious", 0x73));
  m_categoryMap.insert(catbimap::value_type("Parenting", 0));
  m_categoryMap.insert(catbimap::value_type("Art", 0x70));
  m_categoryMap.insert(catbimap::value_type("Musical comedy", 0x64));
  m_categoryMap.insert(catbimap::value_type("Environment", 0x91));
  m_categoryMap.insert(catbimap::value_type("Politics", 0x80));
  m_categoryMap.insert(catbimap::value_type("Animated", 0x55));
  m_categoryMap.insert(catbimap::value_type("Gaming", 0));
  m_categoryMap.insert(catbimap::value_type("Interview", 0x24));
  m_categoryMap.insert(catbimap::value_type("Historical drama", 0xC7));
  m_categoryMap.insert(catbimap::value_type("Biography", 0));
  m_categoryMap.insert(catbimap::value_type("Home improvement", 0));
  m_categoryMap.insert(catbimap::value_type("Hunting", 0xA0));
  m_categoryMap.insert(catbimap::value_type("Outdoors", 0xA0));
  m_categoryMap.insert(catbimap::value_type("Auto", 0x47));
  m_categoryMap.insert(catbimap::value_type("Auto racing", 0x47));
  m_categoryMap.insert(catbimap::value_type("Horror", 0xC4));
  m_categoryMap.insert(catbimap::value_type("Medical", 0x93));
  m_categoryMap.insert(catbimap::value_type("Romance", 0xC6));
  m_categoryMap.insert(catbimap::value_type("Spanish", 0x97));
  m_categoryMap.insert(catbimap::value_type("Adults only", 0xC8));
  m_categoryMap.insert(catbimap::value_type("Musical", 0x64));
  m_categoryMap.insert(catbimap::value_type("Self improvement", 0xA0));
  m_categoryMap.insert(catbimap::value_type("Pro wrestling", 0x40));
  m_categoryMap.insert(catbimap::value_type("Wrestling", 0x40));
  m_categoryMap.insert(catbimap::value_type("Fishing", 0));
  m_categoryMap.insert(catbimap::value_type("Agriculture", 0));
  m_categoryMap.insert(catbimap::value_type("Arts/crafts", 0x70));
  m_categoryMap.insert(catbimap::value_type("Technology", 0x92));
  m_categoryMap.insert(catbimap::value_type("Docudrama", 0xC0));
  m_categoryMap.insert(catbimap::value_type("Science fiction", 0xC3));
  m_categoryMap.insert(catbimap::value_type("Paranormal", 0));
  m_categoryMap.insert(catbimap::value_type("Comedy", 0xC4));
  m_categoryMap.insert(catbimap::value_type("Science", 0));
  m_categoryMap.insert(catbimap::value_type("Travel", 0));
  m_categoryMap.insert(catbimap::value_type("Adventure", 0));
  m_categoryMap.insert(catbimap::value_type("Suspense", 0xC1));
  m_categoryMap.insert(catbimap::value_type("History", 0));
  m_categoryMap.insert(catbimap::value_type("Collectibles", 0));
  m_categoryMap.insert(catbimap::value_type("Crime", 0));
  m_categoryMap.insert(catbimap::value_type("French", 0));
  m_categoryMap.insert(catbimap::value_type("House/garden", 0));
  m_categoryMap.insert(catbimap::value_type("Action", 0));
  m_categoryMap.insert(catbimap::value_type("Fantasy", 0));
  m_categoryMap.insert(catbimap::value_type("Mystery", 0));
  m_categoryMap.insert(catbimap::value_type("Health", 0));
  m_categoryMap.insert(catbimap::value_type("Comedy-drama", 0));
  m_categoryMap.insert(catbimap::value_type("Special", 0));
  m_categoryMap.insert(catbimap::value_type("Holiday", 0));
  m_categoryMap.insert(catbimap::value_type("Weather", 0));
  m_categoryMap.insert(catbimap::value_type("Western", 0));
  m_categoryMap.insert(catbimap::value_type("Children", 0));
  m_categoryMap.insert(catbimap::value_type("Nature", 0));
  m_categoryMap.insert(catbimap::value_type("Animals", 0));
  m_categoryMap.insert(catbimap::value_type("Public affairs", 0));
  m_categoryMap.insert(catbimap::value_type("Educational", 0));
  m_categoryMap.insert(catbimap::value_type("Shopping", 0xA6));
  m_categoryMap.insert(catbimap::value_type("Consumer", 0));
  m_categoryMap.insert(catbimap::value_type("Soap", 0));
  m_categoryMap.insert(catbimap::value_type("Newsmagazine", 0));
  m_categoryMap.insert(catbimap::value_type("Exercise", 0));
  m_categoryMap.insert(catbimap::value_type("Music", 0x60));
  m_categoryMap.insert(catbimap::value_type("Game show", 0));
  m_categoryMap.insert(catbimap::value_type("Sitcom", 0));
  m_categoryMap.insert(catbimap::value_type("Talk", 0));
  m_categoryMap.insert(catbimap::value_type("Crime drama", 0));
  m_categoryMap.insert(catbimap::value_type("Sports non-event", 0x40));
  m_categoryMap.insert(catbimap::value_type("Reality", 0));

  return true;
}

bool PVRcmyth::GetLiveTVPriority()
{
  if(!m_con.IsNull())
  {
    CStdString value;
    value = m_con.GetSetting(m_con.GetHostname(),"LiveTVPriority");

    if( value.compare("1")==0)
      return true;
    else
      return false;
  }
  return false;
}

void PVRcmyth::SetLiveTVPriority(bool enabled)
{
  if(!m_con.IsNull())
  {
    CStdString value = enabled? "1" : "0";
    m_con.SetSetting(m_con.GetHostname(),"LiveTVPriority",value);
  }
}

/***********************************************************
 * PVR Client AddOn Channels library functions
 ***********************************************************/
int PVRcmyth::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR PVRcmyth::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - radio: %i",__FUNCTION__,bRadio);
  for (std::map< int, MythChannel >::iterator it = m_channels.begin(); it != m_channels.end(); it++)
  {
    if (it->second.IsRadio()==bRadio&&!it->second.IsNull())
    {
      PVR_CHANNEL tag;
      tag.bIsHidden=!it->second.Visible();
      tag.bIsRadio=it->second.IsRadio();
      tag.iUniqueId=it->first;
      tag.iChannelNumber=it->second.NumberInt(); //Use ID instead as mythtv channel number is a string?
      PVR_STRCPY(tag.strChannelName,it->second.Name());
      PVR_STRCPY(tag.strIconPath,GetArtWork(FILE_OPS_GET_CHAN_ICONS,it->second.Icon()));
      //Unimplemented
      PVR_STRCLR(tag.strStreamURL);
      PVR_STRCLR(tag.strInputFormat);
      tag.iEncryptionSystem=0;


      PVR->TransferChannelEntry(handle,&tag);
    }
  }
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
  return PVR_ERROR_NO_ERROR;
}

bool PVRcmyth::GetChannel(const PVR_CHANNEL &channel, PVRcmythChannel &myChannel)
{
  for (std::map< int, MythChannel >::iterator it = m_channels.begin(); it != m_channels.end(); it++)
  {
    if ((unsigned int)it->first == channel.iUniqueId)
    {
      myChannel.iUniqueId         = it->first;
      myChannel.bRadio            = it->second.IsRadio();
      myChannel.iChannelNumber    = it->second.NumberInt();
      myChannel.strChannelName    = it->second.Name();
      myChannel.strIconPath       = GetArtWork(FILE_OPS_GET_CHAN_ICONS,it->second.Icon());
      //Unimplemented
      myChannel.strStreamURL      = "";
      myChannel.iEncryptionSystem = 0;
      return true;
    }
  }
  return false;
}

int PVRcmyth::GetChannelGroupsAmount(void)
{
  return m_groups.size();
}

PVR_ERROR PVRcmyth::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - radio: %i",__FUNCTION__,bRadio);
  PVR_CHANNEL_GROUP tag;
  for(boost::unordered_map< CStdString, std::vector< int > >::iterator it=m_groups.begin();it!=m_groups.end();it++)
  {
    //tag.strGroupName=it->first;
    PVR_STRCPY(tag.strGroupName,it->first);
    tag.bIsRadio=bRadio;
    for(std::vector< int >::iterator it2=it->second.begin();it2!=it->second.end();it2++)
      if(m_channels.find(*it2)!=m_channels.end()&&m_channels.at(*it2).IsRadio()==bRadio)
      {
        PVR->TransferChannelGroup(handle,&tag);
        break;
      }
  }
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
  return PVR_ERROR_NO_ERROR;

}

PVR_ERROR PVRcmyth::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - group: %s",__FUNCTION__,group.strGroupName);
  PVR_CHANNEL_GROUP_MEMBER tag;
  int i=0;
  for(std::vector< int >::iterator it=m_groups.at(group.strGroupName).begin();it!=m_groups.at(group.strGroupName).end();it++)
  {
    if(m_channels.find(*it)!=m_channels.end())
    {
      MythChannel chan=m_channels.at(*it); 
      if(group.bIsRadio==chan.IsRadio())
      {
        tag.iChannelNumber=i++;
        tag.iChannelUniqueId=chan.ID();
        //tag.strGroupName=group.strGroupName;
        PVR_STRCPY(tag.strGroupName,group.strGroupName);
	PVR->TransferChannelGroupMember(handle,&tag);
      }
    }
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRcmyth::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - start: %i, end: %i, ChanID: %i",__FUNCTION__,iStart,iEnd,channel.iUniqueId);
  if(iStart!=m_EPGstart&&iEnd!=m_EPGend)
  {
    m_EPG=m_db.GetGuide(iStart,iEnd);
    if(g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s: Fetching EPG - size: %i",__FUNCTION__,m_EPG.size());
    m_EPGstart=iStart;
    m_EPGend=iEnd;
  }
  for(std::vector< MythProgram >::iterator it=m_EPG.begin();it!=m_EPG.end();it++)
  {
    if((unsigned int)it->chanid==channel.iUniqueId)
    {
      EPG_TAG tag;
      tag.endTime=it->endtime;
      tag.iChannelNumber=it->channum;
      tag.startTime=it->starttime;
      CStdString title=it->title;
      CStdString subtitle=it->subtitle;
      if (!subtitle.IsEmpty())
        title+=": " + subtitle;
      tag.strTitle=title;
      tag.strPlot= it->description;
      /*unsigned int seriesid=atoi(it->seriesid);
      if(seriesid!=0)
      tag.iUniqueBroadcastId=atoi(it->seriesid);
      else*/
      tag.iUniqueBroadcastId=(tag.startTime<<16)+(tag.iChannelNumber&0xffff);
      int genre=Genre(it->category);
      tag.iGenreSubType=genre&0x0F;
      tag.iGenreType=genre&0xF0;

      //unimplemented
      tag.strEpisodeName="";
      tag.strGenreDescription="";
      tag.strIconPath="";
      tag.strPlotOutline="";
      tag.bNotify=false;
      tag.firstAired=0;
      tag.iEpisodeNumber=0;
      tag.iEpisodePartNumber=0;
      tag.iParentalRating=0;
      tag.iSeriesNumber=0;
      tag.iStarRating=0;


      PVR->TransferEpgEntry(handle,&tag);
    }
  }
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
  return PVR_ERROR_NO_ERROR;
}

/***********************************************************
 * PVR Client AddOn liveTV library functions
 ***********************************************************/
bool PVRcmyth::OpenLiveStream(const PVR_CHANNEL &channel)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - chanID: %i, channumber: %i",__FUNCTION__,channel.iUniqueId,channel.iChannelNumber);
  //SingleLock<PLATFORM::CMutex> lock(&m_lock); 
  if(m_rec.IsNull())
  {
    MythChannel chan=m_channels.at(channel.iUniqueId);
    for(std::vector<int>::iterator it=m_sources.at(chan.SourceID()).begin();it!=m_sources.at(chan.SourceID()).end();it++)
    {
      m_rec=m_con.GetRecorder(*it);
      if(!m_rec.IsRecording() && m_rec.IsTunable(chan))
      {
	    if(g_bExtraDebug)
	      XBMC->Log(LOG_DEBUG,"%s: Opening new recorder %i",__FUNCTION__,m_rec.ID());
        if ( m_pEventHandler )
        {
	      m_pEventHandler->SetRecorder(m_rec);
        }
        if(m_rec.SpawnLiveTV(chan))
        {
          return true;
        }
      }
      m_rec=MythRecorder();
      if ( m_pEventHandler )
      {
        m_pEventHandler->SetRecorder(m_rec);//Redundant
      }
    }
    if(g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
    return false;
  }
  else
  {
    if(g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
    return true;
  }
}

void PVRcmyth::CloseLiveStream()
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s",__FUNCTION__);
  //SingleLock<PLATFORM::CMutex> lock(&m_lock); 
  if ( m_pEventHandler )
  {
    m_pEventHandler->PreventLiveChainUpdate();
  }
  m_rec.Stop();
  m_rec=MythRecorder();
  if ( m_pEventHandler )
  {
    m_pEventHandler->SetRecorder(m_rec);
    m_pEventHandler->AllowLiveChainUpdate();
  }
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
  return;
}

int PVRcmyth::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - size: %i",__FUNCTION__,iBufferSize);
  //SingleLock<PLATFORM::CMutex> lock(&m_lock); 
  if(m_rec.IsNull())
    return -1;
  int dataread=m_rec.ReadLiveTV(pBuffer,iBufferSize);
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s: Read %i Bytes",__FUNCTION__,dataread);
  else if(dataread==0)
    XBMC->Log(LOG_INFO,"%s: Read 0 Bytes!",__FUNCTION__);
  return dataread;
}

long long PVRcmyth::SeekLiveStream(long long iPosition, int iWhence) { 
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - pos: %i, whence: %i",__FUNCTION__,iPosition,iWhence);
  //SingleLock<PLATFORM::CMutex> lock(&m_lock); 
  if(m_rec.IsNull())
    return -1;
  int whence=iWhence==SEEK_SET?WHENCE_SET:iWhence==SEEK_CUR?WHENCE_CUR:WHENCE_END;
  long long retval= m_rec.LiveTVSeek(iPosition,whence);
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done - pos: %i",__FUNCTION__,retval);
  return retval;
}

long long PVRcmyth::LengthLiveStream()
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s",__FUNCTION__);
  //SingleLock<PLATFORM::CMutex> lock(&m_lock); 
  if(m_rec.IsNull())
    return -1;
  long long retval=m_rec.LiveTVDuration();
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done - duration: %i",__FUNCTION__,retval);
  return retval;
}

bool PVRcmyth::SwitchChannel(const PVR_CHANNEL &channel)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - chanID: %i",__FUNCTION__,channel.iUniqueId);
  MythChannel chan=m_channels.at(channel.iUniqueId);
  bool retval=false;
  retval=m_rec.SetChannel(chan); //Disabled for now. Seeking will hang if using setchannel.
  if(!retval)
  {
    //XBMC->Log(LOG_INFO,"%s - Failed to change to channel: %s(%i) - Reopening Livestream.",__FUNCTION__,channelinfo.strChannelName,channelinfo.iUniqueId);
    CloseLiveStream();
    retval=OpenLiveStream(channel);
    if(!retval)
      XBMC->Log(LOG_ERROR,"%s - Failed to reopening Livestream!",__FUNCTION__);
  }
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
  return retval;
}

PVR_ERROR PVRcmyth::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s",__FUNCTION__);
  MythSignal signal;
  if ( m_pEventHandler )
  {
    signal=m_pEventHandler->GetSignal();
  }
  signalStatus.dAudioBitrate=0;
  signalStatus.dDolbyBitrate=0;
  signalStatus.dVideoBitrate=0;
  signalStatus.iBER=signal.BER();
  signalStatus.iSignal=signal.Signal();
  signalStatus.iSNR=signal.SNR();
  signalStatus.iUNC=signal.UNC();
  CStdString ID;
  CStdString adaptorStatus=signal.AdapterStatus();
  ID.Format("Myth Recorder %i",signal.ID());
  strcpy(signalStatus.strAdapterName,ID.Buffer());
  strcpy(signalStatus.strAdapterStatus,adaptorStatus.Buffer());
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
  return PVR_ERROR_NO_ERROR;
}

/***********************************************************
 * PVR Client AddOn Timers library functions
 ***********************************************************/
//virtual int GetTimersAmount();
//virtual PVR_ERROR GetTimers(ADDON_HANDLE handle);
//virtual PVR_ERROR AddTimer(const PVR_TIMER &timer);
//virtual PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
//virtual PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
/*
void PVRcmyth::PVRtoMythTimer(const PVR_TIMER timer, MythTimer& mt)
{
  CStdString category=Genre(timer.iGenreType);
  mt.Category(category);
  mt.ChanID(timer.iClientChannelUid);
  mt.Callsign(m_channels.at(timer.iClientChannelUid).Callsign());
  mt.Description(timer.strSummary);
  mt.EndOffset(timer.iMarginEnd);
  mt.EndTime(timer.endTime);
  mt.Inactive(timer.state == PVR_TIMER_STATE_ABORTED ||timer.state ==  PVR_TIMER_STATE_CANCELLED);
  mt.Priority(timer.iPriority);
  mt.StartOffset(timer.iMarginStart);
  mt.StartTime(timer.startTime);
  mt.Title(timer.strTitle,true);
  CStdString title=mt.Title(false);
  mt.SearchType(m_db.FindProgram(timer.startTime,timer.iClientChannelUid,title,NULL)?MythTimer::NoSearch:MythTimer::ManualSearch);
  mt.Type(timer.bIsRepeating? (timer.iWeekdays==127? MythTimer::TimeslotRecord : MythTimer::WeekslotRecord) : MythTimer::SingleRecord);
  mt.RecordID(timer.iClientIndex);
}
*/

/***********************************************************
 * PVR Client AddOn Recordings library functions
 ***********************************************************/
//virtual int GetRecordingsAmount(void);
//virtual PVR_ERROR GetRecordings(ADDON_HANDLE handle);
//virtual PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
//virtual PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count);
//virtual bool OpenRecordedStream(const PVR_RECORDING &recinfo);
//virtual void CloseRecordedStream();
//virtual int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
//virtual long long SeekRecordedStream(long long iPosition, int iWhence);
//virtual long long LengthRecordedStream();
