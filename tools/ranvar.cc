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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tools/ranvar.cc,v 1.25 2011/05/16 03:49:09 tom_henderson Exp $ (Xerox)";
#endif

#include <stdio.h>
#include "ranvar.h"

RandomVariable::RandomVariable()
{
	rng_ = RNG::defaultrng(); 
}

int RandomVariable::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "value") == 0) {
			tcl.resultf("%6e", value());
			return(TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "use-rng") == 0) {
			rng_ = (RNG*)TclObject::lookup(argv[2]);
			if (rng_ == 0) {
				tcl.resultf("no such RNG %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
	}
	return(TclObject::command(argc, argv));
}

// Added by Debojyoti Dutta 12 October 2000
// This allows us to seed a randomvariable with 
// our own RNG object. This command is called from 
// expoo.cc and pareto.cc 

int  RandomVariable::seed(char *x){
        
        Tcl& tcl = Tcl::instance();

                rng_ = (RNG*)TclObject::lookup(x);
                if (rng_ == 0) {
                        tcl.resultf("no such RNG %s", x);
                        return(TCL_ERROR);
                }
                return(TCL_OK);
 
}


static class UniformRandomVariableClass : public TclClass {
public:
	UniformRandomVariableClass() : TclClass("RandomVariable/Uniform"){}
	TclObject* create(int, const char*const*) {
		 return(new UniformRandomVariable());
	 }
} class_uniformranvar;

UniformRandomVariable::UniformRandomVariable()
{
	bind("min_", &min_);
	bind("max_", &max_); 
}

UniformRandomVariable::UniformRandomVariable(double min, double max)
{
	min_ = min;
	max_ = max;
}

double UniformRandomVariable::value()
{
	return(rng_->uniform(min_, max_));
}


static class ExponentialRandomVariableClass : public TclClass {
public:
	ExponentialRandomVariableClass() : TclClass("RandomVariable/Exponential") {}
	TclObject* create(int, const char*const*) {
		return(new ExponentialRandomVariable());
	}
} class_exponentialranvar;

ExponentialRandomVariable::ExponentialRandomVariable()
{
	bind("avg_", &avg_);
}

ExponentialRandomVariable::ExponentialRandomVariable(double avg)
{
	avg_ = avg;
}

double ExponentialRandomVariable::value()
{
	return(rng_->exponential(avg_));
}

/*
	Generates Erlang variables following:
	
	                   x^(k-1)*exp(-x/lambda)
	p(x; lambda, k) = ------------------------
                       (k-1)!*lambda^k
*/
static class ErlangRandomVariableClass : public TclClass {
public:
	ErlangRandomVariableClass() : TclClass("RandomVariable/Erlang") {}
	TclObject* create(int, const char*const*) {
		return(new ErlangRandomVariable());
	}
} class_erlangranvar;
 
ErlangRandomVariable::ErlangRandomVariable()
{
	bind("lambda_", &lambda_);
	bind("k_", &k_);
}


ErlangRandomVariable::ErlangRandomVariable(double lambda, int k)
{
	lambda_ = lambda;
	k_ = k;
}

double ErlangRandomVariable::value()
{
	double result = 0;
	for (int i = 0; i < k_; i++) {
		result += rng_->exponential(lambda_);
	}
	return result;

}


/*
	Generates Erlang variables following:
	
	                     x^(alpha-1)*exp(-x/beta)
	p(x; alpha, beta) = --------------------------
                         Gamma(alpha)*beta^alpha
*/
static class GammaRandomVariableClass : public TclClass {
public:
	GammaRandomVariableClass() : TclClass("RandomVariable/Gamma") {}
	TclObject* create(int, const char*const*) {
		return(new GammaRandomVariable());
	}
} class_gammaranvar;

GammaRandomVariable::GammaRandomVariable()
{
	bind("alpha_", &alpha_);
	bind("beta_", &beta_);
}

GammaRandomVariable::GammaRandomVariable(double alpha, double beta)
{
	alpha_ = alpha;
	beta_ = beta;
}

double GammaRandomVariable::value()
{
	// Proposed by Marsaglia in 2000:
	// G. Marsaglia, W. W. Tsang: A simple method for gereating Gamma variables
	// ACM Transactions on mathematical software, Vol. 26, No. 3, Sept. 2000
	if (alpha_ < 1) {
		double u = rng_->uniform(1.0);
		return GammaRandomVariable(1.0 + alpha_, beta_).value() * pow (u, 1.0 / alpha_);
	}
	
	double x, v, u;
	double d = alpha_ - 1.0 / 3.0;
	double c = (1.0 / 3.0) / sqrt (d);

	while (1) {
		do {
			x = rng_->normal(0.0, 1.0);
			v = 1.0 + c * x;
		} while (v <= 0);

		v = v * v * v;
		u = rng_->uniform(1.0);
		if (u < 1 - 0.0331 * x * x * x * x)
			break;
		if (log (u) < 0.5 * x * x + d * (1 - v + log (v)))
			break;
	}
	return beta_ * d * v;
}



static class ParetoRandomVariableClass : public TclClass {
 public:
	ParetoRandomVariableClass() : TclClass("RandomVariable/Pareto") {}
	TclObject* create(int, const char*const*) {
		return(new ParetoRandomVariable());
	}
} class_paretoranvar;

ParetoRandomVariable::ParetoRandomVariable()
{
	bind("avg_", &avg_);
	bind("shape_", &shape_);
}

ParetoRandomVariable::ParetoRandomVariable(double avg, double shape)
{
	avg_ = avg;
	shape_ = shape;
}

double ParetoRandomVariable::value()
{
	/* yuck, user wants to specify shape and avg, but the
	 * computation here is simpler if we know the 'scale'
	 * parameter.  right thing is to probably do away with
	 * the use of 'bind' and provide an API such that we
	 * can update the scale everytime the user updates shape
	 * or avg.
	 */
	return(rng_->pareto(avg_ * (shape_ -1)/shape_, shape_));
}

/* Pareto distribution of the second kind, aka. Lomax distribution */
static class ParetoIIRandomVariableClass : public TclClass {
 public:
        ParetoIIRandomVariableClass() : TclClass("RandomVariable/ParetoII") {}
        TclObject* create(int, const char*const*) {
                return(new ParetoIIRandomVariable());
        }
} class_paretoIIranvar;

ParetoIIRandomVariable::ParetoIIRandomVariable()
{
        bind("avg_", &avg_);
        bind("shape_", &shape_);
}

ParetoIIRandomVariable::ParetoIIRandomVariable(double avg, double shape)
{
        avg_ = avg;
        shape_ = shape;
}

double ParetoIIRandomVariable::value()
{
        return(rng_->paretoII(avg_ * (shape_ - 1), shape_));
}

static class NormalRandomVariableClass : public TclClass {
 public:
        NormalRandomVariableClass() : TclClass("RandomVariable/Normal") {}
        TclObject* create(int, const char*const*) {
                return(new NormalRandomVariable());
        }
} class_normalranvar;
 
NormalRandomVariable::NormalRandomVariable()
{ 
        bind("avg_", &avg_);
        bind("std_", &std_);
}
 
double NormalRandomVariable::value()
{
        return(rng_->normal(avg_, std_));
}

static class LogNormalRandomVariableClass : public TclClass {
 public:
        LogNormalRandomVariableClass() : TclClass("RandomVariable/LogNormal") {}
        TclObject* create(int, const char*const*) {
                return(new LogNormalRandomVariable());
        }
} class_lognormalranvar;
 
LogNormalRandomVariable::LogNormalRandomVariable()
{ 
        bind("avg_", &avg_);
        bind("std_", &std_);
}
 
double LogNormalRandomVariable::value()
{
        return(rng_->lognormal(avg_, std_));
}

static class ConstantRandomVariableClass : public TclClass {
 public:
	ConstantRandomVariableClass() : TclClass("RandomVariable/Constant"){}
	TclObject* create(int, const char*const*) {
		return(new ConstantRandomVariable());
	}
} class_constantranvar;

ConstantRandomVariable::ConstantRandomVariable()
{
	bind("val_", &val_);
}

ConstantRandomVariable::ConstantRandomVariable(double val)
{
	val_ = val;
}

double ConstantRandomVariable::value()
{
	return(val_);
}


/* Hyperexponential distribution code adapted from code provided
 * by Ion Stoica.
 */

static class HyperExponentialRandomVariableClass : public TclClass {
public:
	HyperExponentialRandomVariableClass() : 
	TclClass("RandomVariable/HyperExponential") {}
	TclObject* create(int, const char*const*) {
		return(new HyperExponentialRandomVariable());
	}
} class_hyperexponentialranvar;

HyperExponentialRandomVariable::HyperExponentialRandomVariable()
{
	bind("avg_", &avg_);
	bind("cov_", &cov_);
	alpha_ = .95;
}

HyperExponentialRandomVariable::HyperExponentialRandomVariable(double avg, double cov)
{
	alpha_ = .95;
	avg_ = avg;
	cov_ = cov;
}

double HyperExponentialRandomVariable::value()
{
	double temp, res;
	double u = Random::uniform();

	temp = sqrt((cov_ * cov_ - 1.0)/(2.0 * alpha_ * (1.0 - alpha_)));
	if (u < alpha_)
		res = Random::exponential(avg_ - temp * (1.0 - alpha_) * avg_);
	else
		res = Random::exponential(avg_ + temp * (alpha_) * avg_);
	return(res);
}

static class WeibullRandomVariableClass : public TclClass {
 public:
        WeibullRandomVariableClass() : TclClass("RandomVariable/Weibull") {}
        TclObject* create(int argc, const char*const* argv) {
                if (argc == 6) {
                        return (new WeibullRandomVariable ((double) atof(argv[4]),
                                                           (double) atof(argv[5])));
                }
                else if (argc == 7) {
                        // shape, scale, RNG
                        RNG* rng = (RNG*)TclObject::lookup(argv[6]);
                        return (new WeibullRandomVariable ((double) atof(argv[4]),
                                                           (double) atof(argv[5]),
                                                           rng));
                }
                else {
                        return(new WeibullRandomVariable());
                }
        }                                           
} class_weibullranvar;

WeibullRandomVariable::WeibullRandomVariable()
{        
	bind("shape_", &shape_);
        bind("scale_", &scale_);
}

WeibullRandomVariable::WeibullRandomVariable(double scale, double shape)
{
        shape_ = shape;
        scale_ = scale;
}

WeibullRandomVariable::WeibullRandomVariable(double scale, double shape, 
					     RNG* rng) 
{
        shape_ = shape;
        scale_ = scale;
        rng_ = rng;
}

double WeibullRandomVariable::avg(void)         
{
        return 0;
}

double WeibullRandomVariable::value()
{
        return(rng_->rweibull(scale_, shape_));
}
                                                                               
/*
// Empirical Random Variable:
//  CDF input from file with the following column
//   1.  Possible values in a distrubutions
//   2.  Number of occurances for those values
//   3.  The CDF for those value
//  code provided by Giao Nguyen
*/

static class EmpiricalRandomVariableClass : public TclClass {
public:
	EmpiricalRandomVariableClass() : TclClass("RandomVariable/Empirical"){}
	TclObject* create(int, const char*const*) {
		return(new EmpiricalRandomVariable());
	}
} class_empiricalranvar;

EmpiricalRandomVariable::EmpiricalRandomVariable() : minCDF_(0), maxCDF_(1), maxEntry_(32), table_(0)
{
	bind("minCDF_", &minCDF_);
	bind("maxCDF_", &maxCDF_);
	bind("interpolation_", &interpolation_);
	bind("maxEntry_", &maxEntry_);
}

int EmpiricalRandomVariable::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "loadCDF") == 0) {
			if (loadCDF(argv[2]) == 0) {
				tcl.resultf("%s loadCDF %s: invalid file",
					    name(), argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return RandomVariable::command(argc, argv);
}

int EmpiricalRandomVariable::loadCDF(const char* filename)
{
	FILE* fp;
	char line[256];
	CDFentry* e;

	fp = fopen(filename, "r");
	if (fp == 0) 
		return 0;


	if (table_ == 0)
		table_ = new CDFentry[maxEntry_];
	for (numEntry_=0;  fgets(line, 256, fp);  numEntry_++) {
		if (numEntry_ >= maxEntry_) {	// resize the CDF table
			maxEntry_ *= 2;
			e = new CDFentry[maxEntry_];
			for (int i=numEntry_-1; i >= 0; i--)
				e[i] = table_[i];
			delete table_;
			table_ = e;
		}
		e = &table_[numEntry_];
		// Use * and l together raises a warning
		sscanf(line, "%lf %*f %lf", &e->val_, &e->cdf_);
	}
        fclose(fp);
	return numEntry_;
}

double EmpiricalRandomVariable::value()
{
	if (numEntry_ <= 0)
		return 0;
	double u = rng_->uniform(minCDF_, maxCDF_);
	int mid = lookup(u);
	if (mid && interpolation_ && u < table_[mid].cdf_)
		return interpolate(u, table_[mid-1].cdf_, table_[mid-1].val_,
				   table_[mid].cdf_, table_[mid].val_);
	return table_[mid].val_;
}

double EmpiricalRandomVariable::interpolate(double x, double x1, double y1, double x2, double y2)
{
	double value = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
	if (interpolation_ == INTER_INTEGRAL)	// round up
		return ceil(value);
	return value;
}

int EmpiricalRandomVariable::lookup(double u)
{
	// always return an index whose value is >= u
	int lo, hi, mid;
	if (u <= table_[0].cdf_)
		return 0;
	for (lo=1, hi=numEntry_-1;  lo < hi; ) {
		mid = (lo + hi) / 2;
		if (u > table_[mid].cdf_)
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}
