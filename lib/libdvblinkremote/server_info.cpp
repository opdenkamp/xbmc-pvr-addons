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

GetServerInfoRequest::GetServerInfoRequest()
{ }

GetServerInfoRequest::~GetServerInfoRequest()
{ }

ServerInfo::ServerInfo()
{ }

ServerInfo::ServerInfo(ServerInfo& server_info)
{
    install_id_ = server_info.install_id_;
    server_id_ = server_info.server_id_;
    build_ = server_info.build_;
    version_ = server_info.version_;
}

ServerInfo::~ServerInfo()
{ }

bool GetServerInfoRequestSerializer::WriteObject(std::string& serializedData, GetServerInfoRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("server_info"); 

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool ServerInfoSerializer::ReadObject(ServerInfo& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("server_info");
    object.install_id_ = Util::GetXmlFirstChildElementText(elRoot, "install_id");
    object.server_id_ = Util::GetXmlFirstChildElementText(elRoot, "server_id");
    object.version_ = Util::GetXmlFirstChildElementText(elRoot, "version");
    object.build_ = Util::GetXmlFirstChildElementText(elRoot, "build");
    return true;
  }

  return false;
}
