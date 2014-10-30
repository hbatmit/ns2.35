// This may look like C code, but it is really -*- C++ -*-
/* 
Copyright (C) 1988 Free Software Foundation
    written by Doug Lea (dl@rocky.oswego.edu)

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

Linking this file statically or dynamically with other modules is making
a combined work based on this file.  Thus, the terms and conditions of
the GNU General Public License cover the whole combination.

In addition, as a special exception, the copyright holders of this file
give you permission to combine this file with free software programs or
libraries that are released under the GNU LGPL and with code included in
the standard release of ns-2 under the Apache 2.0 license or under
otherwise-compatible licenses with advertising requirements (or modified
versions of such code, with unchanged license).  You may copy and
distribute such a system following the terms of the GNU GPL for this
file and the licenses of the other code concerned, provided that you
include the source code of that other code when and as the GNU GPL
requires distribution of source code.

Note that people who make modified versions of this file are not
obligated to grant this special exception for their modified versions;
it is their choice whether to do so.  The GNU General Public License
gives permission to release a modified version without this exception;
this exception also makes it possible to release a modified version
which carries forward this exception.
*/


#ifndef _intVec_h
#ifdef __GNUG__
#pragma interface
#endif
#define _intVec_h 1

#include "builtin.h"
#include "int.defs.h"

#ifndef _int_typedefs
#define _int_typedefs 1
typedef void (*intProcedure)(int );
typedef int  (*intMapper)(int );
typedef int  (*intCombiner)(int , int );
typedef int  (*intPredicate)(int );
typedef int  (*intComparator)(int , int );
#endif


class intVec 
{
protected:      
  int                   len;
  int                   *s;                  

                        intVec(int l, int* d);
public:
                        intVec ();
                        intVec (int l);
                        intVec (int l, int  fill_value);
                        intVec (const intVec&);
                        ~intVec ();

  intVec &              operator = (const intVec & a);
  intVec                at(int from = 0, int n = -1);

  int                   capacity() const;
  void                  resize(int newlen);                        

  int&                  operator [] (int n);
  int&                  elem(int n);

  friend intVec         concat(intVec & a, intVec & b);
  friend intVec         map(intMapper f, intVec & a);
  friend intVec         merge(intVec & a, intVec & b, intComparator f);
  friend intVec         combine(intCombiner f, intVec & a, intVec & b);
  friend intVec         reverse(intVec & a);

  void                  reverse();
  void                  sort(intComparator f);
  void                  fill(int  val, int from = 0, int n = -1);

  void                  apply(intProcedure f);
  int                   reduce(intCombiner f, int  base);
  int                   index(int  targ);

  friend int            operator == (intVec& a, intVec& b);
  friend int            operator != (intVec& a, intVec& b);

  void                  error(const char* msg);
  void                  range_error();
};

extern void default_intVec_error_handler(const char*);
extern one_arg_error_handler_t intVec_error_handler;

extern one_arg_error_handler_t 
        set_intVec_error_handler(one_arg_error_handler_t f);


inline intVec::intVec()
{
  len = 0; s = 0;
}

inline intVec::intVec(int l)
{
  s = new int [len = l];
}


inline intVec::intVec(int l, int* d) :len(l), s(d) {}


inline intVec::~intVec()
{
  delete [] s;
}


inline int& intVec::operator [] (int n)
{
  if ((unsigned)n >= (unsigned)len)
    range_error();
  return s[n];
}

inline int& intVec::elem(int n)
{
  return s[n];
}


inline int intVec::capacity() const
{
  return len;
}



inline int operator != (intVec& a, intVec& b)
{
  return !(a == b);
}

#endif
