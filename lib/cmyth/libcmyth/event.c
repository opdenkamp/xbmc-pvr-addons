/*
 *  Copyright (C) 2005-2012, Jon Gettler
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cmyth_local.h>

cmyth_event_t
cmyth_event_get(cmyth_conn_t conn, char * data, int32_t len)
{
	cmyth_proginfo_t proginfo = NULL;
	cmyth_event_t event = cmyth_event_get_message(conn, data, len, &proginfo);
	ref_release(proginfo);
	return event;
}

cmyth_event_t
cmyth_event_get_message(cmyth_conn_t conn, char * data, int32_t len, cmyth_proginfo_t * prog)
{
	int count, err, consumed;
	char tmp[1024];
	cmyth_event_t event;
	cmyth_proginfo_t proginfo = NULL;

	if (conn == NULL)
		return CMYTH_EVENT_UNKNOWN;

	if ((count = cmyth_rcv_length(conn)) <= 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		return CMYTH_EVENT_CLOSE;
	}

	data[0] = 0;

	consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
	count -= consumed;
	if (strcmp(tmp, "BACKEND_MESSAGE") != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, count);
		event = CMYTH_EVENT_UNKNOWN;
		goto out;
	}

	consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
	count -= consumed;

	/*
	 * RECORDING_LIST_CHANGE
	 */
	if (strcmp(tmp, "RECORDING_LIST_CHANGE") == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE;
	}

	/*
	 * RECORDING_LIST_CHANGE ADD
	 */
	else if (strncmp(tmp, "RECORDING_LIST_CHANGE ADD", 25) == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD;
		strncpy(data, tmp + 26, len);
	}

	/*
	 * RECORDING_LIST_CHANGE UPDATE
	 */
	else if (strcmp(tmp, "RECORDING_LIST_CHANGE UPDATE") == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE;
		proginfo = cmyth_proginfo_create();
		if (!proginfo) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				"%s: cmyth_proginfo_create() failed\n",
				__FUNCTION__);
			event = CMYTH_EVENT_UNKNOWN;
			goto out;
		}
		consumed = cmyth_rcv_proginfo(conn, &err, proginfo, count);
		count -= consumed;
		*prog = proginfo;
	}

	/*
	 * RECORDING_LIST_CHANGE DELETE
	 */
	else if (strncmp(tmp, "RECORDING_LIST_CHANGE DELETE", 28) == 0) {
		event = CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE;
		strncpy(data, tmp + 29, len);
	}

	/*
	 * SCHEDULE_CHANGE
	 */
	else if (strcmp(tmp, "SCHEDULE_CHANGE") == 0) {
		event = CMYTH_EVENT_SCHEDULE_CHANGE;
	}

	/*
	 * DONE_RECORDING
	 */
	else if (strncmp(tmp, "DONE_RECORDING", 14) == 0) {
		event = CMYTH_EVENT_DONE_RECORDING;
		strncpy(data, tmp + 15, len);
	}

	/*
	 * QUIT_LIVETV
	 */
	else if (strncmp(tmp, "QUIT_LIVETV", 11) == 0) {
		event = CMYTH_EVENT_QUIT_LIVETV;
	}

	/*
	 * LIVETV_WATCH
	 */
	else if (strncmp(tmp, "LIVETV_WATCH", 12) == 0) {
		event = CMYTH_EVENT_LIVETV_WATCH;
		strncpy(data, tmp + 13, len);
	}

	/*
	 * LIVETV_CHAIN UPDATE
	 */
	else if (strncmp(tmp, "LIVETV_CHAIN UPDATE", 19) == 0) {
		event = CMYTH_EVENT_LIVETV_CHAIN_UPDATE;
		strncpy(data, tmp + 20, len);
	}

	/*
	 * SIGNAL
	 */
	else if (strncmp(tmp, "SIGNAL", 6) == 0) {
		int32_t dstlen = len;
		event = CMYTH_EVENT_SIGNAL;
		/*Get Recorder ID */
		strncat(data, "cardid ", 7);
		strncat(data, tmp + 7, consumed - 12);
		strncat(data, ";", 2);
		/* get slock, signal, seen_pat, matching_pat */
		while (count > 0) {
			/* get signalmonitorvalue name */
			consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
			count -= consumed;
			/*strncat(data,tmp,dstlen-2);
			strncat(data,"=",2);
			dstlen -= consumed;*/
			/* get signalmonitorvalue status */
			consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
			count -= consumed;
			strncat(data,tmp,dstlen-2);
			strncat(data,";",2);
			dstlen -= consumed;
		}
	}

	/*
	 * ASK_RECORDING
	 */
	else if (strncmp(tmp, "ASK_RECORDING", 13) == 0) {
		event = CMYTH_EVENT_ASK_RECORDING;
		strncpy(data, tmp + 14, len);
		if (cmyth_conn_get_protocol_version(conn) >= 37) {
			proginfo = cmyth_proginfo_create();
			if (!proginfo) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					"%s: cmyth_proginfo_create() failed\n",
					__FUNCTION__);
				event = CMYTH_EVENT_UNKNOWN;
				goto out;
			}
			consumed = cmyth_rcv_proginfo(conn, &err, proginfo, count);
			count -= consumed;
			*prog = proginfo;
		}
	}

	/*
	 * CLEAR_SETTINGS_CACHE
	 */
	else if (strncmp(tmp, "CLEAR_SETTINGS_CACHE", 20) == 0) {
		event = CMYTH_EVENT_CLEAR_SETTINGS_CACHE;
	}

	/*
	 * GENERATED_PIXMAP
	 */
	else if (strncmp(tmp, "GENERATED_PIXMAP", 16) == 0) {
		/* capture the file which a pixmap has been generated for */
		event = CMYTH_EVENT_GENERATED_PIXMAP;
		consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
		count -= consumed;
		if (strncmp(tmp, "OK", 2) == 0) {
			/* receive <chanid_timestamp (isoformat)> */
			consumed = cmyth_rcv_string(conn, &err, tmp, sizeof(tmp) - 1, count);
			count -= consumed;
			strncpy(data, tmp, len);
		}
	}

	/*
	 * SYSTEM_EVENT
	 */
	else if (strncmp(tmp, "SYSTEM_EVENT", 12) == 0) {
		event = CMYTH_EVENT_SYSTEM_EVENT;
		strncpy(data, tmp + 13, len);
	}

	/*
	 * UPDATE_FILE_SIZE
	 */
	else if (strncmp(tmp, "UPDATE_FILE_SIZE", 16) == 0) {
		event = CMYTH_EVENT_UPDATE_FILE_SIZE;
		strncpy(data, tmp + 17, len);
	}

	/*
	 * Unknown message
	 */
	else {
		cmyth_dbg(CMYTH_DBG_INFO,
			  "%s: unknown BACKEND_MESSAGE '%s'\n", __FUNCTION__, tmp);
		event = CMYTH_EVENT_UNKNOWN;
	}

	out:
	while(count > 0 && err == 0) {
		consumed = cmyth_rcv_data(conn, &err, tmp, sizeof(tmp) - 1, count);
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: leftover data: count %i, read %i, errno %i\n", __FUNCTION__, count, consumed, err);
		count -= consumed;
	}

	return event;
}

int
cmyth_event_select(cmyth_conn_t conn, struct timeval *timeout)
{
	fd_set fds;
	int ret;
	cmyth_socket_t fd;

	// Trace functions might flood log
	//cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) {\n", __FUNCTION__,
	//			__FILE__, __LINE__);

	if (conn == NULL)
		return -EINVAL;

	fd = conn->conn_fd;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	ret = select((int)fd+1, &fds, NULL, NULL, timeout);

	// Trace functions might flood log
	//cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) }\n",
	//			__FUNCTION__, __FILE__, __LINE__);

	return ret;
}
