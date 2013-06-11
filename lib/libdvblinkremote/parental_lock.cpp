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

ParentalStatus::ParentalStatus()
{ 
  IsEnabled = false;
}

ParentalStatus::ParentalStatus(ParentalStatus& parentalStatus)
{
  IsEnabled = parentalStatus.IsEnabled;
}

ParentalStatus::~ParentalStatus()
{

}

GetParentalStatusRequest::GetParentalStatusRequest(const std::string& clientId) 
  : m_clientId(clientId) 
{ 

}

GetParentalStatusRequest::~GetParentalStatusRequest()
{

}

std::string& GetParentalStatusRequest::GetClientID() 
{ 
  return m_clientId; 
}

SetParentalLockRequest::SetParentalLockRequest(const std::string& clientId) 
  : m_clientId(clientId), m_enabled(false), m_code("") 
{ 

}

SetParentalLockRequest::SetParentalLockRequest(const std::string& clientId, const std::string& code) 
  : m_clientId(clientId), m_enabled(true), m_code(code) 
{ 

}

SetParentalLockRequest::~SetParentalLockRequest() 
{ 

}

std::string& SetParentalLockRequest::GetClientID() 
{ 
  return m_clientId; 
}

bool SetParentalLockRequest::IsEnabled() 
{ 
  return m_enabled; 
}

std::string& SetParentalLockRequest::GetCode() 
{ 
  return m_code; 
}

bool ParentalStatusSerializer::ReadObject(ParentalStatus& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("parental_status");
    object.IsEnabled = Util::GetXmlFirstChildElementTextAsBoolean(elRoot, "is_enabled");
    return true;
  }

  return false;
}

bool GetParentalStatusRequestSerializer::WriteObject(std::string& serializedData, GetParentalStatusRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("parental_lock"); 
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "client_id", objectGraph.GetClientID()));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool SetParentalLockRequestSerializer::WriteObject(std::string& serializedData, SetParentalLockRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("parental_lock"); 
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "client_id", objectGraph.GetClientID()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "is_enable", objectGraph.IsEnabled()));

  if (objectGraph.IsEnabled()) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "code", objectGraph.GetCode()));
  }

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}
