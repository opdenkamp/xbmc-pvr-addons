/*
 *      Copyright (C) 2014 Jean-Luc Barriere
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
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "mythjsonparser.h"
#include "../mythdebug.h"

using namespace Myth;

JanssonPtr MythJSON::ParseResponseJSON(Myth::WSResponse& resp)
{
  // Read content response
  JanssonPtr root;
  size_t r, content_length = resp.GetContentLength();
  char *content = new char[content_length + 1];
  if ((r = resp.ReadContent(content, content_length)) == content_length)
  {
    json_error_t error;
    content[content_length] = '\0';
    DBG(MYTH_DBG_PROTO,"%s: %s\n", __FUNCTION__, content);
    // Parse JSON content
    root.reset(json_loads(content, 0, &error));
    if (!root.isValid())
      DBG(MYTH_DBG_ERROR, "%s: failed to parse: %d: %s\n", __FUNCTION__, error.line, error.text);
  }
  else
  {
    DBG(MYTH_DBG_ERROR, "%s: read error\n", __FUNCTION__);
  }
  delete[] content;
  return root;
}
