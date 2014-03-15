#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 */
#include <string>
#include <vector>
#include <ctime>
#include "uri.h"
#include "platform/util/util.h"

#ifdef TARGET_WINDOWS
#include "os-dependent.h"
#include "windows/WindowsUtils.h"
#endif

/**
 * String tokenize
 * Split string using the given delimiter into a vector of substrings
 */
void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters);

std::wstring StringToWString(const std::string& s);
std::string WStringToString(const std::wstring& s);
std::string lowercase(const std::string& s);
bool stringtobool(const std::string& s);
const char* booltostring(const bool b);

/**
 * @brief Filters forbidden filename characters from channel name and replaces them with _ )
 */
std::string ToThumbFileName(const char* strChannelName);

std::string ToXBMCPath(const std::string& strFileName);
std::string ToWindowsPath(const std::string& strFileName);

/**
 * @brief Macro to silence unused parameter warnings
 */
#ifdef UNUSED
#  undef UNUSED
#endif
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x)  /* x */
#endif

#if defined(TARGET_WINDOWS)
namespace UTF8Util
{
  std::wstring ConvertUTF8ToUTF16(const char* pszTextUTF8);
  std::string  ConvertUTF16ToUTF8(const WCHAR* pszTextUTF16);
}
#endif

/* Addon-specific debug tracing */
extern bool             g_bDebug;
#define TRACE(...) if (g_bDebug) \
  XBMC->Log(LOG_NOTICE, __VA_ARGS__ ); \
else \
  XBMC->Log(LOG_DEBUG, __VA_ARGS__ );
