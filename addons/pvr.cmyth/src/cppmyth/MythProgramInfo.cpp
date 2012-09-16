#include "MythProgramInfo.h"
#include "MythTimestamp.h"
#include "../client.h"

using namespace ADDON;

/*
 *            MythProgramInfo
 */

MythProgramInfo::MythProgramInfo()
  :m_proginfo_t()
{
}

MythProgramInfo::MythProgramInfo(cmyth_proginfo_t cmyth_proginfo)
  :m_proginfo_t(new MythPointer<cmyth_proginfo_t>())
{
  *m_proginfo_t=cmyth_proginfo;

}

  CStdString MythProgramInfo::ProgramID()
  {
    CStdString retval;
    char* progId=cmyth_proginfo_programid(*m_proginfo_t);
    retval=progId;
    ref_release(progId);
    return retval;
  }

  CStdString MythProgramInfo::Title(bool subtitleEncoded)
  {
    CStdString retval;
    char* title=cmyth_proginfo_title(*m_proginfo_t);
    retval=title;
    ref_release(title);
    if(subtitleEncoded)
    {
      CStdString subtitle = this->Subtitle();
      if(!subtitle.empty())
        retval.Format("%s::%s",retval,subtitle);
    }
    return retval;
  }

  CStdString MythProgramInfo::Subtitle()
  {
    CStdString retval;
    char* subtitle=cmyth_proginfo_subtitle(*m_proginfo_t);
    retval=subtitle;
    ref_release(subtitle);
    return retval;
  }

  CStdString MythProgramInfo::Path()
   {
    CStdString retval;
    
    char* path=cmyth_proginfo_pathname(*m_proginfo_t);
 //   XBMC->Log(LOG_DEBUG,"ProgInfo path: %s, status %i",path,CMYTH->ProginfoRecStatus(*m_proginfo_t));
    retval=path;
    ref_release(path);
    return retval;
  }

  CStdString MythProgramInfo::Description()
  {
    CStdString retval;
    char* desc=cmyth_proginfo_description(*m_proginfo_t);
    retval=desc;
    ref_release(desc);
    return retval;
  }

  CStdString MythProgramInfo::ChannelName()
    {
    CStdString retval;
    char* chan=cmyth_proginfo_channame(*m_proginfo_t);
    retval=chan;
    ref_release(chan);
    return retval;
  }

  int  MythProgramInfo::ChannelID()
  {
    return cmyth_proginfo_chan_id(*m_proginfo_t);
  }

  unsigned long MythProgramInfo::RecordID()
  {
    return cmyth_proginfo_recordid(*m_proginfo_t);
  }

  time_t MythProgramInfo::RecStart()
  {
    time_t retval;
    MythTimestamp time=cmyth_proginfo_rec_start(*m_proginfo_t);
    retval=time.UnixTime();
    return retval;
  }

  time_t MythProgramInfo::StartTime()
  {
    time_t retval;
    MythTimestamp time=cmyth_proginfo_start(*m_proginfo_t);
    retval=time.UnixTime();
    return retval;
  }

  time_t MythProgramInfo::EndTime()
  {
    time_t retval;
    MythTimestamp time=cmyth_proginfo_end(*m_proginfo_t);
    retval=time.UnixTime();
    return retval;
  }

  int MythProgramInfo::Priority()
  {
    return cmyth_proginfo_priority(*m_proginfo_t);//Might want to use recpriority2 instead.
  }

  MythProgramInfo::record_status MythProgramInfo::Status()
  {
    return cmyth_proginfo_rec_status(*m_proginfo_t);
  }

  int MythProgramInfo::Duration()
  {
    MythTimestamp end=cmyth_proginfo_rec_end(*m_proginfo_t);
    MythTimestamp start=cmyth_proginfo_rec_start(*m_proginfo_t);
    return end.UnixTime()-start.UnixTime();
    return cmyth_proginfo_length_sec(*m_proginfo_t);
  }

  CStdString MythProgramInfo::Category()
  {
    CStdString retval;
    char* cat=cmyth_proginfo_category(*m_proginfo_t);
    retval=cat;
    ref_release(cat);
    return retval;
    }

  CStdString MythProgramInfo::RecordingGroup()
  {
    CStdString retval;
    char* recgroup=cmyth_proginfo_recgroup(*m_proginfo_t);
    retval=recgroup;
    ref_release(recgroup);
    return retval;
  }

  bool MythProgramInfo::IsWatched()
  {
    unsigned long recording_flags = cmyth_proginfo_flags(*m_proginfo_t);
    return recording_flags & 0x00000200; // FL_WATCHED
  }

  long long MythProgramInfo::uid()
     {
       long long retval=RecStart();
       retval<<=32;
       retval+=ChannelID();
       if(retval>0)
         retval=-retval;
       return retval;
     }
  bool MythProgramInfo::IsNull()
  {
    if(m_proginfo_t==NULL)
      return true;
    return *m_proginfo_t==NULL;
  }
