#include "MythChannel.h"
#include "../client.h"

using namespace ADDON;

/*
*								MythChannel
*/
MythChannel::MythChannel(cmyth_channel_t cmyth_channel,bool isRadio)
  : m_channel_t(new MythPointer<cmyth_channel_t>()),m_radio(isRadio)
{
  *m_channel_t=(cmyth_channel);
}


MythChannel::MythChannel()
  : m_channel_t(),m_radio(false)
{
}

int  MythChannel::ID()
{
  return CMYTH->ChannelChanid(*m_channel_t);
}

int  MythChannel::NumberInt()
{
  return CMYTH->ChannelChannum(*m_channel_t);
}

CStdString MythChannel::Number()
{
  CStdString retval( CMYTH->ChannelChannumstr(*m_channel_t));
  return retval;
} 

 CStdString MythChannel::Callsign()
{
  CStdString retval( CMYTH->ChannelCallsign(*m_channel_t));
  return retval;
} 

int MythChannel::SourceID()
{
  return CMYTH->ChannelSourceid(*m_channel_t);
}

int MythChannel::MultiplexID()
{
  return CMYTH->ChannelMultiplex(*m_channel_t);
}

CStdString  MythChannel::Name()
{
  char* cChan=CMYTH->ChannelName(*m_channel_t);
  CStdString retval(cChan);
  CMYTH->RefRelease(cChan);
  return retval;
}

CStdString  MythChannel::Icon()
{
  char* cIcon=CMYTH->ChannelIcon(*m_channel_t);
  CStdString retval(cIcon);
  CMYTH->RefRelease(cIcon);
  return retval;
}

bool  MythChannel::Visible()
{
  return CMYTH->ChannelVisible(*m_channel_t)>0;
}

bool MythChannel::IsRadio()
{
  return m_radio;
}

bool MythChannel::IsNull()
{
  if(m_channel_t==NULL)
    return true;
  return *m_channel_t==NULL;
}