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
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

#include <math.h>
#include <stdlib.h>

#include "object.h"

#include "dem.h"
#include "topography.h"


static class TopographyClass : public TclClass {
public:
        TopographyClass() : TclClass("Topography") {}
        TclObject* create(int, const char*const*) {
                return (new Topography);
        }
} class_topography;


double
Topography::height(double x, double y) {

	if(grid == 0)
		return 0.0;

#ifndef WIN32
	int a = (int) rint(x/grid_resolution);
	int b = (int) rint(y/grid_resolution);
	int c = (int) rint(maxY);
#else 
	int a = (int) ceil(x/grid_resolution+0.5);
	int b = (int) ceil(y/grid_resolution+0.5);
	int c = (int) ceil(maxY+0.5);
#endif /* !WIN32 */

	return (double) grid[a * c + b];
}


int
Topography::load_flatgrid(int x, int y, int res)
{
	/* No Reason to malloc a grid */

	grid_resolution = res;	// default is 1 meter
	maxX = (double) x;
	maxY = (double) y;

	return 0;
}


int
Topography::load_demfile(const char *fname)
{
	DEMFile *dem;

	fprintf(stderr, "Opening DEM file %s...\n", fname);

	dem = new DEMFile((char*) fname); 
	if(dem == 0)
		return 1;

	fprintf(stderr, "Processing DEM file...\n");

	grid = dem->process();
	if(grid == 0) {
		delete dem;
		return 1;
	}
	dem->dump_ARecord();

	double tx, ty; tx = maxX; ty = maxY;
	dem->range(tx, ty);			// Get the grid size
	maxX = tx; maxY = ty;

	dem->resolution(grid_resolution);	// Get the resolution of each entry

	/* Close the DEM file */
	delete dem;

	/*
	 * Sanity Check
	 */
	if(maxX <= 0.0 || maxY <= 0.0 || grid_resolution <= 0.0)
		return 1;

	fprintf(stderr, "DEM File processing complete...\n");

	return 0;
}


void 
Topography::updateNodesList(class MobileNode* mn, double oldX)
{
	channel_->updateNodesList(mn, oldX);
}


int
Topography::command(int argc, const char*const* argv)
{
	if(argc == 3) {

		if(strcmp(argv[1], "load_demfile") == 0) {
			if(load_demfile(argv[2]))
				return TCL_ERROR;
			return TCL_OK;
		} else if (strcmp(argv[1], "channel") == 0) {
			WirelessChannel *chan;
			chan = (WirelessChannel *)TclObject::lookup(argv[2]);
			if (chan == 0) {
				fprintf(stderr, "%s lookup of channel failed\n", argv[1]);
				return TCL_ERROR;
			} else {
				channel_ = chan;
				return TCL_OK;
			}
		}
	}
	else if(argc == 4) {
		if(strcmp(argv[1], "load_flatgrid") == 0) {
			if(load_flatgrid(atoi(argv[2]), atoi(argv[3])))
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	else if(argc == 5) {
		if(strcmp(argv[1], "load_flatgrid") == 0) {
			if(load_flatgrid(atoi(argv[2]), atoi(argv[3]), atoi(argv[4])))
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return TclObject::command(argc, argv);
}
