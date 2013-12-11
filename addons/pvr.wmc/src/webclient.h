#pragma once

#ifdef WMCPVR_TEST_CLIENT
#include <Afxdisp.h>
#include <afxinet.h>
#endif
#include <vector>
#include "platform/os.h"
#include "platform/util/StdString.h"

//#include <windows.h>
//#include <string>
//#include <vector>

//#include <atlstr.h>
//#include <strsafe.h>

#define CLIENT_BUFFER_SIZE		1024			// buffer size for holding IO strings

class WebClient
{
public:
	WebClient();
	~WebClient();
	
	void SetClientName(CStdString &clientName);
	void SetServerName(CStdString &serverName);
	CStdString GetServerName();
	void SetServerPort(int serverPort);

	CStdString GetString(const CStdString &request);
	std::vector<CStdString> GetVector(const CStdString &request);
	bool GetBool(const CStdString &request);
	int GetInt(const CStdString &request);

	//int Read(unsigned char *pBuffer, unsigned int iBufferSize);
	
private:

#ifdef WMCPVR_TEST_CLIENT
	CInternetSession *pHttpSession;
	CStdioFile *pHttpFile;
#else
	void* webFileHandle;
#endif

	CStdString strServerName;
	CStdString strClientName;
	int intServerPort;

	CStdString strResponse;
	CStdString strRequest;
	
	bool SendRequest();
	bool ReadNextString();
	void CloseRequest();
};
