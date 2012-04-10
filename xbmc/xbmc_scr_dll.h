#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifndef __XBMC_SCR_H__
#define __XBMC_SCR_H__

#include "xbmc_addon_dll.h"
#include "xbmc_scr_types.h"

extern "C"
{

  // Functions that your visualisation must implement
  void Start();
  void Render();
  void GetInfo(SCR_INFO* pInfo);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct ScreenSaver* pScr)
  {
    pScr->Start = Start;
    pScr->Render = Render;
    pScr->GetInfo = GetInfo;
  };
};

#endif
