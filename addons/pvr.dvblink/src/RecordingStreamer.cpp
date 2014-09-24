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

#include <time.h>
#include "RecordingStreamer.h"
 
using namespace dvblinkremotehttp;
using namespace dvblinkremote;
using namespace ADDON;

RecordingStreamer::RecordingStreamer(ADDON::CHelper_libXBMC_addon* xbmc, const std::string& client_id, 
    const std::string& hostname, long port, const std::string& username, const std::string& password)
    : xbmc_(xbmc), playback_handle_(NULL), client_id_(client_id), hostname_(hostname), username_(username), password_(password), port_(port), check_delta_(30)
{
    http_client_ = new HttpPostClient(xbmc_, hostname_, port_, username_, password_);
    dvblink_remote_con_ = DVBLinkRemote::Connect((HttpClient&)*http_client_, hostname_.c_str(), port_, username_.c_str(), password_.c_str());
}

RecordingStreamer::~RecordingStreamer()
{
    delete dvblink_remote_con_;
    delete http_client_;
}

bool RecordingStreamer::OpenRecordedStream(const char* recording_id, std::string& url)
{
    recording_id_ = recording_id;
    url_ = url;
    cur_pos_ = 0;

    prev_check_ = time(NULL);
    get_recording_info(recording_id_, recording_size_, is_in_recording_);

    playback_handle_ = xbmc_->OpenFile(url_.c_str(), 0);

    return playback_handle_ != NULL;
}

void RecordingStreamer::CloseRecordedStream(void)
{
    if (playback_handle_ != NULL)
    {
        xbmc_->CloseFile(playback_handle_);
        playback_handle_ = NULL;
    }
}

int RecordingStreamer::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
    //if this is recording in progress - update its size every minute
    if (is_in_recording_)
    {
        time_t now = time(NULL);
        if (now - prev_check_ > check_delta_)
        {
            get_recording_info(recording_id_, recording_size_, is_in_recording_);

            //reopen original data connection to refresh its file size
            xbmc_->CloseFile(playback_handle_);
            playback_handle_ = xbmc_->OpenFile(url_.c_str(), 0);
            xbmc_->SeekFile(playback_handle_, cur_pos_, SEEK_SET);

            prev_check_ = now;
        }
    }

    unsigned int n = xbmc_->ReadFile(playback_handle_, pBuffer, iBufferSize);
    cur_pos_ += n;

    return n;
}

long long RecordingStreamer::SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
    cur_pos_ = xbmc_->SeekFile(playback_handle_, iPosition, iWhence);
    return cur_pos_;
}

long long RecordingStreamer::PositionRecordedStream(void)
{
    return cur_pos_;
}

long long RecordingStreamer::LengthRecordedStream(void)
{
    return recording_size_;
}

bool RecordingStreamer::get_recording_info(const std::string& recording_id, long long& recording_size, bool& is_in_recording)
{
    bool ret_val = false;
    recording_size = -1;
    is_in_recording = false;

    GetPlaybackObjectRequest getPlaybackObjectRequest(hostname_.c_str(), recording_id);
    getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = false;
    GetPlaybackObjectResponse getPlaybackObjectResponse;

    if (dvblink_remote_con_->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse) != DVBLINK_REMOTE_STATUS_OK)
    {
        std::string error;
        dvblink_remote_con_->GetLastError(error);
        xbmc_->Log(LOG_ERROR, "RecordingStreamer::get_recording_info: Could not get recording info for recording id %s", recording_id.c_str());
    }
    else
    {
        PlaybackItemList& item_list = getPlaybackObjectResponse.GetPlaybackItems();
        if (item_list.size() > 0)
        {
            PlaybackItem* item = item_list[0];
            RecordedTvItem* rectv_item = static_cast<RecordedTvItem*>(item);
            recording_size = rectv_item->Size;
            is_in_recording = rectv_item->State == RecordedTvItem::RECORDED_TV_ITEM_STATE_IN_PROGRESS;
            ret_val = true;
        }
        else
        {

        }
    }

    return ret_val;
}
