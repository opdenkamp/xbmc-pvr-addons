#pragma once

#include "libcmyth.h"
#include "utils/StdString.h"

class MythTimer
{
  friend class MythDatabase;
public:
  MythTimer();
  MythTimer(cmyth_timer_t cmyth_timer,bool release=true);
  int RecordID() const;
  void RecordID(int recordid);
  int ChanID() const;
  void ChanID(int chanid);
  CStdString Callsign() const;
  void Callsign(const CStdString& chancallsign);
  time_t StartTime() const;
  void StartTime(time_t starttime);
  time_t EndTime() const;
  void EndTime(time_t endtime);
  CStdString Title(bool subtitleEncoded) const;
  void Title(const CStdString& title,bool subtitleEncoded);
  CStdString Subtitle() const;
  void Subtitle(const CStdString& subtitle);
  CStdString Description() const;
  void Description(const CStdString& description);
  typedef enum TimerTypes
{
    NotRecording = 0,
    SingleRecord = 1,
    TimeslotRecord,
    ChannelRecord,
    AllRecord,
    WeekslotRecord,
    FindOneRecord,
    OverrideRecord,
    DontRecord,
    FindDailyRecord,
    FindWeeklyRecord
} TimerType;
  TimerType Type() const;
  void Type(TimerType type);
  CStdString Category() const;
  void Category(const CStdString& category);
  int StartOffset() const;
  void StartOffset(int startoffset);
  int EndOffset() const;
  void EndOffset(int endoffset);
  int Priority() const;
  void Priority(int priority);
  bool Inactive() const;
  void Inactive(bool inactive);

typedef enum TimerSearchTypes
{
    NoSearch = 0,
    PowerSearch,
    TitleSearch,
    KeywordSearch,
    PeopleSearch,
    ManualSearch
} TimerSearchType;
  TimerSearchType SearchType() const;
  void SearchType(TimerSearchType searchtype);
  typedef enum DuplicateControlMethods
  {
    CheckNone     = 0x01,
    CheckSub      = 0x02,
    CheckDesc     = 0x04,
    CheckSubDesc  = 0x06,
    CheckSubThenDesc = 0x08
  }  DuplicateControlMethod;
  typedef enum CheckDuplicatesInTypes
  {
    InRecorded     = 0x01,
    InOldRecorded  = 0x02,
    InAll          = 0x0F,
    NewEpi         = 0x10
  } CheckDuplicatesInType;

  DuplicateControlMethod DupMethod();
  void DupMethod(DuplicateControlMethod method);
  CheckDuplicatesInType CheckDupIn();
  void CheckDupIn(CheckDuplicatesInType in);
  CStdString RecGroup();
  void RecGroup(CStdString group);
  CStdString StoreGroup();
  void StoreGroup(CStdString group);
  CStdString PlayGroup();
  void PlayGroup(CStdString group);
  bool AutoTranscode();
  void AutoTranscode(bool enable);
  bool Userjob(int jobnumber);
  void Userjob(int jobnumber, bool enable);
  int Userjobs();
  void Userjobs(int jobs);
  bool AutoCommFlag();
  void AutoCommFlag(bool enable);
  bool AutoExpire();
  void AutoExpire(bool enable);
  int MaxEpisodes();
  void MaxEpisodes(int max);
  bool NewExpireOldRecord();
  void NewExpireOldRecord(bool enable);
  int Transcoder();
  void Transcoder(int transcoder);

private:
  int m_recordid;
  int m_chanid; 
  CStdString m_callsign;
  time_t m_starttime;  
  time_t m_endtime;    
	CStdString m_title;
	CStdString m_description;  
  TimerType m_type;      
	CStdString m_category;
  CStdString m_subtitle;
  int m_priority;
  int m_startoffset;
  int m_endoffset;
  TimerSearchType m_searchtype;
  bool m_inactive;

  DuplicateControlMethod m_dupmethod;
  CheckDuplicatesInType m_dupin;
  CStdString m_recgroup;
  CStdString m_storegroup;
  CStdString m_playgroup;
  bool m_autotranscode;
  int m_userjobs;
  bool m_autocommflag;
  bool m_autoexpire;
  int m_maxepisodes;
  bool m_maxnewest;
  int m_transcoder;
};