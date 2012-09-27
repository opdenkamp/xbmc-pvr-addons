#include "MythTimestamp.h"
#include "../client.h"

using namespace ADDON;


/*
 *            MythTimestamp
 */


MythTimestamp::MythTimestamp()
  :m_timestamp_t()
{
}

MythTimestamp::MythTimestamp(cmyth_timestamp_t cmyth_timestamp)
  :m_timestamp_t(new MythPointer<cmyth_timestamp_t>())
{
  *m_timestamp_t=(cmyth_timestamp);
}

  MythTimestamp::MythTimestamp(CStdString time,bool datetime)
 :m_timestamp_t(new MythPointer<cmyth_timestamp_t>())
{
  (void)datetime;
  *m_timestamp_t=(/*datetime?CMYTH->DatetimeFromString(time.Buffer()):*/CMYTH->cmyth_timestamp_from_string(time.Buffer()));
}
 
  MythTimestamp::MythTimestamp(time_t time)
   :m_timestamp_t(new MythPointer<cmyth_timestamp_t>())
{
  *m_timestamp_t=(CMYTH->cmyth_timestamp_from_unixtime(time));
}

  bool MythTimestamp::operator==(const MythTimestamp &other)
  {
    return CMYTH->cmyth_timestamp_compare(*m_timestamp_t,*other.m_timestamp_t)==0;
  }

  bool MythTimestamp::operator>(const MythTimestamp &other)
  {
    return CMYTH->cmyth_timestamp_compare(*m_timestamp_t,*other.m_timestamp_t)==1;
  }

  bool MythTimestamp::operator<(const MythTimestamp &other)
  {
    return CMYTH->cmyth_timestamp_compare(*m_timestamp_t,*other.m_timestamp_t)==-1;
  }

  time_t MythTimestamp::UnixTime()
  {
    return CMYTH->cmyth_timestamp_to_unixtime(*m_timestamp_t);
  }

  CStdString MythTimestamp::String()
  {
    CStdString retval;
    char time[25];
    bool succeded=CMYTH->cmyth_timestamp_to_string(time,*m_timestamp_t)==0;
    retval=succeded?time:"";
    return retval;
  }

  CStdString MythTimestamp::Isostring()
  {
    CStdString retval;
    char time[25];
    bool succeded=CMYTH->cmyth_timestamp_to_isostring(time,*m_timestamp_t)==0;
    retval=succeded?time:"";
    return retval;
  }

  CStdString MythTimestamp::Displaystring(bool use12hClock)
  {
    CStdString retval;
    char time[25];
    bool succeded=CMYTH->cmyth_timestamp_to_display_string(time,*m_timestamp_t,use12hClock)==0;
    retval=succeded?time:"";
    return retval;
  }

  bool MythTimestamp::IsNull()
  {
    if(m_timestamp_t==NULL)
      return true;
    return *m_timestamp_t==NULL;
  }

