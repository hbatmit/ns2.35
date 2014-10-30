/* autoconf.h.  Generated automatically by configure.  */
/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * autoconf-win32.h
 * Copyright (C) 1997 by the University of Southern California
 * $Id: autoconf-win32.h,v 1.2 2005/08/25 18:58:00 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/* This file should contain variables changed only by autoconf. */

/* XXX Change the following two variables to where your perl and tcl are located! */
#define NSPERL_PATH "C:\\Program\\Perl\\bin\\perl.exe"
#define NSTCLSH_PATH "C:\\Program\\Tcl\\bin\\tclsh80.exe"

/* If you need these from tcl, see the file tcl/lib/ns-autoconf.tcl.in */

/*
 * Put autoconf #define's here to keep them off the command line.
 * see autoconf.info(Header Templates) in the autoconf docs.
 */

/* what does random(3) return? */
#define RANDOM_RETURN_TYPE int

/* type definitions */
typedef char int8_t;		/* cygwin-b20\H-i586-cygwin32\i586-cygwin32\include\sys\types.h(83) */
typedef unsigned char u_int8_t; /* cygwin-b20\H-i586-cygwin32\i586-cygwin32\include\sys\types.h(84) */
typedef short int16_t;		/* cygnus\cygwin-b20\H-i586-cygwin32\i586-cygwin32\include\sys\types.h(85) */
typedef unsigned short u_int16_t; /* from cygwin-b20\H-i586-cygwin32\i586-cygwin32\include\sys\types.h(86) */
typedef int int32_t;		/* cygwin-b20\H-i586-cygwin32\i586-cygwin32\include\sys\types.h(87) */
typedef unsigned int u_int32_t; /* cygwin-b20\H-i586-cygwin32\i586-cygwin32\include\sys\types.h(88) */

typedef __int64 int64_t;	/* C:\Program\Microsoft Visual Studio\VC98\CRT\SRC\STRTOQ.C(19) */
typedef unsigned __int64 u_int64_t; /* C:\Program\Microsoft Visual Studio\VC98\CRT\SRC\STRTOQ.C(20) */
#undef HAVE_INT64
#define SIZEOF_LONG 4

/* functions */
#undef HAVE_BCOPY
#undef HAVE_BZERO
#undef HAVE_GETRUSAGE
#undef HAVE_SBRK
#undef HAVE_SNPRINTF
#undef HAVE_STRTOLL
#undef HAVE_STRTOQ

/* headers */
#define HAVE_STRING_H 1
#undef HAVE_STRINGS_H
#undef HAVE_ARPA_INET_H
#undef HAVE_NETINET_IN_H

