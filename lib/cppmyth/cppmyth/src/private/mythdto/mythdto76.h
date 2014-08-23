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

#ifndef MYTHDTO76_H
#define	MYTHDTO76_H

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

namespace MythDTO76
{
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
    { "FindDay",          IS_INT8,    (setter_t)MythDTORecordSchedule::SetSchedule_FindDay },
    { "FindTime",         IS_STRING,  (setter_t)MythDTORecordSchedule::SetSchedule_FindTime },
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

#endif	/* MYTHDTO76_H */

