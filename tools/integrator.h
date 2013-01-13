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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/tools/integrator.h,v 1.9 1999/09/28 03:46:29 heideman Exp $ (LBL)
 */

#ifndef ns_integrator_h
#define ns_integrator_h

#include "config.h"

class Integrator : public TclObject {
public:
	Integrator();
	void set(double x, double y);
	void newPoint(double x, double y);
	int command(int argc, const char*const* argv);
protected:
	double lastx_;
	double lasty_;
	double sum_;
};

// a set of statistical samples
class Samples : public TclObject {
public:
	Samples() : cnt_(0), sum_(0.0), sqsum_(0.0) { }
	void newPoint(double val) {
		cnt_++;
		sum_ += val;
		val *= val;
		sqsum_ += val;
	}
	int cnt() const { return (cnt_); }
	double sum() const { return (sum_); }
	double mean() const {
		if (cnt_)
			return (sum_ / cnt_);
		return sum_; // yes, that is 0.0...
	}
	double variance() const {
		// use cnt_-1 degrees of freedom
		if (cnt_ > 1)
			return ((sqsum_ - mean() * sum_) / (cnt_ - 1));
		return 0.0;
	}
	void reset() { cnt_ = 0; sum_ = sqsum_ = 0.0; }
	int command(int argc, const char*const* argv);
protected:
	int	cnt_;	// count of samples
	double	sum_;	// sum of x_i
	double	sqsum_;	// sum of (x_i)^2
};
#endif
