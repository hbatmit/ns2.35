/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/tools/ranvar.h,v 1.18 2008/02/01 21:39:43 tom_henderson Exp $ (Xerox)
 */

#ifndef ns_ranvar_h
#define ns_ranvar_h

/* XXX still need to clean up dependencies among parameters such that
 * when one parameter is changed, other parameters are recomputed as
 * appropriate.
 */

#include "random.h"
#include "rng.h"

class RandomVariable : public TclObject {
 public:
	virtual double value() = 0;
	virtual double avg() = 0;
	int command(int argc, const char*const* argv);
	RandomVariable();
	// This is added by Debojyoti Dutta 12th Oct 2000
	int seed(char *);
 protected:
	RNG* rng_;
};

class UniformRandomVariable : public RandomVariable {
 public:
	virtual double value();
	virtual inline double avg() { return (max_-min_)/2; };
	UniformRandomVariable();
	UniformRandomVariable(double, double);
	double* minp()	{ return &min_; };
	double* maxp()	{ return &max_; };
	double min()	{ return min_; };
	double max()	{ return max_; };
	void setmin(double d)	{ min_ = d; };
	void setmax(double d)	{ max_ = d; };
 private:
	double min_;
	double max_;
};

class ExponentialRandomVariable : public RandomVariable {
 public:
	virtual double value();
	ExponentialRandomVariable();
	ExponentialRandomVariable(double);
	double* avgp() { return &avg_; };
	virtual inline double avg() { return avg_; };
	void setavg(double d) { avg_ = d; };
 private:
	double avg_;
};

class ErlangRandomVariable : public RandomVariable {
 public:
	virtual double value();
	ErlangRandomVariable();
	ErlangRandomVariable(double, int);
	virtual inline double avg() { return k_/lambda_; };
 private:
	double lambda_;
	int    k_;
};


class GammaRandomVariable : public RandomVariable {
 public:
	virtual double value();
	GammaRandomVariable();
	GammaRandomVariable(double, double);
	virtual inline double avg() { return alpha_*beta_; };
 private:
	double alpha_;
	double beta_;
};


class ParetoRandomVariable : public RandomVariable {
 public:
	virtual double value();
	ParetoRandomVariable();
	ParetoRandomVariable(double, double);
	double* avgp() { return &avg_; };
	double* shapep() { return &shape_; };
	virtual inline double avg()	{ return avg_; };
	double shape()	{ return shape_; };
	void setavg(double d)	{ avg_ = d; };
	void setshape(double d)	{ shape_ = d; };
 private:
	double avg_;
	double shape_;
	double scale_;
};

class ParetoIIRandomVariable : public RandomVariable {
 public:
        virtual double value();
        ParetoIIRandomVariable();
        ParetoIIRandomVariable(double, double);
        double* avgp() { return &avg_; };
        double* shapep() { return &shape_; };
        virtual inline double avg()   { return avg_; };
        double shape()   { return shape_; };
        void setavg(double d)  { avg_ = d; };
        void setshape(double d)  { shape_ = d; };
 private:
        double avg_;
        double shape_;
        double scale_;
};

class NormalRandomVariable : public RandomVariable {
 public:
        virtual double value();
        NormalRandomVariable();
        inline double* avgp() { return &avg_; };
        inline double* stdp() { return &std_; };
        virtual inline double avg()     { return avg_; };
        inline double std()     { return std_; };
        inline void setavg(double d)    { avg_ = d; };
        inline void setstd(double d)    { std_ = d; };
 private:
        double avg_;
        double std_;
};

class LogNormalRandomVariable : public RandomVariable {
public:
        virtual double value();
        LogNormalRandomVariable();
        inline double* avgp() { return &avg_; };
        inline double* stdp() { return &std_; };
        virtual inline double avg()     { return avg_; };
        inline double std()     { return std_; };
        inline void setavg(double d)    { avg_ = d; };
        inline void setstd(double d)    { std_ = d; };
private:
        double avg_;
        double std_;
};

class ConstantRandomVariable : public RandomVariable {
 public:
	virtual double value();
	virtual double avg(){ return val_;}
	ConstantRandomVariable();
	ConstantRandomVariable(double);
	double* valp() { return &val_; };
	double val() { return val_; };
	void setval(double d) { val_ = d; };
 private:
	double val_;
};

class HyperExponentialRandomVariable : public RandomVariable {
 public:
	virtual double value();
	HyperExponentialRandomVariable();
	HyperExponentialRandomVariable(double, double);
	double* avgp()	{ return &avg_; };
	double* covp()	{ return &cov_; };
	virtual double avg()	{ return avg_; };
	double cov()	{ return cov_; };
	void setavg(double d)	{ avg_ = d; };
	void setcov(double d)	{ cov_ = d; };
 private:
	double avg_;
	double cov_;
	double alpha_;
};

class WeibullRandomVariable : public RandomVariable {
public:
        virtual double value();
        virtual double avg();
        WeibullRandomVariable();
        WeibullRandomVariable(double shape, double scale);
        WeibullRandomVariable(double shape, double scale, RNG* rng);
        double* shapep() { return &shape_; };
        double* scalep() { return &scale_; };
        double shape()   { return shape_; };
        double scale()   { return scale_; };
        void setshape(double d)  { shape_ = d; };       
        void setscale(double d)  { scale_ = d; };
private:
        double shape_;
        double scale_;
};


#define INTER_DISCRETE 0	// no interpolation (discrete)
#define INTER_CONTINUOUS 1	// linear interpolation
#define INTER_INTEGRAL 2	// linear interpolation and round up

struct CDFentry {
	double cdf_;
	double val_;
};

class EmpiricalRandomVariable : public RandomVariable {
public:
	virtual double value();
	virtual double interpolate(double u, double x1, double y1, double x2, double y2);
	virtual double avg(){ return value(); } // junk
	EmpiricalRandomVariable();
	double& minCDF() { return minCDF_; }
	double& maxCDF() { return maxCDF_; }
	int loadCDF(const char* filename);

protected:
	int command(int argc, const char*const* argv);
	int lookup(double u);

	double minCDF_;		// min value of the CDF (default to 0)
	double maxCDF_;		// max value of the CDF (default to 1)
	int interpolation_;	// how to interpolate data (INTER_DISCRETE...)
	int numEntry_;		// number of entries in the CDF table
	int maxEntry_;		// size of the CDF table (mem allocation)
	CDFentry* table_;	// CDF table of (val_, cdf_)
};

#endif




