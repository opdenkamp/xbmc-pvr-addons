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

#pragma once

#include <string>
#include "dvblinkremote.h"
#include "dvblinkremoteserialization.h"
#include "generic_response.h"
#include "request.h"
#include "response.h"
#include "util.h"
#include "tinyxml2/tinyxml2.h"

using namespace dvblinkremote;

namespace dvblinkremoteserialization {
  template <class T>
  class XmlObjectSerializer
  {
  public:
    XmlObjectSerializer();
    virtual ~XmlObjectSerializer() = 0;

    bool WriteObject(std::string& serializedData, T& objectGraph);
    bool ReadObject(T& object, const std::string& xml);
    bool HasChildElement(const tinyxml2::XMLElement& element, const char* childElementName);
  
  protected:
    tinyxml2::XMLDocument& GetXmlDocument() { return *m_xmlDocument; }
    tinyxml2::XMLElement* PrepareXmlDocumentForObjectSerialization(const char* rootElementName);

  private:
    tinyxml2::XMLDocument* m_xmlDocument;
  };

  class XmlObjectSerializerFactory 
  {
  public:
    static bool Serialize(const std::string& dvbLinkCommand, const Request& request, std::string& serializedData);
    static bool Deserialize(const std::string& dvbLinkCommand, const std::string& serializedData, Response& response);
  };

  class GenericResponseSerializer : public XmlObjectSerializer<GenericResponse>
  {
  public:
    GenericResponseSerializer();
    bool ReadObject(GenericResponse& object, const std::string& xml);
  };

  class GetChannelsRequestSerializer : public XmlObjectSerializer<GetChannelsRequest>
  {
  public:
    GetChannelsRequestSerializer() : XmlObjectSerializer<GetChannelsRequest>() { }
    bool WriteObject(std::string& serializedData, GetChannelsRequest& objectGraph);
  };

  class GetChannelsResponseSerializer : public XmlObjectSerializer<ChannelList>
  {
  public:
    GetChannelsResponseSerializer() : XmlObjectSerializer<ChannelList>() { }
    bool ReadObject(ChannelList& object, const std::string& xml);

  private:
    class GetChannelsResponseXmlDataDeserializer : public tinyxml2::XMLVisitor
    {
    private:
      GetChannelsResponseSerializer& m_parent;
      ChannelList& m_channelList;
    
    public:
      GetChannelsResponseXmlDataDeserializer(GetChannelsResponseSerializer& parent, ChannelList& channelList);
      ~GetChannelsResponseXmlDataDeserializer();
      bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute);
    };
  };

  class EpgSearchRequestSerializer : public XmlObjectSerializer<EpgSearchRequest>
  {
  public:
    EpgSearchRequestSerializer() : XmlObjectSerializer<EpgSearchRequest>() { }
    bool WriteObject(std::string& serializedData, EpgSearchRequest& objectGraph);
  };

  class EpgSearchResponseSerializer : public XmlObjectSerializer<EpgSearchResult>
  {
  public:
    EpgSearchResponseSerializer() : XmlObjectSerializer<EpgSearchResult>() { }
    bool ReadObject(EpgSearchResult& object, const std::string& xml);

  private:
    class ChannelEpgXmlDataDeserializer : public tinyxml2::XMLVisitor
    {
    private:
      EpgSearchResponseSerializer& m_parent;
      EpgSearchResult& m_epgSearchResult;
    
    public:
      ChannelEpgXmlDataDeserializer(EpgSearchResponseSerializer& parent, EpgSearchResult& epgSearchResult);
      ~ChannelEpgXmlDataDeserializer();
      bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute);
    };

    class ProgramListXmlDataDeserializer : public tinyxml2::XMLVisitor
    {
    private:
      EpgSearchResponseSerializer& m_parent;
      ChannelEpgData& m_channelEpgData;
    
    public:
      ProgramListXmlDataDeserializer(EpgSearchResponseSerializer& parent, ChannelEpgData& channelEpgData);
      ~ProgramListXmlDataDeserializer();
      bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute);
    };
  };

  class ProgramSerializer
  {
  public:
    static void Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, dvblinkremote::Program& program);
  };

  class StreamRequestSerializer : public XmlObjectSerializer<StreamRequest>
  {
  public:
    StreamRequestSerializer() : XmlObjectSerializer<StreamRequest>() { }
    bool WriteObject(std::string& serializedData, StreamRequest& objectGraph);
  };

  class StreamResponseSerializer : public XmlObjectSerializer<Stream>
  {
  public:
    StreamResponseSerializer() : XmlObjectSerializer<Stream>() { }
    bool ReadObject(Stream& object, const std::string& xml);
  };

  class StopStreamRequestSerializer : public XmlObjectSerializer<StopStreamRequest>
  {
  public:
    StopStreamRequestSerializer() : XmlObjectSerializer<StopStreamRequest>() { }
    bool WriteObject(std::string& serializedData, StopStreamRequest& objectGraph);
  };

  class AddScheduleRequestSerializer : public XmlObjectSerializer<AddScheduleRequest>
  {
  public:
    AddScheduleRequestSerializer() : XmlObjectSerializer<AddScheduleRequest>() { }
    bool WriteObject(std::string& serializedData, AddScheduleRequest& objectGraph);
  };

  class GetSchedulesRequestSerializer : public XmlObjectSerializer<GetSchedulesRequest>
  {
  public:
    GetSchedulesRequestSerializer() : XmlObjectSerializer<GetSchedulesRequest>() { }
    bool WriteObject(std::string& serializedData, GetSchedulesRequest& objectGraph);
  };

  class GetSchedulesResponseSerializer : public XmlObjectSerializer<StoredSchedules>
  {
  public:
    GetSchedulesResponseSerializer() : XmlObjectSerializer<StoredSchedules>() { }
    bool ReadObject(StoredSchedules& object, const std::string& xml);

  private:
    class GetSchedulesResponseXmlDataDeserializer : public tinyxml2::XMLVisitor
    {
    private:
      GetSchedulesResponseSerializer& m_parent;
      StoredSchedules& m_storedSchedules;

    public:
      GetSchedulesResponseXmlDataDeserializer(GetSchedulesResponseSerializer& parent, StoredSchedules& storedSchedules);
      ~GetSchedulesResponseXmlDataDeserializer();
      bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute);
    };
  };

  class UpdateScheduleRequestSerializer : public XmlObjectSerializer<UpdateScheduleRequest>
  {
  public:
    UpdateScheduleRequestSerializer() : XmlObjectSerializer<UpdateScheduleRequest>() { }
    bool WriteObject(std::string& serializedData, UpdateScheduleRequest& objectGraph);
  };

  class GetRecordingsRequestSerializer : public XmlObjectSerializer<GetRecordingsRequest>
  {
  public:
    GetRecordingsRequestSerializer() : XmlObjectSerializer<GetRecordingsRequest>() { }
    bool WriteObject(std::string& serializedData, GetRecordingsRequest& objectGraph);
  };

  class GetRecordingsResponseSerializer : public XmlObjectSerializer<RecordingList>
  {
  public:
    GetRecordingsResponseSerializer() : XmlObjectSerializer<RecordingList>() { }
    bool ReadObject(RecordingList& object, const std::string& xml);

  private:
    class GetRecordingsResponseXmlDataDeserializer : public tinyxml2::XMLVisitor
    {
    private:
      GetRecordingsResponseSerializer& m_parent;
      RecordingList& m_recordingList;
    
    public:
      GetRecordingsResponseXmlDataDeserializer(GetRecordingsResponseSerializer& parent, RecordingList& recordingList);
      ~GetRecordingsResponseXmlDataDeserializer();
      bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute);
    };
  };

  class RemoveRecordingRequestSerializer : public XmlObjectSerializer<RemoveRecordingRequest>
  {
  public:
    RemoveRecordingRequestSerializer() : XmlObjectSerializer<RemoveRecordingRequest>() { }
    bool WriteObject(std::string& serializedData, RemoveRecordingRequest& objectGraph);
  };

  class RemoveScheduleRequestSerializer : public XmlObjectSerializer<RemoveScheduleRequest>
  {
  public:
    RemoveScheduleRequestSerializer() : XmlObjectSerializer<RemoveScheduleRequest>() { }
    bool WriteObject(std::string& serializedData, RemoveScheduleRequest& objectGraph);
  };

  class ParentalStatusSerializer : public XmlObjectSerializer<ParentalStatus>
  {
  public:
    ParentalStatusSerializer() : XmlObjectSerializer<ParentalStatus>() { }
    bool ReadObject(ParentalStatus& object, const std::string& xml);
  };

  class GetParentalStatusRequestSerializer : public XmlObjectSerializer<GetParentalStatusRequest>
  {
  public:
    GetParentalStatusRequestSerializer() : XmlObjectSerializer<GetParentalStatusRequest>() { }
    bool WriteObject(std::string& serializedData, GetParentalStatusRequest& objectGraph);
  };

  class SetParentalLockRequestSerializer : public XmlObjectSerializer<SetParentalLockRequest>
  {
  public:
    SetParentalLockRequestSerializer() : XmlObjectSerializer<SetParentalLockRequest>() { }
    bool WriteObject(std::string& serializedData, SetParentalLockRequest& objectGraph);
  };

  class GetM3uPlaylistRequestSerializer : public XmlObjectSerializer<GetM3uPlaylistRequest>
  {
  public:
    GetM3uPlaylistRequestSerializer() : XmlObjectSerializer<GetM3uPlaylistRequest>() { }
    bool WriteObject(std::string& serializedData, GetM3uPlaylistRequest& objectGraph);
  };

  class GetPlaybackObjectRequestSerializer : public XmlObjectSerializer<GetPlaybackObjectRequest>
  {
  public:
    GetPlaybackObjectRequestSerializer() : XmlObjectSerializer<GetPlaybackObjectRequest>() { }
    bool WriteObject(std::string& serializedData, GetPlaybackObjectRequest& objectGraph);
  };

  class GetPlaybackObjectResponseSerializer : public XmlObjectSerializer<GetPlaybackObjectResponse>
  {
  public:
    GetPlaybackObjectResponseSerializer() : XmlObjectSerializer<GetPlaybackObjectResponse>() { }
    bool ReadObject(GetPlaybackObjectResponse& object, const std::string& xml);

  private:    
    class PlaybackContainerXmlDataDeserializer : public tinyxml2::XMLVisitor
    {
    private:
      GetPlaybackObjectResponseSerializer& m_parent;
      PlaybackContainerList& m_playbackContainerList;

    public:
      PlaybackContainerXmlDataDeserializer(GetPlaybackObjectResponseSerializer& parent, PlaybackContainerList& playbackContainerlist);
      ~PlaybackContainerXmlDataDeserializer();
      bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute);
    };

    class PlaybackItemXmlDataDeserializer : public tinyxml2::XMLVisitor
    {
    private:
      GetPlaybackObjectResponseSerializer& m_parent;
      PlaybackItemList& m_playbackItemList;

    public:
      PlaybackItemXmlDataDeserializer(GetPlaybackObjectResponseSerializer& parent, PlaybackItemList& playbackItemList);
      ~PlaybackItemXmlDataDeserializer();
      bool VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute);
    };
  };

  class RemovePlaybackObjectRequestSerializer : public XmlObjectSerializer<RemovePlaybackObjectRequest>
  {
  public:
    RemovePlaybackObjectRequestSerializer() : XmlObjectSerializer<RemovePlaybackObjectRequest>() { }
    bool WriteObject(std::string& serializedData, RemovePlaybackObjectRequest& objectGraph);
  };

  class StopRecordingRequestSerializer : public XmlObjectSerializer<StopRecordingRequest>
  {
  public:
    StopRecordingRequestSerializer() : XmlObjectSerializer<StopRecordingRequest>() { }
    bool WriteObject(std::string& serializedData, StopRecordingRequest& objectGraph);
  };

  class GetStreamingCapabilitiesRequestSerializer : public XmlObjectSerializer<GetStreamingCapabilitiesRequest>
  {
  public:
    GetStreamingCapabilitiesRequestSerializer() : XmlObjectSerializer<GetStreamingCapabilitiesRequest>() { }
    bool WriteObject(std::string& serializedData, GetStreamingCapabilitiesRequest& objectGraph);
  };

  class StreamingCapabilitiesSerializer : public XmlObjectSerializer<StreamingCapabilities>
  {
  public:
    StreamingCapabilitiesSerializer() : XmlObjectSerializer<StreamingCapabilities>() { }
    bool ReadObject(StreamingCapabilities& object, const std::string& xml);
  };

  class GetRecordingSettingsRequestSerializer : public XmlObjectSerializer<GetRecordingSettingsRequest>
  {
  public:
    GetRecordingSettingsRequestSerializer() : XmlObjectSerializer<GetRecordingSettingsRequest>() { }
    bool WriteObject(std::string& serializedData, GetRecordingSettingsRequest& objectGraph);
  };

  class RecordingSettingsSerializer : public XmlObjectSerializer<RecordingSettings>
  {
  public:
    RecordingSettingsSerializer() : XmlObjectSerializer<RecordingSettings>() { }
    bool ReadObject(RecordingSettings& object, const std::string& xml);
  };

  class SetRecordingSettingsRequestSerializer : public XmlObjectSerializer<SetRecordingSettingsRequest>
  {
  public:
    SetRecordingSettingsRequestSerializer() : XmlObjectSerializer<SetRecordingSettingsRequest>() { }
    bool WriteObject(std::string& serializedData, SetRecordingSettingsRequest& objectGraph);
  };

  class ItemMetadataSerializer
  {
  public:
    static void Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, ItemMetadata& itemMetadata);
    static void Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, RecordedTvItemMetadata& metadata);
    static void Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, VideoItemMetadata& metadata);
  };

  template<class T>
  XmlObjectSerializer<T>::XmlObjectSerializer() 
  { 
    m_xmlDocument = new tinyxml2::XMLDocument(); 
  }

  template<class T>
  XmlObjectSerializer<T>::~XmlObjectSerializer() 
  {
    if (m_xmlDocument) {
      delete m_xmlDocument;
    }
  }

  template<class T>
  bool XmlObjectSerializer<T>::WriteObject(std::string& serializedData, T& objectGraph)
  { 
    return false;
  }

  template<class T>
  bool XmlObjectSerializer<T>::ReadObject(T& object, const std::string& xml)
  {
    return false;
  }

  template<class T>
  tinyxml2::XMLElement* XmlObjectSerializer<T>::PrepareXmlDocumentForObjectSerialization(const char* rootElementName)
  {
    m_xmlDocument->InsertFirstChild(m_xmlDocument->NewDeclaration(DVBLINK_REMOTE_SERIALIZATION_XML_DECLARATION.c_str()));
    tinyxml2::XMLElement* xmlRootElement = m_xmlDocument->NewElement(rootElementName);
    xmlRootElement->SetAttribute("xmlns:i", DVBLINK_REMOTE_SERIALIZATION_XML_I_NAMESPACE.c_str());
    xmlRootElement->SetAttribute("xmlns", DVBLINK_REMOTE_SERIALIZATION_XML_NAMESPACE.c_str());
    m_xmlDocument->InsertEndChild(xmlRootElement);
    return xmlRootElement;
  }

  template<class T>
  bool XmlObjectSerializer<T>::HasChildElement(const tinyxml2::XMLElement& element, const char* childElementName)
  {
    return element.FirstChildElement(childElementName) != NULL;
  }
}
