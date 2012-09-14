//#include "client.h"
//#include "cppmyth/MythTimer.h"
//#include <boost/shared_ptr.hpp>
//#include <string>
//#include <map>
//
//typedef  boost::shared_ptr< CAddonListItem > AddonListItemPtr;
//
//class RecordingRulesWindow 
//{
//public:
//
//  RecordingRulesWindow(std::map< int, MythTimer > &recordingRules);
//  ~RecordingRulesWindow();
//
//  bool Open();
//
//  bool OnClick(int controlId);
//  bool OnFocus(int controlId);
//  bool OnInit();
//  bool OnAction(int actionId);
//  bool OnContextMenu(int controlId,int itemNumber, unsigned int contextButtonId); 
//
//  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
//  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
//  static bool OnInitCB(GUIHANDLE cbhdl);
//  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);
//  static bool OnContextMenuCB(GUIHANDLE cbhdl,int controlId,int itemNumber, unsigned int contextButtonId); 
// 
//private:
//  CAddonGUIWindow      *m_window;
//  CAddonGUIListContainer *m_list;
//  std::map< int, MythTimer > m_recRules;
//  AddonListItemPtr AddRecordingRule(MythTimer &rule);
//};