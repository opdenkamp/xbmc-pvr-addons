/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cmyth_local.h>

static void
cmyth_epginfolist_destroy(cmyth_epginfolist_t el)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!el) {
		return;
	}
	for (i = 0; i < el->epginfolist_count; ++i) {
		if (el->epginfolist_list[i]) {
			ref_release(el->epginfolist_list[i]);
		}
		el->epginfolist_list[i] = NULL;
	}
	if (el->epginfolist_list) {
		free(el->epginfolist_list);
	}
}

cmyth_epginfolist_t
cmyth_epginfolist_create(void)
{
	cmyth_epginfolist_t ret;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_epginfolist_destroy);

	ret->epginfolist_list = NULL;
	ret->epginfolist_count = 0;
	return ret;
}

void
cmyth_epginfo_destroy(cmyth_epginfo_t e)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!e) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!a\n", __FUNCTION__);
		return;
	}
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);

	if (e->callsign) {
		ref_release(e->callsign);
	}
	if (e->channame) {
		ref_release(e->channame);
	}
	if (e->title) {
		ref_release(e->title);
	}
	if (e->subtitle) {
		ref_release(e->subtitle);
	}
	if (e->description) {
		ref_release(e->description);
	}
	if (e->programid) {
		ref_release(e->programid);
	}
	if (e->seriesid) {
		ref_release(e->seriesid);
	}
	if (e->category) {
		ref_release(e->category);
	}
	if (e->category_type) {
		ref_release(e->category_type);
	}
}

cmyth_epginfo_t
cmyth_epginfo_create(void)
{
	cmyth_epginfo_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_epginfo_destroy);

	ret->chanid = 0;
	ret->callsign = NULL;
	ret->channame = NULL;
	ret->sourceid = 0;
	ret->title = NULL;
	ret->subtitle = NULL;
	ret->description = NULL;
	ret->starttime = 0;
	ret->endtime = 0;
	ret->programid = NULL;
	ret->seriesid = NULL;
	ret->category = NULL;
	ret->category_type = NULL;
	ret->channum = 0;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

/*
 * cmyth_epginfolist_get_item()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Retrieve the EPG structure found at index 'index' in the list in 'el'.
 * Return the EPG structure held. Before forgetting the reference to this
 * EPG structure the caller must call ref_release().
 *
 * Return Value:
 *
 * Success: A non-null cmyth_epginfo_t (this is a pointer type)
 *
 * Failure: A NULL cmyth_epginfo_t
 */
cmyth_epginfo_t
cmyth_epginfolist_get_item(cmyth_epginfolist_t el, int index)
{
	if (!el) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL epginfo list\n",
			  __FUNCTION__);
		return NULL;
	}
	if (!el->epginfolist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL list\n",
			  __FUNCTION__);
		return NULL;
	}
	if ((index < 0) || (index >= el->epginfolist_count)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: index %d out of range\n",
			  __FUNCTION__, index);
		return NULL;
	}
	ref_hold(el->epginfolist_list[index]);
	return el->epginfolist_list[index];
}

/*
 * cmyth_epginfolist_get_count()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Retrieve the number of elements in the EPG list
 * structure in 'el'.
 *
 * Return Value:
 *
 * Success: A number >= 0 indicating the number of items in 'el'
 *
 * Failure: -(errno)
 */
int
cmyth_epginfolist_get_count(cmyth_epginfolist_t el)
{
	if (!el) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL epginfo list\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	return el->epginfolist_count;
}

/*
 * cmyth_epginfo_chanid()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'chanid' field of a epginfo structure.
 *
 * Return Value:
 *
 * Success: long chanid number.
 *
 * Failure: 0
 */
uint32_t
cmyth_epginfo_chanid(cmyth_epginfo_t e)
{
	if (!e) {
		return 0;
	}
	return e->chanid;
}

/*
 * cmyth_epginfo_callsign()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'callsign' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_callsign(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->callsign);
}

/*
 * cmyth_epginfo_channame()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'channame' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_channame(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->channame);
}

/*
 * cmyth_epginfo_sourceid()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'sourceid' field of a epginfo structure.
 *
 * Return Value:
 *
 * Success: long sourceid number.
 *
 * Failure: 0
 */
uint32_t
cmyth_epginfo_sourceid(cmyth_epginfo_t e)
{
	if (!e) {
		return 0;
	}
	return e->sourceid;
}

/*
 * cmyth_epginfo_title()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'title' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_title(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->title);
}

/*
 * cmyth_epginfo_subtitle()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'subtitle' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_subtitle(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->subtitle);
}

/*
 * cmyth_epginfo_description()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'description' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_description(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->description);
}

/*
 * cmyth_epginfo_starttime()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'starttime' field of a epginfo structure.
 *
 * Return Value:
 *
 * Success: long starttime number.
 *
 * Failure: 0
 */
time_t
cmyth_epginfo_starttime(cmyth_epginfo_t e)
{
	if (!e) {
		return 0;
	}
	return e->starttime;
}

/*
 * cmyth_epginfo_endtime()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'endtime' field of a epginfo structure.
 *
 * Return Value:
 *
 * Success: long endtime number.
 *
 * Failure: 0
 */
time_t
cmyth_epginfo_endtime(cmyth_epginfo_t e)
{
	if (!e) {
		return 0;
	}
	return e->endtime;
}

/*
 * cmyth_epginfo_programid()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'programid' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_programid(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->programid);
}

/*
 * cmyth_epginfo_seriesid()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'seriesid' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_seriesid(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->seriesid);
}

/*
 * cmyth_epginfo_category()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'category' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_category(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->category);
}

/*
 * cmyth_epginfo_category_type()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'category_type' field of a epginfo structure.
 *
 * The returned string is a pointer to the string within the epginfo
 * structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 * Before forgetting the reference to this string the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_epginfo_category_type(cmyth_epginfo_t e)
{
	if (!e) {
		return NULL;
	}
	return ref_hold(e->category_type);
}

/*
 * cmyth_epginfo_channum()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'channum' field of a epginfo structure.
 *
 * Return Value:
 *
 * Success: long channum number.
 *
 * Failure: 0
 */
uint32_t
cmyth_epginfo_channum(cmyth_epginfo_t e)
{
	if (!e) {
		return 0;
	}
	return e->channum;
}
