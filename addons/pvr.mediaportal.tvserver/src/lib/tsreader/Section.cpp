/* 
 *  Copyright (C) 2006-2010 Team MediaPortal
 *  http://www.team-mediaportal.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "os-dependent.h"
#include "Section.h"

CSection::CSection(void)
{
  Reset();
}

CSection::~CSection(void)
{
}

void CSection::Reset()
{
  table_id = -1;
  table_id_extension = -1;
  section_length = -1;
  section_number = -1;
  version_number = -1;
  section_syntax_indicator = -1;
  BufferPos = 0;
}

CSection& CSection::operator = (const CSection &section)
{
  if (&section == this)
  {
    return *this;
  }
  Copy(section);
  return *this;
}


void CSection::Copy(const CSection &section)
{
  table_id = section.table_id;
  table_id_extension = section.table_id_extension;
  section_length = section.section_length;
  section_number = section.section_number;
  version_number = section.version_number;
  section_syntax_indicator = section.section_syntax_indicator;
  memcpy(Data, section.Data, sizeof(Data));
  BufferPos = 0;
}

int CSection::CalcSectionLength(byte* tsPacket,int start)
{
  if(start >= 188)
    return 0;

  if (BufferPos < 3)
  {
    byte bHi = 0;
    byte bLow = 0;
    if (BufferPos == 1)
    {
      bHi = tsPacket[start];
      bLow = tsPacket[start+1];
    }
    else if (BufferPos == 2)
    {
      bHi = Data[1];
      bLow = tsPacket[start];
    }
    section_length=(int)(((bHi & 0xF) << 8) + bLow);
  }
  else
    section_length = (int)(((Data[1] & 0xF) << 8) + Data[2]);
  return section_length;
}

bool CSection::DecodeHeader()
{
  if (BufferPos < 8)
    return false;

  table_id = Data[0];
  section_syntax_indicator = ((Data[1] >> 7) & 1);

  if (section_length == -1)
    section_length = (((Data[1] & 0xF) << 8) + Data[2]);

  table_id_extension = ((Data[3] << 8) +Data[4]);
  version_number = ((Data[5] >> 1) & 0x1F);
  section_number = Data[6];
  section_syntax_indicator = ((Data[1] >> 7) & 1);

  return true;
}

bool CSection::SectionComplete()
{
  if (!DecodeHeader() && BufferPos-3 > section_length && section_length>0)
    return true;
  if (!DecodeHeader())
    return false;
  return (BufferPos-3 >= section_length);
}
