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

#include "response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

PlaybackContainer::PlaybackContainer(const std::string& objectId, const std::string& parentId, const std::string& name, DVBLinkPlaybackContainerType& containerType, DVBLinkPlaybackContainerContentType& containerContentType)
  : PlaybackObject(PlaybackObject::PLAYBACK_OBJECT_TYPE_CONTAINER, objectId, parentId),
    m_name(name),
    m_containerType((DVBLinkPlaybackContainerType&)containerType),
    m_containerContentType((DVBLinkPlaybackContainerContentType&)containerContentType),
    Description(""), 
    Logo(""), 
    TotalCount(0),
    SourceID("")
{

}

PlaybackContainer::~PlaybackContainer()
{ }

std::string& PlaybackContainer::GetName()
{
  return m_name;
}

PlaybackContainer::DVBLinkPlaybackContainerType& PlaybackContainer::GetContainerType()
{
  return m_containerType;
}

PlaybackContainer::DVBLinkPlaybackContainerContentType& PlaybackContainer::GetContainerContentType()
{
  return m_containerContentType;
}

PlaybackContainerList::~PlaybackContainerList()
{
  for (std::vector<PlaybackContainer*>::iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

GetPlaybackObjectResponseSerializer::PlaybackContainerXmlDataDeserializer::PlaybackContainerXmlDataDeserializer(GetPlaybackObjectResponseSerializer& parent, PlaybackContainerList& playbackContainerList)
  : m_parent(parent), 
    m_playbackContainerList(playbackContainerList)
{

}

GetPlaybackObjectResponseSerializer::PlaybackContainerXmlDataDeserializer::~PlaybackContainerXmlDataDeserializer()
{

}

bool GetPlaybackObjectResponseSerializer::PlaybackContainerXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
  if (strcmp(element.Name(), "container") == 0) 
  {     
    std::string objectId = Util::GetXmlFirstChildElementText(&element, "object_id");
    std::string parentId = Util::GetXmlFirstChildElementText(&element, "parent_id");
    std::string name = Util::GetXmlFirstChildElementText(&element, "name");
    PlaybackContainer::DVBLinkPlaybackContainerType containerType = (PlaybackContainer::DVBLinkPlaybackContainerType)Util::GetXmlFirstChildElementTextAsInt(&element, "container_type");
    PlaybackContainer::DVBLinkPlaybackContainerContentType contentType = (PlaybackContainer::DVBLinkPlaybackContainerContentType)Util::GetXmlFirstChildElementTextAsInt(&element, "content_type");

    PlaybackContainer* playbackContainer = new PlaybackContainer(objectId, parentId, name, containerType, contentType);

    if (m_parent.HasChildElement(element, "description")) 
    {
      playbackContainer->Description = Util::GetXmlFirstChildElementText(&element, "description");
    }

    if (m_parent.HasChildElement(element, "logo")) 
    {
      playbackContainer->Logo = Util::GetXmlFirstChildElementText(&element, "logo");
    }

    if (m_parent.HasChildElement(element, "total_count")) 
    {
      playbackContainer->TotalCount = Util::GetXmlFirstChildElementTextAsInt(&element, "total_count");
    }
    
    if (m_parent.HasChildElement(element, "source_id")) 
    {
      playbackContainer->SourceID = Util::GetXmlFirstChildElementText(&element, "source_id");
    }
    
    m_playbackContainerList.push_back(playbackContainer);

    return false;
  }

  return true;
}
