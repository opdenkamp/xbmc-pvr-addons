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

#include "dvblinkremoteconnection.h"
#include "xml_object_serializer.h"
#include "generic_response.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

DVBLinkRemoteCommunication::DVBLinkRemoteCommunication(dvblinkremotehttp::HttpClient& httpClient, const std::string& hostAddress, const long port)
  : m_httpClient(httpClient), 
    m_hostAddress(hostAddress), 
    m_port(port)
{
  m_username = "";
  m_password = "";
}

DVBLinkRemoteCommunication::DVBLinkRemoteCommunication(dvblinkremotehttp::HttpClient& httpClient, const std::string& hostAddress, const long port, const std::string& username, const std::string& password)
  : m_httpClient(httpClient), 
    m_hostAddress(hostAddress), 
    m_port(port), 
    m_username(username), 
    m_password(password)
{ 
  
}

DVBLinkRemoteCommunication::~DVBLinkRemoteCommunication() 
{
  
}

std::string DVBLinkRemoteCommunication::GetStatusCodeDescription(DVBLinkRemoteStatusCode status) 
{
  std::string str = "";

  switch (status)
  {
  case DVBLINK_REMOTE_STATUS_OK:
    str = DVBLINK_REMOTE_STATUS_DESC_OK;
    break;
  case DVBLINK_REMOTE_STATUS_ERROR:
    str = DVBLINK_REMOTE_STATUS_DESC_ERROR;
    break;
  case DVBLINK_REMOTE_STATUS_INVALID_DATA:
    str = DVBLINK_REMOTE_STATUS_DESC_INVALID_DATA;
    break;
  case DVBLINK_REMOTE_STATUS_INVALID_PARAM:
    str = DVBLINK_REMOTE_STATUS_DESC_INVALID_PARAM;
    break;
  case DVBLINK_REMOTE_STATUS_NOT_IMPLEMENTED:
    str = DVBLINK_REMOTE_STATUS_DESC_NOT_IMPLEMENTED;
    break;
  case DVBLINK_REMOTE_STATUS_MC_NOT_RUNNING:
    str = DVBLINK_REMOTE_STATUS_DESC_MC_NOT_RUNNING;
    break;
  case DVBLINK_REMOTE_STATUS_NO_DEFAULT_RECORDER:
    str = DVBLINK_REMOTE_STATUS_DESC_NO_DEFAULT_RECORDER;
    break;
  case DVBLINK_REMOTE_STATUS_MCE_CONNECTION_ERROR:
    str = DVBLINK_REMOTE_STATUS_DESC_MCE_CONNECTION_ERROR;
    break;
  case DVBLINK_REMOTE_STATUS_CONNECTION_ERROR:
    str = DVBLINK_REMOTE_STATUS_DESC_CONNECTION_ERROR;
    break;
  case DVBLINK_REMOTE_STATUS_UNAUTHORISED:
    str = DVBLINK_REMOTE_STATUS_DESC_UNAUTHORIZED;
    break;
  }

  return str;
}

void DVBLinkRemoteCommunication::GetLastError(std::string& err)
{
  m_errorBuffer[dvblinkremote::DVBLINK_REMOTE_DEFAULT_BUFFER_SIZE - 1] = dvblinkremote::DVBLINK_REMOTE_EOF;
  err.assign(m_errorBuffer);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetChannels(const GetChannelsRequest& request, ChannelList& response) 
{
  return GetData(DVBLINK_REMOTE_GET_CHANNELS_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::SearchEpg(const EpgSearchRequest& request, EpgSearchResult& response) 
{
  return GetData(DVBLINK_REMOTE_SEARCH_EPG_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::PlayChannel(const StreamRequest& request, Stream& response)
{
  return GetData(DVBLINK_REMOTE_PLAY_CHANNEL_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::StopChannel(const StopStreamRequest& request) 
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_STOP_CHANNEL_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetRecordings(const GetRecordingsRequest& request, RecordingList& response)
{
  return GetData(DVBLINK_REMOTE_GET_RECORDINGS_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::RemoveRecording(const RemoveRecordingRequest& request)
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_REMOVE_RECORDING_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::AddSchedule(const AddScheduleRequest& request)
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_ADD_SCHEDULE_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetSchedules(const GetSchedulesRequest& request, StoredSchedules& response)
{
  return GetData(DVBLINK_REMOTE_GET_SCHEDULES_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::UpdateSchedule(const UpdateScheduleRequest& request)
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_UPDATE_SCHEDULE_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::RemoveSchedule(const RemoveScheduleRequest& request)
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_REMOVE_SCHEDULE_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetParentalStatus(const GetParentalStatusRequest& request, ParentalStatus& response)
{
  return GetData(DVBLINK_REMOTE_GET_PARENTAL_STATUS_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::SetParentalLock(const SetParentalLockRequest& request, ParentalStatus& response)
{
  return GetData(DVBLINK_REMOTE_SET_PARENTAL_LOCK_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetPlaybackObject(const GetPlaybackObjectRequest& request, GetPlaybackObjectResponse& response )
{
  return GetData(DVBLINK_REMOTE_GET_OBJECT_CMD, request,response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::RemovePlaybackObject(const RemovePlaybackObjectRequest& request)
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_REMOVE_OBJECT_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::StopRecording(const StopRecordingRequest& request)
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_STOP_RECORDING_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetStreamingCapabilities(const GetStreamingCapabilitiesRequest& request, StreamingCapabilities& response)
{
  return GetData(DVBLINK_REMOTE_GET_STREAMING_CAPABILITIES_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetRecordingSettings(const GetRecordingSettingsRequest& request, RecordingSettings& response)
{
  return GetData(DVBLINK_REMOTE_GET_RECORDING_SETTINGS_CMD, request, response);
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::SetRecordingSettings(const SetRecordingSettingsRequest& request)
{
  Response* response = new Response();
  DVBLinkRemoteStatusCode status = GetData(DVBLINK_REMOTE_SET_RECORDING_SETTING_CMD, request, *response);
  delete response;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetM3uPlaylist(const GetM3uPlaylistRequest& request, M3uPlaylist& response)
{
  return GetData(DVBLINK_REMOTE_GET_PLAYLIST_M3U_CMD, request, response);
}

std::string DVBLinkRemoteCommunication::GetUrl()
{
  char buffer[2000];
  int length = _snprintf_s(buffer, sizeof(buffer), 2000, DVBLINK_REMOTE_SERVER_URL_FORMAT.c_str(), DVBLINK_REMOTE_SERVER_URL_SCHEME.c_str(), m_hostAddress.c_str(), m_port, DVBLINK_REMOTE_SERVER_URL_PATH.c_str());
  std::string url(buffer, length);
  return url;
}

std::string DVBLinkRemoteCommunication::CreateRequestDataParameter(const std::string& command, const std::string& xmlData)
{
  std::string encodedCommand = "";
  std::string encodedXmlData = "";

  m_httpClient.UrlEncode(command, encodedCommand);
  m_httpClient.UrlEncode(xmlData, encodedXmlData);

  std::string data = DVBLINK_REMOTE_HTTP_COMMAND_QUERYSTRING + "=";
  data.append(encodedCommand);
  data.append("&" + DVBLINK_REMOTE_HTTP_XML_PARAM_QUERYSTRING + "=");
  data.append(encodedXmlData);

  return data;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::GetData(const std::string& command, const Request& request, Response& responseObject)
{
  DVBLinkRemoteStatusCode status;
  std::string xmlData;

  ClearErrorBuffer();

  if ((status = SerializeRequestObject(command, request, xmlData)) != DVBLINK_REMOTE_STATUS_OK) {
    WriteError("Serialization of request object failed with error code %d (%s).\n", status, GetStatusCodeDescription(status).c_str());
    return status;
  }

  std::string requestData = CreateRequestDataParameter(command, xmlData);

  dvblinkremotehttp::HttpWebRequest* httpRequest = new dvblinkremotehttp::HttpWebRequest(GetUrl());
  httpRequest->Method = DVBLINK_REMOTE_HTTP_METHOD;
  httpRequest->ContentType = DVBLINK_REMOTE_HTTP_CONTENT_TYPE;
  httpRequest->ContentLength = requestData.length();
  httpRequest->UserName = m_username;
  httpRequest->Password = m_password;
  httpRequest->SetRequestData(requestData);

  if (!m_httpClient.SendRequest(*httpRequest)) {
    status = DVBLINK_REMOTE_STATUS_CONNECTION_ERROR;
    WriteError("HTTP request failed with error code %d (%s).\n", status, GetStatusCodeDescription(status).c_str());
  }
  else {
    dvblinkremotehttp::HttpWebResponse* httpResponse = m_httpClient.GetResponse();

    if (httpResponse->GetStatusCode() == 401) {
      status = DVBLINK_REMOTE_STATUS_UNAUTHORISED;
      WriteError("HTTP response returned status code %d (%s).\n", httpResponse->GetStatusCode(), GetStatusCodeDescription(status).c_str());
    }
    else if (httpResponse->GetStatusCode() != 200) {
      status = DVBLINK_REMOTE_STATUS_ERROR;
      WriteError("HTTP response returned status code %d.\n", httpResponse->GetStatusCode());
    }
    else {
      std::string responseData = httpResponse->GetResponseData();
      
      if ((status = DeserializeResponseData(command, responseData, responseObject)) != DVBLINK_REMOTE_STATUS_OK) {
        WriteError("Deserialization of response data failed with error code %d (%s).\n", status, GetStatusCodeDescription(status).c_str());
      }
    }

    delete httpResponse;
  }

  delete httpRequest;

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::SerializeRequestObject(const std::string& command, const Request& request, std::string& serializedXmlData)
{
  DVBLinkRemoteStatusCode status = DVBLINK_REMOTE_STATUS_OK;

  if (!XmlObjectSerializerFactory::Serialize(command, request, serializedXmlData)) {
    status = DVBLINK_REMOTE_STATUS_INVALID_DATA;
  }

  return status;
}

DVBLinkRemoteStatusCode DVBLinkRemoteCommunication::DeserializeResponseData(const std::string& command, const std::string& responseData, Response& responseObject)
{
  DVBLinkRemoteStatusCode status = DVBLINK_REMOTE_STATUS_OK;

  if (command == DVBLINK_REMOTE_GET_PLAYLIST_M3U_CMD) 
  {
    ((M3uPlaylist&)responseObject).FileContent.assign(responseData);
  }
  else
  {
    GenericResponseSerializer* genericResponseSerializer = new GenericResponseSerializer();
    GenericResponse* genericResponse = new GenericResponse();
    
    if (genericResponseSerializer->ReadObject(*genericResponse, responseData)) {
      if ((status = (DVBLinkRemoteStatusCode)genericResponse->GetStatusCode()) == DVBLINK_REMOTE_STATUS_OK) {
        if (!XmlObjectSerializerFactory::Deserialize(command, genericResponse->GetXmlResult(), responseObject)) {
          status = DVBLINK_REMOTE_STATUS_INVALID_DATA;
        }
      }
    }

    delete genericResponse;
    delete genericResponseSerializer;
  }

  return status;
}

void DVBLinkRemoteCommunication::WriteError(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf_s(m_errorBuffer, format, args);
  va_end(args);
}

void DVBLinkRemoteCommunication::ClearErrorBuffer()
{
  memset(m_errorBuffer, 0, dvblinkremote::DVBLINK_REMOTE_EOF);
}
