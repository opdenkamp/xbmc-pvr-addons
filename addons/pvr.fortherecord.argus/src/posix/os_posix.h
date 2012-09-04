#pragma once
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PVRCLIENT_FORTHERECORD_OS_POSIX_H
#define PVRCLIENT_FORTHERECORD_OS_POSIX_H

#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(TARGET_DARWIN) || defined(__FreeBSD__)
#include <stdio.h> // for fpos_t
#endif
typedef long        LONG;
typedef LONG        HRESULT;

#define _FILE_OFFSET_BITS 64
#define FILE_BEGIN              0
#define FILE_CURRENT            1
#define FILE_END                2

// Success codes
#define S_OK                             0L
#define S_FALSE                          1L
//
#define FAILED(Status)            ((HRESULT)(Status)<0)

// Error codes
#define ERROR_FILENAME_EXCED_RANGE       206L
#define ERROR_INVALID_NAME               123L
#define E_OUTOFMEMORY                    0x8007000EL
#define E_FAIL                           0x8004005EL

#ifdef TARGET_LINUX
#include <limits.h>
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 256
#endif

#if defined(__APPLE__)
  #include <sched.h>
  #include <AvailabilityMacros.h>
  typedef int64_t   off64_t;
  typedef off_t     __off_t;
  typedef off64_t   __off64_t;
  typedef fpos_t fpos64_t;
  #define __stat64 stat
  #define stat64 stat
  #if defined(TARGET_DARWIN_IOS)
    #define statfs64 statfs
  #endif
  #define fstat64 fstat
#elif defined(__FreeBSD__)
  typedef int64_t   off64_t;
  typedef off_t     __off_t;
  typedef off64_t   __off64_t;
  typedef fpos_t fpos64_t;
  #define __stat64 stat
  #define stat64 stat
  #define statfs64 statfs
  #define fstat64 fstat
#else
  #define __stat64 stat64
#endif

#include <string.h>
#define strnicmp(X,Y,N) strncasecmp(X,Y,N)
/* Platform dependent path separator */
#define PATH_SEPARATOR_CHAR '/'
#define PATH_SEPARATOR_STRING "/"

#endif
