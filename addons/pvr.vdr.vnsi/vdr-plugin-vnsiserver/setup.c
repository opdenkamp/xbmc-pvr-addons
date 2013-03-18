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

#include "setup.h"
#include "vnsicommand.h"

int PmtTimeout = 5;
int TimeshiftMode = 0;
int TimeshiftBufferSize = 5;
int TimeshiftBufferFileSize = 6;
char TimeshiftBufferDir[PATH_MAX] = "\0";

cMenuSetupVNSI::cMenuSetupVNSI(void)
{
  newPmtTimeout = PmtTimeout;
  Add(new cMenuEditIntItem( tr("PMT Timeout (0-10)"), &newPmtTimeout));

  timeshiftModesTexts[0] = tr("Off");
  timeshiftModesTexts[1] = tr("RAM");
  timeshiftModesTexts[2] = tr("File");
  newTimeshiftMode = TimeshiftMode;
  Add(new cMenuEditStraItem( tr("Time Shift Mode"), &newTimeshiftMode, 3, timeshiftModesTexts));

  newTimeshiftBufferSize = TimeshiftBufferSize;
  Add(new cMenuEditIntItem( tr("TS Buffersize (RAM) (1-20) x 100MB"), &newTimeshiftBufferSize));

  newTimeshiftBufferFileSize = TimeshiftBufferFileSize;
  Add(new cMenuEditIntItem( tr("TS Buffersize (File) (1-10) x 1GB"), &newTimeshiftBufferFileSize));

  strn0cpy(newTimeshiftBufferDir, TimeshiftBufferDir, sizeof(newTimeshiftBufferDir));
  Add(new cMenuEditStrItem(tr("TS Buffer Directory"), newTimeshiftBufferDir, sizeof(newTimeshiftBufferDir)));
}

void cMenuSetupVNSI::Store(void)
{
  if (newPmtTimeout > 10 || newPmtTimeout < 0)
    newPmtTimeout = 2;
  SetupStore(CONFNAME_PMTTIMEOUT, PmtTimeout = newPmtTimeout);

  SetupStore(CONFNAME_TIMESHIFT, TimeshiftMode = newTimeshiftMode);

  if (newTimeshiftBufferSize > 40)
    newTimeshiftBufferSize = 40;
  else if (newTimeshiftBufferSize < 1)
    newTimeshiftBufferSize = 1;
  SetupStore(CONFNAME_TIMESHIFTBUFFERSIZE, TimeshiftBufferSize = newTimeshiftBufferSize);

  if (newTimeshiftBufferFileSize > 20)
    newTimeshiftBufferFileSize = 20;
  else if (newTimeshiftBufferFileSize < 1)
    newTimeshiftBufferFileSize = 1;
  SetupStore(CONFNAME_TIMESHIFTBUFFERFILESIZE, TimeshiftBufferFileSize = newTimeshiftBufferFileSize);

  SetupStore(CONFNAME_TIMESHIFTBUFFERDIR, strn0cpy(TimeshiftBufferDir, newTimeshiftBufferDir, sizeof(TimeshiftBufferDir)));
}
