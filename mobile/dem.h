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

#ifndef __dem_h__
#define __dem_h__

#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ ("")
#endif /* !__PRETTY_FUNCTION__ */

struct ARecord {
	char	q_name[145];
	int	dl_code;
	int	p_code;
	int	pr_code;
	int	z_code;
	float	p_parm[15];
	int	g_units;	/* ground planimetric coordinates */
	int	e_units;	/* elevation coordinates */
	int	sides;
	float	corners[4][2];

	float	min_elevation;	/* minimum and maximum elevations */
	float	max_elevation;

	float	angle;
	int	a_code;		/* Accuracy code */

	float	x_res;		/* Spatial Resolution */
	float	y_res;
	float	z_res;

	int	rows;		/* number of rows and columns of profiles */
	int	cols;
};

struct BRecord {
	int	row_id;
	int	col_id;
	int	rows;
	int	cols;
	float	x_gpc;
	float	y_gpc;
	float	elevation;
	float	min_elevation;
	float	max_elevation;
};


class DEMFile {
public:
	DEMFile(char *s) : fname(s), demfile(0) { }
	~DEMFile() { if(demfile) fclose(demfile); }
	int*	process(void);
	void	dump_ARecord(void);
	void	dump_BRecord(void);
	void	dump_grid(void);
	void	range(double &x, double &y);
	void	resolution(double &r);

private:
	int	open(void);
	int	read_int(void);
	float	read_float(void);
	void	read_field(void);

	char		*fname;
	FILE		*demfile;
	struct ARecord	a;
	struct BRecord	b;
	int		*grid;
	char		tempbuf[1024];
};

#endif /* __dem_h__ */

