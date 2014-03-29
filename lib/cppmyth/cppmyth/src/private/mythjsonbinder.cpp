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

#include "mythjsonbinder.h"
#include "builtin.h"
#include "../mythdebug.h"

#include <cstdlib>  // for atof
#include <cstring>  // for strcmp
#include <cstdio>
#include <errno.h>

void MythJSON::BindObject(const json_t *json, void *obj, const bindings_t *bl)
{
  const char *value;
  int i, err;

  if (bl == NULL)
    return;

  for (i = 0; i < bl->attr_count; ++i)
  {
    // jansson
    json_t *field = json_object_get(json, bl->attr_bind[i].field);
    if (field == NULL)
      continue;
    value = json_string_value(field);
    if (value != NULL)
    {
      err = 0;
      switch (bl->attr_bind[i].type)
      {
        case IS_STRING:
          bl->attr_bind[i].set(obj, value);
          break;
        case IS_INT8:
        {
          int8_t num = 0;
          err = str2int8(value, &num);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_INT16:
        {
          int16_t num = 0;
          err = str2int16(value, &num);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_INT32:
        {
          int32_t num = 0;
          err = str2int32(value, &num);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_INT64:
        {
          int64_t num = 0;
          err = str2int64(value, &num);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_UINT8:
        {
          uint8_t num = 0;
          err = str2uint8(value, &num);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_UINT16:
        {
          uint16_t num = 0;
          err = str2uint16(value, &num);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_UINT32:
        {
          uint32_t num = 0;
          err = str2uint32(value, &num);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_DOUBLE:
        {
          double num = atof(value);
          bl->attr_bind[i].set(obj, &num);
          break;
        }
        case IS_BOOLEAN:
        {
          bool b = (strcmp(value, "true") == 0 ? true : false);
          bl->attr_bind[i].set(obj, &b);
          break;
        }
        case IS_TIME:
        {
          time_t time = 0;
          err = str2time(value, &time);
          bl->attr_bind[i].set(obj, &time);
          break;
        }
        default:
          break;
      }
      if (err)
        Myth::DBG(MYTH_DBG_ERROR, "%s: failed (%d) field \"%s\" type %d: %s\n", __FUNCTION__, err, bl->attr_bind[i].field, bl->attr_bind[i].type, value);
    }
    else
      Myth::DBG(MYTH_DBG_WARN, "%s: no value for field \"%s\" type %d\n", __FUNCTION__, bl->attr_bind[i].field, bl->attr_bind[i].type);
  }
}
