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

PlaybackItem::PlaybackItem(const DVBLinkPlaybackItemType itemType, const std::string& objectId, const std::string& parentId, const std::string& playbackUrl, const std::string& thumbnailUrl, const ItemMetadata* metadata)
  : PlaybackObject(PlaybackObject::PLAYBACK_OBJECT_TYPE_ITEM, objectId, parentId),
    m_itemType(itemType),
    m_playbackUrl(playbackUrl),
    m_thumbnailUrl(thumbnailUrl),
    m_metadata((ItemMetadata*)metadata),
    CanBeDeleted(false),
    Size(0),
    CreationTime(0)
{

}

PlaybackItem::~PlaybackItem()
{
  if (m_metadata) {
    delete m_metadata;
  }
}

PlaybackItem::DVBLinkPlaybackItemType& PlaybackItem::GetItemType()
{
  return m_itemType;
}

std::string& PlaybackItem::GetPlaybackUrl()
{
  return m_playbackUrl;
}

std::string& PlaybackItem::GetThumbnailUrl() 
{
  return m_thumbnailUrl;
}

ItemMetadata& PlaybackItem::GetMetadata()
{
  return *m_metadata;
}

RecordedTvItemMetadata::RecordedTvItemMetadata()
  : ItemMetadata()
{
  
}

RecordedTvItemMetadata::RecordedTvItemMetadata(const std::string& title, const long startTime, const long duration)
  : ItemMetadata(title, startTime, duration)
{
  
}

RecordedTvItemMetadata::RecordedTvItemMetadata(RecordedTvItemMetadata& recordedTvItemMetadata) 
  : ItemMetadata((ItemMetadata&)recordedTvItemMetadata)
{
  
}

RecordedTvItemMetadata::~RecordedTvItemMetadata()
{

}

RecordedTvItem::RecordedTvItem(const std::string& objectId, const std::string& parentId, const std::string& playbackUrl, const std::string& thumbnailUrl, const RecordedTvItemMetadata* metadata)
  : PlaybackItem(PlaybackItem::PLAYBACK_ITEM_TYPE_RECORDED_TV, objectId, parentId, playbackUrl, thumbnailUrl, (ItemMetadata*)metadata),
    ChannelName(""),
    ChannelNumber(0),
    ChannelSubNumber(0),
    State(RecordedTvItem::RECORDED_TV_ITEM_STATE_IN_PROGRESS)
{

}

RecordedTvItem::~RecordedTvItem()
{

}

VideoItemMetadata::VideoItemMetadata()
  : ItemMetadata()
{
  
}

VideoItemMetadata::VideoItemMetadata(const std::string& title, const long startTime, const long duration)
  : ItemMetadata(title, startTime, duration)
{
  
}

VideoItemMetadata::VideoItemMetadata(VideoItemMetadata& videoItemMetadata) 
  : ItemMetadata((ItemMetadata&)videoItemMetadata)
{
  
}

VideoItemMetadata::~VideoItemMetadata()
{

}

VideoItem::VideoItem(const std::string& objectId, const std::string& parentId, const std::string& playbackUrl, const std::string& thumbnailUrl, const VideoItemMetadata* metadata)
  : PlaybackItem(PlaybackItem::PLAYBACK_ITEM_TYPE_VIDEO, objectId, parentId, playbackUrl, thumbnailUrl, (ItemMetadata*)metadata)
{

}

VideoItem::~VideoItem()
{

}

PlaybackItemList::~PlaybackItemList()
{
  for (std::vector<PlaybackItem*>::iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

GetPlaybackObjectResponseSerializer::PlaybackItemXmlDataDeserializer::PlaybackItemXmlDataDeserializer(GetPlaybackObjectResponseSerializer& parent, PlaybackItemList& playbackItemList)
  : m_parent(parent), 
    m_playbackItemList(playbackItemList)
{

}

GetPlaybackObjectResponseSerializer::PlaybackItemXmlDataDeserializer::~PlaybackItemXmlDataDeserializer()
{

}

bool GetPlaybackObjectResponseSerializer::PlaybackItemXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
  if (strcmp(element.Name(), "recorded_tv") == 0 ||
      strcmp(element.Name(), "video") == 0) 
  {
    PlaybackItem* item;
    std::string objectId = Util::GetXmlFirstChildElementText(&element, "object_id");
    std::string parentId = Util::GetXmlFirstChildElementText(&element, "parent_id");
    std::string playbackUrl = Util::GetXmlFirstChildElementText(&element, "url");
    std::string thumbnailUrl = Util::GetXmlFirstChildElementText(&element, "thumbnail");    

    if (strcmp(element.Name(), "recorded_tv") == 0) 
    {     
      tinyxml2::XMLElement* vElement = (tinyxml2::XMLElement*)element.FirstChildElement("video_info");
      RecordedTvItemMetadata* metadata = new RecordedTvItemMetadata();
      ItemMetadataSerializer::Deserialize((XmlObjectSerializer<Response>&)m_parent, *vElement, *metadata);
      RecordedTvItem* recordedTvitem = new RecordedTvItem(objectId, parentId, playbackUrl, thumbnailUrl, metadata);

      if (m_parent.HasChildElement(element, "channel_name")) 
      {
        recordedTvitem->ChannelName = Util::GetXmlFirstChildElementText(&element, "channel_name");
      }

      if (m_parent.HasChildElement(element, "channel_number")) 
      {
        recordedTvitem->ChannelNumber = Util::GetXmlFirstChildElementTextAsInt(&element, "channel_number");
      }
      
      if (m_parent.HasChildElement(element, "channel_subnumber")) 
      {
        recordedTvitem->ChannelSubNumber = Util::GetXmlFirstChildElementTextAsInt(&element, "channel_subnumber");
      }

      if (m_parent.HasChildElement(element, "state")) 
      {
        recordedTvitem->State = (RecordedTvItem::DVBLinkRecordedTvItemState)Util::GetXmlFirstChildElementTextAsInt(&element, "state");
      }

      item = (PlaybackItem*)recordedTvitem;
    }
    else if (strcmp(element.Name(), "video") == 0) 
    {     
      tinyxml2::XMLElement* vElement = (tinyxml2::XMLElement*)element.FirstChildElement("video_info");
      VideoItemMetadata* metadata = new VideoItemMetadata();
      ItemMetadataSerializer::Deserialize((XmlObjectSerializer<Response>&)m_parent, *vElement, *metadata);
      VideoItem* videoItem = new VideoItem(objectId, parentId, playbackUrl, thumbnailUrl, metadata);
      item = (PlaybackItem*)videoItem;
    }

    if (item) 
    {
      if (m_parent.HasChildElement(element, "can_be_deleted")) 
      {
        item->CanBeDeleted = Util::GetXmlFirstChildElementTextAsBoolean(&element, "can_be_deleted");
      }

      if (m_parent.HasChildElement(element, "size")) 
      {
        item->Size = Util::GetXmlFirstChildElementTextAsLong(&element, "size");
      }

      if (m_parent.HasChildElement(element, "creation_time")) 
      {
        item->CreationTime = Util::GetXmlFirstChildElementTextAsLong(&element, "creation_time");
      }

      m_playbackItemList.push_back(item);
    }
  
    return false;
  }

  return true;
}

void ItemMetadataSerializer::Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, RecordedTvItemMetadata& metadata)
{
  Deserialize(objectSerializer, element, (ItemMetadata&)metadata);
}

void ItemMetadataSerializer::Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, VideoItemMetadata& metadata)
{
  Deserialize(objectSerializer, element, (ItemMetadata&)metadata);
}
