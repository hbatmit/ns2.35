/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* 
 * Copyright 2002, Statistics Research, Bell Labs, Lucent Technologies and
 * The University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Reference
 *     Stochastic Models for Generating Synthetic HTTP Source Traffic 
 *     J. Cao, W.S. Cleveland, Y. Gao, K. Jeffay, F.D. Smith, and M.C. Weigle 
 *     IEEE INFOCOM 2004.
 *
 * Documentation available at http://dirt.cs.unc.edu/packmime/
 * 
 * Contacts: Michele Weigle (mcweigle@cs.unc.edu),
 *           Kevin Jeffay (jeffay@cs.unc.edu)
 */

#include <stdio.h>
#include "rng.h"

#define ROOT_2 1.4142135623730950488016887242096980785697 /*2^1/2*/
#define E22    1.7155277699214135929603792825575449562416 /*sqrt(8/e) */

double RNG::gammln (double xx) 
{
	int j;
	double x, y, tmp, ser;
	static double cof[6]={76.18009172947146,-86.50532032941677,
			      24.01409824083091,-1.231739572450155,
			      0.1208650973866179e-2,-0.5395239384953e-5};
	y = x = xx;
	tmp = x+5.5;
	tmp -= (x+0.5) * log(tmp);
	ser = 1.000000000190015;
	for (j=0; j<=5; j++) 
		ser += cof[j] / ++y;

	return (-tmp + log(2.5066282746310005*ser/x));
}

double RNG::pnorm (double q) 
{ 
	if (q == 0)
		return(0.5);
	else if (q > 0)
		return((1 + erf(q/ROOT_2))/2);
	else
		return(erfc(-q/ROOT_2)/2);
}

double 
RNG::rnorm (void) 
{
	double u, x, y;
	do {
		u = uniform();
		while(u==0)
			u = uniform();
		x = E22 * (uniform() - 0.5) / u;
		y = x * x / 4.0;
	} while (y > 1.0 - u && (y > 1.0/u - 1.0 || y > -log(u)));
	
	return(x);
}

int 
RNG::rbernoulli (double p) {
	double x; 
	int z; 
  
	x = uniform();
	if (x <= p) 
		z=1; 
	else 
		z=0;

	return(z);
}

// this is taken from the R src code
// generate standard exponential variate
double RNG::exp_rand (void)
{
	/** q[k-1] = sum(log(2)^k / k!)  k=1,..,n, 
	 *  The highest n (here 8) is determined by q[n-1] = 1.0 
	 *  within standard precision 
	 */
	const double q[] = {
		0.6931471805599453,
		0.9333736875190459,
		0.9888777961838675,
		0.9984959252914960,
		0.9998292811061389,
		0.9999833164100727,
		0.9999985691438767,
		0.9999998906925558,
		0.9999999924734159,
		0.9999999995283275,
		0.9999999999728814,
		0.9999999999985598,
		0.9999999999999289,
		0.9999999999999968,
		0.9999999999999999,
		1.0000000000000000
	};
	double a, u, ustar, umin;
	int i;

	a = 0.;
	u = uniform();
	for (;;) {
		u += u;
		if (u > 1.0)
			break;
		a += q[0];
	}
	u -= 1.;

	if (u <= q[0])
		return a + u;

	i = 0;
	ustar = uniform();
	umin = ustar;
	do {
		ustar = uniform();
		if (ustar < umin)
			umin = ustar;
		i++;
	} while (u > q[i]);

	return a + umin * q[0];
}

#define repeat for(;;)
double RNG::rgamma (double a, double scale)
{
	/* Constants : */
	const double sqrt32 = 5.656854;
	const double exp_m1 = 0.36787944117144232159;/* exp(-1) = 1/e */

	/** Coefficients q[k] - for q0 = sum(q[k]*a^(-k))
	 *  Coefficients a[k] - for q = q0+(t*t/2)*sum(a[k]*v^k)
	 *  Coefficients e[k] - for exp(q)-1 = sum(e[k]*q^k)
	 */
	const double q1 = 0.04166669;
	const double q2 = 0.02083148;
	const double q3 = 0.00801191;
	const double q4 = 0.00144121;
	const double q5 = -7.388e-5;
	const double q6 = 2.4511e-4;
	const double q7 = 2.424e-4;

	const double a1 = 0.3333333;
	const double a2 = -0.250003;
	const double a3 = 0.2000062;
	const double a4 = -0.1662921;
	const double a5 = 0.1423657;
	const double a6 = -0.1367177;
	const double a7 = 0.1233795;

	const double e1 = 1.0;
	const double e2 = 0.4999897;
	const double e3 = 0.166829;
	const double e4 = 0.0407753;
	const double e5 = 0.010293;

	/* State variables [FIXME for threading!] :*/
	static double aa = 0.;
	static double aaa = 0.;
	static double s, s2, d;    /* no. 1 (step 1) */
	static double q0, b, si, c;/* no. 2 (step 4) */

	double e, p, q, r, t, u, v, w, x, ret_val;

	if (a < 1.) { /* GS algorithm for parameters a < 1 */
		e = 1.0 + exp_m1 * a;
		repeat {
			p = e * uniform();
			if (p >= 1.0) {
				x = -log((e - p) / a);
				if (exp_rand() >= (1.0 - a) * log(x))
					break;
			} else {
				x = exp(log(p) / a);
				if (exp_rand() >= x)
					break;
			}
		}
		return scale * x;
	}

	/* --- a >= 1 : GD algorithm --- */

	/* Step 1: Recalculations of s2, s, d if a has changed */
	if (a != aa) {
		aa = a;
		s2 = a - 0.5;
		s = sqrt(s2);
		d = sqrt32 - s * 12.0;
	}
	/** Step 2: t = standard normal deviate,
         *     x = (s,1/2) -normal deviate. 
	 */

	/* immediate acceptance (i) */
	t = rnorm();
	x = s + 0.5 * t;
	ret_val = x * x;
	if (t >= 0.0)
		return scale * ret_val;

	/* Step 3: u = 0,1 - uniform sample. squeeze acceptance (s) */
	u = uniform();
	if (d * u <= t * t * t)
		return scale * ret_val;

	/* Step 4: recalculations of q0, b, si, c if necessary */

	if (a != aaa) {
		aaa = a;
		r = 1.0 / a;
		q0 = ((((((q7 * r + q6) * r + q5) * r + q4) * r + q3) * r 
		       + q2) * r + q1) * r;

		/** Approximation depending on size of parameter a 
		 * The constants in the expressions for b, si and c 
		 * were established by numerical experiments 
		 */

		if (a <= 3.686) {
			b = 0.463 + s + 0.178 * s2;
			si = 1.235;
			c = 0.195 / s - 0.079 + 0.16 * s;
		} else if (a <= 13.022) {
			b = 1.654 + 0.0076 * s2;
			si = 1.68 / s + 0.275;
			c = 0.062 / s + 0.024;
		} else {
			b = 1.77;
			si = 0.75;
			c = 0.1515 / s;
		}
	}
	/* Step 5: no quotient test if x not positive */

	if (x > 0.0) {
		/* Step 6: calculation of v and quotient q */
		v = t / (s + s);
		if (fabs(v) <= 0.25)
			q = q0 + 0.5 * t * t * 
				((((((a7 * v + a6) * v + a5) * v + a4) * 
				   v + a3) * v + a2) * v + a1) * v;
		else
			q = q0 - s * t + 0.25 * t * t + (s2 + s2) * 
				log(1.0 + v);


		/* Step 7: quotient acceptance (q) */
		if (log(1.0 - u) <= q)
			return scale * ret_val;
	}

	repeat {
		/** Step 8: e = standard exponential deviate
		 *	u =  0,1 -uniform deviate
		 *	t = (b,si)-double exponential (laplace) sample 
		 */
		e = exp_rand();
		u = uniform();
		u = u + u - 1.0;
		if (u < 0.0)
			t = b - si * e;
		else
			t = b + si * e;
		/* Step	 9:  rejection if t < tau(1) = -0.71874483771719 */
		if (t >= -0.71874483771719) {
			/* Step 10:	 calculation of v and quotient q */
			v = t / (s + s);
			if (fabs(v) <= 0.25)
				q = q0 + 0.5 * t * t * 
					((((((a7 * v + a6) * v + a5) * v + a4) * 
					   v + a3) * v + a2) * v + a1) * v;
			else
				q = q0 - s * t + 0.25 * t * t + (s2 + s2) * 
					log(1.0 + v);
			/** Step 11:	 hat acceptance (h) 
			 * (if q not positive go to step 8) 
			 */
			if (q > 0.0) {
		if (q <= 0.5)
			w = ((((e5 * q + e4) * q + e3) * q + e2) * q + e1) * q;
		else
			w = exp(q) - 1.0;
		/* if t is rejected sample again at step 8 */
		if (c * fabs(u) <= w * exp(e - 0.5 * t * t))
			break;
			}
		}
	}
	x = s + 0.5 * t;

	return scale * x * x;
}

