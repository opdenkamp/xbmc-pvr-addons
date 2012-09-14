#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
//$(SolutionDir)\..\..\lib\cmyth\Win32\$(Configuration)\vs2010\libcmyth.lib

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _LINUX
#include "platform/windows/dlfcn-win32.h"
#define LIBCMYTH_DLL "\\pvr.mythtv.cmyth\\libcmyth.dll"
#else
#include <dlfcn.h>
#if defined(__APPLE__)
#if defined(__POWERPC__)
#define LIBCMYTH_DLL "/../system/libcmyth-powerpc-osx.so"
#elif defined(__arm__)
#define LIBCMYTH_DLL "/../system/libcmyth-arm-osx.so"
#else
#define LIBCMYTH_DLL "/../system/libcmyth-x86-osx.so"
#endif
#elif defined(__x86_64__)
#define LIBCMYTH_DLL "/../system/libcmyth-x86_64-linux.so"
#elif defined(_POWERPC)
#define LIBCMYTH_DLL "/../system/libcmyth-powerpc-linux.so"
#elif defined(_POWERPC64)
#define LIBCMYTH_DLL "/../system/libcmyth-powerpc64-linux.so"
#elif defined(_ARMEL)
#define LIBCMYTH_DLL "/../system/libcmyth-arm.so"
#else /* !__x86_64__ && !__powerpc__ */
#define LIBCMYTH_DLL "/../system/libcmyth-i486-linux.so"
#endif /* __x86_64__ */
#endif /* _LINUX */

#ifdef __APPLE__ 
#include <sys/time.h>
#else
#include <time.h>
#endif

#if defined(DEBUG)
#define ref_alloc(l) (__ref_alloc((l), __FILE__, __FUNCTION__, __LINE__))
#else
#define ref_alloc(l) (__ref_alloc((l), (char *)0, (char *)0, 0))
#endif

/*
 * -----------------------------------------------------------------
 * Types
 * -----------------------------------------------------------------
 */

typedef void (*ref_destroy_t)(void *p);

struct cmyth_conn;
typedef struct cmyth_conn *cmyth_conn_t;

/* Sergio: Added to support the new livetv protocol */
struct cmyth_livetv_chain;
typedef struct cmyth_livetv_chain *cmyth_livetv_chain_t;

struct cmyth_recorder;
typedef struct cmyth_recorder *cmyth_recorder_t;

struct cmyth_ringbuf;
typedef struct cmyth_ringbuf *cmyth_ringbuf_t;

struct cmyth_rec_num;
typedef struct cmyth_rec_num *cmyth_rec_num_t;

struct cmyth_posmap;
typedef struct cmyth_posmap *cmyth_posmap_t;

struct cmyth_proginfo;
typedef struct cmyth_proginfo *cmyth_proginfo_t;

struct cmyth_database;
typedef struct cmyth_database *cmyth_database_t;


typedef enum {
	CHANNEL_DIRECTION_UP = 0,
	CHANNEL_DIRECTION_DOWN = 1,
	CHANNEL_DIRECTION_FAVORITE = 2,
	CHANNEL_DIRECTION_SAME = 4
} cmyth_channeldir_t;

typedef enum {
	ADJ_DIRECTION_UP = 1,
	ADJ_DIRECTION_DOWN = 0
} cmyth_adjdir_t;

typedef enum {
	BROWSE_DIRECTION_SAME = 0,
	BROWSE_DIRECTION_UP = 1,
	BROWSE_DIRECTION_DOWN = 2,
	BROWSE_DIRECTION_LEFT = 3,
	BROWSE_DIRECTION_RIGHT = 4,
	BROWSE_DIRECTION_FAVORITE = 5
} cmyth_browsedir_t;

typedef enum {
	WHENCE_SET = 0,
	WHENCE_CUR = 1,
	WHENCE_END = 2
} cmyth_whence_t;

typedef enum {
	CMYTH_EVENT_UNKNOWN = 0,
	CMYTH_EVENT_CLOSE = 1,
	CMYTH_EVENT_RECORDING_LIST_CHANGE,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE,
	CMYTH_EVENT_SCHEDULE_CHANGE,
	CMYTH_EVENT_DONE_RECORDING,
	CMYTH_EVENT_QUIT_LIVETV,
	CMYTH_EVENT_WATCH_LIVETV,
	CMYTH_EVENT_LIVETV_CHAIN_UPDATE,
	CMYTH_EVENT_SIGNAL,
	CMYTH_EVENT_ASK_RECORDING,
	CMYTH_EVENT_SYSTEM_EVENT,
	CMYTH_EVENT_UPDATE_FILE_SIZE,
	CMYTH_EVENT_GENERATED_PIXMAP,
	CMYTH_EVENT_CLEAR_SETTINGS_CACHE
} cmyth_event_t;

#define CMYTH_NUM_SORTS 2
typedef enum {
	MYTHTV_SORT_DATE_RECORDED = 0,
	MYTHTV_SORT_ORIGINAL_AIRDATE
} cmyth_proglist_sort_t;

struct cmyth_timestamp;
typedef struct cmyth_timestamp *cmyth_timestamp_t;

struct cmyth_keyframe;
typedef struct cmyth_keyframe *cmyth_keyframe_t;

struct cmyth_freespace;
typedef struct cmyth_freespace *cmyth_freespace_t;

struct cmyth_proglist;
typedef struct cmyth_proglist *cmyth_proglist_t;

struct cmyth_file;
typedef struct cmyth_file *cmyth_file_t;

struct cmyth_commbreak {
        long long start_mark;
        long long start_offset;
        long long end_mark;
        long long end_offset;
};
typedef struct cmyth_commbreak *cmyth_commbreak_t;

struct cmyth_commbreaklist {
        cmyth_commbreak_t *commbreak_list;
        long commbreak_count;
};
typedef struct cmyth_commbreaklist *cmyth_commbreaklist_t;

/* Sergio: Added to support the tvguide functionality */

struct cmyth_channel;
typedef struct cmyth_channel *cmyth_channel_t;

struct cmyth_chanlist;
typedef struct cmyth_chanlist *cmyth_chanlist_t;

struct cmyth_tvguide_progs;
typedef struct cmyth_tvguide_progs *cmyth_tvguide_progs_t;

/* fetzerch: Added to support querying of free inputs (is tunable on) */
struct cmyth_input {
        char *inputname;
        unsigned long sourceid;
        unsigned long inputid;
        unsigned long cardid;
        unsigned long multiplexid;
        unsigned long livetvorder; /* new in V71 */
};
typedef struct cmyth_input *cmyth_input_t;

struct cmyth_inputlist {
        cmyth_input_t *input_list;
        long input_count;
};
typedef struct cmyth_inputlist *cmyth_inputlist_t;

typedef struct cmyth_program {
	int chanid;
	char callsign[30];
	char name[84];
	int sourceid;
	char title[150];
	char subtitle[150];
	char description[280];
	time_t starttime;
	time_t endtime;
	char programid[30];
	char seriesid[24];
	char category[84];
	int recording;
	int rec_status;
	int channum;
	int event_flags;
	int startoffset;
	int endoffset;
  cmyth_program(){memset(this,0,sizeof(cmyth_program));}
}cmyth_program_t;

typedef struct cmyth_recgrougs {
	char recgroups[33];
}cmyth_recgroups_t;

/*typedef enum {
	RS_DELETED = -5,
	RS_STOPPED = -4,
	RS_RECORDED = -3,
	RS_RECORDING = -2,
	RS_WILL_RECORD = -1,
	RS_DONT_RECORD = 1,
	RS_PREVIOUS_RECORDING = 2,
	RS_CURRENT_RECORDING = 3,
	RS_EARLIER_RECORDING = 4,
	RS_TOO_MANY_RECORDINGS = 5,
	RS_CANCELLED = 6,
	RS_CONFLICT = 7,
	RS_LATER_SHOWING = 8,
	RS_REPEAT = 9,
	RS_LOW_DISKSPACE = 11,
	RS_TUNER_BUSY = 12
} cmyth_proginfo_rec_status_t;*/
typedef enum {
    RS_TUNING = -10,
    RS_FAILED = -9,
    RS_TUNER_BUSY = -8,
    RS_LOW_DISKSPACE = -7,
    RS_CANCELLED = -6,
    RS_MISSED = -5,
    RS_ABORTED = -4,
    RS_RECORDED = -3,
    RS_RECORDING = -2,
    RS_WILL_RECORD = -1,
    RS_UNKNOWN = 0,
    RS__DONT_RECORD = 1,
    RS_PREVIOUS_RECORDING = 2,
    RS_CURRENT_RECORDING = 3,
    RS_EARLIER_SHOWING = 4,
    RS_TOO_MANY_RECORDINGS = 5,
    RS_NOT_LISTED = 6,
    RS_CONFLICT = 7,
    RS_LATER_SHOWING = 8,
    RS_REPEAT = 9,
    RS_INACTIVE = 10,
    RS_NEVER_RECORD = 11,
    RS_OFFLINE = 12,
    RS_OTHER_SHOWING = 13
} cmyth_proginfo_rec_status_t;

/*From libmyth 0.24 Note difference from above:
typedef enum RecStatusTypes {
    rsTuning = -10,
    rsFailed = -9,
    rsTunerBusy = -8,
    rsLowDiskSpace = -7,
    rsCancelled = -6,
    rsMissed = -5,
    rsAborted = -4,
    rsRecorded = -3,
    rsRecording = -2,
    rsWillRecord = -1,
    rsUnknown = 0,
    rsDontRecord = 1,
    rsPreviousRecording = 2,
    rsCurrentRecording = 3,
    rsEarlierShowing = 4,
    rsTooManyRecordings = 5,
    rsNotListed = 6,
    rsConflict = 7,
    rsLaterShowing = 8,
    rsRepeat = 9,
    rsInactive = 10,
    rsNeverRecord = 11,
    rsOffLine = 12,
    rsOtherShowing = 13
} RecStatusType;
*/
struct cmyth_timer;
typedef struct cmyth_timer* cmyth_timer_t;

struct cmyth_timerlist;
typedef struct cmyth_timerlist* cmyth_timerlist_t;

typedef struct cmyth_channelgroups {
	char channelgroup[65];
  unsigned int ID; 
} cmyth_channelgroups_t;

typedef struct  cmyth_rec {
  int recid;
  int sourceid;
} cmyth_rec_t;

typedef struct cmyth_recprofile{
int id;
char name[128];
char cardtype[32];
} cmyth_recprofile_t;

struct cmyth_storagegroup_filelist;
typedef struct cmyth_storagegroup_filelist* cmyth_storagegroup_filelist_t;

struct cmyth_storagegroup_file;
typedef struct cmyth_storagegroup_file* cmyth_storagegroup_file_t;

#define CMYTH_DBG_NONE  -1
#define CMYTH_DBG_ERROR  0
#define CMYTH_DBG_WARN   1
#define CMYTH_DBG_INFO   2
#define CMYTH_DBG_DETAIL 3
#define CMYTH_DBG_DEBUG  4
#define CMYTH_DBG_PROTO  5
#define CMYTH_DBG_ALL    6

#define PROGRAM_ADJUST  3600

class CHelper_libcmyth
{
public:
  CHelper_libcmyth()
  {
    m_libcmyth = NULL;
    m_Handle        = NULL;
  }

  ~CHelper_libcmyth()
  {
    if (m_libcmyth)
    {
      //XBMC_unregister_me();
      dlclose(m_libcmyth);
    }
  }

  bool RegisterMe(void *Handle)
  {
    m_Handle = Handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += LIBCMYTH_DLL;

    m_libcmyth = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libcmyth == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    DbgLevel      = (void (*)(int l))
dlsym(m_libcmyth, "cmyth_dbg_level");
if (DbgLevel == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DbgAll      = (void (*)(void))
dlsym(m_libcmyth, "cmyth_dbg_all");
if (DbgAll == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DbgNone      = (void (*)(void))
dlsym(m_libcmyth, "cmyth_dbg_none");
if (DbgNone == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Dbg      = (void (*)(int level, char* fmt, ...))
dlsym(m_libcmyth, "cmyth_dbg");
if (Dbg == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    SetDbgMsgcallback      = (void (*)(void (* msgcb)(int level,char* )))
dlsym(m_libcmyth, "cmyth_set_dbg_msgcallback");
if (SetDbgMsgcallback == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnConnectCtrl      = (cmyth_conn_t (*)(char* server, unsigned short port, unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_ctrl");
if (ConnConnectCtrl == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnReconnectCtrl      = (int (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_conn_reconnect_ctrl");
if (ConnReconnectCtrl == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnConnectEvent      = (cmyth_conn_t (*)(char* server,  unsigned short port,  unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_event");
if (ConnConnectEvent == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnReconnectEvent    = (int (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_reconnect_event");
if (ConnReconnectEvent == NULL)    { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnConnectFile      = (cmyth_file_t (*)(cmyth_proginfo_t prog, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_file");
if (ConnConnectFile == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnConnectPath      = (cmyth_file_t (*)(char* path, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf,char* sgToGetFrom))
dlsym(m_libcmyth, "cmyth_conn_connect_path");
if (ConnConnectPath == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnConnectRing      = (int (*)(cmyth_recorder_t rec, unsigned buflen,int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_ring");
if (ConnConnectRing == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnConnectRecorder      = (int (*)(cmyth_recorder_t rec, unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_recorder");
if (ConnConnectRecorder == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnCheckBlock      = (int (*)(cmyth_conn_t conn, unsigned long size))
dlsym(m_libcmyth, "cmyth_conn_check_block");
if (ConnCheckBlock == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetRecorderFromNum      = (cmyth_recorder_t (*)(cmyth_conn_t conn, int num))
dlsym(m_libcmyth, "cmyth_conn_get_recorder_from_num");
if (ConnGetRecorderFromNum == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetFreeRecorder      = (cmyth_recorder_t (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_free_recorder");
if (ConnGetFreeRecorder == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetFreespace      = (int (*)(cmyth_conn_t control, long long* total, long long* used))
dlsym(m_libcmyth, "cmyth_conn_get_freespace");
if (ConnGetFreespace == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnHung      = (int (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_conn_hung");
if (ConnHung == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetFreeRecorderCount      = (int (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_free_recorder_count");
if (ConnGetFreeRecorderCount == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetProtocolVersion      = (int (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_protocol_version");
if (ConnGetProtocolVersion == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetSetting      = (char* (*)(cmyth_conn_t conn,const char* hostname, const char* setting))
dlsym(m_libcmyth, "cmyth_conn_get_setting");
if (ConnGetSetting == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnSetSetting      = (int (*)(cmyth_conn_t conn,const char* hostname, const char* setting, const char* value))
dlsym(m_libcmyth, "cmyth_conn_set_setting");
if (ConnSetSetting == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetBackendHostname      = (char* (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_backend_hostname");
if (ConnGetBackendHostname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ConnGetClientHostname      = (char* (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_client_hostname");
if (ConnGetClientHostname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    EventGet      = (cmyth_event_t (*)(cmyth_conn_t conn, char* data, int len))
dlsym(m_libcmyth, "cmyth_event_get");
if (EventGet == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    EventSelect      = (int (*)(cmyth_conn_t conn, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_event_select");
if (EventSelect == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderCreate      = (cmyth_recorder_t (*)(void))
dlsym(m_libcmyth, "cmyth_recorder_create");
if (RecorderCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderDup      = (cmyth_recorder_t (*)(cmyth_recorder_t p))
dlsym(m_libcmyth, "cmyth_recorder_dup");
if (RecorderDup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderIsRecording      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_is_recording");
if (RecorderIsRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetFramerate      = (int (*)(cmyth_recorder_t rec,double* rate))
dlsym(m_libcmyth, "cmyth_recorder_get_framerate");
if (RecorderGetFramerate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetFramesWritten      = (long long (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_frames_written");
if (RecorderGetFramesWritten == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetFreeSpace      = (long long (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_free_space");
if (RecorderGetFreeSpace == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetKeyframePos      = (long long (*)(cmyth_recorder_t rec, unsigned long keynum))
dlsym(m_libcmyth, "cmyth_recorder_get_keyframe_pos");
if (RecorderGetKeyframePos == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetPositionMap      = (cmyth_posmap_t (*)(cmyth_recorder_t rec,unsigned long start,unsigned long end))
dlsym(m_libcmyth, "cmyth_recorder_get_position_map");
if (RecorderGetPositionMap == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetRecording      = (cmyth_proginfo_t (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_recording");
if (RecorderGetRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderStopPlaying      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_stop_playing");
if (RecorderStopPlaying == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderFrontendReady      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_frontend_ready");
if (RecorderFrontendReady == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderCancelNextRecording      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_cancel_next_recording");
if (RecorderCancelNextRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderPause      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_pause");
if (RecorderPause == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderFinishRecording      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_finish_recording");
if (RecorderFinishRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderToggleChannelFavorite      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_toggle_channel_favorite");
if (RecorderToggleChannelFavorite == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderChangeChannel      = (int (*)(cmyth_recorder_t rec, cmyth_channeldir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_channel");
if (RecorderChangeChannel == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderSetChannel      = (int (*)(cmyth_recorder_t rec,char* channame))
dlsym(m_libcmyth, "cmyth_recorder_set_channel");
if (RecorderSetChannel == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderChangeColor      = (int (*)(cmyth_recorder_t rec, cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_color");
if (RecorderChangeColor == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderChangeBrightness      = (int (*)(cmyth_recorder_t rec, cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_brightness");
if (RecorderChangeBrightness == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderChangeContrast      = (int (*)(cmyth_recorder_t rec,  cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_contrast");
if (RecorderChangeContrast == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderChangeHue      = (int (*)(cmyth_recorder_t rec,  cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_hue");
if (RecorderChangeHue == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderCheckChannel      = (int (*)(cmyth_recorder_t rec,char* channame))
dlsym(m_libcmyth, "cmyth_recorder_check_channel");
if (RecorderCheckChannel == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderCheckChannelPrefix      = (int (*)(cmyth_recorder_t rec, char* channame))
dlsym(m_libcmyth, "cmyth_recorder_check_channel_prefix");
if (RecorderCheckChannelPrefix == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetCurProginfo      = (cmyth_proginfo_t (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_cur_proginfo");
if (RecorderGetCurProginfo == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetNextProginfo      = (cmyth_proginfo_t (*)(cmyth_recorder_t rec,cmyth_proginfo_t current,cmyth_browsedir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_get_next_proginfo");
if (RecorderGetNextProginfo == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetInputName      = (int (*)(cmyth_recorder_t rec, char* name, unsigned len))
dlsym(m_libcmyth, "cmyth_recorder_get_input_name");
if (RecorderGetInputName == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderSeek      = (long long (*)(cmyth_recorder_t rec,  long long pos,  cmyth_whence_t whence,  long long curpos))
dlsym(m_libcmyth, "cmyth_recorder_seek");
if (RecorderSeek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderSpawnChainLivetv      = (int (*)(cmyth_recorder_t rec, char* channame))
dlsym(m_libcmyth, "cmyth_recorder_spawn_chain_livetv");
if (RecorderSpawnChainLivetv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderSpawnLivetv      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_spawn_livetv");
if (RecorderSpawnLivetv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderStartStream      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_start_stream");
if (RecorderStartStream == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderEndStream      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_end_stream");
if (RecorderEndStream == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetFilename      = (char* (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_filename");
if (RecorderGetFilename == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderStopLivetv      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_stop_livetv");
if (RecorderStopLivetv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderDoneRingbuf      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_done_ringbuf");
if (RecorderDoneRingbuf == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecorderGetRecorderId      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_recorder_id");
if (RecorderGetRecorderId == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvChainCreate      = (cmyth_livetv_chain_t (*)(char* chainid))
dlsym(m_libcmyth, "cmyth_livetv_chain_create");
if (LivetvChainCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvChainDuration      = (long long (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_livetv_chain_duration");
if (LivetvChainDuration == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvChainSwitch      = (int (*)(cmyth_recorder_t rec, int dir))
dlsym(m_libcmyth, "cmyth_livetv_chain_switch");
if (LivetvChainSwitch == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvChainSwitchLast      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_livetv_chain_switch_last");
if (LivetvChainSwitchLast == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvChainUpdate      = (int (*)(cmyth_recorder_t rec, char* chainid,int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_livetv_chain_update");
if (LivetvChainUpdate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    SpawnLiveTv      = (cmyth_recorder_t (*)(cmyth_recorder_t rec,unsigned buflen,int tcp_rcvbuf,  void (* prog_update_callback)(cmyth_proginfo_t),char** err, char* channame))
dlsym(m_libcmyth, "cmyth_spawn_live_tv");
if (SpawnLiveTv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvChainSetup      = (cmyth_recorder_t (*)(cmyth_recorder_t old_rec, int tcp_rcvbuf, void (* prog_update_callback)(cmyth_proginfo_t)))
dlsym(m_libcmyth, "cmyth_livetv_chain_setup");
if (LivetvChainSetup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvGetBlock      = (int (*)(cmyth_recorder_t rec, char* buf, unsigned long len))
dlsym(m_libcmyth, "cmyth_livetv_get_block");
if (LivetvGetBlock == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvSelect      = (int (*)(cmyth_recorder_t rec, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_livetv_select");
if (LivetvSelect == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvRequestBlock      = (int (*)(cmyth_recorder_t rec, unsigned long len))
dlsym(m_libcmyth, "cmyth_livetv_request_block");
if (LivetvRequestBlock == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvSeek      = (long long (*)(cmyth_recorder_t rec,long long offset, int whence))
dlsym(m_libcmyth, "cmyth_livetv_seek");
if (LivetvSeek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvRead      = (int (*)(cmyth_recorder_t rec,  char* buf,  unsigned long len))
dlsym(m_libcmyth, "cmyth_livetv_read");
if (LivetvRead == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    LivetvKeepRecording      = (int (*)(cmyth_recorder_t rec, cmyth_database_t db, int keep))
dlsym(m_libcmyth, "cmyth_livetv_keep_recording");
if (LivetvKeepRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DatabaseInit      = (cmyth_database_t (*)(char* host, char* db_name, char* user, char* pass))
dlsym(m_libcmyth, "cmyth_database_init");
if (DatabaseInit == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DatabaseClose     = (void (*)(cmyth_database_t db))
dlsym(m_libcmyth, "cmyth_database_close");
if (DatabaseClose == NULL)     { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DatabaseSetHost      = (int (*)(cmyth_database_t db, char* host))
dlsym(m_libcmyth, "cmyth_database_set_host");
if (DatabaseSetHost == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DatabaseSetUser      = (int (*)(cmyth_database_t db, char* user))
dlsym(m_libcmyth, "cmyth_database_set_user");
if (DatabaseSetUser == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DatabaseSetPass      = (int (*)(cmyth_database_t db, char* pass))
dlsym(m_libcmyth, "cmyth_database_set_pass");
if (DatabaseSetPass == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DatabaseSetName      = (int (*)(cmyth_database_t db, char* name))
dlsym(m_libcmyth, "cmyth_database_set_name");
if (DatabaseSetName == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    SetWatchedStatusMysql      = (int (*)(cmyth_database_t db, cmyth_proginfo_t prog, int watchedStat))
dlsym(m_libcmyth, "cmyth_set_watched_status_mysql");
if (SetWatchedStatusMysql == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufPathname      = (char* (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_ringbuf_pathname");
if (RingbufPathname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufCreate      = (cmyth_ringbuf_t (*)(void))
dlsym(m_libcmyth, "cmyth_ringbuf_create");
if (RingbufCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufSetup      = (cmyth_recorder_t (*)(cmyth_recorder_t old_rec))
dlsym(m_libcmyth, "cmyth_ringbuf_setup");
if (RingbufSetup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufRequestBlock      = (int (*)(cmyth_recorder_t rec, unsigned long len))
dlsym(m_libcmyth, "cmyth_ringbuf_request_block");
if (RingbufRequestBlock == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufSelect      = (int (*)(cmyth_recorder_t rec, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_ringbuf_select");
if (RingbufSelect == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufGetBlock      = (int (*)(cmyth_recorder_t rec,char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_ringbuf_get_block");
if (RingbufGetBlock == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufSeek      = (long long (*)(cmyth_recorder_t rec, long long offset, int whence))
dlsym(m_libcmyth, "cmyth_ringbuf_seek");
if (RingbufSeek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RingbufRead      = (int (*)(cmyth_recorder_t rec,char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_ringbuf_read");
if (RingbufRead == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecNumCreate      = (cmyth_rec_num_t (*)(void))
dlsym(m_libcmyth, "cmyth_rec_num_create");
if (RecNumCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecNumGet      = (cmyth_rec_num_t (*)(char* host, unsigned short port, unsigned id))
dlsym(m_libcmyth, "cmyth_rec_num_get");
if (RecNumGet == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RecNumString      = (char* (*)(cmyth_rec_num_t rn))
dlsym(m_libcmyth, "cmyth_rec_num_string");
if (RecNumString == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampCreate      = (cmyth_timestamp_t (*)(void))
dlsym(m_libcmyth, "cmyth_timestamp_create");
if (TimestampCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampFromString      = (cmyth_timestamp_t (*)(char* str))
dlsym(m_libcmyth, "cmyth_timestamp_from_string");
if (TimestampFromString == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampFromUnixtime      = (cmyth_timestamp_t (*)(time_t l))
dlsym(m_libcmyth, "cmyth_timestamp_from_unixtime");
if (TimestampFromUnixtime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampToUnixtime      = (time_t (*)(cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_timestamp_to_unixtime");
if (TimestampToUnixtime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampToString      = (int (*)(char* str, cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_timestamp_to_string");
if (TimestampToString == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampToIsostring      = (int (*)(char* str, cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_timestamp_to_isostring");
if (TimestampToIsostring == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampToDisplayString      = (int (*)(char* str, cmyth_timestamp_t ts, int time_format_12))
dlsym(m_libcmyth, "cmyth_timestamp_to_display_string");
if (TimestampToDisplayString == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    DatetimeToString      = (int (*)(char* str, cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_datetime_to_string");
if (DatetimeToString == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimestampCompare      = (int (*)(cmyth_timestamp_t ts1,cmyth_timestamp_t ts2))
dlsym(m_libcmyth, "cmyth_timestamp_compare");
if (TimestampCompare == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    KeyframeCreate      = (cmyth_keyframe_t (*)(void))
dlsym(m_libcmyth, "cmyth_keyframe_create");
if (KeyframeCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    KeyframeString      = (char* (*)(cmyth_keyframe_t kf))
dlsym(m_libcmyth, "cmyth_keyframe_string");
if (KeyframeString == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PosmapCreate      = (cmyth_posmap_t (*)(void))
dlsym(m_libcmyth, "cmyth_posmap_create");
if (PosmapCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoCreate      = (cmyth_proginfo_t (*)(void))
dlsym(m_libcmyth, "cmyth_proginfo_create");
if (ProginfoCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoStopRecording      = (int (*)(cmyth_conn_t control, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_stop_recording");
if (ProginfoStopRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoCheckRecording      = (int (*)(cmyth_conn_t control,  cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_check_recording");
if (ProginfoCheckRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoDeleteRecording      = (int (*)(cmyth_conn_t control,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_delete_recording");
if (ProginfoDeleteRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoForgetRecording      = (int (*)(cmyth_conn_t control,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_forget_recording");
if (ProginfoForgetRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoGetRecorderNum      = (int (*)(cmyth_conn_t control,cmyth_rec_num_t rnum,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_get_recorder_num");
if (ProginfoGetRecorderNum == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoGetFromBasename      = (cmyth_proginfo_t (*)(cmyth_conn_t control, const char* basename))
dlsym(m_libcmyth, "cmyth_proginfo_get_from_basename");
if (ProginfoGetFromBasename == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoTitle      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_title");
if (ProginfoTitle == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoSubtitle      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_subtitle");
if (ProginfoSubtitle == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoDescription      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_description");
if (ProginfoDescription == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoCategory      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_category");
if (ProginfoCategory == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoChanstr      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chanstr");
if (ProginfoChanstr == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoChansign      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chansign");
if (ProginfoChansign == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoChanname      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_channame");
if (ProginfoChanname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoChanId      = (long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chan_id");
if (ProginfoChanId == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoPathname      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_pathname");
if (ProginfoPathname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoSeriesid      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_seriesid");
if (ProginfoSeriesid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoProgramid      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_programid");
if (ProginfoProgramid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoRecordid      = (unsigned long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_recordid");
if (ProginfoRecordid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoPriority      = (long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_priority");
if (ProginfoPriority == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoStars      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_stars");
if (ProginfoStars == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoRecStart      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_rec_start");
if (ProginfoRecStart == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoRecEnd      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_rec_end");
if (ProginfoRecEnd == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoOriginalairdate      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_originalairdate");
if (ProginfoOriginalairdate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoRecStatus      = (cmyth_proginfo_rec_status_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_rec_status");
if (ProginfoRecStatus == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoFlags      = (unsigned long (*)(  cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_flags");
if (ProginfoFlags == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoLength      = (long long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_length");
if (ProginfoLength == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoHost      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_host");
if (ProginfoHost == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoCompare      = (int (*)(cmyth_proginfo_t a, cmyth_proginfo_t b))
dlsym(m_libcmyth, "cmyth_proginfo_compare");
if (ProginfoCompare == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoLengthSec      = (int (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_length_sec");
if (ProginfoLengthSec == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoGetDetail      = (cmyth_proginfo_t (*)(cmyth_conn_t control,  cmyth_proginfo_t p))
dlsym(m_libcmyth, "cmyth_proginfo_get_detail");
if (ProginfoGetDetail == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoStart      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_start");
if (ProginfoStart == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoEnd      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_end");
if (ProginfoEnd == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoCardId      = (long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_card_id");
if (ProginfoCardId == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoRecgroup      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_recgroup");
if (ProginfoRecgroup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoChanicon      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chanicon");
if (ProginfoChanicon == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProginfoYear      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_year");
if (ProginfoYear == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistCreate      = (cmyth_proglist_t (*)(void))
dlsym(m_libcmyth, "cmyth_proglist_create");
if (ProglistCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistGetAllRecorded      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_all_recorded");
if (ProglistGetAllRecorded == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistGetAllPending      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_all_pending");
if (ProglistGetAllPending == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistGetAllScheduled      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_all_scheduled");
if (ProglistGetAllScheduled == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistGetConflicting      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_conflicting");
if (ProglistGetConflicting == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistGetItem      = (cmyth_proginfo_t (*)(cmyth_proglist_t pl,int index))
dlsym(m_libcmyth, "cmyth_proglist_get_item");
if (ProglistGetItem == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistDeleteItem      = (int (*)(cmyth_proglist_t pl,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proglist_delete_item");
if (ProglistDeleteItem == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistGetCount      = (int (*)(cmyth_proglist_t pl))
dlsym(m_libcmyth, "cmyth_proglist_get_count");
if (ProglistGetCount == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ProglistSort      = (int (*)(cmyth_proglist_t pl, int count, cmyth_proglist_sort_t sort))
dlsym(m_libcmyth, "cmyth_proglist_sort");
if (ProglistSort == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileData      = (cmyth_conn_t (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_data");
if (FileData == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileStart      = (unsigned long long (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_start");
if (FileStart == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileLength      = (unsigned long long (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_length");
if (FileLength == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FilePosition      = (unsigned long long (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_position");
if (FilePosition == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    UpdateFileLength      = (int (*)(cmyth_file_t file, unsigned long long newLength))
dlsym(m_libcmyth, "cmyth_update_file_length");
if (UpdateFileLength == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileGetBlock      = (int (*)(cmyth_file_t file, char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_file_get_block");
if (FileGetBlock == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileRequestBlock      = (int (*)(cmyth_file_t file, unsigned long len))
dlsym(m_libcmyth, "cmyth_file_request_block");
if (FileRequestBlock == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileSeek      = (long long (*)(cmyth_file_t file, long long offset, int whence))
dlsym(m_libcmyth, "cmyth_file_seek");
if (FileSeek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileSelect      = (int (*)(cmyth_file_t file, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_file_select");
if (FileSelect == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileSetClosedCallback      = (void (*)(cmyth_file_t file,void (* callback)(cmyth_file_t)))
dlsym(m_libcmyth, "cmyth_file_set_closed_callback");
if (FileSetClosedCallback == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FileRead      = (int (*)(cmyth_file_t file,char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_file_read");
if (FileRead == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelChanid      = (long (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_chanid");
if (ChannelChanid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelChannum      = (long (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_channum");
if (ChannelChannum == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelChannumstr      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_channumstr");
if (ChannelChannumstr == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelCallsign      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_callsign");
if (ChannelCallsign == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelName      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_name");
if (ChannelName == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelIcon      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_icon");
if (ChannelIcon == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelVisible      = (int (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_visible");
if (ChannelVisible == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChanlistGetItem      = (cmyth_channel_t (*)(cmyth_chanlist_t pl, int index))
dlsym(m_libcmyth, "cmyth_chanlist_get_item");
if (ChanlistGetItem == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChanlistGetCount      = (int (*)(cmyth_chanlist_t pl))
dlsym(m_libcmyth, "cmyth_chanlist_get_count");
if (ChanlistGetCount == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FreespaceCreate      = (cmyth_freespace_t (*)(void))
dlsym(m_libcmyth, "cmyth_freespace_create");
if (FreespaceCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetBookmark      = (long long (*)(cmyth_conn_t conn, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_get_bookmark");
if (GetBookmark == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetBookmarkOffset      = (int (*)(cmyth_database_t db, long chanid, long long mark))
dlsym(m_libcmyth, "cmyth_get_bookmark_offset");
if (GetBookmarkOffset == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetBookmarkMark      = (int (*)(cmyth_database_t, cmyth_proginfo_t, long long))
dlsym(m_libcmyth, "cmyth_get_bookmark_mark");
if (GetBookmarkMark == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    SetBookmark      = (int (*)(cmyth_conn_t conn, cmyth_proginfo_t prog,long long bookmark))
dlsym(m_libcmyth, "cmyth_set_bookmark");
if (SetBookmark == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    CommbreaklistCreate      = (cmyth_commbreaklist_t (*)(void))
dlsym(m_libcmyth, "cmyth_commbreaklist_create");
if (CommbreaklistCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    CommbreakCreate      = (cmyth_commbreak_t (*)(void))
dlsym(m_libcmyth, "cmyth_commbreak_create");
if (CommbreakCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetCommbreaklist      = (cmyth_commbreaklist_t (*)(cmyth_conn_t conn, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_get_commbreaklist");
if (GetCommbreaklist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetCutlist      = (cmyth_commbreaklist_t (*)(cmyth_conn_t conn, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_get_cutlist");
if (GetCutlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RcvCommbreaklist      = (int (*)(cmyth_conn_t conn, int* err, cmyth_commbreaklist_t breaklist, int count))
dlsym(m_libcmyth, "cmyth_rcv_commbreaklist");
if (RcvCommbreaklist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    InputlistCreate      = (cmyth_inputlist_t (*)(void))
dlsym(m_libcmyth, "cmyth_inputlist_create");
if (InputlistCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    InputCreate       = (cmyth_input_t (*)(void))
dlsym(m_libcmyth, "cmyth_input_create");
if (InputCreate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetFreeInputlist      = (cmyth_inputlist_t (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_get_free_inputlist");
if (GetFreeInputlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RcvFreeInputlist      = (int (*)(cmyth_conn_t conn, int* err, cmyth_inputlist_t inputlist, int count))
dlsym(m_libcmyth, "cmyth_rcv_free_inputlist");
if (RcvFreeInputlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetRecgroups      = (int (*)(cmyth_database_t, cmyth_recgroups_t**))
dlsym(m_libcmyth, "cmyth_mysql_get_recgroups");
if (MysqlGetRecgroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlDeleteScheduledRecording      = (int (*)(cmyth_database_t db, char* query))
dlsym(m_libcmyth, "cmyth_mysql_delete_scheduled_recording");
if (MysqlDeleteScheduledRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlInsertIntoRecord      = (int (*)(cmyth_database_t db, char* query, char* query1, char* query2, char* title, char* subtitle, char* description, char* callsign))
dlsym(m_libcmyth, "cmyth_mysql_insert_into_record");
if (MysqlInsertIntoRecord == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetRecordidMysql      = (char* (*)(cmyth_database_t, int, char* , char* , char* , char* , char* ))
dlsym(m_libcmyth, "cmyth_get_recordid_mysql");
if (GetRecordidMysql == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetOffsetMysql      = (int (*)(cmyth_database_t, int, char* , int, char* , char* , char* , char* , char* ))
dlsym(m_libcmyth, "cmyth_get_offset_mysql");
if (GetOffsetMysql == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetProgFinderCharTitle      = (int (*)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, char* program_name))
dlsym(m_libcmyth, "cmyth_mysql_get_prog_finder_char_title");
if (MysqlGetProgFinderCharTitle == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetProgFinderTime      = (int (*)(cmyth_database_t db, cmyth_program_t**prog,  time_t starttime, char* program_name))
dlsym(m_libcmyth, "cmyth_mysql_get_prog_finder_time");
if (MysqlGetProgFinderTime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetGuide      = (int (*)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, time_t endtime))
dlsym(m_libcmyth, "cmyth_mysql_get_guide");
if (MysqlGetGuide == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlTestdbConnection      = (int (*)(cmyth_database_t db,char**message))
dlsym(m_libcmyth, "cmyth_mysql_testdb_connection");
if (MysqlTestdbConnection == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ScheduleRecording      = (int (*)(cmyth_conn_t conn, char* msg))
dlsym(m_libcmyth, "cmyth_schedule_recording");
if (ScheduleRecording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlEscapeChars      = (char* (*)(cmyth_database_t db, char* string))
dlsym(m_libcmyth, "cmyth_mysql_escape_chars");
if (MysqlEscapeChars == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetCommbreakList      = (int (*)(cmyth_database_t db, int chanid, char* start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version))
dlsym(m_libcmyth, "cmyth_mysql_get_commbreak_list");
if (MysqlGetCommbreakList == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetPrevRecorded      = (int (*)(cmyth_database_t db, cmyth_program_t**prog))
dlsym(m_libcmyth, "cmyth_mysql_get_prev_recorded");
if (MysqlGetPrevRecorded == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    GetDeleteList      = (int (*)(cmyth_conn_t, char* , cmyth_proglist_t))
dlsym(m_libcmyth, "cmyth_get_delete_list");
if (GetDeleteList == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MythtvRemovePreviousRecorded      = (int (*)(cmyth_database_t db,char* query))
dlsym(m_libcmyth, "cmyth_mythtv_remove_previous_recorded");
if (MythtvRemovePreviousRecorded == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetChanlist      = (cmyth_chanlist_t (*)(cmyth_database_t db))
dlsym(m_libcmyth, "cmyth_mysql_get_chanlist");
if (MysqlGetChanlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlIsRadio      = (int (*)(cmyth_database_t db, int chanid))
dlsym(m_libcmyth, "cmyth_mysql_is_radio");
if (MysqlIsRadio == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerRecordid      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_recordid");
if (TimerRecordid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerChanid      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_chanid");
if (TimerChanid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerStarttime      = (time_t (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_starttime");
if (TimerStarttime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerEndtime      = (time_t (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_endtime");
if (TimerEndtime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerTitle      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_title");
if (TimerTitle == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerDescription      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_description");
if (TimerDescription == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerType      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_type");
if (TimerType == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerCategory      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_category");
if (TimerCategory == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerSubtitle      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_subtitle");
if (TimerSubtitle == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerPriority      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_priority");
if (TimerPriority == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerStartoffset      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_startoffset");
if (TimerStartoffset == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerEndoffset      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_endoffset");
if (TimerEndoffset == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerSearchtype      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_searchtype");
if (TimerSearchtype == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerInactive      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_inactive");
if (TimerInactive == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerCallsign      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_callsign");
if (TimerCallsign == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerDupMethod      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_dup_method");
if (TimerDupMethod == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerDupIn      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_dup_in");
if (TimerDupIn == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerRecGroup      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_rec_group");
if (TimerRecGroup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerStoreGroup      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_store_group");
if (TimerStoreGroup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerPlayGroup      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_play_group");
if (TimerPlayGroup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerAutotranscode      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_autotranscode");
if (TimerAutotranscode == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerUserjobs      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_userjobs");
if (TimerUserjobs == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerAutocommflag      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_autocommflag");
if (TimerAutocommflag == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerAutoexpire      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_autoexpire");
if (TimerAutoexpire == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerMaxepisodes      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_maxepisodes");
if (TimerMaxepisodes == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerMaxnewest      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_maxnewest");
if (TimerMaxnewest == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerTranscoder      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_transcoder");
if (TimerTranscoder == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerlistGetItem      = (cmyth_timer_t (*)(cmyth_timerlist_t pl, int index))
dlsym(m_libcmyth, "cmyth_timerlist_get_item");
if (TimerlistGetItem == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TimerlistGetCount      = (int (*)(cmyth_timerlist_t pl))
dlsym(m_libcmyth, "cmyth_timerlist_get_count");
if (TimerlistGetCount == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetTimers      = (cmyth_timerlist_t (*)(cmyth_database_t db))
dlsym(m_libcmyth, "cmyth_mysql_get_timers");
if (MysqlGetTimers == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlAddTimer      = (int (*)(cmyth_database_t db, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category,int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder))
dlsym(m_libcmyth, "cmyth_mysql_add_timer");
if (MysqlAddTimer == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlDeleteTimer      = (int (*)(cmyth_database_t db, int recordid))
dlsym(m_libcmyth, "cmyth_mysql_delete_timer");
if (MysqlDeleteTimer == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlUpdateTimer      = (int (*)(cmyth_database_t db, int recordid, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category, int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder))
dlsym(m_libcmyth, "cmyth_mysql_update_timer");
if (MysqlUpdateTimer == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetChannelgroups      = (int (*)(cmyth_database_t db,cmyth_channelgroups_t** changroups))
dlsym(m_libcmyth, "cmyth_mysql_get_channelgroups");
if (MysqlGetChannelgroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetChannelidsInGroup      = (int (*)(cmyth_database_t db,unsigned int groupid,int** chanids))
dlsym(m_libcmyth, "cmyth_mysql_get_channelids_in_group");
if (MysqlGetChannelidsInGroup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelSourceid      = (int (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_sourceid");
if (ChannelSourceid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ChannelMultiplex      = (int (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_multiplex");
if (ChannelMultiplex == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetRecorderList      = (int (*)(cmyth_database_t db,cmyth_rec_t** reclist))
dlsym(m_libcmyth, "cmyth_mysql_get_recorder_list");
if (MysqlGetRecorderList == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetProgFinderTimeTitleChan      = (int (*)(cmyth_database_t db,cmyth_program_t* prog, char* title,time_t starttime,int chanid))
dlsym(m_libcmyth, "cmyth_mysql_get_prog_finder_time_title_chan");
if (MysqlGetProgFinderTimeTitleChan == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetStoragegroups      = (int (*)(cmyth_database_t db, char*** profiles))
dlsym(m_libcmyth, "cmyth_mysql_get_storagegroups");
if (MysqlGetStoragegroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetPlaygroups      = (int (*)(cmyth_database_t db, char*** profiles))
dlsym(m_libcmyth, "cmyth_mysql_get_playgroups");
if (MysqlGetPlaygroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetRecprofiles      = (int (*)(cmyth_database_t db, cmyth_recprofile_t** profiles))
dlsym(m_libcmyth, "cmyth_mysql_get_recprofiles");
if (MysqlGetRecprofiles == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    MysqlGetCardtype      = (char* (*)(cmyth_database_t db, int chanid))
dlsym(m_libcmyth, "cmyth_mysql_get_cardtype");
if (MysqlGetCardtype == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    StoragegroupFilelist      = (int (*)(cmyth_conn_t control, char*** sgFilelist, char* sg2List, char*  mythostname))
dlsym(m_libcmyth, "cmyth_storagegroup_filelist");
if (StoragegroupFilelist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    StoragegroupFilelistGetItem      = (cmyth_storagegroup_file_t (*)(cmyth_storagegroup_filelist_t fl, int index))
dlsym(m_libcmyth, "cmyth_storagegroup_filelist_get_item");
if (StoragegroupFilelistGetItem == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    StoragegroupFilelistCount      = (int (*)(cmyth_storagegroup_filelist_t fl))
dlsym(m_libcmyth, "cmyth_storagegroup_filelist_count");
if (StoragegroupFilelistCount == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    StoragegroupGetFilelist      = (cmyth_storagegroup_filelist_t (*)(cmyth_conn_t control,char* storagegroup, char* hostname))
dlsym(m_libcmyth, "cmyth_storagegroup_get_filelist");
if (StoragegroupGetFilelist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    StoragegroupFileGetFilename      = (char* (*)(cmyth_storagegroup_file_t file))
dlsym(m_libcmyth, "cmyth_storagegroup_file_get_filename");
if (StoragegroupFileGetFilename == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    StoragegroupFileGetLastmodified      = (unsigned long (*)(cmyth_storagegroup_file_t file))
dlsym(m_libcmyth, "cmyth_storagegroup_file_get_lastmodified");
if (StoragegroupFileGetLastmodified == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    StoragegroupFileGetSize      = (unsigned long long (*)(cmyth_storagegroup_file_t file))
dlsym(m_libcmyth, "cmyth_storagegroup_file_get_size");
if (StoragegroupFileGetSize == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RefRelease      = (void (*)(void* p))
dlsym(m_libcmyth, "ref_release");
if (RefRelease == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RefHold      = (void* (*)(void* p))
dlsym(m_libcmyth, "ref_hold");
if (RefHold == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RefStrdup      = (char* (*)(char* str))
dlsym(m_libcmyth, "ref_strdup");
if (RefStrdup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RefRealloc      = (void* (*)(void* p, size_t len))
dlsym(m_libcmyth, "ref_realloc");
if (RefRealloc == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RefSetDestroy      = (void (*)(void* block, ref_destroy_t func))
dlsym(m_libcmyth, "ref_set_destroy");
if (RefSetDestroy == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    RefAllocShow      = (void (*)(void))
dlsym(m_libcmyth, "ref_alloc_show");
if (RefAllocShow == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

  return true;
  }

//dll functions

void (*DbgLevel)(int l);
void (*DbgAll)(void);
void (*DbgNone)(void);
void (*Dbg)(int level, char* fmt, ...);
void (*SetDbgMsgcallback)(void (* msgcb)(int level,char* ));
cmyth_conn_t (*ConnConnectCtrl)(char* server, unsigned short port, unsigned buflen, int tcp_rcvbuf);
int (*ConnReconnectCtrl)(cmyth_conn_t control);
cmyth_conn_t (*ConnConnectEvent)(char* server,  unsigned short port,  unsigned buflen, int tcp_rcvbuf);
int (*ConnReconnectEvent)(cmyth_conn_t conn);
cmyth_file_t (*ConnConnectFile)(cmyth_proginfo_t prog, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf);
cmyth_file_t (*ConnConnectPath)(char* path, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf,char* sgToGetFrom);
int (*ConnConnectRing)(cmyth_recorder_t rec, unsigned buflen,int tcp_rcvbuf);
int (*ConnConnectRecorder)(cmyth_recorder_t rec, unsigned buflen, int tcp_rcvbuf);
int (*ConnCheckBlock)(cmyth_conn_t conn, unsigned long size);
cmyth_recorder_t (*ConnGetRecorderFromNum)(cmyth_conn_t conn, int num);
cmyth_recorder_t (*ConnGetFreeRecorder)(cmyth_conn_t conn);
int (*ConnGetFreespace)(cmyth_conn_t control, long long* total, long long* used);
int (*ConnHung)(cmyth_conn_t control);
int (*ConnGetFreeRecorderCount)(cmyth_conn_t conn);
int (*ConnGetProtocolVersion)(cmyth_conn_t conn);
char* (*ConnGetSetting)(cmyth_conn_t conn,const char* hostname, const char* setting);
int (*ConnSetSetting)(cmyth_conn_t conn,const char* hostname, const char* setting, const char* value);
char* (*ConnGetBackendHostname)(cmyth_conn_t conn);
char* (*ConnGetClientHostname)(cmyth_conn_t conn);
cmyth_event_t (*EventGet)(cmyth_conn_t conn, char* data, int len);
int (*EventSelect)(cmyth_conn_t conn, struct timeval* timeout);
cmyth_recorder_t (*RecorderCreate)(void);
cmyth_recorder_t (*RecorderDup)(cmyth_recorder_t p);
int (*RecorderIsRecording)(cmyth_recorder_t rec);
int (*RecorderGetFramerate)(cmyth_recorder_t rec,double* rate);
long long (*RecorderGetFramesWritten)(cmyth_recorder_t rec);
long long (*RecorderGetFreeSpace)(cmyth_recorder_t rec);
long long (*RecorderGetKeyframePos)(cmyth_recorder_t rec, unsigned long keynum);
cmyth_posmap_t (*RecorderGetPositionMap)(cmyth_recorder_t rec,unsigned long start,unsigned long end);
cmyth_proginfo_t (*RecorderGetRecording)(cmyth_recorder_t rec);
int (*RecorderStopPlaying)(cmyth_recorder_t rec);
int (*RecorderFrontendReady)(cmyth_recorder_t rec);
int (*RecorderCancelNextRecording)(cmyth_recorder_t rec);
int (*RecorderPause)(cmyth_recorder_t rec);
int (*RecorderFinishRecording)(cmyth_recorder_t rec);
int (*RecorderToggleChannelFavorite)(cmyth_recorder_t rec);
int (*RecorderChangeChannel)(cmyth_recorder_t rec, cmyth_channeldir_t direction);
int (*RecorderSetChannel)(cmyth_recorder_t rec,char* channame);
int (*RecorderChangeColor)(cmyth_recorder_t rec, cmyth_adjdir_t direction);
int (*RecorderChangeBrightness)(cmyth_recorder_t rec, cmyth_adjdir_t direction);
int (*RecorderChangeContrast)(cmyth_recorder_t rec,  cmyth_adjdir_t direction);
int (*RecorderChangeHue)(cmyth_recorder_t rec,  cmyth_adjdir_t direction);
int (*RecorderCheckChannel)(cmyth_recorder_t rec,char* channame);
int (*RecorderCheckChannelPrefix)(cmyth_recorder_t rec, char* channame);
cmyth_proginfo_t (*RecorderGetCurProginfo)(cmyth_recorder_t rec);
cmyth_proginfo_t (*RecorderGetNextProginfo)(cmyth_recorder_t rec,cmyth_proginfo_t current,cmyth_browsedir_t direction);
int (*RecorderGetInputName)(cmyth_recorder_t rec, char* name, unsigned len);
long long (*RecorderSeek)(cmyth_recorder_t rec,  long long pos,  cmyth_whence_t whence,  long long curpos);
int (*RecorderSpawnChainLivetv)(cmyth_recorder_t rec, char* channame);
int (*RecorderSpawnLivetv)(cmyth_recorder_t rec);
int (*RecorderStartStream)(cmyth_recorder_t rec);
int (*RecorderEndStream)(cmyth_recorder_t rec);
char* (*RecorderGetFilename)(cmyth_recorder_t rec);
int (*RecorderStopLivetv)(cmyth_recorder_t rec);
int (*RecorderDoneRingbuf)(cmyth_recorder_t rec);
int (*RecorderGetRecorderId)(cmyth_recorder_t rec);
cmyth_livetv_chain_t (*LivetvChainCreate)(char* chainid);
long long (*LivetvChainDuration)(cmyth_recorder_t rec);
int (*LivetvChainSwitch)(cmyth_recorder_t rec, int dir);
int (*LivetvChainSwitchLast)(cmyth_recorder_t rec);
int (*LivetvChainUpdate)(cmyth_recorder_t rec, char* chainid,int tcp_rcvbuf);
cmyth_recorder_t (*SpawnLiveTv)(cmyth_recorder_t rec,unsigned buflen,int tcp_rcvbuf,  void (* prog_update_callback)(cmyth_proginfo_t),char** err, char* channame);
cmyth_recorder_t (*LivetvChainSetup)(cmyth_recorder_t old_rec, int tcp_rcvbuf, void (* prog_update_callback)(cmyth_proginfo_t));
int (*LivetvGetBlock)(cmyth_recorder_t rec, char* buf, unsigned long len);
int (*LivetvSelect)(cmyth_recorder_t rec, struct timeval* timeout);
int (*LivetvRequestBlock)(cmyth_recorder_t rec, unsigned long len);
long long (*LivetvSeek)(cmyth_recorder_t rec,long long offset, int whence);
int (*LivetvRead)(cmyth_recorder_t rec,  char* buf,  unsigned long len);
int (*LivetvKeepRecording)(cmyth_recorder_t rec, cmyth_database_t db, int keep);
cmyth_database_t (*DatabaseInit)(char* host, char* db_name, char* user, char* pass);
void (*DatabaseClose)(cmyth_database_t db);
int (*DatabaseSetHost)(cmyth_database_t db, char* host);
int (*DatabaseSetUser)(cmyth_database_t db, char* user);
int (*DatabaseSetPass)(cmyth_database_t db, char* pass);
int (*DatabaseSetName)(cmyth_database_t db, char* name);
int (*SetWatchedStatusMysql)(cmyth_database_t db, cmyth_proginfo_t prog, int watchedStat);
char* (*RingbufPathname)(cmyth_recorder_t rec);
cmyth_ringbuf_t (*RingbufCreate)(void);
cmyth_recorder_t (*RingbufSetup)(cmyth_recorder_t old_rec);
int (*RingbufRequestBlock)(cmyth_recorder_t rec, unsigned long len);
int (*RingbufSelect)(cmyth_recorder_t rec, struct timeval* timeout);
int (*RingbufGetBlock)(cmyth_recorder_t rec,char* buf,unsigned long len);
long long (*RingbufSeek)(cmyth_recorder_t rec, long long offset, int whence);
int (*RingbufRead)(cmyth_recorder_t rec,char* buf,unsigned long len);
cmyth_rec_num_t (*RecNumCreate)(void);
cmyth_rec_num_t (*RecNumGet)(char* host, unsigned short port, unsigned id);
char* (*RecNumString)(cmyth_rec_num_t rn);
cmyth_timestamp_t (*TimestampCreate)(void);
cmyth_timestamp_t (*TimestampFromString)(char* str);
cmyth_timestamp_t (*TimestampFromUnixtime)(time_t l);
time_t (*TimestampToUnixtime)(cmyth_timestamp_t ts);
int (*TimestampToString)(char* str, cmyth_timestamp_t ts);
int (*TimestampToIsostring)(char* str, cmyth_timestamp_t ts);
int (*TimestampToDisplayString)(char* str, cmyth_timestamp_t ts, int time_format_12);
int (*DatetimeToString)(char* str, cmyth_timestamp_t ts);
int (*TimestampCompare)(cmyth_timestamp_t ts1,cmyth_timestamp_t ts2);
cmyth_keyframe_t (*KeyframeCreate)(void);
char* (*KeyframeString)(cmyth_keyframe_t kf);
cmyth_posmap_t (*PosmapCreate)(void);
cmyth_proginfo_t (*ProginfoCreate)(void);
int (*ProginfoStopRecording)(cmyth_conn_t control, cmyth_proginfo_t prog);
int (*ProginfoCheckRecording)(cmyth_conn_t control,  cmyth_proginfo_t prog);
int (*ProginfoDeleteRecording)(cmyth_conn_t control,cmyth_proginfo_t prog);
int (*ProginfoForgetRecording)(cmyth_conn_t control,cmyth_proginfo_t prog);
int (*ProginfoGetRecorderNum)(cmyth_conn_t control,cmyth_rec_num_t rnum,cmyth_proginfo_t prog);
cmyth_proginfo_t (*ProginfoGetFromBasename)(cmyth_conn_t control, const char* basename);
char* (*ProginfoTitle)(cmyth_proginfo_t prog);
char* (*ProginfoSubtitle)(cmyth_proginfo_t prog);
char* (*ProginfoDescription)(cmyth_proginfo_t prog);
char* (*ProginfoCategory)(cmyth_proginfo_t prog);
char* (*ProginfoChanstr)(cmyth_proginfo_t prog);
char* (*ProginfoChansign)(cmyth_proginfo_t prog);
char* (*ProginfoChanname)(cmyth_proginfo_t prog);
long (*ProginfoChanId)(cmyth_proginfo_t prog);
char* (*ProginfoPathname)(cmyth_proginfo_t prog);
char* (*ProginfoSeriesid)(cmyth_proginfo_t prog);
char* (*ProginfoProgramid)(cmyth_proginfo_t prog);
unsigned long (*ProginfoRecordid)(cmyth_proginfo_t prog);
long (*ProginfoPriority)(cmyth_proginfo_t prog);
char* (*ProginfoStars)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*ProginfoRecStart)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*ProginfoRecEnd)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*ProginfoOriginalairdate)(cmyth_proginfo_t prog);
cmyth_proginfo_rec_status_t (*ProginfoRecStatus)(cmyth_proginfo_t prog);
unsigned long (*ProginfoFlags)(  cmyth_proginfo_t prog);
long long (*ProginfoLength)(cmyth_proginfo_t prog);
char* (*ProginfoHost)(cmyth_proginfo_t prog);
int (*ProginfoCompare)(cmyth_proginfo_t a, cmyth_proginfo_t b);
int (*ProginfoLengthSec)(cmyth_proginfo_t prog);
cmyth_proginfo_t (*ProginfoGetDetail)(cmyth_conn_t control,  cmyth_proginfo_t p);
cmyth_timestamp_t (*ProginfoStart)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*ProginfoEnd)(cmyth_proginfo_t prog);
long (*ProginfoCardId)(cmyth_proginfo_t prog);
char* (*ProginfoRecgroup)(cmyth_proginfo_t prog);
char* (*ProginfoChanicon)(cmyth_proginfo_t prog);
char* (*ProginfoYear)(cmyth_proginfo_t prog);
cmyth_proglist_t (*ProglistCreate)(void);
cmyth_proglist_t (*ProglistGetAllRecorded)(cmyth_conn_t control);
cmyth_proglist_t (*ProglistGetAllPending)(cmyth_conn_t control);
cmyth_proglist_t (*ProglistGetAllScheduled)(cmyth_conn_t control);
cmyth_proglist_t (*ProglistGetConflicting)(cmyth_conn_t control);
cmyth_proginfo_t (*ProglistGetItem)(cmyth_proglist_t pl,int index);
int (*ProglistDeleteItem)(cmyth_proglist_t pl,cmyth_proginfo_t prog);
int (*ProglistGetCount)(cmyth_proglist_t pl);
int (*ProglistSort)(cmyth_proglist_t pl, int count, cmyth_proglist_sort_t sort);
cmyth_conn_t (*FileData)(cmyth_file_t file);
unsigned long long (*FileStart)(cmyth_file_t file);
unsigned long long (*FileLength)(cmyth_file_t file);
unsigned long long (*FilePosition)(cmyth_file_t file);
int (*UpdateFileLength)(cmyth_file_t file, unsigned long long newLength);
int (*FileGetBlock)(cmyth_file_t file, char* buf,unsigned long len);
int (*FileRequestBlock)(cmyth_file_t file, unsigned long len);
long long (*FileSeek)(cmyth_file_t file, long long offset, int whence);
int (*FileSelect)(cmyth_file_t file, struct timeval* timeout);
void (*FileSetClosedCallback)(cmyth_file_t file,void (* callback)(cmyth_file_t));
int (*FileRead)(cmyth_file_t file,char* buf,unsigned long len);
long (*ChannelChanid)(cmyth_channel_t channel);
long (*ChannelChannum)(cmyth_channel_t channel);
char* (*ChannelChannumstr)(cmyth_channel_t channel);
char* (*ChannelCallsign)(cmyth_channel_t channel);
char* (*ChannelName)(cmyth_channel_t channel);
char* (*ChannelIcon)(cmyth_channel_t channel);
int (*ChannelVisible)(cmyth_channel_t channel);
cmyth_channel_t (*ChanlistGetItem)(cmyth_chanlist_t pl, int index);
int (*ChanlistGetCount)(cmyth_chanlist_t pl);
cmyth_freespace_t (*FreespaceCreate)(void);
long long (*GetBookmark)(cmyth_conn_t conn, cmyth_proginfo_t prog);
int (*GetBookmarkOffset)(cmyth_database_t db, long chanid, long long mark);
int (*GetBookmarkMark)(cmyth_database_t, cmyth_proginfo_t, long long);
int (*SetBookmark)(cmyth_conn_t conn, cmyth_proginfo_t prog,long long bookmark);
cmyth_commbreaklist_t (*CommbreaklistCreate)(void);
cmyth_commbreak_t (*CommbreakCreate)(void);
cmyth_commbreaklist_t (*GetCommbreaklist)(cmyth_conn_t conn, cmyth_proginfo_t prog);
cmyth_commbreaklist_t (*GetCutlist)(cmyth_conn_t conn, cmyth_proginfo_t prog);
int (*RcvCommbreaklist)(cmyth_conn_t conn, int* err, cmyth_commbreaklist_t breaklist, int count);
cmyth_inputlist_t (*InputlistCreate)(void);
cmyth_input_t (*InputCreate)(void);
cmyth_inputlist_t (*GetFreeInputlist)(cmyth_recorder_t rec);
int (*RcvFreeInputlist)(cmyth_conn_t conn, int* err, cmyth_inputlist_t inputlist, int count);
int (*MysqlGetRecgroups)(cmyth_database_t, cmyth_recgroups_t**);
int (*MysqlDeleteScheduledRecording)(cmyth_database_t db, char* query);
int (*MysqlInsertIntoRecord)(cmyth_database_t db, char* query, char* query1, char* query2, char* title, char* subtitle, char* description, char* callsign);
char* (*GetRecordidMysql)(cmyth_database_t, int, char* , char* , char* , char* , char* );
int (*GetOffsetMysql)(cmyth_database_t, int, char* , int, char* , char* , char* , char* , char* );
int (*MysqlGetProgFinderCharTitle)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, char* program_name);
int (*MysqlGetProgFinderTime)(cmyth_database_t db, cmyth_program_t**prog,  time_t starttime, char* program_name);
int (*MysqlGetGuide)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, time_t endtime);
int (*MysqlTestdbConnection)(cmyth_database_t db,char**message);
int (*ScheduleRecording)(cmyth_conn_t conn, char* msg);
char* (*MysqlEscapeChars)(cmyth_database_t db, char* string);
int (*MysqlGetCommbreakList)(cmyth_database_t db, int chanid, char* start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version);
int (*MysqlGetPrevRecorded)(cmyth_database_t db, cmyth_program_t**prog);
int (*GetDeleteList)(cmyth_conn_t, char* , cmyth_proglist_t);
int (*MythtvRemovePreviousRecorded)(cmyth_database_t db,char* query);
cmyth_chanlist_t (*MysqlGetChanlist)(cmyth_database_t db);
int (*MysqlIsRadio)(cmyth_database_t db, int chanid);
int (*TimerRecordid)(cmyth_timer_t timer);
int (*TimerChanid)(cmyth_timer_t timer);
time_t (*TimerStarttime)(cmyth_timer_t timer);
time_t (*TimerEndtime)(cmyth_timer_t timer);
char* (*TimerTitle)(cmyth_timer_t timer);
char* (*TimerDescription)(cmyth_timer_t timer);
int (*TimerType)(cmyth_timer_t timer);
char* (*TimerCategory)(cmyth_timer_t timer);
char* (*TimerSubtitle)(cmyth_timer_t timer);
int (*TimerPriority)(cmyth_timer_t timer);
int (*TimerStartoffset)(cmyth_timer_t timer);
int (*TimerEndoffset)(cmyth_timer_t timer);
int (*TimerSearchtype)(cmyth_timer_t timer);
int (*TimerInactive)(cmyth_timer_t timer);
char* (*TimerCallsign)(cmyth_timer_t timer);
int (*TimerDupMethod)(cmyth_timer_t timer);
int (*TimerDupIn)(cmyth_timer_t timer);
char* (*TimerRecGroup)(cmyth_timer_t timer);
char* (*TimerStoreGroup)(cmyth_timer_t timer);
char* (*TimerPlayGroup)(cmyth_timer_t timer);
int (*TimerAutotranscode)(cmyth_timer_t timer);
int (*TimerUserjobs)(cmyth_timer_t timer);
int (*TimerAutocommflag)(cmyth_timer_t timer);
int (*TimerAutoexpire)(cmyth_timer_t timer);
int (*TimerMaxepisodes)(cmyth_timer_t timer);
int (*TimerMaxnewest)(cmyth_timer_t timer);
int (*TimerTranscoder)(cmyth_timer_t timer);
cmyth_timer_t (*TimerlistGetItem)(cmyth_timerlist_t pl, int index);
int (*TimerlistGetCount)(cmyth_timerlist_t pl);
cmyth_timerlist_t (*MysqlGetTimers)(cmyth_database_t db);
int (*MysqlAddTimer)(cmyth_database_t db, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category,int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder);
int (*MysqlDeleteTimer)(cmyth_database_t db, int recordid);
int (*MysqlUpdateTimer)(cmyth_database_t db, int recordid, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category, int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder);
int (*MysqlGetChannelgroups)(cmyth_database_t db,cmyth_channelgroups_t** changroups);
int (*MysqlGetChannelidsInGroup)(cmyth_database_t db,unsigned int groupid,int** chanids);
int (*ChannelSourceid)(cmyth_channel_t channel);
int (*ChannelMultiplex)(cmyth_channel_t channel);
int (*MysqlGetRecorderList)(cmyth_database_t db,cmyth_rec_t** reclist);
int (*MysqlGetProgFinderTimeTitleChan)(cmyth_database_t db,cmyth_program_t* prog, char* title,time_t starttime,int chanid);
int (*MysqlGetStoragegroups)(cmyth_database_t db, char*** profiles);
int (*MysqlGetPlaygroups)(cmyth_database_t db, char*** profiles);
int (*MysqlGetRecprofiles)(cmyth_database_t db, cmyth_recprofile_t** profiles);
char* (*MysqlGetCardtype)(cmyth_database_t db, int chanid);
int (*StoragegroupFilelist)(cmyth_conn_t control, char*** sgFilelist, char* sg2List, char*  mythostname);
cmyth_storagegroup_file_t (*StoragegroupFilelistGetItem)(cmyth_storagegroup_filelist_t fl, int index);
int (*StoragegroupFilelistCount)(cmyth_storagegroup_filelist_t fl);
cmyth_storagegroup_filelist_t (*StoragegroupGetFilelist)(cmyth_conn_t control,char* storagegroup, char* hostname);
char* (*StoragegroupFileGetFilename)(cmyth_storagegroup_file_t file);
unsigned long (*StoragegroupFileGetLastmodified)(cmyth_storagegroup_file_t file);
unsigned long long (*StoragegroupFileGetSize)(cmyth_storagegroup_file_t file);
void (*RefRelease)(void* p);
void* (*RefHold)(void* p);
char* (*RefStrdup)(char* str);
void* (*RefRealloc)(void* p, size_t len);
void (*RefSetDestroy)(void* block, ref_destroy_t func);
void (*RefAllocShow)(void);





protected:
//  int (*XBMC_register_me)(void *HANDLE);
//  void (*XBMC_unregister_me)();

private:
  void *m_libcmyth;
  void *m_Handle;
  struct cb_array
  {
    const char* libPath;
  };
};
