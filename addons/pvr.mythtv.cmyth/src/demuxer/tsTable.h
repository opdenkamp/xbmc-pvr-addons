/*
 *      Copyright (C) 2013 Jean-Luc Barriere
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef TSTABLE_H
#define TSTABLE_H

#include "common.h"

#define TABLE_BUFFER_SIZE       1024

class TSTable
{
public:
  unsigned char buf[TABLE_BUFFER_SIZE];
  uint8_t table_id;
  uint8_t version;
  uint16_t id;
  uint16_t len;
  uint16_t offset;

  TSTable(void)
  : table_id(0xff)
  , version(0xff)
  , id(0xffff)
  , len(0)
  , offset(0)
  {
    memset(buf, 0, sizeof(buf));
  }

  void Reset(void)
  {
    len = 0;
    offset = 0;
  }
};

#endif /* TSTABLE_H */
