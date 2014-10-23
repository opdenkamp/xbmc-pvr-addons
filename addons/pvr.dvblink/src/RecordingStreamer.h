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
 
#pragma once

#include "libXBMC_addon.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "HttpPostClient.h"

class RecordingStreamer
{
public :
    RecordingStreamer(ADDON::CHelper_libXBMC_addon* xbmc, const std::string& client_id, const std::string& hostname, long port, const std::string& username, const std::string& password);
    virtual ~RecordingStreamer();

    bool OpenRecordedStream(const char* recording_id, std::string& url);
    void CloseRecordedStream(void);
    int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
    long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */);
    long long PositionRecordedStream(void);
    long long LengthRecordedStream(void);
protected:
    ADDON::CHelper_libXBMC_addon* xbmc_;
    std::string recording_id_;
    std::string url_;
    long long recording_size_;
    bool is_in_recording_;
    void* playback_handle_;
    long long cur_pos_;
    std::string client_id_;
    std::string hostname_;
    std::string username_;
    std::string password_;
    HttpPostClient* http_client_;
    dvblinkremote::IDVBLinkRemoteConnection* dvblink_remote_con_;
    long port_;
    time_t prev_check_;
    time_t check_delta_;

    bool get_recording_info(const std::string& recording_id, long long& recording_size, bool& is_in_recording);
};
