/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "client.h"
#include "GUIDialogRecordSettings.h"
#include "libXBMC_gui.h"
#include "timers.h"
#include "utils.h"
#include "DateTime.h"

/* Dialog item identifiers */
#define BUTTON_OK                       1
#define BUTTON_CANCEL                   2

#define SPIN_CONTROL_FREQUENCY         10
#define SPIN_CONTROL_AIRTIME           11
#define SPIN_CONTROL_CHANNELS          12
#define SPIN_CONTROL_KEEP              13
#define SPIN_CONTROL_PRERECORD         14
#define SPIN_CONTROL_POSTRECORD        15

#define LABEL_PROGRAM_TITLE            20
#define LABEL_PROGRAM_START_TIME       21
#define LABEL_PROGRAM_CHANNEL          22

using namespace std;
using namespace MPTV;

CGUIDialogRecordSettings::CGUIDialogRecordSettings(const PVR_TIMER &timerinfo, cTimer& timer, const std::string& channelName) :
  m_spinFrequency(NULL),
  m_spinAirtime(NULL),
  m_spinChannels(NULL),
  m_spinKeep(NULL),
  m_spinPreRecord(NULL),
  m_spinPostRecord(NULL),
	m_frequency(Once),
  m_airtime(ThisTime),
  m_channels(ThisChannel),
  m_timerinfo(timerinfo),
  m_timer(timer)
{
  CDateTime startTime(m_timerinfo.startTime);
  CDateTime endTime(m_timerinfo.endTime);
  startTime.GetAsLocalizedTime(m_startTime);
  startTime.GetAsLocalizedDate(m_startDate);
  endTime.GetAsLocalizedTime(m_endTime);

  m_title = m_timerinfo.strTitle;
  m_channel = channelName;

  // needed for every dialog
  m_retVal = -1;				// init to failed load value (due to xml file not being found)
  // Default skin should actually be "skin.confluence", but the fallback mechanism will only
  // find the xml file and not the used image files. This will result in a transparent window
  // which is basically useless. Therefore, it is better to let the dialog fail by using the
  // incorrect fallback skin name "Confluence"
  m_window = GUI->Window_create("DialogRecordSettings.xml", "Confluence", false, true);
  if (m_window)
  {
    m_window->m_cbhdl = this;
    m_window->CBOnInit = OnInitCB;
    m_window->CBOnFocus = OnFocusCB;
    m_window->CBOnClick = OnClickCB;
    m_window->CBOnAction = OnActionCB;
  }
}

CGUIDialogRecordSettings::~CGUIDialogRecordSettings()
{
  GUI->Window_destroy(m_window);
}


bool CGUIDialogRecordSettings::OnInit()
{
	// Display the recording details in the window
  m_window->SetControlLabel(LABEL_PROGRAM_TITLE, m_title.c_str());
  string strTimeSlot = m_startDate + " " + m_startTime + " - " + m_endTime;
  m_window->SetControlLabel(LABEL_PROGRAM_START_TIME, strTimeSlot.c_str());
  m_window->SetControlLabel(LABEL_PROGRAM_CHANNEL, m_channel.c_str());

  // Init spin controls
  m_spinFrequency = GUI->Control_getSpin(m_window, SPIN_CONTROL_FREQUENCY);
  m_spinAirtime = GUI->Control_getSpin(m_window, SPIN_CONTROL_AIRTIME);
  m_spinChannels = GUI->Control_getSpin(m_window, SPIN_CONTROL_CHANNELS);
  m_spinKeep = GUI->Control_getSpin(m_window, SPIN_CONTROL_KEEP);
  m_spinPreRecord = GUI->Control_getSpin(m_window, SPIN_CONTROL_PRERECORD);
  m_spinPostRecord = GUI->Control_getSpin(m_window, SPIN_CONTROL_POSTRECORD);

  if (!m_spinFrequency || !m_spinAirtime || !m_spinChannels || !m_spinKeep || !m_spinPreRecord || !m_spinPostRecord)
    return false;

  // Populate Frequency spin control
  for (int i = 0; i < 5; i++)
  { // show localized recording options
    m_spinFrequency->AddLabel(XBMC->GetLocalizedString(30110 + i), i);
  }
  // set the default value
  m_spinFrequency->SetValue(CGUIDialogRecordSettings::Once);

  // Populate Airtime spin control
  string strThisTime = XBMC->GetLocalizedString(30120);
  strThisTime += "(" + m_startTime + ")";
  m_spinAirtime->AddLabel(strThisTime.c_str(), CGUIDialogRecordSettings::ThisTime);
  m_spinAirtime->AddLabel(XBMC->GetLocalizedString(30121), CGUIDialogRecordSettings::AnyTime);
  // Set the default values
  m_spinAirtime->SetValue(CGUIDialogRecordSettings::ThisTime);
  m_spinAirtime->SetVisible(false);

  // Populate Channels spin control
  for (int i = 0; i < 2; i++)
  { // show localized recording options
    m_spinChannels->AddLabel(XBMC->GetLocalizedString(30125 + i), i);
  }
  // Set the default values
  m_spinChannels->SetValue(CGUIDialogRecordSettings::ThisChannel);
  m_spinChannels->SetVisible(false);

  // Populate Keep spin control
  for (int i = 0; i < 4; i++)
  { // show localized recording options
    m_spinKeep->AddLabel(XBMC->GetLocalizedString(30130 + i), i);
  }
  // Set the default values
  m_spinKeep->SetValue(TvDatabase::Always);

  // Populate PreRecord spin control
  CStdString marginStart;
  marginStart.Format("%d (%s)", m_timerinfo.iMarginStart, XBMC->GetLocalizedString(30136));
  m_spinPreRecord->AddLabel(XBMC->GetLocalizedString(30135), -1);
  m_spinPreRecord->AddLabel(marginStart.c_str(), m_timerinfo.iMarginStart); //value from XBMC
  m_spinPreRecord->SetValue(m_timerinfo.iMarginStart);  // Set the default value
  m_spinPreRecord->AddLabel("0", 0);
  m_spinPreRecord->AddLabel("3", 3);
  m_spinPreRecord->AddLabel("5", 5);
  m_spinPreRecord->AddLabel("7", 7);
  m_spinPreRecord->AddLabel("10", 10);
  m_spinPreRecord->AddLabel("15", 15);

  // Populate PostRecord spin control
  CStdString marginEnd;
  marginEnd.Format("%d (%s)", m_timerinfo.iMarginEnd, XBMC->GetLocalizedString(30136));
  m_spinPostRecord->AddLabel(XBMC->GetLocalizedString(30135), -1);
  m_spinPostRecord->AddLabel(marginEnd.c_str(), m_timerinfo.iMarginEnd); //value from XBMC
  m_spinPostRecord->SetValue(m_timerinfo.iMarginEnd);   // Set the default value
  m_spinPostRecord->AddLabel("0", 0);
  m_spinPostRecord->AddLabel("3", 3);
  m_spinPostRecord->AddLabel("5", 5);
  m_spinPostRecord->AddLabel("7", 7);
  m_spinPostRecord->AddLabel("10", 10);
  m_spinPostRecord->AddLabel("15", 15);
  m_spinPostRecord->AddLabel("20", 20);
  m_spinPostRecord->AddLabel("30", 30);
  m_spinPostRecord->AddLabel("45", 45);
  m_spinPostRecord->AddLabel("60", 60);

  return true;
}

bool CGUIDialogRecordSettings::OnClick(int controlId)
{
  switch(controlId)
  {
    case BUTTON_OK:				// save value from GUI, then FALLS THROUGH TO CANCEL
      m_frequency = (RecordingFrequency) m_spinFrequency->GetValue();
      m_airtime = (RecordingAirtime) m_spinAirtime->GetValue();
      m_channels = (RecordingChannels) m_spinChannels->GetValue();

      /* Update the Timer settings */
      UpdateTimerSettings();

      m_retVal = 1;
      Close();
      break;
    case BUTTON_CANCEL:
      m_retVal = 0;
      Close();
      break;
    /* Limit the available options based on the SPIN settings
    * MediaPortal does not support all combinations
    */
    case SPIN_CONTROL_FREQUENCY:
      m_frequency = (RecordingFrequency) m_spinFrequency->GetValue();

      switch (m_frequency)
      {
        case Once:
        case Weekends:
        case WeekDays:
          m_spinAirtime->SetVisible(false);
          m_spinChannels->SetVisible(false);
          break;
        case Weekly:
          m_spinAirtime->SetVisible(true);
          m_spinChannels->SetVisible(false);
          break;
        case Daily:
          m_spinAirtime->SetVisible(true);
          m_spinChannels->SetVisible(true);
          break;
      }
      break;
    case SPIN_CONTROL_CHANNELS:
      m_channels = (RecordingChannels) m_spinChannels->GetValue();

      //switch (m_frequency)
      //{
      //  case Once:
      //  case Weekends:
      //  case WeekDays:
      //  case Weekly:
      //    m_channels = ThisChannel;
      //    m_spinChannels->SetValue(m_channels);
      //    break;
      //}

      /* This time on any channel is not supported by MediaPortal */
      if (m_channels == AnyChannel)
        m_spinAirtime->SetValue(AnyTime);
      break;
    case SPIN_CONTROL_AIRTIME:
      m_airtime = (RecordingAirtime) m_spinAirtime->GetValue();

      if (m_airtime == ThisTime)
        m_spinChannels->SetValue(ThisChannel);
      break;
  }

  return true;
}

bool CGUIDialogRecordSettings::OnInitCB(GUIHANDLE cbhdl)
{
  CGUIDialogRecordSettings* dialog = static_cast<CGUIDialogRecordSettings*>(cbhdl);
  return dialog->OnInit();
}

bool CGUIDialogRecordSettings::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogRecordSettings* dialog = static_cast<CGUIDialogRecordSettings*>(cbhdl);
  return dialog->OnClick(controlId);
}

bool CGUIDialogRecordSettings::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogRecordSettings* dialog = static_cast<CGUIDialogRecordSettings*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CGUIDialogRecordSettings::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  CGUIDialogRecordSettings* dialog = static_cast<CGUIDialogRecordSettings*>(cbhdl);
  return dialog->OnAction(actionId);
}

bool CGUIDialogRecordSettings::Show()
{
  if (m_window)
    return m_window->Show();

  return false;
}

void CGUIDialogRecordSettings::Close()
{
  if (m_window)
  {
    GUI->Control_releaseSpin(m_spinFrequency);
    GUI->Control_releaseSpin(m_spinAirtime);
    GUI->Control_releaseSpin(m_spinChannels);
    GUI->Control_releaseSpin(m_spinKeep);
    GUI->Control_releaseSpin(m_spinPreRecord);
    GUI->Control_releaseSpin(m_spinPostRecord);
    m_window->Close();
  }
}

int CGUIDialogRecordSettings::DoModal()
{
  if (m_window)
    m_window->DoModal();
  return m_retVal;
}


bool CGUIDialogRecordSettings::OnFocus(int controlId)
{
  return true;
}

/*
 * This callback is called by XBMC before executing its internal OnAction() function
 * Returning "true" tells XBMC that we already handled the action, returing "false"
 * passes action to the XBMC internal OnAction() function
 */
bool CGUIDialogRecordSettings::OnAction(int actionId)
{
  //XBMC->Log(ADDON::LOG_DEBUG, "%s: action = %i\n", __FUNCTION__, actionId);
  if (actionId == ADDON_ACTION_CLOSE_DIALOG || actionId == ADDON_ACTION_PREVIOUS_MENU || actionId == 92 /* Back */)
    return OnClick(BUTTON_CANCEL);
  else
    /* return false to tell XBMC that it should take over */
    return false;
}

void CGUIDialogRecordSettings::UpdateTimerSettings(void)
{
  switch(m_frequency)
  {
    case Once:
      m_timer.SetScheduleRecordingType(TvDatabase::Once);
      break;
    case Weekends:
      m_timer.SetScheduleRecordingType(TvDatabase::Weekends);
      break;
    case WeekDays:
      m_timer.SetScheduleRecordingType(TvDatabase::WorkingDays);
      break;
    case Weekly:
      if (m_airtime == ThisTime)
        m_timer.SetScheduleRecordingType(TvDatabase::Weekly);
      else
        m_timer.SetScheduleRecordingType(TvDatabase::WeeklyEveryTimeOnThisChannel);
      break;
    case Daily:
      switch (m_airtime)
      {
        case ThisTime:
          m_timer.SetScheduleRecordingType(TvDatabase::Daily);
          break;
        case AnyTime:
          if (m_channels == ThisChannel)
            m_timer.SetScheduleRecordingType(TvDatabase::EveryTimeOnThisChannel);
          else
            m_timer.SetScheduleRecordingType(TvDatabase::EveryTimeOnEveryChannel);
          break;
      }
  }

  m_timer.SetKeepMethod((TvDatabase::KeepMethodType)  m_spinKeep->GetValue());
  m_timer.SetPreRecordInterval(m_spinPreRecord->GetValue());
  m_timer.SetPostRecordInterval(m_spinPostRecord->GetValue());
}
