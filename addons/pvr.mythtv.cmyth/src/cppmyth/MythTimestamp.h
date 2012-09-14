#pragma once

#include "../libcmyth.h"
#include "platform/util/StdString.h"
#include <boost/shared_ptr.hpp>
#include "MythPointer.h"

class MythTimestamp
{
public:
  MythTimestamp();
  MythTimestamp(cmyth_timestamp_t cmyth_timestamp);
  MythTimestamp(CStdString time,bool datetime);
  MythTimestamp(time_t time);
  bool operator==(const MythTimestamp &other);
  bool operator!=(const MythTimestamp &other){return !(*this == other);}
  bool operator>(const MythTimestamp &other);
  bool operator>=(const MythTimestamp &other){return (*this == other||*this > other);}
  bool operator<(const MythTimestamp &other);
  bool operator<=(const MythTimestamp &other){return (*this == other||*this < other);}
  time_t UnixTime();
  CStdString String();
  CStdString Isostring();
  CStdString Displaystring(bool use12hClock);
  bool IsNull();
private:
  boost::shared_ptr< MythPointer< cmyth_timestamp_t > > m_timestamp_t;
};
