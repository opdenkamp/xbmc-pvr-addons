/*
*      Copyright (C) 2005-2010 Team XBMC
*      http://www.xbmc.org
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "rest.h"
#include <vector>
#include <stdlib.h>
#include <string.h>

using namespace ADDON;

int cRest::Get(const std::string& command, const std::string& arguments, Json::Value& json_response)
{
	std::string response;
	int retval;
	retval = httpRequest(command, arguments, false, response);

	if (retval != E_FAILED)
	{
		if (response.length() != 0)
		{
			Json::Reader reader;

			bool parsingSuccessful = reader.parse(response, json_response);

			if (!parsingSuccessful)
			{
				XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n",
					response.c_str(),
					reader.getFormatedErrorMessages().c_str());
				return E_FAILED;
			}
		}
		else
		{
			XBMC->Log(LOG_DEBUG, "Empty response");
			return E_EMPTYRESPONSE;
		}
	}

	return retval;
}

int cRest::Post(const std::string& command, const std::string& arguments, Json::Value& json_response)
{
	std::string response;
	int retval;
	retval = httpRequest(command, arguments, true, response);

	if (retval != E_FAILED)
	{
		if (response.length() != 0)
		{
			Json::Reader reader;

			bool parsingSuccessful = reader.parse(response, json_response);

			if (!parsingSuccessful)
			{
				XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n",
					response.c_str(),
					reader.getFormatedErrorMessages().c_str());
				return E_FAILED;
			}
		}
		else
		{
			XBMC->Log(LOG_DEBUG, "Empty response");
			return E_EMPTYRESPONSE;
		}
	}

	return retval;
}

int httpRequest(const std::string& command, const std::string& arguments, const bool write, std::string& json_response)
{
	//PLATFORM::CLockObject critsec(communication_mutex);		
	std::string strUrl = command;
	
	if (write) {	// POST http request
		void* hFile = XBMC->OpenFileForWrite(strUrl.c_str(), 0);
		if (hFile != NULL)
		{
			int rc = XBMC->WriteFile(hFile, arguments.c_str(), arguments.length());
			if (rc >= 0)
			{
				std::string result;
				result.clear();
				char buffer[1024];
				while (XBMC->ReadFileString(hFile, buffer, 1024))
					result.append(buffer);
				json_response = result;
				return 0;
			}
			XBMC->CloseFile(hFile);
		}
	}
	else {	// GET http request	
		strUrl += arguments;
		void* fileHandle = XBMC->OpenFile(strUrl.c_str(), 0);
		if (fileHandle)
		{
			std::string result;
			char buffer[1024];
			while (XBMC->ReadFileString(fileHandle, buffer, 1024))
				result.append(buffer);

			XBMC->CloseFile(fileHandle);
			json_response = result;
			return 0;
		}
	}

	return -1;
}

