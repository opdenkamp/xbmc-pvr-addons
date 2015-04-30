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

#include "request.h"
#include "response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

GetFavoritesRequest::GetFavoritesRequest()
{ }

GetFavoritesRequest::~GetFavoritesRequest()
{ }

ChannelFavorite::ChannelFavorite(std::string& id, std::string& name, favorite_channel_list_t& channels) :
id_(id), name_(name), channels_(channels)
{
}

ChannelFavorite::~ChannelFavorite()
{ }

ChannelFavorites::ChannelFavorites()
{
}

ChannelFavorites::ChannelFavorites(ChannelFavorites& favorites)
{
    favorites_ = favorites.favorites_;
}

ChannelFavoritesSerializer::GetFavoritesResponseXmlDataDeserializer::GetFavoritesResponseXmlDataDeserializer(ChannelFavoritesSerializer& parent, ChannelFavorites& favoritesList)
    : m_parent(parent),
    m_favoritesList(favoritesList)
{

}

ChannelFavorites::~ChannelFavorites()
{

}

ChannelFavoritesSerializer::GetFavoritesResponseXmlDataDeserializer::~GetFavoritesResponseXmlDataDeserializer()
{

}

bool ChannelFavoritesSerializer::GetFavoritesResponseXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
    if (strcmp(element.Name(), "favorite") == 0)
    {
        std::string id = Util::GetXmlFirstChildElementText(&element, "id");
        std::string name = Util::GetXmlFirstChildElementText(&element, "name");

        //channels
        ChannelFavorite::favorite_channel_list_t channels;
        const tinyxml2::XMLElement* channels_element = element.FirstChildElement("channels");
        if (channels_element != NULL)
        {
            const tinyxml2::XMLElement* channel_element = channels_element->FirstChildElement();
            while (channel_element != NULL)
            {
                if (strcmp(channel_element->Name(), "channel") == 0)
                {
                    if (channel_element->GetText() != NULL)
                        channels.push_back(channel_element->GetText());
                }
                channel_element = channel_element->NextSiblingElement();
            }

        }

        ChannelFavorite cf(id, name, channels);

        m_favoritesList.favorites_.push_back(cf);

        return false;
    }

    return true;
}

bool GetFavoritesRequestSerializer::WriteObject(std::string& serializedData, GetFavoritesRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("favorites"); 

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool ChannelFavoritesSerializer::ReadObject(ChannelFavorites& object, const std::string& xml)
{
    tinyxml2::XMLDocument& doc = GetXmlDocument();

    if (doc.Parse(xml.c_str()) == tinyxml2::XML_NO_ERROR) {
        tinyxml2::XMLElement* elRoot = doc.FirstChildElement("favorites");
        GetFavoritesResponseXmlDataDeserializer* xmlDataDeserializer = new GetFavoritesResponseXmlDataDeserializer(*this, object);
        elRoot->Accept(xmlDataDeserializer);
        delete xmlDataDeserializer;

        return true;
    }

  return false;
}
