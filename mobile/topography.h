/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Ported from CMU/Monarch's code, nov'98 -Padma.
   topography.h

*/

#ifndef ns_topography_h
#define ns_topography_h

#include <object.h>
#include "channel.h"

class Topography : public TclObject {

public:
	Topography() { maxX = maxY = grid_resolution = 0.0; grid = 0; }

	/* List-keeper */
	void updateNodesList(class MobileNode *mn, double oldX);
	
	double	lowerX() { return 0.0; }
	double	upperX() { return maxX * grid_resolution; }
	double	lowerY() { return 0.0; }
	double	upperY() { return maxY * grid_resolution; }
	double	resol() { return grid_resolution; }
	double	height(double x, double y);

private:
	virtual int command(int argc, const char*const* argv);
	int	load_flatgrid(int x, int y, int res = 1);
	int	load_demfile(const char *fname);

	double	maxX;
	double	maxY;

	double	grid_resolution;
	int*	grid;

	/* List-keeper */
	WirelessChannel *channel_;

};

#endif // ns_topography_h
