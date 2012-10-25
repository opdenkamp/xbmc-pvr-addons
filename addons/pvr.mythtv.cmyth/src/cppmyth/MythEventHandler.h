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

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

class MythRecorder;
class MythSignal;
class MythFile;

template <class T> class MythPointer;

class MythEventHandler 
{
public:
  MythEventHandler();
  MythEventHandler(const CStdString &server, unsigned short port);

  bool TryReconnect();

  void PreventLiveChainUpdate();
  void AllowLiveChainUpdate();

  MythSignal GetSignal();

  void SetRecorder(const MythRecorder &recorder);
  void SetRecordingListener(const CStdString &recordid, const MythFile &file);
  void EnablePlayback();
  void DisablePlayback();
  bool IsPlaybackActive() const;

private:
  class MythEventHandlerPrivate; // Needs to be within MythEventHandler to inherit friend permissions
  boost::shared_ptr<MythEventHandlerPrivate> m_imp; // Private Implementation
  int m_retryCount;
};
