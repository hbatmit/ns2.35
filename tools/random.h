/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1995 Regents of the University of California.
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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/tools/random.h,v 1.16 1999/10/09 01:06:39 haoboy Exp $ (LBL)
 */

#ifndef ns_random_h
#define ns_random_h

#include <math.h>
#include "config.h"
#include "rng.h"

class Random {
private:
	static RNG *rng() { return RNG::defaultrng(); }

public:
	static void seed(int s) { rng()->set_seed(RNG::RAW_SEED_SOURCE, s); }
	static int seed_heuristically() { rng()->set_seed(RNG::HEURISTIC_SEED_SOURCE); return rng()->seed(); };

	static int random() { return rng()->uniform_positive_int(); }
	static double uniform() { return rng()->uniform_double();}
	static double uniform(double r) { return rng()->uniform(r); }
	static double uniform(double a, double b) { return rng()->uniform(a,b); }
	static double exponential() { return rng()->exponential(); }
	static int integer(int k) { return rng()->uniform(k); }
	static double exponential(double r) { return rng()->exponential(r); }
	static double pareto(double scale, double shape) { return rng()->pareto(scale, shape); }
        static double paretoII(double scale, double shape) { return rng()->paretoII(scale, shape);}
	static double normal(double avg, double std) { return rng()->normal(avg, std); }
	static double lognormal(double avg, double std) { return rng()->lognormal(avg, std); }
};

#endif
