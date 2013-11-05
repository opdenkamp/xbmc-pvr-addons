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

#include "util.h"

using namespace dvblinkremote;

template <class T> 
bool Util::from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
{
  std::istringstream iss(s);
  return !(iss >> f >> t).fail();
}

template bool Util::from_string<int>(int& t, const std::string& s, std::ios_base& (*f)(std::ios_base&));
template bool Util::from_string<long>(long& t, const std::string& s, std::ios_base& (*f)(std::ios_base&));

template <class T>
bool Util::to_string(const T& t, std::string& s)
{
  std::ostringstream result;
  result << t;

  if (!result.fail()) {
    s.assign(result.str());
    return true;
  }

  return false;
}

bool Util::ConvertToInt(const std::string& s, int& value) 
{
  return from_string<int>(value, s, std::dec);
}

bool Util::ConvertToLong(const std::string& s, long& value) 
{
  return from_string<long>(value, s, std::dec);
}

bool Util::ConvertToString(const int& value, std::string& s) 
{
  return to_string(value, s);
}

bool Util::ConvertToString(const unsigned int& value, std::string& s) 
{
  return to_string(value, s);
}

bool Util::ConvertToString(const long& value, std::string& s) 
{
  return to_string(value, s);
}

bool Util::ConvertToString(const bool& value, std::string& s) 
{
  if (value) {
    s = "true";
  }
  else {
    s = "false";
  }

  return true;
}

tinyxml2::XMLElement* Util::CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, const char* value)
{
  tinyxml2::XMLElement* el = xmlDocument->NewElement(elementName);
  el->InsertFirstChild(xmlDocument->NewText(value));
  return el;
}

tinyxml2::XMLElement* Util::CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, const std::string& value)
{
  tinyxml2::XMLElement* el = xmlDocument->NewElement(elementName);
  el->InsertFirstChild(xmlDocument->NewText(value.c_str()));
  return el;
}

tinyxml2::XMLElement* Util::CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, int value) 
{
  std::string s;

  if (Util::ConvertToString(value, s)) {
    return CreateXmlElementWithText(xmlDocument, elementName, s.c_str());
  }

  return NULL;
}

tinyxml2::XMLElement* Util::CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, unsigned int value) 
{
  std::string s;

  if (Util::ConvertToString(value, s)) {
    return CreateXmlElementWithText(xmlDocument, elementName, s.c_str());
  }

  return NULL;
}

tinyxml2::XMLElement* Util::CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, long value) 
{
  std::string s;

  if (Util::ConvertToString(value, s)) {
    return CreateXmlElementWithText(xmlDocument, elementName, s.c_str());
  }

  return NULL;
}

tinyxml2::XMLElement* Util::CreateXmlElementWithText(tinyxml2::XMLDocument* xmlDocument, const char* elementName, bool value) 
{
  std::string s;

  if (Util::ConvertToString(value, s)) {
    return CreateXmlElementWithText(xmlDocument, elementName, s.c_str());
  }

  return NULL;
}

const char* Util::GetXmlFirstChildElementText(const tinyxml2::XMLElement* parentElement, const char* name)
{
  const tinyxml2::XMLElement* el = parentElement->FirstChildElement(name);
  const char* s = "";

  if (el != NULL && el->GetText()) {
    s = el->GetText();
  }

  return s;
}

int Util::GetXmlFirstChildElementTextAsInt(const tinyxml2::XMLElement* parentElement, const char* name)
{
  const tinyxml2::XMLElement* el = parentElement->FirstChildElement(name);
  const char* s = "-1";
  int value;

  if (el != NULL && el->GetText()) {
    s = el->GetText();
  }

  if (s && !Util::ConvertToInt(s, value))
  {
    value = -1;
  }

  return value;
}

long Util::GetXmlFirstChildElementTextAsLong(const tinyxml2::XMLElement* parentElement, const char* name) 
{
  const tinyxml2::XMLElement* el = parentElement->FirstChildElement(name);
  const char* s = "-1";
  long value;

  if (el != NULL && el->GetText()) {
    s = el->GetText();
  }

  if (s && !Util::ConvertToLong(s, value))
  {
    value = -1;
  }

  return value;
}

bool Util::GetXmlFirstChildElementTextAsBoolean(const tinyxml2::XMLElement* parentElement, const char* name) 
{
  const tinyxml2::XMLElement* el = parentElement->FirstChildElement(name);
  const char* s = "false";
  bool value = false;

  if (el != NULL && el->GetText()) {
    s = el->GetText();
  }

  if (s && strcmp(s, "true") != 0)
  {
    value = true;
  }

  return value;
}
