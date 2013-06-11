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
#include "response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

RecordingSettings::RecordingSettings()
{ }

RecordingSettings::~RecordingSettings()
{ }

GetRecordingSettingsRequest::GetRecordingSettingsRequest()
{ }

GetRecordingSettingsRequest::~GetRecordingSettingsRequest()
{ }

SetRecordingSettingsRequest::SetRecordingSettingsRequest(const int timeMarginBeforeScheduledRecordings, const int timeMarginAfterScheduledRecordings, const std::string& recordingPath)
  : m_timeMarginBeforeScheduledRecordings(timeMarginBeforeScheduledRecordings),
    m_timeMarginAfterScheduledRecordings(timeMarginAfterScheduledRecordings),
    m_recordingPath(recordingPath)
{

}

SetRecordingSettingsRequest::~SetRecordingSettingsRequest()
{ }

int SetRecordingSettingsRequest::GetTimeMarginBeforeScheduledRecordings()
{
  return m_timeMarginBeforeScheduledRecordings;
}

int SetRecordingSettingsRequest::GetTimeMarginAfterScheduledRecordings()
{
  return m_timeMarginAfterScheduledRecordings;
}

std::string& SetRecordingSettingsRequest::GetRecordingPath()
{
  return m_recordingPath;
}

bool GetRecordingSettingsRequestSerializer::WriteObject(std::string& serializedData, GetRecordingSettingsRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("recording_settings"); 

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool RecordingSettingsSerializer::ReadObject(RecordingSettings& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("recording_settings");
    object.TimeMarginBeforeScheduledRecordings = Util::GetXmlFirstChildElementTextAsInt(elRoot, "before_margin");
    object.TimeMarginAfterScheduledRecordings = Util::GetXmlFirstChildElementTextAsInt(elRoot, "after_margin");
    object.RecordingPath = Util::GetXmlFirstChildElementText(elRoot, "recording_path");
    object.TotalSpace = Util::GetXmlFirstChildElementTextAsLong(elRoot, "total_space");
    object.AvailableSpace = Util::GetXmlFirstChildElementTextAsLong(elRoot, "avail_space");
    return true;
  }

  return false;
}

bool SetRecordingSettingsRequestSerializer::WriteObject(std::string& serializedData, SetRecordingSettingsRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("recording_settings"); 
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "before_margin", objectGraph.GetTimeMarginBeforeScheduledRecordings()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "after_margin", objectGraph.GetTimeMarginAfterScheduledRecordings()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recording_path", objectGraph.GetRecordingPath()));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}
