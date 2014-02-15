/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
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

#ifndef VNSI_RECEIVER_H
#define VNSI_RECEIVER_H

#include <linux/dvb/frontend.h>
#include <linux/videodev2.h>
#include <vdr/channels.h>
#include <vdr/device.h>
#include <vdr/receiver.h>
#include <vdr/thread.h>
#include <list>

#include "parser.h"
#include "responsepacket.h"
#include "demuxer.h"
#include "videoinput.h"

class cxSocket;
class cChannel;
class cTSParser;
class cResponsePacket;
class cVideoBuffer;
class cVideoInput;

class cLiveStreamer : public cThread
{
private:
  friend class cParser;
  friend class cLivePatFilter;
  friend class cLiveReceiver;

  void sendStreamPacket(sStreamPacket *pkt);
  void sendStreamChange();
  void sendSignalInfo();
  void sendStreamStatus();
  void sendBufferStatus();
  void sendRefTime(sStreamPacket *pkt);

  int               m_ClientID;
  const cChannel   *m_Channel;                      /*!> Channel to stream */
  cDevice          *m_Device;
  cxSocket         *m_Socket;                       /*!> The socket class to communicate with client */
  int               m_Frontend;                     /*!> File descriptor to access used receiving device  */
  dvb_frontend_info m_FrontendInfo;                 /*!> DVB Information about the receiving device (DVB only) */
  v4l2_capability   m_vcap;                         /*!> PVR Information about the receiving device (pvrinput only) */
  cString           m_DeviceString;                 /*!> The name of the receiving device */
  bool              m_startup;
  bool              m_IsAudioOnly;                  /*!> Set to true if streams contains only audio */
  bool              m_IsMPEGPS;                     /*!> TS Stream contains MPEG PS data like from pvrinput */
  uint32_t          m_scanTimeout;                  /*!> Channel scanning timeout (in seconds) */
  cTimeMs           m_last_tick;
  bool              m_SignalLost;
  bool              m_IFrameSeen;
  cResponsePacket   m_streamHeader;
  cVNSIDemuxer      m_Demuxer;
  cVideoBuffer     *m_VideoBuffer;
  cVideoInput       m_VideoInput;
  int               m_Priority;
  uint8_t           m_Timeshift;
  cCondVar          m_Event;
  cMutex            m_Mutex;
  bool              m_IsRetune;

protected:
  virtual void Action(void);
  bool Open(int serial = -1);
  void Close();

public:
  cLiveStreamer(int clientID, uint8_t timeshift, uint32_t timeout = 0);
  virtual ~cLiveStreamer();

  void Activate(bool On);

  bool StreamChannel(const cChannel *channel, int priority, cxSocket *Socket, cResponsePacket* resp);
  bool IsStarting() { return m_startup; }
  bool IsAudioOnly() { return m_IsAudioOnly; }
  bool IsMPEGPS() { return m_IsMPEGPS; }
  bool SeekTime(int64_t time, uint32_t &serial);
  void RetuneChannel(const cChannel *channel);
};

#endif  // VNSI_RECEIVER_H
