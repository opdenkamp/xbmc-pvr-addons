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

GetPlaybackObjectRequest::GetPlaybackObjectRequest(const std::string& serverAddress)
  : m_serverAddress(serverAddress), 
    m_objectId(""), 
    RequestedObjectType(GetPlaybackObjectRequest::REQUESTED_OBJECT_TYPE_ALL), 
    RequestedItemType(GetPlaybackObjectRequest::REQUESTED_ITEM_TYPE_ALL),
    StartPosition(0), 
    RequestCount(-1), 
    IncludeChildrenObjectsForRequestedObject(false)
{

}

GetPlaybackObjectRequest::GetPlaybackObjectRequest(const std::string& serverAddress, const std::string& objectId)
  : m_serverAddress(serverAddress), 
    m_objectId(objectId), 
    RequestedObjectType(GetPlaybackObjectRequest::REQUESTED_OBJECT_TYPE_ALL), 
    RequestedItemType(GetPlaybackObjectRequest::REQUESTED_ITEM_TYPE_ALL),
    StartPosition(0), 
    RequestCount(-1), 
    IncludeChildrenObjectsForRequestedObject(false)
{

}

GetPlaybackObjectRequest::~GetPlaybackObjectRequest()
{ }

std::string& GetPlaybackObjectRequest::GetServerAddress()
{
  return m_serverAddress;
}

std::string& GetPlaybackObjectRequest::GetObjectID()
{
  return m_objectId;
}

bool GetPlaybackObjectRequestSerializer::WriteObject(std::string& serializedData, GetPlaybackObjectRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("object_requester"); 

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "object_id", objectGraph.GetObjectID()));

  if (objectGraph.RequestedObjectType != GetPlaybackObjectRequest::REQUESTED_OBJECT_TYPE_ALL)
  {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "object_type", (int)objectGraph.RequestedObjectType));
  }

  if (objectGraph.RequestedItemType != GetPlaybackObjectRequest::REQUESTED_ITEM_TYPE_ALL)
  {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "item_type", (int)objectGraph.RequestedItemType));
  }

  if (objectGraph.StartPosition != 0)
  {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "start_position", objectGraph.StartPosition));
  }

  if (objectGraph.RequestCount != -1)
  {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "requested_count", objectGraph.RequestCount));
  }

  if (objectGraph.IncludeChildrenObjectsForRequestedObject)
  {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "children_request", true));
  }

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "server_address", objectGraph.GetServerAddress()));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

GetPlaybackObjectResponse::GetPlaybackObjectResponse()
{
  m_playbackContainerList = new PlaybackContainerList();
  m_playbackItemList = new PlaybackItemList();
}

GetPlaybackObjectResponse::~GetPlaybackObjectResponse()
{
  if (m_playbackContainerList) {
    delete m_playbackContainerList;
  }

  if (m_playbackItemList) {
    delete m_playbackItemList;
  }
}

PlaybackContainerList& GetPlaybackObjectResponse::GetPlaybackContainers()
{
  return *m_playbackContainerList;
}

PlaybackItemList& GetPlaybackObjectResponse::GetPlaybackItems() 
{
  return *m_playbackItemList;
}

bool GetPlaybackObjectResponseSerializer::ReadObject(GetPlaybackObjectResponse& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("object");
    
    if (HasChildElement(*elRoot, "containers")) 
    {
      tinyxml2::XMLElement* containers = elRoot->FirstChildElement("containers");

      PlaybackContainerXmlDataDeserializer* playbackContainerXmlDataDeserializer = new PlaybackContainerXmlDataDeserializer(*this, object.GetPlaybackContainers());
      containers->Accept(playbackContainerXmlDataDeserializer);
      delete playbackContainerXmlDataDeserializer;
    }

    if (HasChildElement(*elRoot, "items")) 
    {
      tinyxml2::XMLElement* items = elRoot->FirstChildElement("items");

      PlaybackItemXmlDataDeserializer* playbackItemXmlDataDeserializer = new PlaybackItemXmlDataDeserializer(*this, object.GetPlaybackItems());
      items->Accept(playbackItemXmlDataDeserializer);
      delete playbackItemXmlDataDeserializer;
    }

    if (HasChildElement(*elRoot, "actual_count")) 
    {
      object.ActualCount = Util::GetXmlFirstChildElementTextAsInt(elRoot, "actual_count");
    }

    if (HasChildElement(*elRoot, "total_count")) 
    {
      object.TotalCount = Util::GetXmlFirstChildElementTextAsInt(elRoot, "total_count");
    }

    return true;
  }

  return false;
}

PlaybackObject::PlaybackObject(const DVBLinkPlaybackObjectType objectType, const std::string& objectId, const std::string& parentId)
  : m_objectType(objectType),
    m_objectId(objectId),
    m_parentId(parentId)
{

}

PlaybackObject::~PlaybackObject()
{ }

PlaybackObject::DVBLinkPlaybackObjectType& PlaybackObject::GetObjectType()
{
  return m_objectType;
}

std::string& PlaybackObject::GetObjectID()
{
  return m_objectId;
}

std::string& PlaybackObject::GetParentID() 
{
  return m_parentId;
}
