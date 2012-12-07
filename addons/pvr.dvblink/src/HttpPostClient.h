
#include "libdvblinkremote/dvblinkremote.h"
#include "libdvblinkremote/dvblinkremotehttp.h"
#include "libXBMC_addon.h"

using namespace dvblinkremotehttp;
using namespace ADDON;

class HttpPostClient : public HttpClient
{
public :
	bool SendRequest(HttpWebRequest& request);
	HttpWebResponse* GetResponse();
	void GetLastError(std::string& err);
	void UrlEncode(const std::string& str, std::string& outEncodedStr);
	HttpPostClient(CHelper_libXBMC_addon *XBMC, const std::string& server, const int serverport, const std::string& username, const std::string& password);

private :
	int SendPostRequest(HttpWebRequest& request);
	std::string server;
	long serverport;
	std::string username;
	std::string password;
	CHelper_libXBMC_addon  *XBMC;
	std::string responseData;
	int lastReqeuestErrorCode;

};
