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

#include "vdr/osd.h"
#include "responsepacket.h"

class cxSocket;

class cVnsiOsdProvider : public cOsdProvider
{
public:
  cVnsiOsdProvider(cxSocket *socket);
  virtual ~cVnsiOsdProvider();
  virtual cOsd *CreateOsd(int Left, int Top, uint Level);
  static void SendOsdPacket(int cmd, int wnd, int color = 0, int x0 = 0, int y0 = 0, int x1 = 0, int y1 = 0, const void *data = NULL, int size = 0);
  static bool IsRequestFull();
  static void SendKey(unsigned int key);
private:
  static cResponsePacket m_OsdPacket;
  static cxSocket *m_Socket;
  static cMutex m_Mutex;
  static bool m_RequestFull;
};

