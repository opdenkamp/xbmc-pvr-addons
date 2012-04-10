#ifndef __XBMC_ADDON_DLL_H__
#define __XBMC_ADDON_DLL_H__

/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#ifdef _WIN32
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#include "xbmc_addon_types.h"

#ifdef __cplusplus
extern "C" { 
#endif

  ADDON_STATUS __declspec(dllexport) ADDON_Create(void *callbacks, void* props);
  void         __declspec(dllexport) ADDON_Stop();
  void         __declspec(dllexport) ADDON_Destroy();
  ADDON_STATUS __declspec(dllexport) ADDON_GetStatus();
  bool         __declspec(dllexport) ADDON_HasSettings();
  unsigned int __declspec(dllexport) ADDON_GetSettings(ADDON_StructSetting ***sSet);
  ADDON_STATUS __declspec(dllexport) ADDON_SetSetting(const char *settingName, const void *settingValue);
  void         __declspec(dllexport) ADDON_FreeSettings();

#ifdef __cplusplus
};
#endif

#endif
