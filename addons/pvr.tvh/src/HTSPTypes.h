#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include <deque>
#include <algorithm>
#include <vector>
#include <map>
#include "platform/util/StdString.h"
#include "client.h"

typedef enum {
  DVR_PRIO_IMPORTANT,
  DVR_PRIO_HIGH,
  DVR_PRIO_NORMAL,
  DVR_PRIO_LOW,
  DVR_PRIO_UNIMPORTANT
} dvr_prio_t;

typedef enum {
  DVR_ACTION_TYPE_CUT,
  DVR_ACTION_TYPE_MUTE,
  DVR_ACTION_TYPE_SCENE,
  DVR_ACTION_TYPE_COMBREAK,
  
} dvr_action_type_t;

struct STag
{
  bool                  del;
  uint32_t              id;
  std::string           name;
  std::string           icon;
  std::vector<uint32_t> channels;

  STag() { Clear(); }
  void Clear()
  {
    del   = false;
    id    = 0;
    name.clear();
    icon.clear();
    channels.clear();
  }
};

struct SChannel
{
  bool             del;
  uint32_t         id;
  uint32_t         now;
  uint32_t         next;
  uint32_t         num;
  bool             radio;
  uint32_t         caid;
  std::string      name;
  std::string      icon;

  SChannel() { Clear(); }
  void Clear()
  {
    del   = false;
    id    = 0;
    now   = 0;
    next  = 0;
    num   = 0;
    radio = false;
    caid  = 0;
    name.clear();
    icon.clear();
  }
  bool operator<(const SChannel &right) const
  {
    return num < right.num;
  }
};

struct SRecording
{
  bool             del;
  uint32_t         id;
  uint32_t         channel;
  int64_t          start;
  int64_t          stop;
  std::string      title;
  std::string      path;
  std::string      description;
  PVR_TIMER_STATE  state;
  std::string      error;

  SRecording() { Clear(); }
  void Clear()
  {
    del     = false;
    id      = 0;
    channel = 0;
    start   = 0;
    stop    = 0;
    state   = PVR_TIMER_STATE_ERROR;
    title.clear();
    description.clear();
    error.clear();
  }
  
  bool IsRecording () const
  {
    return state == PVR_TIMER_STATE_COMPLETED ||
           state == PVR_TIMER_STATE_ABORTED   ||
           state == PVR_TIMER_STATE_RECORDING;
  }

  bool IsTimer () const
  {
    return state == PVR_TIMER_STATE_SCHEDULED ||
           state == PVR_TIMER_STATE_RECORDING;
  }
};

struct SEvent
{
  bool        del;
  uint32_t    id;
  uint32_t    next;
  uint32_t    channel;
  uint32_t    content;
  time_t      start;
  time_t      stop;
  uint32_t    stars;
  uint32_t    age;
  time_t      aired;
  uint32_t    season;
  uint32_t    episode;
  uint32_t    part;
  std::string title;
  std::string subtitle;
  std::string desc;
  std::string summary;
  std::string image;

  SEvent() { Clear(); }
  void Clear()
  {
    del     = false;
    id      = 0;
    next    = 0;
    start   = 0;
    stop    = 0;
    stars   = 0;
    age     = 0;
    aired   = 0;
    season  = 0;
    episode = 0;
    part    = 0;
    title.clear();
    subtitle.clear();
    desc.clear();
    summary.clear();
    image.clear();
  }
};

typedef std::map<uint32_t, SChannel>   SChannels;
typedef std::map<uint32_t, STag>       STags;
typedef std::map<uint32_t, SEvent>     SEvents;
typedef std::map<uint32_t, SRecording> SRecordings;

struct SSchedule
{
  bool     del;
  uint32_t channel;
  SEvents events;

  SSchedule() { Clear(); }
  void Clear ()
  {
    del     = false;
    channel = 0;
    events.clear();
  }
};

typedef std::map<int, SSchedule>  SSchedules;

struct SQueueStatus
{
  uint32_t packets; // Number of data packets in queue.
  uint32_t bytes;   // Number of bytes in queue.
  uint32_t delay;   // Estimated delay of queue (in µs)
  uint32_t bdrops;  // Number of B-frames dropped
  uint32_t pdrops;  // Number of P-frames dropped
  uint32_t idrops;  // Number of I-frames dropped

  SQueueStatus() { Clear(); }
  void Clear()
  {
    packets = 0;
    bytes   = 0;
    delay   = 0;
    bdrops  = 0;
    pdrops  = 0;
    idrops  = 0;
  }
};

struct STimeshiftStatus
{
  bool    full;
  int64_t shift;
  int64_t start;
  int64_t end;
};

struct SQuality
{
  std::string fe_status;
  uint32_t    fe_snr;
  uint32_t    fe_signal;
  uint32_t    fe_ber;
  uint32_t    fe_unc;
  
  SQuality() { Clear(); }
  void Clear ()
  {
    fe_status.clear();
    fe_snr    = 0;
    fe_signal = 0;
    fe_ber    = 0;
    fe_unc    = 0;
  }
};

struct SSourceInfo
{
  std::string si_adapter;
  std::string si_network;
  std::string si_mux;
  std::string si_provider;
  std::string si_service;

  SSourceInfo() { Clear(); }
  void Clear ()
  {
    si_adapter.clear();
    si_network.clear();
    si_mux.clear();
    si_provider.clear();
    si_service.clear();
  }
};


