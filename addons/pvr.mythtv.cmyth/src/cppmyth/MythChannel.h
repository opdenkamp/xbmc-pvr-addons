#pragma once

#include "MythPointer.h"
#include "../libcmyth.h"
#include <boost/shared_ptr.hpp>
#include "platform/util/StdString.h"

class MythChannel
{
public:
  MythChannel();
  MythChannel(cmyth_channel_t cmyth_channel,bool isRadio);
  int ID();
  int NumberInt();
  CStdString Number();
  CStdString Callsign();
  int SourceID();
  int MultiplexID();
  CStdString Name();
  CStdString Icon();
  bool Visible();
  bool IsRadio();
  bool IsNull();
private:
  boost::shared_ptr< MythPointer< cmyth_channel_t > > m_channel_t;
  bool m_radio;
};