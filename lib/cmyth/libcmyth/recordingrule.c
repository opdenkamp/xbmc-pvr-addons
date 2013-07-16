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

/*
 * cmyth_recordingrule_destroy()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Destroy the recording schedule structure pointed to by 'rr' and release
 * its storage.  This should only be called by ref_release().
 * All others should use ref_release() to release references to a recording
 * schedule structure.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_recordingrule_destroy(cmyth_recordingrule_t rr)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!rr) {
		return;
	}

	if (rr->title)
		ref_release(rr->title);
	if (rr->description)
		ref_release(rr->description);
	if (rr->category)
		ref_release(rr->category);
	if (rr->subtitle)
		ref_release(rr->subtitle);
	if (rr->callsign)
		ref_release(rr->callsign);
	if (rr->recgroup)
		ref_release(rr->recgroup);
	if (rr->storagegroup)
		ref_release(rr->storagegroup);
	if (rr->playgroup)
		ref_release(rr->playgroup);
	if (rr->starttime)
		ref_release(rr->starttime);
	if (rr->endtime)
		ref_release(rr->endtime);
	if (rr->profile)
		ref_release(rr->profile);
	if (rr->inetref)
		ref_release(rr->inetref);
}

/*
 * cmyth_recordingrule_create()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Create a recording schedule structure to be used to hold channel and return
 * a pointer to the structure.
 * Before forgetting the reference to this recording schedule structure the
 * caller must call ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_channel_t (this type is a pointer)
 *
 * Failure: NULL
 */
cmyth_recordingrule_t
cmyth_recordingrule_create(void)
{
	cmyth_recordingrule_t ret = ref_alloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: ref_alloc() failed\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_recordingrule_destroy);

	ret->starttime = cmyth_timestamp_create();
	if (!ret->starttime) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: cmyth_timestamp_create() failed\n", __FUNCTION__);
		goto err;
	}
	ret->endtime = cmyth_timestamp_create();
	if (!ret->endtime) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: cmyth_timestamp_create() failed\n", __FUNCTION__);
		goto err;
	}
	return ret;

	err:
	ref_release(ret);
	return NULL;
}

/*
 * cmyth_recordingrulelist_destroy()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Destroy the recordings schedule list structure pointed to by 'rrl' and
 * release its storage.  This should only be called by ref_release().
 * All others should use ref_release() to release references to a recordings
 * schedule list structure.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_recordingrulelist_destroy(cmyth_recordingrulelist_t rrl)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!rrl) {
		return;
	}
	for (i  = 0; i < rrl->recordingrulelist_count; ++i) {
		if (rrl->recordingrulelist_list[i]) {
			ref_release(rrl->recordingrulelist_list[i]);
		}
		rrl->recordingrulelist_list[i] = NULL;
	}
	if (rrl->recordingrulelist_list) {
		free(rrl->recordingrulelist_list);
	}
}

/*
 * cmyth_recordingrulelist_create()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Create a recordings schedule list structure and return a pointer to the
 * structure.
 * Before forgetting the reference to this recordings schedule list structure
 * the caller must call ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_recordingrulelist_t (this type is a pointer)
 *
 * Failure: NULL
 */
cmyth_recordingrulelist_t
cmyth_recordingrulelist_create(void)
{
	cmyth_recordingrulelist_t ret;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_recordingrulelist_destroy);

	ret->recordingrulelist_list = NULL;
	ret->recordingrulelist_count = 0;
	return ret;
}

/*
 * cmyth_recordingrule_init()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Initialize a new recording schedule structure.  The structure is initialized
 * to default values.
 * Before forgetting the reference to this recording schedule structure
 * the caller must call ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_recordingrule_t (this type is a pointer)
 *
 * Failure: NULL
 */
cmyth_recordingrule_t
cmyth_recordingrule_init(void)
{
	cmyth_recordingrule_t rr = cmyth_recordingrule_create();
	if (!rr) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s: cmyth_recordingrule_create failed\n", __FUNCTION__);
		return NULL;
	}
	cmyth_recordingrule_set_recordid(rr, 0);
	cmyth_recordingrule_set_chanid(rr, 0);
	cmyth_recordingrule_set_callsign(rr, "");
	cmyth_recordingrule_set_starttime(rr, 0);
	cmyth_recordingrule_set_endtime(rr, 0);
	cmyth_recordingrule_set_title(rr, "");
	cmyth_recordingrule_set_description(rr, "");
	cmyth_recordingrule_set_type(rr, RRULE_DONT_RECORD);
	cmyth_recordingrule_set_category(rr, "");
	cmyth_recordingrule_set_subtitle(rr, "");
	cmyth_recordingrule_set_recpriority(rr, 0);
	cmyth_recordingrule_set_startoffset(rr, 0);
	cmyth_recordingrule_set_endoffset(rr, 0);
	cmyth_recordingrule_set_searchtype(rr, RRULE_NO_SEARCH);
	cmyth_recordingrule_set_inactive(rr, 0);
	cmyth_recordingrule_set_dupmethod(rr, RRULE_CHECK_NONE);
	cmyth_recordingrule_set_dupin(rr, RRULE_IN_RECORDED);
	cmyth_recordingrule_set_recgroup(rr, "Default");
	cmyth_recordingrule_set_storagegroup(rr, "Default");
	cmyth_recordingrule_set_playgroup(rr, "Default");
	cmyth_recordingrule_set_autotranscode(rr, 0);
	cmyth_recordingrule_set_userjobs(rr, 0);
	cmyth_recordingrule_set_autocommflag(rr, 0);
	cmyth_recordingrule_set_autoexpire(rr, 0);
	cmyth_recordingrule_set_maxepisodes(rr, 0);
	cmyth_recordingrule_set_maxnewest(rr, 0);
	cmyth_recordingrule_set_transcoder(rr, 0);
        cmyth_recordingrule_set_profile(rr, "Default");
	cmyth_recordingrule_set_prefinput(rr, 0);
	cmyth_recordingrule_set_autometadata(rr, 0);
	cmyth_recordingrule_set_inetref(rr, "");
	cmyth_recordingrule_set_season(rr, 0);
	cmyth_recordingrule_set_episode(rr, 0);
	cmyth_recordingrule_set_filter(rr, 0);
	return rr;
}

uint32_t
cmyth_recordingrule_recordid(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->recordid;
}

void
cmyth_recordingrule_set_recordid(cmyth_recordingrule_t rr, uint32_t recordid)
{
	rr->recordid = recordid;
}

uint32_t
cmyth_recordingrule_chanid(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->chanid;
}

void
cmyth_recordingrule_set_chanid(cmyth_recordingrule_t rr, uint32_t chanid)
{
	rr->chanid = chanid;
}

char *
cmyth_recordingrule_callsign(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->callsign);
}

void
cmyth_recordingrule_set_callsign(cmyth_recordingrule_t rr, char *callsign)
{
	if (rr->callsign)
		ref_release(rr->callsign);
	rr->callsign = ref_strdup(callsign);
}

time_t
cmyth_recordingrule_starttime(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return cmyth_timestamp_to_unixtime(rr->starttime);
}

void
cmyth_recordingrule_set_starttime(cmyth_recordingrule_t rr, time_t starttime)
{
	if (rr->starttime)
		ref_release(rr->starttime);
	rr->starttime = cmyth_timestamp_from_unixtime(starttime);
}

time_t
cmyth_recordingrule_endtime(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return cmyth_timestamp_to_unixtime(rr->endtime);
}

void
cmyth_recordingrule_set_endtime(cmyth_recordingrule_t rr, time_t endtime)
{
	if (rr->endtime)
		ref_release(rr->endtime);
	rr->endtime = cmyth_timestamp_from_unixtime(endtime);
}

char *
cmyth_recordingrule_title(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->title);
}

void
cmyth_recordingrule_set_title(cmyth_recordingrule_t rr, char *title)
{
	if (rr->title)
		ref_release(rr->title);
	rr->title = ref_strdup(title);
}

char *
cmyth_recordingrule_description(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->description);
}

void
cmyth_recordingrule_set_description(cmyth_recordingrule_t rr, char *description)
{
	if (rr->description)
		ref_release(rr->description);
	rr->description = ref_strdup(description);
}

uint8_t
cmyth_recordingrule_type(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->type;
}

void
cmyth_recordingrule_set_type(cmyth_recordingrule_t rr, uint8_t type)
{
	rr->type = type;
}

char *
cmyth_recordingrule_category(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->category);
}

void
cmyth_recordingrule_set_category(cmyth_recordingrule_t rr, char *category)
{
	if (rr->category)
		ref_release(rr->category);
	rr->category = ref_strdup(category);
}

char *
cmyth_recordingrule_subtitle(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->subtitle);
}

void
cmyth_recordingrule_set_subtitle(cmyth_recordingrule_t rr, char *subtitle)
{
	if (rr->subtitle)
		ref_release(rr->subtitle);
	rr->subtitle = ref_strdup(subtitle);
}

int8_t
cmyth_recordingrule_recpriority(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->recpriority;
}

void
cmyth_recordingrule_set_recpriority(cmyth_recordingrule_t rr, int8_t recpriority)
{
	rr->recpriority = recpriority;
}

uint8_t
cmyth_recordingrule_startoffset(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->startoffset;
}

void
cmyth_recordingrule_set_startoffset(cmyth_recordingrule_t rr, uint8_t startoffset)
{
	rr->startoffset = startoffset;
}

uint8_t
cmyth_recordingrule_endoffset(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->endoffset;
}

void
cmyth_recordingrule_set_endoffset(cmyth_recordingrule_t rr, uint8_t endoffset)
{
	rr->endoffset = endoffset;
}

uint8_t
cmyth_recordingrule_searchtype(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->searchtype;
}

void
cmyth_recordingrule_set_searchtype(cmyth_recordingrule_t rr, uint8_t searchtype)
{
	rr->searchtype = searchtype;
}

uint8_t
cmyth_recordingrule_inactive(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->inactive;
}

void
cmyth_recordingrule_set_inactive(cmyth_recordingrule_t rr, uint8_t inactive)
{
	rr->inactive = inactive;
}

uint8_t
cmyth_recordingrule_dupmethod(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->dupmethod;
}

void
cmyth_recordingrule_set_dupmethod(cmyth_recordingrule_t rr, uint8_t dupmethod)
{
	rr->dupmethod = dupmethod;
}

uint8_t
cmyth_recordingrule_dupin(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->dupin;
}

void
cmyth_recordingrule_set_dupin(cmyth_recordingrule_t rr, uint8_t dupin)
{
	rr->dupin = dupin;
}

char *
cmyth_recordingrule_recgroup(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->recgroup);
}

void
cmyth_recordingrule_set_recgroup(cmyth_recordingrule_t rr, char *recgroup)
{
	if (rr->recgroup)
		ref_release(rr->recgroup);
	rr->recgroup = ref_strdup(recgroup);
}

char *
cmyth_recordingrule_storagegroup(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->storagegroup);
}

void
cmyth_recordingrule_set_storagegroup(cmyth_recordingrule_t rr, char *storagegroup)
{
	if (rr->storagegroup)
		ref_release(rr->storagegroup);
	rr->storagegroup = ref_strdup(storagegroup);
}

char *
cmyth_recordingrule_playgroup(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->playgroup);
}

void
cmyth_recordingrule_set_playgroup(cmyth_recordingrule_t rr, char *playgroup)
{
	if (rr->playgroup)
		ref_release(rr->playgroup);
	rr->playgroup = ref_strdup(playgroup);
}

uint8_t
cmyth_recordingrule_autotranscode(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->autotranscode;
}

void
cmyth_recordingrule_set_autotranscode(cmyth_recordingrule_t rr, uint8_t autotranscode)
{
	rr->autotranscode = autotranscode;
}

uint8_t
cmyth_recordingrule_userjobs(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->userjobs;
}

void
cmyth_recordingrule_set_userjobs(cmyth_recordingrule_t rr, uint8_t userjobs)
{
	rr->userjobs = userjobs;
}

uint8_t
cmyth_recordingrule_autocommflag(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->autocommflag;
}

void
cmyth_recordingrule_set_autocommflag(cmyth_recordingrule_t rr, uint8_t autocommflag)
{
	rr->autocommflag = autocommflag;
}

uint8_t
cmyth_recordingrule_autoexpire(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->autoexpire;
}

void
cmyth_recordingrule_set_autoexpire(cmyth_recordingrule_t rr, uint8_t autoexpire)
{
	rr->autoexpire = autoexpire;
}

uint32_t
cmyth_recordingrule_maxepisodes(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->maxepisodes;
}

void
cmyth_recordingrule_set_maxepisodes(cmyth_recordingrule_t rr, uint32_t maxepisodes)
{
	rr->maxepisodes = maxepisodes;
}

uint8_t
cmyth_recordingrule_maxnewest(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->maxnewest;
}

void
cmyth_recordingrule_set_maxnewest(cmyth_recordingrule_t rr, uint8_t maxnewest)
{
	rr->maxnewest = maxnewest;
}

uint32_t
cmyth_recordingrule_transcoder(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->transcoder;
}

void
cmyth_recordingrule_set_transcoder(cmyth_recordingrule_t rr, uint32_t transcoder)
{
	rr->transcoder = transcoder;
}

char *
cmyth_recordingrule_profile(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->profile);
}

void
cmyth_recordingrule_set_profile(cmyth_recordingrule_t rr, char *profile)
{
	if (rr->profile)
		ref_release(rr->profile);
	rr->profile = ref_strdup(profile);
}

uint32_t
cmyth_recordingrule_prefinput(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->prefinput;
}

void
cmyth_recordingrule_set_prefinput(cmyth_recordingrule_t rr, uint32_t prefinput)
{
	rr->prefinput = prefinput;
}

uint8_t
cmyth_recordingrule_autometadata(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->autometadata;
}

void
cmyth_recordingrule_set_autometadata(cmyth_recordingrule_t rr, uint8_t autometadata)
{
	rr->autometadata = autometadata;
}

char *
cmyth_recordingrule_inetref(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return NULL;
	}
	return ref_hold(rr->inetref);
}

void
cmyth_recordingrule_set_inetref(cmyth_recordingrule_t rr, char *inetref)
{
	if (rr->inetref)
		ref_release(rr->inetref);
	rr->inetref = ref_strdup(inetref);
}

uint16_t
cmyth_recordingrule_season(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->season;
}

void
cmyth_recordingrule_set_season(cmyth_recordingrule_t rr, uint16_t season)
{
	rr->season = season;
}

uint16_t
cmyth_recordingrule_episode(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->episode;
}

void
cmyth_recordingrule_set_episode(cmyth_recordingrule_t rr, uint16_t episode)
{
	rr->episode = episode;
}

uint32_t
cmyth_recordingrule_filter(cmyth_recordingrule_t rr)
{
	if (!rr) {
		return 0;
	}
	return rr->filter;
}

void
cmyth_recordingrule_set_filter(cmyth_recordingrule_t rr, uint32_t filter)
{
	rr->filter = filter;
}

/*
 * cmyth_recordingrulelist_get_item()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieve the recording schedule structure found at index 'index' in the list
 * in 'rrl'.  Return the recording schedule structure held.
 * Before forgetting the reference to this recording schedule structure the
 * caller must call ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_recordingrule_t (this type is a pointer)
 *
 * Failure: NULL
 */
cmyth_recordingrule_t
cmyth_recordingrulelist_get_item(cmyth_recordingrulelist_t rrl, int index)
{
	if (!rrl) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL recordingrule list\n",
			  __FUNCTION__);
		return NULL;
	}
	if (!rrl->recordingrulelist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL list\n",
			  __FUNCTION__);
		return NULL;
	}
	if ((index < 0) || (index >= rrl->recordingrulelist_count)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: index %d out of range\n",
			  __FUNCTION__, index);
		return NULL;
	}
	ref_hold(rrl->recordingrulelist_list[index]);
	return rrl->recordingrulelist_list[index];
}

/*
 * cmyth_recordingrulelist_get_count()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Retrieve the number of elements in the recording schedule list
 * structure in 'rrl'.
 *
 * Return Value:
 *
 * Success: A number >= 0 indicating the number of items in 'rrl'
 *
 * Failure: -(errno)
 */
int
cmyth_recordingrulelist_get_count(cmyth_recordingrulelist_t rrl)
{
	if (!rrl) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL recordingrule list\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	return rrl->recordingrulelist_count;
}
