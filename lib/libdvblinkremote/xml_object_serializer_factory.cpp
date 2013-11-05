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

#include "xml_object_serializer.h"

using namespace dvblinkremoteserialization;

bool XmlObjectSerializerFactory::Serialize(const std::string& dvbLinkCommand, const Request& request, std::string& serializedData)
{
  bool result = true;
  XmlObjectSerializer<Request>* requestSerializer = NULL;

  if (dvbLinkCommand == DVBLINK_REMOTE_GET_CHANNELS_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetChannelsRequestSerializer();
    result = ((GetChannelsRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetChannelsRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_SEARCH_EPG_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new EpgSearchRequestSerializer();
    result = ((EpgSearchRequestSerializer*)requestSerializer)->WriteObject(serializedData, (EpgSearchRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_PLAY_CHANNEL_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new StreamRequestSerializer();
    result = ((StreamRequestSerializer*)requestSerializer)->WriteObject(serializedData, (StreamRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_STOP_CHANNEL_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new StopStreamRequestSerializer();
    result = ((StopStreamRequestSerializer*)requestSerializer)->WriteObject(serializedData, (StopStreamRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_RECORDINGS_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetRecordingsRequestSerializer();
    result = ((GetRecordingsRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetRecordingsRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_REMOVE_RECORDING_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new RemoveRecordingRequestSerializer();
    result = ((RemoveRecordingRequestSerializer*)requestSerializer)->WriteObject(serializedData, (RemoveRecordingRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_ADD_SCHEDULE_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new AddScheduleRequestSerializer();
    result = ((AddScheduleRequestSerializer*)requestSerializer)->WriteObject(serializedData, (AddScheduleRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_SCHEDULES_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetSchedulesRequestSerializer();
    result = ((GetSchedulesRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetSchedulesRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_UPDATE_SCHEDULE_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new UpdateScheduleRequestSerializer();
    result = ((UpdateScheduleRequestSerializer*)requestSerializer)->WriteObject(serializedData, (UpdateScheduleRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_REMOVE_SCHEDULE_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new RemoveScheduleRequestSerializer();
    result = ((RemoveScheduleRequestSerializer*)requestSerializer)->WriteObject(serializedData, (RemoveScheduleRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_PARENTAL_STATUS_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetParentalStatusRequestSerializer();
    result = ((GetParentalStatusRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetParentalStatusRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_SET_PARENTAL_LOCK_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new SetParentalLockRequestSerializer();
    result = ((SetParentalLockRequestSerializer*)requestSerializer)->WriteObject(serializedData, (SetParentalLockRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_PLAYLIST_M3U_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetM3uPlaylistRequestSerializer();
    result = ((GetM3uPlaylistRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetM3uPlaylistRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_OBJECT_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetPlaybackObjectRequestSerializer();
    result = ((GetPlaybackObjectRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetPlaybackObjectRequest&)request);
  }  
  else if (dvbLinkCommand == DVBLINK_REMOTE_REMOVE_OBJECT_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new RemovePlaybackObjectRequestSerializer();
    result = ((RemovePlaybackObjectRequestSerializer*)requestSerializer)->WriteObject(serializedData, (RemovePlaybackObjectRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_STOP_RECORDING_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new StopRecordingRequestSerializer();
    result = ((StopRecordingRequestSerializer*)requestSerializer)->WriteObject(serializedData, (StopRecordingRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_STREAMING_CAPABILITIES_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetStreamingCapabilitiesRequestSerializer();
    result = ((GetStreamingCapabilitiesRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetStreamingCapabilitiesRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_RECORDING_SETTINGS_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new GetRecordingSettingsRequestSerializer();
    result = ((GetRecordingSettingsRequestSerializer*)requestSerializer)->WriteObject(serializedData, (GetRecordingSettingsRequest&)request);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_SET_RECORDING_SETTING_CMD) {
    requestSerializer = (XmlObjectSerializer<Request>*)new SetRecordingSettingsRequestSerializer();
    result = ((SetRecordingSettingsRequestSerializer*)requestSerializer)->WriteObject(serializedData, (SetRecordingSettingsRequest&)request);
  }
  else {
    result = false;
  }

  if (requestSerializer) {
    delete requestSerializer;
  }

  return result;
}

bool XmlObjectSerializerFactory::Deserialize(const std::string& dvbLinkCommand, const std::string& serializedData, Response& response)
{
  bool result = true;
  XmlObjectSerializer<Response>* responseSerializer = NULL;

  if (dvbLinkCommand == DVBLINK_REMOTE_GET_CHANNELS_CMD) {
    GetChannelsResponseSerializer* serializer = new GetChannelsResponseSerializer();
    result = serializer->ReadObject((ChannelList&)response, serializedData);
    delete serializer;
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_SEARCH_EPG_CMD) {
    responseSerializer = (XmlObjectSerializer<Response>*)new EpgSearchResponseSerializer();
    result = ((EpgSearchResponseSerializer*)responseSerializer)->ReadObject((EpgSearchResult&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_PLAY_CHANNEL_CMD) {
    responseSerializer = (XmlObjectSerializer<Response>*)new StreamResponseSerializer();
    result = ((StreamResponseSerializer*)responseSerializer)->ReadObject((Stream&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_RECORDINGS_CMD) {
    responseSerializer = (XmlObjectSerializer<Response>*)new GetRecordingsResponseSerializer();
    result = ((GetRecordingsResponseSerializer*)responseSerializer)->ReadObject((RecordingList&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_SCHEDULES_CMD) {
    responseSerializer = (XmlObjectSerializer<Response>*)new GetSchedulesResponseSerializer();
    result = ((GetSchedulesResponseSerializer*)responseSerializer)->ReadObject((StoredSchedules&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_SET_PARENTAL_LOCK_CMD || dvbLinkCommand == DVBLINK_REMOTE_GET_PARENTAL_STATUS_CMD) {
      responseSerializer = (XmlObjectSerializer<Response>*)new ParentalStatusSerializer();
      result = ((ParentalStatusSerializer*)responseSerializer)->ReadObject((ParentalStatus&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_OBJECT_CMD) {
    responseSerializer = (XmlObjectSerializer<Response>*)new GetPlaybackObjectResponseSerializer();
    result = ((GetPlaybackObjectResponseSerializer*)responseSerializer)->ReadObject((GetPlaybackObjectResponse&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_STREAMING_CAPABILITIES_CMD) {
    responseSerializer = (XmlObjectSerializer<Response>*)new StreamingCapabilitiesSerializer();
    result = ((StreamingCapabilitiesSerializer*)responseSerializer)->ReadObject((StreamingCapabilities&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_GET_RECORDING_SETTINGS_CMD) {
    responseSerializer = (XmlObjectSerializer<Response>*)new RecordingSettingsSerializer();
    result = ((RecordingSettingsSerializer*)responseSerializer)->ReadObject((RecordingSettings&)response, serializedData);
  }
  else if (dvbLinkCommand == DVBLINK_REMOTE_ADD_SCHEDULE_CMD || 
           dvbLinkCommand == DVBLINK_REMOTE_UPDATE_SCHEDULE_CMD ||
           dvbLinkCommand == DVBLINK_REMOTE_REMOVE_SCHEDULE_CMD ||
           dvbLinkCommand == DVBLINK_REMOTE_REMOVE_RECORDING_CMD || 
           dvbLinkCommand == DVBLINK_REMOTE_STOP_CHANNEL_CMD ||
           dvbLinkCommand == DVBLINK_REMOTE_REMOVE_OBJECT_CMD ||
           dvbLinkCommand == DVBLINK_REMOTE_STOP_RECORDING_CMD ||
           dvbLinkCommand == DVBLINK_REMOTE_SET_RECORDING_SETTING_CMD) {
    result = true;
  }
  else {
    result = false;
  }

  if (responseSerializer) {
    delete responseSerializer;
  }

  return result;
}
