/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include "Cards.h"
#include "uri.h"
#include "utils.h"

bool CCards::ParseLines(vector<string>& lines)
{
  if (lines.empty())
    return false;

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); ++it)
  {
    string& data(*it);

    if (!data.empty())
    {
      vector<string> fields;
      Card card;

      uri::decode(data);
      Tokenize(data, fields, "|");
      // field 0 = idCard
      // field 1 = devicePath
      // field 2 = name
      // field 3 = priority
      // field 4 = grabEPG
      // field 5 = lastEpgGrab (2000-01-01 00:00:00 = infinite)
      // field 6 = recordingFolder
      // field 7 = idServer
      // field 8 = enabled
      // field 9 = camType
      // field 10 = timeshiftingFolder
      // field 11 = recordingFormat
      // field 12 = decryptLimit
      // field 13 = preload
      // field 14 = CAM
      // field 15 = NetProvider
      // field 16 = stopgraph
      // field 17 = UNC path recording folder (when shared)
      // field 18 = UNC path timeshift folder (when shared)

      card.IdCard = atoi(fields[0].c_str());
      card.DevicePath = fields[1];
      card.Name = fields[2];
      card.Priority = atoi(fields[3].c_str());
      card.GrabEPG = stringtobool(fields[4]);
      card.LastEpgGrab = DateTimeToTimeT(fields[5]);
      card.RecordingFolder = fields[6];
      card.IdServer = atoi(fields[7].c_str());
      card.Enabled = stringtobool(fields[8]);
      card.CamType = atoi(fields[9].c_str());
      card.TimeshiftFolder = fields[10];
      card.RecordingFormat = atoi(fields[11].c_str());
      card.DecryptLimit = atoi(fields[12].c_str());
      card.Preload = stringtobool(fields[13]);
      card.CAM = stringtobool(fields[14]);
      card.NetProvider = atoi(fields[15].c_str());
      card.StopGraph = stringtobool(fields[16]);

      if (fields.size() > 17) // since TVServerXBMC build 115
      {
        card.RecordingFolderUNC = fields[17];
        card.TimeshiftFolderUNC = fields[18];
      }
      else
      {
        card.RecordingFolderUNC = "";
        card.TimeshiftFolderUNC = "";
      }

      push_back(card);
    }
  }

  return true;
}

bool CCards::GetCard(int id, Card& card)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).IdCard == id)
    {
      card = at(i);
      return true;
    }
  }

  card.IdCard = -1;
  return false;
}