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
 * cmyth_chanlist_destroy()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Destroy the channels list structure pointed to by 'cl' and release
 * its storage.  This should only be called by ref_release(). All others should
 * use ref_release() to release references to a channels list structure.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_chanlist_destroy(cmyth_chanlist_t cl)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!cl) {
		return;
	}
	for (i  = 0; i < cl->chanlist_count; ++i) {
		if (cl->chanlist_list[i]) {
			ref_release(cl->chanlist_list[i]);
		}
		cl->chanlist_list[i] = NULL;
	}
	if (cl->chanlist_list) {
		free(cl->chanlist_list);
	}
}

/*
 * cmyth_chanlist_create()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Create a channels list structure and return a pointer to the structure.
 * Before forgetting the reference to this channel list structure the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_chanlist_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_chanlist_t
 */
cmyth_chanlist_t
cmyth_chanlist_create(void)
{
	cmyth_chanlist_t ret;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_chanlist_destroy);

	ret->chanlist_list = NULL;
	ret->chanlist_count = 0;
	return ret;
}

/*
 * cmyth_chanlist_get_item()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Retrieve the channel structure found at index 'index' in the list in 'cl'.
 * Return the channel structure held.
 * Before forgetting the reference to this channel structure the caller
 * must call ref_release().
 *
 * Return Value:
 *
 * Success: A non-null cmyth_channel_t (this is a pointer type)
 *
 * Failure: A NULL cmyth_channel_t
 */
cmyth_channel_t
cmyth_chanlist_get_item(cmyth_chanlist_t cl, int index)
{
	if (!cl) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program list\n",
			  __FUNCTION__);
		return NULL;
	}
	if (!cl->chanlist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL list\n",
			  __FUNCTION__);
		return NULL;
	}
	if ((index < 0) || (index >= cl->chanlist_count)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: index %d out of range\n",
			  __FUNCTION__, index);
		return NULL;
	}
	ref_hold(cl->chanlist_list[index]);
	return cl->chanlist_list[index];
}

/*
 * cmyth_chanlist_get_count()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Retrieve the number of elements in the channels list
 * structure in 'cl'.
 *
 * Return Value:
 *
 * Success: A number >= 0 indicating the number of items in 'cl'
 *
 * Failure: -(errno)
 */
int
cmyth_chanlist_get_count(cmyth_chanlist_t cl)
{
	if (!cl) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program list\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	return cl->chanlist_count;
}

/*
 * cmyth_channel_destroy()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Destroy the channel structure pointed to by 'c' and release
 * its storage.  This should only be called by ref_release().
 * All others should use ref_release() to release references to
 * a channel structure.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_channel_destroy(cmyth_channel_t c)
{

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!c) {
		return;
	}

	if(c->chanstr)
		ref_release(c->chanstr);
	if(c->name)
		ref_release(c->name);
	if(c->callsign)
		ref_release(c->callsign);
	if(c->icon)
		ref_release(c->icon);
}

/*
 * cmyth_channel_create()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Create a channel structure to be used to hold channel and return
 * a pointer to the structure.  The structure is initialized to
 * default values.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_channel_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_channel_t
 */
cmyth_channel_t
cmyth_channel_create(void)
{
	cmyth_channel_t ret = ref_alloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_channel_destroy);

	return ret;
}

/*
 * cmyth_channel_chanid()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'chanid' field of a channel structure.
 *
 * Return Value:
 *
 * Success: long chan ID.
 *
 * Failure: 0
 */
uint32_t
cmyth_channel_chanid(cmyth_channel_t channel)
{
	if (!channel) {
		return 0;
	}
	return channel->chanid;
}

/*
 * cmyth_channel_channum()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'channum' field of a channel structure.
 *
 * Return Value:
 *
 * Success: long channel number.
 *
 * Failure: 0
 */
uint32_t
cmyth_channel_channum(cmyth_channel_t channel)
{
	if (!channel) {
		return 0;
	}
	return channel->channum;
}

/*
 * cmyth_channel_channumstr()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'chanstr' field of a channel structure.
 *
 * The returned string is a pointer to the string within the channel
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
cmyth_channel_channumstr(cmyth_channel_t channel)
{
	if (!channel) {
		return NULL;
	}
	return ref_hold(channel->chanstr);
}

/*
 * cmyth_channel_name()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'name' field of a channel structure.
 *
 * The returned string is a pointer to the string within the channel
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
cmyth_channel_name(cmyth_channel_t channel)
{
	if (!channel) {
		return NULL;
	}
	return ref_hold(channel->name);
}

/*
 * cmyth_channel_callsign()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'callsign' field of a channel structure.
 *
 * The returned string is a pointer to the string within the channel
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
cmyth_channel_callsign(cmyth_channel_t channel)
{
	if (!channel) {
		return NULL;
	}
	return ref_hold(channel->callsign);
}

/*
 * cmyth_channel_icon()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'icon' field of a channel structure.
 *
 * The returned string is a pointer to the string within the channel
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
cmyth_channel_icon(cmyth_channel_t channel)
{
	if (!channel) {
		return NULL;
	}
	return ref_hold(channel->icon);
}

/*
 * cmyth_channel_visible()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'visible' field of a channel structure.
 *
 * Return Value:
 *
 * Success: visible flag.
 *
 * Failure: 0
 */
uint8_t
cmyth_channel_visible(cmyth_channel_t channel)
{
	if (!channel) {
		return 0;
	}
	return channel->visible;
}

/*
 * cmyth_channel_sourceid()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'sourceid' field of a channel structure.
 *
 * Return Value:
 *
 * Success: long source ID.
 *
 * Failure: 0
 */
uint32_t
cmyth_channel_sourceid(cmyth_channel_t channel)
{
	if (!channel) {
		return 0;
	}
	return channel->sourceid;
}

/*
 * cmyth_channel_multiplex()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'multiplex' field of a channel structure.
 *
 * Return Value:
 *
 * Success: long multiplex ID.
 *
 * Failure: 0
 */
uint32_t
cmyth_channel_multiplex(cmyth_channel_t channel)
{
	if (!channel) {
		return 0;
	}
	return channel->multiplex;
}