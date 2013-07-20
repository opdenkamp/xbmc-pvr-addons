/*
 *  Copyright (C) 2004-2012, Eric Lund, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*! \mainpage cmyth
 *
 * cmyth is a library that provides a C language API to access and control
 * a MythTV backend.
 *
 * \section projectweb Project website
 * http://www.mvpmc.org/
 *
 * \section repos Source repositories
 * http://git.mvpmc.org/
 *
 * \section libraries Libraries
 * \li \link cmyth.h libcmyth \endlink
 * \li \link refmem.h librefmem \endlink
 */

/** \file cmyth.h
 * A C library for communicating with a MythTV server.
 */

#ifndef __CMYTH_H
#define __CMYTH_H

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <stdint.h>

/*
 * -----------------------------------------------------------------
 * Types
 * -----------------------------------------------------------------
 */
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
	ANN_NONE = 0,
	ANN_MONITOR = 1,
	ANN_PLAYBACK,
} cmyth_conn_ann_t;

typedef enum {
	CHANNEL_DIRECTION_UP = 0,
	CHANNEL_DIRECTION_DOWN = 1,
	CHANNEL_DIRECTION_FAVORITE = 2,
	CHANNEL_DIRECTION_SAME = 4,
} cmyth_channeldir_t;

typedef enum {
	ADJ_DIRECTION_UP = 1,
	ADJ_DIRECTION_DOWN = 0,
} cmyth_adjdir_t;

typedef enum {
	BROWSE_DIRECTION_SAME = 0,
	BROWSE_DIRECTION_UP = 1,
	BROWSE_DIRECTION_DOWN = 2,
	BROWSE_DIRECTION_LEFT = 3,
	BROWSE_DIRECTION_RIGHT = 4,
	BROWSE_DIRECTION_FAVORITE = 5,
} cmyth_browsedir_t;

typedef enum {
	WHENCE_SET = 0,
	WHENCE_CUR = 1,
	WHENCE_END = 2,
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
	CMYTH_EVENT_LIVETV_WATCH,
	CMYTH_EVENT_LIVETV_CHAIN_UPDATE,
	CMYTH_EVENT_SIGNAL,
	CMYTH_EVENT_ASK_RECORDING,
	CMYTH_EVENT_SYSTEM_EVENT,
	CMYTH_EVENT_UPDATE_FILE_SIZE,
	CMYTH_EVENT_GENERATED_PIXMAP,
	CMYTH_EVENT_CLEAR_SETTINGS_CACHE,
} cmyth_event_t;

#define CMYTH_NUM_SORTS 2
typedef enum {
	MYTHTV_SORT_DATE_RECORDED = 0,
	MYTHTV_SORT_ORIGINAL_AIRDATE,
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
        int64_t start_mark;
        int64_t start_offset;
        int64_t end_mark;
        int64_t end_offset;
};
typedef struct cmyth_commbreak *cmyth_commbreak_t;

struct cmyth_commbreaklist {
        cmyth_commbreak_t *commbreak_list;
        int commbreak_count;
};
typedef struct cmyth_commbreaklist *cmyth_commbreaklist_t;

/*
 * -----------------------------------------------------------------
 * Debug Output Control
 * -----------------------------------------------------------------
 */

/*
 * Debug level constants used to determine the level of debug tracing
 * to be done and the debug level of any given message.
 */

#define CMYTH_DBG_NONE  -1
#define CMYTH_DBG_ERROR  0
#define CMYTH_DBG_WARN   1
#define CMYTH_DBG_INFO   2
#define CMYTH_DBG_DETAIL 3
#define CMYTH_DBG_DEBUG  4
#define CMYTH_DBG_PROTO  5
#define CMYTH_DBG_ALL    6

/**
 * Set the libcmyth debug level.
 * \param l level
 */
extern void cmyth_dbg_level(int l);

/**
 * Turn on all libcmyth debugging.
 */
extern void cmyth_dbg_all(void);

/**
 * Turn off all libcmyth debugging.
 */
extern void cmyth_dbg_none(void);

/**
 * Print a libcmyth debug message.
 * \param level debug level
 * \param fmt printf style format
 */
extern void cmyth_dbg(int level, char *fmt, ...);

/**
 * Define a callback to use to send messages rather than using stdout
 * \param msgcb function pointer to pass a string to
 */
extern void cmyth_set_dbg_msgcallback(void (*msgcb)(int level,char *));

/*
 * -----------------------------------------------------------------
 * Connection Operations
 * -----------------------------------------------------------------
 */

/**
 * Create a control connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return control handle
 */
extern cmyth_conn_t cmyth_conn_connect_ctrl(char *server,
					    uint16_t port,
					    uint32_t buflen, int32_t tcp_rcvbuf);

/**
 * Reconnect control connection to a backend.
 * \param control control handle
 * \return success: 1
 * \return failure: 0
 */
extern int cmyth_conn_reconnect_ctrl(cmyth_conn_t control);

/**
 * Create an event connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return event handle
 */
extern cmyth_conn_t cmyth_conn_connect_event(char *server,
					     uint16_t port,
					     uint32_t buflen, int32_t tcp_rcvbuf);

/**
 * Reconnect event connection to a backend.
 * \param conn control handle
 * \return success: 1
 * \return failure: 0
 */
extern int cmyth_conn_reconnect_event(cmyth_conn_t conn);

/**
 * Create a monitor connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return control handle
 */
extern cmyth_conn_t cmyth_conn_connect_monitor(char *server,
					    uint16_t port,
					    uint32_t buflen, int32_t tcp_rcvbuf);

/**
 * Reconnect monitor connection to a backend.
 * \param control control handle
 * \return success: 1
 * \return failure: 0
 */
extern int cmyth_conn_reconnect_monitor(cmyth_conn_t control);

/**
 * Create a playback connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return control handle
 */
extern cmyth_conn_t cmyth_conn_connect_playback(char *server,
					    uint16_t port,
					    uint32_t buflen, int32_t tcp_rcvbuf);

/**
 * Reconnect playback connection to a backend.
 * \param control control handle
 * \return success: 1
 * \return failure: 0
 */
extern int cmyth_conn_reconnect_playback(cmyth_conn_t control);

/**
 * Create a file connection to a backend for reading a recording.
 * \param prog program handle
 * \param control control handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return file handle
 */
extern cmyth_file_t cmyth_conn_connect_file(cmyth_proginfo_t prog,
					    cmyth_conn_t control,
					    uint32_t buflen, int32_t tcp_rcvbuf);

/**
 * Create a file connection to a backend.
 * \param path path to file
 * \param control control handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \param storage_group storage group to get from
 * \return file handle
 */
extern cmyth_file_t cmyth_conn_connect_path(char* path, cmyth_conn_t control,
					    uint32_t buflen, int32_t tcp_rcvbuf, char* storage_group);

/**
 * Create a ring buffer connection to a recorder.
 * \param rec recorder handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_conn_connect_ring(cmyth_recorder_t rec, uint32_t buflen,
				   int32_t tcp_rcvbuf);

/**
 * Create a connection to a recorder.
 * \param rec recorder to connect to
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_conn_connect_recorder(cmyth_recorder_t rec,
				       uint32_t buflen, int32_t tcp_rcvbuf);

/**
 * Check whether a block has finished transfering from a backend.
 * \param conn control handle
 * \param size size of block
 * \return complete: 1
 * \return not complete: 0
 * \return failure: -(errno)
 */
extern int cmyth_conn_check_block(cmyth_conn_t conn, uint32_t size);

/**
 * Obtain a recorder from a connection by its recorder number.
 * \param conn connection handle
 * \param num recorder number
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_conn_get_recorder_from_num(cmyth_conn_t conn,
							 int32_t id);

/**
 * Obtain the next available free recorder on a backend.
 * \param conn connection handle
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_conn_get_free_recorder(cmyth_conn_t conn);

/**
 * Get the amount of free disk space on a backend.
 * \param control control handle
 * \param[out] total total disk space
 * \param[out] used used disk space
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_conn_get_freespace(cmyth_conn_t control,
				    int64_t *total, int64_t *used);

/**
 * Determine if a control connection is not responding.
 * \param control control handle
 * \return not hung: 0
 * \return hung: 1
 * \return failure: -(errno)
 */
extern int cmyth_conn_hung(cmyth_conn_t control);

/**
 * Determine the number of free recorders.
 * \param conn connection handle
 * \return number of free recorders
 */
extern int32_t cmyth_conn_get_free_recorder_count(cmyth_conn_t conn);

/**
 * Determine the MythTV protocol version being used.
 * \param conn connection handle
 * \return protocol version
 */
extern int32_t cmyth_conn_get_protocol_version(cmyth_conn_t conn);

/**
 * Return a MythTV setting for a hostname
 * \param conn connection handle
 * \param hostname hostname to retreive the setting from
 * \param setting the setting name to get
 * \return ref counted string with the setting
 */
extern char * cmyth_conn_get_setting(cmyth_conn_t conn,
               const char* hostname, const char* setting);

/**
 * Set a MythTV setting for a hostname
 * \param conn connection handle
 * \param hostname hostname to apply the setting to
 * \param setting the setting name to set
 * \param value the value of the setting
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_conn_set_setting(cmyth_conn_t conn,
               const char* hostname, const char* setting, const char* value);

/**
 * Get the backend hostname used in the settings database
 * \param conn connection handle
 * \return ref counted string with backend hostname
 */
extern char* cmyth_conn_get_backend_hostname(cmyth_conn_t conn);

/**
 * Get the client hostname used when creating the backend connection
 * \param conn connection handle
 * \retval ref counted string with client hostname
 */
extern char* cmyth_conn_get_client_hostname(cmyth_conn_t conn);

/**
 * Issues a run of the re-scheduler
 * \param conn connection handle
 * \param recordid or -1 (0xFFFFFFFF) performs a full run
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_conn_reschedule_recordings(cmyth_conn_t conn, uint32_t recordid);

/*
 * -----------------------------------------------------------------
 * Event Operations
 * -----------------------------------------------------------------
 */

/**
 * Retrieve an event from a backend.
 * \param conn connection handle
 * \param[out] data data, if the event returns any
 * \param len size of data buffer
 * \return event type
 */
extern cmyth_event_t cmyth_event_get(cmyth_conn_t conn, char * data, int32_t len);

/**
 * Selects on the event socket, waiting for an event to show up.
 * allows nonblocking access to events.
 * \return success: >0
 * \retutn timeout: 0
 * \return failure: -(errno)
 */
extern int cmyth_event_select(cmyth_conn_t conn, struct timeval *timeout);

/**
 * Retrieve an event from a backend.
 * \param conn connection handle
 * \param[out] data data, if the event returns any
 * \param len size of data buffer
 * \return event message handle
 */
extern cmyth_event_t cmyth_event_get_message(cmyth_conn_t conn, char * data, int32_t len, cmyth_proginfo_t * proginfo);

/*
 * -----------------------------------------------------------------
 * Recorder Operations
 * -----------------------------------------------------------------
 */

/**
 * Create a new recorder.
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_recorder_create(void);

/**
 * Duplicaate a recorder.
 * \param p recorder handle
 * \return duplicated recorder handle
 */
extern cmyth_recorder_t cmyth_recorder_dup(cmyth_recorder_t p);

/**
 * Determine if a recorder is in use.
 * \param rec recorder handle
 * \return recording: 1
 * \return not recording: 0
 * \return failure: -(errno)
 */
extern int cmyth_recorder_is_recording(cmyth_recorder_t rec);

/**
 * Determine the framerate for a recorder.
 * \param rec recorder handle
 * \param[out] rate framerate
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_recorder_get_framerate(cmyth_recorder_t rec,
					double *rate);

extern int64_t cmyth_recorder_get_frames_written(cmyth_recorder_t rec);

extern int64_t cmyth_recorder_get_free_space(cmyth_recorder_t rec);

extern int64_t cmyth_recorder_get_keyframe_pos(cmyth_recorder_t rec,
						 uint32_t keynum);

extern cmyth_posmap_t cmyth_recorder_get_position_map(cmyth_recorder_t rec,
						      uint32_t start,
						      uint32_t end);

extern cmyth_proginfo_t cmyth_recorder_get_recording(cmyth_recorder_t rec);

extern int cmyth_recorder_stop_playing(cmyth_recorder_t rec);

extern int cmyth_recorder_frontend_ready(cmyth_recorder_t rec);

/**
 * Cancel the next pending recording.
 * \param rec recorder handle
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_recorder_cancel_next_recording(cmyth_recorder_t rec, int cancel);

/**
 * Request that the recorder stop transmitting data.
 * \param rec recorder handle
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_recorder_pause(cmyth_recorder_t rec);

extern int cmyth_recorder_finish_recording(cmyth_recorder_t rec);

extern int cmyth_recorder_toggle_channel_favorite(cmyth_recorder_t rec);

/**
 * Request that the recorder change the channel being recorded.
 * \param rec recorder handle
 * \param direction direction in which to change channel
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_recorder_change_channel(cmyth_recorder_t rec,
					 cmyth_channeldir_t direction);

/**
 * Set the channel for a recorder.
 * \param rec recorder handle
 * \param channame channel name to change to
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_recorder_set_channel(cmyth_recorder_t rec,
				      char *channame);

extern int cmyth_recorder_change_color(cmyth_recorder_t rec,
				       cmyth_adjdir_t direction);

extern int cmyth_recorder_change_brightness(cmyth_recorder_t rec,
					    cmyth_adjdir_t direction);

extern int cmyth_recorder_change_contrast(cmyth_recorder_t rec,
					  cmyth_adjdir_t direction);

extern int cmyth_recorder_change_hue(cmyth_recorder_t rec,
				     cmyth_adjdir_t direction);

extern int cmyth_recorder_check_channel(cmyth_recorder_t rec,
					char *channame);

extern int cmyth_recorder_check_channel_prefix(cmyth_recorder_t rec,
					       char *channame);

/**
 * Request the current program info for a recorder.
 * \param rec recorder handle
 * \return program info handle
 */
extern cmyth_proginfo_t cmyth_recorder_get_cur_proginfo(cmyth_recorder_t rec);

/**
 * Request the next program info for a recorder.
 * \param rec recorder handle
 * \param current current program
 * \param direction direction of next program
 * \return success: 0
 * \return failure: -(errno)
 */
extern cmyth_proginfo_t cmyth_recorder_get_next_proginfo(
	cmyth_recorder_t rec,
	cmyth_proginfo_t current,
	cmyth_browsedir_t direction);

extern int cmyth_recorder_get_input_name(cmyth_recorder_t rec,
					 char *name,
					 unsigned len);

extern int64_t cmyth_recorder_seek(cmyth_recorder_t rec,
				     int64_t pos,
				     int8_t whence,
				     int64_t curpos);

extern int cmyth_recorder_spawn_chain_livetv(cmyth_recorder_t rec, char* channame);

extern int cmyth_recorder_spawn_livetv(cmyth_recorder_t rec);

extern int cmyth_recorder_start_stream(cmyth_recorder_t rec);

extern int cmyth_recorder_end_stream(cmyth_recorder_t rec);
extern char*cmyth_recorder_get_filename(cmyth_recorder_t rec);
extern int cmyth_recorder_stop_livetv(cmyth_recorder_t rec);
extern int cmyth_recorder_done_ringbuf(cmyth_recorder_t rec);
extern uint32_t cmyth_recorder_get_recorder_id(cmyth_recorder_t rec);

/*
 * -----------------------------------------------------------------
 * Live TV Operations
 * -----------------------------------------------------------------
 */

extern cmyth_livetv_chain_t cmyth_livetv_chain_create(char * chainid);

extern cmyth_file_t cmyth_livetv_get_cur_file(cmyth_recorder_t rec);

extern int64_t cmyth_livetv_chain_duration(cmyth_recorder_t rec);

extern int cmyth_livetv_chain_switch(cmyth_recorder_t rec, int dir);

extern int cmyth_livetv_chain_switch_last(cmyth_recorder_t rec);

extern int cmyth_livetv_chain_update(cmyth_recorder_t rec, char * chainid);

/* JLB: Manage program breaks */
extern int cmyth_livetv_watch(cmyth_recorder_t rec, char * msg);

/* JLB: Manage program breaks */
extern int cmyth_livetv_done_recording(cmyth_recorder_t rec, char * msg);

extern cmyth_recorder_t cmyth_spawn_live_tv(cmyth_recorder_t rec,
				uint32_t buflen,
				int32_t tcp_rcvbuf,
				void (*prog_update_callback)(cmyth_proginfo_t),
				char ** err, char * channame);

extern cmyth_recorder_t cmyth_livetv_chain_setup(cmyth_recorder_t old_rec,
				uint32_t buflen, int32_t tcp_rcvbuf,
				void (*prog_update_callback)(cmyth_proginfo_t));

extern int32_t cmyth_livetv_get_block(cmyth_recorder_t rec, char *buf,
                                int32_t len);

extern int cmyth_livetv_select(cmyth_recorder_t rec, struct timeval *timeout);

extern int32_t cmyth_livetv_request_block(cmyth_recorder_t rec, int32_t len);

extern int64_t cmyth_livetv_seek(cmyth_recorder_t rec,
				int64_t offset, int8_t whence);

extern int32_t cmyth_livetv_read(cmyth_recorder_t rec,
				char *buf,
				int32_t len);

extern int cmyth_mysql_tuner_type_check(cmyth_database_t db, cmyth_recorder_t rec, int check_tuner_enabled);

extern int cmyth_mysql_keep_livetv_recording(cmyth_database_t db, cmyth_proginfo_t prog, int8_t keep);

/*
 * -----------------------------------------------------------------
 * Database Operations
 * -----------------------------------------------------------------
 */

extern cmyth_database_t cmyth_database_init(char *host, char *db_name, char *user, char *pass, unsigned short port);
extern void             cmyth_database_close(cmyth_database_t db);

extern int cmyth_database_set_host(cmyth_database_t db, char *host);
extern int cmyth_database_set_user(cmyth_database_t db, char *user);
extern int cmyth_database_set_pass(cmyth_database_t db, char *pass);
extern int cmyth_database_set_name(cmyth_database_t db, char *name);
extern int cmyth_database_set_port(cmyth_database_t db, uint16_t port);

extern uint32_t cmyth_database_get_version(cmyth_database_t db);

extern int cmyth_database_setup(cmyth_database_t db);

/*
 * -----------------------------------------------------------------
 * Ring Buffer Operations
 * -----------------------------------------------------------------
 */
extern char * cmyth_ringbuf_pathname(cmyth_recorder_t rec);

extern cmyth_ringbuf_t cmyth_ringbuf_create(void);

extern cmyth_recorder_t cmyth_ringbuf_setup(cmyth_recorder_t old_rec);

extern int32_t cmyth_ringbuf_request_block(cmyth_recorder_t rec,
				       int32_t len);

extern int cmyth_ringbuf_select(cmyth_recorder_t rec, struct timeval *timeout);

extern int32_t cmyth_ringbuf_get_block(cmyth_recorder_t rec,
				   char *buf,
				   int32_t len);

extern int64_t cmyth_ringbuf_seek(cmyth_recorder_t rec,
				    int64_t offset,
				    int8_t whence);

extern int32_t cmyth_ringbuf_read(cmyth_recorder_t rec,
			      char *buf,
			      int32_t len);
/*
 * -----------------------------------------------------------------
 * Recorder Number Operations
 * -----------------------------------------------------------------
 */
extern cmyth_rec_num_t cmyth_rec_num_create(void);

extern cmyth_rec_num_t cmyth_rec_num_get(char *host,
					 uint16_t port,
					 int32_t id);

extern char *cmyth_rec_num_string(cmyth_rec_num_t rn);

/*
 * -----------------------------------------------------------------
 * Timestamp Operations
 * -----------------------------------------------------------------
 */
extern cmyth_timestamp_t cmyth_timestamp_create(void);

extern cmyth_timestamp_t cmyth_timestamp_from_string(const char *str);

extern cmyth_timestamp_t cmyth_timestamp_from_unixtime(time_t l);

extern cmyth_timestamp_t cmyth_timestamp_utc_from_unixtime(time_t l);

extern time_t cmyth_timestamp_to_unixtime(cmyth_timestamp_t ts);

extern int cmyth_timestamp_to_string(char *str, cmyth_timestamp_t ts);

extern int cmyth_timestamp_to_isostring(char *str, cmyth_timestamp_t ts);

extern int cmyth_timestamp_to_display_string(char *str, cmyth_timestamp_t ts,
				   int time_format_12);

extern int cmyth_datetime_to_string(char *str, cmyth_timestamp_t ts);

extern int cmyth_timestamp_to_numstring(char *str, cmyth_timestamp_t ts);

extern int cmyth_timestamp_compare(cmyth_timestamp_t ts1,
				   cmyth_timestamp_t ts2);

/*
 * -----------------------------------------------------------------
 * Key Frame Operations
 * -----------------------------------------------------------------
 */
extern cmyth_keyframe_t cmyth_keyframe_create(void);

extern char *cmyth_keyframe_string(cmyth_keyframe_t kf);

/*
 * -----------------------------------------------------------------
 * Position Map Operations
 * -----------------------------------------------------------------
 */
extern cmyth_posmap_t cmyth_posmap_create(void);

/*
 * -----------------------------------------------------------------
 * Program Info Operations
 * -----------------------------------------------------------------
 */

/**
 * Program recording status.
 */
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
	RS_DONT_RECORD = 1,
	RS_PREVIOUS_RECORDING = 2,
	RS_CURRENT_RECORDING = 3,
	RS_EARLIER_RECORDING = 4,
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

/**
 * Create a new program info data structure.
 * \return proginfo handle
 */
extern cmyth_proginfo_t cmyth_proginfo_create(void);

extern int cmyth_proginfo_stop_recording(cmyth_conn_t control,
					 cmyth_proginfo_t prog);

extern int cmyth_proginfo_check_recording(cmyth_conn_t control,
					  cmyth_proginfo_t prog);

/**
 * Delete a program.
 * \param control backend control handle
 * \param prog proginfo handle
 * \param force force deletion
 * \param forget allow to record the recording
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_proginfo_delete_recording(cmyth_conn_t control,
					   cmyth_proginfo_t prog, uint8_t force, uint8_t forget);

/**
 * Delete a program such that it may be recorded again.
 * \param control backend control handle
 * \param prog proginfo handle
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_proginfo_forget_recording(cmyth_conn_t control,
					   cmyth_proginfo_t prog);

extern int cmyth_proginfo_get_recorder_num(cmyth_conn_t control,
					   cmyth_rec_num_t rnum,
					   cmyth_proginfo_t prog);

extern cmyth_proginfo_t cmyth_proginfo_get_from_basename(cmyth_conn_t control,
					   const char* basename);

/**
 * Retrieve the program with its timeslot.
 * 'recstartts' is the timeslot start time received from the message of backend
 *  event 'RECORDING LIST CHANGE'.  Uses UTC time since protocol 75.
 * \param control backend control handle
 * \param chanid channel id
 * \param recstartts recstartts
 * \return NULL or proginfo handle
 */
extern cmyth_proginfo_t cmyth_proginfo_get_from_timeslot(cmyth_conn_t control,
					   uint32_t chanid,
					   const cmyth_timestamp_t recstartts);

/**
 * Retrieve the title of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_title(cmyth_proginfo_t prog);

/**
 * Retrieve the subtitle of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_subtitle(cmyth_proginfo_t prog);

/**
 * Retrieve the description of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_description(cmyth_proginfo_t prog);

/**
 * Retrieve the season of a program.
 * \param prog proginfo handle
 * \return season
 */
extern uint16_t cmyth_proginfo_season(cmyth_proginfo_t prog);

/**
 * Retrieve the episode of a program.
 * \param prog proginfo handle
 * \return episode
 */
extern uint16_t cmyth_proginfo_episode(cmyth_proginfo_t prog);

/**
 * Retrieve the category of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_category(cmyth_proginfo_t prog);

/**
 * Retrieve the channel number of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chanstr(cmyth_proginfo_t prog);

/**
 * Retrieve the channel name of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chansign(cmyth_proginfo_t prog);

/**
 * Retrieve the channel name of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_channame(cmyth_proginfo_t prog);

/**
 * Retrieve the channel number of a program.
 * \param prog proginfo handle
 * \return channel number
 */
extern uint32_t cmyth_proginfo_chan_id(cmyth_proginfo_t prog);

/**
 * Retrieve the pathname of a program file.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_pathname(cmyth_proginfo_t prog);

/**
 * Retrieve the series ID of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_seriesid(cmyth_proginfo_t prog);

/**
 * Retrieve the program ID of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_programid(cmyth_proginfo_t prog);

/**
 * Retrieve the inetref of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_inetref(cmyth_proginfo_t prog);

/**
 * Retrieve the record ID of the matching record rule
 * \param prog proginfo handle
 * \return uint32_t
 */
extern uint32_t cmyth_proginfo_recordid(cmyth_proginfo_t prog);

/**
 * Retrieve the priority of a program
 * \param prog proginfo handle
 * \return int32_t
 */
extern int8_t cmyth_proginfo_priority(cmyth_proginfo_t prog);

/**
 * Retrieve the critics rating (number of stars) of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_stars(cmyth_proginfo_t prog);

/**
 * Retrieve the start time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_rec_start(cmyth_proginfo_t prog);

/**
 * Retrieve the end time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_rec_end(cmyth_proginfo_t prog);

/**
 * Retrieve the original air date of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_originalairdate(cmyth_proginfo_t prog);

/**
 * Retrieve the recording status of a program.
 * \param prog proginfo handle
 * \return recording status
 */
extern cmyth_proginfo_rec_status_t cmyth_proginfo_rec_status(
	cmyth_proginfo_t prog);

/**
 * Retrieve the flags associated with a program.
 * \param prog proginfo handle
 * \return flags
 */
extern uint32_t cmyth_proginfo_flags(
  cmyth_proginfo_t prog);

/**
 * Retrieve the size, in bytes, of a program.
 * \param prog proginfo handle
 * \return program length
 */
extern int64_t cmyth_proginfo_length(cmyth_proginfo_t prog);

/**
 * Retrieve the hostname of the MythTV backend that recorded a program.
 * \param prog proginfo handle
 * \return MythTV backend hostname
 */
extern char *cmyth_proginfo_host(cmyth_proginfo_t prog);

extern uint16_t cmyth_proginfo_port(cmyth_proginfo_t prog);

/**
 * Determine if two proginfo handles refer to the same program.
 * \param a proginfo handle a
 * \param b proginfo handle b
 * \return programs are the same: 0
 * \return programs are different: -1
 */
extern int cmyth_proginfo_compare(cmyth_proginfo_t a, cmyth_proginfo_t b);

/**
 * Retrieve the program length in seconds.
 * \param prog proginfo handle
 * \return program length in seconds
 */
extern uint32_t cmyth_proginfo_length_sec(cmyth_proginfo_t prog);

extern cmyth_proginfo_t cmyth_proginfo_get_detail(cmyth_conn_t control,
						  cmyth_proginfo_t p);

/**
 * Retrieve the start time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_start(cmyth_proginfo_t prog);

/**
 * Retrieve the end time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_end(cmyth_proginfo_t prog);

/**
 * Retrieve the card ID where the program was recorded.
 * \param prog proginfo handle
 * \return card ID
 */
extern uint32_t cmyth_proginfo_card_id(cmyth_proginfo_t prog);

/**
 * Retrieve the recording group of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_recgroup(cmyth_proginfo_t prog);

/**
 * Retrieve the channel icon path this program info
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chanicon(cmyth_proginfo_t prog);


/**
 * Retrieve the production year for this program info
 * \param prog proginfo handle
 * \return production year
 */
extern uint16_t cmyth_proginfo_year(cmyth_proginfo_t prog);

/**
 * Retrieve the watched status for this program info
 * \param db database connection
 * \param prog proginfo handle
 * \param watchedStat returned status
 * \return success: 1 = program found, 0 = program not found
 * \return failure: -(errno)
 */
extern int cmyth_mysql_set_watched_status(cmyth_database_t db, cmyth_proginfo_t prog, int watchedStat);

/**
 * Retrieve the storage group for this program info
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_storagegroup(cmyth_proginfo_t prog);

/**
 * Query to generate a pixmap for this program info
 * \param prog proginfo handle
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_proginfo_generate_pixmap(cmyth_conn_t control, cmyth_proginfo_t prog);

/*
 * -----------------------------------------------------------------
 * Program List Operations
 * -----------------------------------------------------------------
 */

extern cmyth_proglist_t cmyth_proglist_create(void);

extern cmyth_proglist_t cmyth_proglist_get_all_recorded(cmyth_conn_t control);

extern cmyth_proglist_t cmyth_proglist_get_all_pending(cmyth_conn_t control);

extern cmyth_proglist_t cmyth_proglist_get_all_scheduled(cmyth_conn_t control);

extern cmyth_proglist_t cmyth_proglist_get_conflicting(cmyth_conn_t control);

extern cmyth_proginfo_t cmyth_proglist_get_item(cmyth_proglist_t pl,
						int index);

extern int cmyth_proglist_delete_item(cmyth_proglist_t pl,
				      cmyth_proginfo_t prog);

extern int cmyth_proglist_get_count(cmyth_proglist_t pl);

extern int cmyth_proglist_sort(cmyth_proglist_t pl, int count,
			       cmyth_proglist_sort_t sort);

/*
 * -----------------------------------------------------------------
 * File Transfer Operations
 * -----------------------------------------------------------------
 */
extern cmyth_conn_t cmyth_file_data(cmyth_file_t file);

extern int64_t cmyth_file_start(cmyth_file_t file);

extern int64_t cmyth_file_length(cmyth_file_t file);

extern int64_t cmyth_file_position(cmyth_file_t file);

extern int cmyth_file_update_length(cmyth_file_t file, int64_t newlen);

extern int32_t cmyth_file_get_block(cmyth_file_t file, char *buf, int32_t len);

extern int32_t cmyth_file_request_block(cmyth_file_t file, int32_t len);

extern int64_t cmyth_file_seek(cmyth_file_t file, int64_t offset, int8_t whence);

extern int cmyth_file_select(cmyth_file_t file, struct timeval *timeout);

extern void cmyth_file_set_closed_callback(cmyth_file_t file, void (*callback)(cmyth_file_t));

extern int32_t cmyth_file_read(cmyth_file_t file, char *buf, int32_t len);

extern int cmyth_file_is_open(cmyth_file_t file);

extern int cmyth_file_set_timeout(cmyth_file_t file, int32_t fast);

/*
 * -----------------------------------------------------------------
 * Free Space Operations
 * -----------------------------------------------------------------
 */
extern cmyth_freespace_t cmyth_freespace_create(void);

/*
 * -----------------------------------------------------------------
 * Bookmark,Commercial Skip Operations
 * -----------------------------------------------------------------
 */
extern int64_t cmyth_get_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog);
extern int64_t cmyth_mysql_get_bookmark_offset(cmyth_database_t db, uint32_t chanid, int64_t mark, time_t starttime, int mode);
extern int cmyth_mysql_update_bookmark_setting(cmyth_database_t db, cmyth_proginfo_t prog);
extern int64_t cmyth_mysql_get_bookmark_mark(cmyth_database_t db, cmyth_proginfo_t prog, int64_t bk, int mode);
extern int cmyth_set_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog, int64_t bookmark);
extern cmyth_commbreaklist_t cmyth_commbreaklist_create(void);
extern cmyth_commbreak_t cmyth_commbreak_create(void);
extern cmyth_commbreaklist_t cmyth_get_commbreaklist(cmyth_conn_t conn, cmyth_proginfo_t prog);
extern cmyth_commbreaklist_t cmyth_get_cutlist(cmyth_conn_t conn, cmyth_proginfo_t prog);
extern int cmyth_rcv_commbreaklist(cmyth_conn_t conn, int *err, cmyth_commbreaklist_t breaklist, int count);

/*
 * -----------------------------------------------------------------
 * Recording Groups Operations
 * -----------------------------------------------------------------
 */

typedef struct cmyth_recgrougs {
	char recgroups[33];
} cmyth_recgroups_t;

extern int cmyth_mysql_get_recgroups(cmyth_database_t, cmyth_recgroups_t **);

/*
 * -----------------------------------------------------------------
 * mysql info
 * -----------------------------------------------------------------
 */

extern int cmyth_mysql_testdb_connection(cmyth_database_t db,char **message);

/*
 * -----------------------------------------------------------------
 * Card Input Operations
 * -----------------------------------------------------------------
 */

struct cmyth_input {
	char *inputname;
	uint32_t sourceid;
	uint32_t inputid;
	uint32_t cardid;
	uint32_t multiplexid;
	uint32_t livetvorder; /* new in V71 */
};

typedef struct cmyth_input *cmyth_input_t;

struct cmyth_inputlist {
	cmyth_input_t *input_list;
	int input_count;
};

typedef struct cmyth_inputlist *cmyth_inputlist_t;

extern cmyth_inputlist_t cmyth_inputlist_create(void);

extern cmyth_input_t cmyth_input_create(void);

extern cmyth_inputlist_t cmyth_get_free_inputlist(cmyth_recorder_t rec);

extern int cmyth_rcv_free_inputlist(cmyth_conn_t conn, int *err, cmyth_inputlist_t inputlist, int count);

/*
 * -----------------------------------------------------------------
 * Recording Schedule Operations
 * -----------------------------------------------------------------
 */

struct cmyth_recordingrule;
typedef struct cmyth_recordingrule *cmyth_recordingrule_t;

struct cmyth_recordingrulelist;
typedef struct cmyth_recordingrulelist *cmyth_recordingrulelist_t;

typedef enum {
	RRULE_NOT_RECORDING = 0,
	RRULE_SINGLE_RECORD = 1,
	RRULE_TIME_SLOT_RECORD,
	RRULE_CHANNEL_RECORD,
	RRULE_ALL_RECORD,
	RRULE_WEEK_SLOT_RECORD,
	RRULE_FIND_ONE_RECORD,
	RRULE_OVERRIDE_RECORD,
	RRULE_DONT_RECORD,
	RRULE_FIND_DAILY_RECORD,
	RRULE_FIND_WEEKLY_RECORD,
	RRULE_TEMPLATE_RECORD
} cmyth_recordingruletype_t;

typedef enum {
	RRULE_NO_SEARCH = 0,
	RRULE_POWER_SEARCH = 1,
	RRULE_TITLE_SEARCH,
	RRULE_KEYWORD_SEARCH,
	RRULE_PEOPLE_SEARCH,
	RRULE_MANUAL_SEARCH
} cmyth_recordingrulesearch_t;

typedef enum {
	RRULE_CHECK_NONE = 0x01,
	RRULE_CHECK_SUBTITLE = 0x02,
	RRULE_CHECK_DESCRIPTION = 0x04,
	RRULE_CHECK_SUBTITLE_AND_DESCRIPTION = 0x06,
	RRULE_CHECK_SUBTITLE_THEN_DESCRIPTION = 0x08
} cmyth_recordingruledupmethod_t;

typedef enum {
	RRULE_IN_RECORDED = 0x01,
	RRULE_IN_OLD_RECORDED = 0x02,
	RRULE_IN_ALL = 0x0F,
	RRULE_NEW_EPI = 0x10
} cmyth_recordingruledupin_t;

/**
 * Initialize a new recording rule structure.  The structure is initialized
 * to default values.
 * Before forgetting the reference to this recording rule structure
 * the caller must call ref_release().
 * \return success: A new recording rule
 * \return failure: NULL
 */
extern cmyth_recordingrule_t cmyth_recordingrule_init(void);

/**
 * Retrieves the 'recordid' field of a recording rule structure.
 * \param rr
 * \return success: recordid
 * \return failure: -(errno)
 */
extern uint32_t cmyth_recordingrule_recordid(cmyth_recordingrule_t rr);

/**
 * Set the 'recordid' field of the recording rule structure 'rr'.
 * \param rr
 * \param recordid
 */
extern void cmyth_recordingrule_set_recordid(cmyth_recordingrule_t rr, uint32_t recordid);

/**
 * Retrieves the 'chanid' field of a recording rule structure.
 * \param rr
 * \return success: chanid
 * \return failure: -(errno)
 */
extern uint32_t cmyth_recordingrule_chanid(cmyth_recordingrule_t rr);

/**
 * Set the 'chanid' field of the recording rule structure 'rr'.
 * \param rr
 * \param chanid
 */
extern void cmyth_recordingrule_set_chanid(cmyth_recordingrule_t rr, uint32_t chanid);

/**
 * Retrieves the 'starttime' field of a recording rule structure.
 * \param rr
 * \return success: time_t starttime
 * \return failure: -(errno)
 */
extern time_t cmyth_recordingrule_starttime(cmyth_recordingrule_t rr);

/**
 * Set the 'starttime' field of the recording rule structure 'rr'.
 * \param rr
 * \param starttime
 */
extern void cmyth_recordingrule_set_starttime(cmyth_recordingrule_t rr, time_t starttime);

/**
 * Retrieves the 'endtime' field of a recording rule structure.
 * \param rr
 * \return success: time_t endtime
 * \return failure: -(errno)
 */
extern time_t cmyth_recordingrule_endtime(cmyth_recordingrule_t rr);

/**
 * Set the 'endtime' field of the recording rule structure 'rr'.
 * \param rr
 * \param endtime
 */
extern void cmyth_recordingrule_set_endtime(cmyth_recordingrule_t rr, time_t endtime);

/**
 * Retrieves the 'title' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: title
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_title(cmyth_recordingrule_t rr);

/**
 * Set the 'title' field of the recording rule structure 'rr'.
 * \param rr
 * \param title
 */
extern void cmyth_recordingrule_set_title(cmyth_recordingrule_t rr, char *title);

/**
 * Retrieves the 'description' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: description
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_description(cmyth_recordingrule_t rr);

/**
 * Set the 'description' field of the recording rule structure 'rr'.
 * \param rr
 * \param description
 */
extern void cmyth_recordingrule_set_description(cmyth_recordingrule_t rr, char *description);

/**
 * Retrieves the 'type' field of a recording rule structure.
 * \param rr
 * \return success: type
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_type(cmyth_recordingrule_t rr);

/**
 * Set the 'type' field of the recording rule structure 'rr'.
 * \param rr
 * \param type
 */
extern void cmyth_recordingrule_set_type(cmyth_recordingrule_t rr, uint8_t type);

/**
 * Retrieves the 'category' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: category
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_category(cmyth_recordingrule_t rr);

/**
 * Set the 'category' field of the recording rule structure 'rr'.
 * \param rr
 * \param category
 */
extern void cmyth_recordingrule_set_category(cmyth_recordingrule_t rr, char *category);

/**
 * Retrieves the 'subtitle' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: subtitle
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_subtitle(cmyth_recordingrule_t rr);

/**
 * Set the 'subtitle' field of the recording rule structure 'rr'.
 * \param rr
 * \param subtitle
 */
extern void cmyth_recordingrule_set_subtitle(cmyth_recordingrule_t rr, char *subtitle);

/**
 * Retrieves the 'recpriority' field of a recording rule structure.
 * \param rr
 * \return success: recpriority
 * \return failure: -(errno)
 */
extern int8_t cmyth_recordingrule_recpriority(cmyth_recordingrule_t rr);

/**
 * Set the 'recpriority' field of the recording rule structure 'rr'.
 * \param rr
 * \param recpriority
 */
extern void cmyth_recordingrule_set_recpriority(cmyth_recordingrule_t rr, int8_t recpriority);

/**
 * Retrieves the 'startoffset' field of a recording rule structure.
 * \param rr
 * \return success: startoffset
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_startoffset(cmyth_recordingrule_t rr);

/**
 * Set the 'startoffset' field of the recording rule structure 'rr'.
 * \param rr
 * \param startoffset
 */
extern void cmyth_recordingrule_set_startoffset(cmyth_recordingrule_t rr, uint8_t startoffset);

/**
 * Retrieves the 'endoffset' field of a recording rule structure.
 * \param rr
 * \return success: endoffset
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_endoffset(cmyth_recordingrule_t rr);

/**
 * Set the 'endoffset' field of the recording rule structure 'rr'.
 * \param rr
 * \param endoffset
 */
extern void cmyth_recordingrule_set_endoffset(cmyth_recordingrule_t rr, uint8_t endoffset);

/**
 * Retrieves the 'searchtype' field of a recording rule structure.
 * \param rr
 * \return success: searchtype
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_searchtype(cmyth_recordingrule_t rr);

/**
 * Set the 'searchtype' field of the recording rule structure 'rr'.
 * \param rr
 * \param searchtype
 */
extern void cmyth_recordingrule_set_searchtype(cmyth_recordingrule_t rr, uint8_t searchtype);

/**
 * Retrieves the 'inactive' field of a recording rule structure.
 * \param rr
 * \return success: inactive flag
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_inactive(cmyth_recordingrule_t rr);

/**
 * Set the 'inactive' field of the recording rule structure 'rr'.
 * \param rr
 * \param inactive
 */
extern void cmyth_recordingrule_set_inactive(cmyth_recordingrule_t rr, uint8_t inactive);

/**
 * Retrieves the 'callsign' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: callsign
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_callsign(cmyth_recordingrule_t rr);

/**
 * Set the 'callsign' field of the recording rule structure 'rr'.
 * \param rr
 * \param callsign
 */
extern void cmyth_recordingrule_set_callsign(cmyth_recordingrule_t rr, char *callsign);

/**
 * Retrieves the 'dupmethod' field of a recording rule structure.
 * \param rr
 * \return success: dupmethod
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_dupmethod(cmyth_recordingrule_t rr);

/**
 * Set the 'dupmethod' field of the recording rule structure 'rr'.
 * \param rr
 * \param dupmethod
 */
extern void cmyth_recordingrule_set_dupmethod(cmyth_recordingrule_t rr, uint8_t dupmethod);

/**
 * Retrieves the 'dupin' field of a recording rule structure.
 * \param rr
 * \return success: dupin
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_dupin(cmyth_recordingrule_t rr);

/**
 * Set the 'dupin' field of the recording rule structure 'rr'.
 * \param rr
 * \param dupin
 */
extern void cmyth_recordingrule_set_dupin(cmyth_recordingrule_t rr, uint8_t dupin);

/**
 * Retrieves the 'recgroup' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: recgroup
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_recgroup(cmyth_recordingrule_t rr);

/**
 *Set the 'recgroup' field of the recording rule structure 'rr'.
 * \param rr
 * \param recgroup
 */
extern void cmyth_recordingrule_set_recgroup(cmyth_recordingrule_t rr, char *recgroup);

/**
 * Retrieves the 'storagegroup' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: storagegroup
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_storagegroup(cmyth_recordingrule_t rr);

/**
 * Set the 'storagegroup' field of the recording rule structure 'rr'.
 * \param rr
 * \param storagegroup
 */
extern void cmyth_recordingrule_set_storagegroup(cmyth_recordingrule_t rr, char *storagegroup);

/**
 * Retrieves the 'playgroup' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: playgroup
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_playgroup(cmyth_recordingrule_t rr);

/**
 * Set the 'playgroup' field of the recording rule structure 'rr'.
 * \param rr
 * \param playgroup
 */
extern void cmyth_recordingrule_set_playgroup(cmyth_recordingrule_t rr, char *playgroup);

/**
 * Retrieves the 'autotranscode' field of a recording rule structure.
 * \param rr
 * \return success: autotranscode
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_autotranscode(cmyth_recordingrule_t rr);

/**
 * Set the 'autotranscode' field of the recording rule structure 'rr'.
 * \param rr
 * \param autotranscode
 */
extern void cmyth_recordingrule_set_autotranscode(cmyth_recordingrule_t rr, uint8_t autotranscode);

/**
 * Retrieves the 'userjobs' field of a recording rule structure.
 * \param rr
 * \return success: userjobs mask
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_userjobs(cmyth_recordingrule_t rr);

/**
 * Set the 'userjobs' field of the recording rule structure 'rr'.
 * \param rr
 * \param userjobs
 */
extern void cmyth_recordingrule_set_userjobs(cmyth_recordingrule_t rr, uint8_t userjobs);

/**
 * Retrieves the 'autocommflag' field of a recording rule structure.
 * \param rr
 * \return success: autocommflag
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_autocommflag(cmyth_recordingrule_t rr);

/**
 * Set the 'autocommflag' field of the recording rule structure 'rr'.
 * \param rr
 * \param autocommflag
 */
extern void cmyth_recordingrule_set_autocommflag(cmyth_recordingrule_t rr, uint8_t autocommflag);

/**
 * Retrieves the 'autoexpire' field of a recording rule structure.
 * \param rr
 * \return success: autoexpire
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_autoexpire(cmyth_recordingrule_t rr);

/**
 * Set the 'autoexpire' field of the recording rule structure 'rr'.
 * \param rr
 * \param autoexpire
 */
extern void cmyth_recordingrule_set_autoexpire(cmyth_recordingrule_t rr, uint8_t autoexpire);

/**
 * Retrieves the 'maxepisodes' field of a recording rule structure.
 * \param rr
 * \return success: maxepisodes
 * \return failure: -(errno)
 */
extern uint32_t cmyth_recordingrule_maxepisodes(cmyth_recordingrule_t rr);

/**
 * Set the 'maxepisodes' field of the recording rule structure 'rr'.
 * \param rr
 * \param maxepisodes
 */
extern void cmyth_recordingrule_set_maxepisodes(cmyth_recordingrule_t rr, uint32_t maxepisodes);

/**
 * Retrieves the 'maxnewest' field of a recording rule structure.
 * \param rr
 * \return success: maxnewest
 * \return failure: -(errno)
 */
extern uint8_t cmyth_recordingrule_maxnewest(cmyth_recordingrule_t rr);

/**
 * Set the 'maxnewest' field of the recording rule structure 'rr'.
 * \param rr
 * \param maxnewest
 */
extern void cmyth_recordingrule_set_maxnewest(cmyth_recordingrule_t rr, uint8_t maxnewest);

/**
 * Retrieves the 'transcoder' field of a recording rule structure.
 * \param rr
 * \return success: transcoder
 * \return failure: -(errno)
 */
extern uint32_t cmyth_recordingrule_transcoder(cmyth_recordingrule_t rr);

/**
 * Set the 'transcoder' field of the recording rule structure 'rr'.
 * \param rr
 * \param transcoder
 */
extern void cmyth_recordingrule_set_transcoder(cmyth_recordingrule_t rr, uint32_t transcoder);

/**
 * Retrieves the 'profile' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: profile
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_profile(cmyth_recordingrule_t rr);

/**
 * Set the 'profile' field of the recording rule structure 'rr'.
 * \param rr
 * \param profile
 */
extern void cmyth_recordingrule_set_profile(cmyth_recordingrule_t rr, char *profile);

/**
 * Retrieves the 'prefinput' field of a recording rule structure.
 * \param rr
 * \return success: prefinput
 */
extern uint32_t cmyth_recordingrule_prefinput(cmyth_recordingrule_t rr);

/**
 * Set the 'prefinput' field of the recording rule structure 'rr'.
 * \param rr
 * \param prefinput
 */
extern void cmyth_recordingrule_set_prefinput(cmyth_recordingrule_t rr, uint32_t prefinput);

/**
 * Retrieves the 'autometadata' field of a recording rule structure.
 * \param rr
 * \return success: autometadata
 */
extern uint8_t cmyth_recordingrule_autometadata(cmyth_recordingrule_t rr);

/**
 * Set the 'autometadata' field of the recording rule structure 'rr'.
 * \param rr
 * \param autometadata
 */
extern void cmyth_recordingrule_set_autometadata(cmyth_recordingrule_t rr, uint8_t autometadata);

/**
 * Retrieves the 'inetref' field of a recording rule structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param rr
 * \return success: inetref
 * \return failure: NULL
 */
extern char *cmyth_recordingrule_inetref(cmyth_recordingrule_t rr);

/**
 * Set the 'inetref' field of the recording rule structure 'rr'.
 * \param rr
 * \param inetref
 */
extern void cmyth_recordingrule_set_inetref(cmyth_recordingrule_t rr, char *inetref);

/**
 * Retrieves the 'season' field of a recording rule structure.
 * \param rr
 * \return success: season
 */
extern uint16_t cmyth_recordingrule_season(cmyth_recordingrule_t rr);

/**
 * Set the 'season' field of the recording rule structure 'rr'.
 * \param rr
 * \param season
 */
extern void cmyth_recordingrule_set_season(cmyth_recordingrule_t rr, uint16_t season);

/**
 * Retrieves the 'episode' field of a recording rule structure.
 * \param rr
 * \return success: episode
 */
extern uint16_t cmyth_recordingrule_episode(cmyth_recordingrule_t rr);

/**
 * Set the 'episode' field of the recording rule structure 'rr'.
 * \param rr
 * \param episode
 */
extern void cmyth_recordingrule_set_episode(cmyth_recordingrule_t rr, uint16_t episode);

/**
 * Retrieves the 'filter' field of a recording rule structure.
 * \param rr
 * \return success: filter
 */
extern uint32_t cmyth_recordingrule_filter(cmyth_recordingrule_t rr);

/**
 * Set the 'filter' field of the recording rule structure 'rr'.
 * \param rr
 * \param filter
 */
extern void cmyth_recordingrule_set_filter(cmyth_recordingrule_t rr, uint32_t filter);

/**
 * Retrieve the recording rule structure found at index 'index' in the list
 * in 'rrl'.  Return the recording rule structure held.
 * Before forgetting the reference to this recording rule structure the
 * caller must call ref_release().
 * \param rrl recordingrulelist handle
 * \param index index in list
 * \return success: recording rule handle
 * \return failure: NULL
 */
extern cmyth_recordingrule_t cmyth_recordingrulelist_get_item(cmyth_recordingrulelist_t rrl, int index);

/**
 * Retrieves the number of elements in the recording rule list structure.
 * \param rrl
 * \return success: A number indicating the number of items in rrl
 * \return failure: -(errno)
 */
extern int cmyth_recordingrulelist_get_count(cmyth_recordingrulelist_t rrl);

/**
 * Returns the recording rule list structure filled from database.
 * \param db
 * \return success: recording rule list handle
 * \return failure: NULL
 */
extern int cmyth_mysql_get_recordingrules(cmyth_database_t db, cmyth_recordingrulelist_t *rll);

/**
 * Add a recording rule within the database.
 * \param db database
 * \param rr recording rule to add
 * \return success: 0 , a new recordid is assigned
 * \return failure: -(errno)
 */
extern int cmyth_mysql_add_recordingrule(cmyth_database_t db, cmyth_recordingrule_t rr);

/**
 * Delete recording rule from the database.
 * \param db database
 * \param recordid handle
 * \return success: 0
 * \return failure: -(errno)
 * \see cmyth_recordingrule_recordid
 */
extern int cmyth_mysql_delete_recordingrule(cmyth_database_t db, uint32_t recordid);

/**
 * Update recording rule within the database.
 * \param db database
 * \param rr recording rule handle
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_mysql_update_recordingrule(cmyth_database_t db, cmyth_recordingrule_t rr);

/**
 * Initialize a new rule from database template.
 * Before forgetting the reference to this recording rule structure
 * the caller must call ref_release().
 * \param db database
 * \param category program category
 * \param category_type program category type
 * \param rr recording rule handle
 * \return success: 0
 * \return failure: -(errno)
 */
extern int cmyth_mysql_recordingrule_from_template(cmyth_database_t db, const char *category, const char *category_type, cmyth_recordingrule_t *rr);

/*
 * -----------------------------------------------------------------
 * Channel Operations
 * -----------------------------------------------------------------
 */

struct cmyth_channel;
typedef struct cmyth_channel *cmyth_channel_t;

struct cmyth_chanlist;
typedef struct cmyth_chanlist *cmyth_chanlist_t;

/**
 * Retrieves the 'chanid' field of a channel structure.
 * \param channel
 * \return success: chanid
 * \return failure: -(errno)
 */
extern uint32_t cmyth_channel_chanid(cmyth_channel_t channel);

/**
 * Retrieves the 'channum' field of a channel structure.
 * \param channel
 * \return success: channum
 * \return failure: -(errno)
 */
extern uint32_t cmyth_channel_channum(cmyth_channel_t channel);

/**
 * Retrieves the 'chanstr' field of a channel structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param channel
 * \return success: channumstr
 * \return failure: NULL
 */
extern char *cmyth_channel_channumstr(cmyth_channel_t channel);

/**
 * Retrieves the 'callsign' field of a channel structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param channel
 * \return success: callsign
 * \return failure: NULL
 */
extern char *cmyth_channel_callsign(cmyth_channel_t channel);

/**
 * Retrieves the 'name' field of a channel structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param channel
 * \return success: name
 * \return failure: NULL
 */
extern char *cmyth_channel_name(cmyth_channel_t channel);

/**
 * Retrieves the 'icon' field of a channel structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param channel
 * \return success: icon
 * \return failure: NULL
 */
extern char *cmyth_channel_icon(cmyth_channel_t channel);

/**
 * Retrieves the 'visible' field of a channel structure.
 * \param channel
 * \return success: visible flag
 * \return failure: -(errno)
 */
extern uint8_t cmyth_channel_visible(cmyth_channel_t channel);

/**
 * Retrieves the 'sourceid' field of a channel structure.
 * \param channel
 * \return success: sourceid
 * \return failure: -(errno)
 */
extern uint32_t cmyth_channel_sourceid(cmyth_channel_t channel);

/**
 * Retrieves the 'multiplex' field of a channel structure.
 * \param channel
 * \return success: multiplex
 * \return failure: -(errno)
 */
extern uint32_t cmyth_channel_multiplex(cmyth_channel_t channel);

/**
 * Retrieve the channel structure found at index 'index' in the list in 'cl'.
 * Return the channel structure held.
 * Before forgetting the reference to this channel structure the caller
 * must call ref_release().
 * \param channel
 * \return success: non-null cmyth_channel_t (this is a pointer type)
 * \return failure: NULL
 */
extern cmyth_channel_t cmyth_chanlist_get_item(cmyth_chanlist_t cl, int index);

/**
 * Retrieves the number of elements in the channels list structure.
 * \param cl
 * \return success: A number indicating the number of items in cl
 * \return failure: -(errno)
 */
extern int cmyth_chanlist_get_count(cmyth_chanlist_t cl);

/**
 * Returns the channels list structure filled from database.
 * Before forgetting the reference to this channels list structure the caller
 * must call ref_release().
 * \param db
 * \return success: channels list handle
 * \return failure: NULL
 */
extern int cmyth_mysql_get_chanlist(cmyth_database_t db, cmyth_chanlist_t *chanlist);

/**
 * Returns 'radio' flag of the channel identified by its 'chanid' from database.
 * \param db
 * \param chanid
 * \return success: radio flag
 * \return failure: -(errno)
 */
extern int cmyth_mysql_is_radio(cmyth_database_t db, uint32_t chanid);

/**
 * Returns capture card type for the channel identified by its 'chanid' from database.
 * \param db
 * \param chanid
 * \return success: capture card type
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_cardtype(cmyth_database_t db, uint32_t chanid, char* *cardtype);

/*
 * -----------------------------------------------------------------
 * Storage Group operations
 * -----------------------------------------------------------------
 */

struct cmyth_storagegroup_file;
typedef struct cmyth_storagegroup_file *cmyth_storagegroup_file_t;

struct cmyth_storagegroup_filelist;
typedef struct cmyth_storagegroup_filelist *cmyth_storagegroup_filelist_t;

/**
 * Returns the storagegroup files list structure of storagegroup.
 * Before forgetting the reference to this storagegroup files list structure
 * the caller must call ref_release().
 * \param control
 * \param storagegroup
 * \param hostname
 * \return success: storage files list handle
 * \return failure: NULL
 */
extern cmyth_storagegroup_filelist_t cmyth_storagegroup_get_filelist(cmyth_conn_t control, char *storagegroup, char *hostname);

/**
 * Returns the storagegroup file structure of storagegroup file.
 * Before forgetting the reference to this storagegroup file structure
 * the caller must call ref_release().
 * \param control
 * \param storagegroup
 * \param hostname
 * \param filename
 * \return success: storage file handle
 * \return failure: NULL
 */
extern cmyth_storagegroup_file_t cmyth_storagegroup_get_fileinfo(cmyth_conn_t control, char *storagegroup, char *hostname, char *filename);

/**
 * Retrieves the number of elements in the storagegroup files list structure.
 * \param fl
 * \return success: A number indicating the number of items in fl
 * \return failure: -(errno)
 */
extern int cmyth_storagegroup_filelist_count(cmyth_storagegroup_filelist_t fl);

/**
 * Retrieve the storagegroup file structure found at index 'index' in the list
 * in 'fl'.
 * Return the storagegroup file structure held.
 * Before forgetting the reference to this storagegroup file structure the
 * caller must call ref_release().
 * \param fl
 * \return success: non-null cmyth_storagegroup_file_t (this is a pointer type)
 * \return failure: NULL
 */
extern cmyth_storagegroup_file_t cmyth_storagegroup_filelist_get_item(cmyth_storagegroup_filelist_t fl, int index);

/**
 * Retrieves the 'filename' field of a storagegroup file structure.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 * \param file
 * \return success: filename
 * \return failure: NULL
 */
extern char *cmyth_storagegroup_file_filename(cmyth_storagegroup_file_t file);

/**
 * Retrieves the 'lastmodified' field of a storagegroup file structure.
 * \param file
 * \return success: time_t lastmodified
 * \return failure: 0
 */
extern time_t cmyth_storagegroup_file_lastmodified(cmyth_storagegroup_file_t file);

/**
 * Retrieves the 'filesize' field of a storagegroup file structure.
 * \param file
 * \return success: int64_t file size
 * \return failure: 0
 */
extern int64_t cmyth_storagegroup_file_size(cmyth_storagegroup_file_t file);

/*
 * -----------------------------------------------------------------
 * Backend configuration infos
 * -----------------------------------------------------------------
 */

typedef struct cmyth_recorder_source {
	uint32_t recid;
	uint32_t sourceid;
} cmyth_recorder_source_t;

/**
 * Retrieves recorder source list from database.
 * \param db
 * \param rsrc recorder source list handle
 * \return success: A number indicating the number of recorder source in rsrc
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_recorder_source_list(cmyth_database_t db, cmyth_recorder_source_t **rsrc);

/**
 * Retrieves recorder source list from database and available for 'channum'.
 * \param db
 * \param channum desired channel number
 * \param rsrc recorder source list handle
 * \return success: A number indicating the number of recorder source in rsrc
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_recorder_source_channum(cmyth_database_t db, char *channum, cmyth_recorder_source_t **rsrc);

typedef struct cmyth_channelgroup {
	char name[65];
	uint32_t grpid;
} cmyth_channelgroup_t;

/**
 * Retrieves channel group list from database.
 * \param db
 * \param changroups channel group list handle
 * \return success: A number indicating the number of channel group in changroups
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_channelgroups(cmyth_database_t db, cmyth_channelgroup_t **changroups);

/**
 * Retrieves channel ID list in group from database.
 * \param db
 * \param chanids channel ID list handle
 * \return success: A number indicating the number of channel group in changroups
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_channelids_in_group(cmyth_database_t db, uint32_t grpid, uint32_t **chanids);

/**
 * Retrieves storagegroup name list from database.
 * \param db
 * \param sg storagegroup name list handle
 * \return success: A number indicating the number of storagegroup name in sg
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_storagegroups(cmyth_database_t db, char** *sg);

typedef struct cmyth_recprofile {
	uint32_t id;
	char name[128];
	char cardtype[32];
} cmyth_recprofile_t;

/**
 * Retrieves recording profile list from database.
 * \param db
 * \param profiles recording profile list handle
 * \return success: A number indicating the number of profile in profiles
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_recprofiles(cmyth_database_t db, cmyth_recprofile_t **profiles);

/**
 * Retrieves play group list from database.
 * \param db
 * \param pg play group list handle
 * \return success: A number indicating the number of playgroup in pg
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_playgroups(cmyth_database_t db, char** *pg);

/**
 * Retrieves global (non host specific) setting from database.
 * Before forgetting the reference to data
 * the caller must call ref_release().
 * \param db
 * \param global setting name
 * \param data setting data handle
 * \return success: 0 not available, 1 available
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_setting(cmyth_database_t db, char *setting, char **data);

/*
 * -----------------------------------------------------------------
 * Recording Markup and associated infos
 * -----------------------------------------------------------------
 */

typedef enum {
	MARK_UNSET = -10,
	MARK_TMP_CUT_END = -5,
	MARK_TMP_CUT_START = -4,
	MARK_UPDATED_CUT = -3,
	MARK_PLACEHOLDER = -2,
	MARK_CUT_END = 0,
	MARK_CUT_START = 1,
	MARK_BOOKMARK = 2,
	MARK_BLANK_FRAME = 3,
	MARK_COMM_START = 4,
	MARK_COMM_END = 5,
	MARK_GOP_START = 6,
	MARK_KEYFRAME = 7,
	MARK_SCENE_CHANGE = 8,
	MARK_GOP_BYFRAME = 9,
	MARK_ASPECT_1_1 = 10, //< deprecated, it is only 1:1 sample aspect ratio
	MARK_ASPECT_4_3 = 11,
	MARK_ASPECT_16_9 = 12,
	MARK_ASPECT_2_21_1 = 13,
	MARK_ASPECT_CUSTOM = 14,
	MARK_VIDEO_WIDTH = 30,
	MARK_VIDEO_HEIGHT = 31,
	MARK_VIDEO_RATE = 32,
	MARK_DURATION_MS = 33,
	MARK_TOTAL_FRAMES = 34
} cmyth_recording_markup_t;

/**
 * Retrieve data for a specific type of recording markup
 * \param db
 * \param prog program info
 * \param type of markup
 * \return success: markup data
 * \return failure: -(errno)
 */
extern int64_t cmyth_mysql_get_recording_markup(cmyth_database_t db, cmyth_proginfo_t prog, cmyth_recording_markup_t type);

/**
 * Retrieve recording framerate (fps x 1000)
 * \param db
 * \param prog program info
 * \return success: recording framerate >0
 * \return unknown: 0
 * \return failure: -(errno)
 */
extern int64_t cmyth_mysql_get_recording_framerate(cmyth_database_t db, cmyth_proginfo_t prog);

/*
 * -----------------------------------------------------------------
 * Recording artworks
 * -----------------------------------------------------------------
 */

/**
 * Retrieve recording artworks
 * Before forgetting the reference to artworks
 * the caller must call ref_release() for each.
 * \param db
 * \param prog program info
 * \param coverart corverart filename handle
 * \param fanart fanart filename handle
 * \param banner banner filename handle
 * \return success: 0 not available, 1 available
 * \return failure: -(errno)
 */
extern int cmyth_mysql_get_recording_artwork(cmyth_database_t db, cmyth_proginfo_t prog, char **coverart, char **fanart, char **banner);

/*
 * -----------------------------------------------------------------
 * EPG info
 * -----------------------------------------------------------------
 */

struct cmyth_epginfo;

typedef struct cmyth_epginfo *cmyth_epginfo_t;

struct cmyth_epginfolist;
typedef struct cmyth_epginfolist *cmyth_epginfolist_t;

extern cmyth_epginfo_t cmyth_epginfolist_get_item(cmyth_epginfolist_t el, int index);
extern int cmyth_epginfolist_get_count(cmyth_epginfolist_t el);
extern uint32_t cmyth_epginfo_chanid(cmyth_epginfo_t e);
extern char * cmyth_epginfo_callsign(cmyth_epginfo_t e);
extern char * cmyth_epginfo_channame(cmyth_epginfo_t e);
extern uint32_t cmyth_epginfo_sourceid(cmyth_epginfo_t e);
extern char * cmyth_epginfo_title(cmyth_epginfo_t e);
extern char * cmyth_epginfo_subtitle(cmyth_epginfo_t e);
extern char * cmyth_epginfo_description(cmyth_epginfo_t e);
extern time_t cmyth_epginfo_starttime(cmyth_epginfo_t e);
extern time_t cmyth_epginfo_endtime(cmyth_epginfo_t e);
extern char * cmyth_epginfo_programid(cmyth_epginfo_t e);
extern char * cmyth_epginfo_seriesid(cmyth_epginfo_t e);
extern char * cmyth_epginfo_category(cmyth_epginfo_t e);
extern char * cmyth_epginfo_category_type(cmyth_epginfo_t e);
extern uint32_t cmyth_epginfo_channum(cmyth_epginfo_t e);

extern int cmyth_mysql_get_prog_finder_char_title(cmyth_database_t db, cmyth_epginfolist_t *epglist, time_t starttime, char *program_name);
extern int cmyth_mysql_get_prog_finder_time(cmyth_database_t db, cmyth_epginfolist_t *epglist,  time_t starttime, char *program_name);
extern int cmyth_mysql_get_prog_finder_time2(cmyth_database_t db, cmyth_epginfolist_t *epglist,  time_t starttime, char *program_name);
extern int cmyth_mysql_get_prog_finder_chan(cmyth_database_t db, cmyth_epginfo_t *epg, uint32_t chanid);
extern int cmyth_mysql_get_prog_finder_time_title_chan(cmyth_database_t db, cmyth_epginfo_t *epg, time_t starttime, char *program_name, uint32_t chanid);
extern int cmyth_mysql_get_guide(cmyth_database_t db, cmyth_epginfolist_t *epglist, uint32_t chanid, time_t starttime, time_t endtime);
extern int cmyth_mysql_get_prev_recorded(cmyth_database_t db, cmyth_epginfolist_t *epglist);

#endif /* __CMYTH_H */
