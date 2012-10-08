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

#include "pvrclient-mythtv.h"

#include "client.h"
#include "tools.h"

#include <time.h>
#include <boost/regex.hpp>

using namespace ADDON;
using namespace PLATFORM;

RecordingRule::RecordingRule(const MythTimer &timer)
  : MythTimer(timer)
  , m_parent(0)
{
}

RecordingRule &RecordingRule::operator=(const MythTimer &t)
{
  MythTimer::operator=(t);
  clear();
  return *this;
}

bool RecordingRule::operator==(const int &id)
{
  return id==RecordID();
}

RecordingRule *RecordingRule::GetParent() const
{
  return m_parent;
}

void RecordingRule::SetParent(RecordingRule &parent)
{
  m_parent = &parent;
}

bool RecordingRule::HasModifiers() const
{
  return !m_modifiers.empty();
}

std::vector<RecordingRule*> RecordingRule::GetModifiers() const
{
  return m_modifiers;
}

void RecordingRule::AddModifier(RecordingRule &modifier)
{
  m_modifiers.push_back(&modifier);
}

bool RecordingRule::SameTimeslot(RecordingRule &rule) const
{
  time_t starttime = StartTime();
  time_t rStarttime = rule.StartTime();

  switch (rule.Type())
  {
  case MythTimer::NotRecording:
  case MythTimer::SingleRecord:
  case MythTimer::OverrideRecord:
  case MythTimer::DontRecord:
    return rStarttime == starttime && rule.EndTime() == EndTime() && rule.ChannelID() == ChannelID();
  case MythTimer::FindDailyRecord:
  case MythTimer::FindWeeklyRecord:
  case MythTimer::FindOneRecord:
    return rule.Title(false) == Title(false);
  case MythTimer::TimeslotRecord:
    return rule.Title(false) == Title(false) && daytime(&starttime) == daytime(&rStarttime) &&  rule.ChannelID() == ChannelID();
  case MythTimer::ChannelRecord:
    return rule.Title(false) == Title(false) && rule.ChannelID() == ChannelID(); //TODO: dup
  case MythTimer::AllRecord:
    return rule.Title(false) == Title(false); //TODO: dup
  case MythTimer::WeekslotRecord:
    return rule.Title(false) == Title(false) && daytime(&starttime) == daytime(&rStarttime) && weekday(&starttime) == weekday(&rStarttime) && rule.ChannelID() == ChannelID();
  }
  return false;
}

void RecordingRule::push_back(std::pair<PVR_TIMER, MythProgramInfo> &_val)
{
  SaveTimerString(_val.first);
  std::vector<std::pair<PVR_TIMER, MythProgramInfo> >::push_back(_val);
}

void RecordingRule::SaveTimerString(PVR_TIMER &timer)
{
  m_stringStore.push_back(boost::shared_ptr<CStdString>(new CStdString(timer.strTitle)));
  PVR_STRCPY(timer.strTitle,m_stringStore.back()->c_str());
  m_stringStore.push_back(boost::shared_ptr<CStdString>(new CStdString(timer.strDirectory)));
  PVR_STRCPY(timer.strDirectory,m_stringStore.back()->c_str());
  m_stringStore.push_back(boost::shared_ptr<CStdString>(new CStdString(timer.strSummary)));
  PVR_STRCPY(timer.strSummary,m_stringStore.back()->c_str());
}

PVRClientMythTV::PVRClientMythTV()
 : m_con()
 , m_pEventHandler(NULL)
 , m_db()
 , m_fileOps(NULL)
 , m_protocolVersion("")
 , m_connectionString("")
 , m_categoryMap()
 , m_EPGstart(0)
 , m_EPGend(0)
 , m_channelGroups()
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
  m_categoryMap.insert(catbimap::value_type("Drama", 0x1F)); // MythTV use 0xF0 but xbmc doesn't implement this.
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
}

PVRClientMythTV::~PVRClientMythTV()
{
  if (m_fileOps)
  {
    delete m_fileOps;
    m_fileOps = NULL;
  }

  if (m_pEventHandler)
  {
    m_pEventHandler->Stop();
    delete m_pEventHandler;
    m_pEventHandler = NULL;
  }
}

int PVRClientMythTV::Genre(CStdString g)
{
  int retval = 0;
  //XBMC->Log(LOG_DEBUG, "- %s - ## Genre ## - %s -", __FUNCTION__, g.c_str());
  try
  {
    if (m_categoryMap.by< mythcat >().count(g))
      retval=m_categoryMap.by< mythcat >().at(g);

  }
  catch(std::out_of_range)
  {
  }
  //XBMC->Log(LOG_DEBUG, "- %s - ## Genre ## - ret: %d -", __FUNCTION__, retval);
  return retval;
}

CStdString PVRClientMythTV::Genre(int g)
{
  CStdString retval = "";
  try
  {
    if (m_categoryMap.by<pvrcat>().count(g))
      retval = m_categoryMap.by<pvrcat>().at(g);
  }
  catch(std::out_of_range)
  {
  }
  return retval;
}

void Log(int level, char *msg)
{
  if (msg && level != CMYTH_DBG_NONE)
  {
    bool doLog = g_bExtraDebug;
    addon_log_t loglevel = LOG_DEBUG;
    switch (level)
    {
    case CMYTH_DBG_ERROR:
      loglevel = LOG_ERROR;
      doLog = true;
      break;
    case CMYTH_DBG_WARN:
    case CMYTH_DBG_INFO:
      loglevel = LOG_INFO;
      break;
    case CMYTH_DBG_DETAIL:
    case CMYTH_DBG_DEBUG:
    case CMYTH_DBG_PROTO:
    case CMYTH_DBG_ALL: 
      loglevel = LOG_DEBUG;
      break;
    }
    if (XBMC && doLog)
      XBMC->Log(loglevel, "LibCMyth: %s", msg);
  }
}

bool PVRClientMythTV::Connect()
{
  // Setup libcmyth logging
  if (g_bExtraDebug)
    cmyth_dbg_all();
  else
    cmyth_dbg_level(CMYTH_DBG_ERROR);
  cmyth_set_dbg_msgcallback(Log);

  // Create MythTV connection
  m_con = MythConnection(g_szHostname, g_iMythPort);
  if (!m_con.IsConnected())
  {
    XBMC->QueueNotification(QUEUE_ERROR, "%s: Failed to connect to MythTV backend %s: %i", __FUNCTION__, g_szHostname.c_str(), g_iMythPort);
    return false;
  }

  // Create event handler
  m_pEventHandler = m_con.CreateEventHandler();
  if (!m_pEventHandler)
  {
    XBMC->QueueNotification(QUEUE_ERROR, "Failed to create MythTV Event Handler");
    return false;
  }

  // Create database connection
  m_protocolVersion.Format("%i", m_con.GetProtocolVersion());
  m_connectionString.Format("%s:%i", g_szHostname, g_iMythPort);
  m_db = MythDatabase(g_szHostname, g_szMythDBname, g_szMythDBuser, g_szMythDBpassword);
  if (m_db.IsNull())
  {
    XBMC->QueueNotification(QUEUE_ERROR, "Failed to connect to MythTV MySQL database %s@%s %s/%s", g_szMythDBname.c_str(), g_szHostname.c_str(), g_szMythDBuser.c_str(), g_szMythDBpassword.c_str());
    if (m_pEventHandler)
      m_pEventHandler->Stop();
    return false;
  }

  // Test database connection
  CStdString db_test;
  if (!m_db.TestConnection(&db_test))
  {
    XBMC->QueueNotification(QUEUE_ERROR, "Failed to connect to MythTV MySQL database %s@%s %s/%s\n%s", g_szMythDBname.c_str(), g_szHostname.c_str(), g_szMythDBuser.c_str(), g_szMythDBpassword.c_str(), db_test.c_str());
    if (m_pEventHandler)
      m_pEventHandler->Stop();
    return false;
  }

  // Create file operation helper (image caching)
  m_fileOps = new FileOps(m_con);

  // Get channel list
  m_channels = m_db.ChannelList();
  if (m_channels.empty())
    XBMC->Log(LOG_INFO,"%s: Empty channel list", __FUNCTION__);

  // Get sources
  m_sources = m_db.SourceList();
  if (m_sources.empty())
    XBMC->Log(LOG_INFO,"%s: Empty source list", __FUNCTION__);

  // Get channel groups
  m_channelGroups = m_db.GetChannelGroups();
  if (m_channelGroups.empty())
    XBMC->Log(LOG_INFO,"%s: No channel groups", __FUNCTION__);

  return true;
}

const char *PVRClientMythTV::GetBackendName()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  return m_con.GetBackendName();
}

const char *PVRClientMythTV::GetBackendVersion() const
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  return m_protocolVersion;
}

const char *PVRClientMythTV::GetConnectionString() const
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  return m_connectionString;
}

bool PVRClientMythTV::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  return m_con.GetDriveSpace(*iTotal, *iUsed);
}

PVR_ERROR PVRClientMythTV::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - start: %i, end: %i, ChannelID: %i", __FUNCTION__, iStart, iEnd, channel.iUniqueId);

  if (iStart != m_EPGstart && iEnd != m_EPGend)
  {
    m_EPG = m_db.GetGuide(iStart, iEnd);
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s: Fetching EPG - size: %i", __FUNCTION__, m_EPG.size());
    m_EPGstart = iStart;
    m_EPGend = iEnd;
  }

  for (std::vector<MythProgram>::iterator it = m_EPG.begin(); it != m_EPG.end(); it++)
  {
    if ((unsigned)it->chanid==channel.iUniqueId)
    {
      EPG_TAG tag;
      memset(&tag, 0, sizeof(EPG_TAG));

      tag.iChannelNumber = it->channum;
      tag.startTime = it->starttime;
      tag.endTime = it->endtime;

      CStdString title = it->title;
      CStdString subtitle = it->subtitle;
      if (!subtitle.IsEmpty())
        title += ": " + subtitle;
      tag.strTitle = title;

      tag.strPlot = it->description;
      tag.iUniqueBroadcastId = (tag.startTime << 16) + (tag.iChannelNumber & 0xffff);

      int genre = Genre(it->category);
      tag.iGenreSubType = genre & 0x0F;
      tag.iGenreType = genre & 0xF0;

      // Unimplemented
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

      PVR->TransferEpgEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

int PVRClientMythTV::GetNumChannels()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  return m_channels.size();
}

PVR_ERROR PVRClientMythTV::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - radio: %i", __FUNCTION__, bRadio);

  for (std::map<int, MythChannel>::iterator it = m_channels.begin(); it != m_channels.end(); it++)
  {
    if (it->second.IsRadio() == bRadio && !it->second.IsNull())
    {
      PVR_CHANNEL tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL));

      tag.iUniqueId = it->first;
      tag.iChannelNumber = it->second.NumberInt(); // Use ID instead as MythTV channel number is a string?
      PVR_STRCPY(tag.strChannelName, it->second.Name());
      tag.bIsHidden = !it->second.Visible();
      tag.bIsRadio = it->second.IsRadio();

      CStdString icon = GetArtWork(FileOps::FileTypeChannelIcon, it->second.Icon());
      PVR_STRCPY(tag.strIconPath, icon);

      // Unimplemented
      PVR_STRCPY(tag.strStreamURL, "");
      PVR_STRCPY(tag.strInputFormat, "");
      tag.iEncryptionSystem=0;

      PVR->TransferChannelEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

int PVRClientMythTV::GetChannelGroupsAmount() const
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  return m_channelGroups.size();
}

PVR_ERROR PVRClientMythTV::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - radio: %i", __FUNCTION__, bRadio);

  for (boost::unordered_map<CStdString, std::vector<int> >::iterator it = m_channelGroups.begin(); it != m_channelGroups.end(); it++)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));

    PVR_STRCPY(tag.strGroupName, it->first);
    tag.bIsRadio = bRadio;

    for (std::vector<int>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
    {
      if (m_channels.find(*it2) != m_channels.end() && m_channels.at(*it2).IsRadio() == bRadio)
      {
        PVR->TransferChannelGroup(handle, &tag);
        break;
      }
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - group: %s", __FUNCTION__, group.strGroupName);

  int i=0;
  for (std::vector<int>::iterator it = m_channelGroups.at(group.strGroupName).begin(); it != m_channelGroups.at(group.strGroupName).end(); it++)
  {
    PVR_CHANNEL_GROUP_MEMBER tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

    if (m_channels.find(*it) != m_channels.end())
    {
      MythChannel chan = m_channels.at(*it);
      if (group.bIsRadio == chan.IsRadio())
      {
        tag.iChannelNumber = i++;
        tag.iChannelUniqueId = chan.ID();
        PVR_STRCPY(tag.strGroupName, group.strGroupName);
        PVR->TransferChannelGroupMember(handle, &tag);
      }
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

int PVRClientMythTV::GetRecordingsAmount(void)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_recordings = m_con.GetRecordedPrograms();
  int res = 0;
  for (boost::unordered_map<CStdString, MythProgramInfo>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if (!it->second.IsNull() && IsRecordingVisible(it->second))
      res++;
  }
  return res;
}

PVR_ERROR PVRClientMythTV::GetRecordings(ADDON_HANDLE handle)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_recordings = m_con.GetRecordedPrograms();
  for (boost::unordered_map< CStdString, MythProgramInfo >::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if (!it->second.IsNull() && IsRecordingVisible(it->second))
    {
      PVR_RECORDING tag;
      memset(&tag, 0, sizeof(PVR_RECORDING));

      tag.recordingTime = it->second.RecStart();
      tag.iDuration = it->second.Duration();
      tag.iPlayCount = it->second.IsWatched() ? 1 : 0;

      CStdString id = it->second.StrUID();
      CStdString path = it->second.Path();
      CStdString title = it->second.Title(true);

      PVR_STRCPY(tag.strRecordingId, id);
      PVR_STRCPY(tag.strTitle, title);
      PVR_STRCPY(tag.strPlot, it->second.Description());
      PVR_STRCPY(tag.strChannelName, it->second.ChannelName());

      int genre = Genre(it->second.Category());
      tag.iGenreSubType = genre&0x0F;
      tag.iGenreType = genre&0xF0;

      // Add recording title to directory to group everything according to its name just like MythTV does
      CStdString strDirectory;
      strDirectory.Format("%s/%s", it->second.RecordingGroup(), it->second.Title(false));
      PVR_STRCPY(tag.strDirectory, strDirectory);

      // Images
      CStdString strIconPath = GetArtWork(FileOps::FileTypeCoverart, title);
      if (strIconPath.IsEmpty())
        strIconPath = m_fileOps->GetPreviewIconPath(path + ".png");

      CStdString strFanartPath = GetArtWork(FileOps::FileTypeFanart, title);

      PVR_STRCPY(tag.strIconPath, strIconPath.c_str());
      PVR_STRCPY(tag.strThumbnailPath, strIconPath.c_str());
      PVR_STRCPY(tag.strFanartPath, strFanartPath.c_str());

      // Unimplemented
      tag.iLifetime = 0;
      tag.iPriority = 0;
      PVR_STRCPY(tag.strPlotOutline, "");
      PVR_STRCPY(tag.strStreamURL, "");

      PVR->TransferRecordingEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

bool PVRClientMythTV::IsRecordingVisible(MythProgramInfo &recording)
{
  // Filter out recordings of special storage groups (like LiveTV or Deleted)

  // When  deleting a recording, it might not be deleted immediately but marked as 'pending delete'.
  // Depending on the protocol version the recording is moved to the group Deleted or
  // the 'delete pending' flag is set
  if (recording.RecordingGroup() == "LiveTV" || recording.RecordingGroup() == "Deleted" || recording.IsDeletePending())
  {
    XBMC->Log(LOG_DEBUG, "%s: Ignoring recording %s", __FUNCTION__, recording.Path().c_str());
    return false;
  }

  return true;
}

PVR_ERROR PVRClientMythTV::DeleteRecording(const PVR_RECORDING &recording)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CStdString id = recording.strRecordingId;
  if (*id.rbegin() == '@')
    id = id.substr(0, id.size() - 1);

  bool ret = m_con.DeleteRecording(m_recordings.at(id));
  if (ret && m_recordings.erase(recording.strRecordingId))
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s - Deleted", __FUNCTION__);
    return PVR_ERROR_NO_ERROR;
  }
  else
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s - Not Deleted", __FUNCTION__);
    return PVR_ERROR_FAILED;
  }
}

PVR_ERROR PVRClientMythTV::SetRecordingPlayCount(const PVR_RECORDING &recording, int count)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CStdString id = recording.strRecordingId;

  if (count > 1) count = 1;
  if (count < 0) count = 0;
  int ret = m_db.SetWatchedStatus(m_recordings.at(id), count > 0);

  if (ret == 1)
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s - Set watched state", __FUNCTION__);
    PVR->TriggerRecordingUpdate();
    return PVR_ERROR_NO_ERROR;
  }
  else
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s - Setting watched state failed: %d)", __FUNCTION__, ret);
    return PVR_ERROR_FAILED;
  }
}

PVR_ERROR PVRClientMythTV::SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
  // MythTV provides it's bookmarks as frame offsets whereas XBMC expects a time offset.
  // The frame offset is calculated by: frameOffset = bookmark * frameRate.
  long long frameOffset = 0;

  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s - Setting Bookmark for: %s to %d", __FUNCTION__, recording.strTitle, lastplayedposition);
  }

  MythProgramInfo progInfo = m_recordings.at(recording.strRecordingId);

  // Calculate the frame offset
  frameOffset = (long long)((float)lastplayedposition * GetRecordingFrameRate(progInfo));
  if (frameOffset < 0) frameOffset = 0;
  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s - FrameOffset: %lld)", __FUNCTION__, frameOffset);
  }

  // Write the bookmark
  int retval = m_con.SetBookmark(progInfo, frameOffset);
  if (retval == 1)
  {
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_ERROR, "%s - Setting Bookmark successful: %d)", __FUNCTION__);
    }
    return PVR_ERROR_NO_ERROR;
  }
  else
  {
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_ERROR, "%s - Setting Bookmark failed: %d)", __FUNCTION__, retval);
    }
    return PVR_ERROR_FAILED;
  }
}

int PVRClientMythTV::GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
  // MythTV provides it's bookmarks as frame offsets whereas XBMC expects a time offset.
  // The bookmark in seconds is calculated by: bookmark = frameOffset / frameRate.
  int bookmark = 0;

  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s - Reading Bookmark for: %s", __FUNCTION__, recording.strTitle);
  }

  MythProgramInfo progInfo = m_recordings.at(recording.strRecordingId);
  long long frameOffset = m_con.GetBookmark(progInfo); // returns 0 if no bookmark was found
  if (frameOffset > 0)
  {
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_DEBUG, "%s - FrameOffset: %lld)", __FUNCTION__, frameOffset);
    }

    float frameRate = GetRecordingFrameRate(progInfo);
    if (frameRate > 0)
    {
      bookmark = (int)((float)frameOffset / frameRate);
      if (g_bExtraDebug)
      {
        XBMC->Log(LOG_DEBUG, "%s - Bookmark: %d)", __FUNCTION__, bookmark);
      }
    }
  }

  // Set the bookmark few seconds earlier (due to the accuracy of the above float operations)
  bookmark = bookmark - 3;
  if (bookmark < 0) bookmark = 0;
  return bookmark;
}

float PVRClientMythTV::GetRecordingFrameRate(MythProgramInfo &recording)
{
  // MythTV uses frame offsets whereas XBMC expects a time offset.
  // This function can be used to convert the frame offsets to time offsets and back.
  // The average frameRate is calculated by: frameRate = frameCount / duration.
  float frameRate = 0.0f;

  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s - Getting Framerate for: %s)", __FUNCTION__, recording.Title(false).c_str());
  }

  // cmyth_get_bookmark_mark returns the appropriate frame offset for the given byte offset (recordedseek table)
  // This can be used to determine the frame count (by querying the max byte offset)
  long long frameCount = m_db.GetBookmarkMark(recording, LLONG_MAX, 0);
  if (frameCount > 0)
  {
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_DEBUG, "%s - FrameCount: %lld)", __FUNCTION__, frameCount);
      XBMC->Log(LOG_DEBUG, "%s - Duration: %d)", __FUNCTION__, recording.Duration());
    }

    if (recording.Duration() > 0)
    {
      // Calculate frameRate
      frameRate = (float)frameCount / (float)recording.Duration();

      if (g_bExtraDebug)
      {
        XBMC->Log(LOG_DEBUG, "%s - FrameRate: %f)", __FUNCTION__, frameRate);
      }
    }
  }
  return frameRate;
}

int PVRClientMythTV::GetTimersAmount(void)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  std::map<int, MythTimer> m_timers = m_db.GetTimers();
  return m_timers.size();
}

PVR_ERROR PVRClientMythTV::GetTimers(ADDON_HANDLE handle)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  std::map<int, MythTimer> timers = m_db.GetTimers();
  m_recordingRules.clear();

  for (std::map<int, MythTimer>::iterator it = timers.begin(); it != timers.end(); it++)
    m_recordingRules.push_back(it->second);

  //Search for modifiers and add links to them
  for (std::vector<RecordingRule>::iterator it = m_recordingRules.begin(); it != m_recordingRules.end(); it++)
    if (it->Type() == MythTimer::DontRecord || it->Type() == MythTimer::OverrideRecord)
      for (std::vector<RecordingRule>::iterator it2 = m_recordingRules.begin(); it2 != m_recordingRules.end(); it2++)
        if (it2->Type() != MythTimer::DontRecord && it2->Type() != MythTimer::OverrideRecord)
          if (it->SameTimeslot(*it2) && !it->GetParent())
          {
            it2->AddModifier(*it);
            it->SetParent(*it2);
          }

  boost::unordered_map<CStdString, MythProgramInfo> upcomingRecordings = m_con.GetPendingPrograms();
  for (boost::unordered_map<CStdString, MythProgramInfo>::iterator it = upcomingRecordings.begin(); it != upcomingRecordings.end(); it++)
  {
    // When deleting a timer from mythweb, it might happen that it's removed from database
    // but it's still present over mythprotocol. Skip those timers, because timers.at would crash.
    if (timers.count(it->second.RecordID()) == 0)
    {
      XBMC->Log(LOG_DEBUG, "%s - Skipping timer that is no more in database", __FUNCTION__);
      continue;
    }

    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    tag.startTime= it->second.StartTime();
    tag.endTime = it->second.EndTime();
    tag.iClientChannelUid = it->second.ChannelID();
    tag.iClientIndex = it->second.RecordID();
    tag.iMarginEnd = timers.at(it->second.RecordID()).EndOffset();
    tag.iMarginStart = timers.at(it->second.RecordID()).StartOffset();
    tag.iPriority = it->second.Priority();
    tag.bIsRepeating = false;
    tag.firstDay = 0;
    tag.iWeekdays = 0;

    int genre = Genre(it->second.Category());
    tag.iGenreSubType = genre & 0x0F;
    tag.iGenreType = genre & 0xF0;

    // Title
    CStdString title = it->second.Title(true);
    if (title.IsEmpty())
    {
      MythProgram epgProgram;
      title= "%";
      m_db.FindProgram(tag.startTime, tag.iClientChannelUid, title, &epgProgram);
      title = epgProgram.title;
    }
    PVR_STRCPY(tag.strTitle, title);

    // Summary
    PVR_STRCPY(tag.strSummary, it->second.Description());

    // Status
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s ## - State: %d - ##", __FUNCTION__, it->second.Status());
    switch (it->second.Status())
    {
    case RS_RECORDING:
    case RS_TUNING:
      tag.state = PVR_TIMER_STATE_RECORDING;
      break;
    case RS_ABORTED:
      tag.state = PVR_TIMER_STATE_ABORTED;
      break;
    case RS_RECORDED:
      tag.state = PVR_TIMER_STATE_COMPLETED;
      break;
    case RS_WILL_RECORD:
      tag.state = PVR_TIMER_STATE_SCHEDULED;
      break;
    case RS_UNKNOWN:
    case RS_DONT_RECORD:
    case RS_PREVIOUS_RECORDING:
    case RS_CURRENT_RECORDING:
    case RS_EARLIER_RECORDING:
    case RS_TOO_MANY_RECORDINGS:
    case RS_NOT_LISTED:
    case RS_CONFLICT:
    case RS_LATER_SHOWING:
    case RS_REPEAT:
    case RS_INACTIVE:
    case RS_NEVER_RECORD:
    case RS_OFFLINE:
    case RS_OTHER_SHOWING:
    case RS_FAILED:
    case RS_TUNER_BUSY:
    case RS_LOW_DISKSPACE:
    case RS_CANCELLED:
    case RS_MISSED:
    default:
      tag.state = PVR_TIMER_STATE_CANCELLED;
      break;
    }

    // Unimplemented
    tag.iEpgUid=0;
    tag.iLifetime=0;
    PVR_STRCPY(tag.strDirectory, "");

    // Recording rules
    std::vector<RecordingRule>::iterator recRule = std::find(m_recordingRules.begin(), m_recordingRules.end() , it->second.RecordID());
    tag.iClientIndex = ((recRule - m_recordingRules.begin()) << 16) + recRule->size();
    //recRule->SaveTimerString(tag);
    std::pair<PVR_TIMER, MythProgramInfo> rrtmp(tag, it->second);
    recRule->push_back(rrtmp);

    PVR->TransferTimerEntry(handle,&tag);
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

// kManualSearch = http://www.gossamer-threads.com/lists/mythtv/dev/155150?search_string=kManualSearch;#155150

PVR_ERROR PVRClientMythTV::AddTimer(const PVR_TIMER &timer)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - title: %s, start: %i, end: %i, chanID: %i", __FUNCTION__, timer.strTitle, timer.startTime, timer.iClientChannelUid);

  MythTimer mt;
  m_con.DefaultTimer(mt);
  CStdString category = Genre(timer.iGenreType);
  mt.SetCategory(category);
  mt.SetChannelID(timer.iClientChannelUid);
  mt.SetCallsign(m_channels.at(timer.iClientChannelUid).Callsign());
  mt.SetDescription(timer.strSummary);
  mt.SetEndOffset(timer.iMarginEnd);
  mt.SetEndTime(timer.endTime);
  mt.SetInactive(timer.state == PVR_TIMER_STATE_ABORTED ||timer.state ==  PVR_TIMER_STATE_CANCELLED);
  mt.SetPriority(timer.iPriority);
  mt.SetStartOffset(timer.iMarginStart);
  mt.SetStartTime(timer.startTime);
  mt.SetTitle(timer.strTitle, true);
  CStdString title = mt.Title(false);
  mt.SetSearchType(m_db.FindProgram(timer.startTime, timer.iClientChannelUid, title, NULL) ? MythTimer::NoSearch : MythTimer::ManualSearch);
  mt.SetType(timer.bIsRepeating ? (timer.iWeekdays == 127 ? MythTimer::TimeslotRecord : MythTimer::WeekslotRecord) : MythTimer::SingleRecord);

  int id = m_db.AddTimer(mt);
  if (id<0)
    return PVR_ERROR_FAILED;

  if (!m_con.UpdateSchedules(id))
    return PVR_ERROR_FAILED;

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - %i", __FUNCTION__, id);

  //PVR->TriggerTimerUpdate();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  //if(g_bExtraDebug)
  //  XBMC->Log(LOG_DEBUG,"%s - title: %s, start: %i, end: %i, chanID: %i, ID: %i",__FUNCTION__,timer.strTitle,timer.startTime,timer.iClientChannelUid,timer.iClientIndex);
  //std::map< int, MythTimer > timers=m_db.GetTimers();
  //RecordingRule r = m_recordingRules[(timer.iClientIndex)>>16]; 
  //if(r.GetParent())
  //  r = *r.GetParent();
  //if(r.Type()!=MythTimer::FindOneRecord && r.Type()!=MythTimer::SingleRecord)
  //{
  //  CStdString line0;
  //  line0.Format(XBMC->GetLocalizedString(30008)/*"This will delete the recording rule and \nan additional %i timer(s)."*/,r.size()-1);
  //  if(!(GUI->Dialog_showYesNo(XBMC->GetLocalizedString(19060)/*"Delete Timer"*/,line0,"",XBMC->GetLocalizedString(30007)/*"Do you still want to delete?"*/,NULL,NULL,NULL)))
  //    return PVR_ERROR_NO_ERROR; 
  //}
  ////delete related Override and Don't Record timers
  //std::vector<RecordingRule* > modifiers = r.GetModifiers();
  //for(std::vector <RecordingRule* >::iterator it = modifiers.begin(); it != modifiers.end(); it++)
  //  m_db.DeleteTimer((*it)->RecordID());
  //if(!m_db.DeleteTimer(r.RecordID()))
  //  return PVR_ERROR_FAILED;
  //m_con.UpdateSchedules(-1);
  //if(g_bExtraDebug)
  //  XBMC->Log(LOG_DEBUG,"%s - Done",__FUNCTION__);
  //return PVR_ERROR_NO_ERROR;
  (void) timer;
  (void) bForceDelete;
  return PVR_ERROR_FAILED;
}

void PVRClientMythTV::PVRtoMythTimer(const PVR_TIMER timer, MythTimer &mt)
{
  CStdString category = Genre(timer.iGenreType);
  mt.SetCategory(category);
  mt.SetChannelID(timer.iClientChannelUid);
  mt.SetCallsign(m_channels.at(timer.iClientChannelUid).Callsign());
  mt.SetDescription(timer.strSummary);
  mt.SetEndOffset(timer.iMarginEnd);
  mt.SetEndTime(timer.endTime);
  mt.SetInactive(timer.state == PVR_TIMER_STATE_ABORTED ||timer.state ==  PVR_TIMER_STATE_CANCELLED);
  mt.SetPriority(timer.iPriority);
  mt.SetStartOffset(timer.iMarginStart);
  mt.SetStartTime(timer.startTime);
  mt.SetTitle(timer.strTitle,true);
  CStdString title = mt.Title(false);
  mt.SetSearchType(m_db.FindProgram(timer.startTime, timer.iClientChannelUid, title, NULL) ? MythTimer::NoSearch : MythTimer::ManualSearch);
  mt.SetType(timer.bIsRepeating ? (timer.iWeekdays == 127 ? MythTimer::TimeslotRecord : MythTimer::WeekslotRecord) : MythTimer::SingleRecord);
  mt.SetRecordID(timer.iClientIndex);
}

PVR_ERROR PVRClientMythTV::UpdateTimer(const PVR_TIMER &timer)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - title: %s, start: %i, end: %i, chanID: %i, ID: %i", __FUNCTION__, timer.strTitle, timer.startTime, timer.iClientChannelUid, timer.iClientIndex);

  RecordingRule r = m_recordingRules[(timer.iClientIndex) >> 16];
  PVR_TIMER oldPvrTimer = r[(timer.iClientIndex) & 0xffff].first;
  {
    bool createNewRule = false;
    bool createNewOverrideRule = false;
    MythTimer mt;
    PVRtoMythTimer(timer, mt);
    mt.SetDescription(oldPvrTimer.strSummary); // Fix broken description
    mt.SetInactive(false);
    mt.SetRecordID(r.RecordID());

    // These should trigger a manual search or a new rule
    if (oldPvrTimer.iClientChannelUid != timer.iClientChannelUid ||
      oldPvrTimer.endTime != timer.endTime ||
      oldPvrTimer.startTime != timer.startTime ||
      oldPvrTimer.startTime != timer.startTime ||
      strcmp(oldPvrTimer.strTitle, timer.strTitle) ||
      //strcmp(oldPvrTimer.strSummary, timer.strSummary) ||
      timer.bIsRepeating
      )
      createNewRule = true;

    // Change type
    if (oldPvrTimer.state != timer.state)
    {
      if (r.Type() != MythTimer::SingleRecord && !createNewRule)
      {
        if (timer.state == PVR_TIMER_STATE_ABORTED || timer.state == PVR_TIMER_STATE_CANCELLED)
          mt.SetType(MythTimer::DontRecord);
        else
          mt.SetType(MythTimer::OverrideRecord);
        createNewOverrideRule = true;
      }
      else
        mt.SetInactive(timer.state == PVR_TIMER_STATE_ABORTED || timer.state == PVR_TIMER_STATE_CANCELLED);
    }

    // These can be updated without triggering a new rule
    if (oldPvrTimer.iMarginEnd != timer.iMarginEnd ||
      oldPvrTimer.iPriority != timer.iPriority ||
      oldPvrTimer.iMarginStart != timer.iMarginStart)
      createNewOverrideRule = true;

    CStdString title = timer.strTitle;
    if (createNewRule)
      mt.SetSearchType(m_db.FindProgram(timer.startTime, timer.iClientChannelUid, title, NULL) ? MythTimer::NoSearch : MythTimer::ManualSearch);
    if (createNewOverrideRule && r.SearchType() == MythTimer::ManualSearch)
      mt.SetSearchType(MythTimer::ManualSearch);

    if (r.Type() == MythTimer::DontRecord || r.Type() == MythTimer::OverrideRecord)
      createNewOverrideRule = false;

    if (createNewRule && r.Type() != MythTimer::SingleRecord)
    {
      if (!m_db.AddTimer(mt))
        return PVR_ERROR_FAILED;

      MythTimer mtold;
      PVRtoMythTimer(oldPvrTimer, mtold);
      mtold.SetType(MythTimer::DontRecord);
      mtold.SetInactive(false);
      mtold.SetRecordID(r.RecordID());
      int id = r.RecordID();
      if (r.Type() == MythTimer::DontRecord || r.Type() == MythTimer::OverrideRecord)
        m_db.UpdateTimer(mtold);
      else
        id=m_db.AddTimer(mtold); // Blocks old record rule
      m_con.UpdateSchedules(id);
    }
    else if (createNewOverrideRule &&  r.Type() != MythTimer::SingleRecord )
    {
      if (mt.Type() != MythTimer::DontRecord && mt.Type() != MythTimer::OverrideRecord)
        mt.SetType(MythTimer::OverrideRecord);
      if (!m_db.AddTimer(mt))
        return PVR_ERROR_FAILED;
    }
    else
    {
      if (!m_db.UpdateTimer(mt))
        return PVR_ERROR_FAILED;
    }
    m_con.UpdateSchedules(mt.RecordID());
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - Done", __FUNCTION__);
  return PVR_ERROR_NO_ERROR;
}

bool PVRClientMythTV::OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - chanID: %i, channumber: %i", __FUNCTION__, channel.iUniqueId, channel.iChannelNumber);

  CLockObject lock(m_lock);
  if (m_rec.IsNull())
  {
    MythChannel chan = m_channels.at(channel.iUniqueId);
    for (std::vector<int>::iterator it = m_sources.at(chan.SourceID()).begin(); it != m_sources.at(chan.SourceID()).end(); it++)
    {
      m_rec = m_con.GetRecorder(*it);
      if (!m_rec.IsRecording() && m_rec.IsTunable(chan))
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_DEBUG,"%s: Opening new recorder %i", __FUNCTION__, m_rec.ID());

        if (m_pEventHandler)
          m_pEventHandler->SetRecorder(m_rec);

        if (m_rec.SpawnLiveTV(chan))
          return true;
      }
      m_rec = MythRecorder();
      if (m_pEventHandler)
      {
        m_pEventHandler->SetRecorder(m_rec); // Redundant
      }
    }

    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s - Done", __FUNCTION__);

    return false;
  }
  else
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s - Done", __FUNCTION__);

    return true;
  }
}


void PVRClientMythTV::CloseLiveStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_lock);

  if (m_pEventHandler)
    m_pEventHandler->PreventLiveChainUpdate();

  m_rec.Stop();
  m_rec = MythRecorder();

  if (m_pEventHandler)
  {
    m_pEventHandler->SetRecorder(m_rec);
    m_pEventHandler->AllowLiveChainUpdate();
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return;
}

int PVRClientMythTV::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - size: %i", __FUNCTION__, iBufferSize);

  CLockObject lock(m_lock);

  if (m_rec.IsNull())
    return -1;

  int dataread = m_rec.ReadLiveTV(pBuffer, iBufferSize);
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Read %i Bytes", __FUNCTION__, dataread);
  else if (dataread==0)
    XBMC->Log(LOG_INFO, "%s: Read 0 Bytes!", __FUNCTION__);
  return dataread;
}

int PVRClientMythTV::GetCurrentClientChannel()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_lock);

  if (m_rec.IsNull())
    return -1;

  MythProgramInfo currentProgram = m_rec.GetCurrentProgram();
  return currentProgram.ChannelID();
}

bool PVRClientMythTV::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - chanID: %i", __FUNCTION__, channelinfo.iUniqueId);

  MythChannel chan = m_channels.at(channelinfo.iUniqueId);
  bool retval = false;
  retval = m_rec.SetChannel(chan);
  if (!retval)
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_INFO, "%s - Failed to change to channel: %s(%i) - Reopening Livestream", __FUNCTION__, channelinfo.strChannelName, channelinfo.iUniqueId);

    CloseLiveStream();
    retval = OpenLiveStream(channelinfo);

    if (!retval)
      XBMC->Log(LOG_ERROR, "%s - Failed to reopening Livestream", __FUNCTION__);
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return retval;
}

long long PVRClientMythTV::SeekLiveStream(long long iPosition, int iWhence)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - pos: %i, whence: %i", __FUNCTION__, iPosition, iWhence);

  CLockObject lock(m_lock);

  if (m_rec.IsNull())
    return -1;

  int whence;
  if (iWhence == SEEK_SET)
    whence = WHENCE_SET;
  else if (iWhence == SEEK_CUR)
    whence = WHENCE_CUR;
  else
    whence = WHENCE_END;

  long long retval = m_rec.LiveTVSeek(iPosition, whence);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - pos: %i", __FUNCTION__, retval);

  return retval;
}

long long PVRClientMythTV::LengthLiveStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_lock);

  if (m_rec.IsNull())
    return -1;

  long long retval = m_rec.LiveTVDuration();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - duration: %i", __FUNCTION__, retval);

  return retval;
}

PVR_ERROR PVRClientMythTV::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  MythSignal signal;
  if (m_pEventHandler)
  {
    signal = m_pEventHandler->GetSignal();
  }

  signalStatus.dAudioBitrate = 0;
  signalStatus.dDolbyBitrate = 0;
  signalStatus.dVideoBitrate = 0;
  signalStatus.iBER = signal.BER();
  signalStatus.iSignal = signal.Signal();
  signalStatus.iSNR = signal.SNR();
  signalStatus.iUNC = signal.UNC();

  CStdString ID;
  ID.Format("Myth Recorder %i", signal.ID());

  CStdString strAdapterStatus = signal.AdapterStatus();
  strcpy(signalStatus.strAdapterName, ID.Buffer());
  strcpy(signalStatus.strAdapterStatus, strAdapterStatus.Buffer());

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

bool PVRClientMythTV::OpenRecordedStream(const PVR_RECORDING &recinfo)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - title: %s, ID: %s, duration: %i", __FUNCTION__, recinfo.strTitle, recinfo.strRecordingId, recinfo.iDuration);

  CStdString id = recinfo.strRecordingId;
  if (*id.rbegin() == '@')
    id = id.substr(0, id.size() - 1);

  m_file = m_con.ConnectFile(m_recordings.at(id));
  if (m_pEventHandler)
    m_pEventHandler->SetRecordingListener(id, m_file);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - %i", __FUNCTION__, !m_file.IsNull());

  return !m_file.IsNull();
}

void PVRClientMythTV::CloseRecordedStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_file = MythFile();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);
}

int PVRClientMythTV::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  XBMC->Log(LOG_DEBUG,"%s - curPos: %i TotalLength: %i", __FUNCTION__, (int)m_file.Position(), (int)m_file.Length());

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - size: %i", __FUNCTION__, iBufferSize);

  int dataread = m_file.Read(pBuffer, iBufferSize);
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Read %i Bytes", __FUNCTION__, dataread);
  else if (dataread == 0)
    XBMC->Log(LOG_INFO, "%s: Read 0 Bytes!", __FUNCTION__);
  return dataread;
}

long long PVRClientMythTV::SeekRecordedStream(long long iPosition, int iWhence)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - pos: %i, whence: %i", __FUNCTION__, iPosition, iWhence);

  int whence;
  if (iWhence == SEEK_SET)
    whence = WHENCE_SET;
  else if (iWhence == SEEK_CUR)
    whence = WHENCE_CUR;
  else
    whence = WHENCE_END;

  long long retval = m_file.Seek(iPosition, whence);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - pos: %i", __FUNCTION__, retval);

  return retval;
}

long long PVRClientMythTV::LengthRecordedStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  long long retval = m_file.Length();
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - duration: %i", __FUNCTION__, retval);
  return retval;
}

PVR_ERROR PVRClientMythTV::CallMenuHook(const PVR_MENUHOOK &menuhook)
{
  (void)menuhook;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

bool PVRClientMythTV::GetLiveTVPriority()
{
  if (!m_con.IsNull())
  {
    CStdString value = m_con.GetSetting(m_con.GetHostname(), "LiveTVPriority");
    if (value.compare("1") == 0)
      return true;
    else
      return false;
  }
  return false;
}

void PVRClientMythTV::SetLiveTVPriority(bool enabled)
{
  if (!m_con.IsNull())
  {
    CStdString value = enabled ? "1" : "0";
    m_con.SetSetting(m_con.GetHostname(), "LiveTVPriority", value);
  }
}

CStdString PVRClientMythTV::GetArtWork(FileOps::FileType storageGroup, const CStdString &shwTitle)
{
  if (storageGroup == FileOps::FileTypeCoverart || storageGroup == FileOps::FileTypeFanart)
  {
    return m_fileOps->GetArtworkPath(shwTitle, storageGroup);
  }
  else if (storageGroup == FileOps::FileTypeChannelIcon)
  {
    return m_fileOps->GetChannelIconPath(shwTitle);
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "%s - ## Not a valid storageGroup ##", __FUNCTION__);
    return "";
  }
}
