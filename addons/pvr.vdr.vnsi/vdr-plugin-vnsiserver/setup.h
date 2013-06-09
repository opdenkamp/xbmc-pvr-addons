/*
* Copyright (C) 2005-2012 Team XBMC
* http://www.xbmc.org
*
* This Program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This Program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with XBMC; see the file COPYING. If not, see
* <http://www.gnu.org/licenses/>.
*
*/

#ifndef VNSI_SETUP_H
#define VNSI_SETUP_H

#include <vdr/plugin.h>

class cMenuSetupVNSI : public cMenuSetupPage
{
private:
  int newPmtTimeout;
  int newTimeshiftMode;
  const char *timeshiftModesTexts[3];
  int newTimeshiftBufferSize;
  int newTimeshiftBufferFileSize;
  char newTimeshiftBufferDir[PATH_MAX];
protected:
  virtual void Store(void);
public:
  cMenuSetupVNSI(void);
};

#endif // VNSI_SETUP_H
