/*
 *      Copyright (C) 2011 Fred Hoogduin
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

#include <vector>
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include "argustvrpc.h"
#include "upcomingrecording.h"

cUpcomingRecording::cUpcomingRecording(void)
{
  channeldisplayname = "";
  channelid = "";
  date = 0;
  starttime = 0;
  stoptime = 0;
  title = "";
  iscancelled = false;
  isallocated = true;
  isinconflict = true;
  id = 0;
  ichannelid = 0;
}

cUpcomingRecording::~cUpcomingRecording(void)
{
}
bool cUpcomingRecording::Parse(const Json::Value& data)
{
  int offset;
  std::string t;
  Json::Value channelobject,programobject;

  programobject = data["Program"];
  date = 0;

  id = programobject["Id"].asInt(); 
  t = programobject["StartTime"].asString();
  starttime = ArgusTV::WCFDateToTimeT(t, offset);
  t = programobject["StopTime"].asString();
  stoptime = ArgusTV::WCFDateToTimeT(t, offset);
  prerecordseconds = programobject["PreRecordSeconds"].asInt();
  postrecordseconds = programobject["PostRecordSeconds"].asInt();
  title = programobject["Title"].asString();
  iscancelled = programobject["IsCancelled"].asBool();
  upcomingprogramid = programobject["UpcomingProgramId"].asString();
  guideprogramid = programobject["GuideProgramId"].asString();
  scheduleid = programobject["ScheduleId"].asString();

  // From the Program class pickup the C# Channel class
  channelobject = programobject["Channel"];
  channelid = channelobject["ChannelId"].asString();
  channeldisplayname = channelobject["DisplayName"].asString();
  ichannelid = channelobject["Id"].asInt(); 

  if (data["CardChannelAllocation"].empty())
    isallocated = false; 

  if (data["ConflictingPrograms"].empty())
    isinconflict = false;

  return true;
}