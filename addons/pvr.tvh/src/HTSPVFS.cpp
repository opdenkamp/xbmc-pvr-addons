/*
 *      Copyright (C) 2014 Adam Sutton
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

#include "platform/threads/mutex.h"
#include "platform/util/timeutils.h"
#include "platform/sockets/tcp.h"

extern "C" {
#include "platform/util/atomic.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

#include "Tvheadend.h"
#include "client.h"

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

CHTSPVFS::CHTSPVFS ( CHTSPConnection &conn )
  : m_conn(conn), m_path(""), m_fileId(0), m_offset(0)
{
  m_buffer.alloc(1000000);
}

CHTSPVFS::~CHTSPVFS ( void )
{
}

void CHTSPVFS::Connected ( void )
{
  /* Re-open */
  if (m_fileId != 0)
  { 
    tvhdebug("re-open vfs file");
    if (!SendFileOpen(true) || !SendFileSeek(m_offset, SEEK_SET, true))
    {
      tvherror("failed to re-open vfs file");
      Close();
    }
  }
}

bool CHTSPVFS::ProcessMessage ( const char *method, htsmsg_t *m )
{
  return false;
}

void CHTSPVFS::Flush ( void )
{
  m_buffer.reset();
  m_offset = 0;
}

/* **************************************************************************
 * VFS API
 * *************************************************************************/

bool CHTSPVFS::Open ( const PVR_RECORDING &rec )
{
  CLockObject lock(m_conn.Mutex());

  /* Close existing */
  Close();

  /* Cache details */
  m_path.Format("dvr/%s", rec.strRecordingId);

  /* Send open */
  if (!SendFileOpen())
  {
    tvherror("failed to open file");
    return false;
  }

  /* Done */
  return true;
}

void CHTSPVFS::Close ( void )
{
  CLockObject lock(m_conn.Mutex());

  if (m_fileId != 0)
    SendFileClose();

  Flush();
  m_fileId = 0;
  m_path   = "";
}

int CHTSPVFS::Read ( unsigned char *buf, unsigned int len )
{
  ssize_t ret;
  CLockObject lock(m_conn.Mutex());

  /* Not opened */
  if (!m_fileId)
    return -1;

  /* Fetch data */
  if (m_buffer.avail() <= len)
  {
    htsmsg_t   *m;
    const void *buf;
    size_t      len;
  
    /* Build */
    m = htsmsg_create_map();
    htsmsg_add_u32(m, "id",   m_fileId);
    htsmsg_add_s64(m, "size", m_buffer.free());
    
    /* Send */
    m = m_conn.SendAndWait("fileRead", m);
    if (m == NULL)
    {
      tvherror("failed to read vfs data");
      return -1;
    }

    /* Process */
    if (htsmsg_get_bin(m, "data", &buf, &len))
    {
      tvherror("malformed fileRead response");
      htsmsg_destroy(m);
      return -1;
    }

    /* Store */
    if (m_buffer.write((unsigned char*)buf, len) != (ssize_t)len)
    {
      tvherror("vfs partial buffer write");
      return -1;
    }
  }

  /* Read */
  ret       = m_buffer.read(buf, len);
  m_offset += ret;
  return (int)ret;
}

long long CHTSPVFS::Seek ( long long pos, int whence )
{
  CLockObject lock(m_conn.Mutex());
  if (m_fileId == 0)
    return -1;
  return SendFileSeek(pos, whence);
}

long long CHTSPVFS::Tell ( void )
{
  CLockObject lock(m_conn.Mutex());
  if (m_fileId == 0)
    return -1;
  return m_offset;
}

long long CHTSPVFS::Size ( void )
{
  int64_t ret = -1;
  CLockObject lock(m_conn.Mutex());
  htsmsg_t *m;
  
  /* Build */
  m = htsmsg_create_map();
  htsmsg_add_u32(m, "id", m_fileId);
  
  /* Send */
  if ((m = m_conn.SendAndWait("fileStat", m)) == NULL)
  {
    tvherror("failed to stat vfs file");
    return -1;
  }

  /* Process */
  if (htsmsg_get_s64(m, "size", &ret))
    tvherror("malformed fileStat response");
  htsmsg_destroy(m);

  return ret;
}

/* **************************************************************************
 * HTSP Messages
 * *************************************************************************/

bool CHTSPVFS::SendFileOpen ( bool force )
{
  htsmsg_t *m;

  /* Build Message */
  m = htsmsg_create_map();
  htsmsg_add_str(m, "file", m_path.c_str());

  /* Send */
  if (force)
    m = m_conn.SendAndWait0("fileOpen", m);
  else
    m = m_conn.SendAndWait("fileOpen", m);
  if (m == NULL)
  {
    tvherror("failed to send fileOpen");
    return false;
  }

  /* Get ID */
  htsmsg_get_u32(m, "id", &m_fileId);
  htsmsg_destroy(m);

  /* Log */
  return m_fileId > 0;
}

void CHTSPVFS::SendFileClose ( void )
{
  htsmsg_t *m;

  /* Build */
  m = htsmsg_create_map();
  htsmsg_add_u32(m, "id", m_fileId);
  
  /* Send */
  m = m_conn.SendAndWait("fileClose", m);
  if (m)
    htsmsg_destroy(m);
  // Note: ignore the return;
}

ssize_t CHTSPVFS::SendFileSeek ( int64_t pos, int whence, bool force )
{
  htsmsg_t *m;
  ssize_t ret = -1;

  /* Build Message */
  m = htsmsg_create_map();
  htsmsg_add_u32(m, "id",     m_fileId);
  htsmsg_add_s64(m, "offset", pos);
  if (whence == SEEK_CUR)
    htsmsg_add_str(m, "whence", "SEEK_CUR");
  else if (whence == SEEK_END)
    htsmsg_add_str(m, "whence", "SEEK_END");

  /* Send */
  if (force)
    m = m_conn.SendAndWait0("fileSeek", m);
  else
    m = m_conn.SendAndWait("fileSeek", m);
  if (m == NULL)
  {
    tvherror("failed to send fileSeek");
    return false;
  }

  /* Get new offset */
  if (htsmsg_get_s64(m, "offset", &ret))
    tvherror("malformed fileSeek response");
  htsmsg_destroy(m);
  
  /* Update */
  if (ret > 0)
  {
    m_offset = ret;
    m_buffer.reset();
  }

  return ret;
}
