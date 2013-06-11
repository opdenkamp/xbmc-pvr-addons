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

/**
  * Namespace for HTTP specific functionality in the DVBLink Remote API library.
  */
namespace dvblinkremotehttp 
{
  /**
    * A constant string representing the HTTP method for post-requests.
    */
  const std::string DVBLINK_REMOTE_HTTP_POST_METHOD = "POST";

  /**
    * A constant string representing the name of the HTTP header Accept.
    */
  const std::string DVBLINK_REMOTE_HTTP_HEADER_ACCEPT = "Accept";

  /**
    * A constant string representing the name of the HTTP header Accept-Charset.
    */
  const std::string DVBLINK_REMOTE_HTTP_HEADER_ACCEPT_CHARSET = "Accept-Charset";

  /**
    * A constant string representing the name of the HTTP header Content-Type.
    */
  const std::string DVBLINK_REMOTE_HTTP_HEADER_CONTENT_TYPE = "Content-Type";

  /**
    * Class for defining a HTTP web request.
    * This is used as input parameter for the HttpClient::SendRequest() method.
    * @see HttpClient::SendRequest()
    */
  class HttpWebRequest
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremotehttp::HttpWebRequest class.
      * @param url A constant string representing the URL for which the request will be sent.
      */
    HttpWebRequest(const std::string& url);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~HttpWebRequest();

    /**
      * Gets the URL.
      * @return A string reference
      */
    std::string& GetUrl();

    /**
      * Gets the data to be sent in the request (using POST method).
      * @return A string reference
      */
    std::string& GetRequestData();

    /**
      * Sets the data to be sent in the request (using POST method).
      * @param data A constant string reference representing the data to be sent in the request.
      */
    void SetRequestData(const std::string& data);

    /**
      * The HTTP method to be used in request.
      * @see DVBLINK_REMOTE_HTTP_POST_METHOD for definition of the POST-method.
      */
    std::string Method;

    /**
      * The MIME type of the data to be sent in request (using POST method).
      */
    std::string ContentType;

    /**
      * The length of the data to be sent in request  (using POST method).
      */
    long ContentLength;

    /**
      * The user name to be used for basic authentication sent in request.
      */
    std::string UserName;

    /**
      * The password to be used for basic authentication sent in request.
      */
    std::string Password;

  private:
    std::string m_url;
    std::string m_requestData;
  };

  /**
    * Class for defining a HTTP web response.
    * This is used as return parameter for the HttpClient::GetResponse method.
    * @see HttpClient::GetResponse()
    */
  class HttpWebResponse 
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremotehttp::HttpWebResponse class.
      * @param statusCode   A constant integer representing the HTTP response code.
      * @param responseData A constant string reference representing the HTTP response data.
      */
    HttpWebResponse(const int statusCode, const std::string& responseData);

    /**
      * Destructor for cleaning up allocated memory.
      */
    ~HttpWebResponse();

    /**
      * Gets the HTTP response code.
      * @return An integer value
      */
    int GetStatusCode();

    /**
      * Gets the HTTP response data.
      * @return A string reference
      */
    std::string& GetResponseData();

    /**
      * The HTTP response data MIME type.
      */
    std::string ContentType;

    /**
      * The HTTP response data length.
      */
    long ContentLength;
  
  private:
    int m_statusCode;
    std::string m_responseData;
  };

  /**
    * An abstract base class for communicating with a server using the HTTP protocol. 
    */
  class HttpClient
  {
  public:
    /**
      * Send a HTTP request.
      * @param[in] request A HttpWebRequest reference representing the HTTP request to be sent.
      * @return Boolean value representing if sending the HTTP request was successful or not.
      */
    virtual bool SendRequest(HttpWebRequest& request) = 0;

    /**
      * Get the HTTP response returned from a HTTP request.
      @return HttpWebResponse pointer representing the response returned from server.
      */
    virtual HttpWebResponse* GetResponse() = 0;

    /**
      * Gets a description of the last occured error.
      * @param[in,out] err A string reference representing the string where the description of the last error will be provided.
      */
    virtual void GetLastError(std::string& err) = 0;

    /**
      * URL encodes a string.
      * @param[in]      str           A constant string reference representing the url to be encoded.
      * @param[in,out]  outEncodedStr A constant string reference where the encoded url will be written.
      */
    virtual void UrlEncode(const std::string& str, std::string& outEncodedStr) = 0;
  };
};
