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
#include <vector>
#include "scheduling.h"

namespace dvblinkremote {
  /** 
    * Base class for DVBLink Client requests.
    */
  class Request { };

  /**
    * Class for defining a get channels request. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetChannels method.
    * @see Channel::Channel()
    * @see IDVBLinkRemoteConnection::GetChannels()
    */
  class GetChannelsRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetChannelsRequest class.
      */
    GetChannelsRequest();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetChannelsRequest();
  };

  /**
    * Represent a strongly typed list of channel identifiers.
    * @see Channel::GetID()
    */
  class ChannelIdentifierList : public std::vector<std::string>
  { 
  public:
    /**
      * Initializes a new instance of the dvblinkremote::ChannelIdentifierList class.
      */
    ChannelIdentifierList();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~ChannelIdentifierList();
  };

  /**
    * Class for defining an electronic program guide (EPG) search request. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::SearchEpg method.
    * @see IDVBLinkRemoteConnection::SearchEpg()
    */
  class EpgSearchRequest : public Request
  {
  public:
    /**
      * Constant long value which specifies that a time value not is bound.
      * Used for the <tt>start time</tt> and <tt>end time</tt>.
      * @see EpgSearchRequest::EpgSearchRequest
      * @see EpgSearchRequest::GetStartTime()
      * @see EpgSearchRequest::SetStartTime
      * @see EpgSearchRequest::GetEndTime()
      * @see EpgSearchRequest::SetEndTime
      */
    static const long EPG_SEARCH_TIME_NOT_BOUND;

    /**
      * Initializes a new instance of the dvblinkremote::EpgSearchRequest class.
      * @param channelId a constant string reference representing the channel identifier for searching the epg.
      * @param startTime an optional constant long representing the start time of programs for searching the epg.
      * @param endTime an optional constant long representing the end time of programs for searching the epg.
      * @param shortEpg an optional constant boolean representing a flag for the level of returned EPG information 
      * from the epg search.
      * \remark By setting a \p startTime or \p endTime the search will only return programs that fully or partially fall 
      * into a specified time span. One of the parameters \p startTime or \p endTime may be -1 
      * (EpgSearchRequest::EPG_SEARCH_TIME_NOT_BOUND), meaning that the search is not bound on the start time or end time side.
      * \remark \p startTime and \p endTime is the number of seconds, counted from UNIX epoc: 00:00:00 UTC on 1 January 1970.
      * \remark \p shortEpg specifies the level of returned EPG information and allows for faster EPG overview build up. 
      * Short EPG information includes only program id, name, start time, duration and genre info. Default value is false.
      */
    EpgSearchRequest(const std::string& channelId, const long startTime = EPG_SEARCH_TIME_NOT_BOUND, const long endTime = EPG_SEARCH_TIME_NOT_BOUND, const bool shortEpg = false);

    /**
      * Initializes a new instance of the dvblinkremote::EpgSearchRequest class.
      * @param channelIdentifierList a constant dvblinkremote::ChannelIdentifierList instance reference representing a 
      * list of channel identifiers for searching the epg.
      * @param startTime an optional constant long representing the start time of programs for searching the epg.
      * @param endTime an optional constant long representing the end time of programs for searching the epg.
      * @param shortEpg an optional constant boolean representing a flag for the level of returned EPG information 
      * from the epg search.
      * \remark By setting a \p startTime or \p endTime the search will only return programs that fully or partially fall 
      * into a specified time span. One of the parameters \p startTime or \p endTime may be -1 
      * (EpgSearchRequest::EPG_SEARCH_TIME_NOT_BOUND), meaning that the search is not bound on the start time or end time side.
      * \remark \p startTime and \p endTime is the number of seconds, counted from UNIX epoc: 00:00:00 UTC on 1 January 1970.
      * \remark \p shortEpg specifies the level of returned EPG information and allows for faster EPG overview build up. 
      * Short EPG information includes only program id, name, start time, duration and genre info. Default value is false.
      */
    EpgSearchRequest(const ChannelIdentifierList& channelIdentifierList, const long startTime = EPG_SEARCH_TIME_NOT_BOUND, const long endTime = EPG_SEARCH_TIME_NOT_BOUND, const bool shortEpg = false);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~EpgSearchRequest();

    /**
      * Gets a list of the channel identifiers of the epg search request.
      * @return Epg search request channel identifiers
      */
    ChannelIdentifierList& GetChannelIdentifiers();

    /**
      * Adds a channel identifier to be searched for.
      * @param channelId a constant strint reference representing the channel identifier to search for.
      */
    void AddChannelID(const std::string& channelId);

    /**
      * String representing the program identifier to search for.
      */
    std::string ProgramID;

    /**
      * String representing the keywords to search for.
      */
    std::string Keywords;

    /**
      * Gets the start time of the epg search request.
      * @return Epg search request start time
      * \remark Number of seconds, counted from UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    long GetStartTime();

    /**
      * Sets the start time to be searched for.
      * @param startTime a constant long representing the start time to search for.
      */
    void SetStartTime(const long startTime);

    /**
      * Gets the end time of the epg search request.
      * @return Epg search request end time
      * \remark Number of seconds, counted from UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    long GetEndTime();

    /**
      * Sets the end time to be searched for.
      * @param endTime a constant long representing the end time to search for.
      */
    void SetEndTime(const long endTime);

    /**
      * Gets if short epg flag is set for the epg search request.
      * @return if short epg flag is set or not
      */
    bool IsShortEpg();

    /**
      * Sets the short epg flag of the search request.
      * @param shortEpg a const boolean representing if short epg flag is set or not.
      */
    void SetShortEpg(const bool shortEpg);

  private:
    ChannelIdentifierList* m_channelIdList;
    long m_startTime;
    long m_endTime;
    bool m_shortEpg;
  };

  /**
    * Abstract base class for stream requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class StreamRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::StreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * @param streamType a constant string reference representing the stream type for the stream request. 
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    StreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, const std::string& streamType);

    /**
      * Pure virtual destructor for cleaning up allocated memory.
      */
    virtual ~StreamRequest() = 0;

    /**
      * Gets the server address for the stream request.
      * @return Stream request server address
      * \remark Ip address/server network name of the DVBLink server. 
      */
    std::string& GetServerAddress();

    /**
      * Gets the DVBLink channel identifier for the stream request.
      * @return Stream request channel identifier
      */
    long GetDVBLinkChannelID();

    /**
      * Gets the unique identification string of the client for the stream request.
      * @return Stream request client identifier
      */
    std::string& GetClientID();

    /**
      * Gets the stream type for the stream request.
      * @return Stream request stream type
      */
    std::string& GetStreamType();

    /**
      * The timeout until channel playback is stopped by a server (indefinite if not specified).
      */
    long Duration;

  private:
    /**
      * Ip address/server network name of the DVBLink server. 
      */
    std::string m_serverAddress; 

    /**
      * DVBLink channel identifier.
      */
    long m_dvbLinkChannelId;

    /**
      * The unique identification string of the client.
      * This should be the same across all DVBLink Client API calls from a given client. 
      * Can be a uuid for example or id/mac of the client device.
    */
    std::string	m_clientId;

    /**
      * The type of requested stream.
      * @see StreamRequest::ANDROID_STREAM_TYPE
      * @see StreamRequest::IPHONE_STREAM_TYPE
      * @see StreamRequest::WINPHONE_STREAM_TYPE
      * @see StreamRequest::RAW_HTTP_STREAM_TYPE
      * @see StreamRequest::RAW_UDP_STREAM_TYPE
      * @see StreamRequest::MP4_STREAM_TYPE
      * @see StreamRequest::H264TS_STREAM_TYPE
      * @see StreamRequest::MP4_STREAM_TYPE
      */
    std::string	m_streamType;
  };

  /**
    * Class for transcoded video stream options. 
    */
  class TranscodingOptions 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::Transcoding class.
      * @param width a constant unsigned integer representing the width in pixels to use for a transcoded video stream.
      * @param height a constant unsigned integer representing the height in pixels to use for a transcoded video stream.
      */
    TranscodingOptions(const unsigned int width, const unsigned int height);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~TranscodingOptions();

    /**
      * Gets the width to be used for transcoding a video stream.
      * @return Width in pixels
      */
    unsigned int GetWidth();

    /**
      * Sets the width to be used for transcoding a video stream.
      * @param width a constant unsigned integer representing width in pixels to be used for transcoding a video stream.
      */
    void SetWidth(const unsigned int width);

    /**
      * Gets the height to be used for transcoding a video stream.
      * @return Height in pixels
      */
    unsigned int GetHeight();

    /**
      * Sets the height to be used for transcoding a video stream.
      * @param height a constant unsigned integer representing height in pixels to be used for transcoding a video stream.
      */
    void SetHeight(const unsigned int height);

    /**
      * Gets the bitrate to be used for transcoding a video stream.
      * @return Bitrate in kilobits/sec
      */
    unsigned int GetBitrate();

    /**
      * Sets the bitrate to be used for transcoding a video stream.
      * @param bitrate a constant unsigned integer representing height in pixels to be used for transcoding a video stream.
      */
    void SetBitrate(const unsigned int bitrate);

    /**
      * Gets the audio track to be used for transcoding a video stream.
      * @return Audio track in ISO-639 language code format.
      */
    std::string& GetAudioTrack();

    /**
      * Sets the audio track to be used for transcoding a video stream.
      * @param audioTrack a constant string reference representing an audio track in ISO-639 language code format to be used for transcoding a video stream.
      */
    void SetAudioTrack(const std::string& audioTrack);

  private:
    /**
      * The width in pixels to use for a transcoded video stream.
      */
    unsigned int m_width;

    /**
      * The height in pixels to use for a transcoded video stream.
      */
    unsigned int m_height;

    /**
      * The bitrate in kilobits/sec to use for a transcoded video stream.
      */
    unsigned int m_bitrate;

    /**
      * The ISO-639 language code to use particular audio track for transcoding.
      */
    std::string	m_audioTrack;	
  };

  /** 
    * Abstract base class for transcoded video stream requests.
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class TranscodedVideoStreamRequest : public StreamRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::TranscodedVideoStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * @param transcodingOptions a dvblinkremote::TranscodingOptions instance reference representing the transcoding options
      * of the stream request. 
      * @param streamType a constant string reference representing the stream type for the stream request. 
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    TranscodedVideoStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, TranscodingOptions& transcodingOptions, const std::string& streamType);

    /**
      * Destructor for cleaning up allocated memory.
      */
    virtual ~TranscodedVideoStreamRequest() = 0;

    /**
      * Gets the transcoding options of the stream request.
      * @return dvblinkremote::TranscodingOptions instance reference.
      */
    TranscodingOptions& GetTranscodingOptions();

  private:
    /**
      * The transcoding options for the stream request.
      */
    TranscodingOptions	m_transcodingOptions;
  };

  /** 
    * Class for Real-time Transport Protocol (RTP) stream requests.
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class RealTimeTransportProtocolStreamRequest : public TranscodedVideoStreamRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::RealTimeTransportProtocolStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * @param transcodingOptions a dvblinkremote::TranscodingOptions instance reference representing the transcoding options
      * of the stream request. 
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    RealTimeTransportProtocolStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, TranscodingOptions& transcodingOptions);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RealTimeTransportProtocolStreamRequest();
  };

  /**
  * Class for MP4 stream requests.
  * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
  * @see IDVBLinkRemoteConnection::PlayChannel()
  */
  class MP4StreamRequest : public TranscodedVideoStreamRequest
  {
  public:
	  /**
	  * Initializes a new instance of the dvblinkremote::MP4StreamRequest class.
	  * @param serverAddress a constant string reference representing the DVBLink server address.
	  * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
	  * @param clientId a constant string reference representing the unique identification string of the client.
	  * @param transcodingOptions a dvblinkremote::TranscodingOptions instance reference representing the transcoding options
	  * of the stream request.
	  * \remark \p serverAddress is the IP address/server network name of the DVBLink server.
	  * \remark \p clientId should be the same across all DVBLink Client API calls from a given client.
	  * It can be a uuid for example or id/mac of the client device.
	  */
	  MP4StreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, TranscodingOptions& transcodingOptions);

	  /**
	  * Destructor for cleaning up allocated memory.
	  */
	  ~MP4StreamRequest();
  };

  /**
  * Class for transcoded (h264 icw AAC) transport stream request.
  * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
  * @see IDVBLinkRemoteConnection::PlayChannel()
  */
  class H264TSStreamRequest : public TranscodedVideoStreamRequest
  {
  public:
      /**
      * Initializes a new instance of the dvblinkremote::H264TSStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client.
      * @param transcodingOptions a dvblinkremote::TranscodingOptions instance reference representing the transcoding options
      * of the stream request.
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server.
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client.
      * It can be a uuid for example or id/mac of the client device.
      */
      H264TSStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, TranscodingOptions& transcodingOptions);

      /**
      * Destructor for cleaning up allocated memory.
      */
      ~H264TSStreamRequest();
  };

  /**
  * Class for transcoded (h264 icw AAC) transport stream request with timeshifting capabilities.
  * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
  * @see IDVBLinkRemoteConnection::PlayChannel()
  */
  class H264TSTimeshiftStreamRequest : public TranscodedVideoStreamRequest
  {
  public:
      /**
      * Initializes a new instance of the dvblinkremote::H264TSStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client.
      * @param transcodingOptions a dvblinkremote::TranscodingOptions instance reference representing the transcoding options
      * of the stream request.
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server.
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client.
      * It can be a uuid for example or id/mac of the client device.
      */
      H264TSTimeshiftStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, TranscodingOptions& transcodingOptions);

      /**
      * Destructor for cleaning up allocated memory.
      */
      ~H264TSTimeshiftStreamRequest();
  };

  /** 
    * Class for HTTP Live %Stream (HLS) stream requests.
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class HttpLiveStreamRequest : public TranscodedVideoStreamRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::HttpLiveStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * @param transcodingOptions a dvblinkremote::TranscodingOptions instance reference representing the transcoding options 
      * of the stream request. 
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    HttpLiveStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, TranscodingOptions& transcodingOptions);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~HttpLiveStreamRequest();
  };

  /** 
    * Class for Windows Media %Stream (ASF) stream requests.
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class WindowsMediaStreamRequest : public TranscodedVideoStreamRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::WindowsMediaStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * @param transcodingOptions a dvblinkremote::TranscodingOptions instance reference representing the transcoding options
      * of the stream request. 
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    WindowsMediaStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, TranscodingOptions& transcodingOptions);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~WindowsMediaStreamRequest();
  };

  /** 
    * Class for Raw HTTP stream requests.
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class RawHttpStreamRequest : public StreamRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::RawHttpStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    RawHttpStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RawHttpStreamRequest();
  };

  /** 
    * Class for Raw HTTP stream requests that supports timeshifting.
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class RawHttpTimeshiftStreamRequest : public StreamRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::RawHttpStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    RawHttpTimeshiftStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RawHttpTimeshiftStreamRequest();
  };

  /** 
    * Class for Raw UDP stream requests.
    * This is used as input parameter for the IDVBLinkRemoteConnection::PlayChannel method.
    * @see IDVBLinkRemoteConnection::PlayChannel()
    */
  class RawUdpStreamRequest : public StreamRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::RawUdpStreamRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param dvbLinkChannelId a constant long representing the DVBLink channel identifier.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * @param clientAddress a constant string reference representing the client address.
      * @param streamingPort a constant unsigned short representing the streaming port.
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    RawUdpStreamRequest(const std::string& serverAddress, const long dvbLinkChannelId, const std::string& clientId, const std::string& clientAddress, const unsigned short int streamingPort);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RawUdpStreamRequest();

    /**
      * Gets the client address of the stream request.
      * @return The client address for the stream request.
      */
    std::string& GetClientAddress();

    /**
      * Gets the streaming port of the stream request.
      * @return The streaming port for the stream request.
      */
    long GetStreamingPort();

  private:
    /**
      * The client address for the stream request.
      */
    std::string	m_clientAddress;

    /**
      * The streaming port for the stream request.
      */
    unsigned short int m_streamingPort;
  };

  /** 
    * Class for stop stream requests.
    * This is used as input parameter for the IDVBLinkRemoteConnection::StopChannel method.
    * @see IDVBLinkRemoteConnection::StopChannel()
    */
  class StopStreamRequest : public Request 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::StopStreamRequest class.
      * @param channelHandle a constant long representing the channel handle of an 
      * existing playing stream to stop.
      * @see dvblinkremote::StreamRequest() for information about the channel handle.
      */
    StopStreamRequest(long channelHandle);

    /**
      * Initializes a new instance of the dvblinkremote::StopStreamRequest class.
      * @param clientId a constant string reference representing the unique 
      * identification string of the client for stopping all playing streams 
      * from that specific client.
      */
    StopStreamRequest(const std::string& clientId);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StopStreamRequest();

    /**
      * Gets the channel handle for the stop stream request.
      * @return Channel handle
      */
    long GetChannelHandle();

    /**
      * Gets the client identifier for the stop stream request.
      * @return Client identifier
      */
    std::string& GetClientID();

  private:
    /**
      * The channel handle for the stop stream request.
      */
    long m_channelHandle;

    /**
      * The unique identification string of the client.
    */
    std::string m_clientId;
  };
  
  class AddScheduleRequest : public Request, public virtual Schedule
  {
  public:
	//AddScheduleRequest(const DVBLinkScheduleType scheduleType, const std::string& channelId);
	AddScheduleRequest();
    virtual ~AddScheduleRequest() = 0;
  };

  /**
    * Class for add manual schedule requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::AddSchedule method.
    * @see IDVBLinkRemoteConnection::AddSchedule()
    */
  class AddManualScheduleRequest : public ManualSchedule, public AddScheduleRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::AddManualScheduleRequest class.
      * @param channelId a constant string reference representing the channel identifier.
      * @param startTime a constant long representing the start time of the schedule.
      * @param duration a constant long representing the duration of the schedule. 
      * @param dayMask a constant long representing the day bitflag of the schedule.
      * \remark Construct the \p dayMask parameter by using bitwize operations on the DVBLinkManualScheduleDayMask.
      * @see DVBLinkManualScheduleDayMask
    * @param title of schedule
      */
    AddManualScheduleRequest(const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title = "");

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~AddManualScheduleRequest();
  };

  /**
    * Class for add schedule by electronic program guide (EPG) requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::AddSchedule method.
    * @see IDVBLinkRemoteConnection::AddSchedule m()
    */
  class AddScheduleByEpgRequest : public EpgSchedule, public AddScheduleRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::AddScheduleByEpgRequest class.
      * @param channelId a constant string reference representing the channel identifier.
      * @param programId a constant string reference representing the program identifier.
      * @param repeat an optional constant boolean representing if the schedule should be 
      * repeated or not. Default value is <tt>false</tt>.
      * @param newOnly an optional constant boolean representing if only new programs 
      * have to be recorded. Default value is <tt>false</tt>.
      * @param recordSeriesAnytime an optional constant boolean representing whether to 
      * record only series starting around original program start time or any of them. 
      * Default value is <tt>false</tt>.
    */
    AddScheduleByEpgRequest(const std::string& channelId, const std::string& programId, const bool repeat = false, const bool newOnly = false, const bool recordSeriesAnytime = false);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~AddScheduleByEpgRequest();
  };

  /**
    * Class for get schedules requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetSchedules method.
    * @see IDVBLinkRemoteConnection::GetSchedules()
    */
  class GetSchedulesRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetSchedulesRequest class.
      */
    GetSchedulesRequest();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetSchedulesRequest();
  };

  /**
    * Class for update schedule requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::UpdateSchedule method.
    * @see IDVBLinkRemoteConnection::UpdateSchedule()
    */
  class UpdateScheduleRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::UpdateScheduleRequest class.
      * @param scheduleId a constant string reference representing the schedule identifier.
      * @param newOnly a constant boolean representing if only new programs 
      * have to be recorded. Default value is <tt>false</tt>.
      * @param recordSeriesAnytime a constant boolean representing whether to 
      * record only series starting around original program start time or any of them. 
      * Default value is <tt>false</tt>.
      * @param recordingsToKeep a constant integer representing how many recordings to 
      * keep for a repeated recording. Default value is <tt>0</tt>, i.e. keep all recordings.
      * \remark \p recordingsToKeep accepted values (1, 2, 3, 4, 5, 6, 7, 10; 0 - keep all)
      */
    UpdateScheduleRequest(const std::string& scheduleId, const bool newOnly, const bool recordSeriesAnytime, const int recordingsToKeep);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~UpdateScheduleRequest();

    /**
      * Gets the identifier for the schedule to be updated.
      * @return Schedule identifier
      */
    std::string& GetScheduleID();

    /**
      * Gets if only new programs are going to be recorded for the schedule to be updated or not.
      * @return boolean value
      */
    bool IsNewOnly();

    /**
      * Gets whether to record only series starting around original program start time or any of 
      * them for the schedule to be updated.
      * @return boolean value
      */
    bool WillRecordSeriesAnytime();

    /**
      * Gets how many recordings to keep for a repeated recording.
      * @return integer value
      */
    int GetRecordingsToKeep();

  private:
    std::string m_scheduleId;
    bool m_newOnly;
    bool m_recordSeriesAnytime;
    int m_recordingsToKeep;
  };

  /**
    * Class for remove schedule requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::RemoveSchedule method.
    * @see IDVBLinkRemoteConnection::RemoveSchedule()
    */
  class RemoveScheduleRequest : public Request 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetRecordingsRequest class.
      * @param scheduleId a constant string reference representing the identifier of the 
      * schedule to be removed.
      */
    RemoveScheduleRequest(const std::string& scheduleId);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RemoveScheduleRequest();

    /**
      * Gets the identifier for the schedule to be removed.
      * @return Schedule identifier
      */
    std::string& GetScheduleID();

  private:
    /**
      * The identifier of the schedule to be removed.
      */
    std::string m_scheduleId;
  };

  /**
    * Class for get recordings requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetRecordings method.
    * @see IDVBLinkRemoteConnection::GetRecordings()
    */
  class GetRecordingsRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetRecordingsRequest class.
      */
    GetRecordingsRequest();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetRecordingsRequest();
  };

  /**
    * Class for remove recording requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::RemoveRecording method.
    * @see IDVBLinkRemoteConnection::RemoveRecording()
    */
  class RemoveRecordingRequest : public Request 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetRecordingsRequest class.
      * @param recordingId a constant string reference representing the identifier of the 
      * recording to be removed.
      */
    RemoveRecordingRequest(const std::string& recordingId);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RemoveRecordingRequest();

    /**
      * Gets the identifier for the recording to be removed.
      * @return Recording identifier
      */
    std::string& GetRecordingID();

  private:
    /**
      * The identifier of the recording to be removed.
      */
    std::string m_recordingId;
  };

  /**
    * Class for get parental status requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetParentalStatus method.
    * @see IDVBLinkRemoteConnection::GetParentalStatus()
    */
  class GetParentalStatusRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetParentalStatusRequest class.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    GetParentalStatusRequest(const std::string& clientId);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetParentalStatusRequest();

    /**
      * Gets the unique identification string of the client.
      * @return Client identifier
      */
    std::string& GetClientID();

  private:
    /**
      * The unique identification string of the client.
      * This should be the same across all DVBLink Client API calls from a given client. 
      * Can be a uuid for example or id/mac of the client device.
      */
    std::string m_clientId;
  };

  /**
    * Class for set parental lock requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::SetParentalLock method.
    * @see IDVBLinkRemoteConnection::SetParentalLock()
    */
  class SetParentalLockRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::SetParentalLockRequest class for disabling a parental lock.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    SetParentalLockRequest(const std::string& clientId);
    
    /**
      * Initializes a new instance of the dvblinkremote::SetParentalLockRequest class for enabling a parental lock.
      * @param clientId a constant string reference representing the unique identification string of the client. 
      * @param code a constant string reference representing the parental lock code. 
      * \remark \p clientId should be the same across all DVBLink Client API calls from a given client. 
      * It can be a uuid for example or id/mac of the client device.
      */
    SetParentalLockRequest(const std::string& clientId, const std::string& code);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~SetParentalLockRequest();

    /**
      * Gets the unique identification string of the client.
      * @return Client identifier
      */
    std::string& GetClientID();

    /**
      * Gets if the parental lock should be enabled or not for the set parental lock request.
      * @return Enabled flag
      */
    bool IsEnabled();
    
    /**
      * Gets the parental lock code for the set parental lock request.
      * @return Parental lock code
      */
    std::string& GetCode();

  private:
    /**
      * The unique identification string of the client.
      * This should be the same across all DVBLink Client API calls from a given client. 
      * Can be a uuid for example or id/mac of the client device.
      */
    std::string m_clientId;

    /**
      * The flag which tells if parental lock should be enabled or disabled.
      */
    bool m_enabled;

    /**
      * The parental lock code if \p m_enabled is \p true.
      */
    std::string m_code;
  };

  /**
    * Class for get M3U playlist requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetM3uPlaylist method.
    * @see IDVBLinkRemoteConnection::GetM3uPlaylist()
    */
  class GetM3uPlaylistRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetM3uPlaylistRequest class.
      */
    GetM3uPlaylistRequest();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetM3uPlaylistRequest();
  };

  /**
    * Class for get playback object requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetPlaybackObject method.
    * @see IDVBLinkRemoteConnection::GetPlaybackObject()
    */
  class GetPlaybackObjectRequest : public Request
  {
  public:
    /**
      * An enum for requesting certain object types to be returned.
      */
    enum DVBLinkRequestedObjectType {
      REQUESTED_OBJECT_TYPE_ALL = -1, /**< All requested object types will be returned */ 
      REQUESTED_OBJECT_TYPE_CONTAINER = 0, /**< Container objects will be returned */ 
      REQUESTED_OBJECT_TYPE_ITEM = 1 /**< Item objects will be returned */ 
    };

    /**
      * An enum for requesting certain item types to be returned.
      */
    enum DVBLinkRequestedItemType {
      REQUESTED_ITEM_TYPE_ALL = -1, /**< All requested item types will be returned */ 
      REQUESTED_ITEM_TYPE_RECORDED_TV = 0, /**< Recorded TV items will be returned */ 
      REQUESTED_ITEM_TYPE_VIDEO = 1, /**< Video items will be returned */ 
      REQUESTED_ITEM_TYPE_AUDIO = 2, /**< Audio items will be returned */ 
      REQUESTED_ITEM_TYPE_IMAGE = 3 /**< Image items will be returned */ 
    };

    /**
      * Initializes a new instance of the dvblinkremote::GetPlaybackObjectRequest class to recieve 
      * the DVBLink server container, i.e. the top level parent of all objects.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      */
    GetPlaybackObjectRequest(const std::string& serverAddress);

    /**
      * Initializes a new instance of the dvblinkremote::GetPlaybackObjectRequest class.
      * @param serverAddress a constant string reference representing the DVBLink server address.
      * @param objectId a constant string reference representing the identifier of the playback 
      * object to recieve.
      * \remark \p serverAddress is the IP address/server network name of the DVBLink server. 
      */
    GetPlaybackObjectRequest(const std::string& serverAddress, const std::string& objectId);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetPlaybackObjectRequest();

    /**
      * Gets the server address for the playback object request.
      * @return Playback object request server address
      * \remark Ip address/server network name of the DVBLink server. 
      */
    std::string& GetServerAddress();

    /**
      * Gets the identifier for the playback object to recieve.
      * @return Playback object identifier
      * \remark Default value is <tt>empty string</tt> which refers to 
      * DVBLink server container, i.e. the top level parent of all objects.
      */
    std::string& GetObjectID();

    /**
      * Indicates which type of objects to be requested.
      * \remark Default value is <tt>REQUESTED_OBJECT_TYPE_ALL</tt>.
      */
    DVBLinkRequestedObjectType RequestedObjectType;
    
    /**
      * Indicates which type of items to be requested.
      * \remark Default value is <tt>REQUESTED_ITEM_TYPE_ALL</tt>.
      */
    DVBLinkRequestedItemType RequestedItemType;

    /**
      * The start position of objects to be requested.
      * \remark Default value is <tt>0</tt>.
      */
    int StartPosition;

    /**
      * The number of objects to be requested.
      * \remark Default value is <tt>-1</tt>, e.g. all.
      */
    int RequestCount;

    /**
      * Indicates if child objects of requested <tt>object identifier</tt> 
      * shall be included in result or not.
      * \remark Default value is <tt>false</tt>. 
      * \remark If <tt>false</tt> – returns information about object itself 
      * as specified by its <tt>object identifier</tt>. 
      * \remark If <tt>true</tt> – returns objects’ children objects – containers and items.
      */
    bool IncludeChildrenObjectsForRequestedObject;

  private:
    /**
      * Ip address/server network name of the DVBLink server. 
      */
    std::string m_serverAddress;

    /**
      * Identifier for the playback object to recieve.
      */
    std::string m_objectId;
  };

  /**
    * Class for remove playback object requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::RemovePlaybackObject method.
    * @see IDVBLinkRemoteConnection::RemovePlaybackObject()
    */
  class RemovePlaybackObjectRequest : public Request 
  {
  public:
    RemovePlaybackObjectRequest(const std::string& objectId);
    ~RemovePlaybackObjectRequest();

    /**
      * Gets the identifier for the playback object to be removed.
      * @return Playback object identifier
      */
    std::string& GetObjectID();

  private:
    /**
      * The identifier of the playback object to be removed.
      */
    std::string m_objectID;
  };

  /**
    * Class for stop recording requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::StopRecording method.
    * @see IDVBLinkRemoteConnection::StopRecording()
    */
  class StopRecordingRequest : public Request 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetRecordingsRequest class.
      * @param objectId a constant string reference representing the object identifier 
      * of the recording to be stopped.
      */
    StopRecordingRequest(const std::string& objectId);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StopRecordingRequest();

    /**
      * Gets the object identifier for the recording to be stopped.
      * @return Recording object identifier
      */
    std::string& GetObjectID();

  private:
    /**
      * The identifier of the recording to be stopped.
      */
    std::string m_objectId;
  };

  /**
    * Class for get streaming capabilities requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetStreamingCapabilities method.
    * @see IDVBLinkRemoteConnection::GetStreamingCapabilities()
    */
  class GetStreamingCapabilitiesRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetStreamingCapabilitiesRequest class.
      */
    GetStreamingCapabilitiesRequest();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetStreamingCapabilitiesRequest();
  };

  /**
    * Class for get recording settings requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::GetRecordingSettings method.
    * @see IDVBLinkRemoteConnection::GetRecordingSettings()
    */
  class GetRecordingSettingsRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetRecordingSettingsRequest class.
      */
    GetRecordingSettingsRequest();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetRecordingSettingsRequest();
  };

  /**
    * Class for set recording settings requests. 
    * This is used as input parameter for the IDVBLinkRemoteConnection::SetRecordingSettings method.
    * @see IDVBLinkRemoteConnection::SetRecordingSettings()
    */
  class SetRecordingSettingsRequest : public Request
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::SetRecordingSettingsRequest class.
      */
    SetRecordingSettingsRequest(const int timeMarginBeforeScheduledRecordings, const int timeMarginAfterScheduledRecordings, const std::string& recordingPath);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~SetRecordingSettingsRequest();

    /**
      * Gets the configured time margin before a schedule recording is started.
      * @return Number of seconds before a schedule recording is started
      */
    int GetTimeMarginBeforeScheduledRecordings();

    /**
      * Gets the configured time margin after a schedule recording is stopped.
      * @return Number of seconds after a schedule recording is stopped
      */
    int GetTimeMarginAfterScheduledRecordings();

    /**
      * Gets the configured file system path where recordings will be stored.
      * @return Configured file system path 
      */
    std::string& GetRecordingPath();

  private:
    int m_timeMarginBeforeScheduledRecordings;
    int m_timeMarginAfterScheduledRecordings;
    std::string m_recordingPath;
  };
}
