/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 
 *      Copyright (C) 2012 Palle Ehmsen(Barcode Madness)
 *      http://www.barcodemadness.com
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "TimeShiftBuffer.h"
using namespace ADDON;

TimeShiftBuffer::TimeShiftBuffer(CHelper_libXBMC_addon  * XBMC, std::string streampath)
{
  last_pos_req_time_ = -1;
  last_pos_ = 0;

  this->XBMC = XBMC;
  streampath_ = streampath;

  m_streamHandle = XBMC->OpenFile(streampath_.c_str(), 0);
}

TimeShiftBuffer::~TimeShiftBuffer(void)
{
  XBMC->CloseFile(m_streamHandle);
}


long long TimeShiftBuffer::Seek(long long iPosition, int iWhence)
{
  if (iPosition == 0 && iWhence == SEEK_CUR)
  {
    return Position();
  }

  long long ret_val = 0;

  char param_buf[1024];
  sprintf(param_buf, "&seek=%lld&whence=%d", iPosition, iWhence);

  std::string req_url = streampath_;
  req_url += param_buf;

  //close streaming handle before executing seek
  XBMC->CloseFile(m_streamHandle);
  //execute seek request
  std::vector<std::string> response_values;
  if (ExecuteServerRequest(req_url, response_values))
  {
    ret_val = atoll(response_values[0].c_str());
  }

  //restart streaming
  m_streamHandle = XBMC->OpenFile(streampath_.c_str(), 0);

  return ret_val;
}

long long TimeShiftBuffer::Position()
{
  long long ret_val = 0;

  time_t duration;
  long long length;
  GetBufferParams(length, duration, ret_val);

  return ret_val;
}

bool TimeShiftBuffer::ExecuteServerRequest(const std::string& url, std::vector<std::string>& response_values)
{
  bool ret_val = false;
  response_values.clear();

  void* req_handle = XBMC->OpenFile(url.c_str(), 0);
  if (req_handle != NULL)
  {
    char resp_buf[1024];
    unsigned int read = XBMC->ReadFile(req_handle, resp_buf, sizeof(resp_buf));

    if (read > 0)
    {
      //add zero at the end to turn response into string
      resp_buf[read] = '\0';
      char* token = strtok( resp_buf, ",");
      while( token != NULL )
      {
        response_values.push_back(token);
        /* Get next token: */
        token = strtok(NULL, ",");
      }
      ret_val = response_values.size() > 0;
    }

    XBMC->CloseFile(req_handle);
  }

  return ret_val;
}

long long TimeShiftBuffer::Length()
{
  long long ret_val = 0;

  time_t duration;
  long long cur_pos;
  GetBufferParams(ret_val, duration, cur_pos);

  return ret_val;
}

bool TimeShiftBuffer::GetBufferParams(long long& length, time_t& duration, long long& cur_pos)
{
  bool ret_val = false;

  std::string req_url = streampath_;
  req_url += "&get_stats=1";

  std::vector<std::string> response_values;
  if (ExecuteServerRequest(req_url, response_values) &&
    response_values.size() == 3)
  {
    length = atoll(response_values[0].c_str());
    duration = (time_t)atoll(response_values[1].c_str());
    cur_pos = atoll(response_values[2].c_str());
    ret_val = true;
  }

  return ret_val;
}

int TimeShiftBuffer::ReadData(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return XBMC->ReadFile(m_streamHandle, pBuffer, iBufferSize);
}

time_t TimeShiftBuffer::GetPlayingTime()
{
  time_t ret_val = last_pos_;

  time_t now;
  now = time(NULL);

  if (last_pos_req_time_ == -1 || now > last_pos_req_time_ + 1)
  {
    long long length, cur_pos;
    time_t duration;
    if (GetBufferParams(length, duration, cur_pos))
    {
      ret_val = now - (time_t)((length - cur_pos) * duration / length);
    }

    last_pos_ = ret_val;
    last_pos_req_time_ = now;
  }

  return ret_val;
}

time_t TimeShiftBuffer::GetBufferTimeStart()
{
  time_t ret_val = 0;

  time_t now;
  now = time(NULL);

  long long length, cur_pos;
  time_t duration;
  if (GetBufferParams(length, duration, cur_pos))
    ret_val = now - duration;

  return ret_val;
}

time_t TimeShiftBuffer::GetBufferTimeEnd()
{
  time_t now;
  now = time(NULL);

  return now;
}
