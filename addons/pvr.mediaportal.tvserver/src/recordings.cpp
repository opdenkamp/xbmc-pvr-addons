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
#include <stdio.h>

using namespace std;

#include "recordings.h"
#include "utils.h"
#include "timers.h"
#include "client.h"
#include "DateTime.h"

using namespace ADDON;

cRecording::cRecording()
{
  m_duration        = 0;
  m_Index           = -1;
  m_cardSettings    = NULL;
  m_channelID       = 0;
  m_isRecording     = false;
  m_genre_type      = 0;
  m_genre_subtype   = 0;
  m_genretable      = NULL;
  m_scheduleID      = 0;
  m_keepUntil       = 0;
  m_timesWatched    = 0;
  m_lastPlayedPosition = 0;
}


cRecording::~cRecording()
{
}

void cRecording::SetCardSettings(CCards* cardSettings)
{
  m_cardSettings = cardSettings;
}

bool cRecording::ParseLine(const std::string& data)
{
  vector<string> fields;

  Tokenize(data, fields, "|");

  if( fields.size() >= 9 )
  {
    //[0] index / mediaportal recording id
    //[1] start time
    //[2] end time
    //[3] channel name
    //[4] title
    //[5] description
    //[6] stream_url (resolved hostname if requested)
    //[7] filename (we can bypass rtsp streaming when XBMC and the TV server are on the same machine)
    //[8] keepUntilDate (DateTime)
    //[9] (optional) original stream_url when resolve hostnames is enabled
    //[10] keepUntil (int)
    //[11] episodeName (string)
    //[12] episodeNumber (string)
    //[13] episodePart (string)
    //[14] seriesNumber (string)
    //[15] scheduleID (int)
    //[16] genre (string)
    //[17] idchannel (int)
    //[18] isrecording (bool)
    //[19] timesWatched (int)
    //[20] stopTime (int)

    m_Index = atoi(fields[0].c_str());

    if ( m_startTime.SetFromDateTime(fields[1]) == false )
    {
      XBMC->Log(LOG_ERROR, "%s: Unable to convert start time '%s' into date+time", __FUNCTION__, fields[1].c_str());
      return false;
    }

    if ( m_endTime.SetFromDateTime(fields[2]) == false )
    {
      XBMC->Log(LOG_ERROR, "%s: Unable to convert end time '%s' into date+time", __FUNCTION__, fields[2].c_str());
      return false;
    }

    m_duration = m_endTime - m_startTime;

    m_channelName = fields[3];
    m_title = fields[4];
    m_description = fields[5];
    m_stream = fields[6];
    m_filePath = fields[7];

    // TODO: fill lifetime with data from MP TV Server
    // From the VDR documentation (VDR is used by Alwinus as basis for the XBMC
    // PVR framework:
    // "The lifetime (int) value corresponds to the the number of days (0..99)
    // a recording made through this timer is guaranteed to remain on disk
    // before it is automatically removed to free up space for a new recording.
    // Note that setting this parameter to very high values for all recordings
    // may soon fill up the entire disk and cause new recordings to fail due to
    // low disk space. The special value 99 means that this recording will live
    // forever, and a value of 0 means that this recording can be deleted any
    // time if a recording with a higher priority needs disk space."
    if ( m_keepUntilDate.SetFromDateTime(fields[8]) == false )
    {
      // invalid date (or outside time_t boundaries)
      m_keepUntilDate = cUndefinedDate;
    }

    if( m_filePath.length() > 0 )
    {
      SplitFilePath();
    }
    else
    {
      m_basePath = "";
      m_fileName = "";
      m_directory = "";
    }


    if (fields.size() >= 10) // Since TVServerXBMC 1.0.8.0
    {
      m_originalurl = fields[9];
    }
    else
    {
      m_originalurl = fields[6];
    }

    if (fields.size() >= 16) // Since TVServerXBMC 1.1.x.105
    {
      m_keepUntil = atoi( fields[10].c_str() );
      m_episodeName = fields[11];
      m_episodeNumber = fields[12];
      m_episodePart = fields[13];
      m_seriesNumber = fields[14];
      m_scheduleID = atoi( fields[15].c_str() );
    }

    if (fields.size() >= 19) // Since TVServerXBMC 1.2.x.111
    {
      m_genre = fields[16];
      m_channelID = atoi( fields[17].c_str() );
      m_isRecording = stringtobool( fields[18] );

      if (m_genretable) m_genretable->GenreToTypes(m_genre, m_genre_type, m_genre_subtype);

      if (fields.size() >= 20) // Since TVServerXBMC 1.2.x.117
      {
        m_timesWatched = atoi( fields[19].c_str() );
        if (fields.size() >= 21) // Since TVServerXBMC 1.2.x.121
        {
          m_lastPlayedPosition = atoi( fields[20].c_str() );
        }
      }
    }

    return true;
  }
  else
  {
    return false;
  }
}


//void cRecording::SetDirectory( string& directory )
//{
//  CStdString tmp;
//  m_basePath = directory;
//  tmp = m_basePath + m_directory + "\\" + m_fileName;
//
//  if( m_basePath.find("smb://") != string::npos )
//  {
//    // Convert to XBMC network share...
//    tmp.Replace("\\","/");
//  }
//
//  m_filePath = tmp;
//}

int cRecording::Lifetime(void) const
{
  // margro: the meaning of the XBMC-PVR Lifetime field is undocumented.
  // Assuming that VDR is the source for this field:
  //  The guaranteed lifetime (in days) of a recording created by this
  //  timer.  0 means that this recording may be automatically deleted
  //  at  any  time  by a new recording with higher priority. 99 means
  //  that this recording will never  be  automatically  deleted.  Any
  //  number  in the range 1...98 means that this recording may not be
  //  automatically deleted in favour of a new  recording,  until  the
  //  given  number  of days since the start time of the recording has
  //  passed by
  TvDatabase::KeepMethodType m_keepmethod = (TvDatabase::KeepMethodType) m_keepUntil;

  switch (m_keepmethod)
  {
    case TvDatabase::UntilSpaceNeeded: //until space needed
    case TvDatabase::UntilWatched: //until watched
      return 0;
      break;
    case TvDatabase::TillDate: //until keepdate
      {
        int diffseconds = m_keepUntilDate - m_startTime;
        int daysremaining = (int)(diffseconds / cSecsInDay);
        // Calculate value in the range 1...98, based on m_keepdate
        if ((daysremaining < MAXLIFETIME) && (daysremaining >= 0))
        {
          return daysremaining;
        }
        else
        {
          // > 98 days => return forever
          return MAXLIFETIME;
        }
      }
      break;
    case TvDatabase::Always: //forever
      return MAXLIFETIME;
    default:
      return MAXLIFETIME;
  }
}

void cRecording::SplitFilePath(void)
{
  size_t found = string::npos;

  // Try to find the base path used for this recording by searching for the
  // card recording folder name in the the recording file name.
  if ((m_cardSettings) && (m_cardSettings->size() > 0))
  {
    for (CCards::iterator it = m_cardSettings->begin(); it < m_cardSettings->end(); ++it)
    {
      // Determine whether the first part of the recording file name is shared with this card
      // Minimal name length of the RecordingFolder should be 3 (drive letter + :\)
      if (it->RecordingFolder.length() >= 3)
      {
        found = m_filePath.find(it->RecordingFolder);
        if (found != string::npos)
        {
          m_basePath = it->RecordingFolder;
          if (m_basePath.at(m_basePath.length() - 1) != '\\')
            m_basePath += "\\";

          // Remove the base path
          m_fileName = m_filePath.substr(it->RecordingFolder.length()+1);

          // Extract subdirectories below the base path
          size_t found2 = m_fileName.find_last_of("/\\");
          if (found2 != string::npos)
          {
            m_directory = m_fileName.substr(0, found2);
            m_fileName = m_fileName.substr(found2+1);
          }
          else
          {
            m_directory = "";
          }

          break;
        }
      }
    }
  }

  if (found == string::npos)
  {
    m_fileName = m_filePath;
    m_directory = "";
    m_basePath = "";
  }
}

void cRecording::SetGenreTable(CGenreTable* genretable)
{
  m_genretable = genretable;
}

time_t cRecording::StartTime(void) const
{
  time_t time = m_startTime.GetAsTime();
  return time;
}

int cRecording::Duration(void) const
{
  return m_duration;
}
