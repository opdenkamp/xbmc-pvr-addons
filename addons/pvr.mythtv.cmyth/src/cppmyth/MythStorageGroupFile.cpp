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

#include "MythStorageGroupFile.h"
#include "MythPointer.h"

MythStorageGroupFile::MythStorageGroupFile()
  : m_storagegroup_file_t(new MythPointer<cmyth_storagegroup_file_t>())
{
}

MythStorageGroupFile::MythStorageGroupFile(cmyth_storagegroup_file_t myth_storagegroup_file)
  : m_storagegroup_file_t(new MythPointer<cmyth_storagegroup_file_t>())
{
  *m_storagegroup_file_t = myth_storagegroup_file;
}

bool MythStorageGroupFile::IsNull() const
{
  if (m_storagegroup_file_t == NULL)
    return true;
  return *m_storagegroup_file_t == NULL;
}

CStdString MythStorageGroupFile::Filename()
{
  char* name = cmyth_storagegroup_file_get_filename(*m_storagegroup_file_t);
  CStdString retval(name);
  ref_release(name);
  return retval;
}

unsigned long long MythStorageGroupFile::Size()
{
  unsigned long long retval = cmyth_storagegroup_file_get_size(*m_storagegroup_file_t);
  return retval;
}

unsigned long MythStorageGroupFile::LastModified()
{
  unsigned long retval = cmyth_storagegroup_file_get_lastmodified(*m_storagegroup_file_t);
  return retval;
}
