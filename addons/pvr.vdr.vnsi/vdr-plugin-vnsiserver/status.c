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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "vnsi.h"
#include "status.h"
#include "vnsiclient.h"
#include <vdr/tools.h>
#include <vdr/recording.h>
#include <vdr/videodir.h>
#include <vdr/shutdown.h>

cVNSIStatus::~cVNSIStatus()
{
  Shutdown();
}

void cVNSIStatus::Shutdown()
{
  Cancel(5);
  cMutexLock lock(&m_mutex);
  for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
  {
    delete (*i);
  }
  m_clients.erase(m_clients.begin(), m_clients.end());
}

void cVNSIStatus::AddClient(cVNSIClient* client)
{
  cMutexLock lock(&m_mutex);
  m_clients.push_back(client);
}

void cVNSIStatus::Action(void)
{
  cTimeMs chanTimer(0);

  // get initial state of the recordings
  int recState = -1;
  Recordings.StateChanged(recState);

  // get initial state of the timers
  int timerState = -1;
  Timers.Modified(timerState);

  // last update of epg
  time_t epgUpdate = cSchedules::Modified();

  // delete old timeshift file
  cString cmd;
  struct stat sb;
  if ((*TimeshiftBufferDir) && stat(TimeshiftBufferDir, &sb) == 0 && S_ISDIR(sb.st_mode))
  {
    if (TimeshiftBufferDir[strlen(TimeshiftBufferDir)-1] == '/')
      cmd = cString::sprintf("rm -f %s*.vnsi", TimeshiftBufferDir);
    else
      cmd = cString::sprintf("rm -f %s/*.vnsi", TimeshiftBufferDir);
  }
  else
  {
#if VDRVERSNUM >= 20102
    cmd = cString::sprintf("rm -f %s/*.vnsi", cVideoDirectory::Name());
#else
    cmd = cString::sprintf("rm -f %s/*.vnsi", VideoDirectory);
#endif
  }
  int ret = system(cmd);

  // set thread priority
  SetPriority(1);

  while (Running())
  {
    m_mutex.Lock();

    // remove disconnected clients
    for (ClientList::iterator i = m_clients.begin(); i != m_clients.end();)
    {
      if (!(*i)->Active())
      {
        INFOLOG("Client with ID %u seems to be disconnected, removing from client list", (*i)->GetID());
        delete (*i);
        i = m_clients.erase(i);
      }
      else {
        i++;
      }
    }

    // trigger clients to reload the modified channel list
    if(m_clients.size() > 0 && chanTimer.TimedOut())
    {
      int modified = Channels.Modified();
      if (modified)
      {
        Channels.SetModified((modified == CHANNELSMOD_USER) ? true : false);
        INFOLOG("Requesting clients to reload channel list");
        for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
          (*i)->ChannelsChange();
      }
      chanTimer.Set(5000);
    }

    // reset inactivity timeout as long as there are clients connected
    if(m_clients.size() > 0) {
      ShutdownHandler.SetUserInactiveTimeout();
    }

    // update recordings
    if(Recordings.StateChanged(recState))
    {
      INFOLOG("Recordings state changed (%i)", recState);
      INFOLOG("Requesting clients to reload recordings list");
      for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
        (*i)->RecordingsChange();
    }

    // update timers
    if(Timers.Modified(timerState))
    {
      INFOLOG("Timers state changed (%i)", timerState);
      INFOLOG("Requesting clients to reload timers");
      for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
      {
        (*i)->TimerChange();
      }
    }

    // update epg
    if((cSchedules::Modified() > epgUpdate + 10) || time(NULL) > epgUpdate + 300)
    {
      for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
      {
        (*i)->EpgChange();
      }
      epgUpdate = cSchedules::Modified();
    }

    m_mutex.Unlock();

    usleep(250*1000);
  }
}
