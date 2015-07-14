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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "client.h"

class CDialogRecordPref
{
public:
    static const int c_keep_all_recordings = 0;
    static const int c_default_margin = -1;

public:
    CDialogRecordPref(ADDON::CHelper_libXBMC_addon* xbmc, CHelper_libXBMC_gui* gui);
	virtual ~CDialogRecordPref();

	bool Show();
	void Close();
	int DoModal();						// returns -1=> load failed, 0=>canceled, 1=>confirmed

  // dialog specific params
	bool RecSeries;
    bool newOnly;
    bool anytime;
    int marginBefore;
    int marginAfter;
    int numberToKeep;

private:
	CAddonGUIRadioButton *_radioRecEpisode;
	CAddonGUIRadioButton *_radioRecSeries;
    CAddonGUIRadioButton *_radioNewOnly;
    CAddonGUIRadioButton *_radioAnytime;
    CAddonGUISpinControl *_marginBefore;
    CAddonGUISpinControl *_marginAfter;
    CAddonGUISpinControl *_marginNumberToKeep;

  // following is needed for every dialog
private:
    CHelper_libXBMC_gui* GUI;
    ADDON::CHelper_libXBMC_addon* XBMC;
    CAddonGUIWindow *_window;
    int _confirmed;		//-1=> load failed, 0=>canceled, 1=>confirmed

    bool OnClick(int controlId);
    bool OnFocus(int controlId);
    bool OnInit();
    bool OnAction(int actionId);

    void HideShowSeriesControls(bool bShow);
    void PopulateMarginSpin(CAddonGUISpinControl* spin);
    void PopulateKeepSpin(CAddonGUISpinControl* spin);

    static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
    static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
    static bool OnInitCB(GUIHANDLE cbhdl);
    static bool OnActionCB(GUIHANDLE cbhdl, int actionId);
};

