/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "categories.h"

Categories::Categories()
  : m_categoriesById(DefaultEITCategories())
{
  // Copy over
  CategoryByIdMap::const_iterator it;
  for (it =  m_categoriesById.begin(); it != m_categoriesById.end(); ++it)
  {
    m_categoriesByName.insert(CategoryByNameMap::value_type(it->second, it->first));
  }
}

std::string Categories::Category(int category) const
{
  CategoryByIdMap::const_iterator it = m_categoriesById.find(category);
  if (it != m_categoriesById.end())
    return it->second;
  return "";
}

int Categories::Category(const std::string &category) const
{
  CategoryByNameMap::const_iterator it = m_categoriesByName.find(category);
  if (it != m_categoriesByName.end())
    return it->second;
  return 0;
}

CategoryByIdMap Categories::DefaultEITCategories() const
{
  CategoryByIdMap categoryMap;
  categoryMap.insert(std::pair<int, std::string>(0x10, "Movie"));
  categoryMap.insert(std::pair<int, std::string>(0x11, "Movie - Detective/Thriller"));
  categoryMap.insert(std::pair<int, std::string>(0x12, "Movie - Adventure/Western/War"));
  categoryMap.insert(std::pair<int, std::string>(0x13, "Movie - Science Fiction/Fantasy/Horror"));
  categoryMap.insert(std::pair<int, std::string>(0x14, "Movie - Comedy"));
  categoryMap.insert(std::pair<int, std::string>(0x15, "Movie - Soap/melodrama/folkloric"));
  categoryMap.insert(std::pair<int, std::string>(0x16, "Movie - Romance"));
  categoryMap.insert(std::pair<int, std::string>(0x17, "Movie - Serious/Classical/Religious/Historical Movie/Drama"));
  categoryMap.insert(std::pair<int, std::string>(0x18, "Movie - Adult   "));
  categoryMap.insert(std::pair<int, std::string>(0x1F, "Drama")); // MythTV use 0xF0 but xbmc doesn't implement this.
  categoryMap.insert(std::pair<int, std::string>(0x20, "News"));
  categoryMap.insert(std::pair<int, std::string>(0x21, "News/weather report"));
  categoryMap.insert(std::pair<int, std::string>(0x22, "News magazine"));
  categoryMap.insert(std::pair<int, std::string>(0x23, "Documentary"));
  categoryMap.insert(std::pair<int, std::string>(0x24, "Intelligent Programmes"));
  categoryMap.insert(std::pair<int, std::string>(0x30, "Entertainment"));
  categoryMap.insert(std::pair<int, std::string>(0x31, "Game Show"));
  categoryMap.insert(std::pair<int, std::string>(0x32, "Variety Show"));
  categoryMap.insert(std::pair<int, std::string>(0x33, "Talk Show"));
  categoryMap.insert(std::pair<int, std::string>(0x40, "Sports"));
  categoryMap.insert(std::pair<int, std::string>(0x41, "Special Events (World Cup, World Series, etc)"));
  categoryMap.insert(std::pair<int, std::string>(0x42, "Sports Magazines"));
  categoryMap.insert(std::pair<int, std::string>(0x43, "Football (Soccer)"));
  categoryMap.insert(std::pair<int, std::string>(0x44, "Tennis/Squash"));
  categoryMap.insert(std::pair<int, std::string>(0x45, "Misc. Team Sports"));
  categoryMap.insert(std::pair<int, std::string>(0x46, "Athletics"));
  categoryMap.insert(std::pair<int, std::string>(0x47, "Motor Sport"));
  categoryMap.insert(std::pair<int, std::string>(0x48, "Water Sport"));
  categoryMap.insert(std::pair<int, std::string>(0x49, "Winter Sports"));
  categoryMap.insert(std::pair<int, std::string>(0x4A, "Equestrian"));
  categoryMap.insert(std::pair<int, std::string>(0x4B, "Martial Sports"));
  categoryMap.insert(std::pair<int, std::string>(0x50, "Kids"));
  categoryMap.insert(std::pair<int, std::string>(0x51, "Pre-School Children's Programmes"));
  categoryMap.insert(std::pair<int, std::string>(0x52, "Entertainment Programmes for 6 to 14"));
  categoryMap.insert(std::pair<int, std::string>(0x53, "Entertainment Programmes for 10 to 16"));
  categoryMap.insert(std::pair<int, std::string>(0x54, "Informational/Educational"));
  categoryMap.insert(std::pair<int, std::string>(0x55, "Cartoons/Puppets"));
  categoryMap.insert(std::pair<int, std::string>(0x60, "Music/Ballet/Dance"));
  categoryMap.insert(std::pair<int, std::string>(0x61, "Rock/Pop"));
  categoryMap.insert(std::pair<int, std::string>(0x62, "Classical Music"));
  categoryMap.insert(std::pair<int, std::string>(0x63, "Folk Music"));
  categoryMap.insert(std::pair<int, std::string>(0x64, "Jazz"));
  categoryMap.insert(std::pair<int, std::string>(0x65, "Musical/Opera"));
  categoryMap.insert(std::pair<int, std::string>(0x66, "Ballet"));
  categoryMap.insert(std::pair<int, std::string>(0x70, "Arts/Culture"));
  categoryMap.insert(std::pair<int, std::string>(0x71, "Performing Arts"));
  categoryMap.insert(std::pair<int, std::string>(0x72, "Fine Arts"));
  categoryMap.insert(std::pair<int, std::string>(0x73, "Religion"));
  categoryMap.insert(std::pair<int, std::string>(0x74, "Popular Culture/Traditional Arts"));
  categoryMap.insert(std::pair<int, std::string>(0x75, "Literature"));
  categoryMap.insert(std::pair<int, std::string>(0x76, "Film/Cinema"));
  categoryMap.insert(std::pair<int, std::string>(0x77, "Experimental Film/Video"));
  categoryMap.insert(std::pair<int, std::string>(0x78, "Broadcasting/Press"));
  categoryMap.insert(std::pair<int, std::string>(0x79, "New Media"));
  categoryMap.insert(std::pair<int, std::string>(0x7A, "Arts/Culture Magazines"));
  categoryMap.insert(std::pair<int, std::string>(0x7B, "Fashion"));
  categoryMap.insert(std::pair<int, std::string>(0x80, "Social/Policical/Economics"));
  categoryMap.insert(std::pair<int, std::string>(0x81, "Magazines/Reports/Documentary"));
  categoryMap.insert(std::pair<int, std::string>(0x82, "Economics/Social Advisory"));
  categoryMap.insert(std::pair<int, std::string>(0x83, "Remarkable People"));
  categoryMap.insert(std::pair<int, std::string>(0x90, "Education/Science/Factual"));
  categoryMap.insert(std::pair<int, std::string>(0x91, "Nature/animals/Environment"));
  categoryMap.insert(std::pair<int, std::string>(0x92, "Technology/Natural Sciences"));
  categoryMap.insert(std::pair<int, std::string>(0x93, "Medicine/Physiology/Psychology"));
  categoryMap.insert(std::pair<int, std::string>(0x94, "Foreign Countries/Expeditions"));
  categoryMap.insert(std::pair<int, std::string>(0x95, "Social/Spiritual Sciences"));
  categoryMap.insert(std::pair<int, std::string>(0x96, "Further Education"));
  categoryMap.insert(std::pair<int, std::string>(0x97, "Languages"));
  categoryMap.insert(std::pair<int, std::string>(0xA0, "Leisure/Hobbies"));
  categoryMap.insert(std::pair<int, std::string>(0xA1, "Tourism/Travel"));
  categoryMap.insert(std::pair<int, std::string>(0xA2, "Handicraft"));
  categoryMap.insert(std::pair<int, std::string>(0xA3, "Motoring"));
  categoryMap.insert(std::pair<int, std::string>(0xA4, "Fitness & Health"));
  categoryMap.insert(std::pair<int, std::string>(0xA5, "Cooking"));
  categoryMap.insert(std::pair<int, std::string>(0xA6, "Advertizement/Shopping"));
  categoryMap.insert(std::pair<int, std::string>(0xA7, "Gardening"));
  categoryMap.insert(std::pair<int, std::string>(0xB0, "Original Language"));
  categoryMap.insert(std::pair<int, std::string>(0xB1, "Black & White"));
  categoryMap.insert(std::pair<int, std::string>(0xB2, "\"Unpublished\" Programmes"));
  categoryMap.insert(std::pair<int, std::string>(0xB3, "Live Broadcast"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Community"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Fundraiser"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Bus./financial"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Variety"));
  categoryMap.insert(std::pair<int, std::string>(0xC6, "Romance-comedy"));
  categoryMap.insert(std::pair<int, std::string>(0x40, "Sports event"));
  categoryMap.insert(std::pair<int, std::string>(0x40, "Sports talk"));
  categoryMap.insert(std::pair<int, std::string>(0x92, "Computers"));
  categoryMap.insert(std::pair<int, std::string>(0xA2, "How-to"));
  categoryMap.insert(std::pair<int, std::string>(0x73, "Religious"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Parenting"));
  categoryMap.insert(std::pair<int, std::string>(0x70, "Art"));
  categoryMap.insert(std::pair<int, std::string>(0x64, "Musical comedy"));
  categoryMap.insert(std::pair<int, std::string>(0x91, "Environment"));
  categoryMap.insert(std::pair<int, std::string>(0x80, "Politics"));
  categoryMap.insert(std::pair<int, std::string>(0x55, "Animated"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Gaming"));
  categoryMap.insert(std::pair<int, std::string>(0x24, "Interview"));
  categoryMap.insert(std::pair<int, std::string>(0xC7, "Historical drama"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Biography"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Home improvement"));
  categoryMap.insert(std::pair<int, std::string>(0xA0, "Hunting"));
  categoryMap.insert(std::pair<int, std::string>(0xA0, "Outdoors"));
  categoryMap.insert(std::pair<int, std::string>(0x47, "Auto"));
  categoryMap.insert(std::pair<int, std::string>(0x47, "Auto racing"));
  categoryMap.insert(std::pair<int, std::string>(0xC4, "Horror"));
  categoryMap.insert(std::pair<int, std::string>(0x93, "Medical"));
  categoryMap.insert(std::pair<int, std::string>(0xC6, "Romance"));
  categoryMap.insert(std::pair<int, std::string>(0x97, "Spanish"));
  categoryMap.insert(std::pair<int, std::string>(0xC8, "Adults only"));
  categoryMap.insert(std::pair<int, std::string>(0x64, "Musical"));
  categoryMap.insert(std::pair<int, std::string>(0xA0, "Self improvement"));
  categoryMap.insert(std::pair<int, std::string>(0x40, "Pro wrestling"));
  categoryMap.insert(std::pair<int, std::string>(0x40, "Wrestling"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Fishing"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Agriculture"));
  categoryMap.insert(std::pair<int, std::string>(0x70, "Arts/crafts"));
  categoryMap.insert(std::pair<int, std::string>(0x92, "Technology"));
  categoryMap.insert(std::pair<int, std::string>(0xC0, "Docudrama"));
  categoryMap.insert(std::pair<int, std::string>(0xC3, "Science fiction"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Paranormal"));
  categoryMap.insert(std::pair<int, std::string>(0xC4, "Comedy"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Science"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Travel"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Adventure"));
  categoryMap.insert(std::pair<int, std::string>(0xC1, "Suspense"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "History"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Collectibles"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Crime"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "French"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "House/garden"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Action"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Fantasy"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Mystery"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Health"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Comedy-drama"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Special"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Holiday"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Weather"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Western"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Children"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Nature"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Animals"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Public affairs"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Educational"));
  categoryMap.insert(std::pair<int, std::string>(0xA6, "Shopping"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Consumer"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Soap"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Newsmagazine"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Exercise"));
  categoryMap.insert(std::pair<int, std::string>(0x60, "Music"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Game show"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Sitcom"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Talk"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Crime drama"));
  categoryMap.insert(std::pair<int, std::string>(0x40, "Sports non-event"));
  categoryMap.insert(std::pair<int, std::string>(0x00, "Reality"));

  return categoryMap;
}
