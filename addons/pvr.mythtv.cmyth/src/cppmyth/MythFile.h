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

#include "MythConnection.h"

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

class MythFile
{
public:
  MythFile();
  MythFile(cmyth_file_t myth_file, MythConnection conn);

  bool IsNull() const;

  unsigned long long Length();
  void UpdateLength(unsigned long long length);

  int Read(void *buffer, unsigned long length);
  long long Seek(long long offset, int whence);
  unsigned long long Position();

private:
  boost::shared_ptr<MythPointer<cmyth_file_t> > m_file_t;
  MythConnection m_conn;
};
