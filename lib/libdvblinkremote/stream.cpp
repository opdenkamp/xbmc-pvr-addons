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

#include "response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

Stream::Stream() 
  : m_channelHandle(-1), m_url("") 
{ 

}

Stream::Stream(const long channelHandle, const std::string& url) 
  : m_channelHandle(channelHandle), m_url(url) 
{ 

}

Stream::Stream(Stream& stream) 
  : m_channelHandle(stream.GetChannelHandle()), m_url(stream.GetUrl())
{ 

}

Stream::~Stream() 
{ 

}

long Stream::GetChannelHandle() 
{ 
  return m_channelHandle; 
}

void Stream::SetChannelHandle(const long channelHandle) 
{ 
  m_channelHandle = channelHandle; 
}

std::string& Stream::GetUrl() 
{ 
  return m_url; 
}

void Stream::SetUrl(const std::string& url) 
{ 
  m_url = url; 
}

bool StreamResponseSerializer::ReadObject(Stream& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("stream");
    long channelHandle = Util::GetXmlFirstChildElementTextAsLong(elRoot, "channel_handle");
    std::string url = Util::GetXmlFirstChildElementText(elRoot, "url");
    object.SetChannelHandle(channelHandle);
    object.SetUrl(url);

    return true;
  }
  
  return false;
}
