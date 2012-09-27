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
  return CMYTH->cmyth_channel_chanid(*m_channel_t);
}

int  MythChannel::NumberInt()
{
  return CMYTH->cmyth_channel_channum(*m_channel_t);
}

CStdString MythChannel::Number()
{
  CStdString retval( CMYTH->cmyth_channel_channumstr(*m_channel_t));
  return retval;
} 

 CStdString MythChannel::Callsign()
{
  CStdString retval( CMYTH->cmyth_channel_callsign(*m_channel_t));
  return retval;
} 

int MythChannel::SourceID()
{
  return CMYTH->cmyth_channel_sourceid(*m_channel_t);
}

int MythChannel::MultiplexID()
{
  return CMYTH->cmyth_channel_multiplex(*m_channel_t);
}

CStdString  MythChannel::Name()
{
  char* cChan=CMYTH->cmyth_channel_name(*m_channel_t);
  CStdString retval(cChan);
  CMYTH->ref_release(cChan);
  return retval;
}

CStdString  MythChannel::Icon()
{
  char* cIcon=CMYTH->cmyth_channel_icon(*m_channel_t);
  CStdString retval(cIcon);
  CMYTH->ref_release(cIcon);
  return retval;
}

bool  MythChannel::Visible()
{
  return CMYTH->cmyth_channel_visible(*m_channel_t)>0;
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