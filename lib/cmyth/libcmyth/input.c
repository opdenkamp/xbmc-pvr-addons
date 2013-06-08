/*
 *  Copyright (C) 2012, Christian Fetzer
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

static void
cmyth_inputlist_destroy(cmyth_inputlist_t il)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!il) {
		return;
	}
	for (i = 0; i < il->input_count; ++i) {
		if (il->input_list[i]) {
			ref_release(il->input_list[i]);
		}
		il->input_list[i] = NULL;
	}
	if (il->input_list) {
		free(il->input_list);
	}
}

cmyth_inputlist_t
cmyth_inputlist_create(void)
{
	cmyth_inputlist_t ret;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_inputlist_destroy);

	ret->input_list = NULL;
	ret->input_count = 0;
	return ret;
}

void
cmyth_input_destroy(cmyth_input_t i)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!i) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!a\n", __FUNCTION__);
		return;
	}
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);

	if (i->inputname) {
		ref_release(i->inputname);
	}
}

cmyth_input_t
cmyth_input_create(void)
{
	cmyth_input_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_input_destroy);

	ret->inputname = NULL;
	ret->sourceid = 0;
	ret->inputid = 0;
	ret->cardid = 0;
	ret->multiplexid = 0;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

cmyth_inputlist_t
cmyth_get_free_inputlist(cmyth_recorder_t rec)
{
	unsigned int len = CMYTH_INT64_LEN + 36;
	int err;
	int count;
	char *buf;
	int r;

	cmyth_inputlist_t inputlist = cmyth_inputlist_create();

	buf = alloca(len);
	if (!buf) {
		return inputlist;
	}

	sprintf(buf,"QUERY_RECORDER %"PRIu32"[]:[]GET_FREE_INPUTS", rec->rec_id);
	pthread_mutex_lock(&rec->rec_conn->conn_mutex);
	if ((err = cmyth_send_message(rec->rec_conn, buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, err);
		goto out;
	}

	count = cmyth_rcv_length(rec->rec_conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_length() failed (%d)\n",
			__FUNCTION__, count);
		goto out;
	}

	if ((r = cmyth_rcv_free_inputlist(rec->rec_conn, &err, inputlist, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_string() failed (%d)\n",
			__FUNCTION__, r);
		goto out;
	}

	out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);
	return inputlist;
}

int cmyth_rcv_free_inputlist(cmyth_conn_t conn, int *err,
			cmyth_inputlist_t inputlist, int count)
{
	int consumed;
	int total = 0;
	char *failed = NULL;
	cmyth_input_t input = NULL;
	char tmp_str[255];

	if (count <= 0) {
		*err = EINVAL;
		return 0;
	}

	while (count) {
		input = cmyth_input_create();
		consumed = cmyth_rcv_string(conn, err, tmp_str, sizeof(tmp_str), count);
		count -= consumed;
		total += consumed;
		if (*err) {
			failed = "cmyth_rcv_string() inputname";
			goto fail;
		}
		input->inputname = ref_strdup(tmp_str);

		consumed = cmyth_rcv_uint32(conn, err, &input->sourceid, count);
		count -= consumed;
		total += consumed;
		if (*err) {
			failed = "cmyth_rcv_ulong() sourceid";
			goto fail;
		}

		consumed = cmyth_rcv_uint32(conn, err, &input->inputid, count);
		count -= consumed;
		total += consumed;
		if (*err) {
			failed = "cmyth_rcv_ulong() inputid";
			goto fail;
		}

		consumed = cmyth_rcv_uint32(conn, err, &input->cardid, count);
		count -= consumed;
		total += consumed;
		if (*err) {
			failed = "cmyth_rcv_ulong() cardid";
			goto fail;
		}

		consumed = cmyth_rcv_uint32(conn, err, &input->multiplexid , count);
		count -= consumed;
		total += consumed;
		if (*err) {
			failed = "cmyth_rcv_ulong() multiplexid";
			goto fail;
		}

		if (conn->conn_version >= 71) {
			consumed = cmyth_rcv_uint32(conn, err, &input->livetvorder, count);
			count -= consumed;
			total += consumed;
			if (*err) {
				failed = "cmyth_rcv_ulong() livetvorder";
				goto fail;
			}
		}

		inputlist->input_list = realloc(inputlist->input_list,
					(++inputlist->input_count) * sizeof(cmyth_input_t));
		inputlist->input_list[inputlist->input_count - 1] = input;
	}

	return total;

	fail:
	ref_release(input);
	cmyth_dbg(CMYTH_DBG_ERROR, "%s: %s failed (%d)\n",
		__FUNCTION__, failed, *err);
	return total;
}
