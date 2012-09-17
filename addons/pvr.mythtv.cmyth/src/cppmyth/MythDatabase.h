#pragma once

#include "libcmyth.h"
#include <vector>
#include <map>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include "utils/StdString.h"
#include "MythPointer.h"

class MythChannel;
class MythTimer;
class MythProgramInfo;

typedef cmyth_program_t MythProgram;
typedef std::pair< CStdString, std::vector< int > > MythChannelGroup;
//typedef std::vector< MythChannel > MythChannelList;

class MythRecordingProfile : public CStdString{
public:
  std::map< int, CStdString > profile;
};


class MythDatabase
{
public:
  MythDatabase();
  MythDatabase(CStdString server,CStdString database,CStdString user,CStdString password);
  bool TestConnection(CStdString &msg);
  std::map< int , MythChannel > ChannelList();
  std::vector< MythProgram > GetGuide(time_t starttime, time_t endtime);
  std::map<int, MythTimer > GetTimers();
  bool FindProgram(const time_t starttime,const int channelid,CStdString &title,MythProgram* pprogram);
  int AddTimer(MythTimer &timer);
  bool DeleteTimer(int recordid);
  bool UpdateTimer(MythTimer &timer);
  boost::unordered_map< CStdString, std::vector< int > > GetChannelGroups();
  std::map< int, std::vector< int > > SourceList();
  bool IsNull();
  std::vector<MythRecordingProfile > GetRecordingProfiles();
  int SetWatchedStatus(MythProgramInfo &recording, bool watched);
private:
  boost::shared_ptr< MythPointerThreadSafe< cmyth_database_t > > m_database_t;
};