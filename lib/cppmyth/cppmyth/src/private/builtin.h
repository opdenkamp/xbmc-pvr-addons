/*
 *      Copyright (C) 2014 Jean-Luc Barriere
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
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef BUILTIN_H
#define	BUILTIN_H

#if defined __cplusplus
#define __STDC_LIMIT_MACROS
extern "C" {
#endif

#include <cppmyth_config.h>

#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#define str2int64 __str2int64
extern int str2int64(const char *str, int64_t *num);

#define str2int32 __str2int32
extern int str2int32(const char *str, int32_t *num);

#define str2int16 __str2int16
extern int str2int16(const char *str, int16_t *num);

#define str2int8 __str2int8
extern int str2int8(const char *str, int8_t *num);

#define str2uint32 __str2uint32
extern int str2uint32(const char *str, uint32_t *num);

#define str2uint16 __str2uint16
extern int str2uint16(const char *str, uint16_t *num);

#define str2uint8 __str2uint8
extern int str2uint8(const char *str, uint8_t *num);

#define int64str __int64str
static CC_INLINE void int64str(int64_t num, char *str)
{
  sprintf(str, "%lld", (long long)num);
}

#define int32str __int32str
static CC_INLINE void int32str(int32_t num, char *str)
{
  sprintf(str, "%ld", (long)num);
}

#define int16str __int16str
static CC_INLINE void int16str(int16_t num, char *str)
{
  sprintf(str, "%d", num);
}

#define int8str __int8str
static CC_INLINE void int8str(int8_t num, char *str)
{
  sprintf(str, "%d", num);
}

#define uint32str __uint32str
static CC_INLINE void uint32str(uint32_t num, char *str)
{
  sprintf(str, "%lu", (unsigned long)num);
}

#define uint16str __uint16str
static CC_INLINE void uint16str(uint16_t num, char *str)
{
  sprintf(str, "%u", num);
}

#define uint8str __uint8str
static CC_INLINE void uint8str(uint8_t num, char *str)
{
  sprintf(str, "%u", num);
}

#if !HAVE_TIMEGM
#define timegm __timegm
extern time_t timegm(struct tm *utctime_tm);
#endif

#if !HAVE_LOCALTIME_R
static CC_INLINE struct tm *localtime_r(const time_t * clock, struct tm *result)
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
#endif

#if !HAVE_GMTIME_R
static CC_INLINE struct tm *gmtime_r(const time_t * clock, struct tm *result)
{
	struct tm *data;
	if (!clock || !result)
		return NULL;
	data = gmtime(clock);
	if (!data)
		return NULL;
	memcpy(result, data, sizeof(*result));
	return result;
}
#endif

#define TIMESTAMP_UTC_LEN (sizeof("YYYY-MM-DDTHH:MM:SSZ") - 1)
#define TIMESTAMP_LEN     (sizeof("YYYY-MM-DDTHH:MM:SS") - 1)
#define DATESTAMP_LEN     (sizeof("YYYY-MM-DD") - 1)
#define INVALID_TIME      (time_t)(-1)

#define str2time __str2time
extern int str2time(const char *str, time_t *time);

#define time2iso8601utc __time2iso8601utc
extern void time2iso8601utc(time_t time, char *str);

#define time2iso8601 __time2iso8601
extern void time2iso8601(time_t time, char *str);

#define time2isodate __time2isodate
extern void time2isodate(time_t time, char *str);

#if defined __cplusplus
}
#endif

#endif	/* BUILTIN_H */
