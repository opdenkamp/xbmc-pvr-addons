/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *************************************************************************
 *  Parts of this file originate from Team MediaPortal's
 *  TsReader DirectShow filter
 *  MediaPortal is a GPL'ed HTPC-Application
 *  Copyright (C) 2005-2012 Team MediaPortal
 *  http://www.team-mediaportal.com
 *
 * Changes compared to Team MediaPortal's version:
 * - Code cleanup for PVR addon usage
 * - Code refactoring for cross platform usage
 *************************************************************************/

#include "TSReader.h"
#include "client.h" //for XBMC->Log
#include "MultiFileReader.h"
#include "utils.h"
#include "TSDebug.h"
#include "platform/util/timeutils.h"
#ifdef LIVE555
#include "MemoryReader.h"
#include "MepoRTSPClient.h"
#include "MemoryBuffer.h"
#endif
#include "FileUtils.h"

using namespace ADDON;

CTsReader::CTsReader(): m_demultiplexer( *this )
{
  m_fileReader      = NULL;
  m_fileDuration    = NULL;
  m_bLiveTv         = false;
  m_bTimeShifting   = false;
  m_bIsRTSP         = false;
  m_cardSettings    = NULL;
  m_cardId          = -1;
  m_State           = State_Stopped;
  m_lastPause       = 0;
  m_WaitForSeekToEof = 0;
  m_bRecording      = false;

#ifdef LIVE555
  m_rtspClient      = NULL;
  m_buffer          = NULL;
#endif
}

CTsReader::~CTsReader(void)
{
  SAFE_DELETE(m_fileReader);
#ifdef LIVE555
  SAFE_DELETE(m_buffer);
  SAFE_DELETE(m_rtspClient);
#endif
}

std::string CTsReader::TranslatePath(const char*  pszFileName)
{
  CStdString sFileName = pszFileName;
#if defined (TARGET_WINDOWS)
  // Can we access the given file already?
  if ( OS::CFile::Exists(pszFileName) )
  {
    XBMC->Log(LOG_DEBUG, "Found the timeshift buffer at: %s\n", pszFileName);
    return ToXBMCPath(sFileName);
  }
  XBMC->Log(LOG_NOTICE, "Cannot access '%s' directly. Assuming multiseat mode. Need to translate to UNC filename.", pszFileName);
#else
  XBMC->Log(LOG_DEBUG, "Multiseat mode; need to translate '%s' to UNC filename.", pszFileName);
#endif

  bool bFound = false;

  // Card Id given? (only for Live TV / Radio). Check for an UNC path (e.g. \\tvserver\timeshift)
  if (m_cardId >= 0)
  {
    Card tscard;

    if ((m_cardSettings) && (m_cardSettings->GetCard(m_cardId, tscard)))
    {
      if (!tscard.TimeshiftFolderUNC.empty())
      {
        sFileName.Replace(tscard.TimeshiftFolder.c_str(), tscard.TimeshiftFolderUNC.c_str());
        bFound = true;
      }
      else
      {
        XBMC->Log(LOG_ERROR, "No timeshift share known for card %i '%s'. Check your TVServerXBMC settings!", tscard.IdCard, tscard.Name.c_str());
      }
    }
  }
  else
  {
    // No Card Id given. This is a recording. Check for an UNC path (e.g. \\tvserver\recordings)
    size_t found = string::npos;

    if ((m_cardSettings) && (m_cardSettings->size() > 0))
    {
      for (CCards::iterator it = m_cardSettings->begin(); it < m_cardSettings->end(); ++it)
      {
        // Determine whether the first part of the recording filename is shared with this card
        found = sFileName.find(it->RecordingFolder);
        if (found != string::npos)
        {
          if (!it->RecordingFolderUNC.empty())
          {
            // Remove the original base path and replace it with the given path
            sFileName.Replace(it->RecordingFolder.c_str(), it->RecordingFolderUNC.c_str());
            bFound = true;
            break;
          }
        }
      }
    }
  }

  sFileName = ToXBMCPath(sFileName);

  if (bFound)
  {
    XBMC->Log(LOG_NOTICE, "Translate path %s -> %s", pszFileName, sFileName.c_str());
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Could not find a network share for '%s'. Check your TVServerXBMC settings!", pszFileName);
    if (!XBMC->FileExists(pszFileName, false))
    {
      XBMC->Log(LOG_ERROR, "Cannot access '%s'", pszFileName);
      XBMC->QueueNotification(QUEUE_ERROR, "Cannot access: %s", pszFileName);
      sFileName.clear();
      return sFileName;
    }
  }

#if defined (TARGET_WINDOWS)
  // Can we now access the given file?
  long errCode;
  if ( !OS::CFile::Exists(sFileName, &errCode) )
  {
    switch(errCode)
    {
      case ERROR_FILE_NOT_FOUND:
        XBMC->Log(LOG_ERROR, "File not found: %s.\n", sFileName.c_str());
        break;
      case ERROR_ACCESS_DENIED:
      {
        char strUserName[256];
        DWORD lLength = 256;

        if (GetUserName(strUserName, &lLength))
        {
          XBMC->Log(LOG_ERROR, "Access denied on %s. Check share access rights for user '%s' or connect as a different user using the Explorer.\n", sFileName.c_str(), strUserName);
        }
        else
        {
          XBMC->Log(LOG_ERROR, "Access denied on %s. Check share access rights.\n", sFileName.c_str());
        }
        XBMC->QueueNotification(QUEUE_ERROR, "Access denied: %s", sFileName.c_str());
        break;
      }
      default:
        XBMC->Log(LOG_ERROR, "Cannot find or access file: %s. Check share access rights.", sFileName.c_str());
    }

    sFileName.clear();
  }
#endif

  return sFileName;
}

long CTsReader::Open(const char* pszFileName)
{
  XBMC->Log(LOG_NOTICE, "TsReader open '%s'", pszFileName);

  m_fileName = pszFileName;

  if (m_State != State_Stopped)
    Close();

  // check file type
  int length = m_fileName.length();

  if ((length > 7) && (strnicmp(m_fileName.c_str(), "rtsp://",7) == 0))
  {
    // rtsp:// stream
    // open stream
    XBMC->Log(LOG_DEBUG, "open rtsp: %s", m_fileName.c_str());
#ifdef LIVE555
    //strcpy(m_rtspClient.m_outFileName, "e:\\temp\\rtsptest.ts");
    if (m_buffer)
      delete m_buffer;
    if (m_rtspClient)
      delete m_rtspClient;
    m_buffer = new CMemoryBuffer();
    m_rtspClient = new CRTSPClient();
    m_rtspClient->Initialize(m_buffer);

    if ( !m_rtspClient->OpenStream(m_fileName.c_str()) )
    {
      SAFE_DELETE(m_rtspClient);
      SAFE_DELETE(m_buffer);
      return E_FAIL;
    }

    m_bIsRTSP = true;
    m_bTimeShifting = true;
    m_bLiveTv = true;

    // are we playing a recording via RTSP
    if (m_fileName.find_first_of("/stream") == string::npos )
    {
      // yes, then we're not timeshifting
      m_bTimeShifting = false;
      m_bLiveTv = false;
    }

    // play
    m_rtspClient->Play(0.0,0.0);
    m_fileReader = new CMemoryReader(*m_buffer);
    m_State = State_Running;
#else
    XBMC->Log(LOG_ERROR, "Failed to open %s. PVR client is compiled without LIVE555 RTSP support.", m_fileName.c_str());
    XBMC->QueueNotification(QUEUE_ERROR, "PVR client has no RTSP support: %s", m_fileName.c_str());
    return E_FAIL;
#endif //LIVE555
  }
  else
  {
    if ((length < 9) || (strnicmp(&m_fileName.c_str()[length-9], ".tsbuffer", 9) != 0))
    {
      // local .ts file
      m_bTimeShifting = false;
      m_bLiveTv = false;
      m_bIsRTSP = false;
      m_fileReader = new FileReader();
    }
    else
    {
      // local timeshift buffer file file
      m_bTimeShifting = true;
      m_bLiveTv = true;
      m_bIsRTSP = false;
      m_fileReader = new MultiFileReader();
    }

    // Translate path (e.g. Local filepath to smb://user:pass@share)
    m_fileName = TranslatePath(m_fileName.c_str());

    if (m_fileName.empty())
      return S_FALSE;

    // open file
    long retval = m_fileReader->OpenFile(m_fileName);
    if (retval != S_OK)
    {
      XBMC->Log(LOG_ERROR, "Failed to open file '%s' as '%s'", pszFileName, m_fileName.c_str());
      return retval;
    }
    // detect audio/video pids
    m_demultiplexer.SetFileReader(m_fileReader);
    m_demultiplexer.Start();

    m_fileReader->SetFilePointer(0LL, FILE_BEGIN);
    m_State = State_Running;
  }
  return S_OK;
}

long CTsReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  if (m_fileReader)
  {
    return m_fileReader->Read(pbData, lDataLength, dwReadBytes);
  }

  *dwReadBytes = 0;
  return S_FALSE;
}

void CTsReader::Close()
{
  if (m_fileReader)
  {
    if (m_bIsRTSP)
    {
#ifdef LIVE555
      XBMC->Log(LOG_NOTICE, "TsReader: closing RTSP client");
      m_rtspClient->Stop();
      SAFE_DELETE(m_rtspClient);
      SAFE_DELETE(m_buffer);
#endif
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "TsReader: closing file");
      m_fileReader->CloseFile();
    }
    SAFE_DELETE(m_fileReader);
    m_State = State_Stopped;
  }
}

bool CTsReader::OnZap(const char* pszFileName, int64_t timeShiftBufferPos, long timeshiftBufferID)
{
  string newFileName;

  XBMC->Log(LOG_NOTICE, "TsReader: OnZap(%s)", pszFileName);

  // Check whether the new channel url/timeshift buffer is changed
  // In case of a new url/timeshift buffer file, close the old one first
  newFileName = TranslatePath(pszFileName);
  if (newFileName != m_fileName)
  {
    Close();
    return (S_OK == Open(pszFileName));
  }
  else
  {
    if (m_fileReader)
    {
      XBMC->Log(LOG_DEBUG,"%s: request new PAT", __FUNCTION__);

      int64_t pos_before, pos_after;
      MultiFileReader* fileReader = dynamic_cast<MultiFileReader*>(m_fileReader);

      pos_before = fileReader->GetFilePointer();

      if ((timeShiftBufferPos > 0) && (timeshiftBufferID != -1))
      {
        pos_after = fileReader->SetCurrentFilePointer(timeShiftBufferPos, timeshiftBufferID);
      }
      else
      {
        pos_after = m_fileReader->SetFilePointer(0LL, FILE_END);
        if ((timeShiftBufferPos > 0) && (pos_after > timeShiftBufferPos))
        {
          /* Move backward */
          pos_after = fileReader->SetFilePointer((timeShiftBufferPos-pos_after), FILE_CURRENT);
        }
      }

      m_demultiplexer.RequestNewPat();
      fileReader->OnChannelChange();

      XBMC->Log(LOG_DEBUG,"%s:: move from %I64d to %I64d tsbufpos  %I64d", __FUNCTION__, pos_before, pos_after, timeShiftBufferPos);
      usleep(100000);
      return true;
    }
    return false;
  }
}

void CTsReader::SetCardSettings(CCards* cardSettings)
{
  m_cardSettings = cardSettings;
}

void CTsReader::SetDirectory( string& directory )
{
  CStdString tmp = directory;

#ifdef TARGET_WINDOWS
  if( tmp.find("smb://") != string::npos )
  {
    // Convert XBMC smb share name back to a real windows network share...
    tmp.Replace("smb://","\\\\");
    tmp.Replace("/","\\");
  }
#else
  //TODO: do something useful...
#endif
  m_basePath = tmp;
}

void CTsReader::SetCardId(int id)
{
  m_cardId = id;
}

bool CTsReader::IsTimeShifting()
{
  return m_bTimeShifting;
}

long CTsReader::Pause()
{
  XBMC->Log(LOG_DEBUG, "TsReader: Pause - IsTimeShifting = %d - state = %d", IsTimeShifting(), m_State);

  if (m_State == State_Running)
  {
    m_lastPause = GetTickCount();
#ifdef LIVE555
    // Are we using rtsp?
    if (m_bIsRTSP)
    {
        XBMC->Log(LOG_DEBUG, "CTsReader::Pause()  ->pause rtsp"); // at position: %f", (m_seekTime.Millisecs() / 1000.0f));
        m_rtspClient->Pause();
    }
#endif //LIVE555
    m_State = State_Paused;
  }
  else if (m_State == State_Paused)
  {
#ifdef LIVE555
    // Are we using rtsp?
    if (m_bIsRTSP)
    {
        XBMC->Log(LOG_DEBUG, "CTsReader::Pause() is paused, continue rtsp"); // at position: %f", (m_seekTime.Millisecs() / 1000.0f));
        m_rtspClient->Continue();
        XBMC->Log(LOG_DEBUG, "CTsReader::Pause() rtsp running"); // at position: %f", (m_seekTime.Millisecs() / 1000.0f));
    }
    m_State = State_Running;
#endif //LIVE555
  }

  XBMC->Log(LOG_DEBUG, "TsReader: Pause - END - state = %d", m_State);
  return S_OK;
}

bool CTsReader::IsSeeking()
{
  return (m_WaitForSeekToEof > 0);
}

int64_t CTsReader::GetFileSize()
{
  return m_fileReader->GetFileSize();
}

int64_t CTsReader::GetFilePointer()
{
  return m_fileReader->GetFilePointer();
}

int64_t CTsReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  return m_fileReader->SetFilePointer(llDistanceToMove, dwMoveMethod);
}
