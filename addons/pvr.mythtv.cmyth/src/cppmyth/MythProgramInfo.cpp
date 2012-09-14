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
    char* progId=CMYTH->ProginfoProgramid(*m_proginfo_t);
    retval=progId;
    CMYTH->RefRelease(progId);
    return retval;
  }

  CStdString MythProgramInfo::Title(bool subtitleEncoded)
  {
    CStdString retval;
    char* title=CMYTH->ProginfoTitle(*m_proginfo_t);
    retval=title;
    CMYTH->RefRelease(title);
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
    char* subtitle=CMYTH->ProginfoSubtitle(*m_proginfo_t);
    retval=subtitle;
    CMYTH->RefRelease(subtitle);
    return retval;
  }

  CStdString MythProgramInfo::Path()
   {
    CStdString retval;
    
    char* path=CMYTH->ProginfoPathname(*m_proginfo_t);
 //   XBMC->Log(LOG_DEBUG,"ProgInfo path: %s, status %i",path,CMYTH->ProginfoRecStatus(*m_proginfo_t));
    retval=path;
    CMYTH->RefRelease(path);
    return retval;
  }

  CStdString MythProgramInfo::Description()
  {
    CStdString retval;
    char* desc=CMYTH->ProginfoDescription(*m_proginfo_t);
    retval=desc;
    CMYTH->RefRelease(desc);
    return retval;
  }

  CStdString MythProgramInfo::ChannelName()
    {
    CStdString retval;
    char* chan=CMYTH->ProginfoChanname(*m_proginfo_t);
    retval=chan;
    CMYTH->RefRelease(chan);
    return retval;
  }

  int  MythProgramInfo::ChannelID()
  {
    return CMYTH->ProginfoChanId(*m_proginfo_t);
  }

  unsigned long MythProgramInfo::RecordID()
  {
    return CMYTH->ProginfoRecordid(*m_proginfo_t);
  }

  time_t MythProgramInfo::RecStart()
  {
    time_t retval;
    MythTimestamp time=CMYTH->ProginfoRecStart(*m_proginfo_t);
    retval=time.UnixTime();
    return retval;
  }

  time_t MythProgramInfo::StartTime()
  {
    time_t retval;
    MythTimestamp time=CMYTH->ProginfoStart(*m_proginfo_t);
    retval=time.UnixTime();
    return retval;
  }

  time_t MythProgramInfo::EndTime()
  {
    time_t retval;
    MythTimestamp time=CMYTH->ProginfoEnd(*m_proginfo_t);
    retval=time.UnixTime();
    return retval;
  }

  int MythProgramInfo::Priority()
  {
    return CMYTH->ProginfoPriority(*m_proginfo_t);//Might want to use recpriority2 instead.
  }

  MythProgramInfo::record_status MythProgramInfo::Status()
  {
    return CMYTH->ProginfoRecStatus(*m_proginfo_t);
  }

  int MythProgramInfo::Duration()
  {
    MythTimestamp end=CMYTH->ProginfoRecEnd(*m_proginfo_t);
    MythTimestamp start=CMYTH->ProginfoRecStart(*m_proginfo_t);
    return end.UnixTime()-start.UnixTime();
    return CMYTH->ProginfoLengthSec(*m_proginfo_t);
  }

  CStdString MythProgramInfo::Category()
  {
    CStdString retval;
    char* cat=CMYTH->ProginfoCategory(*m_proginfo_t);
    retval=cat;
    CMYTH->RefRelease(cat);
    return retval;
    }

  CStdString MythProgramInfo::RecordingGroup()
  {
    CStdString retval;
    char* recgroup=CMYTH->ProginfoRecgroup(*m_proginfo_t);
    retval=recgroup;
    CMYTH->RefRelease(recgroup);
    return retval;
  }

  bool MythProgramInfo::IsWatched()
  {
    unsigned long recording_flags = CMYTH->ProginfoFlags(*m_proginfo_t);
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
