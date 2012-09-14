#include "MythEventHandler.h"
#include "MythRecorder.h"
#include "MythSignal.h"
#include "../client.h"

using namespace ADDON;
/*
 *            Tokenizer
 */

template < class ContainerT >
void tokenize(const std::string& str, ContainerT& tokens,
              const std::string& delimiters = " ", const bool trimEmpty = false)
{
   std::string::size_type pos, lastPos = 0;
   while(true)
   {
      pos = str.find_first_of(delimiters, lastPos);
      if(pos == std::string::npos)
      {
         pos = str.length();

         if(pos != lastPos || !trimEmpty)
            tokens.push_back(typename ContainerT::value_type(str.data()+lastPos,(pos)-lastPos ));
                  //static_cast<ContainerT::value_type::size_type>*/(pos)-lastPos ));

         break;
      }
      else
      {
         if(pos != lastPos || !trimEmpty)
            tokens.push_back(typename ContainerT::value_type(str.data()+lastPos,(pos)-lastPos ));
                  //static_cast<ContainerT::value_type::size_type>(pos)-lastPos ));
      }

      lastPos = pos + 1;
   }
};

/*
*								MythEventHandler
*/

class MythEventHandler::ImpMythEventHandler : public CThread, public CMutex
{
  friend class MythEventHandler;
public:
  ImpMythEventHandler(CStdString server,unsigned short port);
  MythRecorder m_rec;
  MythSignal m_signal;
  MythFile m_file;
  CStdString curRecordingId;
  void UpdateFilesize(CStdString signal);
  void SetRecEventListener(MythFile &file, CStdString recId);
  boost::shared_ptr< MythPointerThreadSafe< cmyth_conn_t > > m_conn_t;
  virtual void* Process(void);	
  virtual ~ImpMythEventHandler();
  void UpdateSignal(CStdString &signal);
  CStdString m_server;
  unsigned short m_port;
  };

MythEventHandler::ImpMythEventHandler::ImpMythEventHandler(CStdString server,unsigned short port)
:m_rec(MythRecorder()),m_conn_t(new MythPointerThreadSafe<cmyth_conn_t>()),CThread(),m_signal(),CMutex(),m_server(server),m_port(port)
  {
    char *cserver=strdup(server.c_str());
    cmyth_conn_t connection=CMYTH->ConnConnectEvent(cserver,port,64*1024, 16*1024);
    free(cserver);
    *m_conn_t=connection;
  }


  MythEventHandler::ImpMythEventHandler::~ImpMythEventHandler()
  {
    StopThread(30);
    CMYTH->RefRelease(*m_conn_t);
    *m_conn_t=0;
  }

MythEventHandler::MythEventHandler(CStdString server,unsigned short port)
  :m_imp(new ImpMythEventHandler(server,port))
{
  m_imp->CreateThread();
}

MythEventHandler::MythEventHandler()
  :m_imp()
{
}

void MythEventHandler::Stop()
{
  m_imp.reset();
}

void MythEventHandler::PreventLiveChainUpdate()
{
  m_imp->Lock();
}

void MythEventHandler::AllowLiveChainUpdate()
{
  m_imp->Unlock();
}

void MythEventHandler::SetRecorder(MythRecorder &rec)
{
  m_imp->Lock();
  m_imp->m_rec=rec;
  m_imp->Unlock();
}

MythSignal MythEventHandler::GetSignal()
{
  return m_imp->m_signal;
}

void MythEventHandler::SetRecordingListener ( MythFile &file, CStdString recId )
{
  m_imp->Lock();
  m_imp->SetRecEventListener(file,recId);
  m_imp->Unlock();
}

bool MythEventHandler::TryReconnect()
{
    bool retval = false;
    if ( m_retry_count < 10 )
    {
        XBMC->Log( LOG_DEBUG, "%s - Trying to reconnect (retry count: %d)", __FUNCTION__, m_retry_count );
        m_retry_count++;
        m_imp->m_conn_t->Lock();
        retval = CMYTH->ConnReconnectEvent( *(m_imp->m_conn_t) );
        m_imp->m_conn_t->Unlock();
        if ( retval )
            m_retry_count = 0;
    }
    if ( g_bExtraDebug && !retval )
        XBMC->Log( LOG_DEBUG, "%s - Unable to reconnect (retry count: %d)", __FUNCTION__, m_retry_count );
    return retval;
}


void MythEventHandler::ImpMythEventHandler::SetRecEventListener ( MythFile &file, CStdString recId )
{
  m_file = file;
  char b [20];
  sscanf(recId.c_str(),"/%4s_%14s",b,b+4);
  CStdString uniqId=b;
  curRecordingId = uniqId;
}

void MythEventHandler::ImpMythEventHandler::UpdateFilesize(CStdString signal)
{
  long long length;
  char b [20];
  sscanf(signal.c_str(),"%4s %4s-%2s-%2sT%2s:%2s:%2s %lli",b,b+4,b+8,b+10,b+12,b+14,b+16,&length);
  
  CStdString uniqId=b;
  
  if (curRecordingId.compare(uniqId) == 0) {
    if(g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"EVENT: %s, --UPDATING CURRENT RECORDING LENGTH-- EVENT msg: %s %ll",
	  __FUNCTION__,uniqId.c_str(),length);
    m_file.UpdateDuration(length);
  }
}

void MythEventHandler::ImpMythEventHandler::UpdateSignal(CStdString &signal)
{
  std::vector< std::string > tok;
  tokenize<std::vector< std::string > >( signal, tok, ";");
  
  for(std::vector<std::string>::iterator it=tok.begin();it!=tok.end();it++)
  {
    std::vector< std::string > tok2;
    tokenize< std::vector< std::string > >(*it,tok2," ");
    if(tok2.size()>=2)
    {
    if(tok2[0]=="slock")
    {
      m_signal.m_AdapterStatus=tok2[1]=="1"?"Locked":"No lock";
    }
    else if(tok2[0]=="signal")
    {
      m_signal.m_Signal=atoi(tok2[1].c_str());
    }
    else if(tok2[0]=="snr")
    {
      m_signal.m_SNR=std::atoi(tok2[1].c_str());
    }
    else if(tok2[0]=="ber")
    {
      m_signal.m_BER=std::atoi(tok2[1].c_str());
    }
    else if(tok2[0]=="ucb")
    {
      m_signal.m_UNC=std::atoi(tok2[1].c_str());
    }
    else if(tok2[0]=="cardid")
    {
      m_signal.m_ID=std::atoi(tok2[1].c_str());
    }
    }
  }
}

void* MythEventHandler::ImpMythEventHandler::Process(void)
{
  const char* events[]={	"CMYTH_EVENT_UNKNOWN",\
    "CMYTH_EVENT_CLOSE",\
    "CMYTH_EVENT_RECORDING_LIST_CHANGE",\
    "CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD",\
    "CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE",\
    "CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE",\
    "CMYTH_EVENT_SCHEDULE_CHANGE",\
    "CMYTH_EVENT_DONE_RECORDING",\
    "CMYTH_EVENT_QUIT_LIVETV",\
    "CMYTH_EVENT_WATCH_LIVETV",\
    "CMYTH_EVENT_LIVETV_CHAIN_UPDATE",\
    "CMYTH_EVENT_SIGNAL",\
    "CMYTH_EVENT_ASK_RECORDING",\
    "CMYTH_EVENT_SYSTEM_EVENT",\
    "CMYTH_EVENT_UPDATE_FILE_SIZE",\
    "CMYTH_EVENT_GENERATED_PIXMAP",\
    "CMYTH_EVENT_CLEAR_SETTINGS_CACHE"};
  cmyth_event_t myth_event;
  char databuf[2049];
  databuf[0]=0;
  timeval timeout;
  timeout.tv_sec=0;
  timeout.tv_usec=100000;

  while(!IsStopped())
  {
    int select = 0;
    m_conn_t->Lock();
    select = CMYTH->EventSelect(*m_conn_t,&timeout);
    m_conn_t->Unlock();
    if(select>0)
    {
        m_conn_t->Lock();
        myth_event=CMYTH->EventGet(*m_conn_t,databuf,2048);
        m_conn_t->Unlock();
        if(g_bExtraDebug)
            XBMC->Log(LOG_DEBUG,"EVENT ID: %s, EVENT databuf: %s",events[myth_event],databuf);
        if(myth_event==CMYTH_EVENT_UPDATE_FILE_SIZE)
        {
            CStdString signal=databuf;
            UpdateFilesize(signal);
            //1044 2012-03-20T20:00:00 54229688
            if(g_bExtraDebug)
                XBMC->Log(LOG_NOTICE,"%s: FILE_SIZE_UPDATE: %i",__FUNCTION__,databuf);
        }
        if(myth_event==CMYTH_EVENT_LIVETV_CHAIN_UPDATE)
        {
            Lock();
            if(!m_rec.IsNull())
            {
                bool retval=m_rec.LiveTVChainUpdate(CStdString(databuf));
                if(g_bExtraDebug)
                    XBMC->Log(LOG_NOTICE,"%s: CHAIN_UPDATE: %i",__FUNCTION__,retval);
            }
            else
                if(g_bExtraDebug)
                    XBMC->Log(LOG_NOTICE,"%s: CHAIN_UPDATE - No recorder",__FUNCTION__);
            Unlock();
        }
        if(myth_event==CMYTH_EVENT_SIGNAL)
        {
            CStdString signal=databuf;
            UpdateSignal(signal);
        }
        if(myth_event==CMYTH_EVENT_SCHEDULE_CHANGE)
        {
            if(g_bExtraDebug)
                XBMC->Log(LOG_NOTICE,"Schedule change",__FUNCTION__);
            PVR->TriggerTimerUpdate();
            PVR->TriggerRecordingUpdate();
        }
        if ( myth_event==CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD || myth_event==CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE || myth_event==CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE || myth_event==CMYTH_EVENT_RECORDING_LIST_CHANGE )
        {
            //XBMC->Log(LOG_NOTICE,"Recording list change",__FUNCTION__);
            PVR->TriggerRecordingUpdate();
        }
        if (myth_event==CMYTH_EVENT_CLOSE || myth_event==CMYTH_EVENT_UNKNOWN)
        {
            XBMC->Log( LOG_NOTICE, "%s - Event client connection closed", __FUNCTION__ );
        }

        databuf[0]=0;

    }
    else if ( select < 0 || CMYTH->ConnHung( *m_conn_t ) )
    {
        XBMC->Log( LOG_NOTICE, "%s - Select returned error; reconnect event client connection", __FUNCTION__ );
        m_conn_t->Lock();
        bool retval = CMYTH->ConnReconnectEvent( *m_conn_t );
        m_conn_t->Unlock();
        if ( retval )
            XBMC->Log( LOG_NOTICE, "%s - Connected client to event socket", __FUNCTION__ );
        else
            XBMC->Log( LOG_NOTICE, "%s - Could not connect client to event socket", __FUNCTION__ );
    }

    //Restore timeout
    timeout.tv_sec=0;
    timeout.tv_usec=100000;
  }
  return NULL;
}
