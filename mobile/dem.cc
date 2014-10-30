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


#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dem.h>

int
DEMFile::open()
{
	if((demfile = fopen(fname, "r")) == 0)
		return EINVAL;
	else
		return 0;
}


int
DEMFile::read_int()
{
	read_field();
	return (atoi(tempbuf));
}

float
DEMFile::read_float()
{
	read_field();
	return (atof(tempbuf));
}

void
DEMFile::read_field()
{
	int i;
	char ch;

	bzero(tempbuf, sizeof(tempbuf));

	do {
		ch = fgetc(demfile);
	} while (ch != EOF && isspace (ch));
	tempbuf[0] = ch;

	for(i = 1; ; i++) {
		if(feof(demfile)) {
			fprintf(stderr, "%s: EOF\n", __PRETTY_FUNCTION__);
			exit(1);
		}
		tempbuf[i] = fgetc(demfile);
		if(tempbuf[i] == 'D')
			tempbuf[i] = 'E';
		if(isspace(tempbuf[i]))
			break;
	}
}


void
DEMFile::resolution(double &r)
{
	r = 5.0;
}


void
DEMFile::range(double &x, double &y)
{
#if 0
	int i;
	double minx = 0.0, miny = 0.0, maxx = 0.0, maxy = 0.0;

	for(i = 0; i < 4; i++) {
		if(minx == 0.0 || minx > a.corners[i][0]) {
			minx = a.corners[i][0];
			miny = a.corners[i][1];
		}
		else if(minx == a.corners[i][0] && miny > a.corners[i][1]) {
			miny = a.corners[i][1];
		}

		if(maxx == 0.0 || maxx < a.corners[i][0]) {
			maxx = a.corners[i][0];
			maxy = a.corners[i][1];
		}
		else if(maxx == a.corners[i][0] && maxy < a.corners[i][1]) {
			maxy = a.corners[i][1];
		}
	}
	x = maxx - minx;
	y = maxy - miny;
#else
	x = y = (double) a.cols - 1;
#endif
}


/* ======================================================================
   Returns a pointer to the "grid".
   ====================================================================== */
int*
DEMFile::process()
{
	int i, j, offset = 0;
	char ch;

	if(fname == 0)
		return 0;

	if(open())
		return 0;

	/* ============================================================
	   A Record
	   ============================================================ */
	bzero(a.q_name, sizeof(a.q_name));
	for(i = 0; ! feof(demfile) && i < (int) sizeof(a.q_name) - 1; i++) {
		ch = fgetc(demfile);
		if(! isspace(ch)) {
			a.q_name[offset] = ch;
			offset++;
		}
		else {
			if(offset && ! isspace(a.q_name[offset-1])) {
				a.q_name[offset] = ch;
				offset++;
			}
		}
	}

	a.dl_code = read_int();
	a.p_code = read_int();
	a.pr_code = read_int();
	a.z_code = read_int();

	/* Map Projection Parameters */
	for(i = 0; i < 15; i++)
		a.p_parm[i] = read_float();	

	a.g_units = read_int();
        a.e_units = read_int();
        a.sides = read_int();

	for(i = 0; i < 4; i++) {
		a.corners[i][0] = read_float();
		a.corners[i][1] = read_float();
	}

	a.min_elevation = read_float();
	a.max_elevation = read_float();
	a.angle = read_float();
#if 0
	a.a_code = read_int();
	a.x_res = read_float();
	a.y_res = read_float();
	a.z_res = read_float();
#else
	read_field();
#endif
	a.rows = read_int();
	a.cols = read_int();

	grid = (int*) malloc(sizeof(int) * a.cols * a.cols);

	/* ============================================================
	   B Records
	   ============================================================ */
	for(int rows = 0; rows < a.cols; rows++) {
		b.row_id = read_int();
		b.col_id = read_int();
		b.rows = read_int();
		b.cols = read_int();
		b.x_gpc = read_float();
		b.y_gpc = read_float();
		b.elevation = read_float();
		b.min_elevation = read_float();
		b.max_elevation = read_float();

		i = rows * a.cols;
		for(j = 0; j < b.rows; j++)
			grid[i+j] = read_int();
	}

	return grid;
}


void
DEMFile::dump_ARecord()
{
	int i;

	fprintf(stdout, "*** A RECORD ***\n");
	fprintf(stdout, "Quadrangle name:                   %s\n", a.q_name);
	fprintf(stdout, "DEM Level code:                    %d\n", a.dl_code);
	fprintf(stdout, "Pattern code:                      %d\n", a.p_code);
	fprintf(stdout, "Planimetric reference system code: %d\n", a.pr_code);
	fprintf(stdout, "Zone code:                         %d\n", a.z_code);

	fprintf(stdout, "Map Projection Parameters:\n");
	for(i = 0; i < 15; i++)
		fprintf(stdout, "                                   %f\n",
			a.p_parm[i]);

	fprintf(stdout, "Ground planimetric coordinates:    %d\n", a.g_units);
	fprintf(stdout, "Elevation coordinates:             %d\n", a.e_units);
	fprintf(stdout, "Sides:                             %d\n", a.sides);

	fprintf(stdout, "Corners:\n");
	for(i = 0; i < 4; i++)
		fprintf(stdout, "                                   %f, %f\n",
			a.corners[i][0], a.corners[i][1]);

	fprintf(stdout, "Min Elevation:                     %f\n", a.min_elevation);
	fprintf(stdout, "Max Elevation:                     %f\n", a.max_elevation);
	fprintf(stdout, "Clockwise angle:                   %f\n", a.angle);
	fprintf(stdout, "Accuracy code:                     %d\n", a.a_code);

	fprintf(stdout, "Spatial Resolution:\n");
	fprintf(stdout, "                                   %f (x)\n", a.x_res);
	fprintf(stdout, "                                   %f (y)\n", a.y_res);
	fprintf(stdout, "                                   %f (z)\n", a.z_res);

	fprintf(stdout, "Rows:                              %d\n", a.rows);
	fprintf(stdout, "Columns:                           %d\n", a.cols);
}


void
DEMFile::dump_BRecord()
{
	fprintf(stdout, "*** B RECORD ***\n");
	fprintf(stdout, "Row ID:                            %d\n", b.row_id);
	fprintf(stdout, "Column ID:                         %d\n", b.col_id);
	fprintf(stdout, "Rows:                              %d\n", b.rows);
	fprintf(stdout, "Columns:                           %d\n", b.cols);
	fprintf(stdout, "Ground planimetric coordinates:    %f, %f\n",
		b.x_gpc, b.y_gpc);
	fprintf(stdout, "Elevation:                         %f\n", b.elevation);
	fprintf(stdout, "Min Elevation:                     %f\n", b.min_elevation);
	fprintf(stdout, "Max Elevation:                     %f\n", b.max_elevation);
}


void
DEMFile::dump_grid()
{
	int i, j;
	fprintf(stdout, "*** X,Y GRID ***\n");
	for(i = 0; i < a.cols; i++) {
		fprintf(stdout, "ROW %5d --- ", i);
		for(j = 0; j < a.cols; j++) {
			fprintf(stdout, "%5d ", grid[i*a.cols + j]);
		}
		fprintf(stdout, "\n");
	}
}


#if 0

void main(int argc, char** argv)
{
	DEMFile *F;

	if(argc != 2)
		exit(1);

	F = new DEMFile(argv[1]);
	F->process();
	F->dump_grid();
}

#endif

