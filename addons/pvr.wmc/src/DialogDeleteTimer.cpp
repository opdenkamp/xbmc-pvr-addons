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

#include "DialogDeleteTimer.h"
#include "libXBMC_gui.h"

#define BUTTON_OK                       1
#define BUTTON_CANCEL                   2
#define BUTTON_CLOSE                   22

#define RADIO_BUTTON_EPISODE			10
#define RADIO_BUTTON_SERIES				11
#define LABEL_SHOWNAME					20


CDialogDeleteTimer::CDialogDeleteTimer(	bool delSeries, std::string showName) 
{
	DeleteSeries = delSeries;
	_showName = showName;

	// specify the xml of the window here
	_confirmed = -1;
	_window = GUI->Window_create("DeleteTimer.xml", "Confluence", false, true);

	// needed for every dialog
	_window->m_cbhdl = this;
	_window->CBOnInit = OnInitCB;
	_window->CBOnFocus = OnFocusCB;
	_window->CBOnClick = OnClickCB;
	_window->CBOnAction = OnActionCB;
}

CDialogDeleteTimer::~CDialogDeleteTimer()
{
  GUI->Window_destroy(_window);
}

bool CDialogDeleteTimer::OnInit()
{
	// display the show name in the window
	_window->SetControlLabel(LABEL_SHOWNAME, _showName.c_str());

	// init radio buttons
	_radioDelEpisode = GUI->Control_getRadioButton(_window, RADIO_BUTTON_EPISODE);
	_radioDelSeries = GUI->Control_getRadioButton(_window, RADIO_BUTTON_SERIES);
	_radioDelEpisode->SetText(XBMC->GetLocalizedString(30121));
	_radioDelSeries->SetText(XBMC->GetLocalizedString(30122));  
	_radioDelEpisode->SetSelected(!DeleteSeries);
	_radioDelSeries->SetSelected(DeleteSeries);  

	return true;
}

bool CDialogDeleteTimer::OnClick(int controlId)
{
	switch(controlId)
	{
		case BUTTON_OK:				// save value from GUI, then FALLS THROUGH TO CANCEL
			DeleteSeries = _radioDelSeries->IsSelected();
		case BUTTON_CANCEL:			// handle releasing of objects
		case BUTTON_CLOSE:
			if (_confirmed == -1)	// if not previously confirmed, set to cancel value
				_confirmed = 0;			
			_window->Close();
			GUI->Control_releaseRadioButton(_radioDelEpisode);
			GUI->Control_releaseRadioButton(_radioDelSeries);
			break;
		case RADIO_BUTTON_EPISODE:
			DeleteSeries = !_radioDelEpisode->IsSelected();
			_radioDelSeries->SetSelected(DeleteSeries);
			break;
		case RADIO_BUTTON_SERIES:
			DeleteSeries = _radioDelSeries->IsSelected();
			_radioDelEpisode->SetSelected(!DeleteSeries);
			break;
	}

  return true;
}

bool CDialogDeleteTimer::OnInitCB(GUIHANDLE cbhdl)
{
  CDialogDeleteTimer* dialog = static_cast<CDialogDeleteTimer*>(cbhdl);
  return dialog->OnInit();
}

bool CDialogDeleteTimer::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
	CDialogDeleteTimer* dialog = static_cast<CDialogDeleteTimer*>(cbhdl);
	if (controlId == BUTTON_OK)
		dialog->_confirmed = 1;
	//dialog->_confirmed = (controlId == BUTTON_OK);
	return dialog->OnClick(controlId);
}

bool CDialogDeleteTimer::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  CDialogDeleteTimer* dialog = static_cast<CDialogDeleteTimer*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CDialogDeleteTimer::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  CDialogDeleteTimer* dialog = static_cast<CDialogDeleteTimer*>(cbhdl);
  return dialog->OnAction(actionId);
}

bool CDialogDeleteTimer::Show()
{
  if (_window)
    return _window->Show();

  return false;
}

void CDialogDeleteTimer::Close()
{
  if (_window)
    _window->Close();
}

int CDialogDeleteTimer::DoModal()
{
  if (_window)
    _window->DoModal();
  return _confirmed;		// return true if user didn't cancel dialog
}

bool CDialogDeleteTimer::OnFocus(int controlId)
{
  return true;
}

bool CDialogDeleteTimer::OnAction(int actionId)
{
  if (actionId == ADDON_ACTION_CLOSE_DIALOG || actionId == ADDON_ACTION_PREVIOUS_MENU || actionId == 92/*back*/)
    return OnClick(BUTTON_CANCEL);
  else
    return false;
}
