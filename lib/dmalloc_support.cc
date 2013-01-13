
/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: dmalloc_support.cc,v 1.8 2005/08/25 18:58:06 johnh Exp $
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


/*
 * Redefine new and friends to use dmalloc.
 */

#ifdef HAVE_LIBDMALLOC

/*
 * XXX dmalloc 3.x is no longer supported
 *
 * - haoboy, Aug 2000
 */

/* 
 * This portion copied from ~dmalloc/dmalloc.cc 
 * Copyright 1999 by Gray Watson
 */
extern "C" {
#include <stdio.h>
#include <stdlib.h>

/* Prototype declaration for TclpAlloc originally
defined in tcl8.3.2/generic/tclAlloc.c */ 
char *TclpAlloc(unsigned int);
char *Tcl_Alloc(unsigned int);


#define DMALLOC_DISABLE

#include "dmalloc.h"
#include "return.h"
}

#ifndef DMALLOC_VERSION_MAJOR
#error DMALLOC 3.x is no longer supported.
#endif

/*
 * An overload function for the C++ new.
 */
void *
operator new[](size_t size)
{
	char	*file;
	GET_RET_ADDR(file);
	return _malloc_leap(file, 0, size);
}

/*
 * An overload function for the C++ delete.
 */
void
operator delete(void *pnt)
{
	char	*file;
	GET_RET_ADDR(file);
	_free_leap(file, 0, pnt);
}

/*
 * An overload function for the C++ delete[].  Thanks to Jens Krinke
 * <j.krinke@gmx.de>
 */
void
operator delete[](void *pnt)
{
	char	*file;
	GET_RET_ADDR(file);
	_free_leap(file, 0, pnt);
}

char *
TclpAlloc(unsigned int nbytes)
{
  char *file;
 
  GET_RET_ADDR(file);
  return (char*) _malloc_leap(file,0,nbytes);
}

char *
Tcl_Alloc (unsigned int size)
  /*    unsigned int size; */
  {
      char *result;
      char *file;

      /* 
       * Replacing the call to TclpAlloc with malloc directly to help 
       * memory debugging 
       *    result = TclpAlloc(size);
       */

      GET_RET_ADDR(file);
      result = (char *)_malloc_leap(file,0,size);
     /*
      * Most systems will not alloc(0), instead bumping it to one so
      * that NULL isn't returned.  Some systems (AIX, Tru64) will alloc(0)
      * by returning NULL, so we have to check that the NULL we get is
      * not in response to alloc(0).
      *
      * The ANSI spec actually says that systems either return NULL *or*
      * a special pointer on failure, but we only check for NULL
      */

      if ((result == NULL) && size) {
	printf("unable to alloc %d bytes", size);
      }
      return result;
}


#endif /* HAVE_LIBDMALLOC */
