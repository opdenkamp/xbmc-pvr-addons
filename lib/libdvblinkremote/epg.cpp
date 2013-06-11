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

#include "request.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

const long EpgSearchRequest::EPG_SEARCH_TIME_NOT_BOUND = -1;

EpgSearchRequest::EpgSearchRequest(const std::string& channelId, const long startTime, const long endTime, const bool shortEpg)
{ 
  m_channelIdList = new ChannelIdentifierList();
  m_channelIdList->push_back(channelId);
  ProgramID = "";
  Keywords = "";
  m_startTime = startTime;
  m_endTime = endTime;
  m_shortEpg = shortEpg;
}

EpgSearchRequest::EpgSearchRequest(const ChannelIdentifierList& channelIdentifierList, const long startTime, const long endTime, const bool shortEpg)
{ 
  m_channelIdList = new ChannelIdentifierList(channelIdentifierList);
  ProgramID = "";
  Keywords = "";
  m_startTime = startTime;
  m_endTime = endTime;
  m_shortEpg = shortEpg;
}

EpgSearchRequest::~EpgSearchRequest() 
{
  delete m_channelIdList;
}

ChannelIdentifierList& EpgSearchRequest::GetChannelIdentifiers() 
{ 
  return *m_channelIdList; 
}

void EpgSearchRequest::AddChannelID(const std::string& channelId) 
{
  m_channelIdList->push_back(channelId); 
}

long EpgSearchRequest::GetStartTime() { return m_startTime; }

void EpgSearchRequest::SetStartTime(const long startTime) { m_startTime = startTime; }

long EpgSearchRequest::GetEndTime() { return m_endTime; }

void EpgSearchRequest::SetEndTime(const long endTime) { m_endTime = endTime; }

bool EpgSearchRequest::IsShortEpg() { return m_shortEpg; }

void EpgSearchRequest::SetShortEpg(const bool shortEpg) { m_shortEpg = shortEpg; }

EpgData::EpgData()
{

}

EpgData::EpgData(EpgData& epgData)
{
  for (std::vector<Program*>::iterator it = epgData.begin(); it < epgData.end(); it++) 
  {
    Program* program = new Program(*((Program*)*it));
    push_back(program);
  }
}

EpgData::~EpgData()
{
  for (std::vector<Program*>::iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

ChannelEpgData::ChannelEpgData(const std::string& channelId) 
  : m_channelId(channelId)
{
  m_epgData = new EpgData();
}

ChannelEpgData::ChannelEpgData(ChannelEpgData& channelEpgData)
  : m_channelId(channelEpgData.GetChannelID())
{
  m_epgData = new EpgData(channelEpgData.GetEpgData());
}

ChannelEpgData::~ChannelEpgData() 
{
  delete m_epgData;
}

std::string& ChannelEpgData::GetChannelID() 
{ 
  return m_channelId; 
}

EpgData& ChannelEpgData::GetEpgData() 
{ 
  return *m_epgData; 
}

void ChannelEpgData::AddProgram(const Program* program) 
{ 
  m_epgData->push_back((Program*)program); 
}

ChannelIdentifierList::ChannelIdentifierList()
{

}

ChannelIdentifierList::~ChannelIdentifierList()
{

}

EpgSearchResult::EpgSearchResult()
{

}

EpgSearchResult::~EpgSearchResult()
{
  for (std::vector<ChannelEpgData*>::iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

bool EpgSearchRequestSerializer::WriteObject(std::string& serializedData, EpgSearchRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("epg_searcher");      
  tinyxml2::XMLElement* xmlChannelIdsElement = rootElement->GetDocument()->NewElement("channels_ids");

  for (ChannelIdentifierList::iterator it = objectGraph.GetChannelIdentifiers().begin(); it < objectGraph.GetChannelIdentifiers().end(); it++) 
  {
    xmlChannelIdsElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_id", (*it))); 
  }
  
  rootElement->InsertEndChild(xmlChannelIdsElement);

  if (!objectGraph.ProgramID.empty()) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "program_id", objectGraph.ProgramID)); 
  }

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "keywords", objectGraph.Keywords)); 
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "start_time", objectGraph.GetStartTime()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "end_time", objectGraph.GetEndTime()));

  if (objectGraph.IsShortEpg()) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "epg_short", objectGraph.IsShortEpg()));
  }
    
  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());

  return true;
}

bool EpgSearchResponseSerializer::ReadObject(EpgSearchResult& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("epg_searcher");
    ChannelEpgXmlDataDeserializer* xmlDataDeserializer = new ChannelEpgXmlDataDeserializer(*this, object);
    elRoot->Accept(xmlDataDeserializer);
    delete xmlDataDeserializer;
    
    return true;
  }
  
  return false;
}

EpgSearchResponseSerializer::ChannelEpgXmlDataDeserializer::ChannelEpgXmlDataDeserializer(EpgSearchResponseSerializer& parent, EpgSearchResult& epgSearchResult) 
  : m_parent(parent), m_epgSearchResult(epgSearchResult) 
{ 

}

EpgSearchResponseSerializer::ChannelEpgXmlDataDeserializer::~ChannelEpgXmlDataDeserializer()
{ 

}

bool EpgSearchResponseSerializer::ChannelEpgXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
  if (strcmp(element.Name(), "channel_epg") == 0) 
  {
    std::string channelId = Util::GetXmlFirstChildElementText(&element, "channel_id");
      
    if (!channelId.empty()) {
      ChannelEpgData* channelEpgData = new ChannelEpgData(channelId);

      ProgramListXmlDataDeserializer* xmlDataDeserializer = new ProgramListXmlDataDeserializer(m_parent, *channelEpgData);
      element.FirstChildElement("dvblink_epg")->Accept(xmlDataDeserializer);
      delete xmlDataDeserializer;

      m_epgSearchResult.push_back(channelEpgData);
    }

    return false;
  }

  return true;
}

EpgSearchResponseSerializer::ProgramListXmlDataDeserializer::ProgramListXmlDataDeserializer(EpgSearchResponseSerializer& parent, ChannelEpgData& channelEpgData) 
  : m_parent(parent), m_channelEpgData(channelEpgData) 
{

}

EpgSearchResponseSerializer::ProgramListXmlDataDeserializer::~ProgramListXmlDataDeserializer()
{ 

}

bool EpgSearchResponseSerializer::ProgramListXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
  if (strcmp(element.Name(), "program") == 0) 
  {    
    Program* p = new Program();
    ProgramSerializer::Deserialize((XmlObjectSerializer<Response>&)m_parent, element, *p);

    m_channelEpgData.AddProgram(p);

    return false;
  }

  return true;
}
