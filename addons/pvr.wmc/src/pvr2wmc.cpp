/*
*      Copyright (C) 2011 Pulse-Eight
*      http://www.pulse-eight.com/
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

#include "tinyxml/XMLUtils.h"
#include "pvr2wmc.h"
#include "utilities.h"
#include "DialogRecordPref.h"
#include "DialogDeleteTimer.h"
#include "platform/util/timeutils.h"

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

#define null NULL

#define STRCPY(dest, src) strncpy(dest, src, sizeof(dest)-1); 
#define FOREACH(ss, vv) for(std::vector<CStdString>::iterator ss = vv.begin(); ss != vv.end(); ++ss)

int64_t _lastRecordingUpdateTime;

Pvr2Wmc::Pvr2Wmc(void)
{
	_iEpgStart = -1;
	_strDefaultIcon =  "http://www.royalty-free.tv/news/wp-content/uploads/2011/06/cc-logo1.jpg";
	_strDefaultMovie = "";

	_socketClient.SetServerName(g_strServerName);
	_socketClient.SetClientName(g_strClientName);
	_socketClient.SetServerPort(g_port);

	_signalStatusCount = 0;
	_discardSignalStatus = false;

	_lastRecordingUpdateTime = 0;
	_streamFile = 0;				// handle to a streamed file
	_streamFileName = "";
	_initialStreamResetCnt = 0;		// used to count how many times we reset the stream position (due to 2 pass demuxer)
	_initialStreamPosition = 0;     // used to set an initial position (multiple clients watching the same live tv buffer)
	_lostStream = false;			// set to true if stream is lost
	_lastStreamSize = 0;
	_isStreamFileGrowing = false;	
	_streamWTV = true;		// by default, assume we are streaming Wtv files (not ts files)
}

Pvr2Wmc::~Pvr2Wmc(void)
{
	_channels.clear();
	_groups.clear();
}

bool Pvr2Wmc::IsServerDown()
{
	CStdString request;
	request.Format("GetServiceStatus|%s|%s", PVRWMC_GetClientVersion(), g_clientOS);
	_socketClient.SetTimeOut(10);					// set a timout interval for checking if server is down
	bool res = _socketClient.GetBool(request);		
	return !res;
}

void Pvr2Wmc::UnLoading()
{
	bool res = _socketClient.GetBool("ClientGoingDown");			// returns true if server is up
}

const char *Pvr2Wmc::GetBackendVersion(void)
{
	if (!IsServerDown())
	{
		CStdString request;
		request.Format("GetServerVersion");
		vector<CStdString> results = _socketClient.GetVector(request);		
		if (results.size() > 1)
		{
			_serverBuild = atoi(results[1]);
			return results[0].c_str();
		}
	}
	return "0.0";
}

int Pvr2Wmc::GetChannelsAmount(void)
{
	return _socketClient.GetInt("GetChannelCount");
}

// test for returned error vector from server, handle accompanying messages if any
bool isServerError(vector<CStdString> results)
{
	if (results[0] == "error")
	{
		if (results.size() > 1 && results[1].length() != 0)
		{
			XBMC->Log(LOG_ERROR, results[1].c_str());	// log more info on error
		}
		if (results.size() > 2 != 0)
		{
			int errorID = atoi(results[2].c_str());
			if (errorID != 0)
			{
				CStdString errStr = XBMC->GetLocalizedString(errorID);
				XBMC->QueueNotification(QUEUE_ERROR, errStr.c_str());
			}
		}
		return true;
	}
	else 
		return false;
}

// look at result vector from server and perform any updates requested
void TriggerUpdates(vector<CStdString> results)
{
	FOREACH(response, results)
	{
		if (*response == "updateTimers")
			PVR->TriggerTimerUpdate();
		else if (*response == "updateRecordings")
			PVR->TriggerRecordingUpdate();
		else if (*response == "updateChannels")
			PVR->TriggerChannelUpdate();
		else if (*response == "updateChannelGroups")
			PVR->TriggerChannelGroupsUpdate();
	}
}

// xbmc call: get all channels for either tv or radio
PVR_ERROR Pvr2Wmc::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	vector<CStdString> results = _socketClient.GetVector("GetChannels");

	FOREACH(response, results)
	{ 
		PVR_CHANNEL xChannel;

		memset(&xChannel, 0, sizeof(PVR_CHANNEL));							// set all mem to zero
		vector<CStdString> v = split(*response, "|");
		// packing: id, bradio, c.OriginalNumber, c.CallSign, c.IsEncrypted, imageStr, c.IsBlocked
		xChannel.iUniqueId = strtoul(v[0].c_str(), 0, 10);					// convert to unsigned int
		xChannel.bIsRadio = Str2Bool(v[1]);
		xChannel.iChannelNumber = atoi(v[2].c_str());
		STRCPY(xChannel.strChannelName,  v[3].c_str());
		//CStdString test = "C:\\Users\\Public\\Recorded TV\\dump.mpg";
		//STRCPY(xChannel.strStreamURL,  test.c_str());
		//STRCPY(xChannel.strInputFormat, "video/wtv");
		xChannel.iEncryptionSystem = Str2Bool(v[4]);
		if (v[5].compare("NULL") != 0)										// if icon path is null
			STRCPY(xChannel.strIconPath,  v[5].c_str()); 
		xChannel.bIsHidden = Str2Bool(v[6]);

		PVR->TransferChannelEntry(handle, &xChannel);
	}
	return PVR_ERROR_NO_ERROR;
}

int Pvr2Wmc::GetChannelGroupsAmount(void)
{
	//return _groups.size(); !!!
	return 1;
}

PVR_ERROR Pvr2Wmc::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
	for (unsigned int iGroupPtr = 0; iGroupPtr < _groups.size(); iGroupPtr++)
	{
		PVR_CHANNELGroup &group = _groups.at(iGroupPtr);
		if (group.bRadio == bRadio)
		{
			PVR_CHANNEL_GROUP xbmcGroup;
			memset(&xbmcGroup, 0, sizeof(PVR_CHANNEL_GROUP));

			xbmcGroup.bIsRadio = bRadio;
			strncpy(xbmcGroup.strGroupName, group.strGroupName.c_str(), sizeof(xbmcGroup.strGroupName) - 1);

			PVR->TransferChannelGroup(handle, &xbmcGroup);
		}
	}

	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Pvr2Wmc::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
	for (unsigned int iGroupPtr = 0; iGroupPtr < _groups.size(); iGroupPtr++)
	{
		PVR_CHANNELGroup &myGroup = _groups.at(iGroupPtr);
		if (!strcmp(myGroup.strGroupName.c_str(),group.strGroupName))
		{
			for (unsigned int iChannelPtr = 0; iChannelPtr < myGroup.members.size(); iChannelPtr++)
			{
				int iId = myGroup.members.at(iChannelPtr) - 1;
				if (iId < 0 || iId > (int)_channels.size() - 1)
					continue;
				PVR_CHANNEL &channel = _channels.at(iId);
				PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
				memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

				strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
				xbmcGroupMember.iChannelUniqueId = channel.iUniqueId;
				xbmcGroupMember.iChannelNumber   = channel.iChannelNumber;

				PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
			}
		}
	}

	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Pvr2Wmc::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	CStdString request;
	request.Format("GetEntries|%d|%d|%d", channel.iUniqueId, iStart, iEnd);		// build the request string

	vector<CStdString> results = _socketClient.GetVector(request);												// get entries from server
	
	FOREACH(response, results)
	{ 
		EPG_TAG xEpg;
		memset(&xEpg, 0, sizeof(EPG_TAG));											// set all mem to zero
		vector<CStdString> v = split(*response, "|");								// split to unpack string

		if (v.size() < 16)
		{
			XBMC->Log(LOG_DEBUG, "Wrong number of field xfered for epg data");
			return PVR_ERROR_NO_ERROR;
		}

		//	e.Id, e.Program.Title, c.OriginalNumber, start_t, end_t,   
		//	e.Program.ShortDescription, e.Program.Description,
		//	origAirDate, e.TVRating, e.Program.StarRating,
		//	e.Program.SeasonNumber, e.Program.EpisodeNumber,
		//	e.Program.EpisodeTitle
		xEpg.iUniqueBroadcastId = atoi(v[0].c_str());		// entry ID
		xEpg.strTitle = v[1].c_str();						// entry title
		xEpg.iChannelNumber = atoi(v[2].c_str());			// channel number
		xEpg.startTime = atol(v[3].c_str());				// start time
		xEpg.endTime = atol(v[4].c_str());					// end time
		xEpg.strPlotOutline = v[5].c_str();					// short plot description (currently using episode name, if there is one)
		xEpg.strPlot = v[6].c_str();						// long plot description
		xEpg.firstAired = atol(v[7].c_str());				// orig air date
		xEpg.iParentalRating = atoi(v[8].c_str());			// tv rating
		xEpg.iStarRating = atoi(v[9].c_str());				// star rating
		xEpg.iSeriesNumber = atoi(v[10].c_str());			// season (?) number
		xEpg.iEpisodeNumber = atoi(v[11].c_str());			// episode number
		xEpg.iGenreType = atoi(v[12].c_str());				// season (?) number
		xEpg.iGenreSubType = atoi(v[13].c_str());			// general genre type
		xEpg.strIconPath = v[14].c_str();					// the icon url
		xEpg.strEpisodeName = v[15].c_str();				// the episode name
		xEpg.strGenreDescription = "";

		PVR->TransferEpgEntry(handle, &xEpg);

	}
	return PVR_ERROR_NO_ERROR;
}


// timer functions -------------------------------------------------------------
int Pvr2Wmc::GetTimersAmount(void)
{
	return _socketClient.GetInt("GetTimerCount");
}

#ifdef TARGET_WINDOWS
UINT_PTR _recTimer = null;					// holds the recording update timer

// this function is a callback for Pvr2Wmc::UpdateRecordingTimer
VOID CALLBACK RecUpdateTimerFunc( 
    HWND hwnd,        // handle to window for timer messages 
    UINT message,     // WM_TIMER message 
    UINT idTimer,     // timer identifier 
    DWORD dwTime)     // current system time 
{ 
	PVR->TriggerRecordingUpdate();			// tell xbmc to update recordings display
	assert(KillTimer(NULL, idTimer));		// stop timer from going again
	_recTimer = null;
	XBMC->Log(LOG_DEBUG, "Recording Files updated by function timer");
}

// activate recording file update after the input number of msec
void Pvr2Wmc::UpdateRecordingTimer(int msec)
{
	if (_recTimer == null)			// if timer is already running do nothing
	{
		_recTimer = SetTimer(
			null,					// hWnd
			0,						// id (if hWnd is null, the id is assigned)
			msec,					// time till timer goes off
			RecUpdateTimerFunc		// update rec file function
		);
	}
}
#endif

PVR_ERROR Pvr2Wmc::AddTimer(const PVR_TIMER &xTmr)  
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	// TODO: haven't figured out how implement changes to timers that aren't SE based
	if (xTmr.iClientIndex != -1)				//  != -1 means user is trying to change params of an existing timer
	{
		return PVR_ERROR_NOT_IMPLEMENTED;
	}

	bool isSeries = false;						// whether show being requested is a series

	// series specific params for recording
	bool recSeries = false;						// if true, request a series recording
	int runType;								// the type of episodes to record (all, new, live)
	bool anyChannel;							// whether to rec series from ANY channel
	bool anyTime;								// whether to rec series at ANY time

	CStdString command;
	CStdString timerStr = Timer2String(xTmr);	// convert timer to string

	if (xTmr.startTime != 0 && xTmr.iEpgUid != -1)		// if we are NOT doing an 'instant' record (=0) AND not a 'manual' record (=-1)
	{

		command = "GetShowInfo" + timerStr;		// request data about the show that will be recorded by the timer
		vector<CStdString> info;				// holds results from server
		info = _socketClient.GetVector(command);				// get results from server

		if (isServerError(info))
		{
			return PVR_ERROR_SERVER_ERROR;
		} 
		else 
		{
			isSeries = info[0] == "True";			// first string determines if show is a series (all that is handled for now)
			if (isSeries)							// if the show is a series, next string contains record params for series
			{
				vector<CStdString> v = split(info[1].c_str(), "|");		// split to unpack string containing series params
				// fill in params for dialog windows
				recSeries = v[0] == "True";								// get reset of params for dialog windows
				runType = atoi(v[1].c_str());							// any=0, firstRun=1, live=2
				anyChannel = v[2] == "True";
				anyTime = v[3] == "True";
				// start dialogwindow
				CDialogRecordPref vWindow(	recSeries, runType, anyChannel, anyTime,
					v[4]/*channelName*/, v[5]/*=startTimeStr*/, v[6]/*showName*/);

				int dlogResult = vWindow.DoModal();
				if (dlogResult == 1)								// present dialog with recording options
				{
					recSeries = vWindow.RecSeries;
					if (recSeries)
					{
						runType = vWindow.RunType;					// the type of episodes to record (0->all, 1->new, 2->live)
						anyChannel = vWindow.AnyChannel;			// whether to rec series from ANY channel
						anyTime = vWindow.AnyTime;					// whether to rec series at ANY time
					}
				}
				else if (dlogResult == 0)
					return PVR_ERROR_NO_ERROR;						// user canceled timer in dialog
				else
				{
				}
			}
		}
	}

	command = "SetTimer" + timerStr;					// create setTimer request

	// if recording a series append series info request
	CStdString extra;
	if (recSeries)
		extra.Format("|%d|%d|%d|%d", recSeries, runType, anyChannel, anyTime);
	else
		extra.Format("|%d", recSeries);

	command.append(extra);								

	vector<CStdString> results = _socketClient.GetVector(command);	// get results from server

	PVR->TriggerTimerUpdate();							// update timers regardless of whether there is an error

	if (isServerError(results))
	{
		return PVR_ERROR_SERVER_ERROR;
	} 
	else 
	{
		XBMC->Log(LOG_DEBUG, "recording added for timer '%s', with rec state %s", xTmr.strTitle, results[0].c_str());

		if (results.size() > 1)											// if there is extra results sent from server...
		{
			FOREACH(result, results)
			{
				vector<CStdString> splitResult = split(*result, "|");	// split to unpack extra info on each result
				CStdString infoStr;

				if (splitResult[0] == "recordingNow")					// recording is active now
				{
#ifdef TARGET_WINDOWS
					UpdateRecordingTimer(10000);						// give time for the recording to start, then update recordings list
#endif
				}
				else if (splitResult[0] == "recordingChannel")			// service picked a different channel for timer
				{
					XBMC->Log(LOG_DEBUG, "timer channel changed by wmc to '%s'", splitResult[1].c_str());
					// build info string and notify user of channel change
					infoStr = XBMC->GetLocalizedString(30009) + splitResult[1];		
					XBMC->QueueNotification(QUEUE_ERROR, infoStr.c_str());
				}
				else if (splitResult[0] == "recordingTime")				// service picked a different start time for timer
				{
					XBMC->Log(LOG_DEBUG, "timer start time changed by wmc to '%s'", splitResult[1].c_str());
					// build info string and notify user of time change
					infoStr = XBMC->GetLocalizedString(30010) + splitResult[1];
					XBMC->QueueNotification(QUEUE_ERROR, infoStr.c_str());
				}
				else if (splitResult[0] == "increasedEndTime")			// end time has been increased on an instant record
				{
					XBMC->Log(LOG_DEBUG, "instant record end time increased by '%s' minutes", splitResult[1].c_str());
					// build info string and notify user of time increase
					infoStr = XBMC->GetLocalizedString(30013) + splitResult[1] + " min";
					XBMC->QueueNotification(QUEUE_ERROR, infoStr.c_str());
				}
			}
		}

		return PVR_ERROR_NO_ERROR;
	}
}

CStdString Pvr2Wmc::Timer2String(const PVR_TIMER &xTmr)
{
	CStdString tStr;

	tStr.Format("|%d|%d|%d|%d|%d|%s|%d|%d|%d|%d|%d",													// format for 11 params:
		xTmr.iClientIndex, xTmr.iClientChannelUid, xTmr.startTime, xTmr.endTime, PVR_TIMER_STATE_NEW,		// 5 params
		xTmr.strTitle, xTmr.iPriority,  xTmr.iMarginStart, xTmr.iMarginEnd, xTmr.bIsRepeating,				// 5 params
		xTmr.iEpgUid);																						// 1 param

	return tStr;
}

PVR_ERROR Pvr2Wmc::DeleteTimer(const PVR_TIMER &xTmr, bool bForceDelete)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	bool deleteSeries = false;

	if (xTmr.bIsRepeating)									// if timer is a series timer, ask if want to cancel series
	{
		CDialogDeleteTimer vWindow(deleteSeries, xTmr.strTitle);
		int dlogResult = vWindow.DoModal();
		if (dlogResult == 1)								// present dialog with delete timer options
		{
			deleteSeries = vWindow.DeleteSeries;
		}
		else if (dlogResult == 0)
			return PVR_ERROR_NO_ERROR;						// user canceled in delete timer dialog
		//else if dialog fails, just delete the episode
	}

	CStdString command = "DeleteTimer" + Timer2String(xTmr);

	CStdString eStr;										// append whether to delete the series or episode
	eStr.Format("|%d", deleteSeries);
	command.append(eStr);

	vector<CStdString> results = _socketClient.GetVector(command);			// get results from server

	PVR->TriggerTimerUpdate();									// update timers regardless of whether there is an error

	if (isServerError(results))									// did the server do it?
	{
		return PVR_ERROR_SERVER_ERROR;
	}
	else
	{
		if (deleteSeries)
			XBMC->Log(LOG_DEBUG, "deleted series timer '%s', with rec state %s", xTmr.strTitle, results[0].c_str());
		else
			XBMC->Log(LOG_DEBUG, "deleted timer '%s', with rec state %s", xTmr.strTitle, results[0].c_str());
		return PVR_ERROR_NO_ERROR;
	}
}

PVR_ERROR Pvr2Wmc::GetTimers(ADDON_HANDLE handle)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	vector<CStdString> responses = _socketClient.GetVector("GetTimers");							

	FOREACH(response, responses)
	{
		PVR_TIMER xTmr;
		memset(&xTmr, 0, sizeof(PVR_TIMER));						// set all struct to zero

		vector<CStdString> v = split(*response, "|");				// split to unpack string
		// eId, chId, start_t, end_t, pState,
		// rp.Program.Title, ""/*recdir*/, rp.Program.EpisodeTitle/*summary?*/, rp.Priority, rp.Request.IsRecurring,
		// eId, preMargin, postMargin, genre, subgenre
		xTmr.iClientIndex = atoi(v[0].c_str());				// timer index (set to same as Entry ID)
		xTmr.iClientChannelUid = atoi(v[1].c_str());		// channel id
		xTmr.startTime = atoi(v[2].c_str());                // start time 
		xTmr.endTime = atoi(v[3].c_str());                  // end time 
		xTmr.state = (PVR_TIMER_STATE)atoi(v[4].c_str());   // current state of time

		STRCPY(xTmr.strTitle, v[5].c_str());				// timer name (set to same as Program title)
		STRCPY(xTmr.strDirectory, v[6].c_str());			// rec directory
		STRCPY(xTmr.strSummary, v[7].c_str());				// currently set to episode title
		xTmr.iPriority = atoi(v[8].c_str());				// rec priority
		xTmr.bIsRepeating = Str2Bool(v[9].c_str());			// repeating rec (set to series flag)

		xTmr.iEpgUid = atoi(v[10].c_str());					// epg ID (same as client ID, except for a 'manual' record)
		xTmr.iMarginStart = atoi(v[11].c_str());			// rec margin at start (sec)
		xTmr.iMarginEnd = atoi(v[12].c_str());				// rec margin at end (sec)
		xTmr.iGenreType = atoi(v[13].c_str());				// genre ID
		xTmr.iGenreSubType = atoi(v[14].c_str());			// sub genre ID

		PVR->TransferTimerEntry(handle, &xTmr);
	}

	// check time since last time Recordings were updated, update if it has been awhile
	if ( PLATFORM::GetTimeMs() >  _lastRecordingUpdateTime + 20000)
	{
		PVR->TriggerRecordingUpdate();
	}
	return PVR_ERROR_NO_ERROR;
}

// recording functions ------------------------------------------------------------------------
int Pvr2Wmc::GetRecordingsAmount(void)
{
	return _socketClient.GetInt("GetRecordingsAmount");
}

// recording file  functions
PVR_ERROR Pvr2Wmc::GetRecordings(ADDON_HANDLE handle)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	vector<CStdString> responses = _socketClient.GetVector("GetRecordings");				

	FOREACH(response, responses)
	{
		PVR_RECORDING xRec;
		memset(&xRec, 0, sizeof(PVR_RECORDING));					// set all struct to zero

		vector<CStdString> v = split(*response, "|");				// split to unpack string

		// r.Id, r.Program.Title, r.FileName, recDir, plotOutline,
		// plot, r.Channel.CallSign, ""/*icon path*/, ""/*thumbnail path*/, ToTime_t(r.RecordingTime),
		// duration, r.RequestedProgram.Priority, r.KeepLength.ToString(), genre, subgenre

		STRCPY(xRec.strRecordingId, v[0].c_str());
		STRCPY(xRec.strTitle, v[1].c_str());
		STRCPY(xRec.strStreamURL, v[2].c_str());
		STRCPY(xRec.strDirectory, v[3].c_str());
		STRCPY(xRec.strPlotOutline, v[4].c_str());
		STRCPY(xRec.strPlot, v[5].c_str());
		STRCPY(xRec.strChannelName, v[6].c_str());
		STRCPY(xRec.strIconPath, v[7].c_str());
		STRCPY(xRec.strThumbnailPath, v[8].c_str());
		xRec.recordingTime = atol(v[9].c_str());
		xRec.iDuration = atoi(v[10].c_str());
		xRec.iPriority = atoi(v[11].c_str());
		xRec.iLifetime = atoi(v[12].c_str());
		xRec.iGenreType = atoi(v[13].c_str());
		xRec.iGenreSubType = atoi(v[14].c_str());
		if (g_bEnableMultiResume)
			xRec.iLastPlayedPosition = atoi(v[15].c_str());
	

		PVR->TransferRecordingEntry(handle, &xRec);
	}

	_lastRecordingUpdateTime = PLATFORM::GetTimeMs();

	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Pvr2Wmc::DeleteRecording(const PVR_RECORDING &recording)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	CStdString command;// = format("DeleteRecording|%s|%s|%s", recording.strRecordingId, recording.strTitle, recording.strStreamURL);
	command.Format("DeleteRecording|%s|%s|%s", recording.strRecordingId, recording.strTitle, recording.strStreamURL);

	vector<CStdString> results = _socketClient.GetVector(command);					// get results from server


	if (isServerError(results))							// did the server do it?
	{
		return PVR_ERROR_NO_ERROR;						// report "no error" so our error shows up
	}
	else
	{
		TriggerUpdates(results);
		//PVR->TriggerRecordingUpdate();					// tell xbmc to update recording display
		XBMC->Log(LOG_DEBUG, "deleted recording '%s'", recording.strTitle);

		//if (results.size() == 2 && results[0] == "updateTimers")	// if deleted recording was actually recording a the time
		//	PVR->TriggerTimerUpdate();								// update timer display too

		return PVR_ERROR_NO_ERROR;
	}
}


PVR_ERROR Pvr2Wmc::RenameRecording(const PVR_RECORDING &recording)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	CStdString command;// = format("RenameRecording|%s|%s", recording.strRecordingId, recording.strTitle);
	command.Format("RenameRecording|%s|%s", recording.strRecordingId, recording.strTitle);

	vector<CStdString> results = _socketClient.GetVector(command);					// get results from server

	if (isServerError(results))							// did the server do it?
	{
		return PVR_ERROR_NO_ERROR;						// report "no error" so our error shows up
	}
	else
	{
		TriggerUpdates(results);
		XBMC->Log(LOG_DEBUG, "deleted recording '%s'", recording.strTitle);
		return PVR_ERROR_NO_ERROR;
	}
}

// set the recording resume position in the wmc database
PVR_ERROR Pvr2Wmc::SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
	CStdString command;
	command.Format("SetResumePosition|%s|%d", recording.strRecordingId, lastplayedposition);
	vector<CStdString> results = _socketClient.GetVector(command);					
	PVR->TriggerRecordingUpdate();		// this is needed to get the new resume point actually used by the player (xbmc bug)								
	return PVR_ERROR_NO_ERROR;
}

// get the rercording resume position from the wmc database
// note: although this resume point time will be displayed to the user in the gui (in the resume dlog)
// the return value is ignored by the xbmc player.  That's why TriggerRecordingUpdate is required in the setting above
int Pvr2Wmc::GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
	CStdString command;
	command.Format("GetResumePosition|%s", recording.strRecordingId); 
	int pos = _socketClient.GetInt(command);
	return pos;
}


CStdString Pvr2Wmc::Channel2String(const PVR_CHANNEL &xCh)
{
	// packing: id, bradio, c.OriginalNumber, c.CallSign, c.IsEncrypted, imageStr, c.IsBlocked
	CStdString chStr;
	chStr.Format("|%d|%d|%d|%s", xCh.iUniqueId, xCh.bIsRadio, xCh.iChannelNumber, xCh.strChannelName);
	return chStr;
}

// live/recorded stream functions --------------------------------------------------------------

bool Pvr2Wmc::OpenLiveStream(const PVR_CHANNEL &channel)
{
	if (IsServerDown())
		return false;

	_lostStream = true;								// init

	CloseLiveStream(false);							// close current stream (if any)

	CStdString request = "OpenLiveStream" + Channel2String(channel);		// request a live stream using channel
	vector<CStdString> results = _socketClient.GetVector(request);						// try to open live stream, get path to stream file

	if (isServerError(results))												// test if server reported an error
	{
		return false;
	} 
	else 
	{
		_streamFileName = results[0];								// get path of streamed file
		_streamWTV = EndsWith(results[0], "wtv");					// true if stream file is a wtv file

		if (results.size() > 1)
			XBMC->Log(LOG_DEBUG, "OpenLiveStream> opening stream: " + results[1]);		// log password safe path of client if available
		else
			XBMC->Log(LOG_DEBUG, "OpenLiveStream> opening stream: " + _streamFileName);	
		
		// Initialise variables for starting stream at an offset
		_initialStreamResetCnt = 0;
		_initialStreamPosition = 0;

		// Check for a specified initial position and save it for the first ReadLiveStream command to use
		if (results.size() > 2)
		{
			_initialStreamPosition = atoll(results[2]);
		}

		_streamFile = XBMC->OpenFile(_streamFileName.c_str(), 0);	// open the video file for streaming, same handle

		if (!_streamFile)	// something went wrong
		{
			CStdString lastError;
#ifdef TARGET_WINDOWS
			int errorVal = GetLastError();
			lastError.Format("Error opening stream file, Win32 error code: %d", errorVal);
#else
			lastError.Format("Error opening stream file");
#endif
			XBMC->Log(LOG_ERROR, lastError.c_str());						// log more info on error
			
			_socketClient.GetBool("StreamStartError|" + _streamFileName);	// tell server stream did not start

			return false;
		}
		else
		{
			_discardSignalStatus = false;			// reset signal status discard flag
			XBMC->Log(LOG_DEBUG, "OpenLiveStream> stream file opened successfully");
		}

		_lostStream = false;						// if we got to here, stream started ok
		_lastStreamSize = 0;
		_isStreamFileGrowing = true;
		return true;								// stream is up
	}
}

bool Pvr2Wmc::SwitchChannel(const PVR_CHANNEL &channel)
{
	CStdString request = "SwitchChannel|" + g_strClientName + Channel2String(channel);		// request a live stream using channel
	return _socketClient.GetBool(request);				// try to open live stream, get path to stream file
}

// read from the live stream file opened in OpenLiveStream
int Pvr2Wmc::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
	if (_lostStream)									// if stream has already been flagged as lost, return 0 bytes 
		return 0;										// after this happens a few time, xbmc will call CloseLiveStream

	if (!_streamWTV)									// if NOT streaming wtv, make sure stream is big enough before it is read
	{						
		int timeout = 0;								// reset timeout counter

		// If we are trying to skip to an initial start position (eg we are watching an existing live stream
		// in a multiple client scenario), we need to do it here, as the Seek command didnt work in OpenLiveStream,
		// XBMC just started playing from the start of the file anyway.  But once the stream is open, XBMC repeatedly 
		// calls ReadLiveStream and a Seek() command done here DOES get actioned.
		// 
		// So the first time we come in here, we can Seek() to our desired start offset.
		//
		// However I found the XBMC demuxer makes an initial pass first and then sets the position back to 0 again and makes a 2nd pass,
		// So we actually need to Seek() to our initial start position more than once.  Because there are other situations where we can end up 
		// at the start of the file (such as the user rewinding) the easiest option at this point is to simply assume the demuxer makes 2 passes,
		// and to reset the Seek position twice before clearing the stored value and thus no longer performing the reset.
		
		// Seek to initial file position if OpenLiveStream stored a starting offset and we are at position 0 (start of the file)
		if (_initialStreamPosition > 0 && PositionLiveStream() == 0)
		{
			long long newPosition = XBMC->SeekFile(_streamFile, _initialStreamPosition, SEEK_SET);
			if (newPosition == _initialStreamPosition)
			{
				XBMC->Log(LOG_DEBUG, "ReadLiveStream> stream file seek to initial position %llu successful", _initialStreamPosition);
			}
			else
			{
				XBMC->Log(LOG_DEBUG, "ReadLiveStream> stream file seek to initial position %llu failed (new position: %llu)", _initialStreamPosition, newPosition);
			}

			_initialStreamResetCnt++;
			if (_initialStreamResetCnt >= 2)
			{
				_initialStreamPosition = 0;				// reset stored value to 0 once we have performed 2 resets (2 pass demuxer)
			}
		}

		long long totalRead = PositionLiveStream();		// get the current file position

		long long fileSize = _lastStreamSize;			// use the last fileSize found, rather than querying host

		if (_isStreamFileGrowing && totalRead + iBufferSize > fileSize)		// if its not big enough for the readsize
			fileSize = ActualFileSize(timeout);								// get the current size of the stream file

		// if the stream file is growing, see if the stream file is big enough to accomodate this read
		// if its not, wait until it is
		while (_isStreamFileGrowing && totalRead + iBufferSize > fileSize)
		{
			usleep(600000);								// wait a little (600ms) before we try again
			timeout++;
			fileSize = ActualFileSize(timeout);			// get new file size after delay

			if (!_isStreamFileGrowing)					// if streamfile is no longer growing...
			{
				if (CheckErrorOnServer())				// see if server says there is an error
				{
					_lostStream = true;					// if an error was posted, close the stream down
					return -1;																
				}
				else
					break;								// terminate loop since stream file isn't growing no sense in waiting
			}
			else if (fileSize == -1)					// if fileSize -1, server is reporting an 'unkown' error with the stream
			{
				XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30003));	// display generic error with stream
				XBMC->Log(LOG_DEBUG, "live tv error, server reported error");
				_lostStream = true;														// flag that stream is down
				return -1;																
			}

			if (timeout > 50 )									// if after 30 sec the file has not grown big enough, timeout
			{
				_lostStream = true;								// flag that stream is down
				if (totalRead == 0 && fileSize == 0)			// if no data was ever read, assume no video signal
				{
					XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30004));
					XBMC->Log(LOG_DEBUG, "no video found for stream");
				} 
				else											// a mysterious reason caused timeout
				{
					XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30003));	// otherwise display generic error
					XBMC->Log(LOG_DEBUG, "live tv timed out, unknown reason");
				}
				return -1;																	// this makes xbmc call closelivestream
			}
		}
	}

	// read from stream file
	unsigned int lpNumberOfBytesRead = XBMC->ReadFile(_streamFile, pBuffer, iBufferSize);

	return lpNumberOfBytesRead;
}

// see if server posted an error for this client
// if server has not posted an error, return False
bool Pvr2Wmc::CheckErrorOnServer()
{
	if (!IsServerDown())
	{
		CStdString request;
		request.Format("CheckError");
		//request.Format("CheckError|%d|%d|%d", checkCnt, (long)streamPos, (long)streamfileSize);
		vector<CStdString> results = _socketClient.GetVector(request);	// see if server posted an error for active stream
		return isServerError(results);
	}
	return false;
}

//#define SEEK_SET    0
//#define SEEK_CUR    1
//#define SEEK_END    2
long long Pvr2Wmc::SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) 
{
	int64_t lFilePos = 0;
	if (_streamFile != 0)
	{
		lFilePos = XBMC->SeekFile(_streamFile, iPosition, iWhence);
	}
	return lFilePos;
}

// return the current file pointer position
long long Pvr2Wmc::PositionLiveStream(void) 
{
	int64_t lFilePos = -1;
	if (_streamFile != 0)
	{
		lFilePos = XBMC->GetFilePosition(_streamFile);
	}
	return lFilePos;
}

long long Pvr2Wmc::ActualFileSize(int count)
{
	long long lFileSize = 0;

	if (_lostStream)									// if stream was lost, return 0 file size (may not be needed)
		return 0;

	if (!_isStreamFileGrowing)							// if stream file is no longer growing, return the last stream size
	{
		lFileSize = _lastStreamSize;
	}
	else
	{
		CStdString request;
		request.Format("StreamFileSize|%d", count);		// request stream size form client, passing number of consecutive queries
		lFileSize = _socketClient.GetLL(request);

		if (lFileSize < -1)								// if server returns a negative file size, it means the stream file is no longer growing (-1 => error)
		{
			lFileSize = -lFileSize;						// make stream file length positive
			_isStreamFileGrowing = false;				// flag that stream file is no longer growing
		}
		_lastStreamSize = lFileSize;					// save this stream size
	}
	return lFileSize;
}

// return the length of the current stream file
long long Pvr2Wmc::LengthLiveStream(void) 
{
	if (_lastStreamSize > 0)
		return _lastStreamSize;
	return -1;
}

void Pvr2Wmc::PauseStream(bool bPaused)
{
}

bool Pvr2Wmc::CloseLiveStream(bool notifyServer /*=true*/)
{
	if (IsServerDown())
		return false;

	if (_streamFile != 0)						// if file is still open, close it
		XBMC->CloseFile(_streamFile);

	_streamFile = 0;							// file handle now closed
	_streamFileName = "";

	_lostStream = true;							// for cleanliness

	if (notifyServer)
	{	
		return _socketClient.GetBool("CloseLiveStream");		// tell server to close down stream
	}
	else
		return true;
}

// this is only called if a recording is actively being recorded, xbmc detects this when the server
// doesn't enter a path in the strStreamURL field during a "GetRecordings"
bool Pvr2Wmc::OpenRecordedStream(const PVR_RECORDING &recording)
{
	if (IsServerDown())
		return false;

	_lostStream = true;								// init

	// request an active recording stream
	CStdString request;// = format("OpenRecordingStream|%s", recording.strRecordingId);	
	request.Format("OpenRecordingStream|%s", recording.strRecordingId);	
	vector<CStdString> results = _socketClient.GetVector(request);	// try to open recording stream, get path to stream file

	if (isServerError(results))								// test if server reported an error
	{
		return false;
	} 
	else 
	{
		_streamFileName = results[0];
		_streamWTV = EndsWith(_streamFileName, "wtv");		// true if stream file is a wtv file

		if (results.size() >  1)
			XBMC->Log(LOG_DEBUG, "OpenRecordedStream> rec stream type: " + results[1]);		// either a 'passive' or 'active' WTV OR a TS file
		
		if (results.size() > 2)
			XBMC->Log(LOG_DEBUG, "OpenRecordedStream> opening stream: " + results[2]);		// log password safe path of client if available
		else
			XBMC->Log(LOG_DEBUG, "OpenRecordedStream> opening stream: " + _streamFileName);	

		_streamFile = XBMC->OpenFile(_streamFileName.c_str(), 0);	// open the video file for streaming, same handle

		if (!_streamFile)	// something went wrong
		{
			CStdString lastError;
#ifdef TARGET_WINDOWS
			int errorVal = GetLastError();
			lastError.Format("Error opening stream file, Win32 error code: %d", errorVal);
#else
			lastError.Format("Error opening stream file");
#endif
			XBMC->Log(LOG_ERROR, lastError.c_str());						// log more info on error
			_socketClient.GetBool("StreamStartError|" + _streamFileName);	// tell server stream did not start
			return false;
		}
		else
			XBMC->Log(LOG_DEBUG, "OpenRecordedStream> stream file opened successfully");

		_lostStream = false;						// stream is open
		_lastStreamSize = 0;
		_isStreamFileGrowing = true;

		return true;								// if we got to here, stream started ok
	}
}

PVR_ERROR Pvr2Wmc::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
	if (IsServerDown())
		return PVR_ERROR_SERVER_ERROR;

	if (!g_bSignalEnable || _discardSignalStatus)
	{
		return PVR_ERROR_NO_ERROR;
	}

	static PVR_SIGNAL_STATUS cachedSignalStatus;

	// Only send request to backend every N times
	if (_signalStatusCount-- <= 0)
	{
      // Reset count to throttle value
		_signalStatusCount = g_signalThrottle;

		CStdString command;
		command.Format("SignalStatus");

		vector<CStdString> results = _socketClient.GetVector(command);					// get results from server

		// strDeviceName, strDeviceStatus, strProvider, strService, strMux
		// iSignal, dVideoBitrate, dAudioBitrate, Error

		if (isServerError(results))							// did the server do it?
		{
			return PVR_ERROR_SERVER_ERROR;					// report "no error" so our error shows up
		}
		else
		{
			if (results.size() >= 9)
			{
				memset(&cachedSignalStatus, 0, sizeof(cachedSignalStatus));
				snprintf(signalStatus.strAdapterName, sizeof(signalStatus.strAdapterName), results[0]);
				snprintf(signalStatus.strAdapterStatus, sizeof(signalStatus.strAdapterStatus), results[1]);
				snprintf(signalStatus.strProviderName, sizeof(signalStatus.strProviderName), results[2]);
				snprintf(signalStatus.strServiceName, sizeof(signalStatus.strServiceName), results[3]);
				snprintf(signalStatus.strMuxName, sizeof(signalStatus.strMuxName), results[4]);
				signalStatus.iSignal = atoi(results[5]) * 655.35;
				signalStatus.dVideoBitrate = atof(results[6]);
				signalStatus.dAudioBitrate = atof(results[7]);
			
				bool error = atoi(results[8]) == 1;
				if (error)
				{
					// Backend indicates it can't provide SignalStatus for this channel
					// Set flag to discard further attempts until a channel change
					_discardSignalStatus = true;
				}
			}
		}
	}
	
	signalStatus = cachedSignalStatus;
	return PVR_ERROR_NO_ERROR;
}
