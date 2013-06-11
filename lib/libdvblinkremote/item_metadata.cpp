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

#include "response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

ItemMetadata::ItemMetadata() 
{
  m_title = "";
  m_startTime = 0;
  m_duration = 0;
  ShortDescription = "";
  SubTitle = "";
  Language = "";
  Actors = "";
  Directors = "";
  Writers = "";
  Producers = "";
  Guests = "";
  Keywords = "";
  Image = "";
  Year = 0;
  EpisodeNumber = 0;
  SeasonNumber = 0;
  Rating = 0;
  MaximumRating = 0;
  IsHdtv = false;
  IsPremiere = false;
  IsRepeat = false;
  IsSeries = false;
  IsRecord = false;
  IsRepeatRecord = false;
  IsCatAction = false;
  IsCatComedy = false;
  IsCatDocumentary = false;
  IsCatDrama = false;
  IsCatEducational = false;
  IsCatHorror = false;
  IsCatKids = false;
  IsCatMovie = false;
  IsCatMusic = false;
  IsCatNews = false;
  IsCatReality = false;
  IsCatRomance = false;
  IsCatScifi = false;
  IsCatSerial = false;
  IsCatSoap = false;
  IsCatSpecial = false;
  IsCatSports = false;
  IsCatThriller = false;
  IsCatAdult = false;
}

ItemMetadata::ItemMetadata(const std::string& title, const long startTime, const long duration)
  : m_title(title), 
    m_startTime(startTime), 
    m_duration(duration)
{
  ShortDescription = "";
  SubTitle = "";
  Language = "";
  Actors = "";
  Directors = "";
  Writers = "";
  Producers = "";
  Guests = "";
  Keywords = "";
  Image = "";
  Year = 0;
  EpisodeNumber = 0;
  SeasonNumber = 0;
  Rating = 0;
  MaximumRating = 0;
  IsHdtv = false;
  IsPremiere = false;
  IsRepeat = false;
  IsSeries = false;
  IsRecord = false;
  IsRepeatRecord = false;
  IsCatAction = false;
  IsCatComedy = false;
  IsCatDocumentary = false;
  IsCatDrama = false;
  IsCatEducational = false;
  IsCatHorror = false;
  IsCatKids = false;
  IsCatMovie = false;
  IsCatMusic = false;
  IsCatNews = false;
  IsCatReality = false;
  IsCatRomance = false;
  IsCatScifi = false;
  IsCatSerial = false;
  IsCatSoap = false;
  IsCatSpecial = false;
  IsCatSports = false;
  IsCatThriller = false;
  IsCatAdult = false;
}

ItemMetadata::ItemMetadata(ItemMetadata& itemMetadata) 
  : m_title(itemMetadata.GetTitle()),
    m_startTime(itemMetadata.GetStartTime()),
    m_duration(itemMetadata.GetDuration())
{
  ShortDescription = itemMetadata.ShortDescription;
  SubTitle = itemMetadata.SubTitle;
  Language = itemMetadata.Language;
  Actors = itemMetadata.Actors;
  Directors = itemMetadata.Directors;
  Writers = itemMetadata.Writers;
  Producers = itemMetadata.Producers;
  Guests = itemMetadata.Guests;
  Keywords = itemMetadata.Keywords;
  Image = itemMetadata.Image;
  Year = itemMetadata.Year;
  EpisodeNumber = itemMetadata.EpisodeNumber;
  SeasonNumber = itemMetadata.SeasonNumber;
  Rating = itemMetadata.Rating;
  MaximumRating = itemMetadata.MaximumRating;
  IsHdtv = itemMetadata.IsHdtv;
  IsPremiere = itemMetadata.IsPremiere;
  IsRepeat = itemMetadata.IsRepeat;
  IsSeries = itemMetadata.IsSeries;
  IsRecord = itemMetadata.IsRecord;
  IsRepeatRecord = itemMetadata.IsRepeatRecord;
  IsCatAction = itemMetadata.IsCatAction;
  IsCatComedy = itemMetadata.IsCatComedy;
  IsCatDocumentary = itemMetadata.IsCatDocumentary;
  IsCatDrama = itemMetadata.IsCatDrama;
  IsCatEducational = itemMetadata.IsCatEducational;
  IsCatHorror = itemMetadata.IsCatHorror;
  IsCatKids = itemMetadata.IsCatKids;
  IsCatMovie = itemMetadata.IsCatMovie;
  IsCatMusic = itemMetadata.IsCatMusic;
  IsCatNews = itemMetadata.IsCatNews;
  IsCatReality = itemMetadata.IsCatReality;
  IsCatRomance = itemMetadata.IsCatRomance;
  IsCatScifi = itemMetadata.IsCatScifi;
  IsCatSerial = itemMetadata.IsCatSerial;
  IsCatSoap = itemMetadata.IsCatSoap;
  IsCatSpecial = itemMetadata.IsCatSpecial;
  IsCatSports = itemMetadata.IsCatSports;
  IsCatThriller = itemMetadata.IsCatThriller;
  IsCatAdult = itemMetadata.IsCatAdult;
}

ItemMetadata::~ItemMetadata() 
{

}

std::string& ItemMetadata::GetTitle() 
{ 
  return m_title; 
}

void ItemMetadata::SetTitle(const std::string& title) 
{ 
  m_title = title; 
}

long ItemMetadata::GetStartTime() 
{ 
  return m_startTime; 
}

void ItemMetadata::SetStartTime(const long startTime) 
{ 
  m_startTime = startTime; 
}

long ItemMetadata::GetDuration() 
{ 
  return m_duration; 
}

void ItemMetadata::SetDuration(const long duration) 
{ 
  m_duration = duration; 
}

void ItemMetadataSerializer::Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, dvblinkremote::ItemMetadata& itemMetadata)
{
  itemMetadata.SetTitle(Util::GetXmlFirstChildElementText(&element, "name"));
  itemMetadata.SetStartTime(Util::GetXmlFirstChildElementTextAsLong(&element, "start_time"));
  itemMetadata.SetDuration(Util::GetXmlFirstChildElementTextAsLong(&element, "duration"));

  itemMetadata.ShortDescription = Util::GetXmlFirstChildElementText(&element, "short_desc");
  itemMetadata.SubTitle = Util::GetXmlFirstChildElementText(&element, "subname");
  itemMetadata.Language = Util::GetXmlFirstChildElementText(&element, "language");
  itemMetadata.Actors = Util::GetXmlFirstChildElementText(&element, "actors");
  itemMetadata.Directors = Util::GetXmlFirstChildElementText(&element, "directors");
  itemMetadata.Writers = Util::GetXmlFirstChildElementText(&element, "writers");
  itemMetadata.Producers = Util::GetXmlFirstChildElementText(&element, "producers");
  itemMetadata.Guests = Util::GetXmlFirstChildElementText(&element, "guests");
  itemMetadata.Keywords = Util::GetXmlFirstChildElementText(&element, "categories");
  itemMetadata.Image = Util::GetXmlFirstChildElementText(&element, "image");
         
  itemMetadata.Year = Util::GetXmlFirstChildElementTextAsLong(&element, "year");
  itemMetadata.EpisodeNumber = Util::GetXmlFirstChildElementTextAsLong(&element, "episode_num");
  itemMetadata.SeasonNumber = Util::GetXmlFirstChildElementTextAsLong(&element, "season_num");
  itemMetadata.Rating = Util::GetXmlFirstChildElementTextAsLong(&element, "stars_num");
  itemMetadata.MaximumRating = Util::GetXmlFirstChildElementTextAsLong(&element, "starsmax_num");
         
  itemMetadata.IsHdtv = objectSerializer.HasChildElement(*&element, "hdtv");
  itemMetadata.IsPremiere = objectSerializer.HasChildElement(*&element, "premiere");
  itemMetadata.IsRepeat = objectSerializer.HasChildElement(*&element, "repeat");
  itemMetadata.IsSeries = objectSerializer.HasChildElement(*&element, "is_series");
  itemMetadata.IsRecord = objectSerializer.HasChildElement(*&element, "is_record");
  itemMetadata.IsRepeatRecord = objectSerializer.HasChildElement(*&element, "is_repeat_record");
         
  itemMetadata.IsCatAction = objectSerializer.HasChildElement(*&element, "cat_action");
  itemMetadata.IsCatComedy = objectSerializer.HasChildElement(*&element, "cat_comedy");
  itemMetadata.IsCatDocumentary = objectSerializer.HasChildElement(*&element, "cat_documentary");
  itemMetadata.IsCatDrama = objectSerializer.HasChildElement(*&element, "cat_drama");
  itemMetadata.IsCatEducational = objectSerializer.HasChildElement(*&element, "cat_educational");
  itemMetadata.IsCatHorror = objectSerializer.HasChildElement(*&element, "cat_horror");
  itemMetadata.IsCatKids = objectSerializer.HasChildElement(*&element, "cat_kids");
  itemMetadata.IsCatMovie = objectSerializer.HasChildElement(*&element, "cat_movie");
  itemMetadata.IsCatMusic = objectSerializer.HasChildElement(*&element, "cat_music");
  itemMetadata.IsCatNews = objectSerializer.HasChildElement(*&element, "cat_news");
  itemMetadata.IsCatReality = objectSerializer.HasChildElement(*&element, "cat_reality");
  itemMetadata.IsCatRomance = objectSerializer.HasChildElement(*&element, "cat_romance");
  itemMetadata.IsCatScifi = objectSerializer.HasChildElement(*&element, "cat_scifi");
  itemMetadata.IsCatSerial = objectSerializer.HasChildElement(*&element, "cat_serial");
  itemMetadata.IsCatSoap = objectSerializer.HasChildElement(*&element, "cat_soap");
  itemMetadata.IsCatSpecial = objectSerializer.HasChildElement(*&element, "cat_special");
  itemMetadata.IsCatSports = objectSerializer.HasChildElement(*&element, "cat_sports");
  itemMetadata.IsCatThriller = objectSerializer.HasChildElement(*&element, "cat_thriller");
  itemMetadata.IsCatAdult = objectSerializer.HasChildElement(*&element, "cat_adult");
}
