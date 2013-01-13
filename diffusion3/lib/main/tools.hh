// 
// tools.hh      : Other utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: tools.hh,v 1.13 2005/09/13 04:53:50 tomh Exp $
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
//

//
// This file contains OS abstractions to make it easy to use diffusion
// in different environments (i.e. in simulations, where time is
// virtualized, and in embeddedd apps where error logging happens in
// some non-trivial way).
//
// This file defines the various debug levels and a global debug level
// variable (global_debug_level) that can be set according to how much
// debug information one would like to get when using DiffPrint (see
// below for further details).

#ifndef _TOOLS_HH_
#define _TOOLS_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

// Defines the various debug levels
#define DEBUG_NEVER            11
#define DEBUG_LOTS_DETAILS     10
#define DEBUG_MORE_DETAILS      8
#define DEBUG_DETAILS           6
#define DEBUG_SOME_DETAILS      4
#define DEBUG_NO_DETAILS        3
#define DEBUG_IMPORTANT         2
#define DEBUG_ALWAYS            1

// Defines the default debug level
#ifdef NS_DIFFUSION
#define DEBUG_DEFAULT           0
#else
#define DEBUG_DEFAULT           1
#endif // NS_DIFFUSION

extern int global_debug_level;

// SetSeed sets the random number generator's seed with the timeval
// structure given in tv (which is not changed)
void SetSeed(struct timeval *tv);

// GetTime returns a timeval structure with the current system time
void GetTime(struct timeval *tv);

// GetRand returns a random number between 0 and RAND_MAX
int GetRand();

// DiffPrint can be used to print messages. In addition to take the
// message to be printed (using the variable argument list format,
// just like fprintf), DiffPrint also requires a debug level for this
// particular message. This is is compared to the global debug level
// and if it is below the current global debug level, the message is
// sent to the logging device (usually set to stderr).
void DiffPrint(int msg_debug_level, const char *fmt, ...);

// Same as DiffPrint but will print the current system time before
// printing the message
void DiffPrintWithTime(int msg_debug_level, const char *fmt, ...);
#endif // !_TOOLS_HH_
