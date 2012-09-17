#include "MythConnection.h"
#include "MythRecorder.h"
#include "MythFile.h"
#include "MythSGFile.h"
#include "MythProgramInfo.h"
#include "MythEventHandler.h"
#include "MythTimer.h"
#include "client.h"

using namespace ADDON;

/* Call 'call', then if 'cond' condition is true see if we're still
 * connected to the control socket and try to re-connect if not.
 * If reconnection is ok, call 'call' again. */
#define CMYTH_CONN_CALL( var, cond, call )  Lock(); \
                                            var = CMYTH->call; \
                                            Unlock(); \
                                            if ( cond ) \
                                            { \
                                                if ( !IsConnected() && TryReconnect() ) \
                                                { \
                                                    Lock(); \
                                                    var = CMYTH->call; \
                                                    Unlock(); \
                                                } \
                                            } \

/* Similar to CMYTH_CONN_CALL, but it will release 'var' if it was not NULL
 * right before calling 'call' again. */
#define CMYTH_CONN_CALL_REF( var, cond, call )  Lock(); \
                                                var = CMYTH->call; \
                                                Unlock(); \
                                                if ( cond ) \
                                                { \
                                                    if ( !IsConnected() && TryReconnect() ) \
                                                    { \
                                                        Lock(); \
                                                        if ( var != NULL ) \
                                                            CMYTH->RefRelease( var ); \
                                                        var = CMYTH->call; \
                                                        Unlock(); \
                                                    } \
                                                } \

/*   
*								MythConnection
*/

MythEventHandler * MythConnection::m_pEventHandler = NULL;

MythConnection::MythConnection():
m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>()),m_server(""),m_port(0),m_retry_count(0)
{  
}


MythConnection::MythConnection(CStdString server,unsigned short port):
m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>),m_server(server),m_port(port),m_retry_count(0)
{
  char *cserver=strdup(server.c_str());
  cmyth_conn_t connection=CMYTH->ConnConnectCtrl(cserver,port,64*1024, 16*1024);
  free(cserver);
  *m_conn_t=(connection);
  
}

std::vector< CStdString > MythConnection::GetStorageGroupFileList_(CStdString sgGetList)
  {
    std::vector< CStdString > retval;
    Lock();
    char **sg;
    CStdString bckHostNme = GetBackendHostname();
    int len = CMYTH->StoragegroupFilelist(*m_conn_t,&sg,sgGetList.Buffer(),bckHostNme.Buffer());
    if(!sg)
      return retval;
    for(int i=0;i<len;i++)
    {
      char *tmp=sg[i];
      CStdString tmpSG(tmp);
      XBMC->Log(LOG_DEBUG,"%s - ############################# - %s",__FUNCTION__,tmpSG.c_str());
      retval.push_back(tmpSG/*.c_str()*/);
    }
    
    CMYTH->RefRelease(sg);
    Unlock();
    return retval;
  }

std::vector< MythSGFile > MythConnection::GetStorageGroupFileList(CStdString storagegroup)
{
    CStdString hostname = GetBackendHostname();
    cmyth_storagegroup_filelist_t filelist = NULL;
    CMYTH_CONN_CALL_REF( filelist, filelist == NULL, StoragegroupGetFilelist( *m_conn_t, storagegroup.Buffer(), hostname.Buffer() ) );
    Lock();
    int len=CMYTH->StoragegroupFilelistCount(filelist);
    std::vector< MythSGFile >  retval(len);
    for(int i=0;i<len;i++)
    {
      cmyth_storagegroup_file_t file=CMYTH->StoragegroupFilelistGetItem(filelist,i);
      retval.push_back(MythSGFile(file));
    }
    CMYTH->RefRelease(filelist);
    Unlock();
    return retval;
}

MythFile MythConnection::ConnectPath(CStdString filename, CStdString storageGroup)
{
  cmyth_file_t file = NULL;
  CMYTH_CONN_CALL_REF( file, file == NULL, ConnConnectPath( filename.Buffer(),*m_conn_t,64*1024, 16*1024,storageGroup.Buffer()) );
  MythFile retval = MythFile( file, *this );
  return retval;
}

bool MythConnection::IsConnected()
{
  Lock();
  bool connected = *m_conn_t != 0 && !CMYTH->ConnHung(*m_conn_t);
  Unlock();
  if ( g_bExtraDebug )
    XBMC->Log( LOG_DEBUG, "%s - %i", __FUNCTION__, connected );
  return connected;
}

bool MythConnection::TryReconnect()
{
    bool retval = false;
    if ( m_retry_count < 10 )
    {
        m_retry_count++;
        Lock();
        retval = CMYTH->ConnReconnectCtrl( *m_conn_t );
        Unlock();
        if ( retval )
        {
            m_retry_count = 0;
            if ( MythConnection::m_pEventHandler && !MythConnection::m_pEventHandler->TryReconnect() )
            {
                XBMC->Log( LOG_ERROR, "%s - Unable to reconnect event handler", __FUNCTION__ );
            }
        }
    }
    if ( g_bExtraDebug && !retval )
        XBMC->Log( LOG_DEBUG, "%s - Unable to reconnect (retry count: %d)", __FUNCTION__, m_retry_count );
    return retval;
}

MythRecorder MythConnection::GetFreeRecorder()
{
  cmyth_recorder_t rec = NULL;
  CMYTH_CONN_CALL_REF( rec, rec == NULL, ConnGetFreeRecorder( *m_conn_t ) );
  MythRecorder retval = MythRecorder( rec, *this );
  return retval;
}

MythRecorder MythConnection::GetRecorder(int n)
{
  cmyth_recorder_t rec = NULL;
  CMYTH_CONN_CALL_REF( rec, rec == NULL, ConnGetRecorderFromNum( *m_conn_t, n ) );
  MythRecorder retval = MythRecorder( rec, *this );
  return retval;
}

  boost::unordered_map<CStdString, MythProgramInfo>  MythConnection::GetRecordedPrograms()
  {
    Lock();
    boost::unordered_map<CStdString, MythProgramInfo>  retval;
    cmyth_proglist_t proglist = NULL;
    CMYTH_CONN_CALL_REF( proglist, proglist == NULL, ProglistGetAllRecorded( *m_conn_t ) );
    int len=CMYTH->ProglistGetCount(proglist);
    for(int i=0;i<len;i++)
    {
      cmyth_proginfo_t cmprog=CMYTH->ProglistGetItem(proglist,i);
      MythProgramInfo prog=CMYTH->ProginfoGetDetail(*m_conn_t,cmprog);
      CStdString path=prog.Path();
      retval.insert(std::pair<CStdString,MythProgramInfo>(path.c_str(),prog));
    }
    CMYTH->RefRelease(proglist);
    Unlock();
    return retval;
  }

  boost::unordered_map<CStdString, MythProgramInfo>  MythConnection::GetPendingPrograms()
   {
    Lock();
    boost::unordered_map<CStdString, MythProgramInfo>  retval;
    cmyth_proglist_t proglist = NULL;
    CMYTH_CONN_CALL_REF( proglist, proglist == NULL, ProglistGetAllPending( *m_conn_t ) );
    int len=CMYTH->ProglistGetCount(proglist);
    for(int i=0;i<len;i++)
    {
      MythProgramInfo prog=CMYTH->ProglistGetItem(proglist,i);
      CStdString filename;
      filename.Format("%i_%i",prog.ChannelID(),prog.StartTime());
      retval.insert(std::pair<CStdString,MythProgramInfo>(filename.c_str(),prog));
    }
    CMYTH->RefRelease(proglist);
    Unlock();
    return retval;
  }

  boost::unordered_map<CStdString, MythProgramInfo>  MythConnection::GetScheduledPrograms()
   {
    Lock();
    boost::unordered_map<CStdString, MythProgramInfo>  retval;
    cmyth_proglist_t proglist = NULL;
    CMYTH_CONN_CALL_REF( proglist, proglist == NULL, ProglistGetAllScheduled( *m_conn_t ) );
    int len=CMYTH->ProglistGetCount(proglist);
    for(int i=0;i<len;i++)
    {
      cmyth_proginfo_t cmprog=CMYTH->ProglistGetItem(proglist,i);
      MythProgramInfo prog=CMYTH->ProginfoGetDetail(*m_conn_t,cmprog);//Release cmprog????
      CStdString path=prog.Path();
      retval.insert(std::pair<CStdString,MythProgramInfo>(path.c_str(),prog));
    }
    CMYTH->RefRelease(proglist);
    Unlock();
    return retval;
  }

  bool  MythConnection::DeleteRecording(MythProgramInfo &recording)
  {
    int retval = 0;
    CMYTH_CONN_CALL( retval, retval != 0, ProginfoDeleteRecording( *m_conn_t, *recording.m_proginfo_t ) );
    return retval==0;
  }


MythEventHandler * MythConnection::CreateEventHandler()
{
    if ( MythConnection::m_pEventHandler )
    {
        delete( MythConnection::m_pEventHandler );
        MythConnection::m_pEventHandler = NULL;
    }
    MythConnection::m_pEventHandler = new MythEventHandler( m_server, m_port );
    return MythConnection::m_pEventHandler;
}

CStdString MythConnection::GetServer()
{
  return m_server;
}

int MythConnection::GetProtocolVersion()
{
  int retval = 0;
  CMYTH_CONN_CALL( retval, retval < 0, ConnGetProtocolVersion( *m_conn_t ) );
  return retval;
}

bool MythConnection::GetDriveSpace(long long &total,long long &used)
{
  int retval = 0;
  CMYTH_CONN_CALL( retval, retval != 0, ConnGetFreespace( *m_conn_t, &total, &used ) );
  return retval==0;
}

bool MythConnection::UpdateSchedules(int id)
{
  CStdString cmd;
  cmd.Format("RESCHEDULE_RECORDINGS %i",id);
  int retval = 0;
  CMYTH_CONN_CALL( retval, retval < 0, ScheduleRecording( *m_conn_t, cmd.Buffer() ) );
  return retval>=0;
}

MythFile MythConnection::ConnectFile(MythProgramInfo &recording)
{
  cmyth_file_t file = NULL;
  /* When file is not NULL doesn't need to mean there is no more connection,
   * so always check after callng ConnConnectFile if still connected to control socket. */
  CMYTH_CONN_CALL_REF( file, true, ConnConnectFile( *recording.m_proginfo_t, *m_conn_t, 64 * 1024, 16 * 1024 ) );
  MythFile retval = MythFile( file, *this );
  return retval;
}

bool MythConnection::IsNull()
{
  if(m_conn_t==NULL)
    return true;
  return *m_conn_t==NULL;
}

void MythConnection::Lock()
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"Lock %i",m_conn_t.get());
  m_conn_t->Lock();
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"Lock acquired %i",m_conn_t.get());
}

void MythConnection::Unlock()
{
  if(g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"Unlock %i",m_conn_t.get());
  m_conn_t->Unlock();
  
}

  CStdString MythConnection::GetSetting(CStdString hostname,CStdString setting)
  {
    CStdString retval;
    Lock();
    char * value = NULL;
    CMYTH_CONN_CALL_REF( value, value == NULL, ConnGetSetting( *m_conn_t, hostname.Buffer(), setting.Buffer() ) );
    retval = value;
    CMYTH->RefRelease(value);
    value = NULL;
    Unlock();
    return retval;
  }

  bool MythConnection::SetSetting(CStdString hostname,CStdString setting,CStdString value)
  {
    int retval = 0;
    Lock();
    CMYTH_CONN_CALL( retval, retval < 0, ConnSetSetting( *m_conn_t, hostname.Buffer(), setting.Buffer(), value.Buffer() ) );
    Unlock();
    return retval>=0;
  }

  CStdString MythConnection::GetHostname()
  {
    CStdString retval;
    Lock();
    char * hostname = NULL;
    CMYTH_CONN_CALL_REF( hostname, hostname == NULL, ConnGetClientHostname( *m_conn_t ) );
    retval = hostname;
    CMYTH->RefRelease(hostname);
    hostname = NULL;
    Unlock();
    return retval;
  }

  CStdString MythConnection::GetBackendHostname()
  {
    CStdString retval;
    Lock();
    char * hostname = NULL;
    CMYTH_CONN_CALL_REF( hostname, hostname == NULL, ConnGetBackendHostname( *m_conn_t ) );
    retval = hostname;
    CMYTH->RefRelease(hostname);
    hostname = NULL;
    Unlock();
    return retval;
  }


  
/*
Profile = Default
recpriority = 0
maxepisodes = 0
maxnewest = 0
recgroup = Default
dupmethod = 6
dupin = 15
playgroup = Default
storagegroup = Default

Defaults for MythTimer
AutoTranscode / DefaultTranscoder
AutoRunUserJob1
AutoRunUserJob2
AutoRunUserJob3
AutoRunUserJob4
autocommflag => AutoCommercialFlag
AutoExpireDefault
transcoder => DefaultTranscoder?? 

start/endoffset => DefaultStartOffset/DefaultEndOffset

*/

  void MythConnection::DefaultTimer(MythTimer &timer)
  {
    timer.AutoTranscode(atoi(GetSetting("NULL","AutoTranscode").c_str())>0);
    timer.Userjob(1,atoi(GetSetting("NULL","AutoRunUserJob1").c_str())>0);
    timer.Userjob(2,atoi(GetSetting("NULL","AutoRunUserJob2").c_str())>0);
    timer.Userjob(3,atoi(GetSetting("NULL","AutoRunUserJob3").c_str())>0);
    timer.Userjob(4,atoi(GetSetting("NULL","AutoRunUserJob4").c_str())>0);
    timer.AutoCommFlag(atoi(GetSetting("NULL","AutoCommercialFlag").c_str())>0);
    timer.AutoExpire(atoi(GetSetting("NULL","AutoExpireDefault").c_str())>0);
    timer.Transcoder(atoi(GetSetting("NULL","DefaultTranscoder").c_str()));
    timer.StartOffset(atoi(GetSetting("NULL","DefaultStartOffset").c_str()));
    timer.StartOffset(atoi(GetSetting("NULL","DefaultEndOffset").c_str()));
        
  }
