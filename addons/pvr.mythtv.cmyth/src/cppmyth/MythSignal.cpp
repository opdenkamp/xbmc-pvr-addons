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

#include "MythSignal.h"

MythSignal::MythSignal()
  : m_AdapterStatus()
  , m_SNR(0)
  , m_Signal(0)
  , m_BER(0)
  , m_UNC(0)
  , m_ID(0)
{
}

CStdString MythSignal::AdapterStatus() const
{
  return m_AdapterStatus;
}

int MythSignal::SNR() const
{
  return m_SNR;
}

int MythSignal::Signal() const
{
  return m_Signal;
}

long MythSignal::BER() const
{
  return m_BER;
}

long MythSignal::UNC() const
{
  return m_UNC;
}

int MythSignal::ID() const
{
  return m_ID;
}
