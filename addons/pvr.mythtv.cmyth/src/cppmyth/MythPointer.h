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

#include "platform/threads/threads.h"

extern "C" {
#include <cmyth/cmyth.h>
#include <refmem/refmem.h>
};

using namespace PLATFORM;

template <class T> class MythPointer
{
public:
  MythPointer()
    : m_mythpointer(0)
  {
  }

  ~MythPointer()
  {
    ref_release(m_mythpointer);
    m_mythpointer = 0;
  }

  operator T()
  {
    return m_mythpointer;
  }

  MythPointer &operator=(const T mythpointer)
  {
    m_mythpointer = mythpointer;
    return *this;
  }

protected:
  T m_mythpointer;
};

template <class T> class MythPointerThreadSafe : public MythPointer<T>, public CMutex
{
public:
  operator T()
  {
    return this->m_mythpointer;
  }

  MythPointerThreadSafe & operator=(const T mythpointer)
  {
    this->m_mythpointer = mythpointer;
    return *this;
  }
};
