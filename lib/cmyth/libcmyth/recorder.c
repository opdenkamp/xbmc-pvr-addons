/*
 *  Copyright (C) 2004-2009, Eric Lund, Jon Gettler
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

/*
 * recorder.c -   functions to handle operations on MythTV recorders.  A
 *                MythTV Recorder is the part of the MythTV backend that
 *                handles capturing video from a video capture card and
 *                storing it in files for transfer to a backend.  A
 *                recorder is the key to live-tv streams as well, and
 *                owns the tuner and channel information (i.e. program
 *                guide data).
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <cmyth_local.h>

/*
 * cmyth_recorder_destroy()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Clean up and free a recorder structure.  This should only be done
 * by the ref_release() code.  Everyone else should call
 * ref_release() because recorder structures are reference
 * counted.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_recorder_destroy(cmyth_recorder_t rec)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!rec) {
		return;
	}

	if (rec->rec_server) {
		ref_release(rec->rec_server);
	}
	if (rec->rec_ring) {
		ref_release(rec->rec_ring);
	}
	if (rec->rec_conn) {
		ref_release(rec->rec_conn);
	}
	if (rec->rec_livetv_chain) {
		ref_release(rec->rec_livetv_chain);
	}
	if (rec->rec_livetv_file) {
		ref_release(rec->rec_livetv_file);
	}

}

/*
 * cmyth_recorder_create()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Allocate and initialize a cmyth recorder structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_recorder_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_recorder_t
 */
cmyth_recorder_t
cmyth_recorder_create(void)
{
	cmyth_recorder_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ret) {
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_recorder_destroy);

	ret->rec_server = NULL;
	ret->rec_port = 0;
	ret->rec_have_stream = 0;
	ret->rec_id = 0;
	ret->rec_ring = NULL;
	ret->rec_conn = NULL;
	ret->rec_framerate = 0.0;
	ret->rec_livetv_chain = NULL;
	ret->rec_livetv_file = NULL;
	return ret;
}

/*
 * cmyth_recorder_dup()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Copy a recorder structure and return the copy
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_recorder_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_recorder_t
 */
cmyth_recorder_t
cmyth_recorder_dup(cmyth_recorder_t old)
{
	cmyth_recorder_t ret = cmyth_recorder_create();
	if (ret == NULL) {
		return NULL;
	}

	ret->rec_have_stream = old->rec_have_stream;
	ret->rec_id = old->rec_id;
	ret->rec_server = ref_hold(old->rec_server);
	ret->rec_port = old->rec_port;
	ret->rec_ring = ref_hold(old->rec_ring);
	ret->rec_conn = ref_hold(old->rec_conn);
	ret->rec_framerate = old->rec_framerate;
	ret->rec_livetv_chain = ref_hold(old->rec_livetv_chain);
	ret->rec_livetv_file = ref_hold(old->rec_livetv_file);

	return ret;
}

/*
 * cmyth_recorder_is_recording()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Determine whether recorder 'rec' is currently recording.  Return
 * the true / false answer.
 *
 * Return Value:
 *
 * Success: 0 if the recorder is idle, 1 if the recorder is recording.
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_is_recording(cmyth_recorder_t rec)
{
	int err, count;
	int r;
	uint8_t c;
	int ret;
	char msg[256];

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]IS_RECORDING",
		 rec->rec_id);

	if ((r = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, r);
		ret = r;
		goto out;
	}

	count = cmyth_rcv_length(rec->rec_conn);
	if ((r = cmyth_rcv_uint8(rec->rec_conn, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto out;
	}

	ret = c;

    out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_get_framerate()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the current frame rate for recorder 'rec'.  Put the result
 * in 'rate'.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_get_framerate(cmyth_recorder_t rec,
			     double *rate)
{
	int err, count;
	int r;
	long ret;
	char msg[256];
	char reply[256];

	if (!rec || !rate) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]GET_FRAMERATE",
		 rec->rec_id);

	if ((r = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, r);
		ret = r;
		goto out;
	}

	count = cmyth_rcv_length(rec->rec_conn);
	if ((r = cmyth_rcv_string(rec->rec_conn, &err,
				reply, sizeof(reply), count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto out;
	}

	*rate = atof(reply);
	ret = 0;

    out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_get_frames_written()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the current number of frames written by recorder 'rec'.
 *
 * Return Value:
 *
 * Success: long long number of frames (>= 0)
 *
 * Failure: long long -(ERRNO)
 */
int64_t
cmyth_recorder_get_frames_written(cmyth_recorder_t rec)
{
	int r, err, count;
	int64_t ret;
	char msg[256];

	if (!rec || !rec->rec_conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return (int64_t) -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	if (rec->rec_conn->conn_version >= 66)
	{
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]GET_FRAMES_WRITTEN", rec->rec_id);

		if ((r = cmyth_send_message(rec->rec_conn, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
				  __FUNCTION__, r);
			ret = r;
			goto fail;
		}

		count = cmyth_rcv_length(rec->rec_conn);
		if ((r = cmyth_rcv_int64(rec->rec_conn, &err, &ret, count)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, r);
			ret = err;
			goto fail;
		}
	}
	else
	{
		ret = (int64_t) -EPERM;
	}

fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_get_free_space()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the current number of bytes of free space on the recorder
 * 'rec'.
 *
 * Return Value:
 *
 * Success: long long freespace (>= 0)
 *
 * Failure: long long -(ERRNO)
 */
int64_t
cmyth_recorder_get_free_space(cmyth_recorder_t rec)
{
	return (int64_t) -ENOSYS;
}

/*
 * cmyth_recorder_get_keyframe_pos()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the position in a recording of the key frame number 'keynum'
 * on the recorder 'rec'.
 *
 * Return Value:
 *
 * Success: long long seek offset >= 0
 *
 * Failure: long long -(ERRNO)
 */
int64_t
cmyth_recorder_get_keyframe_pos(cmyth_recorder_t rec, uint32_t keynum)
{
	int r, err, count;
	int64_t ret;
	char msg[256];

	if (!rec || !rec->rec_conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return (int64_t) -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	if (rec->rec_conn->conn_version >= 66)
	{
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]GET_KEYFRAME_POS[]:[]%"PRIu32, rec->rec_id, keynum);

		if ((r = cmyth_send_message(rec->rec_conn, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
				  __FUNCTION__, r);
			ret = r;
			goto fail;
		}

		count = cmyth_rcv_length(rec->rec_conn);
		if ((r = cmyth_rcv_int64(rec->rec_conn, &err, &ret, count)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, r);
			ret = err;
			goto fail;
		}
	}
	else
	{
		ret = (int64_t) -EPERM;
	}

fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_get_position_map()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a list of {keynum, position} pairs starting at keynum
 * 'start' and ending with keynum 'end' from the current recording on
 * recorder 'rec'.
 *
 * Return Value:
 *
 * Success: A non-NULL, held cmyth_posmap_t
 * Failure: A NULL pointer
 */
cmyth_posmap_t
cmyth_recorder_get_position_map(cmyth_recorder_t rec, uint32_t start, uint32_t end)
{
	int err, count, consumed;
	cmyth_posmap_t ret = NULL;
	char msg[256];
	char tmp[1024];

	if (!rec || !rec->rec_conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	if (rec->rec_conn->conn_version >= 43)
	{
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]FILL_POSITION_MAP[]:[]%"PRIu32"[]:[]%"PRIu32, rec->rec_id, start, end);

		if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
				  __FUNCTION__, err);
			goto fail;
		}

		count = cmyth_rcv_length(rec->rec_conn);
		if (count < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_length() failed (%d)\n",
			__FUNCTION__, count);
			goto fail;
		}

		ret = cmyth_posmap_create();
		if (ret == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_posmap_create() failed\n",
				  __FUNCTION__);
			goto fail;
		}

		if (count > 2) {
			consumed = cmyth_rcv_posmap(rec->rec_conn, &err, ret, count);
			count -= consumed;
			if (err) {
				cmyth_dbg(CMYTH_DBG_ERROR,
				"%s: cmyth_rcv_posmap() failed (%d)\n",
				__FUNCTION__, err);
				ref_release(ret);
				ret = NULL;
			}
		}
		err = 0;
		while(count > 0 && err == 0) {
			consumed = cmyth_rcv_data(rec->rec_conn, &err, (unsigned char*)tmp, sizeof(tmp) - 1, count);
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: leftover data: count %i, read %i, errno %i\n", __FUNCTION__, count, consumed, err);
			count -= consumed;
		}
	}

fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_get_duration_map()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a list of {keynum, duration} pairs starting at keynum
 * 'start' and ending with keynum 'end' from the current recording on
 * recorder 'rec'.
 *
 * Return Value:
 *
 * Success: A non-NULL, held cmyth_posmap_t
 * Failure: A NULL pointer
 */
cmyth_posmap_t
cmyth_recorder_get_duration_map(cmyth_recorder_t rec, uint32_t start, uint32_t end)
{
	int err, count, consumed;
	cmyth_posmap_t ret = NULL;
	char msg[256];
	char tmp[1024];

	if (!rec || !rec->rec_conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	if (rec->rec_conn->conn_version >= 77)
	{
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]FILL_DURATION_MAP[]:[]%"PRIu32"[]:[]%"PRIu32, rec->rec_id, start, end);

		if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
				  __FUNCTION__, err);
			goto fail;
		}

		count = cmyth_rcv_length(rec->rec_conn);
		if (count < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_length() failed (%d)\n",
			__FUNCTION__, count);
			goto fail;
		}

		ret = cmyth_posmap_create();
		if (ret == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_posmap_create() failed\n",
				  __FUNCTION__);
			goto fail;
		}

		if (count > 2) {
			consumed = cmyth_rcv_posmap(rec->rec_conn, &err, ret, count);
			count -= consumed;
			if (err) {
				cmyth_dbg(CMYTH_DBG_ERROR,
				"%s: cmyth_rcv_posmap() failed (%d)\n",
				__FUNCTION__, err);
				ref_release(ret);
				ret = NULL;
			}
		}
		err = 0;
		while(count > 0 && err == 0) {
			consumed = cmyth_rcv_data(rec->rec_conn, &err, (unsigned char*)tmp, sizeof(tmp) - 1, count);
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: leftover data: count %i, read %i, errno %i\n", __FUNCTION__, count, consumed, err);
			count -= consumed;
		}
	}

fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_get_recording()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain from the recorder specified by 'rec' program information for
 * the current recording (i.e. the recording being made right now).
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
cmyth_proginfo_t
cmyth_recorder_get_recording(cmyth_recorder_t rec)
{
	return NULL;
}

/*
 * cmyth_recorder_stop_playing()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' stop playing the current recording
 * or live-tv stream.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_stop_playing(cmyth_recorder_t rec)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_frontend_ready()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Let the recorder 'rec' know that the frontend is ready for data.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_frontend_ready(cmyth_recorder_t rec)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_cancel_next_recording()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' cancels its next scheduled recording.
 * This is used as response to ASK_RECORDING when the user does not want
 * to allow the recorder to be taken for a pending recording.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_cancel_next_recording(cmyth_recorder_t rec, int cancel)
{
	int err;
	int ret = -1;
	char msg[256];

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n", __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]CANCEL_NEXT_RECORDING[]:[]%"PRIu32 ,rec->rec_id, cancel == 1);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_send_message() failed (%d)\n", __FUNCTION__, err);
		goto fail;
	}

	if ((err = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed (%d)\n", __FUNCTION__, err);
		goto fail;
	}

	ret = 0;

fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_pause()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' pause in the middle of the current
 * recording.  This will prevent the recorder from transmitting any
 * data until the recorder is unpaused.  At this moment, it is not
 * clear to me what will cause an unpause.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_pause(cmyth_recorder_t rec)
{
	int ret = -EINVAL;
	char Buffer[255];

	if (rec == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: Invalid args rec = %p\n",
			  __FUNCTION__, rec);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	sprintf(Buffer, "QUERY_RECORDER %"PRIu32"[]:[]PAUSE", rec->rec_id);
	if ((ret = cmyth_send_message(rec->rec_conn, Buffer)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, Buffer);
		goto err;
	}

	if ((ret = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto err;
	}

	ret = 0;

    err:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_finish_recording()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' stop recording the current
 * recording / live-tv.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_finish_recording(cmyth_recorder_t rec)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_toggle_channel_favorite()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' switch among the favorite channels.
 * Note that the recorder must not be actively recording when this
 * request is made or bad things may happen to the server (i.e. it may
 * segfault).
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_toggle_channel_favorite(cmyth_recorder_t rec)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_change_channel()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' change channels in one of the
 * followin
 *
 * CHANNEL_DIRECTION_UP       - Go up one channel in the listing
 *
 * CHANNEL_DIRECTION_DOWN     - Go down one channel in the listing
 *
 * CHANNEL_DIRECTION_FAVORITE - Go to the next favorite channel
 *
 * CHANNEL_DIRECTION_SAME     - Stay on the same (current) channel
 *
 * Note that the recorder must not be actively recording when this
 * request is made or bad things may happen to the server (i.e. it may
 * segfault).
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_change_channel(cmyth_recorder_t rec,
			      cmyth_channeldir_t direction)
{
	int err;
	int ret = -1;
	char msg[256];
	cmyth_livetv_chain_t newchain = NULL;
	cmyth_livetv_chain_t oldchain = NULL;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg),
		 "QUERY_RECORDER %"PRIu32"[]:[]CHANGE_CHANNEL[]:[]%d",
		 rec->rec_id, direction);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto out;
	}

	if ((err = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_okay() failed (%d)\n",
			  __FUNCTION__, err);
		goto out;
	}

	if (rec->rec_ring)
		rec->rec_ring->file_pos = 0;
	else {
		oldchain = rec->rec_livetv_chain;
		newchain = cmyth_livetv_chain_create(oldchain->chainid);
		newchain->livetv_buflen = oldchain->livetv_buflen;
		newchain->livetv_tcp_rcvbuf = oldchain->livetv_tcp_rcvbuf;
		newchain->prog_update_callback = oldchain->prog_update_callback;
		newchain->chain_switch_on_create = 1;
		rec->rec_livetv_chain = newchain;
	}

	ret = 0;

out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);
	if (oldchain)
		ref_release(oldchain);
	return ret;
}

/*
 * cmyth_recorder_set_channel()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' change channels to the channel
 * named 'channame'.
 *
 * Note that the recorder must not be actively recording when this
 * request is made or bad things may happen to the server (i.e. it may
 * segfault).
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_set_channel(cmyth_recorder_t rec, char *channame)
{
	int err;
	int ret = -1;
	char msg[256];
	cmyth_livetv_chain_t newchain = NULL;
	cmyth_livetv_chain_t oldchain = NULL;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg),
		 "QUERY_RECORDER %"PRIu32"[]:[]SET_CHANNEL[]:[]%s",
		 rec->rec_id, channame);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto out;
	}

	if ((err = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_okay() failed (%d)\n",
			  __FUNCTION__, err);
		goto out;
	}

	if (rec->rec_ring)
		rec->rec_ring->file_pos = 0;
	else {
		oldchain = rec->rec_livetv_chain;
		newchain = cmyth_livetv_chain_create(oldchain->chainid);
		newchain->livetv_buflen = oldchain->livetv_buflen;
		newchain->livetv_tcp_rcvbuf = oldchain->livetv_tcp_rcvbuf;
		newchain->prog_update_callback = oldchain->prog_update_callback;
		newchain->chain_switch_on_create = 1;
		rec->rec_livetv_chain = newchain;
	}

	ret = 0;

out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);
	if (oldchain)
		ref_release(oldchain);
	return ret;
}

/*
 * cmyth_recorder_change_color()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' change the color saturation of the
 * currently recording channel (this setting is preserved in the
 * recorder settings for that channel).  The change is controlled by
 * the value of 'direction' which may be:
 *
 *  ADJ_DIRECTION_UP           - Change the value upward one step
 *
 *	ADJ_DIRECTION_DOWN         - Change the value downward one step
 *
 * This may be done while the recorder is recording.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_change_color(cmyth_recorder_t rec, cmyth_adjdir_t direction)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_change_brightness()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' change the brightness (black level)
 * of the currently recording channel (this setting is preserved in
 * the recorder settings for that channel).  The change is controlled
 * by the value of 'direction' which may be:
 *
 *  ADJ_DIRECTION_UP           - Change the value upward one step
 *
 *	ADJ_DIRECTION_DOWN         - Change the value downward one step
 *
 * This may be done while the recorder is recording.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_change_brightness(cmyth_recorder_t rec,
				 cmyth_adjdir_t direction)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_change_contrast()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' change the contrast of the
 * currently recording channel (this setting is preserved in the
 * recorder settings for that channel).  The change is controlled by
 * the value of 'direction' which may be:
 *
 *  ADJ_DIRECTION_UP           - Change the value upward one step
 *
 *	ADJ_DIRECTION_DOWN         - Change the value downward one step
 *
 * This may be done while the recorder is recording.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_change_contrast(cmyth_recorder_t rec, cmyth_adjdir_t direction)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_change_hue()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' change the hue (color balance) of
 * the currently recording channel (this setting is preserved in the
 * recorder settings for that channel).  The change is controlled by
 * the value of 'direction' which may be:
 *
 *  ADJ_DIRECTION_UP           - Change the value upward one step
 *
 *  ADJ_DIRECTION_DOWN         - Change the value downward one step
 *
 * This may be done while the recorder is recording.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_change_hue(cmyth_recorder_t rec, cmyth_adjdir_t direction)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_check_channel()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' check the validity of the channel
 * specified by 'channame'.
 *
 * Return Value:
 *
 * Check the validity of a channel name.
 * Success: 1 - valid channel, 0 - invalid channel
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_check_channel(cmyth_recorder_t rec,
                             char *channame)
{
	int err;
	int ret = -1;
	char msg[256];

	if (!rec || !channame) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: invalid args "
			  "rec = %p, channame = %p\n",
			  __FUNCTION__, rec, channame);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg),
		 "QUERY_RECORDER %"PRIu32"[]:[]CHECK_CHANNEL[]:[]%s",
		 rec->rec_id, channame);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	if ((err = cmyth_rcv_feedback(rec->rec_conn, "1", 1)) < 0) {
		ret = 0;
		goto fail;
	}

	ret = 1;

    fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_check_channel_prefix()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request that the recorder 'rec' check the validity of the channel
 * prefix specified by 'channame'.
 *
 * Return Value:
 *
 * Success: 1 - valid channel, 0 - invalid channel
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_check_channel_prefix(cmyth_recorder_t rec, char *channame)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_get_program_info()
 *
 * Scope: PRIVATE (static)
 *
 * Description:
 *
 * Request program information from the recorder 'rec' for the current
 * program in the program guide (i.e. current channel and time slot).
 *
 * The program information will be used to fill out 'proginfo'.
 *
 * This does not affect the current recording.
 *
 * Return Value:
 *
 * Success: 1 - valid channel, 0 - invalid channel
 *
 * Failure: -(ERRNO)
 */
static cmyth_proginfo_t
cmyth_recorder_get_program_info(cmyth_recorder_t rec)
{
	int err, count, ct;
	char msg[256];
	cmyth_proginfo_t proginfo = NULL;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return NULL;
	}

	proginfo = cmyth_proginfo_create();
	if (proginfo == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: proginfo_create failed\n",
			  __FUNCTION__);
		goto out;
	}
	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	if (rec->rec_conn->conn_version >= 26)
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]GET_CURRENT_RECORDING",
		 	rec->rec_id);
	else
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]GET_PROGRAM_INFO",
		 	rec->rec_id);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ref_release(proginfo);
		proginfo = NULL;
		goto out;
	}

	count = cmyth_rcv_length(rec->rec_conn);

	if (rec->rec_conn->conn_version >= 26)
		ct = cmyth_rcv_proginfo(rec->rec_conn, &err, proginfo, count);
	else
		ct = cmyth_rcv_chaninfo(rec->rec_conn, &err, proginfo, count);

	if (ct != count) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_proginfo() < count\n", __FUNCTION__);
		ref_release(proginfo);
		proginfo = NULL;
		goto out;
	}

  out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return proginfo;
}

/*
 * cmyth_recorder_get_cur_proginfo()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Request program information from the recorder 'rec' for the first
 * program in the program guide (i.e. current channel and time slot).
 *
 * This does not affect the current recording.
 *
 * Return Value:
 *
 * Success: A non-NULL, held cmyth_proginfo_t
 * Failure: A NULL pointer
 *
 */
cmyth_proginfo_t
cmyth_recorder_get_cur_proginfo(cmyth_recorder_t rec)
{
	cmyth_proginfo_t ret;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: invalid args rec = %p\n",
			  __FUNCTION__, rec);
		return NULL;
	}
	if ((ret = cmyth_recorder_get_program_info(rec)) == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_recorder_get_program_info() failed\n",
			  __FUNCTION__);
		return NULL;
	}
	return ret;
}

/*
 * cmyth_recorder_get_next_program_info()
 *
 * Scope: PRIVATE (static)
 *
 * Description:
 *
 * Request program information from the recorder 'rec' for the next
 * program in the program guide from the current program (i.e. current
 * channel and time slot) in the direction specified by 'direction'
 * which may have any of the following values:
 *
 *     BROWSE_DIRECTION_SAME        - Stay in the same place
 *     BROWSE_DIRECTION_UP          - Move up one slot (down one channel)
 *     BROWSE_DIRECTION_DOWN        - Move down one slot (up one channel)
 *     BROWSE_DIRECTION_LEFT        - Move left one slot (down one time slot)
 *     BROWSE_DIRECTION_RIGHT       - Move right one slot (up one time slot)
 *     BROWSE_DIRECTION_FAVORITE    - Move to the next favorite slot
 *
 * The program information will be used to fill out 'proginfo'.
 *
 * This does not affect the current recording.
 *
 * Return Value:
 *
 * Success: 1 - valid channel, 0 - invalid channel
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_get_next_program_info(cmyth_recorder_t rec,
				     cmyth_proginfo_t cur_prog,
				     cmyth_proginfo_t next_prog,
				     cmyth_browsedir_t direction)
{
        int err, count;
        int ret = -1;
        char msg[256];
        char title[256], subtitle[256], desc[256], category[256];
	char callsign[256], iconpath[256];
	char channelname[256], chanid[256], seriesid[256], programid[256];
	char date[256];
	struct tm *tm;
	time_t t;
	cmyth_conn_t control;

        if (!rec) {
                cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
                          __FUNCTION__);
                return -EINVAL;
        }

	control = rec->rec_conn;

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	t = time(NULL);
	tm = localtime(&t);
	snprintf(date, sizeof(date), "%.4d%.2d%.2d%.2d%.2d%.2d",
		 tm->tm_year + 1900, tm->tm_mon + 1,
		 tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

        snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]GET_NEXT_PROGRAM_INFO[]:[]%s[]:[]%"PRIu32"[]:[]%d[]:[]%s",
                 rec->rec_id, cur_prog->proginfo_channame,
		 cur_prog->proginfo_chanId, direction, date);

        if ((err = cmyth_send_message(control, msg)) < 0) {
                cmyth_dbg(CMYTH_DBG_ERROR,
                          "%s: cmyth_send_message() failed (%d)\n",
                          __FUNCTION__, err);
                ret = err;
                goto out;
        }

        count = cmyth_rcv_length(control);

	count -= cmyth_rcv_string(control, &err,
				  title, sizeof(title), count);
	count -= cmyth_rcv_string(control, &err,
				  subtitle, sizeof(subtitle), count);
	count -= cmyth_rcv_string(control, &err,
				  desc, sizeof(desc), count);
	count -= cmyth_rcv_string(control, &err,
				  category, sizeof(category), count);
	count -= cmyth_rcv_timestamp(control, &err,
				     &next_prog->proginfo_start_ts, count);
	count -= cmyth_rcv_timestamp(control, &err,
				     &next_prog->proginfo_end_ts, count);
	count -= cmyth_rcv_string(control, &err,
				  callsign, sizeof(callsign), count);
	count -= cmyth_rcv_string(control, &err,
				  iconpath, sizeof(iconpath), count);
	count -= cmyth_rcv_string(control, &err,
				  channelname, sizeof(channelname), count);
	count -= cmyth_rcv_string(control, &err,
				  chanid, sizeof(chanid), count);
	if (control->conn_version >= 12) {
		count -= cmyth_rcv_string(control, &err,
					  seriesid, sizeof(seriesid), count);
		count -= cmyth_rcv_string(control, &err,
					  programid, sizeof(programid), count);
	}

	if (count != 0) {
		ret = -1;
		goto out;
	}

	/*
	 * if the program info is blank, return an error
	 */
	if ((strlen(title) == 0) && (strlen(subtitle) == 0) &&
	    (strlen(desc) == 0) && (strlen(channelname) == 0) &&
	    (strlen(chanid) == 0)) {
                cmyth_dbg(CMYTH_DBG_ERROR,
                          "%s: blank channel found\n", __FUNCTION__);
		ret = -1;
		goto out;
	}

	next_prog->proginfo_title = ref_strdup(title);
	next_prog->proginfo_subtitle = ref_strdup(subtitle);
	next_prog->proginfo_description = ref_strdup(desc);
	next_prog->proginfo_channame = ref_strdup(channelname);
	next_prog->proginfo_chanstr = ref_strdup(channelname);
	if (control->conn_version > 40) { /* we are hoping this is fixed by next version bump */
		next_prog->proginfo_chansign = ref_strdup(callsign);
	} else {
		next_prog->proginfo_chansign = cmyth_utf8tolatin1(callsign);
	}
	next_prog->proginfo_chanicon = ref_strdup(iconpath);

	next_prog->proginfo_chanId = atoi(chanid);

	ref_hold(next_prog->proginfo_start_ts);
	ref_hold(next_prog->proginfo_end_ts);

	ret = 0;

    out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

        return ret;
}

/*
 * cmyth_recorder_get_next_proginfo()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Request program information from the recorder 'rec' for the next
 * program in the program guide from the current program (i.e. current
 * channel and time slot) in the direction specified by 'direction'
 * which may have any of the following values:
 *
 *     BROWSE_DIRECTION_SAME        - Stay in the same place
 *     BROWSE_DIRECTION_UP          - Move up one slot (down one channel)
 *     BROWSE_DIRECTION_DOWN        - Move down one slot (up one channel)
 *     BROWSE_DIRECTION_LEFT        - Move left one slot (down one time slot)
 *     BROWSE_DIRECTION_RIGHT       - Move right one slot (up one time slot)
 *     BROWSE_DIRECTION_FAVORITE    - Move to the next favorite slot
 *
 * This does not affect the current recording.
 *
 * Return Value:
 *
 * Success: A held non-NULL cmyth_proginfo_t
 * Failure: A NULL pointer
 */
cmyth_proginfo_t
cmyth_recorder_get_next_proginfo(cmyth_recorder_t rec,
				 cmyth_proginfo_t current,
				 cmyth_browsedir_t direction)
{
	cmyth_proginfo_t ret;

        if (!rec || !current) {
                cmyth_dbg(CMYTH_DBG_ERROR, "%s: invalid args "
			  "rec =%p, current = %p\n",
                          __FUNCTION__, rec, current);
                return NULL;
        }
	ret = cmyth_proginfo_create();
	if (ret == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_proginfo_create() failed\n",
			  __FUNCTION__);
		return NULL;
	}
	if (cmyth_recorder_get_next_program_info(rec, current,
						 ret, direction) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_recorder_get_next_program_info()\n",
			  __FUNCTION__);
		ref_release(ret);
		return NULL;
	}
	return ret;
}

/*
 * cmyth_recorder_get_input_name()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Request the current input name from the recorder 'rec'.
 *
 * The input name up to 'len' bytes will be placed in the user
 * supplied string buffer 'name' which must be large enough to hold
 * 'len' bytes.  If the name is greater than or equal to 'len' bytes
 * long, the first 'len' - 1 bytes will be placed in 'name' followed
 * by a '\0' terminator.
 *
 * This does not affect the current recording.
 *
 * Return Value:
 *
 * Success: 1 - valid channel, 0 - invalid channel
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_get_input_name(cmyth_recorder_t rec,
			      char *name,
			      unsigned len)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_seek()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Request the recorder 'rec' to seek to the offset specified by 'pos'
 * using the specifier 'whence' to indicate how to perform the seek.
 * The value of 'whence' may be:
 *
 *    WHENCE_SET - set the seek offset absolutely from the beginning
 *                 of the stream.
 *
 *    WHENCE_CUR - set the seek offset relatively from the current
 *                 offset (as specified in 'curpos').
 *
 *    WHENCE_END - set the seek offset absolutely from the end
 *                 of the stream.
 *
 *
 * Return Value:
 *
 * Success: long long current offset >= 0
 *
 * Failure: (long long) -(ERRNO)
 */
int64_t
cmyth_recorder_seek(cmyth_recorder_t rec,
                    int64_t pos,
		    int8_t whence,
		    int64_t curpos)

{
	return (int64_t) -ENOSYS;
}

/*
 * cmyth_recorder_spawn_livetv()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Request the recorder 'rec' to start recording live-tv on its
 * current channel.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_spawn_livetv(cmyth_recorder_t rec)
{
	int err;
	int ret = -1;
	char msg[256];

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]SPAWN_LIVETV",
		 rec->rec_id);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	if ((err = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_okay() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	ret = 0;

    fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/* Sergio: Added to support the new livetv protocol */
int
cmyth_recorder_spawn_chain_livetv(cmyth_recorder_t rec, char* channame)
{
	int err;
	int ret = -1;
	char msg[256];
	char myhostname[32];
	char datestr[32];
	time_t t;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);


	/* Get our own IP address */
	gethostname(myhostname, 32);

	/* Get the current date and time to create a unique id */
	t = time(NULL);
	strftime(datestr, 32, "%Y-%m-%dT%H:%M:%S", localtime(&t));

	/* Now build the SPAWN_LIVETV message */
	if (rec->rec_conn->conn_version >= 34 && channame)
		snprintf(msg, sizeof(msg),
			"QUERY_RECORDER %"PRIu32"[]:[]SPAWN_LIVETV[]:[]live-%s-%s[]:[]%d[]:[]%s",
			 rec->rec_id, myhostname, datestr, 0, channame);
	else
		snprintf(msg, sizeof(msg),
			"QUERY_RECORDER %"PRIu32"[]:[]SPAWN_LIVETV[]:[]live-%s-%s[]:[]%d",
			 rec->rec_id, myhostname, datestr, 0);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	if ((err = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_okay() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	/* Create an empty livetv chain with the ID used in the spawn command */
	snprintf(msg, sizeof(msg), "live-%s-%s", myhostname, datestr);
	rec->rec_livetv_chain = cmyth_livetv_chain_create(msg);

	ret = 0;

    fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

int
cmyth_recorder_stop_livetv(cmyth_recorder_t rec)
{
	int err;
	int ret = -1;
	char msg[256];

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]STOP_LIVETV",
		 rec->rec_id);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	if ((err = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_okay() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	ret = 0;

    fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

int
cmyth_recorder_done_ringbuf(cmyth_recorder_t rec)
{
	int err;
	int ret = -1;
	char msg[256];

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	if (rec->rec_conn->conn_version >= 26)
		return 0;

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_RECORDER %"PRIu32"[]:[]DONE_RINGBUF",
		 rec->rec_id);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	if ((err = cmyth_rcv_okay(rec->rec_conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_okay() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	ret = 0;

    fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}

/*
 * cmyth_recorder_start_stream()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Request the recorder 'rec' to start a stream of the current
 * recording (or live-tv).
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_start_stream(cmyth_recorder_t rec)
{
	return -ENOSYS;
}

/*
 * cmyth_recorder_end_stream()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Request the recorder 'rec' to end a stream of the current recording
 * (or live-tv).
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_recorder_end_stream(cmyth_recorder_t rec)
{
	return -ENOSYS;
}

char*
cmyth_recorder_get_filename(cmyth_recorder_t rec)
{
	char buf[256], *ret;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return NULL;
	}

	if (rec->rec_conn->conn_version >= 26) {
		if (rec->rec_livetv_chain && rec->rec_livetv_chain->chain_current >= 0) {
			snprintf(buf, sizeof(buf), "%s",
				rec->rec_livetv_chain->chain_urls[rec->rec_livetv_chain->chain_current]);
		}
		else
			buf[0] = '\0';
	}
	else
		snprintf(buf, sizeof(buf), "ringbuf%"PRIu32".nuv", rec->rec_id);

	ret = ref_strdup(buf);

	return ret;
}

uint32_t
cmyth_recorder_get_recorder_id(cmyth_recorder_t rec)
{
	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return 0;
	}

	return rec->rec_id;
}

int
cmyth_recorder_set_live_recording(cmyth_recorder_t rec, uint8_t recording)
{
	int err;
	int ret = -1;
	char msg[256];

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return -ENOSYS;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	if (rec->rec_conn->conn_version >= 26)
	{
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %d[]:[]SET_LIVE_RECORDING[]:[]%u", rec->rec_id, recording);

		if ((err=cmyth_send_message(rec->rec_conn, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
				  __FUNCTION__, err);
			goto fail;
		}

		if ((err=cmyth_rcv_okay(rec->rec_conn)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_okay() failed (%d)\n",
				  __FUNCTION__, err);
			goto fail;
		}

		ret = recording;
	}
	else
	{
		ret = -EPERM;
	}

fail:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	return ret;
}
