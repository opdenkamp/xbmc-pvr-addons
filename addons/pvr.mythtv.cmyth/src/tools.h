/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifndef __TOOLS_H
#define __TOOLS_H
/*
#if defined(_WIN32) || defined(_WIN64)
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#endif
#include "libcmyth.h"
extern CHelper_libcmyth *CMYTH;
#include "utils/StdString.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>*/
#ifndef __WINDOWS__
#include <time.h>
#endif
/*
#define ERRNUL(e) {errno=e;return 0;}
#define ERRSYS(e) {errno=e;return -1;}

#define SECSINDAY  86400

#define KILOBYTE(n) ((n) * 1024)
#define MEGABYTE(n) ((n) * 1024LL * 1024LL)

#define MALLOC(type, size)  (type *)malloc(sizeof(type) * (size))

#define DELETENULL(p) (delete (p), p = NULL)
//
//#define CHECK(s) { if ((s) < 0) LOG_ERROR; } // used for 'ioctl()' calls
#define FATALERRNO (errno && errno != EAGAIN && errno != EINTR)

class cTimeMs
{
private:
  uint64_t begin;
public:
  cTimeMs(int Ms = 0);
      ///< Creates a timer with ms resolution and an initial timeout of Ms.
  static uint64_t Now(void);
  void Set(int Ms = 0);
  bool TimedOut(void);
  uint64_t Elapsed(void);
};

void inline cSleep(const int ms)
{
#if defined(__WINDOWS__)
  Sleep(ms);
#else
  struct timespec timeOut,remains;
  timeOut.tv_sec = ms/1000;
  timeOut.tv_nsec = (ms-timeOut.tv_sec*1000)*1000000; 
  while(EINTR==nanosleep(&timeOut, &remains))
  {
    timeOut=remains;
  }
#endif
}*/

int inline daytime(time_t *time)
{
	struct tm* ptm = localtime( time );
	int retval = ptm->tm_sec*60+ptm->tm_min*60+ptm->tm_hour;
	return retval;	
}

int inline weekday(time_t *time)
{
	struct tm* ptm = localtime( time );
	int retval = ptm->tm_wday;
	return retval;	
}

template <class T> class SingleLock {
private:
  T *m_mutex;
  bool m_locked;
  bool Lock()
  {
    if (m_mutex && !m_locked)
    {
      m_mutex->Lock();
      m_locked = true;
      return true;
    }
    return false;
  }

  bool Unlock()
  {
    if (m_mutex && m_locked)
    {
      m_mutex->Unlock();
      m_locked = false;
      return true;
    }
    return false;
  }

public:
  SingleLock(T *Mutex)
  {
    m_mutex = Mutex;
    m_locked = false;
    Lock();
  }

  ~SingleLock()
  {
    Unlock();    
  }
  
  void Leave() { Unlock(); }
  void Enter() { Lock(); }
};



#endif //__TOOLS_H
