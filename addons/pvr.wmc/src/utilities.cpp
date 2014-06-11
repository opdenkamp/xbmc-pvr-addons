
#include "tinyxml/XMLUtils.h"
#include "utilities.h"

#include "client.h"
#include <string>
#include <stdio.h>
#include <cstdarg>
#ifdef TARGET_WINDOWS
    #include <windows.h>
#else
    #include <stdarg.h>
#endif

using namespace ADDON;

// format related string functions taken from:
// http://www.flipcode.com/archives/Safe_sprintf.shtml

bool Str2Bool(const CStdString str)
{
	return str.compare("True") == 0 ? true:false;
}

std::vector<CStdString> split(const CStdString& s, const CStdString& delim, const bool keep_empty) {
    std::vector<CStdString> result;
    if (delim.empty()) {
        result.push_back(s);
        return result;
    }
    CStdString::const_iterator substart = s.begin(), subend;
    while (true) {
        subend = search(substart, s.end(), delim.begin(), delim.end());
        CStdString temp(substart, subend);
        if (keep_empty || !temp.empty()) {
            result.push_back(temp);
        }
        if (subend == s.end()) {
            break;
        }
        substart = subend + delim.size();
    }
    return result;
}

bool EndsWith(CStdString const &fullString, CStdString const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool StartsWith(CStdString const &fullString, CStdString const &starting)
{
    if (fullString.length() >= starting.length()) {
        return (0 == fullString.compare(0, starting.length(), starting));
    } else {
        return false;
    }
}

bool ReadFileContents(CStdString const &strFileName, CStdString &strContent)
{
	void* fileHandle = XBMC->OpenFile(strFileName.c_str(), 0);
	if (fileHandle)
	{
		char buffer[1024];
		while (XBMC->ReadFileString(fileHandle, buffer, 1024))
			strContent.append(buffer);
		XBMC->CloseFile(fileHandle);
		return true;
	}
	return false;
}

bool WriteFileContents(CStdString const &strFileName, CStdString &strContent)
{
	void* fileHandle = XBMC->OpenFileForWrite(strFileName.c_str(), true);
	if (fileHandle)
	{
		int rc = XBMC->WriteFile(fileHandle, strContent.c_str(), strContent.length());
		if (rc)
		{
			XBMC->Log(LOG_DEBUG, "wrote file %s", strFileName.c_str());
		}
		else
		{
			XBMC->Log(LOG_ERROR, "can not write to %s", strFileName.c_str());
		}
		XBMC->CloseFile(fileHandle);
		return rc >= 0;
	}
	return false;
}