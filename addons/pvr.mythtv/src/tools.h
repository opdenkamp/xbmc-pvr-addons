#pragma once
/*
 *      Copyright (C) 2005-2014 Team XBMC
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef __WINDOWS__
static inline struct tm *localtime_r(const time_t  *clock, struct tm *result)
{
	struct tm *data;
	if (!clock || !result)
		return NULL;
	data = localtime(clock);
	if (!data)
		return NULL;
	memcpy(result, data, sizeof(*result));
	return result;
}
#else
#include <time.h>
#endif

static inline int daytime(time_t *time)
{
  struct tm dtm;
  localtime_r(time, &dtm);
  int retval = dtm.tm_sec + dtm.tm_min * 60 + dtm.tm_hour * 3600;
  return retval;
}

static inline int weekday(time_t *time)
{
  struct tm dtm;
  localtime_r(time, &dtm);
  int retval = dtm.tm_wday;
  return retval;
}
