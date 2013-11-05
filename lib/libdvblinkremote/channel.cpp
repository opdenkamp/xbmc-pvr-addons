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

Channel::Channel(const std::string& id, const long dvbLinkId, const std::string& name, const DVBLinkChannelType type, const int number, const int subNumber) 
  : m_id(id), 
    m_dvbLinkId(dvbLinkId), 
    m_name(name),
    m_type(type),
    Number(number), 
    SubNumber(subNumber),
    ChildLock(false)
{
  
}

Channel::Channel(Channel& channel)
  : m_id(channel.GetID()), 
    m_dvbLinkId(channel.GetDvbLinkID()), 
    m_name(channel.GetName()), 
    m_type(channel.GetChannelType()),
    Number(channel.Number), 
    SubNumber(channel.SubNumber),
    ChildLock(channel.ChildLock)
{

}

Channel::~Channel()
{

}

std::string& Channel::GetID()
{
  return m_id;
}

long Channel::GetDvbLinkID() 
{
  return m_dvbLinkId;
}

std::string& Channel::GetName() 
{
  return m_name;
}

Channel::DVBLinkChannelType& Channel::GetChannelType() 
{
  return m_type;
}

ChannelList::ChannelList()
{

}

ChannelList::~ChannelList()
{
  for (std::vector<Channel*>::const_iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

GetChannelsRequest::GetChannelsRequest()
{

}

GetChannelsRequest::~GetChannelsRequest()
{

}

bool GetChannelsRequestSerializer::WriteObject(std::string& serializedData, GetChannelsRequest& objectGraph)
{
  PrepareXmlDocumentForObjectSerialization("channels");    
  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool GetChannelsResponseSerializer::ReadObject(ChannelList& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("channels");
    GetChannelsResponseXmlDataDeserializer* xmlDataDeserializer = new GetChannelsResponseXmlDataDeserializer(*this, object);
    elRoot->Accept(xmlDataDeserializer);
    delete xmlDataDeserializer;

    return true;
  }
  
  return false;
}

GetChannelsResponseSerializer::GetChannelsResponseXmlDataDeserializer::GetChannelsResponseXmlDataDeserializer(GetChannelsResponseSerializer& parent, ChannelList& channelList) 
  : m_parent(parent), 
    m_channelList(channelList) 
{

}

GetChannelsResponseSerializer::GetChannelsResponseXmlDataDeserializer::~GetChannelsResponseXmlDataDeserializer()
{

}

bool GetChannelsResponseSerializer::GetChannelsResponseXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
  if (strcmp(element.Name(), "channel") == 0) 
  {     
    long channelDvbLinkId = Util::GetXmlFirstChildElementTextAsLong(&element, "channel_dvblink_id");
    std::string channelId = Util::GetXmlFirstChildElementText(&element, "channel_id");
    std::string channelName = Util::GetXmlFirstChildElementText(&element, "channel_name");
    int channelNumber = Util::GetXmlFirstChildElementTextAsInt(&element, "channel_number");
    int channelSubNumber = Util::GetXmlFirstChildElementTextAsInt(&element, "channel_subnumber");
    Channel::DVBLinkChannelType channelType = (Channel::DVBLinkChannelType)Util::GetXmlFirstChildElementTextAsInt(&element, "channel_type");

    Channel* channel = new Channel(channelId, channelDvbLinkId, channelName, channelType, channelNumber, channelSubNumber);

    if (m_parent.HasChildElement(*&element, "channel_child_lock"))
    {
      channel->ChildLock = Util::GetXmlFirstChildElementTextAsBoolean(&element, "channel_child_lock");
    }

    m_channelList.push_back(channel);

    return false;
  }

  return true;
}
