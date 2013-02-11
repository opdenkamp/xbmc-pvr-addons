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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "platform/util/StdString.h"

#include <ctime>

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

class MythRecorder;
class MythSignal;
class MythFile;
class MythProgramInfo;

#include "MythProgramInfo.h"

template <class T> class MythPointer;

class MythEventObserver
{
public:
  // Request to stop Live TV
  virtual void CloseLiveStream() = 0;
};

class MythEventHandler
{
public:
  MythEventHandler(const CStdString &server, unsigned short port);
  void RegisterObserver(MythEventObserver *observer);

  void PreventLiveChainUpdate();
  void AllowLiveChainUpdate();

  MythSignal GetSignal();

  void SetRecorder(const MythRecorder &recorder);
  void SetRecordingListener(const CStdString &recordid, const MythFile &file);
  void EnablePlayback();
  void DisablePlayback();
  bool IsPlaybackActive() const;
  bool IsListening() const;

  //Recordings change events
  enum RecordingChangeType
  {
    CHANGE_ALL = 0,
    CHANGE_ADD = 1,
    CHANGE_UPDATE,
    CHANGE_DELETE
  };

  class RecordingChangeEvent
  {
  public:
    RecordingChangeEvent(RecordingChangeType type, unsigned int chanid, char *recstartts)
      : m_type(type)
      , m_channelID(chanid)
      , m_recordStartTimeSlot(0)
    {
      if (recstartts) {
        MythTimestamp time(recstartts, false);
        m_recordStartTimeSlot = time.UnixTime();
      }
    }

    RecordingChangeEvent(RecordingChangeType type, const MythProgramInfo &prog)
      : m_type(type)
      , m_channelID(0)
      , m_recordStartTimeSlot(0)
      , m_prog(prog)
    {
    }

    RecordingChangeEvent(RecordingChangeType type)
      : m_type(type)
      , m_channelID(0)
      , m_recordStartTimeSlot(0)
    {
    }

    RecordingChangeType Type() const { return m_type; }
    unsigned int ChannelID() const { return m_channelID; };
    time_t RecordingStartTimeslot() const { return m_recordStartTimeSlot; }
    MythProgramInfo Program() const { return m_prog; }

  private:
    RecordingChangeType m_type;
    unsigned int m_channelID;     // ADD and DELETE
    time_t m_recordStartTimeSlot; // ADD and DELETE
    MythProgramInfo m_prog;       // UPDATE
  };

  bool HasRecordingChangeEvent() const;
  RecordingChangeEvent NextRecordingChangeEvent();
  void ClearRecordingChangeEvents();

private:
  class MythEventHandlerPrivate; // Needs to be within MythEventHandler to inherit friend permissions
  boost::shared_ptr<MythEventHandlerPrivate> m_imp; // Private Implementation
};
