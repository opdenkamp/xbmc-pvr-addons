/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 
 *      Copyright (C) 2012 Palle Ehmsen(Barcode Madness)
 *      http://www.barcodemadness.com
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "libdvblinkremote/dvblinkremote.h"
#include "libdvblinkremote/dvblinkremotehttp.h"
#include "libXBMC_addon.h"

class HttpPostClient : public dvblinkremotehttp::HttpClient
{
public :
  bool SendRequest(dvblinkremotehttp::HttpWebRequest& request);
  dvblinkremotehttp::HttpWebResponse* GetResponse();
  void GetLastError(std::string& err);
  void UrlEncode(const std::string& str, std::string& outEncodedStr);
  HttpPostClient(ADDON::CHelper_libXBMC_addon *XBMC, const std::string& server, const int serverport, const std::string& username, const std::string& password);

private :
  int SendPostRequest(dvblinkremotehttp::HttpWebRequest& request);
  std::string m_server;
  long m_serverport;
  std::string m_username;
  std::string m_password;
  ADDON::CHelper_libXBMC_addon  *XBMC;
  std::string m_responseData;
  int m_lastReqeuestErrorCode;

};
