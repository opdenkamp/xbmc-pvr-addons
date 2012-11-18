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

#include "config.h"
#include "vnsiosd.h"
#include "vnsicommand.h"
#include "responsepacket.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <vdr/tools.h>
#include <vdr/remote.h>
#include "cxsocket.h"

#define ACTION_NONE                    0
#define ACTION_MOVE_LEFT               1
#define ACTION_MOVE_RIGHT              2
#define ACTION_MOVE_UP                 3
#define ACTION_MOVE_DOWN               4
#define ACTION_SELECT_ITEM             7
#define ACTION_PREVIOUS_MENU          10

#define REMOTE_0                    58  // remote keys 0-9. are used by multiple windows
#define REMOTE_1                    59  // for example in videoFullScreen.xml window id=2005 you can
#define REMOTE_2                    60  // enter time (mmss) to jump to particular point in the movie
#define REMOTE_3                    61
#define REMOTE_4                    62  // with spincontrols you can enter 3digit number to quickly set
#define REMOTE_5                    63  // spincontrol to desired value
#define REMOTE_6                    64
#define REMOTE_7                    65
#define REMOTE_8                    66
#define REMOTE_9                    67
#define ACTION_NAV_BACK             92

#define ACTION_TELETEXT_RED           215 // Teletext Color buttons to control TopText
#define ACTION_TELETEXT_GREEN         216 //    "       "      "    "     "       "
#define ACTION_TELETEXT_YELLOW        217 //    "       "      "    "     "       "
#define ACTION_TELETEXT_BLUE          218 //    "       "      "    "     "       "


// --- cVnsiOsd -----------------------------------------------------------

#define MAXNUMWINDOWS 7 // OSD windows are counted 0...6
#define MAXOSDMEMORY  92000 // number of bytes available to the OSD (for unmodified DVB cards)

class cVnsiOsd : public cOsd
{
private:
  int osdMem;
  bool shown;
  void Cmd(int cmd, int wnd, int color = 0, int x0 = 0, int y0 = 0, int x1 = 0, int y1 = 0, const void *data = NULL, int size = 0);
protected:
  virtual void SetActive(bool On);
public:
  cVnsiOsd(int Left, int Top, uint Level);
  virtual ~cVnsiOsd();
  virtual eOsdError CanHandleAreas(const tArea *Areas, int NumAreas);
  virtual eOsdError SetAreas(const tArea *Areas, int NumAreas);
  virtual void Flush(void);
};

cVnsiOsd::cVnsiOsd(int Left, int Top, uint Level)
:cOsd(Left, Top, Level)
{
  shown = false;
  osdMem = MAXOSDMEMORY;
  DEBUGLOG("osd created. left: %d, top: %d", Left, Top);
}

cVnsiOsd::~cVnsiOsd()
{
  SetActive(false);
}

void cVnsiOsd::SetActive(bool On)
{
  if (On != Active())
  {
    cOsd::SetActive(On);
    if (On)
    {
      // must clear all windows here to avoid flashing effects - doesn't work if done
      // in Flush() only for the windows that are actually used...
      for (int i = 0; i < MAXNUMWINDOWS; i++)
      {
        Cmd(VNSI_OSD_CLEAR, i);
      }
      if (GetBitmap(0)) // only flush here if there are already bitmaps
        Flush();
    }
    else if (shown)
    {
      for (int i = 0; GetBitmap(i); i++)
      {
        Cmd(VNSI_OSD_CLOSE, i);
      }
      shown = false;
    }
  }
}

eOsdError cVnsiOsd::CanHandleAreas(const tArea *Areas, int NumAreas)
{
  eOsdError Result = cOsd::CanHandleAreas(Areas, NumAreas);
  if (Result == oeOk)
  {
    if (NumAreas > MAXNUMWINDOWS)
      return oeTooManyAreas;
    int TotalMemory = 0;
    for (int i = 0; i < NumAreas; i++)
    {
      if (Areas[i].bpp != 1 && Areas[i].bpp != 2 && Areas[i].bpp != 4 && Areas[i].bpp != 8)
        return oeBppNotSupported;
      if ((Areas[i].Width() & (8 / Areas[i].bpp - 1)) != 0)
        return oeWrongAlignment;
      if (Areas[i].Width() < 1 || Areas[i].Height() < 1 || Areas[i].Width() > 720 || Areas[i].Height() > 576)
        return oeWrongAreaSize;
      TotalMemory += Areas[i].Width() * Areas[i].Height() / (8 / Areas[i].bpp);
    }
    if (TotalMemory > osdMem)
      return oeOutOfMemory;
  }
  return Result;
}

eOsdError cVnsiOsd::SetAreas(const tArea *Areas, int NumAreas)
{
  if (shown)
  {
    for (int i = 0; GetBitmap(i); i++)
    {
      Cmd(VNSI_OSD_CLOSE, i);
    }
    shown = false;
  }
  return cOsd::SetAreas(Areas, NumAreas);
}

void cVnsiOsd::Cmd(int cmd, int wnd, int color, int x0, int y0, int x1, int y1, const void *data, int size)
{
  DEBUGLOG("OSD: cmd: %d, wnd: %d, color: %d, x0: %d, y0: %d, x1: %d, y1: %d",
      cmd, wnd, color, x0, y0, x1, y1);
  cVnsiOsdProvider::SendOsdPacket(cmd, wnd, color, x0, y0, x1, y1, data, size);
}

void cVnsiOsd::Flush(void)
{
  if (!Active())
     return;
  cBitmap *Bitmap;
  bool full = cVnsiOsdProvider::IsRequestFull();
  for (int i = 0; (Bitmap = GetBitmap(i)) != NULL; i++)
  {
    uint8_t reset = !shown || full;
    Cmd(VNSI_OSD_OPEN, i, Bitmap->Bpp(), Left() + Bitmap->X0(), Top() + Bitmap->Y0(), Left() + Bitmap->X0() + Bitmap->Width() - 1, Top() + Bitmap->Y0() + Bitmap->Height() - 1, (void *)&reset, 1);
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    if (!shown || Bitmap->Dirty(x1, y1, x2, y2) || full)
    {
      if (!shown || full)
      {
        x1 = y1 = 0;
        x2 = Bitmap->Width() - 1;
        y2 = Bitmap->Height() - 1;
      }
      // commit colors:
      int NumColors;
      const tColor *Colors = Bitmap->Colors(NumColors);
      if (Colors)
      {
        Cmd(VNSI_OSD_SETPALETTE, i, 0, NumColors, 0, 0, 0, Colors, NumColors*sizeof(tColor));
      }
      // commit modified data:
      int size = (y2-y1) * Bitmap->Width() + (x2-x1+1);
      Cmd(VNSI_OSD_SETBLOCK, i, Bitmap->Width(), x1, y1, x2, y2, Bitmap->Data(x1, y1), size);
    }
    Bitmap->Clean();
  }
  if (!shown)
  {
    // Showing the windows in a separate loop to avoid seeing them come up one after another
    for (int i = 0; (Bitmap = GetBitmap(i)) != NULL; i++)
    {
      Cmd(VNSI_OSD_MOVEWINDOW, i, 0, Left() + Bitmap->X0(), Top() + Bitmap->Y0());
    }
    shown = true;
  }
}

// --- cVnsiOsdProvider -------------------------------------------------------

cResponsePacket cVnsiOsdProvider::m_OsdPacket;
cxSocket *cVnsiOsdProvider::m_Socket;
cMutex cVnsiOsdProvider::m_Mutex;
bool cVnsiOsdProvider::m_RequestFull;

cVnsiOsdProvider::cVnsiOsdProvider(cxSocket *socket)
{
  cMutexLock lock(&m_Mutex);
  INFOLOG("new osd provider");
  m_Socket = socket;
  m_RequestFull = true;
}

cVnsiOsdProvider::~cVnsiOsdProvider()
{
  cMutexLock lock(&m_Mutex);
  m_Socket = NULL;
}

cOsd *cVnsiOsdProvider::CreateOsd(int Left, int Top, uint Level)
{
  cMutexLock lock(&m_Mutex);
  return new cVnsiOsd(Left, Top, Level);
}

void cVnsiOsdProvider::SendOsdPacket(int cmd, int wnd, int color, int x0, int y0, int x1, int y1, const void *data, int size)
{
  cMutexLock lock(&m_Mutex);
  if (!m_Socket)
    return;

  if (!m_OsdPacket.initOsd(cmd, wnd, color, x0, y0, x1, y1))
  {
    ERRORLOG("OSD response packet init fail");
    return;
  }
  m_OsdPacket.setLen(m_OsdPacket.getOSDHeaderLength() + size);
  m_OsdPacket.finaliseOSD();

  m_Socket->write(m_OsdPacket.getPtr(), m_OsdPacket.getOSDHeaderLength(), -1, true);
  if (size)
    m_Socket->write(data, size);
}

bool cVnsiOsdProvider::IsRequestFull()
{
  cMutexLock lock(&m_Mutex);
  bool ret = m_RequestFull;
  m_RequestFull = false;
  return ret;
}

void cVnsiOsdProvider::SendKey(unsigned int key)
{
  if (!cOsd::IsOpen())
  {
    cRemote::Put(kMenu);
    return;
  }
  switch (key)
  {
  case ACTION_MOVE_UP:
    cRemote::Put(kUp);
    break;
  case ACTION_MOVE_DOWN:
    cRemote::Put(kDown);
    break;
  case ACTION_MOVE_LEFT:
    cRemote::Put(kLeft);
    break;
  case ACTION_MOVE_RIGHT:
    cRemote::Put(kRight);
    break;
  case ACTION_PREVIOUS_MENU:
    cRemote::Put(kMenu);
    break;
  case ACTION_NAV_BACK:
    cRemote::Put(kBack);
    break;
  case ACTION_SELECT_ITEM:
    cRemote::Put(kOk);
    break;
  case ACTION_TELETEXT_RED:
    cRemote::Put(kRed);
    break;
  case ACTION_TELETEXT_BLUE:
    cRemote::Put(kBlue);
    break;
  case ACTION_TELETEXT_YELLOW:
    cRemote::Put(kYellow);
    break;
  case ACTION_TELETEXT_GREEN:
    cRemote::Put(kGreen);
    break;
  case REMOTE_0:
    cRemote::Put(k0);
    break;
  case REMOTE_1:
    cRemote::Put(k1);
    break;
  case REMOTE_2:
    cRemote::Put(k2);
    break;
  case REMOTE_3:
    cRemote::Put(k3);
    break;
  case REMOTE_4:
    cRemote::Put(k4);
    break;
  case REMOTE_5:
    cRemote::Put(k5);
    break;
  case REMOTE_6:
    cRemote::Put(k6);
    break;
  case REMOTE_7:
    cRemote::Put(k7);
    break;
  case REMOTE_8:
    cRemote::Put(k8);
    break;
  case REMOTE_9:
    cRemote::Put(k9);
    break;
  default:
    break;
  }
}
