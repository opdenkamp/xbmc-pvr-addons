
#include "webclient.h"
#include "client.h"

using namespace ADDON;

WebClient::WebClient()
{
#ifdef WMCPVR_TEST_CLIENT
	pHttpSession = NULL;
	pHttpFile = NULL;
#endif

	strServerName.clear();
	strClientName.clear();
	strRequest.clear();
	intServerPort = 0;

}

WebClient::~WebClient()
{
//	pHttpFile->Close();
//    pHttpConnection->Close();
//    pHttpSession->Close();
//    delete pHttpFile;
//    delete pHttpConnection;
#ifdef WMCPVR_TEST_CLIENT
	if (pHttpFile) delete pHttpFile;
    if (pHttpSession) delete pHttpSession;
#endif
}


bool WebClient::SendRequest()
{
	bool bResult = false;

	if (!(strServerName.size())) return false;
	if (!(strClientName.size())) return false;
	if (!(strRequest.size())) return false;
	if (!(intServerPort)) return false;

	CStdString strURL;
	strURL.Format("http://%s:%d/?Client=%s&Cmd=%s", strServerName, intServerPort, strClientName, strRequest);
//	strURL.Format("http://www.google.fi");

#ifdef WMCPVR_TEST_CLIENT
	if (pHttpFile) delete pHttpFile;
	if (pHttpSession) delete pHttpSession;

	pHttpSession = new CInternetSession("Pvr.WMC");
	pHttpFile = pHttpSession->OpenURL(strURL);

	if (pHttpFile) bResult = true;
#else
	webFileHandle = XBMC->OpenFile(strURL, 0);
	if (webFileHandle) bResult = true;

#endif
	return bResult;
}

bool WebClient::ReadNextString()
{
	bool bResult = false;
	strResponse.clear();
#ifdef WMCPVR_TEST_CLIENT
	CString tempResponse;
	bool bResult = pHttpFile->ReadString(tempResponse);
	if (bResult) strResponse.append(tempResponse);
#else
    char buffer[1024];
    if (XBMC->ReadFileString(webFileHandle, buffer, 1024)) {
		strResponse.append(buffer);
		XBMC->Log(LOG_ERROR, "WebClient: %s", strResponse.c_str());
	} else {
		XBMC->Log(LOG_ERROR, "WebClient: none");
	}
	
#endif

	return bResult;
}

void WebClient::CloseRequest()
{
	strRequest.clear();
	strResponse.clear();

#ifdef WMCPVR_TEST_CLIENT
	if (pHttpFile) {
		delete pHttpFile;
		pHttpFile = NULL;
	}
	pHttpSession->Close();
	if (pHttpSession) {
		delete pHttpSession;
		pHttpSession = NULL;
	}
#else
    XBMC->CloseFile(webFileHandle);
#endif
}

void WebClient::SetClientName(CStdString &clientName)
{
	strClientName = clientName;
}

void WebClient::SetServerName(CStdString &serverName)
{
	strServerName = serverName;

//	DWORD size = 50;
//	GetComputerName(_clientName, &size);
}

CStdString WebClient::GetServerName()
{
	return strServerName;
}

void WebClient::SetServerPort(int serverPort)
{
	intServerPort = serverPort;
}

CStdString WebClient::GetString(const CStdString &request)
{
	CStdString result;
	strRequest = request;
	if (SendRequest()) {
		while (ReadNextString()) {
			result.append(strResponse);
		}
		CloseRequest();
	}

	return result;
}

std::vector<CStdString> WebClient::GetVector(const CStdString &request)
{
	std::vector<CStdString> results;
	strRequest = request;
	if (SendRequest()) {
		while (ReadNextString()) {
			results.push_back(strResponse);				// fill results line by line
		}
		CloseRequest();
	}
	return results;
}

bool WebClient::GetBool(const CStdString &request)
{
	return GetString(request) == "True";
}

int WebClient::GetInt(const CStdString &request)
{
	return atol(GetString(request).c_str());
}

/** http handling */
/*
int DoRequest(std::string request, std::string &response)
{
  //PLATFORM::CLockObject lock(m_mutex);

  // build request string, adding SID if requred
  //CStdString strURL;
  //if (strstr(resource, "method=session") == NULL)
  //  strURL.Format("http://%s:%d%s&sid=%s", g_szHostname, g_iPort, resource, m_sid);
  //else
  //  strURL.Format("http://%s:%d%s", g_szHostname, g_iPort, resource);

  // ask XBMC to read the URL for us
  int resultCode = HTTP_NOTFOUND;
  void* fileHandle = XBMC->OpenFile(request, 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (XBMC->ReadFileString(fileHandle, buffer, 1024))
      response.append(buffer);
    XBMC->CloseFile(fileHandle);
    resultCode = HTTP_OK;
  }
  
  return resultCode;
}
*/
/*
int main(int argc, char** argv) {
	setupSocket(argv[1]);
	connectToHost(argv[1]);
	sendRequest(argv[1]);
	getResponse();
	
	return 0;
}
*/



/*
//int WebClient::Read(unsigned char *pBuffer, unsigned int iBufferSize)
//{
//	// copied form ReadNext
//	DWORD cbBytesRead;
//	bool bResult;
//	
////	if (OpenPipe())
////	{
//		if (_hPipe == INVALID_HANDLE_VALUE)
//		{
//			DebugString(_T("Pipe is not open in SendString\n"));
//			return false;
//		}
//
//		bResult = ReadFile(			// Read from the pipe.
//			_hPipe,					// Handle of the pipe
//			pBuffer,				// Buffer to receive the reply
//			iBufferSize,			// Size of buffer 
//			&cbBytesRead,			// Number of bytes read 
//			NULL);					// Not overlapped 
//
//
//		if (!bResult)											// if there was an error
//		{
//			if (GetLastError() == ERROR_PIPE_NOT_CONNECTED)		// this will be true if pipe is closed by server (server finished),
//			{													// detect that here to quit the read loop
//				return 0;
//			}
//			else if (GetLastError() != ERROR_MORE_DATA)
//			{
//				DebugString("ReadFile failed w/err 0x%08lx\n", GetLastError());
//				return 0;
//			}
//		} 
//
//		return cbBytesRead;								// return true, we got some data to read
////	}
////	return 0;
//		
//}
*/


