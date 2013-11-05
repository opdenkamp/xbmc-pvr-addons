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
#include <sstream>
#include <iostream>
#include <cstdio>
#include "tinyxml2/tinyxml2.h"

namespace dvblinkremote {
  class Util {
  public:
    static bool ConvertToInt(const std::string& s, int& value);
    static bool ConvertToLong(const std::string& s, long& value);
    static bool ConvertToString(const int& value, std::string&);
    static bool ConvertToString(const unsigned int& value, std::string&);
    static bool ConvertToString(const long& value, std::string&);
    static bool ConvertToString(const bool& value, std::string&);
    static tinyxml2::XMLElement* CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, const char* value);
    static tinyxml2::XMLElement* CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, const std::string& value);
    static tinyxml2::XMLElement* CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, int value);
    static tinyxml2::XMLElement* CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, unsigned int value);
    static tinyxml2::XMLElement* CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, long value);
    static tinyxml2::XMLElement* CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, bool value);
    static const char* GetXmlFirstChildElementText(const tinyxml2::XMLElement* parentElement, const char* name);
    static int GetXmlFirstChildElementTextAsInt(const tinyxml2::XMLElement* parentElement, const char* name);
    static long GetXmlFirstChildElementTextAsLong(const tinyxml2::XMLElement* parentElement, const char* name);
    static bool GetXmlFirstChildElementTextAsBoolean(const tinyxml2::XMLElement* parentElement, const char* name);

  private:
    template <class T> static bool from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&));
    template <class T> static bool to_string(const T& t, std::string& s);
  };
}