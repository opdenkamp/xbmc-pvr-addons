/*
 *  Copyright (C) 2005-2009, Jon Gettler
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
#include <inttypes.h>
#include <errno.h>
#include <cmyth_local.h>


int64_t cmyth_get_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog)
{
	char *buf;
	unsigned int len = CMYTH_TIMESTAMP_LEN + CMYTH_INT64_LEN + 18;
	int err;
	int64_t ret;
	int count;
	int r;
	char start_ts_dt[CMYTH_TIMESTAMP_LEN + 1];
	cmyth_datetime_to_string(start_ts_dt, prog->proginfo_rec_start_ts);
	buf = alloca(len);
	if (!buf) {
		return -ENOMEM;
	}
	sprintf(buf,"%s %"PRIu32" %s","QUERY_BOOKMARK",prog->proginfo_chanId,
		start_ts_dt);
	pthread_mutex_lock(&conn->conn_mutex);;
	if ((err = cmyth_send_message(conn,buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, err);
		ret = err;
		goto out;
	}
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_length() failed (%d)\n",
			__FUNCTION__, count);
		ret = count;
		goto out;
	}
	if ((r=cmyth_rcv_int64(conn, &err, &ret, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_int64() failed (%d)\n",
			__FUNCTION__, r);
		ret = err;
		goto out;
	}

   out:
	pthread_mutex_unlock(&conn->conn_mutex);
	return ret;
}

int cmyth_set_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog, int64_t bookmark)
{
	char *buf;
	unsigned int len = CMYTH_TIMESTAMP_LEN + CMYTH_INT64_LEN * 2 + 18 + 2;
	int ret;
	char start_ts_dt[CMYTH_TIMESTAMP_LEN + 1];
	cmyth_datetime_to_string(start_ts_dt, prog->proginfo_rec_start_ts);
	buf = alloca(len);
	if (!buf) {
		return -ENOMEM;
	}
	if (conn->conn_version >= 66) {
		/*
		 * Since protocol 66 mythbackend expects a single 64 bit integer rather than two 32 bit
		 * hi and lo integers. Nevertheless the backend (at least up to 0.25) checks
		 * the existence of the 4th parameter, so adding a 0.
		 */
		sprintf(buf, "SET_BOOKMARK %"PRIu32" %s %"PRId64" 0", prog->proginfo_chanId,
				start_ts_dt, bookmark);
	}
	else {
		sprintf(buf, "SET_BOOKMARK %"PRIu32" %s %"PRId32" %"PRId32, prog->proginfo_chanId,
				start_ts_dt, (int32_t)(bookmark >> 32), (int32_t)(bookmark & 0xffffffff));
	}
	pthread_mutex_lock(&conn->conn_mutex);;
	if ((ret = cmyth_send_message(conn,buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, ret);
		goto out;
	}
	if ((ret = cmyth_rcv_okay(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
	}
   out:
	pthread_mutex_unlock(&conn->conn_mutex);
	return ret;
}
