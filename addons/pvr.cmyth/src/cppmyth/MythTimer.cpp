#include "MythTimer.h"
#include "../client.h"

using namespace ADDON;

/*
 *              MythTimer
 */


MythTimer::MythTimer()
  : m_recordid(-1),m_chanid(-1),m_callsign(""),m_starttime(0),m_endtime(0),m_title(""),m_description(""),m_type(NotRecording),m_category(""),m_subtitle(""),m_priority(0),m_startoffset(0),m_endoffset(0),m_searchtype(NoSearch),m_inactive(true),
    m_dupmethod(CheckSubDesc), m_dupin(InAll), m_recgroup("Default"), m_storegroup("Default"), m_playgroup("Default"), m_autotranscode(false), m_userjobs(0), m_autocommflag(false), m_autoexpire(false), m_maxepisodes(0), m_maxnewest(0), m_transcoder(0)
{}


  MythTimer::MythTimer(cmyth_timer_t cmyth_timer,bool release)
    : m_recordid(cmyth_timer_recordid(cmyth_timer)),
    m_chanid(cmyth_timer_chanid(cmyth_timer)),
    m_callsign(""),
    m_starttime(cmyth_timer_starttime(cmyth_timer)),
    m_endtime(cmyth_timer_endtime(cmyth_timer)),
    m_title(""),
    m_description(""),
    m_type(static_cast<TimerType>(cmyth_timer_type(cmyth_timer))),
    m_category(""),
    m_subtitle(""),
    m_priority(cmyth_timer_priority(cmyth_timer)),
    m_startoffset(cmyth_timer_startoffset(cmyth_timer)),
    m_endoffset(cmyth_timer_endoffset(cmyth_timer)),
    m_searchtype(static_cast<TimerSearchType>(cmyth_timer_searchtype(cmyth_timer))),
    m_inactive(cmyth_timer_inactive(cmyth_timer)!=0),
    m_dupmethod(static_cast< DuplicateControlMethods >(cmyth_timer_dup_method(cmyth_timer))),
    m_dupin(static_cast< CheckDuplicatesInTypes >(cmyth_timer_dup_in(cmyth_timer))),
    m_recgroup(""),
    m_storegroup(""),
    m_playgroup(""),
    m_autotranscode(cmyth_timer_autotranscode(cmyth_timer) == 1),
    m_userjobs(cmyth_timer_userjobs(cmyth_timer)),
    m_autocommflag(cmyth_timer_autocommflag(cmyth_timer) == 1),
    m_autoexpire(cmyth_timer_autoexpire(cmyth_timer) == 1),
    m_maxepisodes(cmyth_timer_maxepisodes(cmyth_timer)),
    m_maxnewest(cmyth_timer_maxnewest(cmyth_timer) == 1),
    m_transcoder(cmyth_timer_transcoder(cmyth_timer))
  {
    char *title = cmyth_timer_title(cmyth_timer);
    char *description = cmyth_timer_description(cmyth_timer);
    char *category = cmyth_timer_category(cmyth_timer);
    char *subtitle = cmyth_timer_subtitle(cmyth_timer);
    char *callsign = cmyth_timer_callsign(cmyth_timer);
    char *recgroup = cmyth_timer_rec_group(cmyth_timer);
    char *storegroup = cmyth_timer_store_group(cmyth_timer);
    char *playgroup = cmyth_timer_play_group(cmyth_timer);

    m_title = title;
    m_description = description;
    m_category = category;
    m_subtitle = subtitle;
    m_callsign = callsign;
    m_recgroup = recgroup;
    m_storegroup = storegroup;
    m_playgroup = playgroup;

    ref_release(title);
    ref_release(description);
    ref_release(category);
    ref_release(subtitle);
    ref_release(callsign);
    ref_release(recgroup);
    ref_release(storegroup);
    ref_release(playgroup);
    if(release)
      ref_release(cmyth_timer);
  }

  int MythTimer::RecordID() const
  {
    return m_recordid;
  }

  void MythTimer::RecordID(int recordid)
  {
    m_recordid=recordid;
  }

  int MythTimer::ChanID() const
  {
    return m_chanid;
  }

  void MythTimer::ChanID(int chanid)
  {
    m_chanid = chanid;
  }

  CStdString MythTimer::Callsign() const
  {
    return m_callsign;
  }

  void MythTimer::Callsign(const CStdString& channame)
  {
    m_callsign = channame;
  }

  time_t MythTimer::StartTime() const
  {
    return m_starttime;
  }

  void MythTimer::StartTime(time_t starttime)
  {
    m_starttime=starttime;
  }

  time_t MythTimer::EndTime() const
  {
    return m_endtime;
  }

  void MythTimer::EndTime(time_t endtime)
  {
    m_endtime=endtime;
  }

  CStdString MythTimer::Title(bool subtitleEncoded) const
  {
    if(subtitleEncoded && !m_subtitle.empty())
    {
      CStdString retval;
      retval.Format("%s::%s",m_title,m_subtitle);
      return retval;
    }
    return m_title;
  }

  void MythTimer::Title(const CStdString& title,bool subtitleEncoded)
  {
    if(subtitleEncoded)
    {
      size_t seppos = title.find("::");
      if(seppos != CStdString::npos)
      {
        m_title = title.substr(0,seppos);
        m_subtitle = title.substr(seppos+2);
        return;
      }
    }
    m_title=title;
  }

  CStdString MythTimer::Subtitle() const
  {
    return m_subtitle;
  }

  void MythTimer::Subtitle(const CStdString& subtitle)
  {
    m_subtitle=subtitle;
  }

  CStdString MythTimer::Description() const
   {
    return m_description;
  }

   void MythTimer::Description(const CStdString& description)
   {
     m_description=description;
   }

  MythTimer::TimerType MythTimer::Type() const
  {
    return m_type;
  }

   void MythTimer::Type(MythTimer::TimerType type)
   {
     m_type=type;
   }

  CStdString MythTimer::Category() const
  {
    return m_category;
  }

  void MythTimer::Category(const CStdString& category)
  {
    m_category=category;
  }

  int MythTimer::StartOffset() const
  {
    return m_startoffset;
  }
  
    void MythTimer::StartOffset(int startoffset)
    {
      m_startoffset=startoffset;
    }

  int MythTimer::EndOffset() const
  {
    return m_endoffset;
  }

    void MythTimer::EndOffset(int endoffset)
    {
      m_endoffset=endoffset;
    }

  int MythTimer::Priority() const
  {
    return m_priority;
  }

    void MythTimer::Priority(int priority)
    {
      m_priority=priority;
    }

  bool MythTimer::Inactive() const
  {
    return m_inactive;
  }

  void MythTimer::Inactive(bool inactive)
  {
    m_inactive=inactive;
  }

  MythTimer::TimerSearchType MythTimer::SearchType() const
  {
    return m_searchtype;
  }

    void MythTimer::SearchType(MythTimer::TimerSearchType searchtype)
    {
      m_searchtype=searchtype;
    }

   MythTimer::DuplicateControlMethod  MythTimer::DupMethod()
   {
     return m_dupmethod;
   }

  void  MythTimer::DupMethod( MythTimer::DuplicateControlMethod method)
  {
    m_dupmethod = method;
  }

  MythTimer::CheckDuplicatesInType  MythTimer::CheckDupIn()
  {
    return m_dupin;
  }
  
  void  MythTimer::CheckDupIn( MythTimer::CheckDuplicatesInType in)
  {
    m_dupin = in;
  }

  CStdString  MythTimer::RecGroup()
  {
    return m_recgroup;
  }

  void  MythTimer::RecGroup(CStdString group)
  {
    m_recgroup = group;
  }

  CStdString  MythTimer::StoreGroup()
  {
    return m_storegroup;
  }

  void  MythTimer::StoreGroup(CStdString group)
  {
    m_storegroup = group;
  }

  CStdString  MythTimer::PlayGroup()
  {
    return m_playgroup;
  }

  void  MythTimer::PlayGroup(CStdString group)
  {
    m_playgroup = group;
  }

  bool  MythTimer::AutoTranscode()
  {
    return m_autotranscode;
  }

  void  MythTimer::AutoTranscode(bool enable)
  {
    m_autotranscode = enable;
  }

  bool  MythTimer::Userjob(int jobnumber)
  {
    if(jobnumber<1 || jobnumber > 4)
      return false;
    return (m_userjobs&(1<<(jobnumber-1))) == 1 ;
  }

  void  MythTimer::Userjob(int jobnumber, bool enable)
  {
    if(jobnumber<1 || jobnumber > 4)
      return;
    if(enable)
      m_userjobs |= 1 << (jobnumber - 1);
    else
      m_userjobs &= ~(1 << (jobnumber - 1) );
  }

  int MythTimer::Userjobs()
  {
    return m_userjobs;
  }

  void MythTimer::Userjobs(int jobs)
  {
    m_userjobs = jobs;
  }

  bool  MythTimer::AutoCommFlag()
  {
    return m_autocommflag;
  }
  
  void  MythTimer::AutoCommFlag(bool enable)
  {
    m_autocommflag = enable;
  }

  bool  MythTimer::AutoExpire()
  {
    return m_autoexpire;
  }

  void  MythTimer::AutoExpire(bool enable)
  {
    m_autoexpire = enable;
  }

  int  MythTimer::MaxEpisodes()
  {
    return m_maxepisodes;
  }

  void  MythTimer::MaxEpisodes(int max)
  {
    m_maxepisodes = max;
  }

  bool  MythTimer::NewExpireOldRecord()
  {
    return m_maxnewest;
  }

  void  MythTimer::NewExpireOldRecord(bool enable)
  {
    m_maxnewest = enable;
  }

  int MythTimer::Transcoder()
  {
    return m_transcoder;
  }

  void MythTimer::Transcoder(int transcoder)
  {
    m_transcoder = transcoder;
  }
