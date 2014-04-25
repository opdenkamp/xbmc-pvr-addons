#pragma once
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

#include <string>

namespace OS
{
  typedef enum _WindowsVersion
  {
    Unknown = 0,
    Windows2000 = 10,
    WindowsXPHome = 20,
    WindowsXPPro = 21,
    WindowsXPx64 = 22,
    WindowsVista = 30,
    WindowsServer2003 = 31,
    WindowsStorageServer2003 = 32,
    WindowsHomeServer = 33,
    WindowsServer2003R2 = 34,
    Windows7 = 40,
    WindowsServer2008 = 41,
    WindowsServer2008R2 = 42
  } WindowsVersion;

  WindowsVersion Version();

  bool GetEnvironmentVariable(const char* strVarName, std::string& strResult);
}