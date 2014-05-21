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
#include "request.h"

namespace dvblinkremote {
  /** 
    * Base class for DVBLink Client responses.
    */
  class Response { };

  /**
    * Represent a DVBLink channel.
    */
  class Channel
  {
  public:
    /**
      * An enum for channel types.
      */
    enum DVBLinkChannelType {
      CHANNEL_TYPE_TV = 0,
      CHANNEL_TYPE_RADIO = 1,
      CHANNEL_TYPE_OTHER = 2	
    };

    /**
      * Initializes a new instance of the dvblinkremote::Channel class.
      * @param id a constant string reference representing the generic identifier of the channel.
      * @param dvbLinkId a constant long representing the DVBLink identifier of the channel.
      * @param name a constant string reference representing the name of the channel.
      * @param type a constant DVBLinkChannelType instance representing the type of the channel.
      * @param number an optional constant integer representing the number of the channel.
      * @param subNumber an optional constant integer representing the sub-number of the channel.
      */
    Channel(const std::string& id, const long dvbLinkId, const std::string& name, const DVBLinkChannelType type, const int number = -1, const int subNumber = -1);

    /**
      * Initializes a new instance of the dvblinkremote::Channel class by coping another 
      * dvblinkremote::Channel instance.
      * @param channel a dvblinkremote::Channel reference.
      */
    Channel(Channel& channel);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~Channel();

    /**
      * Gets the identifier of the channel.
      * @return Channel identifier
      */
    std::string& GetID();

    /**
      * Gets the DVBLink identifier of the channel. 
      * @return DVBLink channel identifier
      */
    long GetDvbLinkID();
    
    /**
      * Gets the name of the channel.
      * @return Channel name
      */
    std::string& GetName();

    /**
      * Gets the type of the channel.
      * @return ChannelType instance reference
      */
    DVBLinkChannelType& GetChannelType();

    /**
      * Represents the number of the channel.
      */
    int Number;

    /**
      * Represents the sub-number of the channel.
      */
    int SubNumber;

    /**
      * Represents if a child lock is active or not for the channel.
      */
    bool ChildLock;
    
  private:
    std::string m_id;
    long m_dvbLinkId;
    std::string m_name;
    DVBLinkChannelType m_type;
  };

  /**
    * Represent a strongly typed list of DVBLink channels which is used as output 
    * parameter for the IDVBLinkRemoteConnection::GetChannels method.
    * @see Channel::Channel()
    * @see IDVBLinkRemoteConnection::GetChannels()
    */
  class ChannelList : public Response, public std::vector<Channel*> {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::ChannelList class.
      */
    ChannelList();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~ChannelList();
  };

  /**
    * Represent metadata for an item.
    */
  class ItemMetadata
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::ItemMetadata class.
      */
    ItemMetadata();

    /**
      * Initializes a new instance of the dvblinkremote::MetaData class.
      * @param title a constant string reference representing the title of the item.
      * @param startTime a constant long representing the start time of the item.
      * @param duration a constant long representing the duration of the item.
      * \remark startTime and duration is the number of seconds, counted from 
      * UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    ItemMetadata(const std::string& title, const long startTime, const long duration);

    /**
      * Initializes a new instance of the dvblinkremote::ItemMetadata class by coping another 
      * dvblinkremote::ItemMetadata instance.
      * @param itemMetadata a dvblinkremote::ItemMetadata reference.
      */
    ItemMetadata(ItemMetadata& itemMetadata);

    /**
      * Pure virtual destructor for cleaning up allocated memory.
      */
    virtual ~ItemMetadata() = 0;

    /**
      * Gets the title of the item.
      * @return Item title
      */
    std::string& GetTitle();
    
    /**
      * Sets the title of the item.
      * @param title a constant string reference representing the title of the item.
      */
    void SetTitle(const std::string& title);

    /**
      * Gets the start time of the item.
      * @return Item start time
      * \remark Number of seconds, counted from UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    long GetStartTime();

    /**
      * Sets the start time of the item.
      * @param startTime a constant long representing the start time of the item.
      */
    void SetStartTime(const long startTime);

    /**
      * Gets the duration of the item.
      * @return item duration
      * \remark Number of seconds, counted from UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    long GetDuration();

    /**
      * Sets the duration of the item.
      * @param duration a constant long representing the duration of the item.
      */
    void SetDuration(const long duration);

    std::string ShortDescription;
    std::string SubTitle;
    std::string Language;
    std::string Actors;
    std::string Directors;
    std::string Writers;
    std::string Producers;
    std::string Guests;
    std::string Keywords;
    std::string Image;
    long Year;
    long EpisodeNumber;
    long SeasonNumber;
    long Rating;
    long MaximumRating;
    bool IsHdtv;
    bool IsPremiere;
    bool IsRepeat;
    bool IsSeries;
    bool IsRecord;
    bool IsRepeatRecord;
    bool IsCatAction;
    bool IsCatComedy;
    bool IsCatDocumentary;
    bool IsCatDrama;
    bool IsCatEducational;
    bool IsCatHorror;
    bool IsCatKids;
    bool IsCatMovie;
    bool IsCatMusic;
    bool IsCatNews;
    bool IsCatReality;
    bool IsCatRomance;
    bool IsCatScifi;
    bool IsCatSerial;
    bool IsCatSoap;
    bool IsCatSpecial;
    bool IsCatSports;
    bool IsCatThriller;
    bool IsCatAdult;

  private:
    std::string m_title;
    long m_startTime;
    long m_duration;
  };

  /**
    * Represent a program in an electronic program guide (EPG).
    * @see ItemMetadata::ItemMetadata()
    */
  class Program : public ItemMetadata
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::Program class.
      */
    Program();

    /**
      * Initializes a new instance of the dvblinkremote::Program class.
      * @param id a constant string reference representing the identifier of the program.
      * @param title a constant string reference representing the title of the program.
      * @param startTime a constant long representing the start time of the program.
      * @param duration a constant long representing the duration of the program.
      * \remark startTime and duration is the number of seconds, counted from 
      * UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    Program(const std::string& id, const std::string& title, const long startTime, const long duration);

    /**
      * Initializes a new instance of the dvblinkremote::Program class by coping another 
      * dvblinkremote::Program instance.
      * @param program a dvblinkremote::Program reference.
      */
    Program(Program& program);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~Program();

    /**
      * Gets the identifier of the program.
      * @return Program identifier
      */
    std::string& GetID();

    /**
      * Sets the identifier of the program.
      * @param id a constant string reference representing the identifier of the program.
      */
    void SetID(const std::string& id);

  private:
    std::string m_id;
  };

  /**
    * Represent a strongly typed list of programs in an electronic 
    * program guide (EPG).
    * @see Program::Program()
    */
  class EpgData : public std::vector<Program*> {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::EpgData class.
      */
    EpgData();

    /**
      * Initializes a new instance of the dvblinkremote::EpgData class by coping another 
      * dvblinkremote::EpgData instance.
      * @param epgData a dvblinkremote::EpgData reference.
      */
    EpgData(EpgData& epgData);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~EpgData();
  };

  /**
    * Represent electronic program guide (EPG) data for a channel.
    */
  class ChannelEpgData
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::ChannelEpgData class.
      * @param channelId a constant string reference representing the channel identifier for the corresponding electronic program guide (EPG) data.
      */
    ChannelEpgData(const std::string& channelId);

    /**
      * Initializes a new instance of the dvblinkremote::ChannelEpgData class by coping another 
      * dvblinkremote::ChannelEpgData instance.
      * @param channelEpgData a dvblinkremote::ChannelEpgData reference.
      */
    ChannelEpgData(ChannelEpgData& channelEpgData);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~ChannelEpgData();

    /**
      * Get the channel identifier of the electronic program guide (EPG) data.
      * @return Channel identifier.
      */
    std::string& GetChannelID();

    /**
      * Get the electronic program guide (EPG) data.
      * @return Electronic program guide (EPG) data.
      */
    EpgData& GetEpgData();

    /**
      * Adds a program to the electronic program guide (EPG).
      * @param program A constant dvblinkremote::Program pointer.
      */
    void AddProgram(const Program* program);

  private:
    std::string m_channelId;
    EpgData* m_epgData;
  };

  /**
    * Represent a strongly typed list of electronic program guide (EPG) data 
    * for channels which is used as output parameter for the 
    * IDVBLinkRemoteConnection::SearchEpg method.
    * @see ChannelEpgData::ChannelEpgData()
    * @see IDVBLinkRemoteConnection::SearchEpg()
    */
  class EpgSearchResult : public Response, public std::vector<ChannelEpgData*> {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::EpgSearchResult class.
      */
    EpgSearchResult();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~EpgSearchResult();
  };

  /**
  * Represent a DVBLink playing channel which is used as output parameter 
  * for the IDVBLinkRemoteConnection::PlayChannel method.
  * @see IDVBLinkRemoteConnection::PlayChannel()
  */
  class Stream : public Response
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::Stream class.
      */
    Stream();

    /**
      * Initializes a new instance of the dvblinkremote::Stream class.
      * @param channelHandle a constant long representing the channel handle of the stream.
      * @param url a constant string reference representing the url of the stream.
      */
    Stream(const long channelHandle, const std::string& url);
    
    /**
      * Initializes a new instance of the dvblinkremote::Stream class by coping another 
      * dvblinkremote::Stream instance.
      * @param stream a dvblinkremote::Stream reference.
      */
    Stream(Stream& stream);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~Stream();
    
    /**
      * Gets the channel handle of the stream.
      * @return Stream channel handle
      */
    long GetChannelHandle();
    
    /**
      * Sets the channel handle of the stream.
      * @param channelHandle a constant long representing the channel handle of the stram.
      */
    void SetChannelHandle(const long channelHandle);
    
    /**
      * Gets the url of the stream.
      * @return Stream url
      */
    std::string& GetUrl();
    
    /**
      * Sets the url of the stream.
      * @param url a constant string reference representing the url of the stram.
      */
    void SetUrl(const std::string& url);

  private:
    long m_channelHandle;
    std::string m_url;
  };

  /**
    * Represent a recording.
    */
  class Recording
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::Recording class.
      * @param id a constant string reference representing the identifier of the recording.
      * @param scheduleId a constant string reference representing the schedule identifier of the recording.
      * @param channelId a constant string reference representing the channel identifier of the recording.
      * @param program a constant dvblinkremote::Program instance pointer representing the program of the recording.
      */
    Recording(const std::string& id, const std::string& scheduleId, const std::string& channelId, const Program* program);
    
    /**
      * Initializes a new instance of the dvblinkremote::Recording class by coping another 
      * dvblinkremote::Recording instance.
      * @param recording a dvblinkremote::Recording reference.
      */
    Recording(Recording& recording);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~Recording();

    /**
      * Gets the identifier of the recording.
      * @return Recording identifier
      */
    std::string& GetID();
    
    /**
      * Gets the schedule identifier of the recording.
      * @return Recording schedule identifier
      */
    std::string& GetScheduleID();
    
    /**
      * Gets the channel identifier of the recording.
      * @return Recording channel identifier
      */
    std::string& GetChannelID();

    /**
      * Represents if the recording is in active state or not.
      */
    bool IsActive;

    /**
      * Represents if the recording is conflicting with another recording.
      */
    bool IsConflict;

    /**
      * Gets the program of the recording.
      * @return Recording program
      */
    Program& GetProgram();

  private:
    std::string m_id;
    std::string m_scheduleId;
    std::string m_channelId;
    Program* m_program;
  };

  /**
    * Represent a strongly typed list of recordings which is used as output 
    * parameter for the IDVBLinkRemoteConnection::GetRecordings method.
    * @see Recording::Recording()
    * @see IDVBLinkRemoteConnection::GetRecordings()
    */
  class RecordingList : public Response, public std::vector<Recording*>
  {
  public:
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RecordingList();
  };

  /**
    * Class for stored manual schedules.
    */
  class StoredManualSchedule : public ManualSchedule 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::StoredManualSchedule class.
      * @param id a constant string reference representing the schedule identifier.
      * @param channelId a constant string reference representing the channel identifier.
      * @param startTime a constant long representing the start time of the schedule.
      * @param duration a constant long representing the duration of the schedule. 
      * @param dayMask a constant long representing the day bitflag of the schedule.
      * \remark Construct the \p dayMask parameter by using bitwize operations on the DVBLinkManualScheduleDayMask.
      * @see DVBLinkManualScheduleDayMask
      * @param title of schedule
      */
    StoredManualSchedule(const std::string& id, const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title = "");

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StoredManualSchedule();
  };

  /**
    * Represent a strongly typed list of stored manual schedules.
    * @see StoredManualSchedule::StoredManualSchedule()
    */
  class StoredManualScheduleList : public std::vector<StoredManualSchedule*>
  {
  public:
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StoredManualScheduleList();
  };

  /**
    * Class for stored EPG schedules.
    */
  class StoredEpgSchedule : public EpgSchedule 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::StoredEpgSchedule class.
      * @param id a constant string reference representing the schedule identifier.
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
    StoredEpgSchedule(const std::string& id, const std::string& channelId, const std::string& programId, const bool repeat = false, const bool newOnly = false, const bool recordSeriesAnytime = false);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StoredEpgSchedule();
  };

  /**
    * Represent a strongly typed list of stored EPG schedules.
    * @see StoredEpgSchedule::StoredEpgSchedule()
    */
  class StoredEpgScheduleList : public std::vector<StoredEpgSchedule*>
  {
  public:
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StoredEpgScheduleList();
  };

  /**
    * Represent stored schedules which is used as output paramater for 
    * the IDVBLinkRemoteConnection::GetSchedules method.
    * @see IDVBLinkRemoteConnection::GetSchedules()
    */
  class StoredSchedules : public Response
  {
  public:
    StoredSchedules();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StoredSchedules();

    /**
      * Gets a list of stored manual schedules.
      * @return A list of stored manual schedules
      */
    StoredManualScheduleList& GetManualSchedules();
    
    /**
      * Gets a list of stored EPG schedules.
      * @return A list of stored EPG schedules
      */
    StoredEpgScheduleList& GetEpgSchedules();

  private:
    StoredManualScheduleList* m_manualScheduleList;
    StoredEpgScheduleList* m_epgScheduleList;
  };  

  /**
  * Represent parental status which is used as output parameter for the 
  * IDVBLinkRemoteConnection::SetParentalLock and IDVBLinkRemoteConnection::GetParentalStatus 
  * methods.
  * @see IDVBLinkRemoteConnection::SetParentalLock()
  * @see IDVBLinkRemoteConnection::GetParentalStatus()
  */
  class ParentalStatus : public Response
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::ParentalStatus class.
      */
    ParentalStatus();
    
    /**
      * Initializes a new instance of the dvblinkremote::ParentalStatus class by coping another 
      * dvblinkremote::ParentalStatus instance.
      * @param parentalStatus a dvblinkremote::ParentalStatus reference.
      */
    ParentalStatus(ParentalStatus& parentalStatus);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~ParentalStatus();

    /**
      * Represents if the parental lock is enabled or not. 
      */
    bool IsEnabled;
  };

  /**
  * Represent M3U playlist which is used as output parameter for the 
  * IDVBLinkRemoteConnection::GetM3uPlaylist method.
  * @see IDVBLinkRemoteConnection::GetM3uPlaylist()
  */
  class M3uPlaylist : public Response
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::M3uPlaylist class.
      */
    M3uPlaylist();

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~M3uPlaylist();

    /**
      * Represents the M3U playlist file content. 
      */
    std::string FileContent;
  };

  /**
    * Abstract base class for playback objects.
    */
  class PlaybackObject
  {
  public:
    /**
      * An enum for playback object types.
      */
    enum DVBLinkPlaybackObjectType {
      PLAYBACK_OBJECT_TYPE_CONTAINER = 0, /**< Container object type */ 
      PLAYBACK_OBJECT_TYPE_ITEM = 1 /**< Item object type */ 
    };

    /**
      * Initializes a new instance of the dvblinkremote::PlaybackObject class.
      * @param itemType a constant DVBLinkPlaybackObjectType instance representing the type of 
      * the playback item.
      * @param objectId a constant string reference representing the identifier of the playback object.
      * @param parentId a constant string reference representing the identifier of the parent object 
      * for this playback object.
      */
    PlaybackObject(const DVBLinkPlaybackObjectType objectType, const std::string& objectId, const std::string& parentId);
    
    /**
      * Pure virtual destructor for cleaning up allocated memory.
      */
    virtual ~PlaybackObject() = 0;

    /**
      * Gets the type of the playback object .
      * @return DVBLinkPlaybackObjectType instance reference
      */
    DVBLinkPlaybackObjectType& GetObjectType();

    /**
      * Gets the identifier of the playback object.
      * @return Playback object identifier
      */
    std::string& GetObjectID();

    /**
      * Gets the identifier of the parent object for this playback object.
      * @return Parent identifier for this playback object
      */
    std::string& GetParentID();

  private:
    /**
      * The type of object.
      */
    DVBLinkPlaybackObjectType m_objectType;

    /**
      * The identifier for the playback object.
      */
    std::string m_objectId;

    /**
      * The identifier of the parent object to this playback object.
      */
    std::string m_parentId;
  };

  /**
    * Class for playback container.
    */
  class PlaybackContainer : public PlaybackObject
  {
  public :
    /**
      * An enum for playback container types.
      */
    enum DVBLinkPlaybackContainerType {
      PLAYBACK_CONTAINER_TYPE_UNKNOWN = -1, /**< Unknown container type */ 
      PLAYBACK_CONTAINER_TYPE_SOURCE = 0, /**< Source container */ 
      PLAYBACK_CONTAINER_TYPE_TYPE = 1, /**< Type container */ 
      PLAYBACK_CONTAINER_TYPE_CATEGORY = 2, /**< Category container */ 
      PLAYBACK_CONTAINER_TYPE_GROUP = 3 /**< Group container */ 
    };

    /**
      * An enum for playback container content types.
      */
    enum DVBLinkPlaybackContainerContentType {
      PLAYBACK_CONTAINER_CONTENT_TYPE_UNKNOWN = -1, /**< Unknown content type */ 
      PLAYBACK_CONTAINER_CONTENT_TYPE_RECORDED_TV = 0, /**< Recorded TV content */ 
      PLAYBACK_CONTAINER_CONTENT_TYPE_VIDEO = 1, /**< Video content */ 
      PLAYBACK_CONTAINER_CONTENT_TYPE_AUDIO = 2, /**< Audio content */ 
      PLAYBACK_CONTAINER_CONTENT_TYPE_IMAGE = 3 /**< Image content */ 
    };

    /**
      * Initializes a new instance of the dvblinkremote::PlaybackContainer class.
      * @param objectId a constant string reference representing the identifier of the playback object.
      * @param parentId a constant string reference representing the identifier of the parent object 
      * for this playback object.
      * @param name a constant string reference representing the name of the playback container.
      * @param containerType a constant DVBLinkPlaybackContainerType instance representing the type of 
      * the playback container.
      * @param containerContentType a constant DVBLinkPlaybackContainerContentType instance representing the content 
      * type of the playback items in this playback container.
      */
    PlaybackContainer(const std::string& objectId, const std::string& parentId, const std::string& name, DVBLinkPlaybackContainerType& containerType, DVBLinkPlaybackContainerContentType& containerContentType);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~PlaybackContainer();

    /**
      * Gets the name of the playback container.
      * @return Playback container name
      */
    std::string& GetName();

    /**
      * Gets the type of the playback container .
      * @return DVBLinkPlaybackContainerType instance reference
      */
    DVBLinkPlaybackContainerType& GetContainerType();

    /**
      * Gets the content type of the playback items in this playback container .
      * @return DVBLinkPlaybackContainerContentType instance reference
      */
    DVBLinkPlaybackContainerContentType& GetContainerContentType();

    /**
      * The description of the playback container.
      */
    std::string Description;

    /**
      * The logo of the playback container.
      */
    std::string Logo;

    /**
      * The total amount of items in the playback container.
      */
    int TotalCount;

    /**
      * Identifies a physical source of this container.
      * \remark 8F94B459-EFC0-4D91-9B29-EC3D72E92677 is the 
      * built-in dvblink recorder, e.g. Recorded TV items.
      */
    std::string SourceID;

  private:
    std::string m_name;
    DVBLinkPlaybackContainerType& m_containerType;
    DVBLinkPlaybackContainerContentType& m_containerContentType;
  };
  
  /**
    * Represent a strongly typed list of playback containers.
    * @see PlaybackContainer::PlaybackContainer()
    */
  class PlaybackContainerList :  public std::vector<PlaybackContainer*>
  {
  public:
    /**
      * Destructor for cleaning up allocated memory.
      */
   ~PlaybackContainerList(); 
  };

  /**
    * Abstract base class for playback items.
    */
  class PlaybackItem : public PlaybackObject
  {
  public:
    /**
      * An enum for playback item types.
      */
    enum DVBLinkPlaybackItemType {
      PLAYBACK_ITEM_TYPE_RECORDED_TV = 0, /**< Recorded TV item */ 
      PLAYBACK_ITEM_TYPE_VIDEO = 1, /**< Video item */ 
      PLAYBACK_ITEM_TYPE_AUDIO = 2, /**< Audio item */ 
      PLAYBACK_ITEM_TYPE_IMAGE = 3 /**< Image item */ 
    };

    /**
      * Initializes a new instance of the dvblinkremote::PlaybackItem class.
      * @param itemType a constant DVBLinkPlaybackItemType instance representing the type of 
      * the playback item.
      * @param objectId a constant string reference representing the identifier of the playback object.
      * @param parentId a constant string reference representing the identifier of the parent object 
      * for this playback object.
      * @param playbackUrl a constant string reference representing the URL for stream playback of the playback item.
      * @param thumbnailUrl a constant string reference representing the URL to the playback item thumbnail.
      * @param metadata a constant ItemMetadata reference representing the metadata for the playback item.
      */
    PlaybackItem(const DVBLinkPlaybackItemType itemType, const std::string& objectId, const std::string& parentId, const std::string& playbackUrl, const std::string& thumbnailUrl, const ItemMetadata* metadata);
    
    /**
      * Pure virtual destructor for cleaning up allocated memory.
      */
    virtual ~PlaybackItem() = 0;

    /**
      * Gets the type of the playback item .
      * @return DVBLinkPlaybackItemType instance reference
      */
    DVBLinkPlaybackItemType& GetItemType();

    /**
      * Gets the URL for stream playback of the playback item.
      * @return Playback URL
      */
    std::string& GetPlaybackUrl();

    /**
      * Gets the URL to the playback item thumbnail.
      * @return Thumbnail URL
      */
    std::string& GetThumbnailUrl();

    /**
      * Gets the metadata for the playback item.
      * @return ItemMetadata instance reference 
      */
    ItemMetadata& GetMetadata();

    /**
      * Identifies whether this item can be deleted.
      */
    bool CanBeDeleted;

    /**
      * Item file size in bytes.
      */
    long Size;

    /**
      * Time when item was created.
      * \remark Number of seconds, counted from UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    long CreationTime;

  private:
    /**
      * The type of item.
      */
    DVBLinkPlaybackItemType m_itemType;

    /**
      * The URL for stream playback of the playback item.
      */
    std::string m_playbackUrl;

    /**
      * The URL to the playback item thumbnail.
      */
    std::string m_thumbnailUrl;

    /**
      * The metadata of the playback item.
      */
    ItemMetadata* m_metadata;
  };

  /**
    * Represent metadata for a recorded TV item.
    * @see ItemMetadata::ItemMetadata()
    */
  class RecordedTvItemMetadata : public ItemMetadata
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::RecordedTvItemMetadata class.
      */
    RecordedTvItemMetadata();

    /**
      * Initializes a new instance of the dvblinkremote::RecordedTvItemMetadata class.
      * @param title a constant string reference representing the title of a recorded TV item.
      * @param startTime a constant long representing the start time of a recorded TV item.
      * @param duration a constant long representing the duration of a recorded TV item.
      * \remark startTime and duration is the number of seconds, counted from 
      * UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    RecordedTvItemMetadata(const std::string& title, const long startTime, const long duration);

    /**
      * Initializes a new instance of the dvblinkremote::RecordedTvItemMetadata class by coping another 
      * dvblinkremote::RecordedTvItemMetadata instance.
      * @param recordedTvItemMetadata a dvblinkremote::RecordedTvItemMetadata reference.
      */
    RecordedTvItemMetadata(RecordedTvItemMetadata& recordedTvItemMetadata);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RecordedTvItemMetadata();
  };

  /**
    * Class for recorded TV items.
    */
  class RecordedTvItem : public PlaybackItem
  {
  public:
    /**
      * An enum representing the state of the recorded TV item.
      */
    enum DVBLinkRecordedTvItemState {
      RECORDED_TV_ITEM_STATE_IN_PROGRESS = 0, /**< Recording is in progress */ 
      RECORDED_TV_ITEM_STATE_ERROR = 1, /**< Recording not started because of error */ 
      RECORDED_TV_ITEM_STATE_FORCED_TO_COMPLETION = 2, /**< Recording was forced to completion, but may miss certain part at the end because it was cancelled by user */ 
      RECORDED_TV_ITEM_STATE_COMPLETED = 3 /**< Recording completed successfully */ 
    };

    /**
      * Initializes a new instance of the dvblinkremote::RecordedTvPlaybackItem class.
      * @param objectId a constant string reference representing the identifier of the playback object.
      * @param parentId a constant string reference representing the identifier of the parent object 
      * for this playback object.
      * @param playbackUrl a constant string reference representing the URL for stream playback of the recorded tv item.
      * @param thumbnailUrl a constant string reference representing the URL to the recorded tv item thumbnail.
      * @param metadata a constant RecordedTvItemMetadata reference representing the metadata for the recorded tv item.
      */
    RecordedTvItem(const std::string& objectId, const std::string& parentId, const std::string& playbackUrl, const std::string& thumbnailUrl, const RecordedTvItemMetadata* metadata);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RecordedTvItem();

    /**
      * The channel name of the recorded TV item.
      */
    std::string ChannelName;

    /**
      * The channel number of the recorded TV item.
      */
    int ChannelNumber;

    /**
      * The channel sub number of the recorded TV item.
      */
    int ChannelSubNumber;

    /**
      * The state of the recored TV item.
      */
    DVBLinkRecordedTvItemState State;
  };

  /**
    * Represent metadata for a video.
    * @see ItemMetadata::ItemMetadata()
    */
  class VideoItemMetadata : public ItemMetadata
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::VideoItemMetadata class.
      */
    VideoItemMetadata();

    /**
      * Initializes a new instance of the dvblinkremote::VideoItemMetadata class.
      * @param title a constant string reference representing the title of a video item.
      * @param startTime a constant long representing the start time of a video item.
      * @param duration a constant long representing the duration of a video item.
      * \remark startTime and duration is the number of seconds, counted from 
      * UNIX epoc: 00:00:00 UTC on 1 January 1970.
      */
    VideoItemMetadata(const std::string& title, const long startTime, const long duration);

    /**
      * Initializes a new instance of the dvblinkremote::VideoItemMetadata class by coping another 
      * dvblinkremote::VideoItemMetadata instance.
      * @param videoItemMetadata a dvblinkremote::VideoItemMetadata reference.
      */
    VideoItemMetadata(VideoItemMetadata& videoItemMetadata);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~VideoItemMetadata();
  };

  /**
    * Class for video items.
    */
  class VideoItem : public PlaybackItem
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::VideoItem class.
      * @param objectId a constant string reference representing the identifier of the playback object.
      * @param parentId a constant string reference representing the identifier of the parent object 
      * for this playback object.
      * @param playbackUrl a constant string reference representing the URL for stream playback of the video item.
      * @param thumbnailUrl a constant string reference representing the URL to the video item thumbnail.
      * @param metadata a constant VideoItemMetadata reference representing the metadata for the video item.
      */
    VideoItem(const std::string& objectId, const std::string& parentId, const std::string& playbackUrl, const std::string& thumbnailUrl, const VideoItemMetadata* metadata);
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~VideoItem();
  };
   
  /**
    * Represent a strongly typed list of playback items.
    * @see PlaybackItem::PlaybackItem()
    */
  class PlaybackItemList : public std::vector<PlaybackItem*>
  {
  public:
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~PlaybackItemList(); 
  };

  /**
    * Represent playback object response which is used as output parameter 
    * for the IDVBLinkRemoteConnection::GetPlaybackObject method. 
    * @see IDVBLinkRemoteConnection::GetPlaybackObject()
    */
  class GetPlaybackObjectResponse : public Response
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::GetPlaybackObjectResponse class.
      */
    GetPlaybackObjectResponse();
    
    /**
      * Destructor for cleaning up allocated memory.
      */
    ~GetPlaybackObjectResponse();

    /**
      * Gets a list of playback containers.
      * @return A list of playback containers
      */
    PlaybackContainerList& GetPlaybackContainers();

    /**
      * Gets a list of playback items.
      * @return A list of playback items
      */
    PlaybackItemList& GetPlaybackItems();

    /**
      * The number of items and containers in the response.
      */
    int ActualCount;

    /**
      * The total number of items and containers in the container.
      */
    int TotalCount;

  private:
    PlaybackContainerList* m_playbackContainerList;
    PlaybackItemList* m_playbackItemList;
  };

  /**
  * Represent streaming capabilities which is used as output parameter for the 
  * IDVBLinkRemoteConnection::GetStreamingCapabilities method.
  * @see IDVBLinkRemoteConnection::GetStreamingCapabilities()
  */
  class StreamingCapabilities : public Response
  {
  public:
    /**
      * An enum for supported streaming protocols.
      */
    enum DVBLinkSupportedProtocol {
      SUPPORTED_PROTOCOL_NONE = 0, /**< No streaming protocol supported */ 
      SUPPORTED_PROTOCOL_HTTP = 1, /**< HTTP protocol supported */ 
      SUPPORTED_PROTOCOL_UDP = 2, /**< UDP protocol supported */ 
      SUPPORTED_PROTOCOL_RTSP = 4, /**< Real Time Streaming Protocol (RTSP) supported */ 
      SUPPORTED_PROTOCOL_ASF = 8, /**< Windows Media Stream (ASF) protocol supported */ 
      SUPPORTED_PROTOCOL_HLS = 16, /**< HTTP Live Streaming (HLS) protocol supported */ 
      SUPPORTED_PROTOCOL_WEBM = 32, /**< Open Web Media (WebM) protocol supported */ 
      SUPPORTED_PROTOCOL_ALL = 65535 /**< All streaming protocols supported */ 
    };

    /**
      * An enum for supported streaming transcoders.
      */
    enum DVBLinkSupportedTranscoder {
      STREAMING_TRANSCODER_NONE = 0, /**< No streaming transcoder supported */ 
      STREAMING_TRANSCODER_WMV = 1, /**< Windows Media Video (WMV) transcoder supported */ 
      STREAMING_TRANSCODER_WMA = 2, /**< Windows Media Audio (WMA) transcoder supported */ 
      STREAMING_TRANSCODER_H264 = 4, /**< Advanced Video Coding (H.264) transcoder supported */ 
      STREAMING_TRANSCODER_AAC = 8, /**< Advanced Audio Coding (AAC) transcoder supported */ 
      STREAMING_TRANSCODER_RAW = 16, /**< Raw transcoder supported */ 
      STREAMING_TRANSCODER_ALL = 65535 /**< All streaming transcoders supported */ 
    };

    /**
      * Initializes a new instance of the dvblinkremote::StreamingCapabilities class.
      */
    StreamingCapabilities();
    
    /**
      * Initializes a new instance of the dvblinkremote::StreamingCapabilities class by coping another 
      * dvblinkremote::StreamingCapabilities instance.
      * @param streamingCapabilities a dvblinkremote::StreamingCapabilities reference.
      */
    StreamingCapabilities(StreamingCapabilities& streamingCapabilities);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~StreamingCapabilities();

    int SupportedProtocols;

    bool IsProtocolSupported(const DVBLinkSupportedProtocol protocol);
    bool IsProtocolSupported(const int protocolsToCheck);

    int SupportedTranscoders;

    bool IsTranscoderSupported(const DVBLinkSupportedTranscoder transcoder);
    bool IsTranscoderSupported(const int transcodersToCheck);
  };

  /**
  * Represent recording settings which is used as output parameter for the 
  * IDVBLinkRemoteConnection::GetRecordingSettings method.
  * @see IDVBLinkRemoteConnection::GetRecordingSettings()
  */
  class RecordingSettings : public Response
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::RecordingSettings class.
      */
    RecordingSettings();
    
    /**
      * Initializes a new instance of the dvblinkremote::RecordingSettings class by coping another 
      * dvblinkremote::RecordingSettings instance.
      * @param recordingSettings a dvblinkremote::RecordingSettings reference.
      */
    RecordingSettings(RecordingSettings& recordingSettings);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~RecordingSettings();

    /**
      * The configured time margin before a schedule recording is started.
      */
    int TimeMarginBeforeScheduledRecordings;

    /**
      * The configured time margin after a schedule recording is stopped.
      */
    int TimeMarginAfterScheduledRecordings;

    /**
      * The file system path where recordings will be stored. 
      */
    std::string RecordingPath;

    /**
      * The total space in KB.
      */
    long TotalSpace;

    /**
      * The available space in KB. 
      */
    long AvailableSpace;
  };
}
