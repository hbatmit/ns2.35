// 
// tools.cc      : Implements a few utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: tools.cc,v 1.18 2010/03/08 05:54:50 tom_henderson Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.

#include <math.h>

#include "tools.hh"

#ifdef NS_DIFFUSION
#include "scheduler.h"
#include "rng.h"
#include "random.h"
#endif // NS_DIFFUSION

int global_debug_level = DEBUG_DEFAULT;

void GetTime(struct timeval *tv)
{
#ifdef NS_DIFFUSION
  double time;
  long sec, usec;

  time = Scheduler::instance().clock();
  // sec = lrint (time);
  sec = (long) rint (time);
  // usec = lrint ((time - sec) * 1000000);
  usec = (long) rint ((time - sec) * 1000000);
  tv->tv_sec = sec;
  tv->tv_usec = usec;
  DiffPrint(DEBUG_NEVER, "tv->sec = %ld, tv->usec = %ld\n", tv->tv_sec, tv->tv_usec);
#else
  gettimeofday(tv, NULL);
#endif // NS_DIFFUSION
}

#ifdef NS_DIFFUSION
void SetSeed(struct timeval *) 
{
  // Don't need to do anything since NS's RNG is seeded using
  // otcl proc ns-random <seed>
#else
void SetSeed(struct timeval *tv) 
{
  srand(tv->tv_usec);
#endif // NS_DIFFUSION
}

int GetRand()
{
#ifdef NS_DIFFUSION
  return (Random::random());
#else
  return (rand());
#endif // NS_DIFFUSION
}

void DiffPrint(int msg_debug_level, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  if (global_debug_level >= msg_debug_level){
    // Print message
    vfprintf(stderr, fmt, ap);
    fflush(NULL);
  }

  va_end(ap);
}

void DiffPrintWithTime(int msg_debug_level, const char *fmt, ...)
{
  struct timeval tv;
  va_list ap;

  va_start(ap, fmt);

  if (global_debug_level >= msg_debug_level){
    // Get time
    GetTime(&tv);

    // Print Time
    fprintf(stderr, "%ld.%06ld : ", tv.tv_sec, (long int) tv.tv_usec);
    // Print message
    vfprintf(stderr, fmt, ap);
    fflush(NULL);
  }

  va_end(ap);
}
