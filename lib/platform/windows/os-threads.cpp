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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#include "../os.h"
#include "os-threads.h"
using namespace PLATFORM;

static ConditionArg                     g_InitializeConditionVariable;
static ConditionArg                     g_WakeConditionVariable;
static ConditionArg                     g_WakeAllConditionVariable;
static ConditionMutexArg                g_SleepConditionVariableCS;

// check whether vista+ conditions are available at runtime
static bool CheckVistaConditionFunctions(void)
{
  static int iHasVistaConditionFunctions(-1);
  if (iHasVistaConditionFunctions == -1)
  {
    HMODULE handle = GetModuleHandle("Kernel32");
    if (handle == NULL)
    {
      iHasVistaConditionFunctions = 0;
    }
    else
    {
      g_InitializeConditionVariable = (ConditionArg)     GetProcAddress(handle,"InitializeConditionVariable");
      g_WakeConditionVariable       = (ConditionArg)     GetProcAddress(handle,"WakeConditionVariable");
      g_WakeAllConditionVariable    = (ConditionArg)     GetProcAddress(handle,"WakeAllConditionVariable");
      g_SleepConditionVariableCS    = (ConditionMutexArg)GetProcAddress(handle,"SleepConditionVariableCS");

      // 1 when everything is resolved, 0 otherwise
      iHasVistaConditionFunctions = g_InitializeConditionVariable &&
                                    g_WakeConditionVariable &&
                                    g_WakeAllConditionVariable &&
                                    g_SleepConditionVariableCS ? 1 : 0;
    }
  }
  return iHasVistaConditionFunctions == 1;
}

CConditionImpl::CConditionImpl(void)
{
  m_bOnVista = CheckVistaConditionFunctions();
  if (m_bOnVista)
    (*g_InitializeConditionVariable)(m_conditionVista = new CONDITION_VARIABLE);
  else
    m_conditionPreVista = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}

CConditionImpl::~CConditionImpl(void)
{
  if (m_bOnVista)
    delete m_conditionVista;
  else
    ::CloseHandle(m_conditionPreVista);
}

void CConditionImpl::Signal(void)
{
  if (m_bOnVista)
    (*g_WakeConditionVariable)(m_conditionVista);
  else
    ::SetEvent(m_conditionPreVista);
}

void CConditionImpl::Broadcast(void)
{
  if (m_bOnVista)
    (*g_WakeAllConditionVariable)(m_conditionVista);
  else
    ::SetEvent(m_conditionPreVista);
}

bool CConditionImpl::Wait(mutex_t &mutex)
{
  if (m_bOnVista)
  {
    return ((*g_SleepConditionVariableCS)(m_conditionVista, mutex, INFINITE) ? true : false);
  }
  else
  {
    ::ResetEvent(m_conditionPreVista);
    MutexUnlock(mutex);
    DWORD iWaitReturn = ::WaitForSingleObject(m_conditionPreVista, 1000);
    MutexLock(mutex);
    return (iWaitReturn == 0);
  }
}

bool CConditionImpl::Wait(mutex_t &mutex, uint32_t iTimeoutMs)
{
  if (iTimeoutMs == 0)
    return Wait(mutex);

  if (m_bOnVista)
  {
    return ((*g_SleepConditionVariableCS)(m_conditionVista, mutex, iTimeoutMs) ? true : false);
  }
  else
  {
    ::ResetEvent(m_conditionPreVista);
    MutexUnlock(mutex);
    DWORD iWaitReturn = ::WaitForSingleObject(m_conditionPreVista, iTimeoutMs);
    MutexLock(mutex);
    return (iWaitReturn == 0);
  }
}
