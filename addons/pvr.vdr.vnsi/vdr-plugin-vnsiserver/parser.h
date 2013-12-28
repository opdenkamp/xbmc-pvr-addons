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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef VNSI_DEMUXER_H
#define VNSI_DEMUXER_H

#include <vdr/device.h>
#include <queue>

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

/* PES PIDs */
#define PRIVATE_STREAM1   0xBD
#define PADDING_STREAM    0xBE
#define PRIVATE_STREAM2   0xBF
#define PRIVATE_STREAM3   0xFD
#define AUDIO_STREAM_S    0xC0      /* 1100 0000 */
#define AUDIO_STREAM_E    0xDF      /* 1101 1111 */
#define VIDEO_STREAM_S    0xE0      /* 1110 0000 */
#define VIDEO_STREAM_E    0xEF      /* 1110 1111 */

#define AUDIO_STREAM_MASK 0x1F  /* 0001 1111 */
#define VIDEO_STREAM_MASK 0x0F  /* 0000 1111 */
#define AUDIO_STREAM      0xC0  /* 1100 0000 */
#define VIDEO_STREAM      0xE0  /* 1110 0000 */

#define ECM_STREAM        0xF0
#define EMM_STREAM        0xF1
#define DSM_CC_STREAM     0xF2
#define ISO13522_STREAM   0xF3
#define PROG_STREAM_DIR   0xFF

inline bool PesIsHeader(const uchar *p)
{
  return !(p)[0] && !(p)[1] && (p)[2] == 1;
}

inline int PesHeaderLength(const uchar *p)
{
  return 8 + (p)[8] + 1;
}

inline bool PesIsVideoPacket(const uchar *p)
{
  return (((p)[3] & ~VIDEO_STREAM_MASK) == VIDEO_STREAM);
}

inline bool PesIsMPEGAudioPacket(const uchar *p)
{
  return (((p)[3] & ~AUDIO_STREAM_MASK) == AUDIO_STREAM);
}

inline bool PesIsPS1Packet(const uchar *p)
{
  return ((p)[3] == PRIVATE_STREAM1 || (p)[3] == PRIVATE_STREAM3 );
}

inline bool PesIsAudioPacket(const uchar *p)
{
  return (PesIsMPEGAudioPacket(p) || PesIsPS1Packet(p));
}

enum eStreamContent
{
  scVIDEO,
  scAUDIO,
  scSUBTITLE,
  scTELETEXT,
  scPROGRAMM
};

enum eStreamType
{
  stNone,
  stAC3,
  stMPEG2AUDIO,
  stEAC3,
  stAACADTS,
  stAACLATM,
  stDTS,
  stMPEG2VIDEO,
  stH264,
  stDVBSUB,
  stTEXTSUB,
  stTELETEXT,
};

#define PKT_I_FRAME 1
#define PKT_P_FRAME 2
#define PKT_B_FRAME 3
#define PKT_NTYPES  4
struct sStreamPacket
{
  int64_t   id;
  int64_t   dts;
  int64_t   pts;
  int       duration;

  uint8_t   commercial;
  uint8_t   componentindex;

  uint8_t  *data;
  int       size;
  bool      streamChange;
  bool      pmtChange;
  uint32_t  serial;
  uint32_t  reftime;
};

struct sPtsWrap
{
  bool m_Wrap;
  int m_NoOfWraps;
  int m_ConfirmCount;
};

class cTSStream;

#define PES_HEADER_LENGTH 128

class cParser
{
friend class cTSStream;
public:
  cParser(int pID, cTSStream *stream, sPtsWrap *ptsWrap, bool observePtsWraps);
  virtual ~cParser();

  bool AddPESPacket(uint8_t *data, int size);
  virtual void Parse(sStreamPacket *pkt) = 0;
//  void ClearFrame() {m_PesBufferPtr = 0;}
  int ParsePacketHeader(uint8_t *data);
  int ParsePESHeader(uint8_t *buf, size_t len);
  virtual void Reset();
  bool IsVideo() {return m_IsVideo; }
  uint16_t GetError() { return m_Error; }

protected:
  virtual bool IsValidStartCode(uint8_t *buf, int size);

  uint8_t     m_PesHeader[PES_HEADER_LENGTH];
  int         m_PesHeaderPtr;
  int         m_PesPacketLength;
  uint8_t    *m_PesBuffer;
  int         m_PesBufferSize;
  int         m_PesBufferPtr;
  size_t      m_PesBufferInitialSize;
  size_t      m_PesParserPtr;
  size_t      m_PesNextFramePtr;
  int         m_PesTimePos;

  bool        m_FoundFrame;

  int         m_pID;
  int64_t     m_curPTS;
  int64_t     m_curDTS;
  int64_t     m_prevPTS;
  int64_t     m_prevDTS;

  bool        m_IsPusi;
  uint16_t    m_Error;

  cTSStream  *m_Stream;
  bool        m_IsVideo;
  sPtsWrap   *m_PtsWrap;
  bool        m_ObservePtsWraps;
};


class cTSStream
{
private:
  eStreamType           m_streamType;
  const int             m_pID;
  eStreamContent        m_streamContent;
  bool                  m_IsStreamChange;

  bool                  m_pesError;
  cParser              *m_pesParser;

  char                  m_language[4];  // ISO 639 3-letter language code (empty string if undefined)

  int                   m_FpsScale;     // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int                   m_FpsRate;
  int                   m_Height;       // height of the stream reported by the demuxer
  int                   m_Width;        // width of the stream reported by the demuxer
  float                 m_Aspect;       // display aspect of stream

  int                   m_Channels;
  int                   m_SampleRate;
  int                   m_BitRate;
  int                   m_BitsPerSample;
  int                   m_BlockAlign;

  unsigned char         m_subtitlingType;
  uint16_t              m_compositionPageId;
  uint16_t              m_ancillaryPageId;

public:
  cTSStream(eStreamType type, int pid, sPtsWrap *ptsWrap);
  virtual ~cTSStream();

  int ProcessTSPacket(uint8_t *data, sStreamPacket *pkt, bool iframe);
  bool ReadTime(uint8_t *data, int64_t *dts);
  void ResetParser();

  void SetLanguage(const char *language);
  const char *GetLanguage() { return m_language; }
  const eStreamContent Content() const { return m_streamContent; }
  const eStreamType Type() const { return m_streamType; }
  const int GetPID() const { return m_pID; }

  /* Video Stream Information */
  bool SetVideoInformation(int FpsScale, int FpsRate, int Height, int Width, float Aspect);
  void GetVideoInformation(uint32_t &FpsScale, uint32_t &FpsRate, uint32_t &Height, uint32_t &Width, double &Aspect);

  /* Audio Stream Information */
  bool SetAudioInformation(int Channels, int SampleRate, int BitRate, int BitsPerSample, int BlockAlign);
  void GetAudioInformation(uint32_t &Channels, uint32_t &SampleRate, uint32_t &BitRate, uint32_t &BitsPerSample, uint32_t &BlockAlign);

  /* Subtitle related stream information */
  void SetSubtitlingDescriptor(unsigned char SubtitlingType, uint16_t CompositionPageId, uint16_t AncillaryPageId);
  unsigned char SubtitlingType() const { return m_subtitlingType; }
  uint16_t CompositionPageId() const { return m_compositionPageId; }
  uint16_t AncillaryPageId() const { return m_ancillaryPageId; }

  static int64_t Rescale(int64_t a, int64_t b, int64_t c);
};

#endif // VNSI_DEMUXER_H
