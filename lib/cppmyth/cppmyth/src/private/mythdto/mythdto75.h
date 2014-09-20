/*
 *      Copyright (C) 2014 Jean-Luc Barriere
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef MYTHDTO75_H
#define	MYTHDTO75_H

#include "mythdto.h"
#include "version.h"
#include "list.h"
#include "program.h"
#include "channel.h"
#include "recording.h"
#include "artwork.h"
#include "capturecard.h"
#include "videosource.h"
#include "recordschedule.h"

namespace MythDTO75
{
  attr_bind_t version2_0[] =
  {
    { "Version",        IS_STRING,  (setter_t)MythDTOVersion::SetVersion_Version },
    { "Protocol",       IS_UINT32,  (setter_t)MythDTOVersion::SetVersion_Protocol },
    { "Schema",         IS_UINT32,  (setter_t)MythDTOVersion::SetVersion_Schema },
  };
  bindings_t VersionBindArray2_0 = { sizeof(version2_0) / sizeof(attr_bind_t), version2_0 };

  attr_bind_t list[] =
  {
    { "Count",          IS_UINT32,  (setter_t)MythDTOList::SetItemList_Count },
    { "ProtoVer",       IS_UINT32,  (setter_t)MythDTOList::SetItemList_ProtoVer },
  };
  bindings_t ListBindArray = { sizeof(list) / sizeof(attr_bind_t), list };

  attr_bind_t artwork[] =
  {
    { "URL",            IS_STRING,  (setter_t)MythDTOArtwork::SetArtwork_URL },
    { "FileName",       IS_STRING,  (setter_t)MythDTOArtwork::SetArtwork_FileName },
    { "StorageGroup",   IS_STRING,  (setter_t)MythDTOArtwork::SetArtwork_StorageGroup },
    { "Type",           IS_STRING,  (setter_t)MythDTOArtwork::SetArtwork_Type },
  };
  bindings_t ArtworkBindArray = { sizeof(artwork) / sizeof(attr_bind_t), artwork };

  attr_bind_t channel[] =
  {
    { "ChanId",         IS_UINT32,  (setter_t)MythDTOChannel::SetChannel_ChanId },
    { "ChanNum",        IS_STRING,  (setter_t)MythDTOChannel::SetChannel_ChanNum },
    { "CallSign",       IS_STRING,  (setter_t)MythDTOChannel::SetChannel_CallSign },
    { "IconURL",        IS_STRING,  (setter_t)MythDTOChannel::SetChannel_IconURL },
    { "ChannelName",    IS_STRING,  (setter_t)MythDTOChannel::SetChannel_ChannelName },
    { "MplexId",        IS_UINT32,  (setter_t)MythDTOChannel::SetChannel_MplexId },
    { "CommFree",       IS_STRING,  (setter_t)MythDTOChannel::SetChannel_CommFree },
    { "ChanFilters",    IS_STRING,  (setter_t)MythDTOChannel::SetChannel_ChanFilters },
    { "SourceId",       IS_UINT32,  (setter_t)MythDTOChannel::SetChannel_SourceId },
    { "InputId",        IS_UINT32,  (setter_t)MythDTOChannel::SetChannel_InputId },
    { "Visible",        IS_BOOLEAN, (setter_t)MythDTOChannel::SetChannel_Visible },
  };
  bindings_t ChannelBindArray = { sizeof(channel) / sizeof(attr_bind_t), channel };

  attr_bind_t recording[] =
  {
    { "RecordId",       IS_UINT32,  (setter_t)MythDTORecording::SetRecording_RecordId },
    { "Priority",       IS_INT32,   (setter_t)MythDTORecording::SetRecording_Priority },
    { "Status",         IS_INT8,    (setter_t)MythDTORecording::SetRecording_Status },
    { "EncoderId",      IS_UINT32,  (setter_t)MythDTORecording::SetRecording_EncoderId },
    { "RecType",        IS_UINT8,   (setter_t)MythDTORecording::SetRecording_RecType },
    { "DupInType",      IS_UINT8,   (setter_t)MythDTORecording::SetRecording_DupInType },
    { "DupMethod",      IS_UINT8,   (setter_t)MythDTORecording::SetRecording_DupMethod },
    { "StartTs",        IS_TIME,    (setter_t)MythDTORecording::SetRecording_StartTs },
    { "EndTs",          IS_TIME,    (setter_t)MythDTORecording::SetRecording_EndTs },
    { "Profile",        IS_STRING,  (setter_t)MythDTORecording::SetRecording_Profile },
    { "RecGroup",       IS_STRING,  (setter_t)MythDTORecording::SetRecording_RecGroup },
    { "StorageGroup",   IS_STRING,  (setter_t)MythDTORecording::SetRecording_StorageGroup },
    { "PlayGroup",      IS_STRING,  (setter_t)MythDTORecording::SetRecording_PlayGroup },
  };
  bindings_t RecordingBindArray = { sizeof(recording) / sizeof(attr_bind_t), recording };

  attr_bind_t program[] =
  {
    { "StartTime",      IS_TIME,    (setter_t)MythDTOProgram::SetProgram_StartTime },
    { "EndTime",        IS_TIME,    (setter_t)MythDTOProgram::SetProgram_EndTime },
    { "Title",          IS_STRING,  (setter_t)MythDTOProgram::SetProgram_Title },
    { "SubTitle",       IS_STRING,  (setter_t)MythDTOProgram::SetProgram_SubTitle },
    { "Description",    IS_STRING,  (setter_t)MythDTOProgram::SetProgram_Description },
    { "Season",         IS_UINT16,  (setter_t)MythDTOProgram::SetProgram_Season },
    { "Episode",        IS_UINT16,  (setter_t)MythDTOProgram::SetProgram_Episode },
    { "Category",       IS_STRING,  (setter_t)MythDTOProgram::SetProgram_Category },
    { "CatType",        IS_STRING,  (setter_t)MythDTOProgram::SetProgram_CatType},
    { "HostName",       IS_STRING,  (setter_t)MythDTOProgram::SetProgram_HostName },
    { "FileName",       IS_STRING,  (setter_t)MythDTOProgram::SetProgram_FileName },
    { "FileSize",       IS_INT64,   (setter_t)MythDTOProgram::SetProgram_FileSize },
    { "Repeat",         IS_BOOLEAN, (setter_t)MythDTOProgram::SetProgram_Repeat },
    { "ProgramFlags",   IS_INT32,   (setter_t)MythDTOProgram::SetProgram_ProgramFlags },
    { "SeriesId",       IS_STRING,  (setter_t)MythDTOProgram::SetProgram_SeriesId },
    { "ProgramId",      IS_STRING,  (setter_t)MythDTOProgram::SetProgram_ProgramId },
    { "Inetref",        IS_STRING,  (setter_t)MythDTOProgram::SetProgram_Inetref },
    { "LastModified",   IS_TIME,    (setter_t)MythDTOProgram::SetProgram_LastModified },
    { "Stars",          IS_STRING,  (setter_t)MythDTOProgram::SetProgram_Stars },
    { "Airdate",        IS_TIME,    (setter_t)MythDTOProgram::SetProgram_Airdate },
    { "AudioProps",     IS_UINT16,  (setter_t)MythDTOProgram::SetProgram_AudioProps },
    { "VideoProps",     IS_UINT16,  (setter_t)MythDTOProgram::SetProgram_VideoProps },
    { "SubProps",       IS_UINT16,  (setter_t)MythDTOProgram::SetProgram_SubProps },
  };
  bindings_t ProgramBindArray = { sizeof(program) / sizeof(attr_bind_t), program };

  attr_bind_t capturecard[] =
  {
    { "CardId",         IS_UINT32,  (setter_t)MythDTOCaptureCard::SetCaptureCard_CardId },
    { "CardType",       IS_STRING,  (setter_t)MythDTOCaptureCard::SetCaptureCard_CardType },
    { "HostName",       IS_STRING,  (setter_t)MythDTOCaptureCard::SetCaptureCard_HostName },
  };
  bindings_t CaptureCardBindArray = { sizeof(capturecard) / sizeof(attr_bind_t), capturecard };

  attr_bind_t videosource[] =
  {
    { "Id",             IS_UINT32,  (setter_t)MythDTOVideoSource::SetVideoSource_Id },
    { "SourceName",     IS_STRING,  (setter_t)MythDTOVideoSource::SetVideoSource_SourceName },
  };
  bindings_t VideoSourceBindArray = { sizeof(videosource) / sizeof(attr_bind_t), videosource };

  attr_bind_t recordschedule[] =
  {
    { "Id",               IS_UINT32,  (setter_t)MythDTORecordSchedule::SetSchedule_Id },
    { "ParentId",         IS_UINT32,  (setter_t)MythDTORecordSchedule::SetSchedule_ParentId },
    { "Inactive",         IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_Inactive },
    { "Title",            IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_Title },
    { "Subtitle",         IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_Subtitle },
    { "Description",      IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_Description },
    { "Season",           IS_UINT16,  (setter_t)MythDTORecordSchedule::SetSchedule_Season },
    { "Episode",          IS_UINT16,  (setter_t)MythDTORecordSchedule::SetSchedule_Episode },
    { "Category",         IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_Category },
    { "StartTime",        IS_TIME,    (setter_t)MythDTORecordSchedule::SetSchedule_StartTime },
    { "EndTime",          IS_TIME,    (setter_t)MythDTORecordSchedule::SetSchedule_EndTime },
    { "SeriesId",         IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_SeriesId },
    { "ProgramId",        IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_ProgramId },
    { "Inetref",          IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_Inetref },
    { "ChanId",           IS_UINT32,  (setter_t)MythDTORecordSchedule::SetSchedule_ChanId },
    { "CallSign",         IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_CallSign },
    { "Day",              IS_INT8,    (setter_t)MythDTORecordSchedule::SetSchedule_FindDay },
    { "Time",             IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_FindTime },
    { "Type",             IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_Type },
    { "SearchType",       IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_SearchType },
    { "RecPriority",      IS_INT8,    (setter_t)MythDTORecordSchedule::SetSchedule_RecPriority },
    { "PreferredInput",   IS_UINT32,  (setter_t)MythDTORecordSchedule::SetSchedule_PreferredInput },
    { "StartOffset",      IS_UINT8,   (setter_t)MythDTORecordSchedule::SetSchedule_StartOffset },
    { "EndOffset",        IS_UINT8,   (setter_t)MythDTORecordSchedule::SetSchedule_EndOffset },
    { "DupMethod",        IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_DupMethod },
    { "DupIn",            IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_DupIn },
    { "Filter",           IS_UINT32,  (setter_t)MythDTORecordSchedule::SetSchedule_Filter },
    { "RecProfile",       IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_RecProfile },
    { "RecGroup",         IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_RecGroup },
    { "StorageGroup",     IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_StorageGroup },
    { "PlayGroup",        IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_PlayGroup },
    { "AutoExpire",       IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoExpire },
    { "MaxEpisodes",      IS_UINT32,  (setter_t)MythDTORecordSchedule::SetSchedule_MaxEpisodes },
    { "MaxNewest",        IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_MaxNewest },
    { "AutoCommflag",     IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoCommflag },
    { "AutoTranscode",    IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoTranscode },
    { "AutoMetaLookup",   IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoMetaLookup },
    { "AutoUserJob1",     IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoUserJob1 },
    { "AutoUserJob2",     IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoUserJob2 },
    { "AutoUserJob3",     IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoUserJob3 },
    { "AutoUserJob4",     IS_BOOLEAN, (setter_t)MythDTORecordSchedule::SetSchedule_AutoUserJob4 },
    { "Transcoder",       IS_UINT32,  (setter_t)MythDTORecordSchedule::SetSchedule_Transcoder },
  };
  bindings_t recordscheduleBindArray = { sizeof(recordschedule) / sizeof(attr_bind_t), recordschedule };
}

#endif	/* MYTHDTO75_H */

