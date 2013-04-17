/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <cmyth_local.h>

static void
cmyth_storagegroup_file_destroy(cmyth_storagegroup_file_t fl)
{

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!fl) {
		return;
	}
	if(fl->filename)
		ref_release(fl->filename);
	if(fl->storagegroup)
		ref_release(fl->storagegroup);
	if(fl->hostname)
		ref_release(fl->hostname);
}

cmyth_storagegroup_file_t
cmyth_storagegroup_file_create(void)
{
	cmyth_storagegroup_file_t ret = ref_alloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_storagegroup_file_destroy);

	return ret;
}

static void
cmyth_storagegroup_filelist_destroy(cmyth_storagegroup_filelist_t fl)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!fl) {
		return;
	}
	for (i = 0; i < fl->storagegroup_filelist_count; ++i) {
		if (fl->storagegroup_filelist_list[i]) {
			ref_release(fl->storagegroup_filelist_list[i]);
		}
		fl->storagegroup_filelist_list[i] = NULL;
	}
	if (fl->storagegroup_filelist_list) {
		free(fl->storagegroup_filelist_list);
	}
}

cmyth_storagegroup_filelist_t
cmyth_storagegroup_filelist_create(void)
{
	cmyth_storagegroup_filelist_t ret;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_storagegroup_filelist_destroy);

	ret->storagegroup_filelist_list = NULL;
	ret->storagegroup_filelist_count = 0;
	return ret;
}

static int
cmyth_storagegroup_update_fileinfo(cmyth_conn_t control, cmyth_storagegroup_file_t file)
{
	char msg[256];
	int count;
	int err = 0;
	int consumed; /* = profiles;*/
	char tmp_str[2048];

	if (!control) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n", __FUNCTION__);
		return -1;
	}

	if (!file) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no file specified\n", __FUNCTION__);
		return -1;
	}

	snprintf(msg, sizeof(msg), "QUERY_SG_FILEQUERY[]:[]%s[]:[]%s[]:[]%s", file->hostname , file->storagegroup, file->filename);

	err = cmyth_send_message(control, msg);
	if (err < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_send_message() failed (%d)\n", __FUNCTION__, err);
		return -1;
	}

	count = cmyth_rcv_length(control);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_length() failed (%d)\n", __FUNCTION__, count);
		return -1;
	}

	consumed = cmyth_rcv_string(control, &err, tmp_str, sizeof(tmp_str) - 1, count);
	count -= consumed;

	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_string() failed (%d)\n", __FUNCTION__, count);
		return -1;
	} else if (count == 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: QUERY_SG_FILEQUERY failed(%s)\n", __FUNCTION__, tmp_str);
		return -1;
	}

	consumed = cmyth_rcv_string(control, &err, tmp_str, sizeof(tmp_str), count);
	count -= consumed;
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_string() failed (%d)\n", __FUNCTION__, count);
		return -1;
	}
	file->lastmodified = atol(tmp_str);

	consumed = cmyth_rcv_new_int64(control, &err, &(file->size), count, 1);
	count -= consumed;
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_int64() failed (%d)\n", __FUNCTION__, count);
		return -1;
	}

	return 0;
}

cmyth_storagegroup_filelist_t
cmyth_storagegroup_get_filelist(cmyth_conn_t control,char *storagegroup, char *hostname)
{
	char msg[256];
	int res = 0;
	int count = 0;
	int err = 0;
	int i = 0;
	int listsize = 10;
	cmyth_storagegroup_filelist_t ret = 0;
	cmyth_storagegroup_file_t file = 0;
	int consumed = 0; /* = profiles; */
	char tmp_str[32768];

	if (!control) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n", __FUNCTION__);
		return 0;
	}

	pthread_mutex_lock(&control->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_SG_GETFILELIST[]:[]%s[]:[]%s[]:[][]:[]1", hostname, storagegroup);

	err = cmyth_send_message(control, msg);
	if (err < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_send_message() failed (%d)\n", __FUNCTION__, err);
		goto out;
	}

	count = cmyth_rcv_length(control);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_length() failed (%d)\n", __FUNCTION__, count);
		goto out;
	}

	ret = cmyth_storagegroup_filelist_create();

	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		ref_release(ret);
		ret = 0;
		goto out;
	}

	ret->storagegroup_filelist_count = 0;
	ret->storagegroup_filelist_list = malloc(listsize * sizeof(cmyth_storagegroup_file_t));
	if (!ret->storagegroup_filelist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for filelist list\n", __FUNCTION__);
		ref_release(ret);
		ret = 0;
		goto out;
	}
	while (count) {
		consumed = cmyth_rcv_string(control, &err, tmp_str, sizeof(tmp_str) - 1, count);
		count -= consumed;
		if (err) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_string() failed (%d)\n", __FUNCTION__, count);
			ref_release(ret);
			ret = 0;
			goto out;
		}
		if (res>listsize-1)
		{
			listsize += 10;
			ret->storagegroup_filelist_list = realloc(ret->storagegroup_filelist_list, listsize * sizeof(cmyth_storagegroup_file_t));
			if (!ret->storagegroup_filelist_list) {
				cmyth_dbg(CMYTH_DBG_ERROR, "%s: realloc() failed for filelist list\n", __FUNCTION__);
				ref_release(ret);
				ret = 0;
				goto out;
			}
		}
		file = cmyth_storagegroup_file_create();
		file->filename = ref_strdup(tmp_str);
		file->storagegroup = ref_strdup(storagegroup);
		file->hostname = ref_strdup(hostname);
		file->size = 0;
		file->lastmodified = 0;
		ret->storagegroup_filelist_list[res] = file;
		res++;
	}
	ret->storagegroup_filelist_count = res;

	for(i = 0;i < ret->storagegroup_filelist_count;i++) {
		cmyth_storagegroup_update_fileinfo(control, ret->storagegroup_filelist_list[i]);
	}

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: results= %d\n", __FUNCTION__, res);

    out:
	pthread_mutex_unlock(&control->conn_mutex);
	return ret;
}

int
cmyth_storagegroup_filelist_count(cmyth_storagegroup_filelist_t fl)
{
	if (!fl) {
		return -EINVAL;
	}
	return fl->storagegroup_filelist_count;
}

cmyth_storagegroup_file_t
cmyth_storagegroup_filelist_get_item(cmyth_storagegroup_filelist_t fl, int index)
{
	if (!fl || index >= fl->storagegroup_filelist_count) {
		return NULL;
	}
	ref_hold(fl->storagegroup_filelist_list[index]);
	return fl->storagegroup_filelist_list[index];
 }

cmyth_storagegroup_file_t
cmyth_storagegroup_get_fileinfo(cmyth_conn_t control, char *storagegroup, char *hostname, char *filename)
{
	char msg[256];
	int count = 0;
	int err = 0;
	cmyth_storagegroup_file_t ret = NULL;
	int consumed = 0;
	char tmp_str[2048];

	if (!control) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n", __FUNCTION__);
		return 0;
	}

	pthread_mutex_lock(&control->conn_mutex);

	snprintf(msg, sizeof(msg), "QUERY_SG_FILEQUERY[]:[]%s[]:[]%s[]:[]%s", hostname, storagegroup, filename);

	err = cmyth_send_message(control, msg);
	if (err < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_send_message() failed (%d)\n", __FUNCTION__, err);
		goto out;
	}

	count = cmyth_rcv_length(control);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_length() failed (%d)\n", __FUNCTION__, count);
		goto out;
	}

	consumed = cmyth_rcv_string(control, &err, tmp_str, sizeof(tmp_str) - 1, count);
	count -= consumed;
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_string() failed (%d)\n", __FUNCTION__, count);
		ret = NULL;
		goto out;
	} else if (count == 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: QUERY_SG_FILEQUERY failed(%s)\n", __FUNCTION__, tmp_str);
		ret = NULL;
		goto out;
	}

	ret = cmyth_storagegroup_file_create();
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for file\n", __FUNCTION__);
		ref_release(ret);
		ret = NULL;
		goto out;
	}
	ret->filename = ref_strdup(tmp_str);

	consumed = cmyth_rcv_string(control, &err, tmp_str, sizeof(tmp_str) - 1, count);
	count -= consumed;
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_string() failed (%d)\n", __FUNCTION__, count);
		ref_release(ret);
		ret = NULL;
		goto out;
	}
        ret->lastmodified = atol(tmp_str);

	consumed = cmyth_rcv_new_int64(control, &err, &(ret->size), count, 1);
	count -= consumed;
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_int64() failed (%d)\n", __FUNCTION__, count);
		ref_release(ret);
		ret = NULL;
		goto out;
	}

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: filename: %s\n", __FUNCTION__, ret->filename);

out:
	pthread_mutex_unlock(&control->conn_mutex);
	return ret;
}

char *
cmyth_storagegroup_file_filename(cmyth_storagegroup_file_t file)
{
	if (!file) {
		return NULL;
	}
	return ref_hold(file->filename);
}

time_t
cmyth_storagegroup_file_lastmodified(cmyth_storagegroup_file_t file)
{
	if (!file) {
		return 0;
	}
	return file->lastmodified;
}

int64_t
cmyth_storagegroup_file_size(cmyth_storagegroup_file_t file)
{
	if (!file) {
		return 0;
	}
	return file->size;
}