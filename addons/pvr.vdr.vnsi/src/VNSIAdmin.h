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

#include "VNSIData.h"
#include "VNSIChannels.h"
#include "client.h"

class cOSDRender;

class cVNSIAdmin : public cVNSIData
{
public:

  cVNSIAdmin();
  ~cVNSIAdmin();

  bool Open(const std::string& hostname, int port, const char* name = "XBMC osd client");

  bool OnClick(int controlId);
  bool OnFocus(int controlId);
  bool OnInit();
  bool OnAction(int actionId);

  bool Create(int x, int y, int w, int h, void* device);
  void Render();
  void Stop();
  bool Dirty();

  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
  static bool OnInitCB(GUIHANDLE cbhdl);
  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);

  static bool CreateCB(GUIHANDLE cbhdl, int x, int y, int w, int h, void *device);
  static void RenderCB(GUIHANDLE cbhdl);
  static void StopCB(GUIHANDLE cbhdl);
  static bool DirtyCB(GUIHANDLE cbhdl);

protected:
  virtual bool OnResponsePacket(cResponsePacket* resp);
  virtual void OnDisconnect() {};
  virtual void OnReconnect() {};
  bool ConnectOSD();
  bool IsVdrAction(int action);
  bool ReadChannelList(bool radio);
  bool ReadChannelWhitelist(bool radio);
  bool ReadChannelBlacklist(bool radio);
  bool SaveChannelWhitelist(bool radio);
  bool SaveChannelBlacklist(bool radio);
  void ClearListItems();
  void LoadListItemsProviders();
  void LoadListItemsChannels();

private:

  CAddonGUIWindow *m_window;
#if defined(XBMC_GUI_API_VERSION)
  CAddonGUIRenderingControl *m_renderControl;
#endif
  CAddonGUISpinControl *m_spinTimeshiftMode;
  CAddonGUISpinControl *m_spinTimeshiftBufferRam;
  CAddonGUISpinControl *m_spinTimeshiftBufferFile;
  CAddonGUIRadioButton *m_ratioIsRadio;
  std::vector<CAddonListItem*> m_listItems;
  std::map<GUIHANDLE, int> m_listItemsMap;
  std::map<GUIHANDLE, int> m_listItemsChannelsMap;
  CVNSIChannels m_channels;
  bool m_bIsOsdControl;
  bool m_bIsOsdDirty;
  int m_width, m_height;
  int m_osdWidth, m_osdHeight;
  cOSDRender *m_osdRender;
  PLATFORM::CMutex m_osdMutex;
};
