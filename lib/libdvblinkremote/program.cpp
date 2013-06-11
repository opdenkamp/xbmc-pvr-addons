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

#include "response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

Program::Program()
  : ItemMetadata(),
    m_id("")
{
  
}

Program::Program(const std::string& id, const std::string& title, const long startTime, const long duration)
  : ItemMetadata(title, startTime, duration),
    m_id(id)
{
  
}

Program::Program(Program& program) 
  : ItemMetadata((ItemMetadata&)program),
    m_id(program.GetID())
{
  
}

Program::~Program()
{

}

std::string& Program::GetID() 
{ 
  return m_id; 
}

void Program::SetID(const std::string& id) 
{ 
  m_id = id; 
}

void ProgramSerializer::Deserialize(XmlObjectSerializer<Response>& objectSerializer, const tinyxml2::XMLElement& element, dvblinkremote::Program& program)
{
  ItemMetadataSerializer::Deserialize(objectSerializer, element, (ItemMetadata&)program);
  program.SetID(Util::GetXmlFirstChildElementText(&element, "program_id"));
}
