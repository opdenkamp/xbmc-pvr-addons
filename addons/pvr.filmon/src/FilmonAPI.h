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


/* These IDs come from the DVB-SI EIT table "content descriptor"
 * Also known under the name "E-book genre assignments"
 */
#define EPG_EVENT_CONTENTMASK_UNDEFINED                0x00
#define EPG_EVENT_CONTENTMASK_MOVIEDRAMA               0x10
#define EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS       0x20
#define EPG_EVENT_CONTENTMASK_SHOW                     0x30
#define EPG_EVENT_CONTENTMASK_SPORTS                   0x40
#define EPG_EVENT_CONTENTMASK_CHILDRENYOUTH            0x50
#define EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE         0x60
#define EPG_EVENT_CONTENTMASK_ARTSCULTURE              0x70
#define EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
#define EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE       0x90
#define EPG_EVENT_CONTENTMASK_LEISUREHOBBIES           0xA0
#define EPG_EVENT_CONTENTMASK_SPECIAL                  0xB0
#define EPG_EVENT_CONTENTMASK_USERDEFINED              0xF0

typedef enum {
	FILMON_TIMER_STATE_NEW = 0,
	FILMON_TIMER_STATE_SCHEDULED = 1,
	FILMON_TIMER_STATE_RECORDING = 2,
	FILMON_TIMER_STATE_COMPLETED = 3,
} FILMON_TIMER_STATE;

typedef struct {
	int iDuration;
	int iGenreType;
	int iGenreSubType;
	std::string strChannelName;
	std::string strPlotOutline;
	std::string strPlot;
	std::string strRecordingId;
	std::string strStreamURL;
	std::string strTitle;
	std::string strIconPath;
	std::string strThumbnailPath;
	time_t recordingTime;
} FILMON_RECORDING;

typedef struct {
	unsigned int iClientIndex;
	int iClientChannelUid;
	time_t startTime;
	time_t endTime;
	FILMON_TIMER_STATE state;
	std::string strTitle;
	std::string strSummary;
	bool bIsRepeating;
	time_t firstDay;
	int iWeekdays;
	int iEpgUid;
	int iGenreType;
	int iGenreSubType;
	int iMarginStart;
	int iMarginEnd;
} FILMON_TIMER;

typedef struct {
	bool bRadio;
	int iGroupId;
	std::string strGroupName;
	std::vector<unsigned int> members;
} FILMON_CHANNEL_GROUP;

typedef struct {
	unsigned int iBroadcastId;
	std::string strTitle;
	unsigned int iChannelId;
	time_t startTime;
	time_t endTime;
	std::string strPlotOutline;
	std::string strPlot;
	std::string strIconPath;
	int iGenreType;
	int iGenreSubType;
	time_t firstAired;
	int iParentalRating;
	int iStarRating;
	bool bNotify;
	int iSeriesNumber;
	int iEpisodeNumber;
	int iEpisodePartNumber;
	std::string strEpisodeName;
} FILMON_EPG_ENTRY;

typedef struct {
	bool bRadio;
	unsigned int iUniqueId;
	unsigned int iChannelNumber;
	unsigned int iEncryptionSystem;
	std::string strChannelName;
	std::string strIconPath;
	std::string strStreamURL;
	std::vector<FILMON_EPG_ENTRY> epg;
} FILMON_CHANNEL;

bool filmonAPICreate(void);
void filmonAPIDelete(void);
bool filmonAPIkeepAlive(void);
bool filmonAPIlogin(std::string username, std::string password);
void filmonAPIgetUserStorage(long long *iTotal, long long *iUsed);
bool filmonAPIdeleteTimer(unsigned int timerId, bool bForceDelete);
bool filmonAPIaddTimer(int channelId, time_t startTime, time_t endTime);
bool filmonAPIdeleteRecording(unsigned int recordingId);
bool filmonAPIgetChannel(unsigned int channelId, FILMON_CHANNEL *channel);
std::vector<unsigned int> filmonAPIgetChannels(void);
unsigned int filmonAPIgetChannelCount(void);
std::vector<FILMON_CHANNEL_GROUP> filmonAPIgetChannelGroups();
std::vector<FILMON_RECORDING> filmonAPIgetRecordings(void);
std::vector<FILMON_TIMER> filmonAPIgetTimers(void);
std::string filmonAPIConnection();


