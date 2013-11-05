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

#include "curlhttpclient.h"

using namespace dvblinkremotehttp;

CurlHttpClient::CurlHttpClient()
  : m_curlHandle(NULL),
    m_curlCallbackPrepared(false),
    m_curlAuthenticationPrepared(false),
    m_requestSuccess(false),
    m_proxyUrl(""),
    m_verboseLogging(false),
    HttpClient()
{
  ClearCurlCallbackBuffers();
  m_curlHandle = curl_easy_init();
}

CurlHttpClient::CurlHttpClient(const std::string& proxyUrl)
  : m_curlHandle(NULL),
    m_curlCallbackPrepared(false),
    m_curlAuthenticationPrepared(false),
    m_requestSuccess(false),
    m_proxyUrl(proxyUrl),
    m_verboseLogging(false),
    HttpClient()
{
  ClearCurlCallbackBuffers();
  m_curlHandle = curl_easy_init();
}

CurlHttpClient::CurlHttpClient(CurlHttpClientDebugCallbackFunc& debugCallbackFunction)
  : m_curlHandle(NULL),
    m_curlCallbackPrepared(false),
    m_curlAuthenticationPrepared(false),
    m_requestSuccess(false),
    m_proxyUrl(""),
    m_verboseLogging(true),
    m_debugCallbackFunction(debugCallbackFunction),
    HttpClient()
{
  ClearCurlCallbackBuffers();
  m_curlHandle = curl_easy_init(); 
}

CurlHttpClient::CurlHttpClient(const std::string& proxyUrl, CurlHttpClientDebugCallbackFunc& debugCallbackFunction)
  : m_curlHandle(NULL),
    m_curlCallbackPrepared(false),
    m_curlAuthenticationPrepared(false),
    m_requestSuccess(false),
    m_proxyUrl(proxyUrl),
    m_verboseLogging(true),
    m_debugCallbackFunction(debugCallbackFunction),
    HttpClient()
{
  ClearCurlCallbackBuffers();
  m_curlHandle = curl_easy_init(); 
}

CurlHttpClient::~CurlHttpClient()
{
  if (m_curlHandle) {
      curl_easy_cleanup(m_curlHandle);
      m_curlHandle = NULL;
  }
}

bool CurlHttpClient::SendRequest(HttpWebRequest& request)
{
  bool result = false;
  
  PrepareCurlRequest(request);
  curl_easy_setopt(m_curlHandle, CURLOPT_URL, request.GetUrl().c_str());

  struct curl_slist* headers = NULL;
  PrepareCurlHeaders(request, headers);

  if (request.Method == DVBLINK_REMOTE_HTTP_POST_METHOD) {
    curl_easy_setopt(m_curlHandle, CURLOPT_POST, 1);
    curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDS, request.GetRequestData().c_str());
    
    if (request.ContentLength > 0) {
      curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE, request.ContentLength);
    }
  }

  PrepareCurlAuthentication(request);

  if (curl_easy_perform(m_curlHandle) == CURLE_OK) {
    m_requestSuccess = true;
    result = true;
  }

  if (headers) {
    curl_slist_free_all(headers);
  }

  return result;
}

HttpWebResponse* CurlHttpClient::GetResponse() 
{
  if (!m_requestSuccess || !m_curlHandle) {
    return NULL;
  }

  long responseCode;

  curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &responseCode);  
  
  HttpWebResponse* response = new HttpWebResponse(responseCode, m_callbackData);
  
  char* contentType;
  curl_easy_getinfo(m_curlHandle, CURLINFO_CONTENT_TYPE, &contentType);

  if (contentType) {
    response->ContentType.assign(contentType);
  }

  double contentLength;
  curl_easy_getinfo(m_curlHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);

  if (contentLength != -1) {
    response->ContentLength = (long)contentLength;
  }

  return response;
}

void CurlHttpClient::GetLastError(std::string& err)
{
  m_errorBuffer[dvblinkremote::DVBLINK_REMOTE_DEFAULT_BUFFER_SIZE - 1] = dvblinkremote::DVBLINK_REMOTE_EOF;
  err.assign(m_errorBuffer);
}

void CurlHttpClient::UrlEncode(const std::string& str, std::string& outEncodedStr)
{
  if (m_curlHandle) {
    outEncodedStr.assign(curl_easy_escape(m_curlHandle, str.c_str(), str.length()));
  }
}

void CurlHttpClient::PrepareCurlRequest(HttpWebRequest& request)
{
  if (!m_curlHandle) {
    return;
  }

  // Reset any existing request we may have
  curl_easy_setopt(m_curlHandle, CURLOPT_CUSTOMREQUEST, NULL);

  ClearCurlCallbackBuffers();
  PrepareCurlCurlback();
  PrepareCurlAuthentication(request);
}

void CurlHttpClient::PrepareCurlCurlback()
{
  if (!m_curlHandle) {
    return;
  }

  if (!m_curlCallbackPrepared) {
    // Set buffer to get error
    curl_easy_setopt(m_curlHandle, CURLOPT_ERRORBUFFER, m_errorBuffer);

    // Set callback function to get response
    curl_easy_setopt( m_curlHandle, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt( m_curlHandle, CURLOPT_WRITEDATA, this);

    if (!m_proxyUrl.empty()) {
      curl_easy_setopt(m_curlHandle, CURLOPT_PROXY, m_proxyUrl.c_str());
    }

    if (m_verboseLogging) {
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1);
      curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGFUNCTION, CurlHttpClient::CurlDebugCallback);
      curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGDATA, this);
    }

    m_curlCallbackPrepared = true;
  }
}

void CurlHttpClient::PrepareCurlHeaders(HttpWebRequest& request, curl_slist* headers)
{
  if (!m_curlHandle) {
    return;
  }

  if (!request.ContentType.empty()) {
    headers = curl_slist_append(headers, std::string(DVBLINK_REMOTE_HTTP_HEADER_CONTENT_TYPE + ":" + request.ContentType).c_str()); 
  }

  if (headers) {
    curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, headers);
  }
}

void CurlHttpClient::PrepareCurlAuthentication(HttpWebRequest& request)
{
  if (!m_curlHandle) {
    return;
  }

  if (!m_curlAuthenticationPrepared && 
    (!request.UserName.empty() || !request.Password.empty())) {
    std::string credentials = request.UserName + ":" + request.Password;

    curl_easy_setopt(m_curlHandle, CURLOPT_USERPWD, credentials.c_str());
    curl_easy_setopt(m_curlHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

    m_curlAuthenticationPrepared = true;
  }
}

void CurlHttpClient::ClearCurlCallbackBuffers()
{
  m_callbackData = "";
  memset(m_errorBuffer, 0, dvblinkremote::DVBLINK_REMOTE_EOF);
  m_requestSuccess = false;
}

size_t CurlHttpClient::CurlWriteCallback(char* buffer, size_t size, size_t nmemb, CurlHttpClient* pHttpCurlClientObj)
{
  size_t writtenSize = 0;

  if (pHttpCurlClientObj && buffer) {
      writtenSize = pHttpCurlClientObj->SaveLastWebResponse(buffer, (size * nmemb));
  }

  return writtenSize;
}

size_t CurlHttpClient::SaveLastWebResponse(char*& buffer, size_t size)
{
  size_t bytesWritten = 0;

  if (buffer && size )
  {
      m_callbackData.append(buffer, size);
      bytesWritten = size;
  }
  
  return bytesWritten;
}

int CurlHttpClient::CurlDebugCallback(CURL* curl, curl_infotype infotype, char* data, size_t size, CurlHttpClient* pHttpCurlClientObj)
{
  if (pHttpCurlClientObj && pHttpCurlClientObj->m_verboseLogging && pHttpCurlClientObj->m_debugCallbackFunction && data) {
    pHttpCurlClientObj->Debug(infotype, data, size);
  }

  return 0;
}

void CurlHttpClient::Debug(curl_infotype infotype, char* data, size_t size) 
{
  size_t newSize = size;

  if (data[size - 1] == dvblinkremote::DVBLINK_REMOTE_NEWLINE) {
    newSize = size - 1;
  }

  std::string message = std::string(data, newSize);
  std::string type = "";
  
  switch(infotype)
  {
  case CURLINFO_TEXT:
    type = DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_TEXT;
    break;
  case CURLINFO_DATA_OUT:
    type = DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_SENT_REQUEST_DATA;
    break;
  case CURLINFO_DATA_IN:
    type = DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_RECIEVED_RESPONSE_DATA;
    break;
  case CURLINFO_HEADER_OUT:
    type = DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_SENT_REQUEST_HEADERS;
    break;
  case CURLINFO_HEADER_IN:
    type = DVBLINK_REMOTE_HTTP_CURL_DEBUG_MESSAGE_TYPE_RECIEVED_RESPONSE_HEADERS;
    break;
  }

  // Call the registered callback debug function.
  m_debugCallbackFunction(type, message);
}
