/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
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

#include "DialogRecordPref.h"
#include "libXBMC_gui.h"

#include <sstream>
#include <iostream>

#define BUTTON_OK                       1
#define BUTTON_CANCEL                   2
#define BUTTON_CLOSE                    22

#define RADIO_BUTTON_EPISODE            10
#define RADIO_BUTTON_SERIES             11
#define SPIN_CONTROLRunType             12
#define SPIN_CONTROL_CHANNEL            13
#define SPIN_CONTROL_AIRTIME            14
#define LABEL_SHOW_NAME                 20
#define SPIN_CONTROL_RECORD_TYPE        21
#define SPIN_CONTROL_KEEP_COUNT         23
#define LABEL_SHOW_DESCRIPTION          24
#define SPIN_CONTROL_PRE_PADDING        25
#define SPIN_CONTROL_POST_PADDING       26
#define SPIN_CONTROL_RECORDING_DIR      27


CDialogRecordPref::CDialogRecordPref(std::string showName, std::string showDescription, int iPrePadding, int iPostPadding, std::string recordingDirectories) 
{
  _showName = showName;
  _showDescription = showDescription;

  PrePadding = iPrePadding;
  PostPadding = iPostPadding;
  RecordingDirectory = recordingDirectories;

  // needed for every dialog
  _confirmed = -1;				// init to failed load value (due to xml file not being found)
  _window = GUI->Window_create("RecordPrefs.xml", "Confluence", false, true);
  _window->m_cbhdl = this;
  _window->CBOnInit = OnInitCB;
  _window->CBOnFocus = OnFocusCB;
  _window->CBOnClick = OnClickCB;
  _window->CBOnAction = OnActionCB;
}

CDialogRecordPref::~CDialogRecordPref()
{
  GUI->Window_destroy(_window);
}


bool CDialogRecordPref::OnInit()
{
  // display the show name in the window
  _window->SetControlLabel(LABEL_SHOW_NAME, _showName.c_str());
  _window->SetControlLabel(LABEL_SHOW_DESCRIPTION, _showDescription.c_str());

  // init record-type
  _spinRecordType = GUI->Control_getSpin(_window, SPIN_CONTROL_RECORD_TYPE);
  _spinRecordType->AddLabel(XBMC->GetLocalizedString(30121), 0); // record-once
  _spinRecordType->AddLabel(XBMC->GetLocalizedString(30122), 1); // record-season (new episodes)
  _spinRecordType->AddLabel(XBMC->GetLocalizedString(30123), 2); // record-season (all episodes)
  _spinRecordType->AddLabel(XBMC->GetLocalizedString(30124), 3); // record-season (daily, this timeslot)
  _spinRecordType->AddLabel(XBMC->GetLocalizedString(30125), 4); // record-season (weekly, this timeslot)
  _spinRecordType->AddLabel(XBMC->GetLocalizedString(30126), 5); // record-season (mon-fri, this timeslot)
  _spinRecordType->AddLabel(XBMC->GetLocalizedString(30127), 6); // record-season (weekends, this timeslot)
  _spinRecordType->SetValue(0);

  // init keep count
  _spinKeepCount = GUI->Control_getSpin(_window, SPIN_CONTROL_KEEP_COUNT);
  _spinKeepCount->AddLabel(XBMC->GetLocalizedString(30131), 0); // keep all recordings
  for (int i=1; i<31; i++)
  {
    char text[20];
    sprintf(text, "%d", i);
    _spinKeepCount->AddLabel(text, i); // keep all recordings
  }
  _spinKeepCount->SetValue(0);

  // init padding
  _spinPrePadding = GUI->Control_getSpin(_window, SPIN_CONTROL_PRE_PADDING);
  _spinPostPadding = GUI->Control_getSpin(_window, SPIN_CONTROL_POST_PADDING);
  for (int i=0; i<90; i++)
  {
    char text[20];
    sprintf(text, "%d", i);
    _spinPrePadding->AddLabel(text, i);
    _spinPostPadding->AddLabel(text, i);
  }
  _spinPrePadding->SetValue(PrePadding);
  _spinPostPadding->SetValue(PostPadding);

  // recording directories
  _spinRecordingDirectory = GUI->Control_getSpin(_window, SPIN_CONTROL_RECORDING_DIR);
  _spinRecordingDirectory->AddLabel(XBMC->GetLocalizedString(30135), 0); // Default
  std::istringstream directories(RecordingDirectory);
  std::string directory;
  int index = 0;
  while (std::getline(directories, directory, ','))
  {
    _spinRecordingDirectory->AddLabel(directory.c_str(), index++);
    _recordingDirectories.push_back(directory);
  }

  return true;
}

bool CDialogRecordPref::OnClick(int controlId)
{
  switch(controlId)
  {
    case BUTTON_OK:				// save value from GUI, then FALLS THROUGH TO CANCEL
      RecordingType = _spinRecordType->GetValue();
      Keep = _spinKeepCount->GetValue();
      PrePadding = _spinPrePadding->GetValue();
      PostPadding = _spinPostPadding->GetValue();
      RecordingDirectory = "[";
      RecordingDirectory +=_recordingDirectories[_spinRecordingDirectory->GetValue()];
      RecordingDirectory += "]";
    case BUTTON_CANCEL:
    case BUTTON_CLOSE:
      if (_confirmed == -1)// if not previously confirmed, set to cancel value
        _confirmed = 0;
      _window->Close();
      GUI->Control_releaseSpin(_spinRecordType);
      GUI->Control_releaseSpin(_spinKeepCount);
      break;
  }

  return true;
}

bool CDialogRecordPref::OnInitCB(GUIHANDLE cbhdl)
{
  CDialogRecordPref* dialog = static_cast<CDialogRecordPref*>(cbhdl);
  return dialog->OnInit();
}

bool CDialogRecordPref::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
	CDialogRecordPref* dialog = static_cast<CDialogRecordPref*>(cbhdl);
	if (controlId == BUTTON_OK)
		dialog->_confirmed = 1;
	//dialog->_confirmed = (controlId == BUTTON_OK);
	return dialog->OnClick(controlId);
}

bool CDialogRecordPref::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  CDialogRecordPref* dialog = static_cast<CDialogRecordPref*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CDialogRecordPref::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  CDialogRecordPref* dialog = static_cast<CDialogRecordPref*>(cbhdl);
  return dialog->OnAction(actionId);
}

bool CDialogRecordPref::Show()
{
  if (_window)
    return _window->Show();

  return false;
}

void CDialogRecordPref::Close()
{
  if (_window)
    _window->Close();
}

int CDialogRecordPref::DoModal()
{
  if (_window)
    _window->DoModal();
  return _confirmed;		// return true if user didn't cancel dialog
}


bool CDialogRecordPref::OnFocus(int controlId)
{
  return true;
}

bool CDialogRecordPref::OnAction(int actionId)
{
  if (actionId == ADDON_ACTION_CLOSE_DIALOG || actionId == ADDON_ACTION_PREVIOUS_MENU || actionId == 92/*back*/)
    return OnClick(BUTTON_CANCEL);
  else
    return false;
}

