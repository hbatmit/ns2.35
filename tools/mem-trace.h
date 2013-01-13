/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1998 by the University of Southern California
 * $Id: mem-trace.h,v 1.16 2005/08/25 18:58:12 johnh Exp $
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

#include "config.h"
#include <stdio.h>

/* Unix platforms should get these from configure */
#ifdef WIN32
#undef HAVE_GETRUSAGE
#undef HAVE_SBRK
#endif

#ifdef HAVE_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#ifdef HAVE_SBRK
#include <unistd.h>
#endif /* HAVE_SBRK */

/* hpux patch from Ketil Danielsen <Ketil.Danielsen@hiMolde.no> */
#ifdef hpux
#include <sys/resource.h>
#include <sys/syscall.h>
#define getrusage(a, b)  syscall(SYS_GETRUSAGE, a, b)
#endif



#define fDIFF(FIELD) (now_.FIELD - start_.FIELD)
#define absolute(var) (var > 0 ? var : -var)
#define normalize(var) var.tv_sec * 1000 + (int) (var.tv_usec / 1000)

class MemInfo {
public:
	MemInfo() {}
	long stack_;
	long heap_;
	struct timeval utime_, stime_;

	void checkpoint() {
		int i;
		stack_ = (long)&i;

#ifdef HAVE_GETRUSAGE
		struct rusage ru;
		getrusage(RUSAGE_SELF, &ru);
		utime_ = ru.ru_utime;
		stime_ = ru.ru_stime;
#else /* ! HAVE_GETRUSAGE */
		utime_.tv_sec = utime_.tv_usec = 0;
		stime_.tv_sec = stime_.tv_usec = 0;
#endif /* HAVE_GETRUSAGE */

#ifdef HAVE_SBRK
		heap_ = (long)sbrk(0);
#else /* ! HAVE_SBRK */
		heap_ = 0;
#endif /* HAVE_SBRK */
	}
};


class MemTrace {
private:
	MemInfo start_, now_;

public:
	MemTrace() {
		start_.checkpoint();
	}
	void diff(char* prompt) {
		now_.checkpoint();
		fprintf (stdout, "%s: utime/stime: %ld %ld \tstack/heap: %ld %ld\n",
			 prompt, 
			 normalize(now_.utime_) - normalize(start_.utime_), 
			 normalize(now_.stime_) - normalize(start_.stime_), 
			 fDIFF(stack_), fDIFF(heap_));
		start_.checkpoint();
	}
};

