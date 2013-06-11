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

#include "dvblinkremotehttp.h"

using namespace dvblinkremotehttp;

HttpWebRequest::HttpWebRequest(const std::string& url)
  : m_url(url)
{
  Method = DVBLINK_REMOTE_HTTP_POST_METHOD;
  ContentType = "";
  ContentLength = 0;
  m_requestData = "";
}

HttpWebRequest::~HttpWebRequest()
{

}

std::string& HttpWebRequest::GetUrl()
{
  return m_url;
}

std::string& HttpWebRequest::GetRequestData()
{
  return m_requestData;
}

void HttpWebRequest::SetRequestData(const std::string& data)
{
  m_requestData = data;
}

HttpWebResponse::HttpWebResponse(const int statusCode, const std::string& responseData)
  : m_statusCode(statusCode), 
    m_responseData(responseData)
{
  ContentType = "";
  ContentLength = 0;
}

HttpWebResponse::~HttpWebResponse() 
{

}

int HttpWebResponse::GetStatusCode()
{
  return m_statusCode;
}

std::string& HttpWebResponse::GetResponseData()
{
  return m_responseData;
}
