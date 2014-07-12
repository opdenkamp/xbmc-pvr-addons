#pragma once
/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#if defined(TARGET_DARWIN)
#  ifndef PTHREAD_MUTEX_RECURSIVE_NP
#    define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#  endif
#endif

namespace PLATFORM
{
  inline pthread_mutexattr_t *GetRecursiveMutexAttribute(void)
  {
    static pthread_mutexattr_t g_mutexAttr;
    static bool bAttributeInitialised = false;
    if (!bAttributeInitialised)
    {
      pthread_mutexattr_init(&g_mutexAttr);
      pthread_mutexattr_settype(&g_mutexAttr, PTHREAD_MUTEX_RECURSIVE);
      bAttributeInitialised = true;
    }
    return &g_mutexAttr;
  }

  inline struct timespec GetAbsTime(uint64_t iIncreaseBy = 0)
  {
    struct timespec now;
    #ifdef __APPLE__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now.tv_sec  = tv.tv_sec;
    now.tv_nsec = tv.tv_usec * 1000;
    #else
    clock_gettime(CLOCK_REALTIME, &now);
    #endif
    now.tv_nsec += iIncreaseBy % 1000 * 1000000;
    now.tv_sec  += iIncreaseBy / 1000 + now.tv_nsec / 1000000000;
    now.tv_nsec %= 1000000000;
    return now;
  }

  typedef pthread_t thread_t;

  #define ThreadsCreate(thread, func, arg)         (pthread_create(&thread, NULL, (void *(*) (void *))func, (void *)arg) == 0)
  #define ThreadsWait(thread, retval)              (pthread_join(thread, retval) == 0)

  typedef pthread_mutex_t mutex_t;
  #define MutexCreate(mutex)                       pthread_mutex_init(&mutex, GetRecursiveMutexAttribute());
  #define MutexDelete(mutex)                       pthread_mutex_destroy(&mutex);
  #define MutexLock(mutex)                         (pthread_mutex_lock(&mutex) == 0)
  #define MutexTryLock(mutex)                      (pthread_mutex_trylock(&mutex) == 0)
  #define MutexUnlock(mutex)                       pthread_mutex_unlock(&mutex)

  class CConditionImpl
  {
  public:
    CConditionImpl(void)
    {
      pthread_cond_init(&m_condition, NULL);
    }

    virtual ~CConditionImpl(void)
    {
      pthread_cond_destroy(&m_condition);
    }

    void Signal(void)
    {
      pthread_cond_signal(&m_condition);
    }

    void Broadcast(void)
    {
      pthread_cond_broadcast(&m_condition);
    }

    bool Wait(mutex_t &mutex)
    {
      sched_yield();
      return (pthread_cond_wait(&m_condition, &mutex) == 0);
    }

    bool Wait(mutex_t &mutex, uint32_t iTimeoutMs)
    {
      if (iTimeoutMs == 0)
        return Wait(mutex);

      sched_yield();
      struct timespec timeout = GetAbsTime(iTimeoutMs);
      return (pthread_cond_timedwait(&m_condition, &mutex, &timeout) == 0);
    }

    pthread_cond_t m_condition;
  };
}
