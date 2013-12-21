/**
 * \brief  Proof of concept code to access ArgusTV's REST api using C++ code
 * \author Marcel Groothuis, Fred Hoogduin
 ***************************************************************************
 *      Copyright (C) 2010-2012 Marcel Groothuis, Fred Hoogduin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************
 * Depends on:
 * - jsoncpp: http://jsoncpp.sourceforge.net/
 *
 * Tested under Windows and Linux
 */

#include <stdio.h>
#include <sys/stat.h>
#include "platform/os.h"
#include "client.h"
#include "pvrclient-argustv.h"
#include "utils.h"
#include "argustvrpc.h"
#include "platform/threads/mutex.h"
#include "platform/util/StdString.h"

using namespace ADDON;

// Some version dependent API strings
#define ATV_GETEPG_45 "ArgusTV/Guide/FullPrograms/%s/%i-%02i-%02iT%02i:%02i:%02i/%i-%02i-%02iT%02i:%02i:%02i/false"

/**
 * \brief Namespace with ArgusTV related code
 */
namespace ArgusTV
{
  PLATFORM::CMutex communication_mutex;

  /**
   * \brief Do some internal housekeeping at the start
   */
  void Initialize(void)
  {
    //// due to lack of static constructors...
    //curl_global_init(CURL_GLOBAL_ALL);
  }


  // The usable urls:
  //http://localhost:49943/ArgusTV/Control/help
  //http://localhost:49943/ArgusTV/Scheduler/help
  //http://localhost:49943/ArgusTV/Guide/help
  //http://localhost:49943/ArgusTV/Core/help
  //http://localhost:49943/ArgusTV/Configuration/help
  //http://localhost:49943/ArgusTV/Log/help

  int ArgusTVRPC(const std::string& command, const std::string& arguments, std::string& json_response)
  {
    PLATFORM::CLockObject critsec(communication_mutex);
    std::string url = g_szBaseURL + command;
    int retval = E_FAILED;
    XBMC->Log(LOG_DEBUG, "URL: %s\n", url.c_str());
    void* hFile = XBMC->OpenFileForWrite(url.c_str(), 0);
    if (hFile != NULL)
    {
      int rc = XBMC->WriteFile(hFile, arguments.c_str(), arguments.length());
      if (rc >= 0)
      {
        std::string result;
        result.clear();
        char buffer[1024];
        while (XBMC->ReadFileString(hFile, buffer, 1023))
          result.append(buffer);
        json_response = result;
        retval = 0;
      }
      else
      {
        XBMC->Log(LOG_ERROR, "can not write to %s", url.c_str());
      }
      XBMC->CloseFile(hFile);
    }
    else
    {
      XBMC->Log(LOG_ERROR, "can not open %s for write", url.c_str());
    }
    return retval;
  }

  int ArgusTVRPCToFile(const std::string& command, const std::string& arguments, std::string& filename, long& http_response)
  {
    PLATFORM::CLockObject critsec(communication_mutex);
    std::string url = g_szBaseURL + command;
    int retval = E_FAILED;
    XBMC->Log(LOG_DEBUG, "URL: %s writing to file %s\n", url.c_str(), filename.c_str());
    /* Open the output file */
    FILE *ofile = fopen(filename.c_str(), "w+b");
    if (ofile == NULL)
    {
      XBMC->Log(LOG_ERROR, "can not open %s", filename.c_str());
      return E_FAILED;
    }
    else
    {
      void* hFile = XBMC->OpenFileForWrite(url.c_str(), 0);
      if (hFile != NULL)
      {
        http_response = XBMC->WriteFile(hFile, arguments.c_str(), arguments.length());
        if (http_response >= 0)
        {
          byte buffer[1024];
          int bytesRead = 0;
          retval = 0;
          do
          {
            bytesRead = XBMC->ReadFile(hFile, buffer, sizeof(buffer));
            int written = fwrite(buffer, sizeof(byte), bytesRead, ofile);
            if (bytesRead != written)
            {
              XBMC->Log(LOG_ERROR, "Error while writing to %s (%d bytes written, while asked to write %d bytes).", 
                filename.c_str(), written, bytesRead);
              retval = E_FAILED;
              break;
            }
          } while (bytesRead == sizeof(buffer));
        }
        else
        {
          XBMC->Log(LOG_ERROR, "can not write to %s", url.c_str());
        }
        XBMC->CloseFile(hFile);
      }
      else
      {
        XBMC->Log(LOG_ERROR, "can not open %s for write", url.c_str());
      }
      /* close output file */
      fclose(ofile);
    }
    return retval;
  }

  int ArgusTVJSONRPC(const std::string& command, const std::string& arguments, Json::Value& json_response)
  {
    std::string response;
    int retval = E_FAILED;
    retval = ArgusTVRPC(command, arguments, response);

    if (retval != E_FAILED)
    {
#ifdef DEBUG
      // Print only the first 512 bytes, otherwise XBMC will crash...
      XBMC->Log(LOG_DEBUG, "Response: %s\n", response.substr(0,512).c_str());
#endif
      if (response.length() != 0)
      {
        Json::Reader reader;

        bool parsingSuccessful = reader.parse(response, json_response);

        if ( !parsingSuccessful )
        {
            XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n", 
            response.c_str(),
            reader.getFormatedErrorMessages().c_str() );
            return E_FAILED;
        }
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Empty response");
        return E_EMPTYRESPONSE;
      }
#ifdef DEBUG
      printValueTree(stdout, json_response);
#endif
    }

    return retval;
  }

  /*
   * \brief Get the logo for a channel
   * \param channelGUID GUID of the channel
   */
  std::string GetChannelLogo(const std::string& channelGUID)
  {
#if defined(TARGET_WINDOWS)
    char tmppath[MAX_PATH];
    GetTempPath(MAX_PATH, tmppath);
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
    std::string tmppath = "/tmp/";
#else
#error implement for your OS!
#endif
    std::string finalpath = tmppath;
    finalpath += channelGUID;
    std::string path = finalpath;
    finalpath += ".png";
    path += ".$$$";

    struct tm* modificationtime;
    struct stat buf;
    if (stat(finalpath.c_str(), &buf) != -1)
    {
      modificationtime = localtime((const time_t *) &buf.st_mtime);
    }
    else
    {
      time_t prehistoric = 0;
      modificationtime = localtime(&prehistoric);
    }

    char command[512];

    snprintf(command, 512, "ArgusTV/Scheduler/ChannelLogo/%s/100/100/false/%d-%02d-%02d", channelGUID.c_str(), 
      modificationtime->tm_year + 1900, modificationtime->tm_mon + 1, modificationtime->tm_mday);

    long http_response;
    int retval = ArgusTVRPCToFile(command, "", path, http_response);
    if (retval != 0)
    {
      XBMC->Log(LOG_ERROR, "couldn't retrieve the temporary channel logo file %s.\n", path.c_str());
      return "";
    }

    if (http_response == 200)
    {
      (void) remove(finalpath.c_str());
      if (rename(path.c_str(), finalpath.c_str()) == -1)
      {
        XBMC->Log(LOG_ERROR, "couldn't rename temporary channel logo file %s to %s.\n", path.c_str(), finalpath.c_str());
        finalpath = "";
      }
    }
    else
    {
      // cleanup temporary file
      if (remove(path.c_str()) == -1)
        XBMC->Log(LOG_ERROR, "couldn't delete temporary channel logo file %s.\n", path.c_str());
      // so was the logo not there (204) or was our local version still valid (304)?
      if (http_response == 204)
      {
        finalpath = "";
      }
    }

    return finalpath;
  }

  /*
   * \brief Get the list with channel groups from 4TR
   * \param channelType The channel type (Television or Radio)
   */
  int RequestChannelGroups(enum ChannelType channelType, Json::Value& response)
  {
    int retval = -1;
        
    if (channelType == Television)
    {
      retval = ArgusTVJSONRPC("ArgusTV/Scheduler/ChannelGroups/Television", "?visibleOnly=false", response);
    }
    else if (channelType == Radio)
    {
      retval = ArgusTVJSONRPC("ArgusTV/Scheduler/ChannelGroups/Radio", "?visibleOnly=false", response);
    }
        
    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "RequestChannelGroups failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /*
   * \brief Get the list with channels for the given channel group from 4TR
   * \param channelGroupId GUID of the channel group
   */
  int RequestChannelGroupMembers(const std::string& channelGroupId, Json::Value& response)
  {
    int retval = -1;

    std::string command = "ArgusTV/Scheduler/ChannelsInGroup/" + channelGroupId;
    retval = ArgusTVJSONRPC(command, "", response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "RequestChannelGroupMembers failed. Return value: %i\n", retval);
    }
        
    return retval;
  }
    
  /*
   * \brief Get the list with TV channel groups from 4TR
   */
  int RequestTVChannelGroups(Json::Value& response)
  {
    return RequestChannelGroups(Television, response);
  }
    
  /*
   * \brief Get the list with Radio channel groups from 4TR
   */
  int RequestRadioChannelGroups(Json::Value& response)
  {
    return RequestChannelGroups(Radio, response);
  }

  /*
   * \brief Get the list with channels from 4TR
   * \param channelType The channel type (Television or Radio)
   */
  int GetChannelList(enum ChannelType channelType, Json::Value& response)
  {
    int retval = -1;

    if (channelType == Television)
    {
      retval = ArgusTVJSONRPC("ArgusTV/Scheduler/Channels/Television", "?visibleOnly=false", response);
    }
    else if (channelType == Radio)
    {
      retval = ArgusTVJSONRPC("ArgusTV/Scheduler/Channels/Radio", "?visibleOnly=false", response);        
    }

    if(retval >= 0)
    {           
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /*
   * \brief Ping core service.
   * \param requestedApiVersion The API version the client needs, pass in Constants.ArgusTVRestApiVersion.
   * \return 0 if client and server are compatible, -1 if the client is too old, +1 if the client is newer than the server and -2 if the connection failed (server down?)
   */
  int Ping(int requestedApiVersion)
  {
    Json::Value response;
    char command[128];
    int version = -2;

    snprintf(command, 128, "ArgusTV/Core/Ping/%i", requestedApiVersion);

    int retval = ArgusTVJSONRPC(command, "", response);

    if (retval != E_FAILED)
    {
      if (response.type() == Json::intValue)
      {
        version = response.asInt();
      }
    }

    return version;
  }

  /**
   * \brief Returns information (free disk space) from all recording disks.
   */
  int GetRecordingDisksInfo(Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetRecordingDisksInfo");
    int retval = ArgusTVJSONRPC("ArgusTV/Control/GetRecordingDisksInfo", "", response);

    if (retval < 0)
    {
      XBMC->Log(LOG_ERROR, "GetRecordingDisksInfo failed");
    }
    return retval;
  }

  /**
   * \brief Returns version information (for display only)
   */
  int GetDisplayVersion(Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetDisplayVersion");
    int retval = ArgusTVJSONRPC("ArgusTV/Core/Version", "", response);

    if (retval < 0)
    {
      XBMC->Log(LOG_ERROR, "GetDisplayVersion failed");
    }
    return retval;
  }

  /**
   * \brief GetPluginServices Get all configured plugin services. {activeOnly} = Set to true to only receive active plugins. 
   * \brief Returns an array containing zero or more plugin services.
   * \param activeonly  set to true to only receive active plugins
   * \param response Reference to a std::string used to store the json response string
   * \return  0 when successful
   */
  int GetPluginServices(bool activeonly, Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetPluginServices");
    int retval = E_FAILED;

    std::string args = activeonly ? "?activeOnly=true" : "?activeOnly=false";
    retval = ArgusTV::ArgusTVJSONRPC("ArgusTV/Control/PluginServices", args, response);
    if(retval >= 0)
    {           
      if (response.type() != Json::arrayValue)
      {
        retval = E_FAILED;
        XBMC->Log(LOG_NOTICE, "GetPluginServices did not return a Json::arrayValue [%d].", response.type());
      }
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "GetPluginServices remote call failed.");
    }
    return retval;
  }

  /**
   * \brief AreRecordingSharesAccessible
   * \param thisplugin the plugin to check
   * \param response Reference to a std::string used to store the json response string
   * \return  0 when successful
   */
  int AreRecordingSharesAccessible(Json::Value& thisplugin, Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "AreRecordingSharesAccessible");
    Json::FastWriter writer;
    std::string arguments = writer.write(thisplugin);

    int retval = ArgusTVJSONRPC("ArgusTV/Control/AreRecordingSharesAccessible", arguments, response);

    if (response.type() != Json::arrayValue)
    {
      // response on error is a objectValue
      // TODO: parse it to display the error
      return -1;
    }

    return retval;
  }

  int GetLiveStreams()
  {
    Json::Value response;
    int retval = ArgusTVJSONRPC("ArgusTV/Control/GetLiveStreams", "", response);

    if (retval != E_FAILED)
    {
      if (response.type() == Json::arrayValue)
      {
        // int size = response.size();

        // parse live stream list
        // for ( int index =0; index < size; ++index )
        // {
        // printf("Found live stream %i: %s\n", index, response["LiveStream"]["RtspUrl"].asString.c_str());
        // }
      }
    }
    return retval;
  }

  //Remember the last LiveStream object to be able to stop the stream again
  Json::Value g_current_livestream;

  int TuneLiveStream(const std::string& channel_id, ChannelType channeltype, const std::string channelname, std::string& stream)
  {
    // Send the channel object in json format, *and* a LiveStream object when there is a current 
    // LiveStream present.
    // ATV will answer with a LiveStream object.
    stream = "";

    char command[512];
      
    snprintf(command, 512, "{\"Channel\":{\"BroadcastStart\":\"\",\"BroadcastStop\":\"\",\"ChannelId\":\"%s\",\"ChannelType\":%i,\"DefaultPostRecordSeconds\":0,\"DefaultPreRecordSeconds\":0,\"DisplayName\":\"%s\",\"GuideChannelId\":\"00000000-0000-0000-0000-000000000000\",\"LogicalChannelNumber\":null,\"Sequence\":0,\"Version\":0,\"VisibleInGuide\":true},\"LiveStream\":",
      channel_id.c_str(), channeltype, channelname.c_str());
    std::string arguments = command;
    if (!g_current_livestream.empty())
    {
      Json::FastWriter writer;
      arguments.append(writer.write(g_current_livestream)).append("}");
    }
    else
    {
      arguments.append("null}");
    }

    XBMC->Log(LOG_DEBUG, "ArgusTV/Control/TuneLiveStream, body [%s]", arguments.c_str());

    Json::Value response;
    int retval = ArgusTVJSONRPC("ArgusTV/Control/TuneLiveStream", arguments, response);

    if (retval != E_FAILED)
    {
      if (response.type() == Json::objectValue)
      {
        // First analyse the return code from the server
        ArgusTV::LiveStreamResult livestreamresult = (ArgusTV::LiveStreamResult) response["LiveStreamResult"].asInt();
        XBMC->Log(LOG_DEBUG, "TuneLiveStream result %d.", livestreamresult);
        if (livestreamresult != ArgusTV::Succeed)
        {
          return livestreamresult;
        }

        // Ok, pick up the returned LiveStream object
        Json::Value livestream = response["LiveStream"];
        if (livestream != Json::nullValue)
        {
          g_current_livestream = livestream;
        }
        else
        {
          XBMC->Log(LOG_DEBUG, "No LiveStream received from server.");
          return E_FAILED;
        }
        stream = g_current_livestream["TimeshiftFile"].asString();
        //stream = g_current_livestream["RtspUrl"].asString();
        XBMC->Log(LOG_DEBUG, "Tuned live stream: %s\n", stream.c_str());
        return E_SUCCESS;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue");
        return E_FAILED;
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "TuneLiveStream failed");
    }
    return E_FAILED;
  }


  int StopLiveStream()
  {
    if(!g_current_livestream.empty())
    {
      Json::FastWriter writer;
      std::string arguments = writer.write(g_current_livestream);

      std::string response;
      int retval = ArgusTVRPC("ArgusTV/Control/StopLiveStream", arguments, response);

      g_current_livestream.clear();

      return retval;
    }
    else
    {
      return E_FAILED;
    }
  }

  std::string GetLiveStreamURL(void)
  {
    std::string stream = "";

    if(!g_current_livestream.empty())
    {
      stream = g_current_livestream["RtspUrl"].asString();
    }
    return stream;
  }

  int SignalQuality(Json::Value& response)
  {
    if(!g_current_livestream.empty())
    {
      Json::FastWriter writer;
      std::string arguments = writer.write(g_current_livestream);

      int retval = ArgusTVJSONRPC("ArgusTV/Control/GetLiveStreamTuningDetails", arguments, response);

      //if (retval != E_FAILED)
      //{
      //  printValueTree(response);
      //}

      return retval;
    }
    else
    {
      return E_FAILED;
    }
  }

  bool KeepLiveStreamAlive()
  {
    //Example request:
    //{"CardId":"String content","Channel":{"BroadcastStart":"String content","BroadcastStop":"String content","ChannelId":"1627aea5-8e0a-4371-9022-9b504344e724","ChannelType":0,"DefaultPostRecordSeconds":2147483647,"DefaultPreRecordSeconds":2147483647,"DisplayName":"String content","GuideChannelId":"1627aea5-8e0a-4371-9022-9b504344e724","LogicalChannelNumber":2147483647,"Sequence":2147483647,"Version":2147483647,"VisibleInGuide":true},"RecorderTunerId":"1627aea5-8e0a-4371-9022-9b504344e724","RtspUrl":"String content","StreamLastAliveTime":"\/Date(928142400000+0200)\/","StreamStartedTime":"\/Date(928142400000+0200)\/","TimeshiftFile":"String content"}
    //Example response:
    //true
    if(!g_current_livestream.empty())
    {
      Json::FastWriter writer;
      std::string arguments = writer.write(g_current_livestream);

      Json::Value response;
      int retval = ArgusTVJSONRPC("ArgusTV/Control/KeepLiveStreamAlive", arguments, response);

      if (retval != E_FAILED)
      {
        //if (response == "true")
        //{
        return true;
        //}
      }
    }

    return false;
  }

  int GetEPGData(const std::string& guidechannel_id, struct tm epg_start, struct tm epg_end, Json::Value& response)
  {
    if ( guidechannel_id.length() > 0 )
    {
      char command[256];
      
      //Format: ArgusTV/Guide/Programs/{guideChannelId}/{lowerTime}/{upperTime}
      snprintf(command, 256, ATV_GETEPG_45, 
               guidechannel_id.c_str(),
               epg_start.tm_year + 1900, epg_start.tm_mon + 1, epg_start.tm_mday,
               epg_start.tm_hour, epg_start.tm_min, epg_start.tm_sec,
               epg_end.tm_year + 1900, epg_end.tm_mon + 1, epg_end.tm_mday,
               epg_end.tm_hour, epg_end.tm_min, epg_end.tm_sec);

      int retval = ArgusTVJSONRPC(command, "", response);

      return retval;
    }

    return E_FAILED;
  }

  int GetRecordingGroupByTitle(Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetRecordingGroupByTitle");
    int retval = E_FAILED;
 
    retval = ArgusTV::ArgusTVJSONRPC("ArgusTV/Control/RecordingGroups/Television/GroupByProgramTitle", "", response);
    if(retval >= 0)
    {           
      if (response.type() != Json::arrayValue)
      {
        retval = E_FAILED;
        XBMC->Log(LOG_NOTICE, "GetRecordingGroupByTitle did not return a Json::arrayValue [%d].", response.type());
      }
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "GetRecordingGroupByTitle remote call failed.");
    }
    return retval;
  }

  int GetFullRecordingsForTitle(const std::string& title, Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetFullRecordingsForTitle(\"%s\")", title.c_str());
    std::string command = "ArgusTV/Control/GetFullRecordings/Television?includeNonExisting=false";
    Json::Value jsArgument;
    jsArgument["ScheduleId"] = Json::nullValue;
    jsArgument["ProgramTitle"] = title;
    jsArgument["Category"] = Json::nullValue;
    jsArgument["ChannelId"] = Json::nullValue;
    Json::FastWriter writer;
    std::string arguments = writer.write(jsArgument);

    int retval = ArgusTV::ArgusTVJSONRPC(command, arguments, response);
    if (retval < 0)
    {
      XBMC->Log(LOG_NOTICE, "GetFullRecordingsForTitle remote call failed. (%d)", retval);
    }

    return retval;
  }

  int GetRecordingById(const std::string& id, Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetRecordingById");

    std::string command = "ArgusTV/Control/RecordingById/" + id;

    int retval = ArgusTV::ArgusTVJSONRPC(command, "", response);
    return retval;
  }

  int DeleteRecording(const std::string recordingfilename)
  {
    std::string response;

    XBMC->Log(LOG_DEBUG, "DeleteRecording");

    std::string command = "ArgusTV/Control/DeleteRecording?deleteRecordingFile=true";
    std::string arguments = recordingfilename;

    return ArgusTV::ArgusTVRPC(command, arguments, response);
  }

  int SetRecordingLastWatched(const std::string& recordingfilename)
  {
    std::string response;

    XBMC->Log(LOG_DEBUG, "SetRecordingLastWatched");

    std::string command = "ArgusTV/Control/SetRecordingLastWatched";
    std::string arguments = recordingfilename;

    int retval = ArgusTV::ArgusTVRPC(command, arguments, response);
    return retval;
  }

  int GetRecordingLastWatchedPosition(const std::string& recordingfilename, Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetRecordingLastWatchedPosition(\"%s\",...)", recordingfilename.c_str());

    std::string command = "ArgusTV/Control/RecordingLastWatchedPosition";
    std::string arguments = recordingfilename;

    int retval = ArgusTV::ArgusTVJSONRPC(command, arguments, response);
    if (retval == E_EMPTYRESPONSE) retval = 0;
    if (retval < 0)
    {
      XBMC->Log(LOG_DEBUG, "GetRecordingLastWatchedPosition failed. Return value: %i\n", retval);
    }
    return retval;
  }

  int SetRecordingLastWatchedPosition(const std::string& recordingfilename, int lastwatchedposition)
  {
    std::string response;
    char tmp[512];

    XBMC->Log(LOG_DEBUG, "SetRecordingLastWatchedPosition(\"%s\", %d)", recordingfilename.c_str(), lastwatchedposition);

    snprintf(tmp, 512, "{\"LastWatchedPositionSeconds\":%d, \"RecordingFileName\":%s}", lastwatchedposition, recordingfilename.c_str());
    std::string arguments = tmp;
    std::string command = "ArgusTV/Control/SetRecordingLastWatchedPosition";

    int retval = ArgusTV::ArgusTVRPC(command, arguments, response);
    if (retval < 0)
    {
      XBMC->Log(LOG_DEBUG, "SetRecordingLastWatchedPosition failed. Return value: %i\n", retval);
    }
    return retval;
  }

  int SetRecordingFullyWatchedCount(const std::string& recordingfilename, int playcount)
  {
    std::string response;
    char tmp[512];

    XBMC->Log(LOG_DEBUG, "SetRecordingFullyWatchedCount(\"%s\", %d)", recordingfilename.c_str(), playcount);

    snprintf(tmp, 512, "{\"RecordingFileName\":%s,\"FullyWatchedCount\":%d}", recordingfilename.c_str(), playcount);
    std::string arguments = tmp;
    std::string command = "ArgusTV/Control/SetRecordingFullyWatchedCount";

    int retval = ArgusTV::ArgusTVRPC(command, arguments, response);
    if (retval < 0)
    {
      XBMC->Log(LOG_DEBUG, "SetRecordingFullyWatchedCount failed. Return value: %i\n", retval);
    }
    return retval;
  }

  int GetScheduleById(const std::string& id, Json::Value& response)
  {
    int retval = E_FAILED;

    XBMC->Log(LOG_DEBUG, "GetScheduleById");

    std::string command = "ArgusTV/Scheduler/ScheduleById/" + id;

    retval = ArgusTV::ArgusTVJSONRPC(command, "", response);
    if(retval >= 0)
    {
      if (response.type() != Json::objectValue)
      {
        retval = E_FAILED;
        XBMC->Log(LOG_NOTICE, "GetScheduleById did not return a Json::objectValue [%d].", response.type());
      }
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "GetScheduleById remote call failed.");
    }

    return retval;
  }

  /**
   * \brief Fetch the detailed information of a guide program
   * \param id unique id (guid) of the program
   * \param response Reference to a std::string used to store the json response string
   */
  int GetProgramById(const std::string& id, Json::Value& response)
  {
    int retval = E_FAILED;

    XBMC->Log(LOG_DEBUG, "GetProgramById");

    std::string command = "ArgusTV/Guide/Program/" + id;

    retval = ArgusTV::ArgusTVJSONRPC(command, "", response);
    if(retval >= 0)
    {
      if (response.type() != Json::objectValue)
      {
        retval = E_FAILED;
        XBMC->Log(LOG_NOTICE, "GetProgramById did not return a Json::objectValue [%d].", response.type());
      }
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "GetProgramById remote call failed.");
    }

    return retval;
  }

  /**
   * \brief Fetch the list of schedules for tv or radio
   * \param channeltype  The type of channel to fetch the list for
   */
  int GetScheduleList(enum ChannelType channelType, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetScheduleList");

    // http://madcat:49943/ArgusTV/Scheduler/Schedules/0/82
    char command[256];
    
    //Format: ArgusTV/Guide/Programs/{guideChannelId}/{lowerTime}/{upperTime}
    snprintf(command, 256, "ArgusTV/Scheduler/Schedules/%i/%i" ,
      channelType, Recording );
    retval = ArgusTVJSONRPC(command, "", response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetScheduleList failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Fetch the list of upcoming programs from type 'recording'
   * \currently not used
   */
  int GetUpcomingPrograms(Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetUpcomingPrograms");

    // http://madcat:49943/ArgusTV/Scheduler/UpcomingPrograms/82?includeCancelled=true
    retval = ArgusTVJSONRPC("ArgusTV/Scheduler/UpcomingPrograms/82?includeCancelled=false", "", response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetUpcomingPrograms failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Fetch the list of upcoming recordings
   */
  int GetUpcomingRecordings(Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetUpcomingRecordings");

    // http://madcat:49943/ArgusTV/Control/UpcomingRecordings/7?includeCancelled=true
    retval = ArgusTVJSONRPC("ArgusTV/Control/UpcomingRecordings/7?includeActive=true", "", response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetUpcomingRecordings failed. Return value: %i\n", retval);
    }

    return retval;
  }

    /**
   * \brief Fetch the list of currently active recordings
   */
  int GetActiveRecordings(Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetActiveRecordings");

    retval = ArgusTVJSONRPC("ArgusTV/Control/ActiveRecordings", "", response);

    if(retval >= 0)
    {           
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetActiveRecordings failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Cancel a currently active recording
   */
  int AbortActiveRecording(Json::Value& activeRecording)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "AbortActiveRecording");

    Json::FastWriter writer;
    std::string arguments = writer.write(activeRecording);

    std::string response;
    retval = ArgusTVRPC("ArgusTV/Control/AbortActiveRecording", arguments, response);

    if(retval != 0)
    {
      XBMC->Log(LOG_DEBUG, "AbortActiveRecording failed. Return value: %i\n", retval);
    }

    return retval;
  }


  /**
   * \brief Cancel an upcoming program
   */
  int CancelUpcomingProgram(const std::string& scheduleid, const std::string& channelid, const time_t starttime, const std::string& upcomingprogramid)
  {
    int retval = -1;
    std::string response;

    XBMC->Log(LOG_DEBUG, "CancelUpcomingProgram");
    struct tm* convert = gmtime(&starttime);
    struct tm tm_start = *convert;

    //Format: ArgusTV/Scheduler/CancelUpcomingProgram/{scheduleId}/{channelId}/{startTime}?guideProgramId={guideProgramId}
    char command[256];
    snprintf(command, 256, "ArgusTV/Scheduler/CancelUpcomingProgram/%s/%s/%i-%02i-%02iT%02i:%02i:%02i?guideProgramId=%s" ,
      scheduleid.c_str(), channelid.c_str(),
      tm_start.tm_year + 1900, tm_start.tm_mon + 1, tm_start.tm_mday,
      tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec,
      upcomingprogramid.c_str() );
    retval = ArgusTVRPC(command, "", response);

    if (retval < 0)
    {
      XBMC->Log(LOG_DEBUG, "CancelUpcomingProgram failed. Return value: %i\n", retval);
    }

    return retval;
  }

    /**
   * \brief Retrieve an empty schedule from the server
   */
  int GetEmptySchedule(Json::Value& response)
  {
    int retval = -1;
    XBMC->Log(LOG_DEBUG, "GetEmptySchedule");

    retval = ArgusTVJSONRPC("ArgusTV/Scheduler/EmptySchedule/0/82", "", response);

    if(retval >= 0)
    {           
      if( response.type() != Json::objectValue)
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetEmptySchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }


  /**
   * \brief Add a xbmc timer as a one time schedule
   */
  int AddOneTimeSchedule(const std::string& channelid, const time_t starttime, const std::string& title, int prerecordseconds, int postrecordseconds, int lifetime, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "AddOneTimeSchedule");
    struct tm* convert = localtime(&starttime);
    struct tm tm_start = *convert;

    // Get empty schedule from the server
    Json::Value newSchedule;
    if (ArgusTV::GetEmptySchedule(newSchedule) < 0) return retval;

    // Fill relevant members
    CStdString modifiedtitle = title;
    modifiedtitle.Replace("\"", "\\\"");

    newSchedule["IsOneTime"] = Json::Value(true);
    newSchedule["KeepUntilMode"] = Json::Value(lifetimeToKeepUntilMode(lifetime));
    newSchedule["KeepUntilValue"] = Json::Value(lifetimeToKeepUntilValue(lifetime));
    newSchedule["Name"] = Json::Value(modifiedtitle.c_str());
    newSchedule["PostRecordSeconds"] = Json::Value(postrecordseconds);
    newSchedule["PreRecordSeconds"] = Json::Value(prerecordseconds);

    Json::Value rule(Json::objectValue);
    rule["Arguments"] = Json::arrayValue;
    rule["Arguments"].append(Json::Value(modifiedtitle.c_str()));
    rule["Type"] = Json::Value("TitleEquals");
    newSchedule["Rules"].append(rule);

    char formatbuffer[256];
    rule = Json::objectValue;
    rule["Arguments"] = Json::arrayValue;
    snprintf(formatbuffer, sizeof(formatbuffer), "%i-%02i-%02iT00:00:00", tm_start.tm_year + 1900, tm_start.tm_mon + 1, tm_start.tm_mday);
    rule["Arguments"].append(Json::Value(formatbuffer));
    rule["Type"] = Json::Value("OnDate");
    newSchedule["Rules"].append(rule);

    rule = Json::objectValue;
    rule["Arguments"] = Json::arrayValue;
    snprintf(formatbuffer, sizeof(formatbuffer), "%02i:%02i:%02i", tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec);
    rule["Arguments"].append(Json::Value(formatbuffer));
    rule["Type"] = Json::Value("AroundTime");
    newSchedule["Rules"].append(rule);

    rule = Json::objectValue;
    rule["Arguments"] = Json::arrayValue;
    rule["Arguments"].append(Json::Value(channelid.c_str()));
    rule["Type"] = Json::Value("Channels");
    newSchedule["Rules"].append(rule);

    Json::FastWriter writer;
    std::string tmparguments = writer.write(newSchedule);

    retval = ArgusTVJSONRPC("ArgusTV/Scheduler/SaveSchedule", tmparguments.c_str(), response);

    if(retval >= 0)
    {
      if( response.type() != Json::objectValue)
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "AddOneTimeSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Add a xbmc timer as a manual schedule
   */
  int AddManualSchedule(const std::string& channelid, const time_t starttime, const time_t duration, const std::string& title, int prerecordseconds, int postrecordseconds, int lifetime, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "AddManualSchedule");
    struct tm* convert = localtime(&starttime);
    struct tm tm_start = *convert;
    time_t recordingduration = duration;
    int duration_sec = recordingduration % 60;
    recordingduration /= 60;
    int duration_min = recordingduration % 60;
    recordingduration /= 60;
    int duration_hrs = recordingduration;

    // Get empty schedule from the server
    Json::Value newSchedule;
    if (ArgusTV::GetEmptySchedule(newSchedule) < 0) return retval;

    // Fill relevant members
    CStdString modifiedtitle = title;
    modifiedtitle.Replace("\"", "\\\"");

    newSchedule["IsOneTime"] = Json::Value(true);
    newSchedule["KeepUntilMode"] = Json::Value(lifetimeToKeepUntilMode(lifetime));
    newSchedule["KeepUntilValue"] = Json::Value(lifetimeToKeepUntilValue(lifetime));
    newSchedule["Name"] = Json::Value(modifiedtitle.c_str());
    newSchedule["PostRecordSeconds"] = Json::Value(postrecordseconds);
    newSchedule["PreRecordSeconds"] = Json::Value(prerecordseconds);

    Json::Value rule(Json::objectValue);
    char formatbuffer[256];
    rule["Arguments"] = Json::arrayValue;
    snprintf(formatbuffer, sizeof(formatbuffer), "%i-%02i-%02iT%02i:%02i:%02i", 
      tm_start.tm_year + 1900, tm_start.tm_mon + 1, tm_start.tm_mday,
      tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec);
    rule["Arguments"].append(Json::Value(formatbuffer));
    snprintf(formatbuffer, sizeof(formatbuffer), "%02i:%02i:%02i",
      duration_hrs, duration_min, duration_sec);
    rule["Arguments"].append(Json::Value(formatbuffer));
    rule["Type"] = Json::Value("ManualSchedule");
    newSchedule["Rules"].append(rule);

    rule = Json::objectValue;
    rule["Arguments"] = Json::arrayValue;
    rule["Arguments"].append(Json::Value(channelid.c_str()));
    rule["Type"] = Json::Value("Channels");
    newSchedule["Rules"].append(rule);

    Json::FastWriter writer;
    std::string tmparguments = writer.write(newSchedule);

    retval = ArgusTVJSONRPC("ArgusTV/Scheduler/SaveSchedule", tmparguments, response);

    if(retval >= 0)
    {
      if( response.type() != Json::objectValue)
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "AddManualSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Delete a ArgusTV schedule
   */
  int DeleteSchedule(const std::string& scheduleid)
  {
    int retval = -1;
    std::string response;

    XBMC->Log(LOG_DEBUG, "DeleteSchedule");

    //Format: ArgusTV/Scheduler/DeleteSchedule/d21ec04f-22e0-4bf8-accf-317ecc0fb0f9
    char command[256];
    snprintf(command, 256, "ArgusTV/Scheduler/DeleteSchedule/%s" , scheduleid.c_str());
    retval = ArgusTVRPC(command, "", response);

    if (retval < 0)
    {
      XBMC->Log(LOG_DEBUG, "DeleteSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Get the upcoming programs for a given schedule
   */
  int GetUpcomingProgramsForSchedule(const Json::Value& schedule, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetUpcomingProgramsForSchedule");

    char arguments[1024];
    Json::FastWriter writer;
    snprintf( arguments, sizeof(arguments), "{\"IncludeCancelled\":true,\"Schedule\":%s}", writer.write(schedule).c_str());

    retval = ArgusTVJSONRPC("ArgusTV/Scheduler/UpcomingProgramsForSchedule", arguments, response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetUpcomingProgramsForSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Convert a XBMC Lifetime value to the 4TR keepUntilMode setting
   * \param lifetime the XBMC lifetime value (in days) 
   */
  int lifetimeToKeepUntilMode(int lifetime)
  {
    if (lifetime > 364)
      return ArgusTV::Forever;

    if (lifetime < 2)
      return ArgusTV::UntilSpaceIsNeeded;
    
    return ArgusTV::NumberOfDays;
  }

  /**
   * \brief Convert a XBMC Lifetime value to the 4TR keepUntilValue setting
   * \param lifetime the XBMC lifetime value (in days) 
   */
  int lifetimeToKeepUntilValue(int lifetime)
  {
    if (lifetime > 364 || lifetime < 2)
      return 0;

    return lifetime;
  }

  time_t WCFDateToTimeT(const std::string& wcfdate, int& offset)
  {
    time_t ticks;
    char offsetc;
    int offsetv;

    if (wcfdate.empty())
    {
      return 0;
    }

    //WCF compatible format "/Date(1290896700000+0100)/" => 2010-11-27 23:25:00
    ticks = atoi(wcfdate.substr(6, 10).c_str()); //only take the first 10 chars (fits in a 32-bit time_t value)
    offsetc = wcfdate[19]; // + or -
    offsetv = atoi(wcfdate.substr(20, 4).c_str());

    offset = (offsetc == '+' ? offsetv : -offsetv);

    return ticks;
  }

  std::string TimeTToWCFDate(const time_t thetime)
  {
    std::string wcfdate;

    wcfdate.clear();
    if (thetime != 0)
    {
      struct tm *gmTime;
      time_t localEpoch, gmEpoch;

      /*First get local epoch time*/
      localEpoch = time(NULL);

      /* Using local time epoch get the GM Time */
      gmTime = gmtime(&localEpoch);

      /* Convert gm time in to epoch format */
      gmEpoch = mktime(gmTime);

      /* get the absolute different between them */
      double utcoffset = difftime(localEpoch, gmEpoch);
      int iOffset = (int) utcoffset;

      time_t utctime = thetime - iOffset;

      iOffset = (iOffset / 36);

      char ticks[15], offset[8];
      snprintf(ticks, sizeof(ticks), "%010i", (int) utctime);
      snprintf(offset, sizeof(offset), "%s%04i", iOffset < 0 ? "-" : "+", abs(iOffset));
      char result[29];
      snprintf(result, sizeof(result), "\\/Date(%s000%s)\\/", ticks, offset );
      wcfdate = result;
    }
    return wcfdate;
  }

}

   
//TODO: implement all functionality for a XBMC PVR client
// Misc:
//------
// -GetBackendTime
//   The time at the PVR Backend side
//
// EPG
//-----
//
// Channels
// -DeleteChannel (optional)
// -RenameChannel (optional)
// -MoveChannel (optional)
//
// Recordings
//------------
// -RenameRecording
// -Cutmark functions  (optional)
// Playback:
// -OpenRecordedStream
// -CloseRecordedStream
// -ReadRecordedStream
// -PauseRecordedStream
//
// Timers (schedules)
//--------------------
// -RenameTimer
// -UpdateTimer
//
// Live TV/Radio
// -PauseLiveStream
