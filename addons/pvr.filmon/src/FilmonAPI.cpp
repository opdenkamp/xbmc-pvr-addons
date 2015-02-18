/*
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

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <algorithm>
#include <string>
#include <cstdio>
#include <sstream>

#include <json/json.h>

#include <unistd.h>

#include "FilmonAPI.h"
#include "client.h"
#include "md5.h"

#define FILMON_URL "http://www.filmon.com/"
#define FILMON_ONE_HOUR_RECORDING_SIZE 508831234
#define REQUEST_RETRIES 4
#define REQUEST_RETRY_TIMEOUT 500000 // 0.5s
#define RESPONSE_OUTPUT_LENGTH 128

#define RECORDED_STATUS "Recorded"
#define TIMER_STATUS "Accepted"
#define OFF_AIR "Off Air"

/* open without caching. regardless to file type. */
#define READ_NO_CACHE 0x08

using namespace ADDON;

// Attempt at genres
typedef struct {
    int genreType;
    const char* group;
} genreEntry;

static genreEntry genreTable[] = {
    {EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS, "NEWS TV"},
    {EPG_EVENT_CONTENTMASK_SPORTS, "SPORTS"},
    {EPG_EVENT_CONTENTMASK_SHOW, "COMEDY"},
    {EPG_EVENT_CONTENTMASK_MOVIEDRAMA, "FEATURE MOVIE"},
    {EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE, "MUSIC"},
    {EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE, "DOCUMENTARY"},
    {EPG_EVENT_CONTENTMASK_LEISUREHOBBIES, "LIFESTYLE"},
    {EPG_EVENT_CONTENTMASK_MOVIEDRAMA, "SHORT FILMS"},
    {EPG_EVENT_CONTENTMASK_CHILDRENYOUTH, "KIDS"},
    {EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE, "SCIENCE & TECHNOLOGY VOD"},
    {EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE, "EDUCATION"},
    {EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS, "BUSINESS TV"},
    {EPG_EVENT_CONTENTMASK_LEISUREHOBBIES, "CARS & AUTO"},
    {EPG_EVENT_CONTENTMASK_LEISUREHOBBIES, "FOOD AND WINE"},
    {EPG_EVENT_CONTENTMASK_SHOW, "CLASSIC TV"},
    {EPG_EVENT_CONTENTMASK_SHOW, "HORROR"}
};

#define GENRE_TABLE_LEN sizeof(genreTable)/sizeof(genreTable[0])

bool filmonRequest(std::string path, std::string params = "",
	unsigned int retries = REQUEST_RETRIES);
bool filmonAPIgetRecordingsTimers(bool completed = false);

std::string filmonUsername = "";
std::string filmonpassword = "";
std::string sessionKeyParam = "";
std::string swfPlayer = "";

long long storageUsed = 0;
long long storageTotal = 0;

std::vector<unsigned int> channelList;
std::vector<FILMON_CHANNEL_GROUP> groups;
std::vector<FILMON_RECORDING> recordings;
std::vector<FILMON_TIMER> timers;

bool connected = false;

std::string response;

std::string intToString(unsigned int i) {
	std::ostringstream oss;
	oss << i;
	return oss.str();
}

unsigned int stringToInt(std::string s) {
	return atoi(s.c_str());
}

std::string timeToHourMin(unsigned int t) {
	time_t tt = (time_t) t;
	tm *gmtm = gmtime(&tt);
	return intToString(gmtm->tm_hour) + intToString(gmtm->tm_min);
}

// Timer settings not supported in Filmon
void setTimerDefaults(FILMON_TIMER *t) {
	t->bIsRepeating = false;
	t->firstDay = 0;
	t->iWeekdays = 0;
	t->iEpgUid = 0;
	t->iGenreType = 0;
	t->iGenreSubType = 0;
	t->iMarginStart = 0;
	t->iMarginEnd = 0;
}


// Free response
void clearResponse() {
	response.clear();
}

// Initialize connection
bool filmonAPICreate(void) {
	connected = true;
	return connected;
}

// Remove connection
void filmonAPIDelete(void) {
	connected = false;
}

// Connection URL
std::string filmonAPIConnection() {
	if (connected) {
		return std::string(FILMON_URL);
	} else {
		return std::string(OFF_AIR);
	}
}

// Make a request
bool filmonRequest(std::string path, std::string params,
		unsigned int retries) {
	std::string request = FILMON_URL;

	// Add params
	request.append(path);
	if (params.length() > 0) {
		request.append("?");
		request.append(params);
	}

	// Allow request retries
	do {
		XBMC->Log(LOG_DEBUG, "request is %s", request.c_str());
		void *fileHandle = XBMC->OpenFile(request.c_str(), READ_NO_CACHE);
		if (!fileHandle) {
			XBMC->Log(LOG_ERROR,  "request failure");
			clearResponse();
			usleep(REQUEST_RETRY_TIMEOUT);
		} else {
			char buffer[4096];
			while (int read = XBMC->ReadFile(fileHandle, buffer, 4096))
				response.append(buffer, read);
			XBMC->CloseFile(fileHandle);

			XBMC->Log(LOG_DEBUG, "response is %s",
				response.substr(0, RESPONSE_OUTPUT_LENGTH).c_str());
		}
	} while (response.empty() && --retries > 0);

	if (!response.empty()) {
		return true;
	} else {
		filmonAPIDelete();
		filmonAPICreate();
		return false;
	}
}

// Logout user
void filmonAPIlogout(void) {
	bool res = filmonRequest("tv/api/logout");
	if (res == true) {
		clearResponse();
	}
}

// Keepalive
bool filmonAPIkeepAlive(void) {
	bool res = filmonRequest("tv/api/keep-alive", sessionKeyParam);
	if (!res) {
		// Login again if it failed
		filmonAPIlogout();
		filmonAPIlogin(filmonUsername, filmonpassword);
	} else {
		clearResponse();
	}
	return res;
}

// Session
bool filmonAPIgetSessionKey(void) {
	bool res = filmonRequest("tv/api/init?channelProvider=ipad&app_id=IGlsbSBuVCJ7UDwZBl0eBR4JGgEBERhRXlBcWl0CEw==");
	if (res == true) {
		Json::Value root;
		Json::Reader reader;
		reader.parse(response, root);
		Json::Value sessionKey = root["session_key"];
		sessionKeyParam = "session_key=";
		sessionKeyParam.append(sessionKey.asString());
		XBMC->Log(LOG_DEBUG, "got session key %s",
			sessionKey.asString().c_str());
		clearResponse();
	}
	return res;
}

// Login subscriber
bool filmonAPIlogin(std::string username, std::string password) {

	bool res = filmonAPIgetSessionKey();
	if (res) {
		XBMC->Log(LOG_DEBUG, "logging in user");
		filmonUsername = username;
		filmonpassword = password;

		std::string md5pwd = PVRXBMC::XBMC_MD5::GetMD5(password);
		std::transform(md5pwd.begin(), md5pwd.end(), md5pwd.begin(), ::tolower);

		std::string params = "login=" + username + "&password=" + md5pwd;
		res = filmonRequest("tv/api/login", sessionKeyParam + "&" + params, 1);
		if (res) {
			Json::Value root;
			Json::Reader reader;
			reader.parse(response, root);
			// Favorite channels
			channelList.clear();
			Json::Value favouriteChannels = root["favorite-channels"];
			unsigned int channelCount = favouriteChannels.size();
			for (unsigned int channel = 0; channel < channelCount; channel++) {
				Json::Value chId = favouriteChannels[channel]["channel"]["id"];
				channelList.push_back(chId.asUInt());
				XBMC->Log(LOG_INFO, "added channel %u",
					chId.asUInt());
			}
			clearResponse();
		}
	}
	return res;
}

// SWF player URL
void filmonAPIgetswfPlayer() {
	swfPlayer = std::string(
			"/tv/modules/FilmOnTV/files/flashapp/filmon/FilmonPlayer.swf?v=56");
	bool res = filmonRequest("tv/", "");
	if (res == true) {
		char *resp = (char *) malloc(response.length());
		strcpy(resp, response.c_str());
		char *token = strtok(resp, " ");
		while (token != NULL) {
			if (strcmp(token, "flash_config") == 0) {
				token = strtok(NULL, " ");
				token = strtok(NULL, " ");
				break;
			}
			token = strtok(NULL, " ");
		}
		Json::Value root;
		Json::Reader reader;
		if (reader.parse(std::string(token), root)) {
			Json::Value streamer = root["streamer"];
			swfPlayer = streamer.asString();
			XBMC->Log(LOG_DEBUG, "parsed flash config %s",
					swfPlayer.c_str());
		}
		clearResponse();
	}
	swfPlayer = std::string("http://www.filmon.com") + swfPlayer;
	XBMC->Log(LOG_INFO, "swfPlayer is %s", swfPlayer.c_str());
}

int filmonAPIgetGenre(std::string group) {
	for (unsigned int i = 0; i < GENRE_TABLE_LEN; i++) {
		if (group.compare(std::string(genreTable[i].group)) == 0) {
			return genreTable[i].genreType;
		}
	}
	return EPG_EVENT_CONTENTMASK_UNDEFINED;
}

// Channel stream (RTMP)
std::string filmonAPIgetRtmpStream(std::string url, std::string name) {
	char urlDelimiter = '/';
	std::vector < std::string > streamElements;
	if (swfPlayer.empty()) {
		filmonAPIgetswfPlayer();
	}
	for (size_t p = 0, q = 0; p != url.npos; p = q) {
		std::string element = url.substr(p + (p != 0),
				(q = url.find(urlDelimiter, p + 1)) - p - (p != 0));
		streamElements.push_back(element);
	}
	if (streamElements.size() > 4) {
		std::string app = streamElements[3] + '/' + streamElements[4];
		std::string streamUrl = url + " playpath=" + name + " app=" + app
				+ " swfUrl=" + swfPlayer + " pageurl=http://www.filmon.com/"
				+ " live=1 timeout=10 swfVfy=1";
		return streamUrl;
	} else {
		XBMC->Log(LOG_ERROR, "no stream available");
		return std::string("");
	}
}

// Channel
bool filmonAPIgetChannel(unsigned int channelId, FILMON_CHANNEL *channel) {
	bool res = filmonRequest("tv/api/channel/" + intToString(channelId),
			sessionKeyParam);
	if (res == true) {
		Json::Value root;
		Json::Reader reader;
		reader.parse(response, root);
		Json::Value title = root["title"];
		Json::Value group = root["group"];
		Json::Value icon = root["extra_big_logo"];
		Json::Value streams = root["streams"];
		Json::Value tvguide = root["tvguide"];
		std::string streamURL;
		unsigned int streamCount = streams.size();
		unsigned int stream = 0;
		for (stream = 0; stream < streamCount; stream++) {
			std::string quality = streams[stream]["quality"].asString();
			if (quality.compare(std::string("high")) == 0 || quality.compare(std::string("480p")) == 0 || quality.compare(std::string("HD")) == 0) {
				XBMC->Log(LOG_DEBUG, "high quality stream found: %s", quality.c_str());
				break;
			} else {
				XBMC->Log(LOG_DEBUG, "low quality stream found: %s", quality.c_str());
			}
		}
		std::string chTitle = title.asString();
		std::string iconPath = icon.asString();
		streamURL = streams[stream]["url"].asString();
		if (streamURL.find("rtmp://") == 0) {
			streamURL = filmonAPIgetRtmpStream(streamURL, streams[stream]["name"].asString());
			XBMC->Log(LOG_DEBUG, "RTMP stream available: %s", streamURL.c_str());
		} else {
			XBMC->Log(LOG_DEBUG, "HLS stream available: %s", streamURL.c_str());
		}

		// Fix channel names logos
		if (chTitle.compare(std::string("CBEEBIES/BBC Four")) == 0) {
			chTitle = std::string("BBC Four/CBEEBIES");
			iconPath =
					std::string(
							"https://dl.dropboxusercontent.com/u/3129606/tvicons/BBC%20FOUR.png");
		}
		if (chTitle.compare(std::string("CBBC/BBC Three")) == 0) {
			chTitle = std::string("BBC Three/CBBC");
			iconPath =
					std::string(
							"https://dl.dropboxusercontent.com/u/3129606/tvicons/BBC%20THREE.png");
		}
		XBMC->Log(LOG_DEBUG, "title is %s", chTitle.c_str());

		channel->bRadio = false;
		channel->iUniqueId = channelId;
		channel->iChannelNumber = channelId;
		channel->iEncryptionSystem = 0;
		channel->strChannelName = chTitle;
		channel->strIconPath = iconPath;
		channel->strStreamURL = streamURL;
		(channel->epg).clear();

		// Get EPG
		XBMC->Log(LOG_DEBUG, "building EPG");
		unsigned int entries = 0;
		unsigned int programmeCount = tvguide.size();
		std::string offAir = std::string("OFF_AIR");
		for (unsigned int p = 0; p < programmeCount; p++) {
			Json::Value broadcastId = tvguide[p]["programme"];
			std::string programmeId = broadcastId.asString();
			Json::Value startTime = tvguide[p]["startdatetime"];
			Json::Value endTime = tvguide[p]["enddatetime"];
			Json::Value programmeName = tvguide[p]["programme_name"];
			Json::Value plot = tvguide[p]["programme_description"];
			Json::Value images = tvguide[p]["images"];
			FILMON_EPG_ENTRY epgEntry;
			if (programmeId.compare(offAir) != 0) {
				epgEntry.strTitle = programmeName.asString();
				epgEntry.iBroadcastId = stringToInt(programmeId);
				if (plot.isNull() != true) {
					epgEntry.strPlot = plot.asString();
				}
				if (!images.empty()) {
					Json::Value programmeIcon = images[(unsigned int)0]["url"];
					epgEntry.strIconPath = programmeIcon.asString();
				} else {
					epgEntry.strIconPath = "";
				}
			} else {
				epgEntry.strTitle = offAir;
				epgEntry.iBroadcastId = 0;
				epgEntry.strPlot = "";
				epgEntry.strIconPath = "";
			}
			epgEntry.iChannelId = channelId;
			if (startTime.isString()) {
				epgEntry.startTime = stringToInt(startTime.asString());
				epgEntry.endTime = stringToInt(endTime.asString());
			} else {
				epgEntry.startTime = startTime.asUInt();
				epgEntry.endTime = endTime.asUInt();
			}
			epgEntry.strPlotOutline = "";
			epgEntry.iGenreType = filmonAPIgetGenre(group.asString());
			epgEntry.iGenreSubType = 0;
			(channel->epg).push_back(epgEntry);
			entries++;
		}
		XBMC->Log(LOG_DEBUG, "number of EPG entries is %u", entries);
		clearResponse();
	}
	return res;
}

// Channel groups
std::vector<FILMON_CHANNEL_GROUP> filmonAPIgetChannelGroups() {
	bool res = filmonRequest("tv/api/groups", sessionKeyParam);
	if (res == true) {
		Json::Value root;
		Json::Reader reader;
		reader.parse(response, root);
		for (unsigned int i = 0; i < root.size(); i++) {
			Json::Value groupName = root[i]["group"];
			Json::Value groupId = root[i]["group_id"];
			Json::Value channels = root[i]["channels"];
			FILMON_CHANNEL_GROUP group;
			group.bRadio = false;
			group.iGroupId = stringToInt(groupId.asString());
			group.strGroupName = groupName.asString();
			std::vector<unsigned int> members;
			unsigned int membersCount = channels.size();
			for (unsigned int j = 0; j < membersCount; j++) {
				Json::Value member = channels[j];
				unsigned int ch = stringToInt(member.asString());
				if (std::find(channelList.begin(), channelList.end(), ch)
				!= channelList.end()) {
					members.push_back(ch);
					XBMC->Log(LOG_INFO, "added channel %u to group %s",
						ch, group.strGroupName.c_str());
				}
			}
			if (members.size() > 0) {
				group.members = members;
				groups.push_back(group);
				XBMC->Log(LOG_INFO, "added group %s",
					group.strGroupName.c_str());
			}
		}
		clearResponse();
	}
	return groups;
}

// The list of subscriber channels
std::vector<unsigned int> filmonAPIgetChannels(void) {
	return channelList;
}

// The count of subscriber channels
unsigned int filmonAPIgetChannelCount(void) {
	if (channelList.empty()) {
		return 0;
	} else
		return channelList.size();
}

// Gets all timers and recordings
bool filmonAPIgetRecordingsTimers(bool completed) {
	bool res = filmonRequest("tv/api/dvr/list", sessionKeyParam);
	if (res == true) {
		Json::Value root;
		Json::Reader reader;
		reader.parse(response, root);

		// Usage
		Json::Value total = root["userStorage"]["total"];
		Json::Value used = root["userStorage"]["recorded"];
		storageTotal = (long long int) (total.asDouble() * FILMON_ONE_HOUR_RECORDING_SIZE); // bytes
		storageUsed = (long long int) (used.asDouble() * FILMON_ONE_HOUR_RECORDING_SIZE); // bytes
		XBMC->Log(LOG_DEBUG, "recordings total is %u", storageTotal);
		XBMC->Log(LOG_DEBUG, "recordings used is %u", storageUsed);

		bool timersCleared = false;
		bool recordingsCleared = false;

		Json::Value recordingsTimers = root["recordings"];
		for (unsigned int recordingId = 0;
				recordingId < recordingsTimers.size(); recordingId++) {
			std::string recTimId =
					recordingsTimers[recordingId]["id"].asString();
			std::string recTimTitle =
					recordingsTimers[recordingId]["title"].asString();
			unsigned int recTimStart = stringToInt(
					recordingsTimers[recordingId]["time_start"].asString());
			unsigned int recDuration = stringToInt(
					recordingsTimers[recordingId]["length"].asString());

			Json::Value status = recordingsTimers[recordingId]["status"];
			if (completed && status.asString().compare(std::string(RECORDED_STATUS)) == 0) {
				if (recordingsCleared == false) {
					recordings.clear();
					recordingsCleared = true;
				}
				FILMON_RECORDING recording;
				recording.strRecordingId = recTimId;
				recording.strTitle = recTimTitle;
				recording.strStreamURL =
						recordingsTimers[recordingId]["download_link"].asString();
				recording.strPlot =
						recordingsTimers[recordingId]["description"].asString();
				recording.recordingTime = recTimStart;
				recording.iDuration = recDuration;
				recording.strIconPath =	recordingsTimers[recordingId]["images"]["channel_logo"].asString();
				recording.strThumbnailPath = recordingsTimers[recordingId]["images"]["poster"].asString();
				recordings.push_back(recording);
				XBMC->Log(LOG_DEBUG, "found completed recording %s", recording.strTitle.c_str());
			} else if (status.asString().compare(std::string(TIMER_STATUS))
					== 0) {
				if (timersCleared == false) {
					timers.clear();
					timersCleared = true;
				}

				FILMON_TIMER timer;
				timer.iClientIndex = stringToInt(recTimId);
				timer.iClientChannelUid = stringToInt(
						recordingsTimers[recordingId]["channel_id"].asString());
				timer.startTime = recTimStart;
				timer.endTime = timer.startTime + recDuration;
				timer.strTitle = recTimTitle;
				timer.state = FILMON_TIMER_STATE_NEW;
				timer.strSummary =
						recordingsTimers[recordingId]["description"].asString();
				setTimerDefaults(&timer);
				time_t t = time(0);
				if (t >= timer.startTime && t <= timer.endTime) {
					XBMC->Log(LOG_DEBUG, "found active timer %s", timer.strTitle.c_str());
					timer.state = FILMON_TIMER_STATE_RECORDING;
				} else if (t < timer.startTime) {
					XBMC->Log(LOG_DEBUG, "found scheduled timer %s", timer.strTitle.c_str());
					timer.state = FILMON_TIMER_STATE_SCHEDULED;
				} else if (t > timer.endTime) {
					XBMC->Log(LOG_DEBUG, "found completed timer %s", timer.strTitle.c_str());
					timer.state = FILMON_TIMER_STATE_COMPLETED;
				}
				timers.push_back(timer);
			}
		}
		clearResponse();
	}
	return res;
}

// Wrapper to get recordings
std::vector<FILMON_RECORDING> filmonAPIgetRecordings(void) {
	bool completed = true;
	if (filmonAPIgetRecordingsTimers(completed) != true) {
		XBMC->Log(LOG_ERROR, "failed to get recordings");
	}
	return recordings;
}

// Delete a recording
bool filmonAPIdeleteRecording(unsigned int recordingId) {
	bool res = false;
	XBMC->Log(LOG_DEBUG, "number recordings is %u", recordings.size());
	for (unsigned int i = 0; i < recordings.size(); i++) {
		XBMC->Log(LOG_DEBUG, "looking for recording %u", recordingId);
		if ((recordings[i].strRecordingId).compare(intToString(recordingId)) == 0) {
			std::string params = "record_id=" + recordings[i].strRecordingId;
			res = filmonRequest("tv/api/dvr/remove",
					sessionKeyParam + "&" + params);
			if (res) {
				Json::Value root;
				Json::Reader reader;
				reader.parse(response, root);
				if (root["success"].asBool()) {
					recordings.erase(recordings.begin() + i);
					XBMC->Log(LOG_DEBUG, "deleted recording");
				} else {
					res = false;
				}
				clearResponse();
			}
			break;
		}
		XBMC->Log(LOG_DEBUG, "found recording %u", recordings[i].strRecordingId.c_str());
	}
	return res;
}

// Get timers
std::vector<FILMON_TIMER> filmonAPIgetTimers(void) {
	if (filmonAPIgetRecordingsTimers() != true) {
		XBMC->Log(LOG_ERROR, "failed to get timers");
	}
	return timers;
}

// Add a timer
bool filmonAPIaddTimer(int channelId, time_t startTime, time_t endTime) {
	bool res = filmonRequest("tv/api/tvguide/" + intToString(channelId),
			sessionKeyParam);
	if (res) {
		Json::Value root;
		Json::Reader reader;
		reader.parse(response, root);
		for (unsigned int i = 0; i < root.size(); i++) {
			Json::Value start = root[i]["startdatetime"];
			Json::Value end = root[i]["enddatetime"];
			time_t epgStartTime = 0;
			time_t epgEndTime = 0;
			if (start.isString()) {
				epgStartTime = stringToInt(start.asString());
				epgEndTime = stringToInt(end.asString());
			} else {
				epgStartTime = start.asUInt();
				epgEndTime = end.asUInt();
			}
			if (epgStartTime == startTime || epgEndTime == endTime) {
				Json::Value broadcastId = root[i]["programme"];
				std::string programmeId = broadcastId.asString();
				Json::Value progName = root[i]["programme_name"];
				Json::Value progDesc = root[i]["programme_description"];
				std::string programmeName = progName.asString();
				std::string programmeDesc = progDesc.asString();

				std::string params = "channel_id=" + intToString(channelId)
								+ "&programme_id=" + programmeId + "&start_time="
								+ intToString(epgStartTime);
				res = filmonRequest("tv/api/dvr/add",
						sessionKeyParam + "&" + params);
				if (res) {
					Json::Value root;
					Json::Reader reader;
					reader.parse(response, root);
					if (root["success"].asBool()) {
						FILMON_TIMER timer;
						timer.iClientIndex = stringToInt(programmeId);
						timer.iClientChannelUid = channelId;
						timer.startTime = epgStartTime;
						timer.endTime = epgEndTime;
						timer.strTitle = programmeName;
						timer.strSummary = programmeDesc;
						time_t t = time(0);
						if (t >= epgStartTime && t <= epgEndTime) {
							timer.state = FILMON_TIMER_STATE_RECORDING;
						} else {
							timer.state = FILMON_TIMER_STATE_SCHEDULED;
						}
						setTimerDefaults(&timer);
						timers.push_back(timer);
						XBMC->Log(LOG_DEBUG, "addded timer");
					} else {
						res = false;
					}
				}
				break;
			}
		}
		clearResponse();
	}
	return res;
}

// Delete a timer
bool filmonAPIdeleteTimer(unsigned int timerId, bool bForceDelete) {
	bool res = true;
	for (unsigned int i = 0; i < timers.size(); i++) {
		XBMC->Log(LOG_DEBUG, "looking for timer %u", timerId);
		if (timers[i].iClientIndex == timerId) {
			time_t t = time(0);
			if ((t >= timers[i].startTime && t <= timers[i].endTime
					&& bForceDelete) || t < timers[i].startTime
					|| t > timers[i].endTime) {
				std::string params = "record_id=" + intToString(timerId);
				res = filmonRequest("tv/api/dvr/remove",
						sessionKeyParam + "&" + params);
				if (res) {
					Json::Value root;
					Json::Reader reader;
					reader.parse(response, root);
					if (root["success"].asBool()) {
						timers.erase(timers.begin() + i);
						XBMC->Log(LOG_DEBUG, "deleted timer");
					} else {
						res = false;
					}
					clearResponse();
				}
			}
			break;
		}
		XBMC->Log(LOG_DEBUG, "found timer %u", timerId);
	}
	return res;
}

// Recording usage in bytes
void filmonAPIgetUserStorage(long long *iTotal, long long *iUsed) {
	*iTotal = storageTotal;
	*iUsed = storageUsed;
}

//int main(int argc, char *argv[]) {
//	filmonAPICreate();
//	std::string username = argv[1];
//	std::string password = argv[2];
//	filmonAPIlogin(username, password);
//	std::vector<FILMON_CHANNEL_GROUP> grps = filmonAPIgetChannelGroups();
//	for (int i = 0; i < grps.size(); i++) {
//		std::cout << grps[i].strGroupName << std::endl;
//		for (int j = 0; j < grps[i].members.size();j++) {
//			std::cout << grps[i].members[j] << std::endl;
//		}
//	}
//	filmonAPIDelete();
//}

