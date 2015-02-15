/*
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *		Copyright (C) 2014 Stephen Denham
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

#include "PVRFilmonData.h"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace ADDON;


PVRFilmonData::PVRFilmonData(void) {
	onLoad = true;
}

PVRFilmonData::~PVRFilmonData(void) {
	m_channels.clear();
	m_groups.clear();
	m_recordings.clear();
	m_timers.clear();
	filmonAPIDelete();
}

bool PVRFilmonData::Load(std::string user, std::string pwd) {
	PLATFORM::CLockObject lock(m_mutex);
	username = user;
	password = pwd;
	bool res = filmonAPICreate();
	if (res) {
		res = filmonAPIlogin(username, password);
		if (res) {
			XBMC->QueueNotification(QUEUE_INFO, "Filmon user logged in");
			lastTimeChannels = 0;
			lastTimeGroups = 0;
		} else {
			XBMC->QueueNotification(QUEUE_ERROR, "Filmon user failed to login");
		}
	}
	onLoad = true;
	return res;
}

const char* PVRFilmonData::GetBackendName(void) {
	return "Filmon API";
}

const char* PVRFilmonData::GetBackendVersion(void) {
	return "2.2";
}

const char* PVRFilmonData:: GetConnection(void) {
	return filmonAPIConnection().c_str();
}

void PVRFilmonData::GetDriveSpace(long long *iTotal, long long *iUsed) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "getting user storage from API");
	filmonAPIgetUserStorage(iTotal, iUsed);
	*iTotal = *iTotal/10;
	*iUsed = *iUsed/10;
	return;
}

int PVRFilmonData::GetChannelsAmount(void) {
	unsigned int chCount = filmonAPIgetChannelCount();
	XBMC->Log(LOG_DEBUG, "channel count is %d ", chCount);
	return chCount;
}

PVR_ERROR PVRFilmonData::GetChannels(ADDON_HANDLE handle, bool bRadio) {
	PLATFORM::CLockObject lock(m_mutex);
	bool res = false;
	bool expired = false;
	if (time(0) - lastTimeChannels > FILMON_CACHE_TIME) {
		XBMC->Log(LOG_DEBUG, "cache expired, getting channels from API");
		m_channels.clear();
		expired = true;
	}

	std::vector<unsigned int> channelList = filmonAPIgetChannels();
	unsigned int channelCount = channelList.size();
	unsigned int channelId = 0;

	for (unsigned int i = 0; i < channelCount; i++) {
		FILMON_CHANNEL channel;
		channelId = channelList[i];
		if (expired) {
			res = filmonAPIgetChannel(channelId, &channel);
			if (onLoad == true) {
				XBMC->QueueNotification(QUEUE_INFO, "Filmon loaded %s",
						channel.strChannelName.c_str());
			}

		} else {
			for (unsigned int j = 0; j < m_channels.size(); j++) {
				if (m_channels[j].iUniqueId == channelId) {
					channel = m_channels[j];
					res = true;
					break;
				}
			}
		}
		if (res && channel.bRadio == bRadio) {
			PVR_CHANNEL xbmcChannel;
			memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

			xbmcChannel.iUniqueId = channel.iUniqueId;
			xbmcChannel.bIsRadio = false;
			xbmcChannel.iChannelNumber = channel.iChannelNumber;
			strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(),
					sizeof(xbmcChannel.strChannelName) - 1);
			strncpy(xbmcChannel.strStreamURL, channel.strStreamURL.c_str(),
					sizeof(xbmcChannel.strStreamURL) - 1);
			xbmcChannel.iEncryptionSystem = channel.iEncryptionSystem;
			strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(),
					sizeof(xbmcChannel.strIconPath) - 1);
			xbmcChannel.bIsHidden = false;
			if (expired) {
				m_channels.push_back(channel);
			}
			PVR->TransferChannelEntry(handle, &xbmcChannel);
		}
	}
	if (lastTimeChannels == 0) {
		XBMC->QueueNotification(QUEUE_INFO, "Filmon loaded %d channels",
				m_channels.size());
	}
	if (expired) {
		lastTimeChannels = time(0);
	}
	onLoad = false;
	return PVR_ERROR_NO_ERROR;
}

int PVRFilmonData::GetChannelGroupsAmount(void) {
	XBMC->Log(LOG_DEBUG, "getting number of groups");
	return m_groups.size();
}

PVR_ERROR PVRFilmonData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio) {
	PLATFORM::CLockObject lock(m_mutex);
	if (bRadio == false) {
		if (time(0) - lastTimeGroups > FILMON_CACHE_TIME) {
			XBMC->Log(LOG_DEBUG,
					"cache expired, getting channel groups from API");
			m_groups = filmonAPIgetChannelGroups();
			lastTimeGroups = time(0);
		}
		for (unsigned int grpId = 0; grpId < m_groups.size(); grpId++) {
			PVRFilmonChannelGroup group = m_groups[grpId];
			PVR_CHANNEL_GROUP xbmcGroup;
			memset(&xbmcGroup, 0, sizeof(PVR_CHANNEL_GROUP));
			xbmcGroup.bIsRadio = bRadio;
			strncpy(xbmcGroup.strGroupName, group.strGroupName.c_str(),
					sizeof(xbmcGroup.strGroupName) - 1);
			PVR->TransferChannelGroup(handle, &xbmcGroup);
			XBMC->Log(LOG_DEBUG, "found group %s", xbmcGroup.strGroupName);
		}
	}
	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRFilmonData::GetChannelGroupMembers(ADDON_HANDLE handle,
		const PVR_CHANNEL_GROUP &group) {
	PLATFORM::CLockObject lock(m_mutex);
	if (time(0) - lastTimeGroups > FILMON_CACHE_TIME) {
		XBMC->Log(LOG_DEBUG,
				"cache expired, getting channel groups members from API");
		m_groups = filmonAPIgetChannelGroups();
		lastTimeGroups = time(0);
	}
	for (unsigned int grpId = 0; grpId < m_groups.size(); grpId++) {
		PVRFilmonChannelGroup grp = m_groups.at(grpId);
		if (strcmp(grp.strGroupName.c_str(), group.strGroupName) == 0) {
			for (unsigned int chId = 0; chId < grp.members.size(); chId++) {
				PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
				memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
				strncpy(xbmcGroupMember.strGroupName, group.strGroupName,
						sizeof(xbmcGroupMember.strGroupName) - 1);
				xbmcGroupMember.iChannelUniqueId = grp.members[chId];
				xbmcGroupMember.iChannelNumber = grp.members[chId];
				XBMC->Log(LOG_DEBUG, "add member %d", grp.members[chId]);
				PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
			}
			break;
		}
	}
	return PVR_ERROR_NO_ERROR;
}

// Returns index into channel vector, refreshes channel entry
int PVRFilmonData::UpdateChannel(unsigned int channelId) {
	int index = -1;
	XBMC->Log(LOG_DEBUG, "updating channel %d ", channelId);
	for (unsigned int i = 0; i < m_channels.size(); i++) {
		if (m_channels[i].iUniqueId == channelId) {
			if (time(0) - lastTimeChannels > FILMON_CACHE_TIME) {
				XBMC->Log(LOG_DEBUG, "cache expired, getting channel from API");
				filmonAPIgetChannel(channelId, &m_channels[i]);
			}
			index = i;
			break;
		}
	}
	return index;
}

// Called periodically to refresh EPG
PVR_ERROR PVRFilmonData::GetEPGForChannel(ADDON_HANDLE handle,
		const PVR_CHANNEL &channel, time_t iStart, time_t iEnd) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "getting EPG for channel");
	unsigned int broadcastIdCount = lastTimeChannels;
	int chIndex = PVRFilmonData::UpdateChannel(channel.iUniqueId);
	if (chIndex >= 0) {
		PVRFilmonChannel ch = m_channels[chIndex];
		for (unsigned int epgId = 0; epgId < ch.epg.size(); epgId++) {
			PVRFilmonEpgEntry &epgEntry = ch.epg.at(epgId);
			if (epgEntry.startTime >= iStart && epgEntry.endTime <= iEnd) {
				EPG_TAG tag;
				memset(&tag, 0, sizeof(EPG_TAG));
				tag.iUniqueBroadcastId = broadcastIdCount++;
				tag.strTitle = epgEntry.strTitle.c_str();
				tag.iChannelNumber = epgEntry.iChannelId;
				tag.startTime = epgEntry.startTime;
				tag.endTime = epgEntry.endTime;
				tag.strPlotOutline = epgEntry.strPlotOutline.c_str();
				tag.strPlot = epgEntry.strPlot.c_str();
				tag.strIconPath = epgEntry.strIconPath.c_str();
				tag.iGenreType = epgEntry.iGenreType;
				tag.iGenreSubType = epgEntry.iGenreSubType;
				tag.strGenreDescription = "";
				tag.firstAired = 0;
				tag.iParentalRating = 0;
				tag.iStarRating = 0;
				tag.bNotify = false;
				tag.iSeriesNumber = 0;
				tag.iEpisodeNumber = 0;
				tag.iEpisodePartNumber = 0;
				tag.strEpisodeName = "";
				PVR->TransferEpgEntry(handle, &tag);
			}
		}
		if (time(0) - lastTimeChannels > FILMON_CACHE_TIME) {
			// Get PVR to re-read refreshed channels
			if (filmonAPIlogin(username, password)) {
				PVR->TriggerChannelGroupsUpdate();
				PVR->TriggerChannelUpdate();
			}
		}
		return PVR_ERROR_NO_ERROR;
	} else {
		return PVR_ERROR_SERVER_ERROR;
	}
}

int PVRFilmonData::GetRecordingsAmount(void) {
	XBMC->Log(LOG_DEBUG, "getting number of recordings");
	return m_recordings.size();
}

PVR_ERROR PVRFilmonData::GetRecordings(ADDON_HANDLE handle) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "getting recordings from API");
	m_recordings = filmonAPIgetRecordings();
	for (std::vector<PVRFilmonRecording>::iterator it = m_recordings.begin();
			it != m_recordings.end(); it++) {
		PVRFilmonRecording &recording = *it;
		PVR_RECORDING xbmcRecording;

		xbmcRecording.iDuration = recording.iDuration;
		xbmcRecording.iGenreType = recording.iGenreType;
		xbmcRecording.iGenreSubType = recording.iGenreSubType;
		xbmcRecording.recordingTime = recording.recordingTime;

		strncpy(xbmcRecording.strChannelName, recording.strChannelName.c_str(),
				sizeof(xbmcRecording.strChannelName) - 1);
		strncpy(xbmcRecording.strPlotOutline, recording.strPlotOutline.c_str(),
				sizeof(xbmcRecording.strPlotOutline) - 1);
		strncpy(xbmcRecording.strPlot, recording.strPlot.c_str(),
				sizeof(xbmcRecording.strPlot) - 1);
		strncpy(xbmcRecording.strRecordingId, recording.strRecordingId.c_str(),
				sizeof(xbmcRecording.strRecordingId) - 1);
		strncpy(xbmcRecording.strTitle, recording.strTitle.c_str(),
				sizeof(xbmcRecording.strTitle) - 1);
		strncpy(xbmcRecording.strDirectory, "Filmon",
				sizeof(xbmcRecording.strChannelName) - 1);
		strncpy(xbmcRecording.strStreamURL, recording.strStreamURL.c_str(),
				sizeof(xbmcRecording.strStreamURL) - 1);
		strncpy(xbmcRecording.strIconPath, recording.strIconPath.c_str(),
				sizeof(xbmcRecording.strIconPath) - 1);
		strncpy(xbmcRecording.strThumbnailPath,
				recording.strThumbnailPath.c_str(),
				sizeof(xbmcRecording.strThumbnailPath) - 1);

		PVR->TransferRecordingEntry(handle, &xbmcRecording);
	}
	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRFilmonData::DeleteRecording(const PVR_RECORDING &recording) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "deleting recording %s", recording.strRecordingId);
	if (filmonAPIdeleteRecording((unsigned int)atoi(recording.strRecordingId))) {
		PVR->TriggerRecordingUpdate();
		return PVR_ERROR_NO_ERROR;
	} else {
		return PVR_ERROR_SERVER_ERROR;
	}
}

int PVRFilmonData::GetTimersAmount(void) {
	XBMC->Log(LOG_DEBUG, "getting number of timers");
	return m_timers.size();
}

// Gets called every 5 minutes, same as Filmon session lifetime
PVR_ERROR PVRFilmonData::GetTimers(ADDON_HANDLE handle) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "getting timers from API");
	if (filmonAPIkeepAlive()) { // Keeps session alive
		m_timers = filmonAPIgetTimers();
		for (std::vector<PVRFilmonTimer>::iterator it = m_timers.begin();
				it != m_timers.end(); it++) {
			PVRFilmonTimer &timer = *it;
			if ((PVR_TIMER_STATE) timer.state < PVR_TIMER_STATE_COMPLETED) {
				PVR_TIMER xbmcTimer;

				xbmcTimer.iClientIndex = timer.iClientIndex;
				xbmcTimer.iClientChannelUid = timer.iClientChannelUid;
				strncpy(xbmcTimer.strTitle, timer.strTitle.c_str(),
						sizeof(xbmcTimer.strTitle) - 1);
				strncpy(xbmcTimer.strSummary, timer.strSummary.c_str(),
						sizeof(xbmcTimer.strSummary) - 1);
				xbmcTimer.startTime = timer.startTime;
				xbmcTimer.endTime = timer.endTime;
				xbmcTimer.state = (PVR_TIMER_STATE) timer.state;
				xbmcTimer.iTimerType = timer.bIsRepeating ? PVR_TIMERTYPE_MANUAL_SERIE : PVR_TIMERTYPE_MANUAL_ONCE;
				xbmcTimer.firstDay = timer.firstDay;
				xbmcTimer.iWeekdays = timer.iWeekdays;
				xbmcTimer.iEpgUid = timer.iEpgUid;
				xbmcTimer.iGenreType = timer.iGenreType;
				xbmcTimer.iGenreSubType = timer.iGenreSubType;
				xbmcTimer.iMarginStart = timer.iMarginStart;
				xbmcTimer.iMarginEnd = timer.iMarginEnd;

				PVR->TransferTimerEntry(handle, &xbmcTimer);
			}
		}
		PVR->TriggerRecordingUpdate();
		return PVR_ERROR_NO_ERROR;
	} else {
		return PVR_ERROR_SERVER_ERROR;
	}
}

PVR_ERROR PVRFilmonData::AddTimer(const PVR_TIMER &timer) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "adding timer");
	if (filmonAPIaddTimer(timer.iClientChannelUid, timer.startTime,
			timer.endTime)) {
		PVR->TriggerTimerUpdate();
		return PVR_ERROR_NO_ERROR;
	} else {
		return PVR_ERROR_SERVER_ERROR;
	}
}

PVR_ERROR PVRFilmonData::DeleteTimer(const PVR_TIMER &timer,
		bool bForceDelete) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "deleting timer %d", timer.iClientIndex);
	if (filmonAPIdeleteTimer(timer.iClientIndex, bForceDelete)) {
		PVR->TriggerTimerUpdate();
		return PVR_ERROR_NO_ERROR;
	} else {
		return PVR_ERROR_SERVER_ERROR;
	}
}

PVR_ERROR PVRFilmonData::UpdateTimer(const PVR_TIMER& timer) {
	PLATFORM::CLockObject lock(m_mutex);
	XBMC->Log(LOG_DEBUG, "updating timer");
	if (filmonAPIdeleteTimer(timer.iClientIndex, true)
			&& filmonAPIaddTimer(timer.iClientChannelUid, timer.startTime,
					timer.endTime)) {
		PVR->TriggerTimerUpdate();
		return PVR_ERROR_NO_ERROR;
	} else {
		return PVR_ERROR_SERVER_ERROR;
	}
}
