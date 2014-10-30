/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 *  File: RngStream.cc for multiple streams of Random Numbers 
 *  Copyright (C) 2001  Pierre L'Ecuyer (lecuyer@iro.umontreal.ca)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 *
 *  Linking this random number generator statically or dynamically with
 *  other modules is making a combined work based on this random number
 *  generator.  Thus, the terms and conditions of the GNU General Public
 *  License cover the whole combination.
 *
 *  In addition, as a special exception, the copyright holders of this
 *  random number generator give you permission to combine this random
 *  number generator program with free software programs or libraries
 *  that are released under the GNU LGPL and with code included in the
 *  standard release of ns-2 under the Apache 2.0 license or under
 *  otherwise-compatible licenses with advertising requirements (or
 *  modified versions of such code, with unchanged license).  You may
 *  copy and distribute such a system following the terms of the GNU GPL
 *  for this random number generator and the licenses of the other code
 *  concerned, provided  that you include the source code of that other
 *  code when and as the GNU GPL requires distribution of source code.
 *
 *  Note that people who make modified versions of this random number
 *  generator are not obligated to grant this special exception for
 *  their modified versions; it is their choice whether to do so.
 *  The GNU General Public License gives permission to release a
 *  modified  version without this exception; this exception also makes
 *  it possible to release a modified version which carries forward
 *  this exception.
 *
 *  //Incorporated into rng.cc and modified to maintain backward
 *  //compatibility with ns-2.1b8.  Users can use their current scripts
 *  //unmodified with the new RNG.  To get the same results as with the
 *  //previous RNG, define OLD_RNG when compiling (e.g.,
 *  //make -D OLD_RNG).
 *  // - Michele Weigle, University of North Carolina
 *  // (mcweigle@cs.unc.edu)  October 10, 2001
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/tools/rng.cc,v 1.31 2006/02/21 15:20:20 mahrenho Exp $ (LBL)";
#endif

/* new random number generator */

#ifndef WIN32
#include <sys/time.h>			// for gettimeofday
#include <unistd.h>
#endif

#include <stdio.h>
#ifndef OLD_RNG
#include <string.h>
#endif /* !OLD_RNG */
#include "rng.h"

#ifdef OLD_RNG
/*
 * RNGImplementation
 */

/*
 * Generate a periodic sequence of pseudo-random numbers with
 * a period of 2^31 - 2.  The generator is the "minimal standard"
 * multiplicative linear congruential generator of Park, S.K. and
 * Miller, K.W., "Random Number Generators: Good Ones are Hard to Find,"
 * CACM 31:10, Oct. 88, pp. 1192-1201.
 *
 * The algorithm implemented is:  Sn = (a*s) mod m.
 *   The modulus m can be approximately factored as: m = a*q + r,
 *   where q = m div a and r = m mod a.
 *
 * Then Sn = g(s) + m*d(s)
 *   where g(s) = a(s mod q) - r(s div q)
 *   and d(s) = (s div q) - ((a*s) div m)
 *
 * Observations:
 *   - d(s) is either 0 or 1.
 *   - both terms of g(s) are in 0, 1, 2, . . ., m - 1.
 *   - |g(s)| <= m - 1.
 *   - if g(s) > 0, d(s) = 0, else d(s) = 1.
 *   - s mod q = s - k*q, where k = s div q.
 *
 * Thus Sn = a(s - k*q) - r*k,
 *   if (Sn <= 0), then Sn += m.
 *
 * To test an implementation for A = 16807, M = 2^31-1, you should
 *   get the following sequences for the given starting seeds:
 *
 *   s0, s1,    s2,        s3,          . . . , s10000,     . . . , s551246 
 *    1, 16807, 282475249, 1622650073,  . . . , 1043618065, . . . , 1003 
 *    1973272912, 1207871363, 531082850, 967423018
 *
 * It is important to check for s10000 and s551246 with s0=1, to guard 
 * against overflow.
*/

/*
 * The sparc assembly code [no longer here] is based on Carta, D.G., "Two Fast
 * Implementations of the 'Minimal Standard' Random Number
 * Generator," CACM 33:1, Jan. 90, pp. 87-88.
 *
 * ASSUME that "the product of two [signed 32-bit] integers (a, sn)
 *        will occupy two [32-bit] registers (p, q)."
 * Thus: a*s = (2^31)p + q
 *
 * From the observation that: x = y mod z is but
 *   x = z * the fraction part of (y/z),
 * Let: sn = m * Frac(as/m)
 *
 * For m = 2^31 - 1,
 *   sn = (2^31 - 1) * Frac[as/(2^31 -1)]
 *      = (2^31 - 1) * Frac[as(2^-31 + 2^-2(31) + 2^-3(31) + . . .)]
 *      = (2^31 - 1) * Frac{[(2^31)p + q] [2^-31 + 2^-2(31) + 2^-3(31) + . . 
.]}
 *      = (2^31 - 1) * Frac[p+(p+q)2^-31+(p+q)2^-2(31)+(p+q)3^(-31)+ . . .]
 *
 * if p+q < 2^31:
 *   sn = (2^31 - 1) * Frac[p + a fraction + a fraction + a fraction + . . .]
 *      = (2^31 - 1) * [(p+q)2^-31 + (p+q)2^-2(31) + (p+q)3^(-31) + . . .]
 *      = p + q
 *
 * otherwise:
 *   sn = (2^31 - 1) * Frac[p + 1.frac . . .]
 *      = (2^31 - 1) * (-1 + 1.frac . . .)
 *      = (2^31 - 1) * [-1 + (p+q)2^-31 + (p+q)2^-2(31) + (p+q)3^(-31) + . . .]
 *      = p + q - 2^31 + 1
 */
 

#define A	16807L		/* multiplier, 7**5 */
#define M	2147483647L	/* modulus, 2**31 - 1; both used in random */
#define INVERSE_M ((double)4.656612875e-10)	/* (1.0/(double)M) */

long
RNGImplementation::next()
{
	long L, H;
	L = A * (seed_ & 0xffff);
	H = A * (seed_ >> 16);
	
	seed_ = ((H & 0x7fff) << 16) + L;
	seed_ -= 0x7fffffff;
	seed_ += H >> 15;
	
	if (seed_ <= 0) {
		seed_ += 0x7fffffff;
	}
	return(seed_);
}

double
RNGImplementation::next_double()
{
	long i = next();
	return i * INVERSE_M;
}
#endif /* OLD_RNG */

/*
 * RNG implements a nice front-end around RNGImplementation
 */
#ifndef stand_alone
static class RNGClass : public TclClass {
public:
	RNGClass() : TclClass("RNG") {}
	TclObject* create(int, const char*const*) {
		return(new RNG());
	}
} class_rng;
#endif /* stand_alone */

/* default RNG */

RNG* RNG::default_ = NULL;

double
RNG::normal(double avg, double std)
{
	static int parity = 0;
	static double nextresult;
	double sam1, sam2, rad;
   
	if (std == 0) return avg;
	if (parity == 0) {
		sam1 = 2*uniform() - 1;
		sam2 = 2*uniform() - 1;
		while ((rad = sam1*sam1 + sam2*sam2) >= 1) {
			sam1 = 2*uniform() - 1;
			sam2 = 2*uniform() - 1;
		}
		rad = sqrt((-2*log(rad))/rad);
		nextresult = sam2 * rad;
		parity = 1;
		return (sam1 * rad * std + avg);
	}
	else {
		parity = 0;
		return (nextresult * std + avg);
	}
}

#ifndef stand_alone
int
RNG::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 3) {
		if (strcmp(argv[1], "testint") == 0) {
			int n = atoi(argv[2]);
			tcl.resultf("%d", uniform(n));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "testdouble") == 0) {
			double d = atof(argv[2]);
			tcl.resultf("%6e", uniform(d));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "seed") == 0) {
			int s = atoi(argv[2]);
			// NEEDSWORK: should be a way to set seed to PRDEF_SEED_SOURCE
			if (s) {
				if (s <= 0 || (unsigned int)s >= MAXINT) {
					tcl.resultf("Setting random number seed to known bad value.");
					return TCL_ERROR;
				};
				set_seed(RAW_SEED_SOURCE, s);
			} else set_seed(HEURISTIC_SEED_SOURCE, 0);
			return(TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "next-random") == 0) {
			tcl.resultf("%u", uniform_positive_int());
			return(TCL_OK);
		}
		if (strcmp(argv[1], "seed") == 0) {
#ifdef OLD_RNG
			tcl.resultf("%u", stream_.seed());
#else
			tcl.resultf("%u", seed());
#endif /* OLD_RNG */
			return(TCL_OK);
		}
#ifndef OLD_RNG
		if (strcmp (argv[1], "next-substream") == 0) {
			reset_next_substream();
			return (TCL_OK);
		}
		if (strcmp (argv[1], "all-seeds") == 0) {
			unsigned long seeds[6];
			get_state (seeds);
			tcl.resultf ("%lu %lu %lu %lu %lu %lu",
				     seeds[0], seeds[1], seeds[2], 
				     seeds[3], seeds[4], seeds[5]);
			return (TCL_OK);
		}
		if (strcmp (argv[1], "reset-start-substream") == 0) {
			reset_start_substream();
			return (TCL_OK);
		}
#endif /* !OLD_RNG */
		if (strcmp(argv[1], "default") == 0) {
			default_ = this;
			return(TCL_OK);
		}
		//#if 0
		if (strcmp(argv[1], "test") == 0) {
			RNGTest test; test.verbose_mil();
			//if (test())
			//	tcl.resultf("RNG test failed");
			//else
			//	tcl.resultf("RNG test passed");
			return(TCL_OK);
		}
		//#endif
	} else if (argc == 4) {
		if (strcmp(argv[1], "seed") == 0) {
			int s = atoi(argv[3]);
			if (strcmp(argv[2], "raw") == 0) {
				set_seed(RNG::RAW_SEED_SOURCE, s);
			} else if (strcmp(argv[2], "predef") == 0) {
				set_seed(RNG::PREDEF_SEED_SOURCE, s);
				// s is the index in predefined seed array
				// 0 <= s < 64
			} else if (strcmp(argv[2], "heuristic") == 0) {
				set_seed(RNG::HEURISTIC_SEED_SOURCE, 0);
			}
			return(TCL_OK);
		}
		if (strcmp(argv[1], "normal") == 0) {
			double avg = strtod(argv[2], NULL);
			double std = strtod(argv[3], NULL);
			tcl.resultf("%g", normal(avg, std));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "lognormal") == 0) {
			double avg = strtod(argv[2], NULL);
			double std = strtod(argv[3], NULL);
			tcl.resultf("%g", lognormal(avg, std));
			return (TCL_OK);
		}
	}
	return(TclObject::command(argc, argv));
}
#endif /* stand_alone */

void
RNG::set_seed(RNGSources source, int seed)
{
	/* The following predefined seeds are evenly spaced around
	 * the 2^31 cycle.  Each is approximately 33,000,000 elements
	 * apart.
	 */
#define N_SEEDS_ 64
	static long predef_seeds[N_SEEDS_] = {
		1973272912L,  188312339L, 1072664641L,  694388766L,
		2009044369L,  934100682L, 1972392646L, 1936856304L,
		1598189534L, 1822174485L, 1871883252L,  558746720L,
		605846893L, 1384311643L, 2081634991L, 1644999263L,
		773370613L,  358485174L, 1996632795L, 1000004583L,
		1769370802L, 1895218768L,  186872697L, 1859168769L,
		349544396L, 1996610406L,  222735214L, 1334983095L,
		144443207L,  720236707L,  762772169L,  437720306L,
		939612284L,  425414105L, 1998078925L,  981631283L,
		1024155645L,  822780843L,  701857417L,  960703545L,
		2101442385L, 2125204119L, 2041095833L,   89865291L,
		898723423L, 1859531344L,  764283187L, 1349341884L,
		678622600L,  778794064L, 1319566104L, 1277478588L,
		538474442L,  683102175L,  999157082L,  985046914L,
		722594620L, 1695858027L, 1700738670L, 1995749838L,
		1147024708L,  346983590L,  565528207L,  513791680L
	};
	static long heuristic_sequence = 0;

	switch (source) {
	case RAW_SEED_SOURCE:
		if (seed <= 0 || (unsigned int)seed >= MAXINT)    // Wei Ye
			abort();
		// use it as it is
		break;
	case PREDEF_SEED_SOURCE:
		if (seed < 0 || seed >= N_SEEDS_)
			abort();
		seed = predef_seeds[seed];
		break;
	case HEURISTIC_SEED_SOURCE:
		timeval tv;
		gettimeofday(&tv, 0);
		heuristic_sequence++;   // Always make sure we're different than last time.
		seed = (tv.tv_sec ^ tv.tv_usec ^ (heuristic_sequence << 8)) & 0x7fffffff;
		break;
	};
	// set it
	// NEEDSWORK: should we throw out known bad seeds?
	// (are there any?)
	if (seed < 0)
		seed = -seed;
#ifdef OLD_RNG
	stream_.set_seed(seed);
#else
	set_seed(seed);
#endif /* OLD_RNG */

	// Toss away the first few values of heuristic seed.
	// In practice this makes sequential heuristic seeds
	// generate different first values.
	//
	// How many values to throw away should be the subject
	// of careful analysis.  Until then, I just throw away
	// ``a bunch''.  --johnh
	if (source == HEURISTIC_SEED_SOURCE) {
		int i;
		for (i = 0; i < 128; i++) {
#ifdef OLD_RNG			
			stream_.next();
#else
			next();
#endif /* OLD_RNG */
		};
	};
}


/*
 * RNGTest:
 * Make sure the RNG makes known values.
 * Optionally, print out some stuff.
 *
 * gcc rng.cc -Drng_stand_alone -o rng_test -lm
 *
 * Simple test program:
 */
#ifdef rng_stand_alone
int main() { RNGTest test; test.verbose(); }
#endif /* rng_stand_alone */

#ifdef rng_test
RNGTest::RNGTest()
{
	  RNG rng(RNG::RAW_SEED_SOURCE, 1L);

	  int i; 
	  long r;
	  for (i = 0; i < 10000; i++) 
	  r = rng.uniform_positive_int();

#ifdef OLD_RNG
	  if (r != 1043618065L) {
		  fprintf (stderr, "r (%lu) != 1043618065L\n", r);
		  exit(-1);
	  }
#else
	  if (r != 1179983147L) {
		  fprintf (stderr, "r (%lu) != 1179983147L\n", r);
		  exit(-1);
	  }
#endif /* OLD_RNG */

	  for (i = 10000; i < 551246; i++)
		  r = rng.uniform_positive_int();

#ifdef OLD_RNG
	  if (r != 1003L) {
		  fprintf (stderr, "r (%lu) != 1003L\n", r);
		  exit(-1);
	  }
#else
	  if (r != 817829295L) {
		  fprintf (stderr, "r (%lu) != 817829295L\n", r);
		  exit(-1);
	  }
#endif /* OLD_RNG */

}

void
RNGTest::first_n(RNG::RNGSources source, long seed, int n)
{
	RNG rng(source, seed);

	for (int i = 0; i < n; i++) {
		long r = rng.uniform_positive_int();
		printf("%10lu ", r);
	};
	printf("\n");
}

void
RNGTest::first_n_mil(RNG::RNGSources source, long seed, int n, FILE *outfile)
{
	RNG rng(source, seed);
	// print the first 1000 no, then every millionth upto n millions
	long m = n * 1000000;
	for (int i = 0; i < m; i++) {
		long r = rng.uniform_positive_int();
		if (i < 100 || (i % 1000000 == 0))
			fprintf(outfile, "%10lu ", r);
	};
	fprintf(outfile, "\n");
}

void 
RNGTest::verbose_mil()
{
	FILE *outfile = NULL;
	outfile = fopen("temp.rands", "w");
	
	if (outfile == NULL)
		fprintf(stderr, "Cannot create temp.rands\n");

	fprintf (outfile, "default: ");
	first_n_mil(RNG::RAW_SEED_SOURCE, 188312339, 5, outfile);
	int i = 1;
	fprintf (outfile, "predef source %2u: ", i);
	first_n_mil(RNG::PREDEF_SEED_SOURCE, i, 5, outfile);
	
}

void
RNGTest::verbose()
{
	printf ("default: ");
	first_n(RNG::RAW_SEED_SOURCE, 1L, 5);

	int i;
	for (i = 0; i < 64; i++) {
		printf ("predef source %2u: ", i);
		first_n(RNG::PREDEF_SEED_SOURCE, i, 5);
	};

	printf("heuristic seeds should be different from each other and on each run.\n");
	for (i = 0; i < 64; i++) {
		printf ("heuristic source %2u: ", i);
		first_n(RNG::HEURISTIC_SEED_SOURCE, i, 5);
	};
}

#endif /* rng_test */

#ifndef OLD_RNG
using namespace std; 
namespace 
{ 
	const double m1 = 4294967087.0; 
	const double m2 = 4294944443.0; 
	const double norm = 1.0 / (m1 + 1.0); 
	const double a12 = 1403580.0; 
	const double a13n = 810728.0; 
	const double a21 = 527612.0; 
	const double a23n = 1370589.0; 
	const double two17 = 131072.0; 
	const double two53 = 9007199254740992.0; 
	const double fact = 5.9604644775390625e-8; /* 1 / 2^24 */ 

	// The following are the transition matrices of the two MRG 
	// components (in matrix form), raised to the powers -1, 1, 
	// 2^76, and 2^127, resp. 

	const double InvA1[3][3] = { // Inverse of A1p0 
		{ 184888585.0, 0.0, 1945170933.0 }, 
		{ 1.0, 0.0, 0.0 }, 
		{ 0.0, 1.0, 0.0 } 
	}; 

	const double InvA2[3][3] = { // Inverse of A2p0 
		{ 0.0, 360363334.0, 4225571728.0 }, 
		{ 1.0, 0.0, 0.0 }, 
		{ 0.0, 1.0, 0.0 } 
	}; 

	const double A1p0[3][3] = { 
		{ 0.0, 1.0, 0.0 }, 
		{ 0.0, 0.0, 1.0 }, 
		{ -810728.0, 1403580.0, 0.0 } 
	}; 

	const double A2p0[3][3] = { 
		{ 0.0, 1.0, 0.0 }, 
		{ 0.0, 0.0, 1.0 }, 
		{ -1370589.0, 0.0, 527612.0 } 
	}; 

	const double A1p76[3][3] = { 
		{ 82758667.0, 1871391091.0, 4127413238.0 }, 
		{ 3672831523.0, 69195019.0, 1871391091.0 }, 
		{ 3672091415.0, 3528743235.0, 69195019.0 } 
	}; 

	const double A2p76[3][3] = { 
		{ 1511326704.0, 3759209742.0, 1610795712.0 }, 
		{ 4292754251.0, 1511326704.0, 3889917532.0 }, 
		{ 3859662829.0, 4292754251.0, 3708466080.0 } 
	}; 

	const double A1p127[3][3] = { 
		{ 2427906178.0, 3580155704.0, 949770784.0 }, 
		{ 226153695.0, 1230515664.0, 3580155704.0 }, 
		{ 1988835001.0, 986791581.0, 1230515664.0 } 
	}; 

	const double A2p127[3][3] = { 
		{ 1464411153.0, 277697599.0, 1610723613.0 }, 
		{ 32183930.0, 1464411153.0, 1022607788.0 }, 
		{ 2824425944.0, 32183930.0, 2093834863.0 } 
	}; 

	//------------------------------------------------------------------- 
	// Return (a*s + c) MOD m; a, s, c and m must be < 2^35 
	// 

	double MultModM (double a, double s, double c, double m) 
	{ 
		double v; 
		long a1; 
		v=a*s+c; 

		if (v >= two53 || v <= -two53) { 
			a1 = static_cast<long> (a / two17); a -= a1 * two17; 
			v =a1*s; 
			a1 = static_cast<long> (v / m); v -= a1 * m; 
			v = v * two17 + a * s + c; 
		} 
		a1 = static_cast<long> (v / m); 
		/* in case v < 0)*/ 
		if ((v -= a1 * m) < 0.0) return v += m; else return v; 
	} 

	//------------------------------------------------------------------- 
	// Compute the vector v = A*s MOD m. Assume that -m < s[i] < m. 
	// Works also when v = s. 
	// 
	void MatVecModM (const double A[3][3], const double s[3], double v[3], 
			 double m) 
	{ 
		int i; 
		double x[3]; // Necessary if v = s 
		for (i = 0; i < 3; ++i) { 
			x[i] = MultModM (A[i][0], s[0], 0.0, m); 
			x[i] = MultModM (A[i][1], s[1], x[i], m); 
			x[i] = MultModM (A[i][2], s[2], x[i], m); 
		} 
		for (i = 0; i < 3; ++i) 
			v[i] = x[i]; 
	} 

	//------------------------------------------------------------------- 
	// Compute the matrix C = A*B MOD m. Assume that -m < s[i] < m. 
	// Note: works also if A = C or B = C or A = B = C. 
	// 
	void MatMatModM (const double A[3][3], const double B[3][3], 
			 double C[3][3], double m) 
	{ 
		int i, j; 
		double V[3], W[3][3]; 
		for (i = 0; i < 3; ++i) { 
			for (j = 0; j < 3; ++j) 
				V[j] = B[j][i]; 
			MatVecModM (A, V, V, m); 
			for (j = 0; j < 3; ++j) 

				W[j][i] = V[j]; 
		} 
		for (i = 0; i < 3; ++i) 
			for (j = 0; j < 3; ++j) 
				C[i][j] = W[i][j]; 
	} 

	//------------------------------------------------------------------- 
	// Compute the matrix B = (A^(2^e) Mod m); works also if A = B. 
	// 
	void MatTwoPowModM (const double A[3][3], double B[3][3], double m, 
			    long e) 
	{ 
		int i, j; 
		/* initialize: B = A */ 
		if (A != B) { 
			for (i = 0; i < 3; ++i) 
				for (j = 0; j < 3; ++j) 
					B[i][j] = A[i][j]; 
		} 
		/* Compute B = A^(2^e) mod m */ 
		for (i = 0; i < e; i++) 
			MatMatModM (B, B, B, m); 
	} 

	//------------------------------------------------------------------- 
	// Compute the matrix B = (A^n Mod m); works even if A = B. 
	// 
	void MatPowModM (const double A[3][3], double B[3][3], double m, 
			 long n) 
	{ 
		int i, j; 
		double W[3][3]; 
		/* initialize: W = A; B = I */ 
		for (i = 0; i < 3; ++i) 
			for (j = 0; j < 3; ++j) { 
				W[i][j] = A[i][j]; 
				B[i][j] = 0.0; 
			} 
		for (j = 0; j < 3; ++j) 
			B[j][j] = 1.0; 
		/* Compute B = A^n mod m using the binary decomposition of n */
		while (n > 0) { 
			if (n % 2) MatMatModM (W, B, B, m); 
			MatMatModM (W, W, W, m); 

			n/=2; 
		} 
	} 

	//-------------------------------------------------------------------- 
	// Check that the seeds are legitimate values. Returns 0 if legal 
	// seeds, -1 otherwise. 
	// 
	int CheckSeed (const unsigned long seed[6]) 
	{ 
		int i; 
		for (i = 0; i < 3; ++i) { 
			if (seed[i] >= m1) { 
				fprintf (stderr, "****************************************\n\n");
				fprintf (stderr, "ERROR: Seed[%d] >= 4294967087, Seed is not set.", i);
				fprintf (stderr, "\n\n****************************************\n\n");
				return (-1); 
			} 
		} 
		for (i = 3; i < 6; ++i) { 
			if (seed[i] >= m2) { 
				fprintf (stderr, "****************************************\n\n");
				fprintf (stderr, "ERROR: Seed[%d] >= 429444443, Seed is not set.", i);
				fprintf (stderr, "\n\n****************************************\n\n");
				return (-1); 
			} 
		} 
		if (seed[0] == 0 && seed[1] == 0 && seed[2] == 0) { 
			fprintf (stderr, "****************************************\n\n");
			fprintf (stderr, "ERROR: First 3 seeds = 0.\n\n");
			fprintf (stderr, "****************************************\n\n");
			return (-1); 
		} 
		if (seed[3] == 0 && seed[4] == 0 && seed[5] == 0) { 
			fprintf (stderr, "****************************************\n\n");
			fprintf (stderr, "ERROR: Last 3 seeds = 0.\n\n");
			fprintf (stderr, "****************************************\n\n");
			return (-1); 
		} 
		return 0; 
	} 
} // end of anonymous namespace 

//------------------------------------------------------------------------- 
// Generate the next random number. 
// 
double RNG::U01 () 
{ 
	long k; 
	double p1, p2, u; 
	/* Component 1 */ 
	p1 = a12 * Cg_[1] - a13n * Cg_[0]; 
	k = static_cast<long> (p1 / m1); 
	p1 -= k * m1; 
	if (p1 < 0.0) p1 += m1; 
	Cg_[0] = Cg_[1]; Cg_[1] = Cg_[2]; Cg_[2] = p1; 
	/* Component 2 */ 
	p2 = a21 * Cg_[5] - a23n * Cg_[3]; 
	k = static_cast<long> (p2 / m2); 
	p2 -= k * m2; 
	if (p2 < 0.0) p2 += m2; 
	Cg_[3] = Cg_[4]; Cg_[4] = Cg_[5]; Cg_[5] = p2; 
	/* Combination */ 
	u = ((p1 > p2) ? (p1 - p2) * norm : (p1 - p2 + m1) * norm); 
	return (anti_ == false) ? u : (1 - u); 
} 

//------------------------------------------------------------------------- 
// Generate the next random number with extended (53 bits) precision. 
// 
double RNG::U01d () 
{ 
	double u; 
	u = U01(); 
	if (anti_) { 
		// Don't forget that U01() returns 1 - u in 
		// the antithetic case 
		u += (U01() - 1.0) * fact; 
		return (u < 0.0) ? u + 1.0 : u; 
	} else { 
		u += U01() * fact; 
		return (u < 1.0) ? u : (u - 1.0); 
	} 
} 

//************************************************************************* 
// Public members of the class start here 
//------------------------------------------------------------------------- 

/*
 * Functions for backward compatibility:
 *
 *   RNG (long seed)
 *   void set_seed (long seed)
 *   long seed()
 *   long next()
 *   double next_double()
 */
RNG::RNG (long seed) 
{
	set_seed (seed);
	init();
}

void RNG::init()
{
	anti_ = false; 
	inc_prec_ = false; 

	/* Information on a stream. The arrays {Cg_, Bg_, Ig_} contain the
	   current state of the stream, the starting state of the current
	   SubStream, and the starting state of the stream. This stream
	   generates antithetic variates if anti_ = true. It also generates
	   numbers with extended precision (53 bits if machine follows IEEE
	   754 standard) if inc_prec_ = true. next_seed_ will be the seed of
	   the next declared RngStream. */

	for (int i = 0; i < 6; ++i) { 
		Bg_[i] = Cg_[i] = Ig_[i] = next_seed_[i]; 
	} 
	MatVecModM (A1p127, next_seed_, next_seed_, m1); 
	MatVecModM (A2p127, &next_seed_[3], &next_seed_[3], m2); 
}

void RNG::set_seed (long seed) 
{
	if (seed == 0) {
		set_seed (HEURISTIC_SEED_SOURCE, seed);
	}
	else {
		unsigned long seed_vec[6] = {0, 0, 0, 0, 0, 0};
		for (int i=0; i<6; i++) {
			seed_vec[i] = (unsigned long) seed;
		}
		set_package_seed (seed_vec);
		init();
	}
}

long RNG::seed() 
{
	unsigned long seed[6];
	get_state(seed);
	return ((long) seed[0]);
}

long RNG::next()
{
	return (rand_int(0, MAXINT));
}

double RNG::next_double()
{
	return (rand_u01());
}
/* End of backward compatibility functions */

// The default seed of the package; will be the seed of the first 
// declared RNG, unless set_package_seed is called. 
// 
double RNG::next_seed_[6] = 
{ 
	12345.0, 12345.0, 12345.0, 12345.0, 12345.0, 12345.0 
}; 

//------------------------------------------------------------------------- 
// constructor 
// 
RNG::RNG (const char *s) 
{ 
	if (strlen (s) > 99) {
		strncpy (name_, s, 99);
		name_[100] = 0;
	}
	else 
		strcpy (name_, s);

	init();
}


//------------------------------------------------------------------------- 
// Reset Stream to beginning of Stream. 
// 
void RNG::reset_start_stream () 
{ 
	for (int i = 0; i < 6; ++i) 
		Cg_[i] = Bg_[i] = Ig_[i]; 
} 

//------------------------------------------------------------------------- 
// Reset Stream to beginning of SubStream. 
// 
void RNG::reset_start_substream () 
{ 
	for (int i = 0; i < 6; ++i) 
		Cg_[i] = Bg_[i]; 
} 

//------------------------------------------------------------------------- 
// Reset Stream to NextSubStream. 
// 
void RNG::reset_next_substream () 
{ 
	MatVecModM(A1p76, Bg_, Bg_, m1); 
	MatVecModM(A2p76, &Bg_[3], &Bg_[3], m2); 
	for (int i = 0; i < 6; ++i) 
		Cg_[i] = Bg_[i]; 
} 

//------------------------------------------------------------------------- 
void RNG::set_package_seed (const unsigned long seed[6]) 
{ 
	if (CheckSeed (seed)) 
		abort();
	for (int i = 0; i < 6; ++i) 
		next_seed_[i] = seed[i]; 
} 

//------------------------------------------------------------------------- 
void RNG::set_seed (const unsigned long seed[6]) 
{ 
	if (CheckSeed (seed)) 
		abort();
	for (int i = 0; i < 6; ++i) 
		Cg_[i] = Bg_[i] = Ig_[i] = seed[i]; 
} 

//------------------------------------------------------------------------- 
// if e > 0, let n = 2^e + c; 
// if e < 0, let n = -2^(-e) + c; 
// if e = 0, let n = c. 
// Jump n steps forward if n > 0, backwards if n < 0. 
// 
void RNG::advance_state (long e, long c) 
{ 
	double B1[3][3], C1[3][3], B2[3][3], C2[3][3]; 
	if (e > 0) { 
		MatTwoPowModM (A1p0, B1, m1, e); 
		MatTwoPowModM (A2p0, B2, m2, e); 
	} else if (e < 0) { 
		MatTwoPowModM (InvA1, B1, m1, -e); 
		MatTwoPowModM (InvA2, B2, m2, -e); 
	} 
	if (c >= 0) { 
		MatPowModM (A1p0, C1, m1, c); 
		MatPowModM (A2p0, C2, m2, c); 
	} else { 
		MatPowModM (InvA1, C1, m1, -c); 
		MatPowModM (InvA2, C2, m2, -c); 
	} 
	if (e) { 
		MatMatModM (B1, C1, C1, m1); 
		MatMatModM (B2, C2, C2, m2); 
	} 
	MatVecModM (C1, Cg_, Cg_, m1); 
	MatVecModM (C2, &Cg_[3], &Cg_[3], m2); 
} 

//------------------------------------------------------------------------- 
void RNG::get_state (unsigned long seed[6]) const 
{ 
	for (int i = 0; i < 6; ++i) 
		seed[i] = static_cast<unsigned long> (Cg_[i]); 
} 

//------------------------------------------------------------------------- 
void RNG::write_state () const 
{ 
	printf ("The current state of the Rngstream %s:\n", name_);
	printf (" Cg_ = { ");
	for(int i=0;i<5;i++) { 
		printf ("%lu, ", (unsigned long) Cg_[i]);
	} 
	printf ("%lu }\n\n", (unsigned long) Cg_[5]);
} 

//------------------------------------------------------------------------- 
void RNG::write_state_full () const 
{ 
	int i; 
	printf ("The RNG %s:\n", name_);
	printf (" anti_ = %s", (anti_ ? "true" : "false")); 
	printf (" inc_prec_ = %s\n", (inc_prec_ ? "true" : "false")); 

	printf (" Ig_ = { ");
	for (i = 0; i < 5; i++) { 
		printf ("%lu, ", (unsigned long) Ig_[i]);
	} 
	printf ("%lu }\n", (unsigned long) Ig_[5]);

	printf (" Bg_ = { ");
	for (i = 0; i < 5; i++) { 
		printf ("%lu, ", (unsigned long) Bg_[i]);
	} 
	printf ("%lu }\n", (unsigned long) Bg_[5]);

	printf (" Cg_ = { ");
	for (i = 0; i < 5; i++) { 
		printf ("%lu, ", (unsigned long) Cg_[i]);
	} 
	printf ("%lu }\n\n", (unsigned long) Cg_[5]);
} 

//------------------------------------------------------------------------- 
void RNG::increased_precis (bool incp) 
{ 
	inc_prec_ = incp; 
} 

//------------------------------------------------------------------------- 
void RNG::set_antithetic (bool a) 
{ 
	anti_ = a; 
} 

//------------------------------------------------------------------------- 
// Generate the next random number. 
// 
double RNG::rand_u01 () 
{ 
	if (inc_prec_) 
		return U01d(); 
	else 
		return U01(); 
} 

//------------------------------------------------------------------------- 
// Generate the next random integer. 
// 
long RNG::rand_int (long low, long high) 
{ 
	//	return (long) low + (long) (((double) (high-low) * drn) + 0.5);
	return ((long) (low + (unsigned long) (((unsigned long) 
						(high-low+1)) * rand_u01())));
} 
#endif /* !OLD_RNG */
