//#include "client.h"
//#include "recordingRules.h"
//
//using namespace ADDON;
//
//#define BUTTON_START                    5
//#define BUTTON_BACK                     32
//#define BUTTON_CANCEL                   7
//#define HEADER_LABEL                    8
//
//#define RECRULE_NEWRULE 0
//#define RECRULE_EDIT 1
//#define RECRULE_ENABLE 2
//#define RECRULE_DELETE 3
//
//RecordingRulesWindow::RecordingRulesWindow(std::map< int, MythTimer > &recordingRules)
//  :m_window(NULL),m_list(NULL),m_recRules(recordingRules)
//{
//
//}
//
//RecordingRulesWindow::~RecordingRulesWindow()
//{
//
//}
//
//bool RecordingRulesWindow::Open()
//{
//  m_window = GUI->Window_create("RecordingRules.xml", "Confluence", false, true);
//  //m_window = GUI->Window_create("test.xml", "Confluence", false, true);
//  m_window->m_cbhdl   = this;
//  m_window->CBOnInit  = OnInitCB;
//  m_window->CBOnFocus = OnFocusCB;
//  m_window->CBOnClick = OnClickCB;
//  m_window->CBOnAction= OnActionCB;
//  m_window->CBOnContextMenu = OnContextMenuCB;
//  m_window->DoModal();
//
//  GUI->Window_destroy(m_window);
//  
//  return true;
//}
//
//
//bool RecordingRulesWindow::OnAction(int actionId)
//{
//  if (actionId == ADDON_ACTION_CLOSE_DIALOG || actionId == ADDON_ACTION_PREVIOUS_MENU)
//    OnClick(BUTTON_BACK);
//
//  return true;
//}
//
//bool RecordingRulesWindow::OnInitCB(GUIHANDLE cbhdl)
//{
//  RecordingRulesWindow* window = static_cast<RecordingRulesWindow*>(cbhdl);
//  return window->OnInit();
//}
//
//bool RecordingRulesWindow::OnClickCB(GUIHANDLE cbhdl, int controlId)
//{
//  RecordingRulesWindow* window = static_cast<RecordingRulesWindow*>(cbhdl);
//  return window->OnClick(controlId);
//}
//
//bool RecordingRulesWindow::OnFocusCB(GUIHANDLE cbhdl, int controlId)
//{
//  RecordingRulesWindow* window = static_cast<RecordingRulesWindow*>(cbhdl);
//  return window->OnFocus(controlId);
//}
//
//bool RecordingRulesWindow::OnActionCB(GUIHANDLE cbhdl, int actionId)
//{
//  RecordingRulesWindow* window = static_cast<RecordingRulesWindow*>(cbhdl);
//  return window->OnAction(actionId);
//}
//
//bool RecordingRulesWindow::OnContextMenuCB(GUIHANDLE cbhdl,int controlId,int itemNumber, unsigned int contextButtonId)
//{
//  RecordingRulesWindow* window = static_cast<RecordingRulesWindow*>(cbhdl);
//  return window->OnContextMenu(controlId, itemNumber, contextButtonId);
//}
//bool RecordingRulesWindow::OnClick(int controlId)
//{
//  if (controlId == BUTTON_BACK)
//  {
//    m_window->Close();
//
//  }
//  if (controlId == 14 )
//  {
//    int l=m_list->GetSelected();
//    XBMC->Log(LOG_DEBUG,"CLP: %i",l);
//  }
//  return true;
//}
//
//bool RecordingRulesWindow::OnFocus(int controlId)
//{
//  return true;
//}
//
//bool RecordingRulesWindow::OnInit()
//{
//
//  m_list = GUI->Control_getListContainer(m_window,14);
//  
//  AddonListItemPtr listItem(GUI->ListItem_create("Add rule...","","","",""));
//  listItem->SetProperty("time","Add rule...");
//  listItem->SetProperty("ContextMenuOverride","1401");
//  m_window->AddContextMenuButton(1401,RECRULE_NEWRULE,"New Rule");
//  m_list->AddItem(listItem.get());
//  for(std::map< int, MythTimer >::iterator it=m_recRules.begin();it!=m_recRules.end();it++)
//    m_list->AddItem(AddRecordingRule(it->second).get());
//  if(m_recRules.size())
//  {
//    //m_window->AddContextMenuButton(1401,RECRULE_SORTNAME,"Sort by: Name");
//    //m_window->AddContextMenuButton(1401,RECRULE_SORTDATE,"Sort by: Name");
//  }
//  m_window->AddContextMenuButton(14,RECRULE_NEWRULE,"New Rule");
//  m_window->AddContextMenuButton(14,RECRULE_EDIT,"Edit Rule");
//  m_window->AddContextMenuButton(14,RECRULE_ENABLE,"Rule enabled");
//  m_window->AddContextMenuButton(14,RECRULE_DELETE,"Delete");
//  return true;
//}
//
//bool RecordingRulesWindow::OnContextMenu(int controlId,int itemNumber, unsigned int contextButtonId)
//{
//  if(controlId == 14 || controlId == 1401)
//  {
//    CAddonListItem* listItem = m_list->GetItem(itemNumber);
//    CStdString t = m_recRules.at(atoi(listItem->GetLabel())).Title(false);
//    switch(contextButtonId)
//    {
//    case RECRULE_NEWRULE:      
//      XBMC->Log(LOG_DEBUG,"Item number: %i selected. Item title: %s. Button new rule pressed",itemNumber,t.c_str());
//      break;
//    case RECRULE_EDIT:
//      XBMC->Log(LOG_DEBUG,"Item number: %i selected. Button edit rule pressed",itemNumber);
//      break;
//    case RECRULE_ENABLE:
//      XBMC->Log(LOG_DEBUG,"Item number: %i selected. Button enable rule pressed",itemNumber);
//      break;
//    case RECRULE_DELETE:
//      XBMC->Log(LOG_DEBUG,"Item number: %i selected. Button delete rule pressed",itemNumber);
//      break;
//      }
//    delete listItem;
//  }
//  
//  return true;
//}
//
//CStdString DayToString(int day)
//{
//  switch(day)
//  {
//  case 0:
//    return XBMC->GetLocalizedString(19149);
//  case 1:
//    return XBMC->GetLocalizedString(19150);
//  case 2:
//    return XBMC->GetLocalizedString(19151);
//  case 3:
//    return XBMC->GetLocalizedString(19152);
//  case 4:
//    return XBMC->GetLocalizedString(19153);
//  case 5:
//    return XBMC->GetLocalizedString(19154);
//  case 6:
//    return XBMC->GetLocalizedString(19155);
//  }
//  return CStdString("");
//}
//
// 
//AddonListItemPtr RecordingRulesWindow::AddRecordingRule(MythTimer &rule)
//{
//  CStdString path;
//  CStdString id;
//  CStdString channel;
//  CStdString time;
//  CStdString search;
//  CStdString status("Enabled");
//  id.Format("%i",rule.RecordID());
//  path.Format("pvr://recordingrules/%i",rule.RecordID());
//  AddonListItemPtr listItem(GUI->ListItem_create(id.c_str(),"","","",path.c_str()));
//  if(rule.Type() == MythTimer::AllRecord || rule.ChanID()==0)
//    channel = "Any";
//  else
//    channel = rule.Callsign(); //TODO: Chan name instead??
//  
//  time_t starttime = rule.StartTime();
//  tm *lc = localtime(&starttime);
//  int shift = lc->tm_wday? lc->tm_wday-1 : 6;//Monday is the first day
//
//  switch( rule.Type()) 
//  {
//
//    case MythTimer::DontRecord:      
//    case MythTimer::OverrideRecord:
//  case MythTimer::SingleRecord:
//    time.Format("%s %s %s %s",
//      DayToString(shift),
//      XBMC->GetLocalizedDate(rule.StartTime(),false,true),
//      XBMC->GetLocalizedString(19159),
//      XBMC->GetLocalizedTime(rule.StartTime(), false)
//      );
//    
//    if(rule.SearchType() == MythTimer::ManualSearch )
//    {
//      time.Format("%s %s %s",time,
//        XBMC->GetLocalizedString(19160),
//        XBMC->GetLocalizedTime(rule.EndTime(), false));
//    }
//    break;
//  case MythTimer::FindOneRecord:
//    time="Once";
//    break;
//  case MythTimer::WeekslotRecord:
//    time.Format("%s %s %s %s %s",
//      "Every ",
//      DayToString(shift),
//      XBMC->GetLocalizedDate(rule.StartTime(),false,true),
//      XBMC->GetLocalizedString(19159),
//      XBMC->GetLocalizedTime(rule.StartTime(), false)
//      );
//    if(rule.SearchType() == MythTimer::ManualSearch )
//    {
//      time.Format("%s %s %s",time,
//        XBMC->GetLocalizedString(19160),
//        XBMC->GetLocalizedTime(rule.EndTime(), false));
//    }
//    break;
//  case MythTimer::FindWeeklyRecord:
//    time="Once every week";
//    break;
//  case MythTimer::TimeslotRecord:
//    time.Format("%s %s %s",
//      "Daily ",
//      XBMC->GetLocalizedString(19159),
//      XBMC->GetLocalizedTime(rule.StartTime(), false)
//      );
//    if(rule.SearchType() == MythTimer::ManualSearch )
//    {
//      time.Format("%s %s %s",time,
//        XBMC->GetLocalizedString(19160),
//        XBMC->GetLocalizedTime(rule.EndTime(), false));
//    }
//    break;
//    break;
//  case MythTimer::FindDailyRecord:
//    time="Once daily";
//    break;
//  case MythTimer::ChannelRecord:
//  case MythTimer::AllRecord:
//    time="Each episode";
//    break;
//  
//
//  }
//  switch( rule.SearchType() )
//  {
//  case MythTimer::NoSearch:
//    search.Format("%s",
//      rule.Title(false));
//    break;
//  case MythTimer::TitleSearch:
//    search.Format("%s %s %s",
//      XBMC->GetLocalizedString(369),//Title
//      ": ",
//      rule.Description()
//      );
//    break;
//  case MythTimer::KeywordSearch:
//    search.Format("%s %s %s",
//      XBMC->GetLocalizedString(21861), //keywords
//      ": ",
//      rule.Description()
//      );
//   break;
//   case MythTimer::PeopleSearch:
//    search.Format("%s %s %s",
//      XBMC->GetLocalizedString(21861),//Actors: 21861
//      ": ", 
//      rule.Description()
//      );
//   break;
//   case MythTimer::PowerSearch:
//    search.Format("%s %s %s %s",
//      "Powersearch: SELECT FROM ",
//      rule.Subtitle(),
//      " WHERE ",
//      rule.Description());
//    
//   break;
//  }
//  if(rule.Inactive())
//    status = "Disabled";
//  else if(rule.Type() == MythTimer::DontRecord)
//    status = "Don't record";
//  else if(rule.Type() == MythTimer::OverrideRecord)
//    status = "Override";
//  //remove newlines
//  search.Replace("\n"," ");
//  listItem->SetProperty("channel",channel.c_str());
//  listItem->SetProperty("time",time.c_str());
//  listItem->SetProperty("search",search.c_str());
//  listItem->SetProperty("status",status.c_str());
//
//  return listItem;
//}
//
