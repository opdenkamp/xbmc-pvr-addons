/*
 *      Copyright (C) 2014 Fred Hoogduin
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
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "client.h" //for XBMC->Log
#include "argustvrpc.h"
#include "EventsThread.h"

using namespace ADDON;

CEventsThread::CEventsThread(void)
{
  XBMC->Log(LOG_DEBUG, "CEventsThread:: constructor");
}


CEventsThread::~CEventsThread(void)
{
  XBMC->Log(LOG_DEBUG, "CEventsThread:: destructor");
  if (m_subscribed)
  {
    int retval = ArgusTV::UnsubscribeServiceEvents(m_monitorId);
    if (retval < 0)
    {
      XBMC->Log(LOG_NOTICE, "CEventsThread:: unsubscribe from events failed");
    }
  }
}

void CEventsThread::Connect()
{
  XBMC->Log(LOG_DEBUG, "CEventsThread::Connect");
  // Subscribe to service events
  Json::Value response;
  int retval = ArgusTV::SubscribeServiceEvents(ArgusTV::AllEvents, response);
  if (retval >= 0)
  {
    m_monitorId = response.asString();
    m_subscribed = true;
    XBMC->Log(LOG_DEBUG, "CEventsThread:: monitorId = %s", m_monitorId.c_str());
  }
  else
  {
    m_subscribed = false;
    XBMC->Log(LOG_NOTICE, "CEventsThread:: subscribe to events failed");
  }
}

void *CEventsThread::Process()
{
  XBMC->Log(LOG_DEBUG, "CEventsThread:: thread started");
  while (!IsStopped() && m_subscribed)
  {
    // Get service events
    Json::Value response;
    int retval = ArgusTV::GetServiceEvents(m_monitorId, response);
    if (retval >= 0)
    {
      if (response["Expired"].asBool())
      {
        // refresh subscription
        Connect();
      }
      else
      {
        // Process service events
        Json::Value events = response["Events"];
        if (events.size() > 0u) HandleEvents(events);
      }
    }
    // The new PLATFORM:: thread library has a problem with stopping a thread that is doing a long sleep
    for (int i = 0; i < 100; i++)
    {
      if (Sleep(100)) break;
    }
  }
  XBMC->Log(LOG_DEBUG, "CEventsThread:: thread stopped");
  return NULL;
}

void CEventsThread::HandleEvents(Json::Value events)
{
  XBMC->Log(LOG_DEBUG, "CEventsThread::HandleEvents");
  int size = events.size();
  bool mustUpdateTimers = false;
  bool mustUpdateRecordings = false;
  // Aggregate events
  for (int i = 0; i < size; i++)
  {
    Json::Value event = events[i];
    std::string eventName = event["Name"].asString();
    XBMC->Log(LOG_DEBUG, "CEventsThread:: ARGUS TV reports event %s", eventName.c_str());
    if (eventName == "UpcomingRecordingsChanged")
    {
      XBMC->Log(LOG_DEBUG, "Timers changed");
      mustUpdateTimers = true;
    }
    else if (eventName == "RecordingStarted" || eventName == "RecordingEnded")
    {
      XBMC->Log(LOG_DEBUG, "Recordings changed");
      mustUpdateRecordings = true;
    }
  }
  // Handle aggregated events
  if (mustUpdateTimers)
  {
    XBMC->Log(LOG_DEBUG, "CEventsThread:: Timers update triggered");
    PVR->TriggerTimerUpdate();
  }
  if (mustUpdateRecordings)
  {
    XBMC->Log(LOG_DEBUG, "CEventsThread:: Recordings update triggered");
    PVR->TriggerRecordingUpdate();
  }
}
