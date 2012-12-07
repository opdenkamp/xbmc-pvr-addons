#include "HttpPostClient.h"
#include "base64.h"

#ifdef _WIN32
#include <Winsock2.h>
#else
 #include <netdb.h>
#endif

#define SEND_RQ(MSG) \
  /*cout<<send_str;*/ \
  send(sock,MSG,strlen(MSG),0);


/* Converts a hex character to its integer value */
char from_hex(char ch)
{
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code)
{
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(const char *str)
{
  char *pstr = (char*) str, *buf = (char *)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr)
{
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(const char *str)
{
  char *pstr = (char *)str, *buf = (char*)malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr)
{
    if (*pstr == '%')
{
      if (pstr[1] && pstr[2])
{
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+')
{ 
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}


HttpPostClient::HttpPostClient(CHelper_libXBMC_addon  *XBMC, const std::string& server, const int serverport, const std::string& username, const std::string& password)
{
  this->XBMC = XBMC;
  m_server = server;
  m_serverport = serverport;
  m_username = username;
  m_password = password;
}

int HttpPostClient::SendPostRequest(HttpWebRequest& request)
{
  std::string buffer;
  std::string message;
  char content_header[100];

  buffer.append("POST /cs/ HTTP/1.0\r\n");
  sprintf(content_header,"Host: %s:%d\r\n",m_server.c_str(),(int)m_serverport);
  buffer.append(content_header);
  buffer.append("Content-Type: application/x-www-form-urlencoded\r\n");
  if (m_username.compare("") != 0)
  {
    sprintf(content_header,"%s:%s",m_username.c_str(),m_password.c_str());
    sprintf(content_header, "Authorization: Basic %s\r\n",base64_encode((const char*)content_header,strlen(content_header)).c_str());
    buffer.append(content_header);
  }
  sprintf(content_header,"Content-Length: %d\r\n",request.ContentLength);
  buffer.append(content_header);
  buffer.append("\r\n");
  buffer.append(request.GetRequestData());

  #ifdef _WIN32
  {
    WSADATA  WsaData;
    WSAStartup(0x0101, &WsaData);
  }
  #endif

  sockaddr_in sin;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
  {
    return -100;
  }
  sin.sin_family = AF_INET;
  sin.sin_port = htons((unsigned short)m_serverport); 

  struct hostent * host_addr = gethostbyname(m_server.c_str());
  if (host_addr==NULL)
  {
    return -103;
  }
  sin.sin_addr.s_addr = *((int*)*host_addr->h_addr_list) ;

  if (connect(sock, (const struct sockaddr *)&sin, sizeof(sockaddr_in)) == -1 )
  {
    return -101;
  }

  SEND_RQ(buffer.c_str());

  char c1[1];
  int l,line_length;
  bool loop = true;
  bool bHeader = false;


  while (loop)
  {
    l = recv(sock, c1, 1, 0);
    if (l<0) loop = false;
    if (c1[0]=='\n')
    {
      if (line_length == 0)
        loop = false;
      line_length = 0;
      if (message.find("200") != 0)
        bHeader = true;
    }
    else if (c1[0]!='\r')
    {
      line_length++;
    }
    message += c1[0];
  }

  message="";
  if (bHeader)
  {
    char p[1024];
    while ((l = recv(sock,p,1023,0)) > 0)  {
      p[l] = '\0';
      message += p;
    }
  }
  else 
  {
    return -102;
  }




  //TODO: Use xbmc file code when it allows to post content-type application/x-www-form-urlencoded and authentication
  /*
  void* hFile = XBMC->OpenFileForWrite(request.GetUrl().c_str(), 0);
  if (hFile != NULL)
  {

    int rc = XBMC->WriteFile(hFile, buffer.c_str(), buffer.length());
    if (rc >= 0)
    {
      std::string result;
      result.clear();
      char buffer[1024];
      while (XBMC->ReadFileString(hFile, buffer, 1023))
        result.append(buffer);

    }
    else
    {
      XBMC->Log(LOG_ERROR, "can not write to %s",request.GetUrl().c_str());
    }

    XBMC->CloseFile(hFile);
  }
  else
  {
    XBMC->Log(LOG_ERROR, "can not open %s for write", request.GetUrl().c_str());
  }

  */
  m_responseData.clear();
  m_responseData.append(message);
  return 200;
}

bool HttpPostClient::SendRequest(HttpWebRequest& request)
{
  m_lastReqeuestErrorCode = SendPostRequest(request);
  return (m_lastReqeuestErrorCode == 200);
}

HttpWebResponse* HttpPostClient::GetResponse()
{
  if (m_lastReqeuestErrorCode != 200)
  {
    return NULL;
  }
  HttpWebResponse* response = new HttpWebResponse(200, m_responseData);
  return response;
}

void HttpPostClient::GetLastError(std::string& err)
{

}

void HttpPostClient::UrlEncode(const std::string& str, std::string& outEncodedStr)
{
  outEncodedStr.append(url_encode(str.c_str()));
}

