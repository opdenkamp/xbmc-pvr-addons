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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include "recordinggroup.h"

cRecordingGroup::cRecordingGroup(void)
{
  category = "";
  channeldisplayname = "";
  channelid = "";
  channeltype = ArgusTV::Television;
  isrecording = false;
  latestprogramstarttime = 0;
  programtitle = "";
  recordinggroupmode = ArgusTV::GroupByProgramTitle;
  recordingscount = 0;
  scheduleid = "";
  schedulename = "";
  schedulepriority = ArgusTV::Normal;
}

cRecordingGroup::~cRecordingGroup(void)
{
}

bool cRecordingGroup::Parse(const Json::Value& data)
{
    //Json::printValueTree(data);

  category = data["Category"].asString();
  channeldisplayname = data["ChannelDisplayName"].asString();
  channelid = data["ChannelId"].asString();
  channeltype = (ArgusTV::ChannelType) data["ChannelType"].asInt();
  isrecording = data["IsRecording"].asBool();
  int offset;
  std::string lpst = data["LatestProgramStartTime"].asString();
  latestprogramstarttime = ArgusTV::WCFDateToTimeT(lpst, offset);
  latestprogramstarttime += ((offset/100)*3600);
  programtitle = data["ProgramTitle"].asString();
  recordinggroupmode = (ArgusTV::RecordingGroupMode) data["RecordingGroupMode"].asInt();
  recordingscount = data["RecordingsCount"].asInt();
  scheduleid = data["ScheduleId"].asString();
  schedulename = data["ScheduleName"].asString();
  schedulepriority = (ArgusTV::SchedulePriority) data["SchedulePriority"].asInt();

  return true;
}
