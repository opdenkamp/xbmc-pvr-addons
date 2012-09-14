
#include "MythFile.h"
#include "../client.h"


using namespace ADDON;

/*
 *        MythFile
 */

/* Call 'call', then if 'cond' condition is true see if we're still
 * connected to the control socket and try to re-connect if not.
 * If reconnection is ok, call 'call' again. */
#define CMYTH_FILE_CALL( var, cond, call )  m_conn.Lock(); \
                                            var = CMYTH->call; \
                                            m_conn.Unlock(); \
                                            if ( cond ) \
                                            { \
                                                if ( !m_conn.IsConnected() && m_conn.TryReconnect() ) \
                                                { \
                                                    m_conn.Lock(); \
                                                    var = CMYTH->call; \
                                                    m_conn.Unlock(); \
                                                } \
                                            } \

 MythFile::MythFile()
   :m_file_t(new MythPointer<cmyth_file_t>()),m_conn(MythConnection())
 {
 }

  MythFile::MythFile(cmyth_file_t myth_file,MythConnection conn)
    : m_file_t(new MythPointer<cmyth_file_t>()),m_conn(conn)
 {
   *m_file_t=myth_file;
 }
 
void MythFile::UpdateDuration (unsigned long long length )
 {
    int retval = 0;
    CMYTH_FILE_CALL( retval, retval < 0, UpdateFileLength( *m_file_t, length ) );
  }
  
  bool  MythFile::IsNull()
  {
    if(m_file_t==NULL)
      return true;
    return *m_file_t==NULL;
  }

  int MythFile::Read(void* buffer,long long length)
  {
    int bytesRead;
    CMYTH_FILE_CALL( bytesRead, bytesRead < 0, FileRead(*m_file_t, static_cast< char * >( buffer ), length ) );
    return bytesRead;
  }

  long long MythFile::Seek(long long offset, int whence)
  {
    long long retval = 0;
    CMYTH_FILE_CALL( retval, retval < 0, FileSeek( *m_file_t, offset, whence ) );
    return retval;
  }
  
  unsigned long long MythFile::CurrentPosition()
  {
    unsigned long long retval = 0;
    CMYTH_FILE_CALL( retval, retval < 0, FilePosition( *m_file_t ) );
    return retval;
  }
  
  unsigned long long MythFile::Duration()
  {
    unsigned long long retval = 0;
    CMYTH_FILE_CALL( retval, retval < 0, FileLength( *m_file_t ) );
    return retval;
  }