#pragma once

#include <vector>
#include <string>


std::vector<CStdString> split(const CStdString& s, const CStdString& delim, const bool keep_empty = true);

bool Str2Bool(const CStdString str);

bool EndsWith(CStdString const &fullString, CStdString const &ending);
bool StartsWith(CStdString const &fullString, CStdString const &starting);

