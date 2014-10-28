#pragma once
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

#include <vector>
#include "platform/util/StdString.h"
#include "platform/threads/mutex.h"
#include "client.h"
#include "libXBMC_pvr.h"
#include "FilmonAPI.h"

#define FILMON_CACHE_TIME 10800 // 3 hours

typedef FILMON_EPG_ENTRY PVRFilmonEpgEntry;
typedef FILMON_CHANNEL PVRFilmonChannel;
typedef FILMON_RECORDING PVRFilmonRecording;
typedef FILMON_TIMER PVRFilmonTimer;
typedef FILMON_CHANNEL_GROUP PVRFilmonChannelGroup;

class PVRFilmonData {
public:
	PVRFilmonData(void);
	virtual ~PVRFilmonData(void);

	virtual bool Load(std::string user, std::string pwd);

	virtual const char* GetBackendName(void);
	virtual const char *GetBackendVersion(void);
	virtual const char* GetConnection(void);
	virtual void GetDriveSpace(long long *iTotal, long long *iUsed);

	virtual int GetChannelsAmount(void);
	virtual PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);

	virtual int GetChannelGroupsAmount(void);
	virtual PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
	virtual PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle,
			const PVR_CHANNEL_GROUP &group);

	virtual PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle,
			const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

	virtual int GetRecordingsAmount(void);
	virtual PVR_ERROR GetRecordings(ADDON_HANDLE handle);
	virtual PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);

	virtual int GetTimersAmount(void);
	virtual PVR_ERROR GetTimers(ADDON_HANDLE handle);
	virtual PVR_ERROR AddTimer(const PVR_TIMER &timer);
	virtual PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
	virtual PVR_ERROR UpdateTimer(const PVR_TIMER &timer);

private:
	int UpdateChannel(unsigned int channelId);
	PLATFORM::CMutex m_mutex;
	std::vector<PVRFilmonChannelGroup> m_groups;
	std::vector<PVRFilmonChannel> m_channels;
	std::vector<PVRFilmonRecording> m_recordings;
	std::vector<PVRFilmonTimer> m_timers;
	time_t lastTimeGroups;
	time_t lastTimeChannels;
	std::string username;
	std::string password;
	bool onLoad;
};
