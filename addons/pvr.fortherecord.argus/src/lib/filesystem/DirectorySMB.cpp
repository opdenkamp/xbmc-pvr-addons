
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

/*
* know bugs:
* - when opening a server for the first time with ip adres and the second time
*   with server name, access to the server is denied.
* - when browsing entire network, user can't go back one step
*   share = smb://, user selects a workgroup, user selects a server.
*   doing ".." will go back to smb:// (entire network) and not to workgroup list.
*
* debugging is set to a max of 10 for release builds (see local.h)
*/

#include "platform/os.h"
#include "client.h"
#include "DirectorySMB.h"
#include "platform/threads/mutex.h"
#include <libsmbclient.h>

#if defined(TARGET_DARWIN)
#define XBMC_SMB_MOUNT_PATH "Library/Application Support/XBMC/Mounts/"
#else
#define XBMC_SMB_MOUNT_PATH "/media/xbmc/smb/"
#endif

using namespace std;
using namespace ADDON;

namespace PLATFORM
{

CSMBDirectory::CSMBDirectory(void)
{
#ifdef _LINUX
  smb.AddActiveConnection();
#endif
}

CSMBDirectory::~CSMBDirectory(void)
{
#ifdef _LINUX
  smb.AddIdleConnection();
#endif
}

int CSMBDirectory::Open(const CStdString &url)
{
  smb.Init();
  CStdString strAuth;
  return OpenDir(url);
}

/// \brief Checks authentication against SAMBA share
/// \param url The SMB style path
/// \return SMB file descriptor
int CSMBDirectory::OpenDir(const CStdString& url)
{
  int fd = -1;

  // remove the / or \ at the end. the samba library does not strip them off
  // don't do this for smb:// !!
  CStdString s = url;
  int len = s.length();
  if (len > 1 && s.at(len - 2) != '/' &&
      (s.at(len - 1) == '/' || s.at(len - 1) == '\\'))
  {
    s.erase(len - 1, 1);
  }

  XBMC->Log(LOG_DEBUG, "%s - Using authentication url %s", __FUNCTION__, s.c_str());
  { PLATFORM::CLockObject lock(smb);
    fd = smbc_opendir(s.c_str());
  }

  if (fd < 0)
  {
      XBMC->Log(LOG_ERROR, "%s: Directory open on %s failed (%d, \"%s\")\n", __FUNCTION__, url.c_str(), errno, strerror(errno));
  }

  if (fd < 0)
  {
    // write error to logfile
    XBMC->Log(LOG_ERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' error : '%s'", url.c_str(), errno, strerror(errno));
  }

  return fd;
}

}
