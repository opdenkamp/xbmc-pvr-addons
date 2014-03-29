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

#include "mytheventhandler.h"
#include "mythdebug.h"
#include "proto/mythprotoevent.h"
#include "private/builtin.h"
#include "private/platform/threads/threads.h"
#include "private/platform/util/util.h"

#include <vector>
#include <map>

using namespace Myth;

///////////////////////////////////////////////////////////////////////////////
////
//// EventHandlerThread
////

EventHandler::EventHandlerThread::EventHandlerThread(const std::string& server, unsigned port)
: m_event(new ProtoEvent(server,port))
{
}

EventHandler::EventHandlerThread::~EventHandlerThread()
{
  SAFE_DELETE(m_event);
}

///////////////////////////////////////////////////////////////////////////////
////
//// BasicEventHandler
////

class BasicEventHandler : public EventHandler::EventHandlerThread, private PLATFORM::CThread
{
public:
  BasicEventHandler(const std::string& server, unsigned port);
  virtual ~BasicEventHandler();
  // Implements MythEventHandlerThread
  virtual std::string GetServer() const;
  virtual unsigned GetPort() const;
  virtual bool Start();
  virtual void Stop();
  virtual bool IsRunning();
  virtual bool IsConnected();
  virtual unsigned CreateSubscription(EventSubscriber *sub);
  virtual bool SubscribeForEvent(unsigned subid, EVENT_t event);
  virtual void RevokeSubscription(unsigned subid);

private:
  PLATFORM::CMutex *m_mutex;
  unsigned m_timeout;
  // About subscriptions
  typedef std::map<EVENT_t, std::vector<unsigned> > subscriptionsByEvent_t;
  subscriptionsByEvent_t m_subscriptionsByEvent;
  typedef std::map<unsigned, EventSubscriber*> subscriptions_t;
  subscriptions_t m_subscriptions;

  void DispatchEvent(const EventMessage& msg);
  virtual void* Process(void);
  void AnnounceStatus(const char *status);
  void AnnounceTimer();
  void RetryConnect();
};

BasicEventHandler::BasicEventHandler(const std::string& server, unsigned port)
: EventHandlerThread(server, port), PLATFORM::CThread()
, m_mutex(new PLATFORM::CMutex)
, m_timeout(0)
{
}

BasicEventHandler::~BasicEventHandler()
{
  if (m_event->IsOpen())
    this->Stop();
  SAFE_DELETE(m_mutex);
}

std::string BasicEventHandler::GetServer() const
{
  return m_event->GetServer();
}

unsigned BasicEventHandler::GetPort() const
{
  return m_event->GetPort();
}

bool BasicEventHandler::Start()
{
  this->Stop();
  if (!m_event->Open())
    return false;
  PLATFORM::CThread::CreateThread();
  return true;
}

void BasicEventHandler::Stop()
{
  if (PLATFORM::CThread::IsRunning())
    PLATFORM::CThread::StopThread();
  if (m_event->IsOpen())
    m_event->Close();
  m_mutex->Clear();
}

bool BasicEventHandler::IsRunning()
{
  return PLATFORM::CThread::IsRunning();
}

bool BasicEventHandler::IsConnected()
{
  return m_event->IsOpen();
}

unsigned BasicEventHandler::CreateSubscription(EventSubscriber* sub)
{
  unsigned id = 0;
  PLATFORM::CLockObject lock(*m_mutex);
  subscriptions_t::const_iterator it = m_subscriptions.begin();
  while (it != m_subscriptions.end())
  {
    id = it->first;
    if (sub == it->second)
      return id;
    ++it;
  }
  m_subscriptions.insert(std::make_pair(++id, sub));
  return id;
}

bool BasicEventHandler::SubscribeForEvent(unsigned subid, EVENT_t event)
{
  PLATFORM::CLockObject lock(*m_mutex);
  // Only for registered subscriber
  subscriptions_t::const_iterator it1 = m_subscriptions.find(subid);
  if (it1 == m_subscriptions.end())
    return false;
  std::vector<unsigned>::const_iterator it2 = m_subscriptionsByEvent[event].begin();
  while (it2 != m_subscriptionsByEvent[event].end())
  {
    if (*it2 == subid)
      return true;
    ++it2;
  }
  m_subscriptionsByEvent[event].push_back(subid);
  return true;
}

void BasicEventHandler::RevokeSubscription(unsigned subid)
{
  PLATFORM::CLockObject lock(*m_mutex);
  subscriptions_t::iterator it;
  it = m_subscriptions.find(subid);
  if (it != m_subscriptions.end())
    m_subscriptions.erase(it);
}

void BasicEventHandler::DispatchEvent(const EventMessage& msg)
{
  PLATFORM::CLockObject lock(*m_mutex);
  std::vector<std::vector<unsigned>::iterator> revoked;
  std::vector<unsigned>::iterator it1 = m_subscriptionsByEvent[msg.event].begin();
  while (it1 != m_subscriptionsByEvent[msg.event].end())
  {
    subscriptions_t::const_iterator it2 = m_subscriptions.find(*it1);
    if (it2 != m_subscriptions.end())
      it2->second->HandleBackendMessage(msg);
    else
      revoked.push_back(it1);
    ++it1;
  }
  std::vector<std::vector<unsigned>::iterator>::const_iterator itr;
  for (itr = revoked.begin(); itr != revoked.end(); ++itr)
    m_subscriptionsByEvent[msg.event].erase(*itr);
}

void *BasicEventHandler::Process()
{
  AnnounceStatus(EVENTHANDLER_CONNECTED);
  m_timeout = EVENTHANDLER_TIMEOUT;
  while (!PLATFORM::CThread::IsStopped())
  {
    int r;
    EventMessage msg;
    r = m_event->RcvBackendMessage(m_timeout, msg);
    if (r > 0)
      DispatchEvent(msg);
    else if (r < 0)
    {
      AnnounceStatus(EVENTHANDLER_DISCONNECTED);
      RetryConnect();
    }
    else
    {
      AnnounceTimer();
    }
  }
  // Close connection
  AnnounceStatus(EVENTHANDLER_DISCONNECTED);
  m_event->Close();
  return NULL;
}

void BasicEventHandler::AnnounceStatus(const char *status)
{
  EventMessage msg;
  msg.event = EVENT_HANDLER_STATUS;
  msg.subject.push_back(status);
  msg.subject.push_back(m_event->GetServer());
  DispatchEvent(msg);
}

void BasicEventHandler::AnnounceTimer()
{
  char buf[32];
  EventMessage msg;
  msg.event = EVENT_HANDLER_TIMER;
  msg.subject.push_back(buf);
  DispatchEvent(msg);
}

void BasicEventHandler::RetryConnect()
{
  unsigned c = 0;
  while (!PLATFORM::CThread::IsStopped())
  {
    usleep(500000);
    if (++c > 9)  // 5 seconds
    {
      if (m_event->Open())
      {
        AnnounceStatus(EVENTHANDLER_CONNECTED);
        break;
      }
      c = 0;
      DBG(MYTH_DBG_INFO, "%s: could not open event socket (%d)\n", __FUNCTION__, m_event->GetSocketErrNo());
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
////
//// EventHandler
////

EventHandler::EventHandler(const std::string& server, unsigned port)
: m_imp()
{
  // Choose implementation
  m_imp = EventHandlerThreadPtr(new BasicEventHandler(server, port));
}
