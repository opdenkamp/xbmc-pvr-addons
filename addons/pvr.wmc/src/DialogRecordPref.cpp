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

#define BUTTON_OK                       1
#define BUTTON_CANCEL                   2
#define BUTTON_CLOSE                   22

#define RADIO_BUTTON_EPISODE			10
#define RADIO_BUTTON_SERIES				11
#define SPIN_CONTROLRunType				12
#define SPIN_CONTROL_CHANNEL			13
#define SPIN_CONTROL_AIRTIME			14
#define LABEL_SHOWNAME					20

CDialogRecordPref::CDialogRecordPref(	bool recSeries, int runtype, bool anyChannel, bool anyTime,
										std::string currentChannelName, std::string currentAirTime, std::string showName) 
{
	RecSeries = recSeries;
	RunType = runtype;
	AnyChannel = anyChannel;
	AnyTime = anyTime;
	_currentChannel = currentChannelName;
	_currentAirTime = currentAirTime;
	_showName = showName;

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
	_window->SetControlLabel(LABEL_SHOWNAME, _showName.c_str());

	// init radio buttons
	_radioRecEpisode = GUI->Control_getRadioButton(_window, RADIO_BUTTON_EPISODE);
	_radioRecSeries = GUI->Control_getRadioButton(_window, RADIO_BUTTON_SERIES);
	_radioRecEpisode->SetText(XBMC->GetLocalizedString(30101));
	_radioRecSeries->SetText(XBMC->GetLocalizedString(30102));
	_radioRecEpisode->SetSelected(!RecSeries);
	_radioRecSeries->SetSelected(RecSeries);  

	// init runtype spin control
	_spinRunType = GUI->Control_getSpin(_window, SPIN_CONTROLRunType);
	_spinRunType->AddLabel(XBMC->GetLocalizedString(30104), 0); // any show type
	_spinRunType->AddLabel(XBMC->GetLocalizedString(30105), 1);	// first run only
	_spinRunType->AddLabel(XBMC->GetLocalizedString(30106), 2);	// live only
	_spinRunType->SetValue(RunType);

	// init channel spin control
	_spinChannel = GUI->Control_getSpin(_window, SPIN_CONTROL_CHANNEL);
	_spinChannel->AddLabel(_currentChannel.c_str(), 0);						// add current channel
	_spinChannel->AddLabel(XBMC->GetLocalizedString(30108), 1);				// add "Any channel"
	_spinChannel->SetValue(AnyChannel ? 1:0);
	
	// init airtime spin control
	_spinAirTime = GUI->Control_getSpin(_window, SPIN_CONTROL_AIRTIME);
	_spinAirTime->AddLabel(_currentAirTime.c_str(), 0);						// current air time
	_spinAirTime->AddLabel(XBMC->GetLocalizedString(30111), 1);				// "Anytime" 
	_spinAirTime->SetValue(AnyTime ? 1:0);
	
	// set visibility of spin controls based on whether dialog is set to rec a series
	_spinRunType->SetVisible(RecSeries);
	_spinChannel->SetVisible(RecSeries);
	_spinAirTime->SetVisible(RecSeries);

  return true;
}

bool CDialogRecordPref::OnClick(int controlId)
{
	switch(controlId)
	{
		case BUTTON_OK:				// save value from GUI, then FALLS THROUGH TO CANCEL
			RecSeries = _radioRecSeries->IsSelected();
			RunType = _spinRunType->GetValue();
			AnyChannel = _spinChannel->GetValue() == 1;
			AnyTime = _spinAirTime->GetValue() == 1;
		case BUTTON_CANCEL:
		case BUTTON_CLOSE:
			if (_confirmed == -1)		// if not previously confirmed, set to cancel value
				_confirmed = 0;			
			_window->Close();
			GUI->Control_releaseRadioButton(_radioRecEpisode);
			GUI->Control_releaseRadioButton(_radioRecSeries);
			GUI->Control_releaseSpin(_spinRunType);
			GUI->Control_releaseSpin(_spinChannel);
			GUI->Control_releaseSpin(_spinAirTime);
			break;
		case RADIO_BUTTON_EPISODE:
			RecSeries = !_radioRecEpisode->IsSelected();
			_radioRecSeries->SetSelected(RecSeries);
			_spinRunType->SetVisible(RecSeries);
			_spinChannel->SetVisible(RecSeries);
			_spinAirTime->SetVisible(RecSeries);
			break;
		case RADIO_BUTTON_SERIES:
			RecSeries = _radioRecSeries->IsSelected();
			_radioRecEpisode->SetSelected(!RecSeries);
			_spinRunType->SetVisible(RecSeries);
			_spinChannel->SetVisible(RecSeries);
			_spinAirTime->SetVisible(RecSeries);
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
