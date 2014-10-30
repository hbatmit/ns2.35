// This may look like C code, but it is really -*- C++ -*-

/* 
Copyright (C) 1988, 1992 Free Software Foundation
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

/*
  arithmetic, etc. functions on built in types
*/


#ifndef _builtin_h
#ifdef __GNUG__
#pragma interface
#endif
#define _builtin_h 1

#include <stddef.h>
// #include <std.h>
// #include <cmath>
#include <stdio.h>

#ifndef __GNUC__
#define __attribute__(x)
#endif

typedef void (*one_arg_error_handler_t)(const char*);
typedef void (*two_arg_error_handler_t)(const char*, const char*);

long         gcd(long, long);
long         lg(unsigned long); 
double       pow(double, long);
long         pow(long, long);

extern "C" double       start_timer();
extern "C" double       return_elapsed_time(double last_time = 0.0);

char*        dtoa(double x, char cvt = 'g', int width = 0, int prec = 6);

unsigned int hashpjw(const char*);
unsigned int multiplicativehash(int);
unsigned int foldhash(double);

extern void default_one_arg_error_handler(const char*) __attribute__ ((noreturn));
extern void default_two_arg_error_handler(const char*, const char*) __attribute__ ((noreturn));

extern two_arg_error_handler_t lib_error_handler;

extern two_arg_error_handler_t 
       set_lib_error_handler(two_arg_error_handler_t f);


#if !defined(IV)

inline short abs(short arg) 
{
  return (arg < 0)? -arg : arg;
}

inline int sign(long arg)
{
  return (arg == 0) ? 0 : ( (arg > 0) ? 1 : -1 );
}

inline int sign(double arg)
{
  return (arg == 0.0) ? 0 : ( (arg > 0.0) ? 1 : -1 );
}

inline long sqr(long arg)
{
  return arg * arg;
}

#if ! _G_MATH_H_INLINES /* hpux and SCO define this in math.h */
inline double sqr(double arg)
{
  return arg * arg;
}
#endif

inline int even(long arg)
{
  return !(arg & 1);
}

inline int odd(long arg)
{
  return (arg & 1);
}

inline long lcm(long x, long y)
{
  return x / gcd(x, y) * y;
}

inline void (setbit)(long& x, long b)
{
  x |= (1 << b);
}

inline void clearbit(long& x, long b)
{
  x &= ~(1 << b);
}

inline int testbit(long x, long b)
{
  return ((x & (1 << b)) != 0);
}

#endif
#endif
