#include "MythDatabase.h"
#include "MythChannel.h"
#include "MythProgramInfo.h"
#include "MythTimer.h"
#include "../client.h"

using namespace ADDON;

/*
*								MythDatabase
*/

/* Call 'call', then if 'cond' condition is true disconnect from
 * the database and call 'call' again. It should automatically
 * try to connect again. */
#define CMYTH_DB_CALL( var, cond, call )  m_database_t->Lock(); \
                                          var = CMYTH->call; \
                                          m_database_t->Unlock(); \
                                          if ( cond ) \
                                          { \
                                              m_database_t->Lock(); \
                                              CMYTH->cmyth_database_close( *m_database_t ); \
                                              var = CMYTH->call; \
                                              m_database_t->Unlock(); \
                                          } \

MythDatabase::MythDatabase()
  :m_database_t()
{
}


MythDatabase::MythDatabase(CStdString server,CStdString database,CStdString user,CStdString password):
m_database_t(new MythPointerThreadSafe<cmyth_database_t>())
{
  char *cserver=strdup(server.c_str());
  char *cdatabase=strdup(database.c_str());
  char *cuser=strdup(user.c_str());
  char *cpassword=strdup(password.c_str());

  *m_database_t=(CMYTH->cmyth_database_init(cserver,cdatabase,cuser,cpassword));

  free(cserver);
  free(cdatabase);
  free(cuser);
  free(cpassword);
}

bool  MythDatabase::TestConnection(CStdString &msg)
{
  char* cmyth_msg;
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, cmyth_mysql_testdb_connection( *m_database_t, &cmyth_msg ) );
  msg=cmyth_msg;
  CMYTH->ref_release(cmyth_msg);
  return retval == 1;
}


std::map<int,MythChannel> MythDatabase::ChannelList()
{
  std::map<int,MythChannel> retval;
  cmyth_chanlist_t cChannels = NULL;
  CMYTH_DB_CALL( cChannels, cChannels == NULL, cmyth_mysql_get_chanlist( *m_database_t ) );
  int nChannels=CMYTH->cmyth_chanlist_get_count(cChannels);
  for(int i=0;i<nChannels;i++)
  {
    cmyth_channel_t chan=CMYTH->cmyth_chanlist_get_item(cChannels,i);
    int chanid=CMYTH->cmyth_channel_chanid(chan);
    retval.insert(std::pair<int,MythChannel>(chanid,MythChannel(chan,1==CMYTH->cmyth_mysql_is_radio(*m_database_t,chanid))));
  }
  CMYTH->ref_release(cChannels);
  return retval;
}

std::vector<MythProgram> MythDatabase::GetGuide(time_t starttime, time_t endtime)
{
  MythProgram *programs=0;
  m_database_t->Lock();
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, cmyth_mysql_get_guide( *m_database_t, &programs, starttime, endtime ) );
  m_database_t->Unlock();
  if(len<1)
    return std::vector<MythProgram>();
  std::vector<MythProgram> retval(programs,programs+len);
  CMYTH->ref_release(programs);
  return retval;
}

std::map<int, MythTimer> MythDatabase::GetTimers()
{
  std::map<int, MythTimer> retval;
  cmyth_timerlist_t timers = NULL;
  CMYTH_DB_CALL( timers, timers == NULL, cmyth_mysql_get_timers( *m_database_t ) );
  int nTimers=CMYTH->cmyth_timerlist_get_count(timers);
  for(int i=0;i<nTimers;i++)
  {
    cmyth_timer_t timer=CMYTH->cmyth_timerlist_get_item(timers,i);
    MythTimer t(timer);
    retval.insert(std::pair<int,MythTimer>(t.RecordID(),t));
  }
  CMYTH->ref_release(timers);
  return retval;
}

std::vector<MythRecordingProfile > MythDatabase::GetRecordingProfiles()
{
  std::vector<MythRecordingProfile > retval;
  cmyth_recprofile* cmythProfiles;
  m_database_t->Lock();
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, cmyth_mysql_get_recprofiles( *m_database_t, &cmythProfiles ) );
  for(int i=0;i<len;i++)
  {
    std::vector<MythRecordingProfile >::iterator it = std::find(retval.begin(),retval.end(),CStdString(cmythProfiles[i].cardtype));
    if(it == retval.end())
    {
      retval.push_back(MythRecordingProfile());
      it = --retval.end();
      it->Format("%s",cmythProfiles[i].cardtype);
    }
    it->profile.insert(std::pair<int, CStdString >(cmythProfiles[i].id,cmythProfiles[i].name));
  }
  CMYTH->ref_release(cmythProfiles);
  m_database_t->Unlock();
  return retval;
}

int MythDatabase::SetWatchedStatus(MythProgramInfo &recording, bool watched)
{
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, cmyth_set_watched_status_mysql( *m_database_t, *recording.m_proginfo_t, watched?1:0 ) );
  return retval;
}

int MythDatabase::AddTimer(MythTimer &timer)
{
  int retval=0;
  CMYTH_DB_CALL( retval, retval < 0, cmyth_mysql_add_timer( *m_database_t, timer.ChanID(), timer.m_callsign.Buffer(),
                                                    timer.m_description.Buffer(), timer.StartTime(), timer.EndTime(),
                                                    timer.m_title.Buffer(), timer.m_category.Buffer(), timer.Type(),
                                                    timer.m_subtitle.Buffer(), timer.Priority(), timer.StartOffset(),
                                                    timer.EndOffset(), timer.SearchType(), timer.Inactive()?1:0,
                                                    timer.DupMethod(), timer.CheckDupIn(), timer.RecGroup().Buffer(),
                                                    timer.StoreGroup().Buffer(), timer.PlayGroup().Buffer(),
                                                    timer.AutoTranscode(), timer.Userjobs(), timer.AutoCommFlag(),
                                                    timer.AutoExpire(), timer.MaxEpisodes(), timer.NewExpireOldRecord(),
                                                    timer.Transcoder() ) );
  timer.m_recordid=retval;
  return retval;
}

bool MythDatabase::DeleteTimer(int recordid)
{
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, cmyth_mysql_delete_timer( *m_database_t, recordid ) );
  return retval==0;
}

bool MythDatabase::UpdateTimer(MythTimer &timer)
{
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, cmyth_mysql_update_timer( *m_database_t, timer.RecordID(), timer.ChanID(), timer.m_callsign.Buffer(),
                                                       timer.m_description.Buffer(), timer.StartTime(), timer.EndTime(),
                                                       timer.m_title.Buffer(), timer.m_category.Buffer(), timer.Type(),
                                                       timer.m_subtitle.Buffer(), timer.Priority(), timer.StartOffset(),
                                                       timer.EndOffset(), timer.SearchType(), timer.Inactive()?1:0,
                                                       timer.DupMethod(), timer.CheckDupIn(), timer.RecGroup().Buffer(),
                                                       timer.StoreGroup().Buffer(), timer.PlayGroup().Buffer(),
                                                       timer.AutoTranscode(), timer.Userjobs(), timer.AutoCommFlag(),
                                                       timer.AutoExpire(), timer.MaxEpisodes(), timer.NewExpireOldRecord(),
                                                       timer.Transcoder() ) );
  return retval==0;
}

bool MythDatabase::FindProgram(const time_t starttime,const int channelid,CStdString &title,MythProgram* pprogram)
{
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, cmyth_mysql_get_prog_finder_time_title_chan( *m_database_t, pprogram, title.Buffer(), starttime, channelid ) );
  return retval>0;
}

boost::unordered_map< CStdString, std::vector< int > > MythDatabase::GetChannelGroups()
{
  boost::unordered_map< CStdString, std::vector< int > > retval;
  cmyth_channelgroups_t *cg =0;
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, cmyth_mysql_get_channelgroups( *m_database_t, &cg ) );
  if(!cg)
    return retval;
  m_database_t->Lock();
  for(int i=0;i<len;i++)
  {
    MythChannelGroup changroup;
    changroup.first=cg[i].channelgroup;
    int* chanid=0;
    int numchan=CMYTH->cmyth_mysql_get_channelids_in_group(*m_database_t,cg[i].ID,&chanid);
    if(numchan)
    {
      changroup.second=std::vector<int>(chanid,chanid+numchan);
      CMYTH->ref_release(chanid);
    }
    else 
      changroup.second=std::vector<int>();
    
    retval.insert(changroup);
  }
  CMYTH->ref_release(cg);
  m_database_t->Unlock();
  return retval;
}

std::map< int, std::vector< int > > MythDatabase::SourceList()
{
  std::map< int, std::vector< int > > retval;
  cmyth_rec_t *r=0;
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, cmyth_mysql_get_recorder_list( *m_database_t, &r ) );
  m_database_t->Lock();
  for(int i=0;i<len;i++)
  {
    std::map< int, std::vector< int > >::iterator it=retval.find(r[i].sourceid);
    if(it!=retval.end())
      it->second.push_back(r[i].recid);
    else
      retval[r[i].sourceid]=std::vector<int>(1,r[i].recid);
  }
  CMYTH->ref_release(r);
  r=0;
  m_database_t->Unlock();
  return retval;
}

bool MythDatabase::IsNull()
{
  if(m_database_t==NULL)
    return true;
  return *m_database_t==NULL;
}

long long MythDatabase::GetBookmarkMark(MythProgramInfo &recording, long long bk, int mode)
{
  long long mark = 0;
  CMYTH_DB_CALL(mark, mark < 0, cmyth_get_bookmark_mark(*m_database_t, *recording.m_proginfo_t, bk, mode));
  return mark;
}
