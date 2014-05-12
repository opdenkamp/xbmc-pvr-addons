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

#include "AsyncState.h"
#include "client.h"

using namespace PLATFORM;

AsyncState::AsyncState()
{
  SetState(ASYNC_NONE);
}

void AsyncState::SetState(eAsyncState state)
{
  CLockObject lock(m_mutex);
  m_state = state;
}

void AsyncState::WaitForState(eAsyncState state, int timeoutMs /* = -1*/)
{
  // use global response timeout if no specific timeout has been defined
  if (timeoutMs == -1)
  {
    CLockObject lock(g_mutex);
    timeoutMs = g_iResponseTimeout * 1000;
  }

  int timeSlept = 0;

  // sleep for WAIT_PERIOD until the timeout is reached or the state changes
  while (GetState() < state && timeSlept < timeoutMs)
  {
    usleep(WAIT_PERIOD_MS * 1000);
    timeSlept += WAIT_PERIOD_MS;
  }
}
