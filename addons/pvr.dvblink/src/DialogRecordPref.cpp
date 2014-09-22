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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DialogRecordPref.h"
#include "libXBMC_gui.h"

using namespace ADDON;

#define BUTTON_OK                       1
#define BUTTON_CANCEL                   2
#define BUTTON_CLOSE                   22

#define RADIO_BUTTON_EPISODE			10
#define RADIO_BUTTON_SERIES				11

CDialogRecordPref::CDialogRecordPref(CHelper_libXBMC_addon* xbmc, CHelper_libXBMC_gui* gui, bool recSeries)
{
	RecSeries = recSeries;
    GUI = gui;
    XBMC = xbmc;

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
	// init radio buttons
	_radioRecEpisode = GUI->Control_getRadioButton(_window, RADIO_BUTTON_EPISODE);
	_radioRecSeries = GUI->Control_getRadioButton(_window, RADIO_BUTTON_SERIES);
	_radioRecEpisode->SetSelected(!RecSeries);
	_radioRecSeries->SetSelected(RecSeries);  

  return true;
}

bool CDialogRecordPref::OnClick(int controlId)
{
	switch(controlId)
	{
		case BUTTON_OK:				// save value from GUI, then FALLS THROUGH TO CANCEL
			RecSeries = _radioRecSeries->IsSelected();
		case BUTTON_CANCEL:
		case BUTTON_CLOSE:
			if (_confirmed == -1)		// if not previously confirmed, set to cancel value
				_confirmed = 0;			
			_window->Close();
			GUI->Control_releaseRadioButton(_radioRecEpisode);
			GUI->Control_releaseRadioButton(_radioRecSeries);
			break;
		case RADIO_BUTTON_EPISODE:
			RecSeries = !_radioRecEpisode->IsSelected();
			_radioRecSeries->SetSelected(RecSeries);
			break;
		case RADIO_BUTTON_SERIES:
			RecSeries = _radioRecSeries->IsSelected();
			_radioRecEpisode->SetSelected(!RecSeries);
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
