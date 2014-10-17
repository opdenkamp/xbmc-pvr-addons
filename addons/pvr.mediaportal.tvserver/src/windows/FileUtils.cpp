/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../FileUtils.h"
#include "platform/os.h"
#include <string>
#include "platform/util/StdString.h"
#include "../utils.h"

namespace OS
{
  bool CFile::Exists(const std::string& strFileName, long* errCode)
  {
    CStdString strWinFile = ToWindowsPath(strFileName);
    CStdStringW strWFile = UTF8Util::ConvertUTF8ToUTF16(strWinFile.c_str());
    DWORD dwAttr = GetFileAttributesW(strWFile.c_str());

    if(dwAttr != 0xffffffff)
    {
      return true;
    }

    if (errCode)
      *errCode = GetLastError();

    return false;
  }
}
