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

#ifndef MYTHDTO_ARTWORK_H
#define	MYTHDTO_ARTWORK_H

#include "../../mythtypes.h"

namespace MythDTOArtwork
{
  void SetArtwork_URL(Myth::Artwork *obj, const char *val) { obj->url = val; }
  void SetArtwork_FileName(Myth::Artwork *obj, const char *val) { obj->fileName = val; }
  void SetArtwork_StorageGroup(Myth::Artwork *obj, const char *val) { obj->storageGroup = val; }
  void SetArtwork_Type(Myth::Artwork *obj, const char *val) { obj->type = val; }
}

#endif	/* MYTHDTO_ARTWORK_H */
