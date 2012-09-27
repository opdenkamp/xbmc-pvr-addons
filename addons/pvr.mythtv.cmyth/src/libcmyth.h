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
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-powerpc-osx.so"
#elif defined(__arm__)
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-arm-osx.so"
#else
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-x86-osx.so"
#endif
#elif defined(__x86_64__)
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-x86_64-linux.so"
#elif defined(_POWERPC)
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-powerpc-linux.so"
#elif defined(_POWERPC64)
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-powerpc64-linux.so"
#elif defined(_ARMEL)
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-arm.so"
#else /* !__x86_64__ && !__powerpc__ */
#define LIBCMYTH_DLL "/pvr.mythtv.cmyth/libcmyth-i486-linux.so"
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

    cmyth_dbg_level      = (void (*)(int l))
dlsym(m_libcmyth, "cmyth_dbg_level");
if (cmyth_dbg_level == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_dbg_all      = (void (*)(void))
dlsym(m_libcmyth, "cmyth_dbg_all");
if (cmyth_dbg_all == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_dbg_none      = (void (*)(void))
dlsym(m_libcmyth, "cmyth_dbg_none");
if (cmyth_dbg_none == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_dbg      = (void (*)(int level, char* fmt, ...))
dlsym(m_libcmyth, "cmyth_dbg");
if (cmyth_dbg == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_set_dbg_msgcallback      = (void (*)(void (* msgcb)(int level,char* )))
dlsym(m_libcmyth, "cmyth_set_dbg_msgcallback");
if (cmyth_set_dbg_msgcallback == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_connect_ctrl      = (cmyth_conn_t (*)(char* server, unsigned short port, unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_ctrl");
if (cmyth_conn_connect_ctrl == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_reconnect_ctrl      = (int (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_conn_reconnect_ctrl");
if (cmyth_conn_reconnect_ctrl == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_connect_event      = (cmyth_conn_t (*)(char* server,  unsigned short port,  unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_event");
if (cmyth_conn_connect_event == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_reconnect_event    = (int (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_reconnect_event");
if (cmyth_conn_reconnect_event == NULL)    { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_connect_file      = (cmyth_file_t (*)(cmyth_proginfo_t prog, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_file");
if (cmyth_conn_connect_file == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_connect_path      = (cmyth_file_t (*)(char* path, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf,char* sgToGetFrom))
dlsym(m_libcmyth, "cmyth_conn_connect_path");
if (cmyth_conn_connect_path == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_connect_ring      = (int (*)(cmyth_recorder_t rec, unsigned buflen,int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_ring");
if (cmyth_conn_connect_ring == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_connect_recorder      = (int (*)(cmyth_recorder_t rec, unsigned buflen, int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_conn_connect_recorder");
if (cmyth_conn_connect_recorder == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_check_block      = (int (*)(cmyth_conn_t conn, unsigned long size))
dlsym(m_libcmyth, "cmyth_conn_check_block");
if (cmyth_conn_check_block == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_recorder_from_num      = (cmyth_recorder_t (*)(cmyth_conn_t conn, int num))
dlsym(m_libcmyth, "cmyth_conn_get_recorder_from_num");
if (cmyth_conn_get_recorder_from_num == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_free_recorder      = (cmyth_recorder_t (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_free_recorder");
if (cmyth_conn_get_free_recorder == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_freespace      = (int (*)(cmyth_conn_t control, long long* total, long long* used))
dlsym(m_libcmyth, "cmyth_conn_get_freespace");
if (cmyth_conn_get_freespace == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_hung      = (int (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_conn_hung");
if (cmyth_conn_hung == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_free_recorder_count      = (int (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_free_recorder_count");
if (cmyth_conn_get_free_recorder_count == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_protocol_version      = (int (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_protocol_version");
if (cmyth_conn_get_protocol_version == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_setting      = (char* (*)(cmyth_conn_t conn,const char* hostname, const char* setting))
dlsym(m_libcmyth, "cmyth_conn_get_setting");
if (cmyth_conn_get_setting == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_set_setting      = (int (*)(cmyth_conn_t conn,const char* hostname, const char* setting, const char* value))
dlsym(m_libcmyth, "cmyth_conn_set_setting");
if (cmyth_conn_set_setting == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_backend_hostname      = (char* (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_backend_hostname");
if (cmyth_conn_get_backend_hostname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_conn_get_client_hostname      = (char* (*)(cmyth_conn_t conn))
dlsym(m_libcmyth, "cmyth_conn_get_client_hostname");
if (cmyth_conn_get_client_hostname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_event_get      = (cmyth_event_t (*)(cmyth_conn_t conn, char* data, int len))
dlsym(m_libcmyth, "cmyth_event_get");
if (cmyth_event_get == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_event_select      = (int (*)(cmyth_conn_t conn, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_event_select");
if (cmyth_event_select == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_create      = (cmyth_recorder_t (*)(void))
dlsym(m_libcmyth, "cmyth_recorder_create");
if (cmyth_recorder_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_dup      = (cmyth_recorder_t (*)(cmyth_recorder_t p))
dlsym(m_libcmyth, "cmyth_recorder_dup");
if (cmyth_recorder_dup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_is_recording      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_is_recording");
if (cmyth_recorder_is_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_framerate      = (int (*)(cmyth_recorder_t rec,double* rate))
dlsym(m_libcmyth, "cmyth_recorder_get_framerate");
if (cmyth_recorder_get_framerate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_frames_written      = (long long (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_frames_written");
if (cmyth_recorder_get_frames_written == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_free_space      = (long long (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_free_space");
if (cmyth_recorder_get_free_space == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_keyframe_pos      = (long long (*)(cmyth_recorder_t rec, unsigned long keynum))
dlsym(m_libcmyth, "cmyth_recorder_get_keyframe_pos");
if (cmyth_recorder_get_keyframe_pos == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_position_map      = (cmyth_posmap_t (*)(cmyth_recorder_t rec,unsigned long start,unsigned long end))
dlsym(m_libcmyth, "cmyth_recorder_get_position_map");
if (cmyth_recorder_get_position_map == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_recording      = (cmyth_proginfo_t (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_recording");
if (cmyth_recorder_get_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_stop_playing      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_stop_playing");
if (cmyth_recorder_stop_playing == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_frontend_ready      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_frontend_ready");
if (cmyth_recorder_frontend_ready == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_cancel_next_recording      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_cancel_next_recording");
if (cmyth_recorder_cancel_next_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_pause      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_pause");
if (cmyth_recorder_pause == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_finish_recording      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_finish_recording");
if (cmyth_recorder_finish_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_toggle_channel_favorite      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_toggle_channel_favorite");
if (cmyth_recorder_toggle_channel_favorite == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_change_channel      = (int (*)(cmyth_recorder_t rec, cmyth_channeldir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_channel");
if (cmyth_recorder_change_channel == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_set_channel      = (int (*)(cmyth_recorder_t rec,char* channame))
dlsym(m_libcmyth, "cmyth_recorder_set_channel");
if (cmyth_recorder_set_channel == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_change_color      = (int (*)(cmyth_recorder_t rec, cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_color");
if (cmyth_recorder_change_color == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_change_brightness      = (int (*)(cmyth_recorder_t rec, cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_brightness");
if (cmyth_recorder_change_brightness == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_change_contrast      = (int (*)(cmyth_recorder_t rec,  cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_contrast");
if (cmyth_recorder_change_contrast == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_change_hue      = (int (*)(cmyth_recorder_t rec,  cmyth_adjdir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_change_hue");
if (cmyth_recorder_change_hue == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_check_channel      = (int (*)(cmyth_recorder_t rec,char* channame))
dlsym(m_libcmyth, "cmyth_recorder_check_channel");
if (cmyth_recorder_check_channel == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_check_channel_prefix      = (int (*)(cmyth_recorder_t rec, char* channame))
dlsym(m_libcmyth, "cmyth_recorder_check_channel_prefix");
if (cmyth_recorder_check_channel_prefix == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_cur_proginfo      = (cmyth_proginfo_t (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_cur_proginfo");
if (cmyth_recorder_get_cur_proginfo == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_next_proginfo      = (cmyth_proginfo_t (*)(cmyth_recorder_t rec,cmyth_proginfo_t current,cmyth_browsedir_t direction))
dlsym(m_libcmyth, "cmyth_recorder_get_next_proginfo");
if (cmyth_recorder_get_next_proginfo == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_input_name      = (int (*)(cmyth_recorder_t rec, char* name, unsigned len))
dlsym(m_libcmyth, "cmyth_recorder_get_input_name");
if (cmyth_recorder_get_input_name == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_seek      = (long long (*)(cmyth_recorder_t rec,  long long pos,  cmyth_whence_t whence,  long long curpos))
dlsym(m_libcmyth, "cmyth_recorder_seek");
if (cmyth_recorder_seek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_spawn_chain_livetv      = (int (*)(cmyth_recorder_t rec, char* channame))
dlsym(m_libcmyth, "cmyth_recorder_spawn_chain_livetv");
if (cmyth_recorder_spawn_chain_livetv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_spawn_livetv      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_spawn_livetv");
if (cmyth_recorder_spawn_livetv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_start_stream      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_start_stream");
if (cmyth_recorder_start_stream == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_end_stream      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_end_stream");
if (cmyth_recorder_end_stream == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_filename      = (char* (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_filename");
if (cmyth_recorder_get_filename == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_stop_livetv      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_stop_livetv");
if (cmyth_recorder_stop_livetv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_done_ringbuf      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_done_ringbuf");
if (cmyth_recorder_done_ringbuf == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_recorder_get_recorder_id      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_recorder_get_recorder_id");
if (cmyth_recorder_get_recorder_id == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_chain_create      = (cmyth_livetv_chain_t (*)(char* chainid))
dlsym(m_libcmyth, "cmyth_livetv_chain_create");
if (cmyth_livetv_chain_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_chain_duration      = (long long (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_livetv_chain_duration");
if (cmyth_livetv_chain_duration == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_chain_switch      = (int (*)(cmyth_recorder_t rec, int dir))
dlsym(m_libcmyth, "cmyth_livetv_chain_switch");
if (cmyth_livetv_chain_switch == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_chain_switch_last      = (int (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_livetv_chain_switch_last");
if (cmyth_livetv_chain_switch_last == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_chain_update      = (int (*)(cmyth_recorder_t rec, char* chainid,int tcp_rcvbuf))
dlsym(m_libcmyth, "cmyth_livetv_chain_update");
if (cmyth_livetv_chain_update == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_spawn_live_tv      = (cmyth_recorder_t (*)(cmyth_recorder_t rec,unsigned buflen,int tcp_rcvbuf,  void (* prog_update_callback)(cmyth_proginfo_t),char** err, char* channame))
dlsym(m_libcmyth, "cmyth_spawn_live_tv");
if (cmyth_spawn_live_tv == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_chain_setup      = (cmyth_recorder_t (*)(cmyth_recorder_t old_rec, int tcp_rcvbuf, void (* prog_update_callback)(cmyth_proginfo_t)))
dlsym(m_libcmyth, "cmyth_livetv_chain_setup");
if (cmyth_livetv_chain_setup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_get_block      = (int (*)(cmyth_recorder_t rec, char* buf, unsigned long len))
dlsym(m_libcmyth, "cmyth_livetv_get_block");
if (cmyth_livetv_get_block == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_select      = (int (*)(cmyth_recorder_t rec, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_livetv_select");
if (cmyth_livetv_select == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_request_block      = (int (*)(cmyth_recorder_t rec, unsigned long len))
dlsym(m_libcmyth, "cmyth_livetv_request_block");
if (cmyth_livetv_request_block == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_seek      = (long long (*)(cmyth_recorder_t rec,long long offset, int whence))
dlsym(m_libcmyth, "cmyth_livetv_seek");
if (cmyth_livetv_seek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_read      = (int (*)(cmyth_recorder_t rec,  char* buf,  unsigned long len))
dlsym(m_libcmyth, "cmyth_livetv_read");
if (cmyth_livetv_read == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_livetv_keep_recording      = (int (*)(cmyth_recorder_t rec, cmyth_database_t db, int keep))
dlsym(m_libcmyth, "cmyth_livetv_keep_recording");
if (cmyth_livetv_keep_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_database_init      = (cmyth_database_t (*)(char* host, char* db_name, char* user, char* pass))
dlsym(m_libcmyth, "cmyth_database_init");
if (cmyth_database_init == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_database_close     = (void (*)(cmyth_database_t db))
dlsym(m_libcmyth, "cmyth_database_close");
if (cmyth_database_close == NULL)     { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_database_set_host      = (int (*)(cmyth_database_t db, char* host))
dlsym(m_libcmyth, "cmyth_database_set_host");
if (cmyth_database_set_host == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_database_set_user      = (int (*)(cmyth_database_t db, char* user))
dlsym(m_libcmyth, "cmyth_database_set_user");
if (cmyth_database_set_user == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_database_set_pass      = (int (*)(cmyth_database_t db, char* pass))
dlsym(m_libcmyth, "cmyth_database_set_pass");
if (cmyth_database_set_pass == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_database_set_name      = (int (*)(cmyth_database_t db, char* name))
dlsym(m_libcmyth, "cmyth_database_set_name");
if (cmyth_database_set_name == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_set_watched_status_mysql      = (int (*)(cmyth_database_t db, cmyth_proginfo_t prog, int watchedStat))
dlsym(m_libcmyth, "cmyth_set_watched_status_mysql");
if (cmyth_set_watched_status_mysql == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_pathname      = (char* (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_ringbuf_pathname");
if (cmyth_ringbuf_pathname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_create      = (cmyth_ringbuf_t (*)(void))
dlsym(m_libcmyth, "cmyth_ringbuf_create");
if (cmyth_ringbuf_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_setup      = (cmyth_recorder_t (*)(cmyth_recorder_t old_rec))
dlsym(m_libcmyth, "cmyth_ringbuf_setup");
if (cmyth_ringbuf_setup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_request_block      = (int (*)(cmyth_recorder_t rec, unsigned long len))
dlsym(m_libcmyth, "cmyth_ringbuf_request_block");
if (cmyth_ringbuf_request_block == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_select      = (int (*)(cmyth_recorder_t rec, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_ringbuf_select");
if (cmyth_ringbuf_select == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_get_block      = (int (*)(cmyth_recorder_t rec,char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_ringbuf_get_block");
if (cmyth_ringbuf_get_block == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_seek      = (long long (*)(cmyth_recorder_t rec, long long offset, int whence))
dlsym(m_libcmyth, "cmyth_ringbuf_seek");
if (cmyth_ringbuf_seek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_ringbuf_read      = (int (*)(cmyth_recorder_t rec,char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_ringbuf_read");
if (cmyth_ringbuf_read == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_rec_num_create      = (cmyth_rec_num_t (*)(void))
dlsym(m_libcmyth, "cmyth_rec_num_create");
if (cmyth_rec_num_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_rec_num_get      = (cmyth_rec_num_t (*)(char* host, unsigned short port, unsigned id))
dlsym(m_libcmyth, "cmyth_rec_num_get");
if (cmyth_rec_num_get == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_rec_num_string      = (char* (*)(cmyth_rec_num_t rn))
dlsym(m_libcmyth, "cmyth_rec_num_string");
if (cmyth_rec_num_string == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_create      = (cmyth_timestamp_t (*)(void))
dlsym(m_libcmyth, "cmyth_timestamp_create");
if (cmyth_timestamp_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_from_string      = (cmyth_timestamp_t (*)(char* str))
dlsym(m_libcmyth, "cmyth_timestamp_from_string");
if (cmyth_timestamp_from_string == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_from_unixtime      = (cmyth_timestamp_t (*)(time_t l))
dlsym(m_libcmyth, "cmyth_timestamp_from_unixtime");
if (cmyth_timestamp_from_unixtime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_to_unixtime      = (time_t (*)(cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_timestamp_to_unixtime");
if (cmyth_timestamp_to_unixtime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_to_string      = (int (*)(char* str, cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_timestamp_to_string");
if (cmyth_timestamp_to_string == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_to_isostring      = (int (*)(char* str, cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_timestamp_to_isostring");
if (cmyth_timestamp_to_isostring == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_to_display_string      = (int (*)(char* str, cmyth_timestamp_t ts, int time_format_12))
dlsym(m_libcmyth, "cmyth_timestamp_to_display_string");
if (cmyth_timestamp_to_display_string == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_datetime_to_string      = (int (*)(char* str, cmyth_timestamp_t ts))
dlsym(m_libcmyth, "cmyth_datetime_to_string");
if (cmyth_datetime_to_string == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timestamp_compare      = (int (*)(cmyth_timestamp_t ts1,cmyth_timestamp_t ts2))
dlsym(m_libcmyth, "cmyth_timestamp_compare");
if (cmyth_timestamp_compare == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_keyframe_create      = (cmyth_keyframe_t (*)(void))
dlsym(m_libcmyth, "cmyth_keyframe_create");
if (cmyth_keyframe_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_keyframe_string      = (char* (*)(cmyth_keyframe_t kf))
dlsym(m_libcmyth, "cmyth_keyframe_string");
if (cmyth_keyframe_string == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_posmap_create      = (cmyth_posmap_t (*)(void))
dlsym(m_libcmyth, "cmyth_posmap_create");
if (cmyth_posmap_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_create      = (cmyth_proginfo_t (*)(void))
dlsym(m_libcmyth, "cmyth_proginfo_create");
if (cmyth_proginfo_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_stop_recording      = (int (*)(cmyth_conn_t control, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_stop_recording");
if (cmyth_proginfo_stop_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_check_recording      = (int (*)(cmyth_conn_t control,  cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_check_recording");
if (cmyth_proginfo_check_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_delete_recording      = (int (*)(cmyth_conn_t control,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_delete_recording");
if (cmyth_proginfo_delete_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_forget_recording      = (int (*)(cmyth_conn_t control,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_forget_recording");
if (cmyth_proginfo_forget_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_get_recorder_num      = (int (*)(cmyth_conn_t control,cmyth_rec_num_t rnum,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_get_recorder_num");
if (cmyth_proginfo_get_recorder_num == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_get_from_basename      = (cmyth_proginfo_t (*)(cmyth_conn_t control, const char* basename))
dlsym(m_libcmyth, "cmyth_proginfo_get_from_basename");
if (cmyth_proginfo_get_from_basename == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_title      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_title");
if (cmyth_proginfo_title == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_subtitle      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_subtitle");
if (cmyth_proginfo_subtitle == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_description      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_description");
if (cmyth_proginfo_description == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_category      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_category");
if (cmyth_proginfo_category == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_chanstr      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chanstr");
if (cmyth_proginfo_chanstr == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_chansign      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chansign");
if (cmyth_proginfo_chansign == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_channame      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_channame");
if (cmyth_proginfo_channame == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_chan_id      = (long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chan_id");
if (cmyth_proginfo_chan_id == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_pathname      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_pathname");
if (cmyth_proginfo_pathname == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_seriesid      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_seriesid");
if (cmyth_proginfo_seriesid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_programid      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_programid");
if (cmyth_proginfo_programid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_recordid      = (unsigned long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_recordid");
if (cmyth_proginfo_recordid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_priority      = (long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_priority");
if (cmyth_proginfo_priority == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_stars      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_stars");
if (cmyth_proginfo_stars == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_rec_start      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_rec_start");
if (cmyth_proginfo_rec_start == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_rec_end      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_rec_end");
if (cmyth_proginfo_rec_end == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_originalairdate      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_originalairdate");
if (cmyth_proginfo_originalairdate == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_rec_status      = (cmyth_proginfo_rec_status_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_rec_status");
if (cmyth_proginfo_rec_status == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_flags      = (unsigned long (*)(  cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_flags");
if (cmyth_proginfo_flags == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_length      = (long long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_length");
if (cmyth_proginfo_length == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_host      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_host");
if (cmyth_proginfo_host == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_compare      = (int (*)(cmyth_proginfo_t a, cmyth_proginfo_t b))
dlsym(m_libcmyth, "cmyth_proginfo_compare");
if (cmyth_proginfo_compare == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_length_sec      = (int (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_length_sec");
if (cmyth_proginfo_length_sec == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_get_detail      = (cmyth_proginfo_t (*)(cmyth_conn_t control,  cmyth_proginfo_t p))
dlsym(m_libcmyth, "cmyth_proginfo_get_detail");
if (cmyth_proginfo_get_detail == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_start      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_start");
if (cmyth_proginfo_start == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_end      = (cmyth_timestamp_t (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_end");
if (cmyth_proginfo_end == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_card_id      = (long (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_card_id");
if (cmyth_proginfo_card_id == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_recgroup      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_recgroup");
if (cmyth_proginfo_recgroup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_chanicon      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_chanicon");
if (cmyth_proginfo_chanicon == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proginfo_year      = (char* (*)(cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proginfo_year");
if (cmyth_proginfo_year == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_create      = (cmyth_proglist_t (*)(void))
dlsym(m_libcmyth, "cmyth_proglist_create");
if (cmyth_proglist_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_get_all_recorded      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_all_recorded");
if (cmyth_proglist_get_all_recorded == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_get_all_pending      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_all_pending");
if (cmyth_proglist_get_all_pending == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_get_all_scheduled      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_all_scheduled");
if (cmyth_proglist_get_all_scheduled == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_get_conflicting      = (cmyth_proglist_t (*)(cmyth_conn_t control))
dlsym(m_libcmyth, "cmyth_proglist_get_conflicting");
if (cmyth_proglist_get_conflicting == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_get_item      = (cmyth_proginfo_t (*)(cmyth_proglist_t pl,int index))
dlsym(m_libcmyth, "cmyth_proglist_get_item");
if (cmyth_proglist_get_item == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_delete_item      = (int (*)(cmyth_proglist_t pl,cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_proglist_delete_item");
if (cmyth_proglist_delete_item == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_get_count      = (int (*)(cmyth_proglist_t pl))
dlsym(m_libcmyth, "cmyth_proglist_get_count");
if (cmyth_proglist_get_count == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_proglist_sort      = (int (*)(cmyth_proglist_t pl, int count, cmyth_proglist_sort_t sort))
dlsym(m_libcmyth, "cmyth_proglist_sort");
if (cmyth_proglist_sort == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_data      = (cmyth_conn_t (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_data");
if (cmyth_file_data == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_start      = (unsigned long long (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_start");
if (cmyth_file_start == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_length      = (unsigned long long (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_length");
if (cmyth_file_length == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_position      = (unsigned long long (*)(cmyth_file_t file))
dlsym(m_libcmyth, "cmyth_file_position");
if (cmyth_file_position == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_update_file_length      = (int (*)(cmyth_file_t file, unsigned long long newLength))
dlsym(m_libcmyth, "cmyth_update_file_length");
if (cmyth_update_file_length == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_get_block      = (int (*)(cmyth_file_t file, char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_file_get_block");
if (cmyth_file_get_block == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_request_block      = (int (*)(cmyth_file_t file, unsigned long len))
dlsym(m_libcmyth, "cmyth_file_request_block");
if (cmyth_file_request_block == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_seek      = (long long (*)(cmyth_file_t file, long long offset, int whence))
dlsym(m_libcmyth, "cmyth_file_seek");
if (cmyth_file_seek == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_select      = (int (*)(cmyth_file_t file, struct timeval* timeout))
dlsym(m_libcmyth, "cmyth_file_select");
if (cmyth_file_select == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_set_closed_callback      = (void (*)(cmyth_file_t file,void (* callback)(cmyth_file_t)))
dlsym(m_libcmyth, "cmyth_file_set_closed_callback");
if (cmyth_file_set_closed_callback == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_file_read      = (int (*)(cmyth_file_t file,char* buf,unsigned long len))
dlsym(m_libcmyth, "cmyth_file_read");
if (cmyth_file_read == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_chanid      = (long (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_chanid");
if (cmyth_channel_chanid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_channum      = (long (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_channum");
if (cmyth_channel_channum == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_channumstr      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_channumstr");
if (cmyth_channel_channumstr == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_callsign      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_callsign");
if (cmyth_channel_callsign == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_name      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_name");
if (cmyth_channel_name == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_icon      = (char* (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_icon");
if (cmyth_channel_icon == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_visible      = (int (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_visible");
if (cmyth_channel_visible == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_chanlist_get_item      = (cmyth_channel_t (*)(cmyth_chanlist_t pl, int index))
dlsym(m_libcmyth, "cmyth_chanlist_get_item");
if (cmyth_chanlist_get_item == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_chanlist_get_count      = (int (*)(cmyth_chanlist_t pl))
dlsym(m_libcmyth, "cmyth_chanlist_get_count");
if (cmyth_chanlist_get_count == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_freespace_create      = (cmyth_freespace_t (*)(void))
dlsym(m_libcmyth, "cmyth_freespace_create");
if (cmyth_freespace_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_bookmark      = (long long (*)(cmyth_conn_t conn, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_get_bookmark");
if (cmyth_get_bookmark == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_bookmark_offset      = (int (*)(cmyth_database_t db, long chanid, long long mark, char *starttime, int mode))
dlsym(m_libcmyth, "cmyth_get_bookmark_offset");
if (cmyth_get_bookmark_offset == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_bookmark_mark      = (long long (*)(cmyth_database_t db, cmyth_proginfo_t prog, long long bk, int mode))
dlsym(m_libcmyth, "cmyth_get_bookmark_mark");
if (cmyth_get_bookmark_mark == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_set_bookmark      = (int (*)(cmyth_conn_t conn, cmyth_proginfo_t prog, long long bookmark))
dlsym(m_libcmyth, "cmyth_set_bookmark");
if (cmyth_set_bookmark == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_commbreaklist_create      = (cmyth_commbreaklist_t (*)(void))
dlsym(m_libcmyth, "cmyth_commbreaklist_create");
if (cmyth_commbreaklist_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_commbreak_create      = (cmyth_commbreak_t (*)(void))
dlsym(m_libcmyth, "cmyth_commbreak_create");
if (cmyth_commbreak_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_commbreaklist      = (cmyth_commbreaklist_t (*)(cmyth_conn_t conn, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_get_commbreaklist");
if (cmyth_get_commbreaklist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_cutlist      = (cmyth_commbreaklist_t (*)(cmyth_conn_t conn, cmyth_proginfo_t prog))
dlsym(m_libcmyth, "cmyth_get_cutlist");
if (cmyth_get_cutlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_rcv_commbreaklist      = (int (*)(cmyth_conn_t conn, int* err, cmyth_commbreaklist_t breaklist, int count))
dlsym(m_libcmyth, "cmyth_rcv_commbreaklist");
if (cmyth_rcv_commbreaklist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_inputlist_create      = (cmyth_inputlist_t (*)(void))
dlsym(m_libcmyth, "cmyth_inputlist_create");
if (cmyth_inputlist_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_input_create       = (cmyth_input_t (*)(void))
dlsym(m_libcmyth, "cmyth_input_create");
if (cmyth_input_create == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_free_inputlist      = (cmyth_inputlist_t (*)(cmyth_recorder_t rec))
dlsym(m_libcmyth, "cmyth_get_free_inputlist");
if (cmyth_get_free_inputlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_rcv_free_inputlist      = (int (*)(cmyth_conn_t conn, int* err, cmyth_inputlist_t inputlist, int count))
dlsym(m_libcmyth, "cmyth_rcv_free_inputlist");
if (cmyth_rcv_free_inputlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_recgroups      = (int (*)(cmyth_database_t, cmyth_recgroups_t**))
dlsym(m_libcmyth, "cmyth_mysql_get_recgroups");
if (cmyth_mysql_get_recgroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_delete_scheduled_recording      = (int (*)(cmyth_database_t db, char* query))
dlsym(m_libcmyth, "cmyth_mysql_delete_scheduled_recording");
if (cmyth_mysql_delete_scheduled_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_insert_into_record      = (int (*)(cmyth_database_t db, char* query, char* query1, char* query2, char* title, char* subtitle, char* description, char* callsign))
dlsym(m_libcmyth, "cmyth_mysql_insert_into_record");
if (cmyth_mysql_insert_into_record == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_recordid_mysql      = (char* (*)(cmyth_database_t, int, char* , char* , char* , char* , char* ))
dlsym(m_libcmyth, "cmyth_get_recordid_mysql");
if (cmyth_get_recordid_mysql == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_offset_mysql      = (int (*)(cmyth_database_t, int, char* , int, char* , char* , char* , char* , char* ))
dlsym(m_libcmyth, "cmyth_get_offset_mysql");
if (cmyth_get_offset_mysql == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_prog_finder_char_title      = (int (*)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, char* program_name))
dlsym(m_libcmyth, "cmyth_mysql_get_prog_finder_char_title");
if (cmyth_mysql_get_prog_finder_char_title == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_prog_finder_time      = (int (*)(cmyth_database_t db, cmyth_program_t**prog,  time_t starttime, char* program_name))
dlsym(m_libcmyth, "cmyth_mysql_get_prog_finder_time");
if (cmyth_mysql_get_prog_finder_time == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_guide      = (int (*)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, time_t endtime))
dlsym(m_libcmyth, "cmyth_mysql_get_guide");
if (cmyth_mysql_get_guide == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_testdb_connection      = (int (*)(cmyth_database_t db,char**message))
dlsym(m_libcmyth, "cmyth_mysql_testdb_connection");
if (cmyth_mysql_testdb_connection == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_schedule_recording      = (int (*)(cmyth_conn_t conn, char* msg))
dlsym(m_libcmyth, "cmyth_schedule_recording");
if (cmyth_schedule_recording == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_escape_chars      = (char* (*)(cmyth_database_t db, char* string))
dlsym(m_libcmyth, "cmyth_mysql_escape_chars");
if (cmyth_mysql_escape_chars == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_commbreak_list      = (int (*)(cmyth_database_t db, int chanid, char* start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version))
dlsym(m_libcmyth, "cmyth_mysql_get_commbreak_list");
if (cmyth_mysql_get_commbreak_list == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_prev_recorded      = (int (*)(cmyth_database_t db, cmyth_program_t**prog))
dlsym(m_libcmyth, "cmyth_mysql_get_prev_recorded");
if (cmyth_mysql_get_prev_recorded == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_get_delete_list      = (int (*)(cmyth_conn_t, char* , cmyth_proglist_t))
dlsym(m_libcmyth, "cmyth_get_delete_list");
if (cmyth_get_delete_list == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mythtv_remove_previous_recorded      = (int (*)(cmyth_database_t db,char* query))
dlsym(m_libcmyth, "cmyth_mythtv_remove_previous_recorded");
if (cmyth_mythtv_remove_previous_recorded == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_chanlist      = (cmyth_chanlist_t (*)(cmyth_database_t db))
dlsym(m_libcmyth, "cmyth_mysql_get_chanlist");
if (cmyth_mysql_get_chanlist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_is_radio      = (int (*)(cmyth_database_t db, int chanid))
dlsym(m_libcmyth, "cmyth_mysql_is_radio");
if (cmyth_mysql_is_radio == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_recordid      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_recordid");
if (cmyth_timer_recordid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_chanid      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_chanid");
if (cmyth_timer_chanid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_starttime      = (time_t (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_starttime");
if (cmyth_timer_starttime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_endtime      = (time_t (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_endtime");
if (cmyth_timer_endtime == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_title      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_title");
if (cmyth_timer_title == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_description      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_description");
if (cmyth_timer_description == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_type      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_type");
if (cmyth_timer_type == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_category      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_category");
if (cmyth_timer_category == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_subtitle      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_subtitle");
if (cmyth_timer_subtitle == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_priority      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_priority");
if (cmyth_timer_priority == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_startoffset      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_startoffset");
if (cmyth_timer_startoffset == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_endoffset      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_endoffset");
if (cmyth_timer_endoffset == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_searchtype      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_searchtype");
if (cmyth_timer_searchtype == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_inactive      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_inactive");
if (cmyth_timer_inactive == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_callsign      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_callsign");
if (cmyth_timer_callsign == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_dup_method      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_dup_method");
if (cmyth_timer_dup_method == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_dup_in      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_dup_in");
if (cmyth_timer_dup_in == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_rec_group      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_rec_group");
if (cmyth_timer_rec_group == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_store_group      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_store_group");
if (cmyth_timer_store_group == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_play_group      = (char* (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_play_group");
if (cmyth_timer_play_group == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_autotranscode      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_autotranscode");
if (cmyth_timer_autotranscode == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_userjobs      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_userjobs");
if (cmyth_timer_userjobs == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_autocommflag      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_autocommflag");
if (cmyth_timer_autocommflag == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_autoexpire      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_autoexpire");
if (cmyth_timer_autoexpire == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_maxepisodes      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_maxepisodes");
if (cmyth_timer_maxepisodes == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_maxnewest      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_maxnewest");
if (cmyth_timer_maxnewest == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timer_transcoder      = (int (*)(cmyth_timer_t timer))
dlsym(m_libcmyth, "cmyth_timer_transcoder");
if (cmyth_timer_transcoder == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timerlist_get_item      = (cmyth_timer_t (*)(cmyth_timerlist_t pl, int index))
dlsym(m_libcmyth, "cmyth_timerlist_get_item");
if (cmyth_timerlist_get_item == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_timerlist_get_count      = (int (*)(cmyth_timerlist_t pl))
dlsym(m_libcmyth, "cmyth_timerlist_get_count");
if (cmyth_timerlist_get_count == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_timers      = (cmyth_timerlist_t (*)(cmyth_database_t db))
dlsym(m_libcmyth, "cmyth_mysql_get_timers");
if (cmyth_mysql_get_timers == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_add_timer      = (int (*)(cmyth_database_t db, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category,int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder))
dlsym(m_libcmyth, "cmyth_mysql_add_timer");
if (cmyth_mysql_add_timer == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_delete_timer      = (int (*)(cmyth_database_t db, int recordid))
dlsym(m_libcmyth, "cmyth_mysql_delete_timer");
if (cmyth_mysql_delete_timer == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_update_timer      = (int (*)(cmyth_database_t db, int recordid, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category, int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder))
dlsym(m_libcmyth, "cmyth_mysql_update_timer");
if (cmyth_mysql_update_timer == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_channelgroups      = (int (*)(cmyth_database_t db,cmyth_channelgroups_t** changroups))
dlsym(m_libcmyth, "cmyth_mysql_get_channelgroups");
if (cmyth_mysql_get_channelgroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_channelids_in_group      = (int (*)(cmyth_database_t db,unsigned int groupid,int** chanids))
dlsym(m_libcmyth, "cmyth_mysql_get_channelids_in_group");
if (cmyth_mysql_get_channelids_in_group == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_sourceid      = (int (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_sourceid");
if (cmyth_channel_sourceid == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_channel_multiplex      = (int (*)(cmyth_channel_t channel))
dlsym(m_libcmyth, "cmyth_channel_multiplex");
if (cmyth_channel_multiplex == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_recorder_list      = (int (*)(cmyth_database_t db,cmyth_rec_t** reclist))
dlsym(m_libcmyth, "cmyth_mysql_get_recorder_list");
if (cmyth_mysql_get_recorder_list == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_prog_finder_time_title_chan      = (int (*)(cmyth_database_t db,cmyth_program_t* prog, char* title,time_t starttime,int chanid))
dlsym(m_libcmyth, "cmyth_mysql_get_prog_finder_time_title_chan");
if (cmyth_mysql_get_prog_finder_time_title_chan == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_storagegroups      = (int (*)(cmyth_database_t db, char*** profiles))
dlsym(m_libcmyth, "cmyth_mysql_get_storagegroups");
if (cmyth_mysql_get_storagegroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_playgroups      = (int (*)(cmyth_database_t db, char*** profiles))
dlsym(m_libcmyth, "cmyth_mysql_get_playgroups");
if (cmyth_mysql_get_playgroups == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_recprofiles      = (int (*)(cmyth_database_t db, cmyth_recprofile_t** profiles))
dlsym(m_libcmyth, "cmyth_mysql_get_recprofiles");
if (cmyth_mysql_get_recprofiles == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_mysql_get_cardtype      = (char* (*)(cmyth_database_t db, int chanid))
dlsym(m_libcmyth, "cmyth_mysql_get_cardtype");
if (cmyth_mysql_get_cardtype == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_storagegroup_filelist      = (int (*)(cmyth_conn_t control, char*** sgFilelist, char* sg2List, char*  mythostname))
dlsym(m_libcmyth, "cmyth_storagegroup_filelist");
if (cmyth_storagegroup_filelist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_storagegroup_filelist_get_item      = (cmyth_storagegroup_file_t (*)(cmyth_storagegroup_filelist_t fl, int index))
dlsym(m_libcmyth, "cmyth_storagegroup_filelist_get_item");
if (cmyth_storagegroup_filelist_get_item == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_storagegroup_filelist_count      = (int (*)(cmyth_storagegroup_filelist_t fl))
dlsym(m_libcmyth, "cmyth_storagegroup_filelist_count");
if (cmyth_storagegroup_filelist_count == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_storagegroup_get_filelist      = (cmyth_storagegroup_filelist_t (*)(cmyth_conn_t control,char* storagegroup, char* hostname))
dlsym(m_libcmyth, "cmyth_storagegroup_get_filelist");
if (cmyth_storagegroup_get_filelist == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_storagegroup_file_get_filename      = (char* (*)(cmyth_storagegroup_file_t file))
dlsym(m_libcmyth, "cmyth_storagegroup_file_get_filename");
if (cmyth_storagegroup_file_get_filename == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_storagegroup_file_get_lastmodified      = (unsigned long (*)(cmyth_storagegroup_file_t file))
dlsym(m_libcmyth, "cmyth_storagegroup_file_get_lastmodified");
if (cmyth_storagegroup_file_get_lastmodified == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    cmyth_storagegroup_file_get_size      = (unsigned long long (*)(cmyth_storagegroup_file_t file))
dlsym(m_libcmyth, "cmyth_storagegroup_file_get_size");
if (cmyth_storagegroup_file_get_size == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ref_release      = (void (*)(void* p))
dlsym(m_libcmyth, "ref_release");
if (ref_release == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ref_hold      = (void* (*)(void* p))
dlsym(m_libcmyth, "ref_hold");
if (ref_hold == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ref_strdup      = (char* (*)(char* str))
dlsym(m_libcmyth, "ref_strdup");
if (ref_strdup == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ref_realloc      = (void* (*)(void* p, size_t len))
dlsym(m_libcmyth, "ref_realloc");
if (ref_realloc == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ref_set_destroy      = (void (*)(void* block, ref_destroy_t func))
dlsym(m_libcmyth, "ref_set_destroy");
if (ref_set_destroy == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ref_alloc_show      = (void (*)(void))
dlsym(m_libcmyth, "ref_alloc_show");
if (ref_alloc_show == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

  return true;
  }

//dll functions

void (*cmyth_dbg_level)(int l);
void (*cmyth_dbg_all)(void);
void (*cmyth_dbg_none)(void);
void (*cmyth_dbg)(int level, char* fmt, ...);
void (*cmyth_set_dbg_msgcallback)(void (* msgcb)(int level,char* ));
cmyth_conn_t (*cmyth_conn_connect_ctrl)(char* server, unsigned short port, unsigned buflen, int tcp_rcvbuf);
int (*cmyth_conn_reconnect_ctrl)(cmyth_conn_t control);
cmyth_conn_t (*cmyth_conn_connect_event)(char* server,  unsigned short port,  unsigned buflen, int tcp_rcvbuf);
int (*cmyth_conn_reconnect_event)(cmyth_conn_t conn);
cmyth_file_t (*cmyth_conn_connect_file)(cmyth_proginfo_t prog, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf);
cmyth_file_t (*cmyth_conn_connect_path)(char* path, cmyth_conn_t control, unsigned buflen, int tcp_rcvbuf,char* sgToGetFrom);
int (*cmyth_conn_connect_ring)(cmyth_recorder_t rec, unsigned buflen,int tcp_rcvbuf);
int (*cmyth_conn_connect_recorder)(cmyth_recorder_t rec, unsigned buflen, int tcp_rcvbuf);
int (*cmyth_conn_check_block)(cmyth_conn_t conn, unsigned long size);
cmyth_recorder_t (*cmyth_conn_get_recorder_from_num)(cmyth_conn_t conn, int num);
cmyth_recorder_t (*cmyth_conn_get_free_recorder)(cmyth_conn_t conn);
int (*cmyth_conn_get_freespace)(cmyth_conn_t control, long long* total, long long* used);
int (*cmyth_conn_hung)(cmyth_conn_t control);
int (*cmyth_conn_get_free_recorder_count)(cmyth_conn_t conn);
int (*cmyth_conn_get_protocol_version)(cmyth_conn_t conn);
char* (*cmyth_conn_get_setting)(cmyth_conn_t conn,const char* hostname, const char* setting);
int (*cmyth_conn_set_setting)(cmyth_conn_t conn,const char* hostname, const char* setting, const char* value);
char* (*cmyth_conn_get_backend_hostname)(cmyth_conn_t conn);
char* (*cmyth_conn_get_client_hostname)(cmyth_conn_t conn);
cmyth_event_t (*cmyth_event_get)(cmyth_conn_t conn, char* data, int len);
int (*cmyth_event_select)(cmyth_conn_t conn, struct timeval* timeout);
cmyth_recorder_t (*cmyth_recorder_create)(void);
cmyth_recorder_t (*cmyth_recorder_dup)(cmyth_recorder_t p);
int (*cmyth_recorder_is_recording)(cmyth_recorder_t rec);
int (*cmyth_recorder_get_framerate)(cmyth_recorder_t rec,double* rate);
long long (*cmyth_recorder_get_frames_written)(cmyth_recorder_t rec);
long long (*cmyth_recorder_get_free_space)(cmyth_recorder_t rec);
long long (*cmyth_recorder_get_keyframe_pos)(cmyth_recorder_t rec, unsigned long keynum);
cmyth_posmap_t (*cmyth_recorder_get_position_map)(cmyth_recorder_t rec,unsigned long start,unsigned long end);
cmyth_proginfo_t (*cmyth_recorder_get_recording)(cmyth_recorder_t rec);
int (*cmyth_recorder_stop_playing)(cmyth_recorder_t rec);
int (*cmyth_recorder_frontend_ready)(cmyth_recorder_t rec);
int (*cmyth_recorder_cancel_next_recording)(cmyth_recorder_t rec);
int (*cmyth_recorder_pause)(cmyth_recorder_t rec);
int (*cmyth_recorder_finish_recording)(cmyth_recorder_t rec);
int (*cmyth_recorder_toggle_channel_favorite)(cmyth_recorder_t rec);
int (*cmyth_recorder_change_channel)(cmyth_recorder_t rec, cmyth_channeldir_t direction);
int (*cmyth_recorder_set_channel)(cmyth_recorder_t rec,char* channame);
int (*cmyth_recorder_change_color)(cmyth_recorder_t rec, cmyth_adjdir_t direction);
int (*cmyth_recorder_change_brightness)(cmyth_recorder_t rec, cmyth_adjdir_t direction);
int (*cmyth_recorder_change_contrast)(cmyth_recorder_t rec,  cmyth_adjdir_t direction);
int (*cmyth_recorder_change_hue)(cmyth_recorder_t rec,  cmyth_adjdir_t direction);
int (*cmyth_recorder_check_channel)(cmyth_recorder_t rec,char* channame);
int (*cmyth_recorder_check_channel_prefix)(cmyth_recorder_t rec, char* channame);
cmyth_proginfo_t (*cmyth_recorder_get_cur_proginfo)(cmyth_recorder_t rec);
cmyth_proginfo_t (*cmyth_recorder_get_next_proginfo)(cmyth_recorder_t rec,cmyth_proginfo_t current,cmyth_browsedir_t direction);
int (*cmyth_recorder_get_input_name)(cmyth_recorder_t rec, char* name, unsigned len);
long long (*cmyth_recorder_seek)(cmyth_recorder_t rec,  long long pos,  cmyth_whence_t whence,  long long curpos);
int (*cmyth_recorder_spawn_chain_livetv)(cmyth_recorder_t rec, char* channame);
int (*cmyth_recorder_spawn_livetv)(cmyth_recorder_t rec);
int (*cmyth_recorder_start_stream)(cmyth_recorder_t rec);
int (*cmyth_recorder_end_stream)(cmyth_recorder_t rec);
char* (*cmyth_recorder_get_filename)(cmyth_recorder_t rec);
int (*cmyth_recorder_stop_livetv)(cmyth_recorder_t rec);
int (*cmyth_recorder_done_ringbuf)(cmyth_recorder_t rec);
int (*cmyth_recorder_get_recorder_id)(cmyth_recorder_t rec);
cmyth_livetv_chain_t (*cmyth_livetv_chain_create)(char* chainid);
long long (*cmyth_livetv_chain_duration)(cmyth_recorder_t rec);
int (*cmyth_livetv_chain_switch)(cmyth_recorder_t rec, int dir);
int (*cmyth_livetv_chain_switch_last)(cmyth_recorder_t rec);
int (*cmyth_livetv_chain_update)(cmyth_recorder_t rec, char* chainid,int tcp_rcvbuf);
cmyth_recorder_t (*cmyth_spawn_live_tv)(cmyth_recorder_t rec,unsigned buflen,int tcp_rcvbuf,  void (* prog_update_callback)(cmyth_proginfo_t),char** err, char* channame);
cmyth_recorder_t (*cmyth_livetv_chain_setup)(cmyth_recorder_t old_rec, int tcp_rcvbuf, void (* prog_update_callback)(cmyth_proginfo_t));
int (*cmyth_livetv_get_block)(cmyth_recorder_t rec, char* buf, unsigned long len);
int (*cmyth_livetv_select)(cmyth_recorder_t rec, struct timeval* timeout);
int (*cmyth_livetv_request_block)(cmyth_recorder_t rec, unsigned long len);
long long (*cmyth_livetv_seek)(cmyth_recorder_t rec,long long offset, int whence);
int (*cmyth_livetv_read)(cmyth_recorder_t rec,  char* buf,  unsigned long len);
int (*cmyth_livetv_keep_recording)(cmyth_recorder_t rec, cmyth_database_t db, int keep);
cmyth_database_t (*cmyth_database_init)(char* host, char* db_name, char* user, char* pass);
void (*cmyth_database_close)(cmyth_database_t db);
int (*cmyth_database_set_host)(cmyth_database_t db, char* host);
int (*cmyth_database_set_user)(cmyth_database_t db, char* user);
int (*cmyth_database_set_pass)(cmyth_database_t db, char* pass);
int (*cmyth_database_set_name)(cmyth_database_t db, char* name);
int (*cmyth_set_watched_status_mysql)(cmyth_database_t db, cmyth_proginfo_t prog, int watchedStat);
char* (*cmyth_ringbuf_pathname)(cmyth_recorder_t rec);
cmyth_ringbuf_t (*cmyth_ringbuf_create)(void);
cmyth_recorder_t (*cmyth_ringbuf_setup)(cmyth_recorder_t old_rec);
int (*cmyth_ringbuf_request_block)(cmyth_recorder_t rec, unsigned long len);
int (*cmyth_ringbuf_select)(cmyth_recorder_t rec, struct timeval* timeout);
int (*cmyth_ringbuf_get_block)(cmyth_recorder_t rec,char* buf,unsigned long len);
long long (*cmyth_ringbuf_seek)(cmyth_recorder_t rec, long long offset, int whence);
int (*cmyth_ringbuf_read)(cmyth_recorder_t rec,char* buf,unsigned long len);
cmyth_rec_num_t (*cmyth_rec_num_create)(void);
cmyth_rec_num_t (*cmyth_rec_num_get)(char* host, unsigned short port, unsigned id);
char* (*cmyth_rec_num_string)(cmyth_rec_num_t rn);
cmyth_timestamp_t (*cmyth_timestamp_create)(void);
cmyth_timestamp_t (*cmyth_timestamp_from_string)(char* str);
cmyth_timestamp_t (*cmyth_timestamp_from_unixtime)(time_t l);
time_t (*cmyth_timestamp_to_unixtime)(cmyth_timestamp_t ts);
int (*cmyth_timestamp_to_string)(char* str, cmyth_timestamp_t ts);
int (*cmyth_timestamp_to_isostring)(char* str, cmyth_timestamp_t ts);
int (*cmyth_timestamp_to_display_string)(char* str, cmyth_timestamp_t ts, int time_format_12);
int (*cmyth_datetime_to_string)(char* str, cmyth_timestamp_t ts);
int (*cmyth_timestamp_compare)(cmyth_timestamp_t ts1,cmyth_timestamp_t ts2);
cmyth_keyframe_t (*cmyth_keyframe_create)(void);
char* (*cmyth_keyframe_string)(cmyth_keyframe_t kf);
cmyth_posmap_t (*cmyth_posmap_create)(void);
cmyth_proginfo_t (*cmyth_proginfo_create)(void);
int (*cmyth_proginfo_stop_recording)(cmyth_conn_t control, cmyth_proginfo_t prog);
int (*cmyth_proginfo_check_recording)(cmyth_conn_t control,  cmyth_proginfo_t prog);
int (*cmyth_proginfo_delete_recording)(cmyth_conn_t control,cmyth_proginfo_t prog);
int (*cmyth_proginfo_forget_recording)(cmyth_conn_t control,cmyth_proginfo_t prog);
int (*cmyth_proginfo_get_recorder_num)(cmyth_conn_t control,cmyth_rec_num_t rnum,cmyth_proginfo_t prog);
cmyth_proginfo_t (*cmyth_proginfo_get_from_basename)(cmyth_conn_t control, const char* basename);
char* (*cmyth_proginfo_title)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_subtitle)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_description)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_category)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_chanstr)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_chansign)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_channame)(cmyth_proginfo_t prog);
long (*cmyth_proginfo_chan_id)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_pathname)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_seriesid)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_programid)(cmyth_proginfo_t prog);
unsigned long (*cmyth_proginfo_recordid)(cmyth_proginfo_t prog);
long (*cmyth_proginfo_priority)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_stars)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*cmyth_proginfo_rec_start)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*cmyth_proginfo_rec_end)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*cmyth_proginfo_originalairdate)(cmyth_proginfo_t prog);
cmyth_proginfo_rec_status_t (*cmyth_proginfo_rec_status)(cmyth_proginfo_t prog);
unsigned long (*cmyth_proginfo_flags)(  cmyth_proginfo_t prog);
long long (*cmyth_proginfo_length)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_host)(cmyth_proginfo_t prog);
int (*cmyth_proginfo_compare)(cmyth_proginfo_t a, cmyth_proginfo_t b);
int (*cmyth_proginfo_length_sec)(cmyth_proginfo_t prog);
cmyth_proginfo_t (*cmyth_proginfo_get_detail)(cmyth_conn_t control,  cmyth_proginfo_t p);
cmyth_timestamp_t (*cmyth_proginfo_start)(cmyth_proginfo_t prog);
cmyth_timestamp_t (*cmyth_proginfo_end)(cmyth_proginfo_t prog);
long (*cmyth_proginfo_card_id)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_recgroup)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_chanicon)(cmyth_proginfo_t prog);
char* (*cmyth_proginfo_year)(cmyth_proginfo_t prog);
cmyth_proglist_t (*cmyth_proglist_create)(void);
cmyth_proglist_t (*cmyth_proglist_get_all_recorded)(cmyth_conn_t control);
cmyth_proglist_t (*cmyth_proglist_get_all_pending)(cmyth_conn_t control);
cmyth_proglist_t (*cmyth_proglist_get_all_scheduled)(cmyth_conn_t control);
cmyth_proglist_t (*cmyth_proglist_get_conflicting)(cmyth_conn_t control);
cmyth_proginfo_t (*cmyth_proglist_get_item)(cmyth_proglist_t pl,int index);
int (*cmyth_proglist_delete_item)(cmyth_proglist_t pl,cmyth_proginfo_t prog);
int (*cmyth_proglist_get_count)(cmyth_proglist_t pl);
int (*cmyth_proglist_sort)(cmyth_proglist_t pl, int count, cmyth_proglist_sort_t sort);
cmyth_conn_t (*cmyth_file_data)(cmyth_file_t file);
unsigned long long (*cmyth_file_start)(cmyth_file_t file);
unsigned long long (*cmyth_file_length)(cmyth_file_t file);
unsigned long long (*cmyth_file_position)(cmyth_file_t file);
int (*cmyth_update_file_length)(cmyth_file_t file, unsigned long long newLength);
int (*cmyth_file_get_block)(cmyth_file_t file, char* buf,unsigned long len);
int (*cmyth_file_request_block)(cmyth_file_t file, unsigned long len);
long long (*cmyth_file_seek)(cmyth_file_t file, long long offset, int whence);
int (*cmyth_file_select)(cmyth_file_t file, struct timeval* timeout);
void (*cmyth_file_set_closed_callback)(cmyth_file_t file,void (* callback)(cmyth_file_t));
int (*cmyth_file_read)(cmyth_file_t file,char* buf,unsigned long len);
long (*cmyth_channel_chanid)(cmyth_channel_t channel);
long (*cmyth_channel_channum)(cmyth_channel_t channel);
char* (*cmyth_channel_channumstr)(cmyth_channel_t channel);
char* (*cmyth_channel_callsign)(cmyth_channel_t channel);
char* (*cmyth_channel_name)(cmyth_channel_t channel);
char* (*cmyth_channel_icon)(cmyth_channel_t channel);
int (*cmyth_channel_visible)(cmyth_channel_t channel);
cmyth_channel_t (*cmyth_chanlist_get_item)(cmyth_chanlist_t pl, int index);
int (*cmyth_chanlist_get_count)(cmyth_chanlist_t pl);
cmyth_freespace_t (*cmyth_freespace_create)(void);
long long (*cmyth_get_bookmark)(cmyth_conn_t conn, cmyth_proginfo_t prog);
int (*cmyth_get_bookmark_offset)(cmyth_database_t db, long chanid, long long mark, char *starttime, int mode);
long long (*cmyth_get_bookmark_mark)(cmyth_database_t db, cmyth_proginfo_t prog, long long bk, int mode);
int (*cmyth_set_bookmark)(cmyth_conn_t conn, cmyth_proginfo_t prog,long long bookmark);
cmyth_commbreaklist_t (*cmyth_commbreaklist_create)(void);
cmyth_commbreak_t (*cmyth_commbreak_create)(void);
cmyth_commbreaklist_t (*cmyth_get_commbreaklist)(cmyth_conn_t conn, cmyth_proginfo_t prog);
cmyth_commbreaklist_t (*cmyth_get_cutlist)(cmyth_conn_t conn, cmyth_proginfo_t prog);
int (*cmyth_rcv_commbreaklist)(cmyth_conn_t conn, int* err, cmyth_commbreaklist_t breaklist, int count);
cmyth_inputlist_t (*cmyth_inputlist_create)(void);
cmyth_input_t (*cmyth_input_create)(void);
cmyth_inputlist_t (*cmyth_get_free_inputlist)(cmyth_recorder_t rec);
int (*cmyth_rcv_free_inputlist)(cmyth_conn_t conn, int* err, cmyth_inputlist_t inputlist, int count);
int (*cmyth_mysql_get_recgroups)(cmyth_database_t, cmyth_recgroups_t**);
int (*cmyth_mysql_delete_scheduled_recording)(cmyth_database_t db, char* query);
int (*cmyth_mysql_insert_into_record)(cmyth_database_t db, char* query, char* query1, char* query2, char* title, char* subtitle, char* description, char* callsign);
char* (*cmyth_get_recordid_mysql)(cmyth_database_t, int, char* , char* , char* , char* , char* );
int (*cmyth_get_offset_mysql)(cmyth_database_t, int, char* , int, char* , char* , char* , char* , char* );
int (*cmyth_mysql_get_prog_finder_char_title)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, char* program_name);
int (*cmyth_mysql_get_prog_finder_time)(cmyth_database_t db, cmyth_program_t**prog,  time_t starttime, char* program_name);
int (*cmyth_mysql_get_guide)(cmyth_database_t db, cmyth_program_t**prog, time_t starttime, time_t endtime);
int (*cmyth_mysql_testdb_connection)(cmyth_database_t db,char**message);
int (*cmyth_schedule_recording)(cmyth_conn_t conn, char* msg);
char* (*cmyth_mysql_escape_chars)(cmyth_database_t db, char* string);
int (*cmyth_mysql_get_commbreak_list)(cmyth_database_t db, int chanid, char* start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version);
int (*cmyth_mysql_get_prev_recorded)(cmyth_database_t db, cmyth_program_t**prog);
int (*cmyth_get_delete_list)(cmyth_conn_t, char* , cmyth_proglist_t);
int (*cmyth_mythtv_remove_previous_recorded)(cmyth_database_t db,char* query);
cmyth_chanlist_t (*cmyth_mysql_get_chanlist)(cmyth_database_t db);
int (*cmyth_mysql_is_radio)(cmyth_database_t db, int chanid);
int (*cmyth_timer_recordid)(cmyth_timer_t timer);
int (*cmyth_timer_chanid)(cmyth_timer_t timer);
time_t (*cmyth_timer_starttime)(cmyth_timer_t timer);
time_t (*cmyth_timer_endtime)(cmyth_timer_t timer);
char* (*cmyth_timer_title)(cmyth_timer_t timer);
char* (*cmyth_timer_description)(cmyth_timer_t timer);
int (*cmyth_timer_type)(cmyth_timer_t timer);
char* (*cmyth_timer_category)(cmyth_timer_t timer);
char* (*cmyth_timer_subtitle)(cmyth_timer_t timer);
int (*cmyth_timer_priority)(cmyth_timer_t timer);
int (*cmyth_timer_startoffset)(cmyth_timer_t timer);
int (*cmyth_timer_endoffset)(cmyth_timer_t timer);
int (*cmyth_timer_searchtype)(cmyth_timer_t timer);
int (*cmyth_timer_inactive)(cmyth_timer_t timer);
char* (*cmyth_timer_callsign)(cmyth_timer_t timer);
int (*cmyth_timer_dup_method)(cmyth_timer_t timer);
int (*cmyth_timer_dup_in)(cmyth_timer_t timer);
char* (*cmyth_timer_rec_group)(cmyth_timer_t timer);
char* (*cmyth_timer_store_group)(cmyth_timer_t timer);
char* (*cmyth_timer_play_group)(cmyth_timer_t timer);
int (*cmyth_timer_autotranscode)(cmyth_timer_t timer);
int (*cmyth_timer_userjobs)(cmyth_timer_t timer);
int (*cmyth_timer_autocommflag)(cmyth_timer_t timer);
int (*cmyth_timer_autoexpire)(cmyth_timer_t timer);
int (*cmyth_timer_maxepisodes)(cmyth_timer_t timer);
int (*cmyth_timer_maxnewest)(cmyth_timer_t timer);
int (*cmyth_timer_transcoder)(cmyth_timer_t timer);
cmyth_timer_t (*cmyth_timerlist_get_item)(cmyth_timerlist_t pl, int index);
int (*cmyth_timerlist_get_count)(cmyth_timerlist_t pl);
cmyth_timerlist_t (*cmyth_mysql_get_timers)(cmyth_database_t db);
int (*cmyth_mysql_add_timer)(cmyth_database_t db, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category,int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder);
int (*cmyth_mysql_delete_timer)(cmyth_database_t db, int recordid);
int (*cmyth_mysql_update_timer)(cmyth_database_t db, int recordid, int chanid,char* callsign,char* description, time_t starttime, time_t endtime,char* title,char* category, int type,char* subtitle,int priority,int startoffset,int endoffset,int searchtype,int inactive,  int dup_method, int dup_in, char* rec_group, char* store_group, char* play_group, int autotranscode, int userjobs, int autocommflag, int autoexpire, int maxepisodes, int maxnewest, int transcoder);
int (*cmyth_mysql_get_channelgroups)(cmyth_database_t db,cmyth_channelgroups_t** changroups);
int (*cmyth_mysql_get_channelids_in_group)(cmyth_database_t db,unsigned int groupid,int** chanids);
int (*cmyth_channel_sourceid)(cmyth_channel_t channel);
int (*cmyth_channel_multiplex)(cmyth_channel_t channel);
int (*cmyth_mysql_get_recorder_list)(cmyth_database_t db,cmyth_rec_t** reclist);
int (*cmyth_mysql_get_prog_finder_time_title_chan)(cmyth_database_t db,cmyth_program_t* prog, char* title,time_t starttime,int chanid);
int (*cmyth_mysql_get_storagegroups)(cmyth_database_t db, char*** profiles);
int (*cmyth_mysql_get_playgroups)(cmyth_database_t db, char*** profiles);
int (*cmyth_mysql_get_recprofiles)(cmyth_database_t db, cmyth_recprofile_t** profiles);
char* (*cmyth_mysql_get_cardtype)(cmyth_database_t db, int chanid);
int (*cmyth_storagegroup_filelist)(cmyth_conn_t control, char*** sgFilelist, char* sg2List, char*  mythostname);
cmyth_storagegroup_file_t (*cmyth_storagegroup_filelist_get_item)(cmyth_storagegroup_filelist_t fl, int index);
int (*cmyth_storagegroup_filelist_count)(cmyth_storagegroup_filelist_t fl);
cmyth_storagegroup_filelist_t (*cmyth_storagegroup_get_filelist)(cmyth_conn_t control,char* storagegroup, char* hostname);
char* (*cmyth_storagegroup_file_get_filename)(cmyth_storagegroup_file_t file);
unsigned long (*cmyth_storagegroup_file_get_lastmodified)(cmyth_storagegroup_file_t file);
unsigned long long (*cmyth_storagegroup_file_get_size)(cmyth_storagegroup_file_t file);
void (*ref_release)(void* p);
void* (*ref_hold)(void* p);
char* (*ref_strdup)(char* str);
void* (*ref_realloc)(void* p, size_t len);
void (*ref_set_destroy)(void* block, ref_destroy_t func);
void (*ref_alloc_show)(void);





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
