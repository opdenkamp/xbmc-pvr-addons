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

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "pvr2wmc.h"
#include "platform/util/util.h"

using namespace std;
using namespace ADDON;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

#define DEFAULT_PORT 9080
#define DEFAULT_SIGNAL_ENABLE false
#define DEFAULT_SIGNAL_THROTTLE 10
#define DEFAULT_MULTI_RESUME true

Pvr2Wmc*		_wmc			= NULL;
bool			_bCreated       = false;
ADDON_STATUS	_CurStatus      = ADDON_STATUS_UNKNOWN;
bool			_bIsPlaying     = false;
PVR_CHANNEL		_currentChannel;
PVR_MENUHOOK	*menuHook       = NULL;

CStdString		g_strServerName;							// the name of the server to connect to
CStdString		g_strClientName;							// the name of the computer running addon
int				g_port;
bool			g_bSignalEnable;
int				g_signalThrottle;
bool			g_bEnableMultiResume;
CStdString		g_clientOS;									// OS of client, passed to server

/* User adjustable settings are saved here.
* Default values are defined inside client.h
* and exported to the other source files.
*/
CStdString g_strUserPath             = "";
CStdString g_strClientPath           = "";

CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;
CHelper_libXBMC_gui   *GUI            = NULL; 

#define LOCALHOST "127.0.0.1"

extern "C" {

	void ADDON_ReadSettings(void)		// todo: get settings for real
	{
		char buffer[512];

		if (!XBMC)
			return;

		g_strServerName = LOCALHOST;			// either "mediaserver" OR "." / "127.0.0.1"
		g_port = DEFAULT_PORT;
		g_bSignalEnable = DEFAULT_SIGNAL_ENABLE;
		g_signalThrottle = DEFAULT_SIGNAL_THROTTLE;
		g_bEnableMultiResume = DEFAULT_MULTI_RESUME;

		/* Read setting "port" from settings.xml */
		if (!XBMC->GetSetting("port", &g_port))
		{
			XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, using '%i'", DEFAULT_PORT);
		}

		if (XBMC->GetSetting("host", &buffer))
		{ 
			g_strServerName = buffer;
			XBMC->Log(LOG_DEBUG, "Settings: host='%s', port=%i", g_strServerName.c_str(), g_port);
		}
		else
		{
			XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, using '127.0.0.1'");
		}

		if (!XBMC->GetSetting("signal", &g_bSignalEnable))
		{
			XBMC->Log(LOG_ERROR, "Couldn't get 'signal' setting, using '%s'", DEFAULT_SIGNAL_ENABLE);
		}

		if (!XBMC->GetSetting("signal_throttle", &g_signalThrottle))
		{
			XBMC->Log(LOG_ERROR, "Couldn't get 'signal_throttle' setting, using '%s'", DEFAULT_SIGNAL_THROTTLE);
		}
		
		if (!XBMC->GetSetting("multiResume", &g_bEnableMultiResume))
		{
			XBMC->Log(LOG_ERROR, "Couldn't get 'multiResume' setting, using '%s'", DEFAULT_MULTI_RESUME);
		}
		

		// get the name of the computer client is running on
#ifdef TARGET_WINDOWS
		gethostname(buffer, 50); 
		g_strClientName = buffer;		// send this computers name to server

		// get windows version
		OSVERSIONINFO osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		g_clientOS.Format("windows(%d.%d)", osvi.dwMajorVersion, osvi.dwMinorVersion);	// set windows version string
#else
		g_strClientName = "";			// empty string signals to server to display the IP address
		g_clientOS = "";				// set to the client OS name
#endif
	}

	// create this addon (called by xbmc)
	ADDON_STATUS ADDON_Create(void* hdl, void* props)
	{
		if (!hdl || !props)
			return ADDON_STATUS_UNKNOWN;

		PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

		// register the addon
		XBMC = new CHelper_libXBMC_addon;
		if (!XBMC->RegisterMe(hdl))
		{
			SAFE_DELETE(XBMC);
			return ADDON_STATUS_PERMANENT_FAILURE;
		}

		// register gui
		GUI = new CHelper_libXBMC_gui;
		if (!GUI->RegisterMe(hdl))
		{
			SAFE_DELETE(GUI);
			SAFE_DELETE(XBMC);
			return ADDON_STATUS_PERMANENT_FAILURE;
		}

		// register as pvr
		PVR = new CHelper_libXBMC_pvr;	
		if (!PVR->RegisterMe(hdl))
		{
			SAFE_DELETE(PVR);
			SAFE_DELETE(GUI);
			SAFE_DELETE(XBMC);
			return ADDON_STATUS_PERMANENT_FAILURE;
		}

		XBMC->Log(LOG_DEBUG, "%s - Creating the PVR-WMC add-on", __FUNCTION__);

		_CurStatus     = ADDON_STATUS_UNKNOWN;
		g_strUserPath   = pvrprops->strUserPath;
		g_strClientPath = pvrprops->strClientPath;

		ADDON_ReadSettings();

		_wmc = new Pvr2Wmc;								// create interface to ServerWMC
		if (_wmc->IsServerDown())						// check if server is down, if it is shut her down
		{
			SAFE_DELETE(_wmc);
			SAFE_DELETE(PVR);
			SAFE_DELETE(GUI);
			SAFE_DELETE(XBMC);
			_CurStatus = ADDON_STATUS_LOST_CONNECTION;
		}
		else
		{
			_bCreated = true;
			_CurStatus = ADDON_STATUS_OK;

		}
		return _CurStatus;
	}

	// get status of addon's interface to wmc server
	ADDON_STATUS ADDON_GetStatus()
	{
		// check whether we're still connected 
		if (_CurStatus == ADDON_STATUS_OK)
		{
			if (_wmc == NULL)
				_CurStatus = ADDON_STATUS_LOST_CONNECTION;
			else if (_wmc->IsServerDown())
				_CurStatus = ADDON_STATUS_LOST_CONNECTION;
		}

		return _CurStatus;
	}

	void ADDON_Destroy()
	{
		if (_wmc)
			_wmc->UnLoading();
		SAFE_DELETE(PVR);
		SAFE_DELETE(GUI);
		_bCreated = false;
		_CurStatus = ADDON_STATUS_UNKNOWN;
	}

	bool ADDON_HasSettings()
	{
		return true;
	}

	unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
	{
		return 0;
	}

	// Called everytime a setting is changed by the user and to inform AddOn about
	// new setting and to do required stuff to apply it.
	ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
	{
		if (!XBMC)
			return ADDON_STATUS_OK;

		CStdString sName = settingName;

		if (sName == "host")
		{
			CStdString oldName = g_strServerName;
			g_strServerName = (const char*)settingValue;
			//if (g_strServerName == ".")
			//	g_strServerName = LOCALHOST;
			XBMC->Log(LOG_INFO, "Setting 'host' changed from %s to %s", g_strServerName.c_str(), (const char*) settingValue);
			if (oldName != g_strServerName)
				return ADDON_STATUS_NEED_RESTART;
		}

		return ADDON_STATUS_OK;
	}

	void ADDON_Stop()
	{
	}

	void ADDON_FreeSettings()
	{
	}

	void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
	{
	}

	/***********************************************************
	* PVR Client AddOn specific public library functions
	***********************************************************/

	const char* GetPVRAPIVersion(void)
	{
		static const char *strApiVersion = XBMC_PVR_API_VERSION;
		return strApiVersion;
	}

	const char* GetMininumPVRAPIVersion(void)
	{
		static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
		return strMinApiVersion;
	}
	
	const char* GetGUIAPIVersion(void)
	{
	  static const char *strGuiApiVersion = XBMC_GUI_API_VERSION;
	  return strGuiApiVersion;
	}

	const char* GetMininumGUIAPIVersion(void)
	{
	  static const char *strMinGuiApiVersion = XBMC_GUI_MIN_API_VERSION;
	  return strMinGuiApiVersion;
	}

	PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
	{
		pCapabilities->bSupportsEPG                = true;
		pCapabilities->bSupportsRecordings         = true;
		pCapabilities->bSupportsTimers             = true;
		pCapabilities->bSupportsTV                 = true;
		pCapabilities->bSupportsRadio              = false;
		pCapabilities->bSupportsChannelGroups      = true;
		pCapabilities->bHandlesInputStream         = true;
		pCapabilities->bHandlesDemuxing            = false;
		pCapabilities->bSupportsChannelScan        = false;
		pCapabilities->bSupportsLastPlayedPosition = g_bEnableMultiResume;

		return PVR_ERROR_NO_ERROR;
	}

	const char *GetBackendName(void)
	{
		static const char *strBackendName = "ServerWMC";
		return strBackendName;
	}

	const char *GetBackendVersion(void)
	{
		if (_wmc)
			return _wmc->GetBackendVersion();
		else
			return "0.0";
	}

	const char *GetConnectionString(void)
	{
		static CStdString strConnectionString = "connected";
		return strConnectionString.c_str();
	}

	PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
	{
		*iTotal = 1024 * 1024 * 1024;
		*iUsed  = 0;
		return PVR_ERROR_NO_ERROR;
	}

	PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
	{
		if (_wmc)
			return _wmc->GetEPGForChannel(handle, channel, iStart, iEnd);

		return PVR_ERROR_SERVER_ERROR;
	}

	int GetCurrentClientChannel(void)
	{
		return _currentChannel.iUniqueId;
	}

	PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
	{
		return PVR_ERROR_NOT_IMPLEMENTED;
	}

	PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
	{
		if (_wmc)
			return _wmc->SignalStatus(signalStatus);

		return PVR_ERROR_NO_ERROR;
	}

	// channel functions
	int GetChannelsAmount(void)
	{
		if (_wmc)
			return _wmc->GetChannelsAmount();

		return -1;
	}

	PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
	{
		if (_wmc)
			return _wmc->GetChannels(handle, bRadio);

		return PVR_ERROR_SERVER_ERROR;
	}

	int GetChannelGroupsAmount(void)
	{
		if (_wmc)
			return _wmc->GetChannelGroupsAmount();

		return -1;
	}

	PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
	{
		if (_wmc)
			return _wmc->GetChannelGroups(handle, bRadio);

		return PVR_ERROR_SERVER_ERROR;
	}

	PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
	{
		if (_wmc)
			return _wmc->GetChannelGroupMembers(handle, group);

		return PVR_ERROR_SERVER_ERROR;
	}


	// timer functions
	int GetTimersAmount(void) 
	{ 
		if (_wmc)
			return _wmc->GetTimersAmount();

		return PVR_ERROR_SERVER_ERROR;
	}

	PVR_ERROR GetTimers(ADDON_HANDLE handle) 
	{ 
		if (_wmc)
			return _wmc->GetTimers(handle);

		return PVR_ERROR_SERVER_ERROR;
	}

	PVR_ERROR AddTimer(const PVR_TIMER &timer) 
	{ 
		if (_wmc)
			return _wmc->AddTimer(timer);

		return PVR_ERROR_NO_ERROR; 
	}

	PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
	{ 
		if (_wmc)
			return _wmc->AddTimer(timer);
		return PVR_ERROR_NO_ERROR;
	}

	PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete) 
	{
		if (_wmc)
			return _wmc->DeleteTimer(timer, bForceDelete);
		return PVR_ERROR_NO_ERROR;
	}

	// recording file functions
	PVR_ERROR GetRecordings(ADDON_HANDLE handle) 
	{ 
		if (_wmc)
			return _wmc->GetRecordings(handle);
		return PVR_ERROR_NO_ERROR;
	}

	int GetRecordingsAmount(void) 
	{ 
		if (_wmc)
			return _wmc->GetRecordingsAmount();

		return -1;
	}

	PVR_ERROR RenameRecording(const PVR_RECORDING &recording) 
	{ 
		if (_wmc)
			return _wmc->RenameRecording(recording);
		return PVR_ERROR_NOT_IMPLEMENTED; 
	}

	//#define TRUESWITCH

	bool SwitchChannel(const PVR_CHANNEL &channel)
	{
		//CloseLiveStream();				
#ifdef TRUESWITCH

		if (_wmc)
		{ 
			return _wmc->SwitchChannel(channel);
		} 
		return false;
#else
		return OpenLiveStream(channel);		// this will perform the closing of a current live stream itself
#endif
	}

	// live stream functions
	bool OpenLiveStream(const PVR_CHANNEL &channel)
	{
		if (_wmc)
		{
			//CloseLiveStream();
			if (_wmc->OpenLiveStream(channel))
			{
				_bIsPlaying = true;
				return true;
			}
		}
		return false;
	}

	int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) 
	{ 
		if (_wmc)
		{
			return _wmc->ReadLiveStream(pBuffer, iBufferSize);
		}
		return -1;
	}

	void CloseLiveStream(void)
	{
		_bIsPlaying = false;
		if (_wmc)
		{
			_wmc->CloseLiveStream();
		}
	}

	long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) 
	{ 
		if (_wmc)
			return _wmc->SeekLiveStream(iPosition, iWhence);
		else
			return -1; 
	}

	long long PositionLiveStream(void) 
	{ 
		if (_wmc)
			return _wmc->PositionLiveStream();
		else
			return -1; 
	}

	long long LengthLiveStream(void) 
	{ 
		if (_wmc)
			return _wmc->LengthLiveStream();
		else
			return -1; 
	}

	void PauseStream(bool bPaused)
	{
		if (_wmc)
			return _wmc->PauseStream(bPaused);
	}

	bool CanPauseStream(void) 
	{
		return true; 
	}

	bool CanSeekStream(void) 
	{ 
		return true; 
	}

	// recorded stream functions (other "Open" these just call the live stream functions)
	bool OpenRecordedStream(const PVR_RECORDING &recording) 
	{ 
		if (_wmc)
		{
			CloseLiveStream();
			if (_wmc->OpenRecordedStream(recording))
			{
				_bIsPlaying = true;
				return true;
			}
		}
		return false; 
	}

	int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) 
	{ 
		if (_wmc)
		{
			return _wmc->ReadLiveStream(pBuffer, iBufferSize);
		}
		return -1;
	}

	void CloseRecordedStream(void) 
	{
		_bIsPlaying = false;
		if (_wmc)
		{
			_wmc->CloseLiveStream();
		}
	}

	long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) 
	{ 
		if (_wmc)
			return _wmc->SeekLiveStream(iPosition, iWhence);
		else
			return -1; 
	}

	long long PositionRecordedStream(void) 
	{ 
		if (_wmc)
			return _wmc->PositionLiveStream();
		else
			return -1; 
	}

	long long LengthRecordedStream(void) 
	{ 
		if (_wmc)
			return _wmc->LengthLiveStream();
		else
			return -1; 
	}

	// recorded file functions
	PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) 
	{ 
		if (_wmc)
			return _wmc->DeleteRecording(recording);
		return PVR_ERROR_NO_ERROR; 
	}


	PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) 
	{ 
		if (_wmc && g_bEnableMultiResume)
			return _wmc->SetRecordingLastPlayedPosition(recording, lastplayedposition);
		return PVR_ERROR_NOT_IMPLEMENTED; 
	}
	int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) 
	{ 
		if (_wmc && g_bEnableMultiResume)
			return _wmc->GetRecordingLastPlayedPosition(recording);
		return -1; 
	}


	PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item)
	{ 
		return PVR_ERROR_NOT_IMPLEMENTED;
	}

	/** UNUSED API FUNCTIONS */
	PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
	PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
	PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
	PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
	PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel)  {  return PVR_ERROR_NOT_IMPLEMENTED;  }
	PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
	void DemuxReset(void) {}
	void DemuxFlush(void) {}
	PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
	void DemuxAbort(void) {}
	DemuxPacket* DemuxRead(void) { return NULL; }
	unsigned int GetChannelSwitchDelay(void) { return 0; }
	const char * GetLiveStreamURL(const PVR_CHANNEL &channel)  {  return "";  }
	bool SeekTime(int,bool,double*) { return false; }
	void SetSpeed(int) {};
   
	PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
	time_t GetPlayingTime() { return 0; }
	time_t GetBufferTimeStart() { return 0; }
	time_t GetBufferTimeEnd() { return 0; }

}
