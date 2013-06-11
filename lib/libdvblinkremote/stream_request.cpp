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

#include "dvblinkremote.h"
#include "request.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

StreamRequest::StreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, const std::string& streamType)
  : m_serverAddress(serverAddress), 
    m_dvbLinkChannelId(dvbLinkChannelId), 
    m_clientId(clientId), 
    m_streamType(streamType)
{ 
  Duration = -1;
}

StreamRequest::~StreamRequest() 
{ 

}

std::string& StreamRequest::GetServerAddress() 
{ 
  return m_serverAddress; 
}

long StreamRequest::GetDVBLinkChannelID() 
{ 
  return m_dvbLinkChannelId; 
}

std::string& StreamRequest::GetClientID() 
{ 
  return m_clientId; 
}

std::string& StreamRequest::GetStreamType() 
{ 
  return m_streamType; 
}

RawHttpStreamRequest::RawHttpStreamRequest(const std::string& serverAddress, const long channelDvbLinkId, const std::string& clientId)
  : StreamRequest(serverAddress, channelDvbLinkId, clientId, DVBLINK_REMOTE_STREAM_TYPE_RAW_HTTP)
{

}

RawHttpStreamRequest::~RawHttpStreamRequest()
{

}

RawUdpStreamRequest::RawUdpStreamRequest(const std::string& serverAddress, const long channelDvbLinkId, const std::string& clientId, const std::string& clientAddress, const unsigned short int streamingPort)
  : m_clientAddress(clientAddress), m_streamingPort(streamingPort), StreamRequest(serverAddress, channelDvbLinkId, clientId, DVBLINK_REMOTE_STREAM_TYPE_RAW_UDP)
{ }

RawUdpStreamRequest::~RawUdpStreamRequest()
{

}

std::string& RawUdpStreamRequest::GetClientAddress() 
{ 
  return m_clientAddress; 
}

long RawUdpStreamRequest::GetStreamingPort() 
{ 
  return m_streamingPort; 
}

HttpLiveStreamRequest::HttpLiveStreamRequest(const std::string& serverAddress, const long channelDvbLinkId, const std::string& clientId, TranscodingOptions& transcodingOptions)
  : TranscodedVideoStreamRequest(serverAddress, channelDvbLinkId, clientId, transcodingOptions, DVBLINK_REMOTE_STREAM_TYPE_IPHONE)
{ 

}

HttpLiveStreamRequest::~HttpLiveStreamRequest()
{

}

RealTimeTransportProtocolStreamRequest::RealTimeTransportProtocolStreamRequest(const std::string& serverAddress, const long channelDvbLinkId, const std::string& clientId, TranscodingOptions& transcodingOptions)
  : TranscodedVideoStreamRequest(serverAddress, channelDvbLinkId, clientId, transcodingOptions, DVBLINK_REMOTE_STREAM_TYPE_ANDROID)
{ 

}

RealTimeTransportProtocolStreamRequest::~RealTimeTransportProtocolStreamRequest()
{

}

WindowsMediaStreamRequest::WindowsMediaStreamRequest(const std::string& serverAddress, const long channelDvbLinkId, const std::string& clientId, TranscodingOptions& transcodingOptions)
  : TranscodedVideoStreamRequest(serverAddress, channelDvbLinkId, clientId, transcodingOptions, DVBLINK_REMOTE_STREAM_TYPE_WINPHONE)
{ 
  
}

WindowsMediaStreamRequest::~WindowsMediaStreamRequest()
{

}

bool StreamRequestSerializer::WriteObject(std::string& serializedData, StreamRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("stream");

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_dvblink_id", objectGraph.GetDVBLinkChannelID()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "client_id", objectGraph.GetClientID()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "stream_type", objectGraph.GetStreamType()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "server_address", objectGraph.GetServerAddress()));

  if (objectGraph.GetStreamType() == DVBLINK_REMOTE_STREAM_TYPE_RAW_UDP) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "client_address", ((RawUdpStreamRequest&)objectGraph).GetClientAddress()));
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "streaming_port", ((RawUdpStreamRequest&)objectGraph).GetStreamingPort()));
  }

  if (objectGraph.GetStreamType() == DVBLINK_REMOTE_STREAM_TYPE_ANDROID ||
    objectGraph.GetStreamType() == DVBLINK_REMOTE_STREAM_TYPE_IPHONE ||
    objectGraph.GetStreamType() == DVBLINK_REMOTE_STREAM_TYPE_WINPHONE) {
      tinyxml2::XMLElement* xmlTranscoderElement = GetXmlDocument().NewElement("transcoder");

      TranscodingOptions to = ((TranscodedVideoStreamRequest&)objectGraph).GetTranscodingOptions();

      xmlTranscoderElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "height", to.GetHeight()));
      xmlTranscoderElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "width", to.GetWidth()));

      if (to.GetBitrate() > 0) {
        xmlTranscoderElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "bitrate", to.GetBitrate()));
      }

      if (!to.GetAudioTrack().empty()) {
        xmlTranscoderElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "audio_track", to.GetAudioTrack()));
      }

      rootElement->InsertEndChild(xmlTranscoderElement);
  }

  if (objectGraph.Duration > 0) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "duration", objectGraph.Duration));
  }

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());

  return true;
}
