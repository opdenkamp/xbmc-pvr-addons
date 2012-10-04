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

#include "MythFile.h"
#include "MythPointer.h"

/* Call 'call', then if 'cond' condition is true see if we're still
 * connected to the control socket and try to re-connect if not.
 * If reconnection is ok, call 'call' again. */
#define CMYTH_FILE_CALL(var, cond, call) m_conn.Lock(); \
                                         var = call; \
                                         m_conn.Unlock(); \
                                         if (cond) \
                                         { \
                                           if (!m_conn.IsConnected() && m_conn.TryReconnect()) \
                                           { \
                                             m_conn.Lock(); \
                                             var = call; \
                                             m_conn.Unlock(); \
                                           } \
                                         } \

MythFile::MythFile()
  : m_file_t(new MythPointer<cmyth_file_t>())
  , m_conn(MythConnection())
{
}

MythFile::MythFile(cmyth_file_t myth_file, MythConnection conn)
  : m_file_t(new MythPointer<cmyth_file_t>())
  , m_conn(conn)
{
  *m_file_t = myth_file;
}

bool MythFile::IsNull() const
{
  if (m_file_t == NULL)
    return true;
  return *m_file_t == NULL;
}

unsigned long long MythFile::Length()
{
  unsigned long long retval = 0;
  CMYTH_FILE_CALL(retval, (long long)retval < 0, cmyth_file_length(*m_file_t));
  return retval;
}

void MythFile::UpdateLength(unsigned long long length)
{
  int retval = 0;
  CMYTH_FILE_CALL(retval, retval < 0, cmyth_update_file_length(*m_file_t, length));
}

int MythFile::Read(void *buffer, unsigned long length)
{
  int bytesRead;
  CMYTH_FILE_CALL(bytesRead, bytesRead < 0, cmyth_file_read(*m_file_t, static_cast< char * >(buffer), length));
  return bytesRead;
}

long long MythFile::Seek(long long offset, int whence)
{
  long long retval = 0;
  CMYTH_FILE_CALL(retval, retval < 0, cmyth_file_seek(*m_file_t, offset, whence));
  return retval;
}

unsigned long long MythFile::Position()
{
  unsigned long long retval = 0;
  CMYTH_FILE_CALL( retval, (long long)retval < 0, cmyth_file_position(*m_file_t));
  return retval;
}
