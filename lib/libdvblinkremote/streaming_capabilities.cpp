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

GetStreamingCapabilitiesRequest::GetStreamingCapabilitiesRequest()
{ }

GetStreamingCapabilitiesRequest::~GetStreamingCapabilitiesRequest()
{ }

StreamingCapabilities::StreamingCapabilities() 
  : SupportedProtocols(0),
    SupportedTranscoders(0)
{ }

StreamingCapabilities::StreamingCapabilities(StreamingCapabilities& streamingCapabilities)
{
  SupportedProtocols = streamingCapabilities.SupportedProtocols;
  SupportedTranscoders = streamingCapabilities.SupportedTranscoders;
}

StreamingCapabilities::~StreamingCapabilities()
{ }

bool StreamingCapabilities::IsProtocolSupported(const StreamingCapabilities::DVBLinkSupportedProtocol protocol)
{
  return ((SupportedProtocols & protocol) == protocol);
}

bool StreamingCapabilities::IsProtocolSupported(const int protocolsToCheck)
{
  return ((SupportedProtocols & protocolsToCheck) == protocolsToCheck);
}

bool StreamingCapabilities::IsTranscoderSupported(const StreamingCapabilities::DVBLinkSupportedTranscoder transcoder)
{
  return ((SupportedTranscoders & transcoder) == transcoder);
}

bool StreamingCapabilities::IsTranscoderSupported(const int transcodersToCheck)
{
  return ((SupportedTranscoders & transcodersToCheck) == transcodersToCheck);
}

bool GetStreamingCapabilitiesRequestSerializer::WriteObject(std::string& serializedData, GetStreamingCapabilitiesRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("streaming_caps"); 

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool StreamingCapabilitiesSerializer::ReadObject(StreamingCapabilities& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("streaming_caps");
    object.SupportedProtocols = Util::GetXmlFirstChildElementTextAsInt(elRoot, "protocols");
    object.SupportedTranscoders = Util::GetXmlFirstChildElementTextAsInt(elRoot, "transcoders");
    return true;
  }

  return false;
}
