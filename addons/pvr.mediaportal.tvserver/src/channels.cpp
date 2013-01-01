/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *
 */

#include <vector>
#include "channels.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

cChannel::cChannel()
{
  uid = 0;
  external_id = 0;
  iswebstream = false;
  encrypted = false;
  visibleinguide = true;
}

cChannel::~cChannel()
{
}

bool cChannel::Parse(const std::string& data)
{
  vector<string> fields;

  Tokenize(data, fields, "|");

  if (fields.size() >= 4)
  {
    // Expected format:
    // ListTVChannels, ListRadioChannels
    // 0 = channel uid
    // 1 = channel external id/number
    // 2 = channel name
    // 3 = isencrypted ("0"/"1")
    // ListRadioChannels only: (TVServerXBMC >= v1.1.0.100)
    // 4 = iswebstream
    // 5 = webstream url
    // 6 = visibleinguide (TVServerXBMC >= v1.2.3.120)

    uid = atoi(fields[0].c_str());
    external_id = atoi(fields[1].c_str());
    name = fields[2];
    encrypted = (strncmp(fields[3].c_str(), "1", 1) == 0);
    
    if (fields.size() >= 6)
    {
      iswebstream = (strncmp(fields[4].c_str(), "1", 1) == 0);
      url = fields[5].c_str();

      if (fields.size() >= 7)
      {
        visibleinguide = (strncmp(fields[6].c_str(), "1", 1) == 0);
      }
    }

    return true;
  }
  else
  {
    return false;
  }
}
