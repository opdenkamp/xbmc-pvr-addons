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

#ifndef JANSSONPTR_H
#define	JANSSONPTR_H

#include "../atomic.h"

#include <jansson.h>
#include <cstddef>  // for NULL

class JanssonPtr
{
public:

  JanssonPtr() : p(NULL), c(NULL) { }

  explicit JanssonPtr(json_t *s) : p(s), c(NULL)
  {
    if (p != NULL)
      c = new atomic_t(1);
  }

  JanssonPtr(const JanssonPtr& s) : p(s.p), c(s.c)
  {
    if (c != NULL)
      atomic_increment(c);
  }

  JanssonPtr& operator=(const JanssonPtr& s)
  {
    if (this != &s)
    {
      reset();
      p = s.p;
      c = s.c;
      if (c != NULL)
        atomic_increment(c);
    }
    return *this;
  }

  ~JanssonPtr()
  {
    reset();
  }

  bool isValid()
  {
    return p != NULL;
  }

  void reset()
  {
    if (c != NULL)
    {
      if (*c == 1)
        json_decref(p);
      if (!atomic_decrement(c))
        delete c;
    }
    c = NULL;
    p = NULL;
  }

  void reset(json_t *s)
  {
    if (p != s)
    {
      reset();
      if (s != NULL)
      {
        p = s;
        c = new atomic_t(1);
      }
    }
  }

  json_t *get() const
  {
    return (c != NULL) ? p : NULL;
  }

  unsigned use_count() const
  {
    return (unsigned) (c != NULL ? *c : 0);
  }

  json_t *operator->() const
  {
    return get();
  }

  json_t& operator*() const
  {
    return *get();
  }

  operator bool() const
  {
    return p != NULL;
  }

  bool operator!() const
  {
    return p == NULL;
  }

protected:
  json_t *p;
  atomic_t *c;
};

#endif	/* JANSSONPTR_H */
