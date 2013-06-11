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

StopStreamRequest::StopStreamRequest(long channelHandle) 
  : m_channelHandle(channelHandle), m_clientId("")
{

}

StopStreamRequest::StopStreamRequest(const std::string& clientId) 
  : m_clientId(clientId), m_channelHandle(-1)
{

}

StopStreamRequest::~StopStreamRequest() 
{ 

}

long StopStreamRequest::GetChannelHandle() 
{ 
  return m_channelHandle; 
}

std::string& StopStreamRequest::GetClientID() 
{ 
  return m_clientId; 
}

bool StopStreamRequestSerializer::WriteObject(std::string& serializedData, StopStreamRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("stop_stream");

  if (objectGraph.GetChannelHandle() > 0) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_handle", objectGraph.GetChannelHandle()));
  }  
    
  if (!objectGraph.GetClientID().empty()) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "client_id", objectGraph.GetClientID()));
  }
    
  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}
