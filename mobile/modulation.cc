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
#include <random.h>
//#include <debug.h>
#include <modulation.h>

/* ======================================================================
   Binary Phase Shift Keying
   ====================================================================== */
BPSK::BPSK()
{
	Rs = 0;
}

BPSK::BPSK(int S)
{
	Rs = S;
}

int
BPSK::BitError(double Pr)
{
	double Pe;			// probability of error
	double x;
	int nbit = 0;			// number of bit errors tolerated

	if(nbit == 0) {
		Pe = ProbBitError(Pr);
	}
	else {
		Pe = ProbBitError(Pr, nbit);
	}

	// quick check
	if(Pe == 0.0)
		return 0;		// no bit errors

	// scale the error probabilty
	Pe *= 1e3;

	x = (double)(((int)Random::uniform()) % 1000);

	if(x < Pe)
		return 1;		// bit error
	else
		return 0;		// no bit errors
}

double
BPSK::ProbBitError(double)
{
	double Pe = 0.0;

	return Pe;
}

double
BPSK::ProbBitError(double, int)
{
	double Pe = 0.0;

	return Pe;
}


