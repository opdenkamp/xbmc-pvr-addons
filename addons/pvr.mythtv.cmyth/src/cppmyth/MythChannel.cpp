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

#include "MythChannel.h"
#include "MythPointer.h"

MythChannel::MythChannel()
  : m_channel_t()
  , m_radio(false)
{
}

MythChannel::MythChannel(cmyth_channel_t cmyth_channel, bool isRadio)
  : m_channel_t(new MythPointer<cmyth_channel_t>())
  , m_radio(isRadio)
{
  *m_channel_t = cmyth_channel;
}

bool MythChannel::IsNull() const
{
  if (m_channel_t == NULL)
    return true;
  return *m_channel_t == NULL;
}

int MythChannel::ID()
{
  return cmyth_channel_chanid(*m_channel_t);
}

CStdString MythChannel::Name()
{
  char* cChan = cmyth_channel_name(*m_channel_t);
  CStdString retval(cChan);
  ref_release(cChan);
  return retval;
}

int MythChannel::NumberInt()
{
  return cmyth_channel_channum(*m_channel_t);
}

CStdString MythChannel::Number()
{
  // Must not be deleted
  return CStdString(cmyth_channel_channumstr(*m_channel_t));
}

CStdString MythChannel::Callsign()
{
  char* cChan = cmyth_channel_callsign(*m_channel_t);
  CStdString retval(cChan);
  ref_release(cChan);
  return retval;
}

CStdString MythChannel::Icon()
{
  char* cIcon = cmyth_channel_icon(*m_channel_t);
  CStdString retval(cIcon);
  ref_release(cIcon);
  return retval;
}

bool MythChannel::Visible()
{
  return cmyth_channel_visible(*m_channel_t) > 0;
}

bool MythChannel::IsRadio() const
{
  return m_radio;
}

int MythChannel::SourceID()
{
  return cmyth_channel_sourceid(*m_channel_t);
}

int MythChannel::MultiplexID()
{
  return cmyth_channel_multiplex(*m_channel_t);
}
