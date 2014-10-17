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

#include "WindowsUtils.h"
#include <windows.h>
#include <stdio.h>
#include <string>

namespace OS
{
  typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

  WindowsVersion Version()
  {
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    BOOL bOsVersionInfoEx;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

    if(bOsVersionInfoEx == NULL ) return Unknown;

    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
    HMODULE kernel32_dll = GetModuleHandle(TEXT("kernel32.dll"));

    if (NULL != kernel32_dll)
    {
      PGNSI GetNativeSystemInfo;
      GetNativeSystemInfo = (PGNSI) GetProcAddress( kernel32_dll, "GetNativeSystemInfo");
      if (NULL != GetNativeSystemInfo)
        GetNativeSystemInfo(&si);
      else
        GetSystemInfo(&si);
    }
    else
      GetSystemInfo(&si);

    if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && osvi.dwMajorVersion > 4 )
    {
      // Test for the specific product.
      if ( osvi.dwMajorVersion == 6 )
      {
        if( osvi.dwMinorVersion == 0 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            return WindowsVista;
          else
            return WindowsServer2008;
        }

        if ( osvi.dwMinorVersion == 1 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            return Windows7;
          else
            return WindowsServer2008R2;
        }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
      {
        if( GetSystemMetrics(SM_SERVERR2) )
          return WindowsServer2003R2;
        else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
          return WindowsStorageServer2003;
        else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
          return WindowsHomeServer;
        else if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
          return WindowsXPx64;
        else
          return WindowsServer2003;
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
      {
        if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
          return WindowsXPHome;
        else
          return WindowsXPPro;
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
      {
        return Windows2000;
      }
    }

    return Unknown;
  }

  bool GetEnvironmentVariable(const char* strVarName, std::string& strResult)
  {
    char strBuffer[4096];
    DWORD dwRet;

    dwRet = ::GetEnvironmentVariable(strVarName, strBuffer, 4096);

    if(0 == dwRet)
    {
      dwRet = GetLastError();
      if( ERROR_ENVVAR_NOT_FOUND == dwRet )
      {
        strResult.clear();
        return false;
      }
    }
    strResult = strBuffer;
    return true;
  }
}
