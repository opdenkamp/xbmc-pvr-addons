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

/**
 * \file cmyth_local.h
 * Local definitions which are internal to libcmyth.
 */

#ifndef __CMYTH_LOCAL_H
#define __CMYTH_LOCAL_H

#include <stdio.h>
#include <stdlib.h>
#if !defined(_MSC_VER)
#include <sys/time.h>
#else
#include <winsock2.h>
#endif
#include <refmem/refmem.h>
#include <cmyth/cmyth.h>
#include <time.h>
#include <inttypes.h>
#include <mysql/mysql.h>

#if defined(_MSC_VER)
#include "cmyth_msc.h"
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef int cmyth_socket_t;
#define closesocket(fd) close(fd)
#endif /* _MSC_VER */

/*
 * Some useful constants
 */
#define CMYTH_INT64_LEN (sizeof("-18446744073709551616") - 1)
#define CMYTH_INT32_LEN (sizeof("-4294967296") - 1)
#define CMYTH_INT16_LEN (sizeof("-65536") - 1)
#define CMYTH_INT8_LEN (sizeof("-256") - 1)
#define CMYTH_TIMESTAMP_LEN (sizeof("YYYY-MM-DDTHH:MM:SS") - 1)
#define CMYTH_TIMESTAMP_UTC_LEN (sizeof("YYYY-MM-DDTHH:MM:SSZ") - 1)
#define CMYTH_TIMESTAMP_NUMERIC_LEN (sizeof("YYYYMMDDHHMMSS") - 1)
#define CMYTH_DATESTAMP_LEN (sizeof("YYYY-MM-DD") - 1)
#define CMYTH_UTC_LEN (sizeof("1240120680") - 1)
#define CMYTH_COMMBREAK_START 4
#define CMYTH_COMMBREAK_END 5
#define CMYTH_CUTLIST_START 1
#define CMYTH_CUTLIST_END 0

/**
 * MythTV backend connection
 */
struct cmyth_conn {
	cmyth_socket_t   conn_fd;        /**< socket file descriptor */
	unsigned char    *conn_buf;      /**< connection buffer */
	uint32_t         conn_buflen;    /**< buffer size */
	int32_t          conn_len;       /**< amount of data in buffer */
	int32_t          conn_pos;       /**< current position in buffer */
	uint32_t         conn_version;   /**< protocol version */
	volatile int8_t  conn_hang;      /**< is connection stuck? */
	int32_t          conn_tcp_rcvbuf;/**< TCP receive buffer size */
	char *           server;         /**< hostname of server */
	uint16_t         port;           /**< port of server */
	cmyth_conn_ann_t conn_ann;       /**< connection announcement */
	pthread_mutex_t  conn_mutex;
};

/* Sergio: Added to support new livetv protocol */
struct cmyth_livetv_chain {
	char *chainid;
	int32_t chain_ct;
	volatile int8_t chain_switch_on_create;
	int32_t chain_current;
	void (*prog_update_callback)(cmyth_proginfo_t prog);
	cmyth_proginfo_t *progs;
	char **chain_urls;
	cmyth_file_t *chain_files; /* File pointers for the urls */
	volatile int8_t livetv_watch; /* JLB: Manage program breaks */
	int32_t livetv_buflen;
	int32_t livetv_tcp_rcvbuf;
	int32_t livetv_block_len;
};

/* Sergio: Added to clean up database interaction */
struct cmyth_database {
	char * db_host;
	char * db_user;
	char * db_pass;
	char * db_name;
	uint16_t db_port;
	MYSQL * mysql;
	int8_t db_setup; /* JLB: 0 = No setup, 1 = setup done */
	uint32_t db_version; /* 0 = unknown else DBSchemaVer */
	int8_t db_tz_utc; /* JLB: 0 = No conversion, 1 = Enable UTC time zone conversion */
	char db_tz_name[64]; /* JLB: db time zone name to convert query projection */
};

struct cmyth_recorder {
	int8_t rec_have_stream;
	uint32_t rec_id;
	char *rec_server;
	uint16_t rec_port;
	cmyth_ringbuf_t rec_ring;
	cmyth_conn_t rec_conn;
	/* Sergio: Added to support new livetv protocol */
	cmyth_livetv_chain_t rec_livetv_chain;
	cmyth_file_t rec_livetv_file;
	double rec_framerate;
};

/**
 * MythTV file connection
 */
struct cmyth_file {
	cmyth_conn_t file_data;		/**< backend connection */
	uint32_t file_id;		/**< file identifier */
	/** callback when close is completed */
	void (*closed_callback)(cmyth_file_t file);
	int64_t file_start;	/**< file start offest */
	int64_t file_length;	/**< file length */
	int64_t file_pos;	/**< current file position */
	int64_t file_req;	/**< current file position requested */
	cmyth_conn_t file_control;	/**< master backend connection */
};

struct cmyth_ringbuf {
	cmyth_conn_t conn_data;
	uint32_t file_id;
	char *ringbuf_url;
	int64_t ringbuf_size;
	int64_t file_length;
	int64_t file_pos;
	int64_t ringbuf_fill;
	char *ringbuf_hostname;
	uint16_t ringbuf_port;
};

struct cmyth_rec_num {
	char *recnum_host;
	uint16_t recnum_port;
	int32_t recnum_id;
};

struct cmyth_keyframe {
	uint32_t keyframe_number;
	int64_t keyframe_pos;
};

struct cmyth_posmap {
	struct cmyth_keyframe **posmap_list;
	int posmap_count;
};

struct cmyth_freespace {
	int64_t freespace_total;
	int64_t freespace_used;
};

struct cmyth_timestamp {
	unsigned long timestamp_year;
	unsigned long timestamp_month;
	unsigned long timestamp_day;
	unsigned long timestamp_hour;
	unsigned long timestamp_minute;
	unsigned long timestamp_second;
	int timestamp_isdst;
	int timestamp_isutc;
};

struct cmyth_proginfo {
	char *proginfo_title;
	char *proginfo_subtitle;
	char *proginfo_description;
	uint16_t proginfo_season;    /* new in V67 */
	uint16_t proginfo_episode;    /* new in V67 */
	char *proginfo_syndicated_episode; /* new in V76 */
	char *proginfo_category;
	uint32_t proginfo_chanId;
	char *proginfo_chanstr;
	char *proginfo_chansign;
	char *proginfo_channame;  /* Deprecated in V8, simulated for compat. */
	char *proginfo_chanicon;  /* New in V8 */
	char *proginfo_url;
	int64_t proginfo_Length;
	cmyth_timestamp_t proginfo_start_ts;
	cmyth_timestamp_t proginfo_end_ts;
	uint32_t proginfo_conflicting; /* Deprecated in V8, always 0 */
	char *proginfo_unknown_0;   /* May be new 'conflicting' in V8 */
	uint32_t proginfo_recording;
	uint32_t proginfo_override;
	char *proginfo_hostname;
	uint32_t proginfo_source_id; /* ??? in V8 */
	uint32_t proginfo_card_id;   /* ??? in V8 */
	uint32_t proginfo_input_id;  /* ??? in V8 */
	int32_t proginfo_rec_priority;  /* ??? in V8 */
	int8_t proginfo_rec_status; /* ??? in V8 */
	uint32_t proginfo_record_id;  /* ??? in V8 */
	uint8_t proginfo_rec_type;   /* ??? in V8 */
	uint8_t proginfo_rec_dupin;   /* ??? in V8 */
	uint8_t proginfo_rec_dupmethod;  /* new in V8 */
	cmyth_timestamp_t proginfo_rec_start_ts;
	cmyth_timestamp_t proginfo_rec_end_ts;
	uint8_t proginfo_repeat;   /* ??? in V8 */
	uint32_t proginfo_program_flags;
	char *proginfo_rec_profile;  /* new in V8 */
	char *proginfo_recgroup;    /* new in V8 */
	char *proginfo_chancommfree;    /* new in V8 */
	char *proginfo_chan_output_filters;    /* new in V8 */
	char *proginfo_seriesid;    /* new in V8 */
	char *proginfo_programid;    /* new in V12 */
	char *proginfo_inetref;    /* new in V67 */
	cmyth_timestamp_t proginfo_lastmodified;    /* new in V12 */
	char *proginfo_stars;    /* new in V12 */
	cmyth_timestamp_t proginfo_originalairdate;	/* new in V12 */
	char *proginfo_pathname;
	uint16_t proginfo_port;
        uint8_t proginfo_hasairdate;
	char *proginfo_host;
	uint32_t proginfo_version;
	char *proginfo_playgroup; /* new in v18 */
	int32_t proginfo_recpriority_2;  /* new in V25 */
	uint32_t proginfo_parentid; /* new in V31 */
	char *proginfo_storagegroup; /* new in v32 */
	uint16_t proginfo_audioproperties; /* new in v35 */
	uint16_t proginfo_videoproperties; /* new in v35 */
	uint16_t proginfo_subtitletype; /* new in v35 */
	uint16_t proginfo_year; /* new in v43 */
	uint16_t proginfo_partnumber; /* new in V76 */
	uint16_t proginfo_parttotal; /* new in V76 */
};

struct cmyth_proglist {
	cmyth_proginfo_t *proglist_list;
	int proglist_count;
	pthread_mutex_t proglist_mutex;
};

/*
 * Private funtions in socket.c
 */
#define cmyth_send_message __cmyth_send_message
extern int cmyth_send_message(cmyth_conn_t conn, char *request);

#define cmyth_rcv_length __cmyth_rcv_length
extern int cmyth_rcv_length(cmyth_conn_t conn);

#define cmyth_rcv_string __cmyth_rcv_string
extern int cmyth_rcv_string(cmyth_conn_t conn,
			    int *err,
			    char *buf, int buflen,
			    int count);

#define cmyth_rcv_okay __cmyth_rcv_okay
extern int cmyth_rcv_okay(cmyth_conn_t conn);

#define cmyth_rcv_feedback __cmyth_rcv_feedback
extern int cmyth_rcv_feedback(cmyth_conn_t conn, char *fb, int fblen);

#define cmyth_rcv_version __cmyth_rcv_version
extern int cmyth_rcv_version(cmyth_conn_t conn, uint32_t *vers);

#define cmyth_rcv_int8 __cmyth_rcv_int8
extern int cmyth_rcv_int8(cmyth_conn_t conn, int *err, int8_t *buf, int count);

#define cmyth_rcv_int16 __cmyth_rcv_int16
extern int cmyth_rcv_int16(cmyth_conn_t conn, int *err, int16_t *buf, int count);

#define cmyth_rcv_int32 __cmyth_rcv_int32
extern int cmyth_rcv_int32(cmyth_conn_t conn, int *err, int32_t *buf, int count);

#define cmyth_rcv_old_int64 __cmyth_rcv_old_int64
extern int cmyth_rcv_old_int64(cmyth_conn_t conn, int *err, int64_t *buf, int count);

#define cmyth_rcv_new_int64 __cmyth_rcv_new_int64
extern int cmyth_rcv_new_int64(cmyth_conn_t conn, int *err, int64_t *buf, int count, int forced);

#define cmyth_rcv_int64(conn, err, buf, count)	cmyth_rcv_new_int64(conn, err, buf, count, 0)

#define cmyth_rcv_uint8 __cmyth_rcv_uint8
extern int cmyth_rcv_uint8(cmyth_conn_t conn, int *err, uint8_t *buf, int count);

#define cmyth_rcv_uint16 __cmyth_rcv_uint16
extern int cmyth_rcv_uint16(cmyth_conn_t conn, int *err, uint16_t *buf, int count);

#define cmyth_rcv_uint32 __cmyth_rcv_uint32
extern int cmyth_rcv_uint32(cmyth_conn_t conn, int *err, uint32_t *buf, int count);

#define cmyth_rcv_data __cmyth_rcv_data
extern int cmyth_rcv_data(cmyth_conn_t conn, int *err, unsigned char *buf, int buflen, int count);

#define cmyth_rcv_timestamp __cmyth_rcv_timestamp
extern int cmyth_rcv_timestamp(cmyth_conn_t conn, int *err,
			       cmyth_timestamp_t *ts_p,
			       int count);
#define cmyth_rcv_datetime __cmyth_rcv_datetime
extern int cmyth_rcv_datetime(cmyth_conn_t conn, int *err,
			      cmyth_timestamp_t *ts_p,
			      int count);

#define cmyth_rcv_proginfo __cmyth_rcv_proginfo
extern int cmyth_rcv_proginfo(cmyth_conn_t conn, int *err,
			      cmyth_proginfo_t buf,
			      int count);

#define cmyth_rcv_chaninfo __cmyth_rcv_chaninfo
extern int cmyth_rcv_chaninfo(cmyth_conn_t conn, int *err,
			      cmyth_proginfo_t buf,
			      int count);

#define cmyth_rcv_proglist __cmyth_rcv_proglist
extern int cmyth_rcv_proglist(cmyth_conn_t conn, int *err,
			      cmyth_proglist_t buf,
			      int count);

#define cmyth_rcv_keyframe __cmyth_rcv_keyframe
extern int cmyth_rcv_keyframe(cmyth_conn_t conn, int *err,
			      cmyth_keyframe_t buf,
			      int count);

#define cmyth_rcv_posmap __cmyth_rcv_posmap
extern int cmyth_rcv_posmap(cmyth_conn_t conn, int *err,
			    cmyth_posmap_t buf,
			    int count);

#define cmyth_rcv_freespace __cmyth_rcv_freespace
extern int cmyth_rcv_freespace(cmyth_conn_t conn, int *err,
			       cmyth_freespace_t buf,
			       int count);

#define cmyth_rcv_recorder __cmyth_rcv_recorder
extern int cmyth_rcv_recorder(cmyth_conn_t conn, int *err,
			      cmyth_recorder_t buf,
			      int count);

#define cmyth_rcv_ringbuf __cmyth_rcv_ringbuf
extern int cmyth_rcv_ringbuf(cmyth_conn_t conn, int *err, cmyth_ringbuf_t buf,
			     int count);

#define cmyth_toupper_string __cmyth_toupper_string
extern void cmyth_toupper_string(char *str);

/*
 * From proginfo.c
 */
#define cmyth_proginfo_string __cmyth_proginfo_string
extern char *cmyth_proginfo_string(cmyth_conn_t control, cmyth_proginfo_t prog);

/*
 * From file.c
 */
#define cmyth_file_create __cmyth_file_create
extern cmyth_file_t cmyth_file_create(cmyth_conn_t control);

/*
 * From timestamp.c
 */
#define cmyth_timestamp_diff __cmyth_timestamp_diff
extern int cmyth_timestamp_diff(cmyth_timestamp_t, cmyth_timestamp_t);

/*
 * From mythtv_mysql.c
 */

extern MYSQL * cmyth_db_get_connection(cmyth_database_t db);


/*
 * From mysql_query.c
 */

typedef struct cmyth_mysql_query_s cmyth_mysql_query_t;

extern cmyth_mysql_query_t * cmyth_mysql_query_create(cmyth_database_t db, const char * query_string);

extern void cmyth_mysql_query_reset(cmyth_mysql_query_t *query);

extern int cmyth_mysql_query_param_int32(cmyth_mysql_query_t * query, int32_t param);

extern int cmyth_mysql_query_param_uint32(cmyth_mysql_query_t * query, uint32_t param);

extern int cmyth_mysql_query_param_int64(cmyth_mysql_query_t * query, int64_t param);

extern int cmyth_mysql_query_param_int(cmyth_mysql_query_t * query, int param);

extern int cmyth_mysql_query_param_uint(cmyth_mysql_query_t * query, unsigned int param);

extern int cmyth_mysql_query_param_unixtime(cmyth_mysql_query_t * query, time_t param, int tz_utc);

extern int cmyth_mysql_query_param_str(cmyth_mysql_query_t * query, const char *param);

extern char * cmyth_mysql_query_string(cmyth_mysql_query_t * query);

extern MYSQL_RES * cmyth_mysql_query_result(cmyth_mysql_query_t * query);

extern int cmyth_mysql_query(cmyth_mysql_query_t * query);

extern char* cmyth_utf8tolatin1(char* s);

/*
 * From channel.c
 */
struct cmyth_channel {
	uint32_t chanid;
	uint32_t channum;
	char *chanstr;
	char *callsign;
	char *name;
	char *icon;
	uint8_t visible;
	uint8_t radio;
	uint32_t sourceid;
	uint32_t multiplex;
};

struct cmyth_chanlist {
	cmyth_channel_t *chanlist_list;
	int chanlist_count;
};

#define cmyth_channel_create __cmyth_channel_create
extern cmyth_channel_t cmyth_channel_create(void);

#define cmyth_chanlist_create __cmyth_chanlist_create
extern cmyth_chanlist_t cmyth_chanlist_create(void);

/*
 * From recordingrule.c
 */
struct cmyth_recordingrule {
	uint32_t recordid;
	uint32_t chanid;
	cmyth_timestamp_t starttime;
	cmyth_timestamp_t endtime;
	char* title;
	char* description;
	uint8_t type;                    //enum
	char* category;
	char* subtitle;
	int8_t recpriority;              //range -99,+99
	uint8_t startoffset;             //nb minutes
	uint8_t endoffset;               //nb minutes
	uint8_t searchtype;              //enum
	uint8_t inactive;                //bool
	char* callsign;
	uint8_t dupmethod;               //enum
	uint8_t dupin;                   //enum
	char* recgroup;
	char* storagegroup;
	char* playgroup;
	uint8_t autotranscode;           //bool
	uint8_t userjobs;                //#1111
	uint8_t autocommflag;            //bool
	uint8_t autoexpire;              //bool
	uint32_t maxepisodes;            //range 0,100
	uint8_t maxnewest;               //bool
	uint32_t transcoder;             //recordingprofiles id
	uint32_t parentid;               //parent rule recordid
	char* profile;
	uint32_t prefinput;
	char* programid;
	char* seriesid;
	uint8_t autometadata;            //DB version 1278
	char* inetref;                   //DB version 1278
	uint16_t season;                 //DB version 1278
	uint16_t episode;                //DB version 1278
	uint32_t filter;                 //DB version 1276
	};

struct cmyth_recordingrulelist {
	cmyth_recordingrule_t *recordingrulelist_list;
	int recordingrulelist_count;
};

#define cmyth_recordingrule_create __cmyth_recordingrule_create
extern cmyth_recordingrule_t cmyth_recordingrule_create(void);

#define cmyth_recordingrulelist_create __cmyth_recordingrulelist_create
extern cmyth_recordingrulelist_t cmyth_recordingrulelist_create(void);

/*
 * From storagegroup.c
 */
struct cmyth_storagegroup_file {
	char* filename;
	char* storagegroup;
	char* hostname;
	time_t lastmodified;
	int64_t size;
};

struct cmyth_storagegroup_filelist {
	cmyth_storagegroup_file_t *storagegroup_filelist_list;
	int storagegroup_filelist_count;
};

#define cmyth_storagegroup_file_create __cmyth_storagegroup_file_create
extern cmyth_storagegroup_file_t cmyth_storagegroup_file_create(void);

#define cmyth_storagegroup_filelist_create __cmyth_storagegroup_filelist_create
extern cmyth_storagegroup_filelist_t cmyth_storagegroup_filelist_create(void);

/*
 * From epginfo.c
 */
struct cmyth_epginfo {
	uint32_t chanid;
	char* callsign;
	char* channame;
	uint32_t sourceid;
	char* title;
	char* subtitle;
	char* description;
	time_t starttime;
	time_t endtime;
	char* programid;
	char* seriesid;
	char* category;
	char* category_type;
	uint32_t channum;
};

struct cmyth_epginfolist {
	cmyth_epginfo_t *epginfolist_list;
	int epginfolist_count;
};

#define cmyth_epginfo_create __cmyth_epginfo_create
extern cmyth_epginfo_t cmyth_epginfo_create(void);

#define cmyth_epginfolist_create __cmyth_epginfolist_create
extern cmyth_epginfolist_t cmyth_epginfolist_create(void);

/*
 * From mythtv_mysql.c
 */
#define cmyth_mysql_escape_chars __cmyth_mysql_escape_chars
extern char *cmyth_mysql_escape_chars(cmyth_database_t db, char * string);

#endif /* __CMYTH_LOCAL_H */
