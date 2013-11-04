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
#include "dvblinkremotehttp.h"
#include "response.h"
#include "request.h"

namespace dvblinkremote 
{

  #ifndef _MSC_VER
  #define vsprintf_s vsprintf
  #define _snprintf_s(a,b,c,...) snprintf(a,b,__VA_ARGS__)
  #endif

  class DVBLinkRemoteCommunication : public IDVBLinkRemoteConnection
  {
  public:
    DVBLinkRemoteCommunication(dvblinkremotehttp::HttpClient& httpClient, const std::string& hostAddress, const long port);
    DVBLinkRemoteCommunication(dvblinkremotehttp::HttpClient& httpClient, const std::string& hostAddress, const long port, const std::string& username, const std::string& password);
    ~DVBLinkRemoteCommunication();

    DVBLinkRemoteStatusCode GetChannels(const GetChannelsRequest& request, ChannelList& response);
    DVBLinkRemoteStatusCode SearchEpg(const EpgSearchRequest& request, EpgSearchResult& response);
    DVBLinkRemoteStatusCode PlayChannel(const StreamRequest& request, Stream& response);
    DVBLinkRemoteStatusCode StopChannel(const StopStreamRequest& request);
    DVBLinkRemoteStatusCode GetRecordings(const GetRecordingsRequest& request, RecordingList& response);
    DVBLinkRemoteStatusCode RemoveRecording(const RemoveRecordingRequest& request);
    DVBLinkRemoteStatusCode AddSchedule(const AddScheduleRequest& request);
    DVBLinkRemoteStatusCode GetSchedules(const GetSchedulesRequest& request, StoredSchedules& response);
    DVBLinkRemoteStatusCode UpdateSchedule(const UpdateScheduleRequest& request);
    DVBLinkRemoteStatusCode RemoveSchedule(const RemoveScheduleRequest& request);
    DVBLinkRemoteStatusCode GetParentalStatus(const GetParentalStatusRequest& request, ParentalStatus& response);
    DVBLinkRemoteStatusCode SetParentalLock(const SetParentalLockRequest& request, ParentalStatus& response);
    DVBLinkRemoteStatusCode GetM3uPlaylist(const GetM3uPlaylistRequest& request, M3uPlaylist& response);
    DVBLinkRemoteStatusCode GetPlaybackObject(const GetPlaybackObjectRequest& request, GetPlaybackObjectResponse& response);
    DVBLinkRemoteStatusCode RemovePlaybackObject(const RemovePlaybackObjectRequest& request);
    DVBLinkRemoteStatusCode StopRecording(const StopRecordingRequest& request);
    DVBLinkRemoteStatusCode GetStreamingCapabilities(const GetStreamingCapabilitiesRequest& request, StreamingCapabilities& response);
    DVBLinkRemoteStatusCode GetRecordingSettings(const GetRecordingSettingsRequest& request, RecordingSettings& response);
    DVBLinkRemoteStatusCode SetRecordingSettings(const SetRecordingSettingsRequest& request);
    void GetLastError(std::string& err);

  private:
    dvblinkremotehttp::HttpClient& m_httpClient;
    std::string m_hostAddress;
    long m_port;
    std::string m_username;
    std::string m_password;
    char m_errorBuffer[dvblinkremote::DVBLINK_REMOTE_DEFAULT_BUFFER_SIZE];

    DVBLinkRemoteStatusCode GetData(const std::string& command, const Request& request, Response& response);
    DVBLinkRemoteStatusCode SerializeRequestObject(const std::string& command, const Request& request, std::string& requestXmlData);
    DVBLinkRemoteStatusCode DeserializeResponseData(const std::string& command, const std::string& responseData, Response& responseObject);
    std::string GetUrl();
    std::string CreateRequestDataParameter(const std::string& command, const std::string& xmlData);
    std::string GetStatusCodeDescription(DVBLinkRemoteStatusCode status);
    void WriteError(const char* format, ...);
    void ClearErrorBuffer();
  };
};
