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

#include "generic_response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

GenericResponse::GenericResponse() 
{ 
  m_statusCode = DVBLINK_REMOTE_STATUS_OK;
  m_xmlResult = "";
}
    
GenericResponse::GenericResponse(const int statusCode, const std::string& xmlResult) 
  : m_statusCode(statusCode), m_xmlResult(xmlResult) 
{ 

}
    
GenericResponse::~GenericResponse() 
{

};

int GenericResponse::GetStatusCode(void) 
{ 
  return m_statusCode; 
}

void GenericResponse::SetStatusCode(const int statusCode) 
{ 
  m_statusCode = statusCode; 
}

std::string& GenericResponse::GetXmlResult() 
{ 
  return m_xmlResult; 
}

void GenericResponse::SetXmlResult(const std::string& xmlResult) 
{ 
  m_xmlResult = std::string(xmlResult); 
}

GenericResponseSerializer::GenericResponseSerializer()
   : XmlObjectSerializer<GenericResponse>() 
{ 
  
}

bool GenericResponseSerializer::ReadObject(GenericResponse& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("response");
    int statusCode = Util::GetXmlFirstChildElementTextAsInt(elRoot, "status_code");

    if (statusCode == -1) {
      object.SetStatusCode(DVBLINK_REMOTE_STATUS_INVALID_DATA);
    }

    std::string xml_result = std::string(Util::GetXmlFirstChildElementText(elRoot, "xml_result"));

    if (!xml_result.empty()) {
      object.SetXmlResult(xml_result);
    }

    return true;
  }

  return false;
}
