#pragma once
/*
*      Copyright (C) 2010 Marcel Groothuis
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
*  MA 02110-1301  USA
*  http://www.gnu.org/copyleft/gpl.html
*
*/
#include "client.h"
#include <string>
#include <json/json.h>

#define E_SUCCESS 0
#define E_FAILED -1
#define E_EMPTYRESPONSE -2

class cRest
{
public:
	cRest(void) {};
	~cRest(void) {};

	int Get(const std::string& command, const std::string& arguments, Json::Value& json_response);
	int Post(const std::string& command, const std::string& arguments, Json::Value& json_response);
};



int httpRequest(const std::string& command, const std::string& arguments, const bool write, std::string& json_response);


