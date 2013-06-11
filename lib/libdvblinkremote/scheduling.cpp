/***************************************************************************
 * Copyright (C) 2012 Marcus Efraimsson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

#include "scheduling.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

Schedule::Schedule()
{

}

Schedule::Schedule(const DVBLinkScheduleType scheduleType, const std::string& channelId, const int recordingsToKeep) 
  : m_scheduleType(scheduleType), 
    m_channelId(channelId), 
    RecordingsToKeep(recordingsToKeep)
{ 
  m_id = "";
  UserParameter = "";
  ForceAdd = false;
}

Schedule::Schedule(const DVBLinkScheduleType scheduleType, const std::string& id, const std::string& channelId, const int recordingsToKeep) 
  : m_scheduleType(scheduleType), 
    m_id(id),
    m_channelId(channelId), 
    RecordingsToKeep(recordingsToKeep)
{ 
  UserParameter = "";
  ForceAdd = false;
}

Schedule::~Schedule()
{

}

std::string& Schedule::GetID() 
{ 
  return m_id; 
}

std::string& Schedule::GetChannelID() 
{ 
  return m_channelId; 
}

Schedule::DVBLinkScheduleType& Schedule::GetScheduleType() 
{ 
  return m_scheduleType; 
}

ManualSchedule::ManualSchedule(const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title)
  : Schedule(SCHEDULE_TYPE_MANUAL, channelId), 
    m_startTime(startTime), 
    m_duration(duration), 
    m_dayMask(dayMask), 
    Title(title)
{

}

ManualSchedule::ManualSchedule(const std::string& id, const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title)
  : Schedule(SCHEDULE_TYPE_MANUAL, id, channelId), 
    m_startTime(startTime), 
    m_duration(duration), 
    m_dayMask(dayMask), 
    Title(title)
{

}

ManualSchedule::~ManualSchedule()
{

}

long ManualSchedule::GetStartTime() 
{ 
  return m_startTime; 
}

long ManualSchedule::GetDuration() 
{ 
  return m_duration; 
}
    
long ManualSchedule::GetDayMask() 
{ 
  return m_dayMask; 
}

EpgSchedule::EpgSchedule(const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, const bool recordSeriesAnytime)
  : Schedule(SCHEDULE_TYPE_BY_EPG, channelId), 
    m_programId(programId), 
    Repeat(repeat), 
    NewOnly(newOnly), 
    RecordSeriesAnytime(recordSeriesAnytime)
{

}

EpgSchedule::EpgSchedule(const std::string& id, const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, const bool recordSeriesAnytime)
  : Schedule(SCHEDULE_TYPE_BY_EPG, id, channelId), 
    m_programId(programId), 
    Repeat(repeat), 
    NewOnly(newOnly), 
    RecordSeriesAnytime(recordSeriesAnytime)
{

}

EpgSchedule::~EpgSchedule()
{

}

std::string& EpgSchedule::GetProgramID() 
{ 
  return m_programId; 
}

AddScheduleRequest::AddScheduleRequest()
{

}

AddScheduleRequest::~AddScheduleRequest()
{

}


AddManualScheduleRequest::AddManualScheduleRequest(const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title)
  : ManualSchedule(channelId, startTime, duration, dayMask, title), AddScheduleRequest(), Schedule(Schedule::SCHEDULE_TYPE_MANUAL, channelId)
{

}

AddManualScheduleRequest::~AddManualScheduleRequest()
{

}

AddScheduleByEpgRequest::AddScheduleByEpgRequest(const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, const bool recordSeriesAnytime)
  : EpgSchedule(channelId, programId, repeat, newOnly, recordSeriesAnytime), AddScheduleRequest(), Schedule(Schedule::SCHEDULE_TYPE_BY_EPG, channelId)
{

}

AddScheduleByEpgRequest::~AddScheduleByEpgRequest()
{

}

bool AddScheduleRequestSerializer::WriteObject(std::string& serializedData, AddScheduleRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("schedule");
  
  if (!objectGraph.UserParameter.empty()) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "user_param", objectGraph.UserParameter));
  }

  if (objectGraph.ForceAdd) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "force_add", objectGraph.ForceAdd));
  }

  if (objectGraph.GetScheduleType() == objectGraph.SCHEDULE_TYPE_MANUAL) {
    AddManualScheduleRequest& addManualScheduleRequest = (AddManualScheduleRequest&)objectGraph;

    tinyxml2::XMLElement* manualElement = GetXmlDocument().NewElement("manual");
    rootElement->InsertEndChild(manualElement);

    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_id", addManualScheduleRequest.GetChannelID()));

    if (!addManualScheduleRequest.Title.empty()) {
      manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "title", addManualScheduleRequest.Title));
    }

    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "start_time", addManualScheduleRequest.GetStartTime()));
    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "duration", addManualScheduleRequest.GetDuration()));
    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "day_mask", addManualScheduleRequest.GetDayMask()));
    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recordings_to_keep", addManualScheduleRequest.RecordingsToKeep));
  }
    
  if (objectGraph.GetScheduleType() == objectGraph.SCHEDULE_TYPE_BY_EPG) {
    AddScheduleByEpgRequest& addScheduleByEpgRequest = (AddScheduleByEpgRequest&)objectGraph;

    tinyxml2::XMLElement* byEpgElement = GetXmlDocument().NewElement("by_epg");
    rootElement->InsertEndChild(byEpgElement);
    byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_id", addScheduleByEpgRequest.GetChannelID()));
    byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "program_id", addScheduleByEpgRequest.GetProgramID()));

    if (addScheduleByEpgRequest.Repeat) {
      byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "repeat", addScheduleByEpgRequest.Repeat));
    }

    if (addScheduleByEpgRequest.NewOnly) {
      byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "new_only", addScheduleByEpgRequest.NewOnly));
    }

    if (addScheduleByEpgRequest.RecordSeriesAnytime) {
      byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "record_series_anytime", addScheduleByEpgRequest.RecordSeriesAnytime));
    }

    byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recordings_to_keep", addScheduleByEpgRequest.RecordingsToKeep));
  }    

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

GetSchedulesRequest::GetSchedulesRequest()
{

}

GetSchedulesRequest::~GetSchedulesRequest()
{

}

bool GetSchedulesRequestSerializer::WriteObject(std::string& serializedData, GetSchedulesRequest& objectGraph)
{
  PrepareXmlDocumentForObjectSerialization("schedules");    
  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

StoredManualSchedule::StoredManualSchedule(const std::string& id, const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title)
  : ManualSchedule(id, channelId, startTime, duration, dayMask, title), Schedule(Schedule::SCHEDULE_TYPE_MANUAL,id, channelId)
{

}

StoredManualSchedule::~StoredManualSchedule()
{ }

StoredManualScheduleList::~StoredManualScheduleList()
{
  for (std::vector<StoredManualSchedule*>::const_iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

StoredEpgSchedule::StoredEpgSchedule(const std::string& id, const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, const bool recordSeriesAnytime)
  : EpgSchedule(id, channelId, programId, repeat, newOnly, recordSeriesAnytime), Schedule(Schedule::SCHEDULE_TYPE_BY_EPG,id, channelId)
{

}

StoredEpgSchedule::~StoredEpgSchedule()
{ }

StoredEpgScheduleList::~StoredEpgScheduleList()
{
  for (std::vector<StoredEpgSchedule*>::const_iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

StoredSchedules::StoredSchedules()
{
  m_manualScheduleList = new StoredManualScheduleList();
  m_epgScheduleList = new StoredEpgScheduleList();
}

StoredSchedules::~StoredSchedules() 
{
  if (m_manualScheduleList) {
    delete m_manualScheduleList;
  }

  if (m_epgScheduleList) {
    delete m_epgScheduleList;
  }
}

StoredManualScheduleList& StoredSchedules::GetManualSchedules() 
{
  return *m_manualScheduleList;
}

StoredEpgScheduleList& StoredSchedules::GetEpgSchedules() 
{
  return *m_epgScheduleList;
}

bool GetSchedulesResponseSerializer::ReadObject(StoredSchedules& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("schedules");
    GetSchedulesResponseXmlDataDeserializer* xmlDataDeserializer = new GetSchedulesResponseXmlDataDeserializer(*this, object);
    elRoot->Accept(xmlDataDeserializer);
    delete xmlDataDeserializer;
    
    return true;
  }

  return false;
}

GetSchedulesResponseSerializer::GetSchedulesResponseXmlDataDeserializer::GetSchedulesResponseXmlDataDeserializer(GetSchedulesResponseSerializer& parent, StoredSchedules& storedSchedules) 
  : m_parent(parent), 
    m_storedSchedules(storedSchedules) 
{ }

GetSchedulesResponseSerializer::GetSchedulesResponseXmlDataDeserializer::~GetSchedulesResponseXmlDataDeserializer()
{ }

bool GetSchedulesResponseSerializer::GetSchedulesResponseXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
  if (strcmp(element.Name(), "schedule") == 0) 
  {    	
    std::string scheduleId = Util::GetXmlFirstChildElementText(&element, "schedule_id");
    std::string userParam = Util::GetXmlFirstChildElementText(&element, "user_param");
    bool forceadd = Util::GetXmlFirstChildElementTextAsBoolean(&element, "force_add");

    if (m_parent.HasChildElement(element, "by_epg")) {
      tinyxml2::XMLElement* epg = (tinyxml2::XMLElement*)(&element)->FirstChildElement("by_epg");

      std::string channelid = Util::GetXmlFirstChildElementText(epg, "channel_id");
      std::string programid = Util::GetXmlFirstChildElementText(epg, "program_id");		  
      
      StoredEpgSchedule* s = new StoredEpgSchedule(scheduleId, channelid, programid);
      s->ForceAdd = forceadd;
      s->UserParameter = userParam;
      
      if (m_parent.HasChildElement(*epg, "repeat")) {
        s->Repeat = Util::GetXmlFirstChildElementTextAsBoolean(epg, "repeat");
      }

      if (m_parent.HasChildElement(*epg, "new_only")) {
        s->NewOnly = Util::GetXmlFirstChildElementTextAsBoolean(epg, "new_only");
      }

      if (m_parent.HasChildElement(*epg, "record_series_anytime")) {
        s->RecordSeriesAnytime = Util::GetXmlFirstChildElementTextAsBoolean(epg, "record_series_anytime");
      }

      s->RecordingsToKeep = Util::GetXmlFirstChildElementTextAsInt(epg, "recordings_to_keep");

      m_storedSchedules.GetEpgSchedules().push_back(s);
    }
    else if (m_parent.HasChildElement(element, "manual")) {
      tinyxml2::XMLElement* manual = (tinyxml2::XMLElement*)(&element)->FirstChildElement("manual");

      std::string channelId = Util::GetXmlFirstChildElementText(manual, "channel_id");
      std::string title = Util::GetXmlFirstChildElementText(manual, "title");
      long startTime = Util::GetXmlFirstChildElementTextAsLong(manual, "start_time");
      int duration = Util::GetXmlFirstChildElementTextAsLong(manual, "duration");
      long dayMask = Util::GetXmlFirstChildElementTextAsLong(manual, "day_mask");

      StoredManualSchedule* s = new StoredManualSchedule(scheduleId, channelId, startTime, duration, dayMask, title);
      s->ForceAdd = forceadd;
      s->UserParameter = userParam;

      s->RecordingsToKeep = Util::GetXmlFirstChildElementTextAsInt(manual, "recordings_to_keep");

      m_storedSchedules.GetManualSchedules().push_back(s);
    }
    else {
      return false;
    }

    return false;
  }

  return true;
}

UpdateScheduleRequest::UpdateScheduleRequest(const std::string& scheduleId, const bool newOnly, const bool recordSeriesAnytime, const int recordingsToKeep) 
  : m_scheduleId(scheduleId),
    m_newOnly(newOnly),
    m_recordSeriesAnytime(recordSeriesAnytime),
    m_recordingsToKeep(recordingsToKeep)
{

}

UpdateScheduleRequest::~UpdateScheduleRequest()
{

}

std::string& UpdateScheduleRequest::GetScheduleID() 
{ 
  return m_scheduleId; 
}

bool UpdateScheduleRequest::IsNewOnly() 
{ 
  return m_newOnly; 
}

bool UpdateScheduleRequest::WillRecordSeriesAnytime() 
{ 
  return m_recordSeriesAnytime; 
}

int UpdateScheduleRequest::GetRecordingsToKeep() 
{ 
  return m_recordingsToKeep; 
}

bool UpdateScheduleRequestSerializer::WriteObject(std::string& serializedData, UpdateScheduleRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("update_schedule");
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "schedule_id", objectGraph.GetScheduleID()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "new_only", objectGraph.IsNewOnly()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "record_series_anytime", objectGraph.WillRecordSeriesAnytime()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recordings_to_keep", objectGraph.GetRecordingsToKeep()));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

RemoveScheduleRequest::RemoveScheduleRequest(const std::string& scheduleId) 
  : m_scheduleId(scheduleId)
{

}

RemoveScheduleRequest::~RemoveScheduleRequest()
{

}

std::string& RemoveScheduleRequest::GetScheduleID() 
{ 
  return m_scheduleId; 
}

bool RemoveScheduleRequestSerializer::WriteObject(std::string& serializedData, RemoveScheduleRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("remove_schedule");
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "schedule_id", objectGraph.GetScheduleID()));
    
  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
 
  return true;
}
