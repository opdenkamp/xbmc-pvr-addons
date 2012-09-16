#include "MythConnection.h"
#include "MythRecorder.h"
#include "MythFile.h"
#include "MythSGFile.h"
#include "MythProgramInfo.h"
#include "MythEventHandler.h"
#include "MythTimer.h"
#include "../client.h"

using namespace ADDON;

/* Call 'call', then if 'cond' condition is true see if we're still
 * connected to the control socket and try to re-connect if not.
 * If reconnection is ok, call 'call' again. */
#define CMYTH_CONN_CALL( var, cond, call )  Lock(); \
                                            var = call; \
                                            Unlock(); \
                                            if ( cond ) \
                                            { \
                                                if ( !IsConnected() && TryReconnect() ) \
                                                { \
                                                    Lock(); \
                                                    var = call; \
                                                    Unlock(); \
                                                } \
                                            } \

/* Similar to CMYTH_CONN_CALL, but it will release 'var' if it was not NULL
 * right before calling 'call' again. */
#define CMYTH_CONN_CALL_REF( var, cond, call )  Lock(); \
                                                var = call; \
                                                Unlock(); \
                                                if ( cond ) \
                                                { \
                                                    if ( !IsConnected() && TryReconnect() ) \
                                                    { \
                                                        Lock(); \
                                                        if ( var != NULL ) \
                                                            ref_release( var ); \
                                                        var = call; \
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
  cmyth_conn_t connection=cmyth_conn_connect_ctrl(cserver,port,64*1024, 16*1024);
  free(cserver);
  *m_conn_t=(connection);
  
}

std::vector< CStdString > MythConnection::GetStorageGroupFileList_(CStdString sgGetList)
  {
    std::vector< CStdString > retval;
    Lock();
    char **sg;
    CStdString bckHostNme = GetBackendHostname();
    int len = cmyth_storagegroup_filelist(*m_conn_t,&sg,sgGetList.Buffer(),bckHostNme.Buffer());
    if(!sg)
      return retval;
    for(int i=0;i<len;i++)
    {
      char *tmp=sg[i];
      CStdString tmpSG(tmp);
      XBMC->Log(LOG_DEBUG,"%s - ############################# - %s",__FUNCTION__,tmpSG.c_str());
      retval.push_back(tmpSG/*.c_str()*/);
    }
    
    ref_release(sg);
    Unlock();
    return retval;
  }

std::vector< MythSGFile > MythConnection::GetStorageGroupFileList(CStdString storagegroup)
{
    CStdString hostname = GetBackendHostname();
    cmyth_storagegroup_filelist_t filelist = NULL;
    CMYTH_CONN_CALL_REF( filelist, filelist == NULL, cmyth_storagegroup_get_filelist( *m_conn_t, storagegroup.Buffer(), hostname.Buffer() ) );
    Lock();
    int len=cmyth_storagegroup_filelist_count(filelist);
    std::vector< MythSGFile >  retval(len);
    for(int i=0;i<len;i++)
    {
      cmyth_storagegroup_file_t file=cmyth_storagegroup_filelist_get_item(filelist,i);
      retval.push_back(MythSGFile(file));
    }
    ref_release(filelist);
    Unlock();
    return retval;
}

MythFile MythConnection::ConnectPath(CStdString filename, CStdString storageGroup)
{
  cmyth_file_t file = NULL;
  CMYTH_CONN_CALL_REF( file, file == NULL, cmyth_conn_connect_path( filename.Buffer(),*m_conn_t,64*1024, 16*1024,storageGroup.Buffer()) );
  MythFile retval = MythFile( file, *this );
  return retval;
}

bool MythConnection::IsConnected()
{
  Lock();
  bool connected = *m_conn_t != 0 && !cmyth_conn_hung(*m_conn_t);
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
        retval = cmyth_conn_reconnect_ctrl( *m_conn_t );
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
  CMYTH_CONN_CALL_REF( rec, rec == NULL, cmyth_conn_get_free_recorder( *m_conn_t ) );
  MythRecorder retval = MythRecorder( rec, *this );
  return retval;
}

MythRecorder MythConnection::GetRecorder(int n)
{
  cmyth_recorder_t rec = NULL;
  CMYTH_CONN_CALL_REF( rec, rec == NULL, cmyth_conn_get_recorder_from_num( *m_conn_t, n ) );
  MythRecorder retval = MythRecorder( rec, *this );
  return retval;
}

  boost::unordered_map<CStdString, MythProgramInfo>  MythConnection::GetRecordedPrograms()
  {
    Lock();
    boost::unordered_map<CStdString, MythProgramInfo>  retval;
    cmyth_proglist_t proglist = NULL;
    CMYTH_CONN_CALL_REF( proglist, proglist == NULL, cmyth_proglist_get_all_recorded( *m_conn_t ) );
    int len=cmyth_proglist_get_count(proglist);
    for(int i=0;i<len;i++)
    {
      cmyth_proginfo_t cmprog=cmyth_proglist_get_item(proglist,i);
      MythProgramInfo prog=cmyth_proginfo_get_detail(*m_conn_t,cmprog);
      CStdString path=prog.Path();
      retval.insert(std::pair<CStdString,MythProgramInfo>(path.c_str(),prog));
    }
    ref_release(proglist);
    Unlock();
    return retval;
  }

  boost::unordered_map<CStdString, MythProgramInfo>  MythConnection::GetPendingPrograms()
   {
    Lock();
    boost::unordered_map<CStdString, MythProgramInfo>  retval;
    cmyth_proglist_t proglist = NULL;
    CMYTH_CONN_CALL_REF( proglist, proglist == NULL, cmyth_proglist_get_all_pending( *m_conn_t ) );
    int len=cmyth_proglist_get_count(proglist);
    for(int i=0;i<len;i++)
    {
      MythProgramInfo prog=cmyth_proglist_get_item(proglist,i);
      CStdString filename;
      filename.Format("%i_%i",prog.ChannelID(),prog.StartTime());
      retval.insert(std::pair<CStdString,MythProgramInfo>(filename.c_str(),prog));
    }
    ref_release(proglist);
    Unlock();
    return retval;
  }

  boost::unordered_map<CStdString, MythProgramInfo>  MythConnection::GetScheduledPrograms()
   {
    Lock();
    boost::unordered_map<CStdString, MythProgramInfo>  retval;
    cmyth_proglist_t proglist = NULL;
    CMYTH_CONN_CALL_REF( proglist, proglist == NULL, cmyth_proglist_get_all_scheduled( *m_conn_t ) );
    int len=cmyth_proglist_get_count(proglist);
    for(int i=0;i<len;i++)
    {
      cmyth_proginfo_t cmprog=cmyth_proglist_get_item(proglist,i);
      MythProgramInfo prog=cmyth_proginfo_get_detail(*m_conn_t,cmprog);//Release cmprog????
      CStdString path=prog.Path();
      retval.insert(std::pair<CStdString,MythProgramInfo>(path.c_str(),prog));
    }
    ref_release(proglist);
    Unlock();
    return retval;
  }

  bool  MythConnection::DeleteRecording(MythProgramInfo &recording)
  {
    int retval = 0;
    CMYTH_CONN_CALL( retval, retval != 0, cmyth_proginfo_delete_recording( *m_conn_t, *recording.m_proginfo_t ) );
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
  CMYTH_CONN_CALL( retval, retval < 0, cmyth_conn_get_protocol_version( *m_conn_t ) );
  return retval;
}

bool MythConnection::GetDriveSpace(long long &total,long long &used)
{
  int retval = 0;
  CMYTH_CONN_CALL( retval, retval != 0, cmyth_conn_get_freespace( *m_conn_t, &total, &used ) );
  return retval==0;
}

bool MythConnection::UpdateSchedules(int id)
{
  CStdString cmd;
  cmd.Format("RESCHEDULE_RECORDINGS %i",id);
  int retval = 0;
  CMYTH_CONN_CALL( retval, retval < 0, cmyth_schedule_recording( *m_conn_t, cmd.Buffer() ) );
  return retval>=0;
}

MythFile MythConnection::ConnectFile(MythProgramInfo &recording)
{
  cmyth_file_t file = NULL;
  /* When file is not NULL doesn't need to mean there is no more connection,
   * so always check after callng ConnConnectFile if still connected to control socket. */
  CMYTH_CONN_CALL_REF( file, true, cmyth_conn_connect_file( *recording.m_proginfo_t, *m_conn_t, 64 * 1024, 16 * 1024 ) );
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
    CMYTH_CONN_CALL_REF( value, value == NULL, cmyth_conn_get_setting( *m_conn_t, hostname.Buffer(), setting.Buffer() ) );
    retval = value;
    ref_release(value);
    value = NULL;
    Unlock();
    return retval;
  }

  bool MythConnection::SetSetting(CStdString hostname,CStdString setting,CStdString value)
  {
    int retval = 0;
    Lock();
    CMYTH_CONN_CALL( retval, retval < 0, cmyth_conn_set_setting( *m_conn_t, hostname.Buffer(), setting.Buffer(), value.Buffer() ) );
    Unlock();
    return retval>=0;
  }

  CStdString MythConnection::GetHostname()
  {
    CStdString retval;
    Lock();
    char * hostname = NULL;
    CMYTH_CONN_CALL_REF( hostname, hostname == NULL, cmyth_conn_get_client_hostname( *m_conn_t ) );
    retval = hostname;
    ref_release(hostname);
    hostname = NULL;
    Unlock();
    return retval;
  }

  CStdString MythConnection::GetBackendHostname()
  {
    CStdString retval;
    Lock();
    char * hostname = NULL;
    CMYTH_CONN_CALL_REF( hostname, hostname == NULL, cmyth_conn_get_backend_hostname( *m_conn_t ) );
    retval = hostname;
    ref_release(hostname);
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
