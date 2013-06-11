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

#pragma once

#include <string>
#include "dvblinkremote.h"
#include "dvblinkremotehttp.h"
#include "curl/curl.h"

namespace dvblinkremotehttp 
{
  typedef void (CurlHttpClientDebugCallbackFunc)(std::string type, std::string message); 

  const std::string DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_TEXT = "TEXT";
  const std::string DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_SENT_REQUEST_DATA = "SENT_REQUEST_DATA";
  const std::string DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_SENT_REQUEST_HEADERS = "SENT_REQUEST_HEADERS";
  const std::string DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_RECIEVED_RESPONSE_DATA = "RECIEVED_RESPONSE_DATA";
  const std::string DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_RECIEVED_RESPONSE_HEADERS = "RECIEVED_RESPONSE_HEADERS";

  class CurlHttpClient : public HttpClient
  {
  public:
    CurlHttpClient();
    CurlHttpClient(const std::string& proxyUrl);
    CurlHttpClient(CurlHttpClientDebugCallbackFunc& debugCallbackFunction);
    CurlHttpClient(const std::string& proxyUrl, CurlHttpClientDebugCallbackFunc& debugCallbackFunction);
    ~CurlHttpClient();
    bool SendRequest(HttpWebRequest& request);
    HttpWebResponse* GetResponse();
    void GetLastError(std::string& err);
    void UrlEncode(const std::string& str, std::string& outEncodedStr);

  private:
    CURL* m_curlHandle;
    char m_errorBuffer[dvblinkremote::DVBLINK_REMOTE_DEFAULT_BUFFER_SIZE];
    std::string m_callbackData;
    bool m_curlCallbackPrepared;
    bool m_curlAuthenticationPrepared;
    bool m_requestSuccess;
    std::string m_proxyUrl;
    bool m_verboseLogging;
    CurlHttpClientDebugCallbackFunc* m_debugCallbackFunction;

    void PrepareCurlRequest(HttpWebRequest& request);
    void PrepareCurlCurlback();
    void PrepareCurlHeaders(HttpWebRequest& request, curl_slist* headers);
    void PrepareCurlAuthentication(HttpWebRequest& request);
    size_t SaveLastWebResponse(char*& buffer, size_t size);
    void Debug(curl_infotype infotype, char* data, size_t size);
    void ClearCurlCallbackBuffers();
    static size_t CurlWriteCallback(char* buffer, size_t size, size_t nmemb, CurlHttpClient* pHttpCurlClientObj);
    static int CurlDebugCallback(CURL* curl, curl_infotype infotype, char* data, size_t size, CurlHttpClient* pHttpCurlClientObj);
  };
};
