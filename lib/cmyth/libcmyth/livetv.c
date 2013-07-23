/*
 *  Copyright (C) 2006-2012, Sergio Slobodrian
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
 * livetv.c -     functions to handle operations on MythTV livetv chains.  A
 *                MythTV livetv chain is the part of the backend that handles
 *                recording of live-tv for streaming to a MythTV frontend.
 *                This allows the watcher to do things like pause, rewind
 *                and so forth on live-tv.
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <cmyth_local.h>

static int cmyth_livetv_chain_has_url(cmyth_recorder_t rec, char * url);
static int cmyth_livetv_chain_add_file(cmyth_recorder_t rec,
                                       char * url, cmyth_file_t fp);
static int cmyth_livetv_chain_add_url(cmyth_recorder_t rec, char * url);
static int cmyth_livetv_chain_add(cmyth_recorder_t rec, char * url,
                                  cmyth_file_t fp, cmyth_proginfo_t prog);


/*
 * cmyth_livetv_chain_destroy()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Clean up and free a livetv chain structure.  This should only be done
 * by the ref_release() code.  Everyone else should call
 * ref_release() because ring buffer structures are reference
 * counted.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_livetv_chain_destroy(cmyth_livetv_chain_t ltc)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ltc) {
		return;
	}

	if (ltc->chainid) {
		ref_release(ltc->chainid);
	}
	if (ltc->chain_urls) {
		for(i=0;i<ltc->chain_ct; i++)
			if (ltc->chain_urls[i])
				ref_release(ltc->chain_urls[i]);
		ref_release(ltc->chain_urls);
	}
	if (ltc->chain_files) {
		for(i=0;i<ltc->chain_ct; i++)
			if (ltc->chain_files[i])
				ref_release(ltc->chain_files[i]);
		ref_release(ltc->chain_files);
	}
	if (ltc->progs) {
		for(i=0;i<ltc->chain_ct; i++)
			if (ltc->progs[i])
				ref_release(ltc->progs[i]);
		ref_release(ltc->progs);
	}
}

/*
 * cmyth_livetv_chain_create()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Allocate and initialize a ring buffer structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_livetv_chain_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_livetv_chain_t
 */
cmyth_livetv_chain_t
cmyth_livetv_chain_create(char * chainid)
{
	cmyth_livetv_chain_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ret) {
		return NULL;
	}

	ret->chainid = ref_strdup(chainid);
	ret->chain_ct = 0;
	ret->chain_switch_on_create = 0;
	ret->chain_current = -1;
	ret->chain_urls = NULL;
	ret->chain_files = NULL;
	ret->progs = NULL;
	ret->livetv_watch = 0; /* JLB: Manage program breaks */
	ret->livetv_buflen = 0;
	ret->livetv_tcp_rcvbuf = 0;
	ret->livetv_block_len = 0;
	ref_set_destroy(ret, (ref_destroy_t)cmyth_livetv_chain_destroy);
	return ret;
}

/*
	Returns the index of the chain entry with the URL or 0 if the
	URL isn't there.
*/
/*
 * cmyth_livetv_chain_has_url()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Returns the index of the chain entry with the URL or -1 if the
 * URL isn't there.
 *
 * Return Value:
 *
 * Success: The index of the entry in the chain that has the
 * 					specified URL.
 *
 * Failure: -1 if the URL doesn't appear in the chain.
 */
int cmyth_livetv_chain_has_url(cmyth_recorder_t rec, char * url)
{
	int found, i;
	found = 0;
	if(rec->rec_livetv_chain) {
		if(rec->rec_livetv_chain->chain_ct > 0) {
			for(i=0;i<rec->rec_livetv_chain->chain_ct; i++) {
				if(strcmp(rec->rec_livetv_chain->chain_urls[i],url) == 0) {
					found = 1;
					break;
				}
			}
		}
	}
	return found?i:-1;
}

#if 0
static int cmyth_livetv_chain_current(cmyth_recorder_t rec);
int
cmyth_livetv_chain_current(cmyth_recorder_t rec)
{
	return rec->rec_livetv_chain->chain_current;
}

static cmyth_file_t cmyth_livetv_get_cur_file(cmyth_recorder_t rec);
cmyth_file_t
cmyth_livetv_get_cur_file(cmyth_recorder_t rec)
{
	return rec->rec_livetv_file;
}
#endif

/*
 * cmyth_livetv_chain_add_file()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Called to add a file handle to a livetv chain structure. The handle is added
 * only if the url is already there.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Faiure: -1
 */
static int
cmyth_livetv_chain_add_file(cmyth_recorder_t rec, char * url, cmyth_file_t ft)
{

	int cur;
	int ret = 0;
	cmyth_file_t tmp;

	if(rec->rec_livetv_chain) {
		if(rec->rec_livetv_chain->chain_ct > 0) {
			/* Is this file already in the chain? */
			if((cur = cmyth_livetv_chain_has_url(rec, url)) != -1) {
				/* Release the existing handle after holding the new */
				/* this allows them to be the same. */
				tmp = rec->rec_livetv_chain->chain_files[cur];
				rec->rec_livetv_chain->chain_files[cur] = ref_hold(ft);
				ref_release(tmp);
			}
		}
		else {
			cmyth_dbg(CMYTH_DBG_ERROR,
			 		"%s: attempted to add file for %s to an empty chain\n",
			 		__FUNCTION__, url);
			ret = -1;
		}
	}
	else {
		cmyth_dbg(CMYTH_DBG_ERROR,
		 		"%s: attempted to add file for %s to an non existant chain\n",
		 		__FUNCTION__, url);
		ret = -1;
	}
	return ret;
}

/*
 * cmyth_livetv_chain_add_prog()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Called to add program info to a livetv chain structure. The info is added
 * only if the url is already there.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Faiure: -1
 */
static int
cmyth_livetv_chain_add_prog(cmyth_recorder_t rec, char * url,
														cmyth_proginfo_t prog)
{

	int cur;
	int ret = 0;
	cmyth_proginfo_t tmp;

	if(rec->rec_livetv_chain) {
		if(rec->rec_livetv_chain->chain_ct > 0) {
			/* Is this file already in the chain? */
			if((cur = cmyth_livetv_chain_has_url(rec, url)) != -1) {
				/* Release the existing handle after holding the new */
				/* this allows them to be the same. */
				tmp = rec->rec_livetv_chain->progs[cur];
				rec->rec_livetv_chain->progs[cur] = ref_hold(prog);
				ref_release(tmp);
			}
		}
		else {
			cmyth_dbg(CMYTH_DBG_ERROR,
			 		"%s: attempted to add prog for %s to an empty chain\n",
			 		__FUNCTION__, url);
			ret = -1;
		}
	}
	else {
		cmyth_dbg(CMYTH_DBG_ERROR,
		 		"%s: attempted to add prog for %s to an non existant chain\n",
		 		__FUNCTION__, url);
		ret = -1;
	}
	return ret;
}

/*
 * cmyth_livetv_chain_add_url()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Called to add a url to a livetv chain structure. The url is added
 * only if it is not already there.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Faiure: -1
 */
static int
cmyth_livetv_chain_add_url(cmyth_recorder_t rec, char * url)
{
	char ** tmp;
	cmyth_file_t * fp;
	cmyth_proginfo_t * pi;
	int ret = 0;

	if(cmyth_livetv_chain_has_url(rec,url) == -1) {
		if(rec->rec_livetv_chain->chain_ct == 0) {
			rec->rec_livetv_chain->chain_ct = 1;
			/* Nothing in the chain yet, allocate the space */
			tmp = (char**)ref_alloc(sizeof(char *));
			fp = (cmyth_file_t *)ref_alloc(sizeof(cmyth_file_t));
			pi = (cmyth_proginfo_t *)ref_alloc(sizeof(cmyth_proginfo_t));
		}
		else {
			rec->rec_livetv_chain->chain_ct++;
			tmp = (char**)ref_realloc(rec->rec_livetv_chain->chain_urls,
								sizeof(char *)*rec->rec_livetv_chain->chain_ct);
			fp = (cmyth_file_t *)
					ref_realloc(rec->rec_livetv_chain->chain_files,
								sizeof(cmyth_file_t)*rec->rec_livetv_chain->chain_ct);
			pi = (cmyth_proginfo_t *)
					ref_realloc(rec->rec_livetv_chain->progs,
								sizeof(cmyth_proginfo_t)*rec->rec_livetv_chain->chain_ct);
		}
		if(tmp != NULL && fp != NULL) {
			rec->rec_livetv_chain->chain_urls = ref_hold(tmp);
			rec->rec_livetv_chain->chain_files = ref_hold(fp);
			rec->rec_livetv_chain->progs = ref_hold(pi);
			ref_release(tmp);
			ref_release(fp);
			ref_release(pi);
			rec->rec_livetv_chain->chain_urls[rec->rec_livetv_chain->chain_ct-1]
							= ref_strdup(url);
			rec->rec_livetv_chain->chain_files[rec->rec_livetv_chain->chain_ct-1]
							= ref_hold(NULL);
			rec->rec_livetv_chain->progs[rec->rec_livetv_chain->chain_ct-1]
							= ref_hold(NULL);
		}
		else {
			if (tmp)
				ref_release(tmp);

			if (fp)
				ref_release(fp);

			if (pi)
				ref_release(pi);

			ret = -1;
			cmyth_dbg(CMYTH_DBG_ERROR,
			 		"%s: memory allocation request failed\n",
			 		__FUNCTION__);
		}

	}
	return ret;
}

/*
 * cmyth_livetv_chain_add()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Called to add a url and file pointer to a livetv chain structure.
 * The url is added only if it is not already there. The file pointer
 * will be held (ref count increased) by this call if successful.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Faiure: -1
 */
int
cmyth_livetv_chain_add(cmyth_recorder_t rec, char * url, cmyth_file_t ft,
											 cmyth_proginfo_t pi)
{
	int ret = 0;

	if(cmyth_livetv_chain_has_url(rec, url) == -1)
		ret = cmyth_livetv_chain_add_url(rec, url);
	if(ret != -1)
		ret = cmyth_livetv_chain_add_file(rec, url, ft);
	if(ret != -1)
		ret = cmyth_livetv_chain_add_prog(rec, url, pi);

	return ret;
}

/*
 * cmyth_livetv_chain_update()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Called in response to the backend's notification of a chain update.
 * The recorder is supplied and will be queried for the current recording
 * to determine if a new file needs to be added to the chain of files
 * in the live tv instance.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -1
 */
int
cmyth_livetv_chain_update(cmyth_recorder_t rec, char * chainid)
{
	int ret;
	char url[1024];
	cmyth_proginfo_t loc_prog = NULL;
	cmyth_file_t ft = NULL;

	ret = 0;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}

	if (!rec->rec_livetv_chain) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: rec_livetv_chain is NULL\n",
			  __FUNCTION__, url);
		return -1;
	}

	if (strncmp(rec->rec_livetv_chain->chainid, chainid, strlen(chainid)) == 0) {
		loc_prog = cmyth_recorder_get_cur_proginfo(rec);
		if (!loc_prog) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: recorder is not recording\n",
				  __FUNCTION__);
			ret = -1;
			goto out;
		}
		sprintf(url, "myth://%s:%d%s", loc_prog->proginfo_hostname, rec->rec_port,
				loc_prog->proginfo_pathname);

		/*
			  Now check if this file is in the recorder chain and if not
			  then open a new file transfer and add it to the chain.
		*/

		if (cmyth_livetv_chain_has_url(rec, url) == -1) {
			ft = cmyth_conn_connect_file(loc_prog, rec->rec_conn, rec->rec_livetv_chain->livetv_buflen, rec->rec_livetv_chain->livetv_tcp_rcvbuf);
			if (!ft) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_conn_connect_file(%s) failed\n",
					  __FUNCTION__, url);
				ret = -1;
				goto out;
			}
			/*
			 * file in the chain could be dummy and then
			 * backend close file transfer socket immediately.
			 * In other cases add the chain else wait next chain update to
			 * add a valid program.
			 */
			if (cmyth_file_is_open(ft) > 0) {
				if(cmyth_livetv_chain_add(rec, url, ft, loc_prog) == -1) {
					cmyth_dbg(CMYTH_DBG_ERROR,
						  "%s: cmyth_livetv_chain_add(%s) failed\n",
						  __FUNCTION__, url);
					ret = -1;
					goto out;
				}
				/*
				 * New program is inserted. Switch OFF watch signal and
				 * switch to the new file when required.
				 */
				rec->rec_livetv_chain->livetv_watch = 0;
				if (rec->rec_livetv_chain->chain_switch_on_create
					&& cmyth_livetv_chain_switch_last(rec) == 1) {
					rec->rec_livetv_chain->chain_switch_on_create = 0;
				}
				cmyth_dbg(CMYTH_DBG_INFO,
					  "%s: chain count is %d\n",
					  __FUNCTION__, rec->rec_livetv_chain->chain_ct);
			}
			else {
				ret = -1;
			}
		}
	}
	else {
		cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: chainid doesn't match recorder's chainid!!\n",
				  __FUNCTION__, url);
		ret = -1;
	}

out:
	ref_release(ft);
	ref_release(loc_prog);

	return ret;
}

/* JLB: Manage program breaks
 * cmyth_livetv_watch()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Called in response to the backend's notification of a livetv watch.
 * The recorder is supplied and will be updated for the watch signal.
 * This event is used to manage program breaks while watching live tv.
 * When the guide data marks the end of one show and the beginning of
 * the next, which will be recorded to a new file, this instructs the
 * frontend to terminate the existing playback, and change channel to
 * the new file. Before updating livetv chain and switching to new file
 * we must to wait for event DONE_RECORDING that informs the current
 * show is completed. Then we will call livetv chain update to get
 * current program info. Watch signal will be down during this period.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -1
 */
int
cmyth_livetv_watch(cmyth_recorder_t rec, char * msg)
{
	uint32_t rec_id;
	int flag;
	int ret;

	ret = 0;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}
	/*
	 * Parse msg. Should be %ld %d
	 */
	if (strlen(msg) >= 3 && sscanf(msg,"%"PRIu32" %d", &rec_id, &flag) == 2) {
		if (rec_id == rec->rec_id) {
			rec->rec_livetv_chain->livetv_watch = 1;
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: wait event for recorder: %"PRIu32"\n",
				  __FUNCTION__, rec_id);
		}
		else {
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: nothing to trigger for recorder: %"PRIu32"\n",
				  __FUNCTION__, rec_id);
		}
	}
	else {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: bad message: %s",
			  __FUNCTION__, msg);
		ret = -1;
	}

	return ret;
}

/* JLB: Manage program breaks
 * cmyth_livetv_done_recording()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Indicates that an active recording has completed on the specified
 * recorder. used to manage program breaks while watching live tv.
 * When receive event for recorder, we force an update of livetv chain
 * to get current program info when chain is not yet updated.
 * Watch signal is used when up, to mark the break period and
 * queuing the frontend for reading file buffer.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -1
 */
int
cmyth_livetv_done_recording(cmyth_recorder_t rec, char * msg)
{
	uint32_t rec_id;
	int ret;

	ret = 0;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}
	/*
	 * Parse msg. Should be 'recordid ...'
	 */
	if (strlen(msg) >= 1 && sscanf(msg,"%"PRIu32, &rec_id) == 1) {
		if (rec_id == rec->rec_id
			&& rec->rec_livetv_chain->livetv_watch == 1
			&& cmyth_recorder_is_recording(rec) == 1) {
			/*
			 * Last recording is now completed but watch signal is ON.
			 * Then force live tv chain update for the new current
			 * program. We will retry 3 times before returning.
			 */
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: previous recording done. Start chain update\n",
				  __FUNCTION__);
			ret = -1;
			if (rec->rec_livetv_chain->chainid && rec->rec_livetv_chain->chain_ct > 0) {
				int i;
				for (i = 0; i < 3; i++) {
					if (cmyth_livetv_chain_update(rec, rec->rec_livetv_chain->chainid) < 0)
						break;
					if (rec->rec_livetv_chain->livetv_watch == 0) {
						ret = 0;
						break;
					}
					usleep(100000);
				}
			}
			else {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: chainid is null for recorder %"PRIu32"\n",
					  __FUNCTION__, rec_id);
			}
		}
		else {
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: nothing to trigger for recorder: %"PRIu32"\n",
				  __FUNCTION__, rec_id);
		}
	}
	else {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: unknown message: %s",
			  __FUNCTION__, msg);
		ret = -1;
	}

	return ret;
}

/*
 * cmyth_livetv_chain_setup()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Set up the file information the recorder needs to watch live
 * tv.  The recorder is supplied.  This will be duplicated and
 * released, so the caller can re-use the same variable to hold the
 * return.  The new copy of the recorder will have a livetv chain
 * within it.
 *
 * Return Value:
 *
 * Success: A pointer to a new recorder structure with a livetvchain
 *
 * Failure: NULL but the recorder passed in is not released the
 *					caller needs to do this on a failure.
 */
cmyth_recorder_t
cmyth_livetv_chain_setup(cmyth_recorder_t rec, uint32_t buflen, int32_t tcp_rcvbuf,
			 void (*prog_update_callback)(cmyth_proginfo_t))
{

	cmyth_recorder_t new_rec = NULL;
	char url[1024];
	cmyth_proginfo_t loc_prog;
	cmyth_file_t ft = NULL;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return NULL;
	}

	/* Get the current recording information */
	loc_prog = cmyth_recorder_get_cur_proginfo(rec);

	if (loc_prog == NULL) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: could not get current filename\n",
			  __FUNCTION__);
		goto out;
	}

	new_rec = cmyth_recorder_dup(rec);
	if (new_rec == NULL) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: cannot create recorder\n",
			  __FUNCTION__);
		goto out;
	}
	ref_release(rec);

	if(new_rec->rec_livetv_chain == NULL) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: error no livetv_chain\n",
			  __FUNCTION__);
		new_rec = NULL;
		goto out;
	}

	/* JLB: Set tcp receive buffer for the chain files */
	new_rec->rec_livetv_chain->livetv_buflen = buflen;
	new_rec->rec_livetv_chain->livetv_tcp_rcvbuf = tcp_rcvbuf;
	/* JLB: Manage program breaks. Switch OFF watch signal */
	new_rec->rec_livetv_chain->livetv_watch = 0;

	new_rec->rec_livetv_chain->prog_update_callback = prog_update_callback;

	sprintf(url, "myth://%s:%d%s", loc_prog->proginfo_hostname, new_rec->rec_port,
				loc_prog->proginfo_pathname);

	if(cmyth_livetv_chain_has_url(new_rec, url) == -1) {
		ft = cmyth_conn_connect_file(loc_prog, new_rec->rec_conn, new_rec->rec_livetv_chain->livetv_buflen, new_rec->rec_livetv_chain->livetv_tcp_rcvbuf);
		if (!ft) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_conn_connect_file(%s) failed\n",
				  __FUNCTION__, url);
			new_rec = NULL;
			goto out;
		}
		/*
		 * Since 0.25: First file in the chain could be dummy and then
		 * backend close file transfer socket immediately.
		 * In other cases add the chain else wait next chain update to
		 * add a valid program.
		 */
		if (cmyth_file_is_open(ft) > 0) {
			if(cmyth_livetv_chain_add(new_rec, url, ft, loc_prog) == -1) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_livetv_chain_add(%s) failed\n",
					  __FUNCTION__, url);
				new_rec = NULL;
			}
			else {
				/* now switch to the valid program */
				cmyth_livetv_chain_switch_last(new_rec);
				new_rec->rec_livetv_chain->chain_switch_on_create = 0;
			}
		}
		else {
			/*
			 * we must wait the chain update from backend. Then we will switching
			 * to the new program immediately.
			 */
			new_rec->rec_livetv_chain->chain_switch_on_create = 1;
		}
	}

    out:
	ref_release(ft);
	ref_release(loc_prog);

	return new_rec;
}

/*
 * cmyth_livetv_chain_get_block()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Read incoming file data off the network into a buffer of length len.
 *
 * Return Value:
 *
 * Sucess: number of bytes read into buf
 *
 * Failure: an int containing -errno
 */
int32_t
cmyth_livetv_chain_get_block(cmyth_recorder_t rec, char *buf, int32_t len)
{
	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

  return cmyth_file_get_block(rec->rec_livetv_file, buf, len);
}

static int
cmyth_livetv_chain_select(cmyth_recorder_t rec, struct timeval *timeout)
{
	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

  return cmyth_file_select(rec->rec_livetv_file, timeout);
}


/*
 * cmyth_livetv_chain_switch()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Switches to the next or previous chain depending on the
 * value of dir. Dir is usually 1 or -1.
 *
 * Return Value:
 *
 * Sucess: 1
 *
 * Failure: 0
 */
int
cmyth_livetv_chain_switch(cmyth_recorder_t rec, int dir)
{
	int ret, i;
	cmyth_file_t oldfile = NULL;

	if (dir == 0)
		return 1;

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	ret = 0;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: switch file: current=%d , dir=%d\n",
		  __FUNCTION__, rec->rec_livetv_chain->chain_current, dir);

	if (dir > 0 && rec->rec_livetv_chain->chain_current == rec->rec_livetv_chain->chain_ct - dir) {
		 /* No more file in the chain */
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: no more file\n",
			  __FUNCTION__);
		/* JLB: Manage program breaks
		 * If livetv_watch is ON then we are waiting next chain update and adding a new file.
		 * Timeout is 2 secondes before release.
		 */
		if (rec->rec_livetv_chain->livetv_watch != 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: wait until livetv_watch is OFF\n",
				  __FUNCTION__);
			for (i = 0; i < 4; i++) {
				pthread_mutex_unlock(&rec->rec_conn->conn_mutex);
				usleep(500000);
				pthread_mutex_lock(&rec->rec_conn->conn_mutex);
				if (rec->rec_livetv_chain->livetv_watch == 0)
					break;
			}
			if (rec->rec_livetv_chain->livetv_watch != 0) {
				/* The chain is not updated yet
				 * Return to retry later
				 */
				ret = 0;
				goto out;
			}
		}
	}

	if((dir < 0 && rec->rec_livetv_chain->chain_current + dir >= 0)
		|| (rec->rec_livetv_chain->chain_current <
			  rec->rec_livetv_chain->chain_ct - dir)) {
		oldfile = rec->rec_livetv_file;
		ret = rec->rec_livetv_chain->chain_current += dir;
		rec->rec_livetv_file = ref_hold(rec->rec_livetv_chain->chain_files[ret]);
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: file switch to %d\n",__FUNCTION__,ret);
		if (rec->rec_livetv_chain->prog_update_callback) {
			rec->rec_livetv_chain
					->prog_update_callback(rec->rec_livetv_chain->progs[ret]);
		}
		ret = 1;
	}

	out:

	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);
	if (oldfile)
		ref_release(oldfile);
	return ret;
}

int
cmyth_livetv_chain_switch_last(cmyth_recorder_t rec)
{
	int dir;

	if(rec->rec_conn->conn_version < 26)
		return 1;

	dir = rec->rec_livetv_chain->chain_ct
			- rec->rec_livetv_chain->chain_current - 1;

	return cmyth_livetv_chain_switch(rec, dir);
}

/*
 * cmyth_livetv_chain_request_block()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a file data block of a certain size, and return when the
 * block has been transfered.
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
static int32_t
cmyth_livetv_chain_request_block(cmyth_recorder_t rec, int32_t len)
{
	int ret, retry;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) {\n", __FUNCTION__,
				__FILE__, __LINE__);

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	do {
		retry = 0;
		ret = cmyth_file_request_block(rec->rec_livetv_file, len);
		if (ret == 0) { /* We've gotten to the end, need to progress in the chain */
			/* Switch if there are files left in the chain */
			retry = cmyth_livetv_chain_switch(rec, 1);
		}
	}
	while (retry);

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) }\n",
				__FUNCTION__, __FILE__, __LINE__);

	return ret;
}

int32_t cmyth_livetv_chain_read(cmyth_recorder_t rec, char *buf, int32_t len)
{
	int ret, retry;
	int32_t vlen, rlen, nlen;

	if (rec == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	/*
	 * Set the requested block size to the recommended value. It was
	 * estimated the previous time.
	 * = 0 : No limit. Use the input value (len)
	 * > 0 : limit is capped at this value
	 */
	vlen = rec->rec_livetv_chain->livetv_block_len;

	do {
		if (vlen == 0 || vlen > len) {
			rlen = len;
		}
		else {
			rlen = vlen;
		}

		retry = 0;
		ret = cmyth_file_read(rec->rec_livetv_file, buf, rlen);
		if (ret < 0) {
			break;
		}
		nlen = ret;
		if (nlen == 0) {
			/* eof, switch to next file */
			retry = cmyth_livetv_chain_switch(rec, 1);
			if (retry == 1) {
				/* Already requested ? seek to 0 */
				cmyth_file_seek(rec->rec_livetv_file, 0, WHENCE_SET);
				/* Chain switch done. Retry without limit */
				vlen = 0;
			}
			else if (rec->rec_livetv_chain->livetv_watch == 0) {
				/*
				 * Current buffer is empty on the last file.
				 * We are waiting some time refilling buffer
				 * Timeout is 0.5 seconde before release.
				 */
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: wait some 250ms before request block\n",
					  __FUNCTION__);
				usleep(250000);
				vlen = 4096;
			}
		}
		else if (nlen < rlen) {
			/* Returned size is less than requested size: decrease size to ret */
			vlen = nlen;
		}
		else if (vlen < len) {
			/* else increase size until no limit */
			vlen = nlen + 4096;
		}
		else {
			vlen = 0;
		}

	} while(retry);

	/* Store the recommended value of block size for next time */
	rec->rec_livetv_chain->livetv_block_len = vlen;

	return ret;
}

/*
 * cmyth_livetv_chain_duration()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Get current chain duration
 *
 * Return Value:
 *
 * Sucess: chain duration
 *
 * Failure: an int containing -errno
 */

int64_t
cmyth_livetv_chain_duration(cmyth_recorder_t rec)
{
  int cur, ct;
  int64_t ret=0;
  ct  = rec->rec_livetv_chain->chain_ct;
  for (cur = 0; cur < ct; cur++) {
			ret += rec->rec_livetv_chain->chain_files[cur]->file_length;
		}
  return ret;
}

/*
 * cmyth_livetv_chain_seek()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Seek to a new position in the file based on the value of whence:
 *	WHENCE_SET
 *		The offset is set to offset bytes.
 *	WHENCE_CUR
 *		The offset is set to the current position plus offset bytes.
 *	WHENCE_END
 *		The offset is set to the size of the file minus offset bytes.
 *
 * Return Value:
 *
 * Sucess: 0
 *
 * Failure: an int containing -errno
 */
static int64_t
cmyth_livetv_chain_seek(cmyth_recorder_t rec, int64_t offset, int8_t whence)
{
	int64_t ret;
	cmyth_file_t fp;
	int cur, ct;

	if (rec == NULL)
		return -EINVAL;

	fp = NULL;
	cur = -1;
	ct  = rec->rec_livetv_chain->chain_ct;

	if (whence == WHENCE_END) {
		offset = - rec->rec_livetv_file->file_req - offset;
		for (cur = rec->rec_livetv_chain->chain_current; cur < ct; cur++) {
			offset += rec->rec_livetv_chain->chain_files[cur]->file_length;
		}

		cur = rec->rec_livetv_chain->chain_current;
		fp  = rec->rec_livetv_chain->chain_files[cur];
		whence = WHENCE_CUR;
	}

	if (whence == WHENCE_SET) {
		for (cur = 0; cur < ct; cur++) {
    			fp = rec->rec_livetv_chain->chain_files[cur];
			if (offset < fp->file_length)
				break;
			offset -= fp->file_length;
		}
	}

	if (whence == WHENCE_CUR) {

		if (offset == 0) {
			cur     = rec->rec_livetv_chain->chain_current;
			offset += rec->rec_livetv_chain->chain_files[cur]->file_req;
			for (; cur > 0; cur--) {
				offset += rec->rec_livetv_chain->chain_files[cur-1]->file_length;
			}
			return offset;
		} else {
			cur = rec->rec_livetv_chain->chain_current;
			fp  = rec->rec_livetv_chain->chain_files[cur];
		}

		offset += fp->file_req;

		while (offset > fp->file_length) {
			cur++;
			offset -= fp->file_length;
			if(cur == ct)
				return -1;
			fp = rec->rec_livetv_chain->chain_files[cur];
		}

		while (offset < 0) {
			cur--;
			if(cur < 0)
				return -1;
			fp = rec->rec_livetv_chain->chain_files[cur];
			offset += fp->file_length;
		}

		whence = WHENCE_SET;
	}

	if (cur >=0 && cur < rec->rec_livetv_chain->chain_ct && fp)
	{
		if ((ret = cmyth_file_seek(fp, offset, whence)) >= 0) {
			cur -= rec->rec_livetv_chain->chain_current;
			if (cmyth_livetv_chain_switch(rec, cur) == 1) {
				/*
				 * Return the new position in the chain
				 */
				for (cur = 0; cur < rec->rec_livetv_chain->chain_current; cur++) {
					fp = rec->rec_livetv_chain->chain_files[cur];
					ret += fp->file_length;
				}
				return ret;
			}
		}
	}

	return -1;
}

/*
 * cmyth_livetv_read()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request and read a block of data from backend
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
int32_t cmyth_livetv_read(cmyth_recorder_t rec, char *buf, int32_t len)
{
	if(rec->rec_conn->conn_version >= 26)
		return cmyth_livetv_chain_read(rec, buf, len);
	else
		return cmyth_ringbuf_read(rec, buf, len);

}

/*
 * cmyth_livetv_seek()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Seek to a new position in the file based on the value of whence:
 *	WHENCE_SET
 *		The offset is set to offset bytes.
 *	WHENCE_CUR
 *		The offset is set to the current position plus offset bytes.
 *	WHENCE_END
 *		The offset is set to the size of the file minus offset bytes.
 * This function will select the appropriate call based on the protocol.
 *
 * Return Value:
 *
 * Sucess: 0
 *
 * Failure: an int containing -errno
 */
int64_t
cmyth_livetv_seek(cmyth_recorder_t rec, int64_t offset, int8_t whence)
{
	int64_t rtrn;

	if(rec->rec_conn->conn_version >= 26)
		rtrn = cmyth_livetv_chain_seek(rec, offset, whence);
	else
		rtrn = cmyth_ringbuf_seek(rec, offset, whence);

	return rtrn;
}

/*
 * cmyth_livetv_request_block()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a file data block of a certain size, and return when the
 * block has been transfered. This function will select the appropriate
 * call to use based on the protocol.
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
int32_t
cmyth_livetv_request_block(cmyth_recorder_t rec, int32_t len)
{
	int32_t rtrn;

	if(rec->rec_conn->conn_version >= 26)
		rtrn = cmyth_livetv_chain_request_block(rec, len);
	else
		rtrn = cmyth_ringbuf_request_block(rec, len);

	return rtrn;
}

int
cmyth_livetv_select(cmyth_recorder_t rec, struct timeval *timeout)
{
	int rtrn;

	if(rec->rec_conn->conn_version >= 26)
		rtrn = cmyth_livetv_chain_select(rec, timeout);
	else
		rtrn = cmyth_ringbuf_select(rec, timeout);

	return rtrn;
}

/*
 * cmyth_livetv_get_block()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Read incoming file data off the network into a buffer of length len.
 *
 * Return Value:
 *
 * Sucess: number of bytes read into buf
 *
 * Failure: -1
 */
int32_t
cmyth_livetv_get_block(cmyth_recorder_t rec, char *buf, int32_t len)
{
	int rtrn;

	if(rec->rec_conn->conn_version >= 26)
		rtrn = cmyth_livetv_chain_get_block(rec, buf, len);
	else
		rtrn = cmyth_ringbuf_get_block(rec, buf, len);

	return rtrn;
}

cmyth_recorder_t
cmyth_spawn_live_tv(cmyth_recorder_t rec, uint32_t buflen, int32_t tcp_rcvbuf,
	void (*prog_update_callback)(cmyth_proginfo_t),
	char ** err, char* channame)
{
	cmyth_recorder_t rtrn = NULL;
	int i;

	if(rec->rec_conn->conn_version >= 26) {
		if (cmyth_recorder_spawn_chain_livetv(rec, channame) != 0) {
			*err = "Spawn livetv failed.";
			goto err;
		}

		for(i=0; i<20; i++) {
			if(cmyth_recorder_is_recording(rec) != 1)
				sleep(1);
			else
				break;
		}

		if ((rtrn = cmyth_livetv_chain_setup(rec, buflen, tcp_rcvbuf,
							prog_update_callback)) == NULL) {
			*err = "Failed to setup livetv.";
			goto err;
		}
	}
	else {
		if ((rtrn = cmyth_ringbuf_setup(rec)) == NULL) {
			*err = "Failed to setup ringbuffer.";
			goto err;
		}

		if (cmyth_conn_connect_ring(rtrn, buflen, tcp_rcvbuf) != 0) {
			*err = "Cannot connect to mythtv ringbuffer.";
			goto err;
		}

		if (cmyth_recorder_spawn_livetv(rtrn) != 0) {
			*err = "Spawn livetv failed.";
			goto err;
		}
	}

	if(rtrn->rec_conn->conn_version < 34 && channame) {
		if (cmyth_recorder_pause(rtrn) != 0) {
			*err = "Failed to pause recorder to change channel";
			goto err;
		}

		if (cmyth_recorder_set_channel(rtrn, channame) != 0) {
			*err = "Failed to change channel on recorder";
			goto err;
		}
	}

	err:

	return rtrn;
}
