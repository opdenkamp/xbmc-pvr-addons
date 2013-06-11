/***************************************************************************
 * Copyright (C) 2012 Marcus Efraimsson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

#include "request.h"

using namespace dvblinkremote;

TranscodingOptions::TranscodingOptions(const unsigned int width, const unsigned int height) 
  : m_width(width), m_height(height) 
{
  m_bitrate = -1;
  m_audioTrack = "";
}

TranscodingOptions::~TranscodingOptions()
{

}

unsigned int TranscodingOptions::GetWidth() 
{ 
  return m_width; 
}

void TranscodingOptions::SetWidth(const unsigned int width) 
{ 
  m_width = width; 
}

unsigned int TranscodingOptions::GetHeight() 
{ 
  return m_height; 
}

void TranscodingOptions::SetHeight(const unsigned int height) 
{ 
  m_height = height; 
}

unsigned int TranscodingOptions::GetBitrate() 
{ 
  return m_bitrate; 
}

void TranscodingOptions::SetBitrate(const unsigned int bitrate) 
{ 
  m_bitrate = bitrate; 
}

std::string& TranscodingOptions::GetAudioTrack() 
{ 
  return m_audioTrack; 
}

void TranscodingOptions::SetAudioTrack(const std::string& audioTrack) 
{ 
  m_audioTrack = audioTrack; 
}
