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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef ASYNCSTATE_H
#define	ASYNCSTATE_H

#include "platform/threads/mutex.h"

/**
 * Represents the possible states
 */
enum eAsyncState
{
  ASYNC_NONE = 0,
  ASYNC_CHN = 1,
  ASYNC_DVR = 2,
  ASYNC_EPG = 3,
  ASYNC_DONE = 4
};

/**
 * State tracker for the initial sync process. This class is thread-safe.
 */
class AsyncState
{
public:
  AsyncState();

  virtual ~AsyncState()
  {
  };

  /**
   * @return the current state
   */
  inline eAsyncState GetState()
  {
    PLATFORM::CLockObject lock(m_mutex);
    return m_state;
  }

  /**
   * Changes the current state to "state"
   * @param state the new state
   */
  void SetState(eAsyncState state);

  /**
   * Waits for the current state to change into "state" or higher
   * before the timeout is reached
   * @param state the minimum state desired
   * @param timeout timeout in milliseconds. Defaults to -1 which means the 
   * global response timeout will be used.
   * @return whether the state changed or not
   */
  bool WaitForState(eAsyncState state, int timeoutMs = -1);

private:

  eAsyncState m_state;
  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_condition;

};

#endif	/* ASYNCSTATE_H */

