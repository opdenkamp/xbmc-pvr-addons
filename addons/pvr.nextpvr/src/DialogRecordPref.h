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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include "pvrclient-nextpvr.h"
#include <vector>

//struct TimerInfo
//{
//	bool		isSeries;
//	int			runType;
//	bool		anyChannel;
//	bool		anyTime;
//	std::string currentChannelName;
//	std::string currentAirTime;
//	std::string showName;
//}

class CDialogRecordPref
{

public:
  CDialogRecordPref(std::string showName, std::string showDescription, int iDefaultPrePadding, int iDefaultPostPadding, std::string recordingDirectories);
	virtual ~CDialogRecordPref();

	bool Show();
	void Close();
	int DoModal();						// returns -1=> load failed, 0=>canceled, 1=>confirmed
	//bool LoadFailed();					

  // dialog specific params
  int RecordingType;
  int Keep;
  int PrePadding;
  int PostPadding;
  std::string RecordingDirectory;
private:
  std::string _currentChannel;		// these are just used for dialog display
  std::string _currentAirTime;
  std::string _showName;
  std::string _showDescription;
  std::vector<std::string> _recordingDirectories;

private:
  CAddonGUISpinControl *_spinRecordType;
  CAddonGUISpinControl *_spinPrePadding;
  CAddonGUISpinControl *_spinPostPadding;
  CAddonGUISpinControl *_spinKeepCount;
  CAddonGUISpinControl *_spinRecordingDirectory;

  // following is needed for every dialog
private:
  CAddonGUIWindow *_window;
  int _confirmed;		//-1=> load failed, 0=>canceled, 1=>confirmed

  bool OnClick(int controlId);
  bool OnFocus(int controlId);
  bool OnInit();
  bool OnAction(int actionId);

  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
  static bool OnInitCB(GUIHANDLE cbhdl);
  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);


};

