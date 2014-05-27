/*
 *  Copyright (C) 2006-2009, Simon Hyde
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 */

/** \file safe_string.h
 * Some basic string handling routines to help avoid segfaults/buffer overflows
 */

#ifndef __CMYTH_STRING_H
#define __CMYTH_STRING_H

#include <stdio.h>

static inline char * safe_strncpy(char *dest,const char *src,size_t n)
{
    if(src == NULL)
    {
	dest[0] = '\0';
    }
    else
    {
	dest[n-1] = '\0';
	strncpy(dest,src,n-1);
    }
    return dest;
}

#define sizeof_strncpy(dest,src) (safe_strncpy(dest,src,sizeof(dest)))

#define safe_atol(str) (((str) == NULL)? (int32_t)0: atol(str))
#define safe_atoll(str) (((str) == NULL)? (int64_t)0: atoll(str))
#define safe_atoi(str) (((str) == NULL)? (int)0: atoi(str))

#endif /* __CMYTH_STRING_H */
