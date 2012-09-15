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
                                              CMYTH->DatabaseClose( *m_database_t ); \
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

  *m_database_t=(CMYTH->DatabaseInit(cserver,cdatabase,cuser,cpassword));

  free(cserver);
  free(cdatabase);
  free(cuser);
  free(cpassword);
}

bool  MythDatabase::TestConnection(CStdString &msg)
{
  char* cmyth_msg;
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, MysqlTestdbConnection( *m_database_t, &cmyth_msg ) );
  msg=cmyth_msg;
  CMYTH->RefRelease(cmyth_msg);
  return retval == 1;
}


std::map<int,MythChannel> MythDatabase::ChannelList()
{
  std::map<int,MythChannel> retval;
  cmyth_chanlist_t cChannels = NULL;
  CMYTH_DB_CALL( cChannels, cChannels == NULL, MysqlGetChanlist( *m_database_t ) );
  int nChannels=CMYTH->ChanlistGetCount(cChannels);
  for(int i=0;i<nChannels;i++)
  {
    cmyth_channel_t chan=CMYTH->ChanlistGetItem(cChannels,i);
    int chanid=CMYTH->ChannelChanid(chan);
    retval.insert(std::pair<int,MythChannel>(chanid,MythChannel(chan,1==CMYTH->MysqlIsRadio(*m_database_t,chanid))));
  }
  CMYTH->RefRelease(cChannels);
  return retval;
}

std::vector<MythProgram> MythDatabase::GetGuide(time_t starttime, time_t endtime)
{
  MythProgram *programs=0;
  m_database_t->Lock();
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, MysqlGetGuide( *m_database_t, &programs, starttime, endtime ) );
  m_database_t->Unlock();
  if(len<1)
    return std::vector<MythProgram>();
  std::vector<MythProgram> retval(programs,programs+len);
  CMYTH->RefRelease(programs);
  return retval;
}

std::map<int, MythTimer> MythDatabase::GetTimers()
{
  std::map<int, MythTimer> retval;
  cmyth_timerlist_t timers = NULL;
  CMYTH_DB_CALL( timers, timers == NULL, MysqlGetTimers( *m_database_t ) );
  int nTimers=CMYTH->TimerlistGetCount(timers);
  for(int i=0;i<nTimers;i++)
  {
    cmyth_timer_t timer=CMYTH->TimerlistGetItem(timers,i);
    MythTimer t(timer);
    retval.insert(std::pair<int,MythTimer>(t.RecordID(),t));
  }
  CMYTH->RefRelease(timers);
  return retval;
}

std::vector<MythRecordingProfile > MythDatabase::GetRecordingProfiles()
{
  std::vector<MythRecordingProfile > retval;
  cmyth_recprofile* cmythProfiles;
  m_database_t->Lock();
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, MysqlGetRecprofiles( *m_database_t, &cmythProfiles ) );
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
  CMYTH->RefRelease(cmythProfiles);
  m_database_t->Unlock();
  return retval;
}

int MythDatabase::SetWatchedStatus(MythProgramInfo &recording, bool watched)
{
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, SetWatchedStatusMysql( *m_database_t, *recording.m_proginfo_t, watched?1:0 ) );
  return retval;
}

int MythDatabase::AddTimer(MythTimer &timer)
{
  int retval=0;
  CMYTH_DB_CALL( retval, retval < 0, MysqlAddTimer( *m_database_t, timer.ChanID(), timer.m_callsign.Buffer(),
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
  CMYTH_DB_CALL( retval, retval < 0, MysqlDeleteTimer( *m_database_t, recordid ) );
  return retval==0;
}

bool MythDatabase::UpdateTimer(MythTimer &timer)
{
  int retval = 0;
  CMYTH_DB_CALL( retval, retval < 0, MysqlUpdateTimer( *m_database_t, timer.RecordID(), timer.ChanID(), timer.m_callsign.Buffer(),
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
  CMYTH_DB_CALL( retval, retval < 0, MysqlGetProgFinderTimeTitleChan( *m_database_t, pprogram, title.Buffer(), starttime, channelid ) );
  return retval>0;
}

boost::unordered_map< CStdString, std::vector< int > > MythDatabase::GetChannelGroups()
{
  boost::unordered_map< CStdString, std::vector< int > > retval;
  cmyth_channelgroups_t *cg =0;
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, MysqlGetChannelgroups( *m_database_t, &cg ) );
  if(!cg)
    return retval;
  m_database_t->Lock();
  for(int i=0;i<len;i++)
  {
    MythChannelGroup changroup;
    changroup.first=cg[i].channelgroup;
    int* chanid=0;
    int numchan=CMYTH->MysqlGetChannelidsInGroup(*m_database_t,cg[i].ID,&chanid);
    if(numchan)
    {
      changroup.second=std::vector<int>(chanid,chanid+numchan);
      CMYTH->RefRelease(chanid);
    }
    else 
      changroup.second=std::vector<int>();
    
    retval.insert(changroup);
  }
  CMYTH->RefRelease(cg);
  m_database_t->Unlock();
  return retval;
}

std::map< int, std::vector< int > > MythDatabase::SourceList()
{
  std::map< int, std::vector< int > > retval;
  cmyth_rec_t *r=0;
  int len = 0;
  CMYTH_DB_CALL( len, len < 0, MysqlGetRecorderList( *m_database_t, &r ) );
  m_database_t->Lock();
  for(int i=0;i<len;i++)
  {
    std::map< int, std::vector< int > >::iterator it=retval.find(r[i].sourceid);
    if(it!=retval.end())
      it->second.push_back(r[i].recid);
    else
      retval[r[i].sourceid]=std::vector<int>(1,r[i].recid);
  }
  CMYTH->RefRelease(r);
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
