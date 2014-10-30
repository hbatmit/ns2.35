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

#include <sys/types.h>
#include <sys/stat.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "packmime_OL_ranvar.h"

/**
 * Reference:
 * Modeling Persistence in Hydrological Time Series Using Fractional 
 * Differencing
 * By J.R.M. Hosking, Water Resources Research, Vol 20, No 12, P1898-1908, 
 * Dec 1984
 *
 * y[t] = sum_j=1^p (AR[j] * y[t-j]) + x[t] - sum_j=1^q (MA[j] * x[t-j]
 */
#define ROOT_2 1.4142135623730950488016887242096980785697 /*2^1/2*/
#define E22    1.7155277699214135929603792825575449562416 /*sqrt(8/e) */
#define EPSILON 1e-2
#define Min(a, b)      ((a) < (b) ? (a) : (b))

/**
 * hard wired model constants from fitting BL-MH packet trace
 * (all tcp, both directions), parameters values from 
 * /home/cao/MySwork/packet/dataObj/superpose/.Data/pac.fit1.para.vs.rate.fit
 * ; the original code to produce parameters is in superpose_par_vs_rate.S
 */
//static struct OL_weibull_params pac_ia_weibull_params = {
//  -0.487,   /* shapeParam0 */
//  0.032,    /* shapeParam1 */
//  1.702     /* shapeParam2 */
//};

static struct OL_arima_params pac_ia_arima_params = {
  0.41,      /* d */
  1000,      /* number of finite AR rep of farima */
  0, 1,      /* pAR=0, qMA */

  -1.345,    /* varRatioParam0 */ 
  0.009,     /* varRatioParam1 */
  2.217      /* varRatioParam2 */
};


/*:::::::::::::::::::::: Packet Inter-Arrival RanVar :::::::::::::::::*/
static class PackMimeOLPacketIARandomVariableClass : public TclClass {
public:
	PackMimeOLPacketIARandomVariableClass() : 
	  TclClass("RandomVariable/PackMimeOLPacketIA"){}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) {
			// bitrate
			return(new PackMimeOLPacketIARandomVariable((double)atof(argv[4])));
		} else if (argc == 6) {
			// bitrate, RNG		      
			RNG* rng = (RNG*)TclObject::lookup(argv[5]);
			return(new PackMimeOLPacketIARandomVariable((double)atof(argv[4]), rng));
		} else if (argc == 7) {
			// bitrate, RNG
			RNG* rng = (RNG*)TclObject::lookup(argv[5]);
			PackMimeOLPacketIARandomVariable *iarng = new PackMimeOLPacketIARandomVariable((double)atof(argv[4]), rng);
			RandomVariable *sizerng_ = (RandomVariable*)TclObject::lookup(argv[6]);
			iarng->setPktSizeRng(sizerng_);
			return(iarng);
		} else {
			return(new PackMimeOLPacketIARandomVariable());
		}
	 }
} class_PacketIAranvar;

PackMimeOLPacketIARandomVariable::PackMimeOLPacketIARandomVariable(): 	
fARIMA(NULL), myrng(NULL), sizerng(NULL)
{
	bind_vars();
}

PackMimeOLPacketIARandomVariable::PackMimeOLPacketIARandomVariable(double bitrate_) :
bitrate(bitrate_), fARIMA(NULL), myrng(NULL), sizerng(NULL)
{
	bind_vars();
}

PackMimeOLPacketIARandomVariable::PackMimeOLPacketIARandomVariable(double bitrate_, RNG* rng_) :
bitrate(bitrate_), fARIMA(NULL), myrng(rng_), sizerng(NULL)
{
	bind_vars();
}

void PackMimeOLPacketIARandomVariable::bind_vars() {
	bind ("bitrate", &bitrate);
	bind ("pacrate", &pacrate);
	bind ("connum", &connum);
	bind ("conbps", &conbps);	
	bind ("conpacrate", &conpacrate);
}

/* initialize for pac.ia */
void PackMimeOLPacketIARandomVariable::initialize() {
	double sigmaTotal;
	struct OL_arima_params *arima = &pac_ia_arima_params;
//	struct OL_weibull_params *weibull = &pac_ia_weibull_params;
	double d = arima->d;
	double logit_simia;

	assert(arima->N >= arima->qMA);
	
	if (sizerng==NULL)
		sizerng = new PackMimeOLPacketSizeRandomVariable();
	if (conbps<=0)
		conbps = conpacrate * sizerng->avg() * 8;
	if (connum<=0)
		connum = bitrate/conbps;
	if (pacrate<=0)
		pacrate = bitrate/sizerng->avg()/8;

	/**
	 * old method
	 * varRatio = 1.0 - pow(2.0, arima->varRatioParam0 - 
	 *		     arima->varRatioParam1 * pow(log(rate)/LOG2,
	 *		     arima->varRatioParam2)); 
	 */
	logit_simia = pow(2.0, -0.6661682 + 0.4192597*log(connum)/LOG2);
	varRatio = logit_simia/(1+logit_simia);

	sigmaTotal = 1.0;
	sigmaNoise = sigmaTotal * pow(varRatio,0.5);
	sigmaEpsilon = sigmaTotal * 
		pow(exp(2.0*myrng->gammln(1.0-d)-myrng->gammln(1.0-2.0*d))/(2+2*d/(1-d)) * (1-varRatio),
		    0.5);

	/**
	 * shape = 1.0 - pow(2.0, 
	 *	    weibull->shapeParam0 - 
	 *	    weibull->shapeParam1 * pow(log(rate)/LOG2, 
	 *				       weibull->shapeParam2));
	 */
	logit_simia = pow(2.0, -2.462267 + 0.4920002*log(connum)/LOG2);
	shape = logit_simia/(1+logit_simia);
	scale = 1.0 / (connum * exp(myrng->gammln(1+1.0/shape)));
	if (!myrng)
		myrng = new RNG();
	fARIMA = new FARIMA(myrng, arima->d, arima->N);
}

PackMimeOLPacketIARandomVariable::~PackMimeOLPacketIARandomVariable() {
	delete myrng;
	delete fARIMA;
}

/** 
 * generate packet IA according to the marginal distribution 
 */
double PackMimeOLPacketIARandomVariable::transform(double p)
{
	double yt;
 
	yt = myrng->qweibull(p, shape, scale);
  
	return(yt);
}

/**
 * generate packet IA according to Long-Range Dependent plus Noise (LRDN) model 
 */
double PackMimeOLPacketIARandomVariable::value() {
	double yt;

	if (fARIMA == NULL)
		initialize();

	yt = fARIMA->Next();
	yt = yt * sigmaEpsilon + myrng->rnorm() * sigmaNoise;
	yt = transform(myrng->pnorm(yt));

	return(yt);
}

double PackMimeOLPacketIARandomVariable::avg() {
	
	if (fARIMA == NULL)
		initialize();

	if (pacrate!=0)
		return (1/pacrate);
	else
		return 0;
}


/**
 * hard wired model constants from fitting BL-MH packet trace
 */
static struct OL_arima_params pac_size_arima_params = {
  0.41,         /* d */
  1000,         /* number of finite AR rep of farima */
  0, 1,         /* pAR=0, qMA */
  
  -0.051,       /* varRatioParam0 */
  0.08,         /* varRatioParam1 */ 
  1.597         /* varRatioParam2 */
};

/*:::::::::::::::::::::: Packet Size RanVar :::::::::::::::::*/
static class PackMimeOLPacketSizeRandomVariableClass : public TclClass {
public:
	PackMimeOLPacketSizeRandomVariableClass() : 
	  TclClass("RandomVariable/PackMimeOLPacketSize"){}
	TclObject* create(int argc, const char*const* argv) {		
		if (argc == 5) {
			// rate	
			return(new PackMimeOLPacketSizeRandomVariable((double)atof(argv[4])));		
		} else if (argc == 6) {
			// rate, RNG
			RNG* rng = (RNG*)TclObject::lookup(argv[5]);
			return(new PackMimeOLPacketSizeRandomVariable((double)atof(argv[4]), rng));
		} else if (argc == 7) {
			// rate, RNG, filename
			RNG* rng = (RNG*)TclObject::lookup(argv[5]);
			return(new PackMimeOLPacketSizeRandomVariable((double)atof(argv[4]), rng, argv[6]));
		} else {
			return(new PackMimeOLPacketSizeRandomVariable());
		}
	 }
} class_PackMimeOLPacketSizeranvar;

PackMimeOLPacketSizeRandomVariable::PackMimeOLPacketSizeRandomVariable():
	pac_size_dist_file(NULL), fARIMA(NULL), myrng(NULL)
{	
	bind_vars();
}

PackMimeOLPacketSizeRandomVariable::PackMimeOLPacketSizeRandomVariable(double bitrate_):
bitrate(bitrate_), pac_size_dist_file(NULL), fARIMA(NULL), myrng(NULL)
{
	bind_vars();
}

PackMimeOLPacketSizeRandomVariable::PackMimeOLPacketSizeRandomVariable(double bitrate_, RNG* rng_):
bitrate(bitrate_), pac_size_dist_file(NULL), fARIMA(NULL), myrng(rng_)
{		
	bind_vars();
}

PackMimeOLPacketSizeRandomVariable::PackMimeOLPacketSizeRandomVariable(double bitrate_, RNG* rng_, const char *pac_size_dist_file_):
bitrate(bitrate_), fARIMA(NULL), myrng(rng_)
{	
	bind_vars();
	pac_size_dist_file = new char[strlen(pac_size_dist_file_)+1];
        strcpy(pac_size_dist_file, pac_size_dist_file_);	
}

void PackMimeOLPacketSizeRandomVariable::bind_vars() {
	bind ("bitrate", &bitrate);
	bind ("connum", &connum);
	bind ("conbps", &conbps);
	bind ("conpacrate", &conpacrate);
}

int PackMimeOLPacketSizeRandomVariable::def_size_dist_left[14] =  {40,    41,    44,    45,    48,    49,    52,    53,    85,    221,   576,   577,   1401,  1500};
int PackMimeOLPacketSizeRandomVariable::def_size_dist_right[14] = {40,    43,    44,    47,    48,    51,    52,    84,    220,   575,   576,   1400,  1499,  1500};
double PackMimeOLPacketSizeRandomVariable::def_size_prob[14] =    {0.300, 0.000, 0.030, 0.000, 0.030, 0.010, 0.060, 0.070, 0.050, 0.070, 0.120, 0.060, 0.050, 0.150 };

/**
 * initialise for pac.size
 */
void PackMimeOLPacketSizeRandomVariable::initialize()
{
	double sigmaTotal;
	struct OL_arima_params *arima = &pac_size_arima_params;
	double d = arima->d;
	double logit_simsize;
  
	assert(arima->N >= arima->qMA);
	
	if (conbps<=0)
		conbps = conpacrate * avg() * 8;
	if (connum<=0)
		connum = bitrate/conbps;
	/**
	 * old method 
	 * varRatio = 1.0 - pow(2.0, 
         *              arima->varRatioParam0 - 
         *              arima->varRatioParam1 * pow(log(rate)/LOG2, 
	 *					   arima->varRatioParam2));
	 */
	logit_simsize = pow(2.0, -3.117395 + 0.7224436*log(connum)/LOG2);
	varRatio = logit_simsize/(1+logit_simsize);

	sigmaTotal = 1.0;
	sigmaNoise = sigmaTotal * pow(varRatio,0.5);
	sigmaEpsilon = sigmaTotal * 
		pow(exp(2.0*myrng->gammln(1.0-d)-myrng->gammln(1.0-2.0*d))/(2+2*d/(1-d)) * (1-varRatio),
		    0.5); 
	if (!myrng)
		myrng = new RNG();
	fARIMA = new FARIMA(myrng, arima->d, arima->N);
	init_dist();
}

void PackMimeOLPacketSizeRandomVariable::init_dist() {
	int n;

	if (pac_size_dist_file==NULL) {	
		size_dist_left = def_size_dist_left;
		size_dist_right = def_size_dist_right;
		size_prob = def_size_prob;
		n_size_dist = sizeof(def_size_dist_left)/sizeof(int);
	} else
		read_dist();
	
  
	/* cumsum_size_prob = cumsum(prob0, prob1, prob2, ..., prob(n-1))
	 * cumsum_size_prob[last] = 1 
	 */
	cumsum_size_prob = new double[n_size_dist];

	for(n=0; n<n_size_dist; n++){
		if(n==0) {
			cumsum_size_prob[0] = size_prob[0];
		} else {
			cumsum_size_prob[n] = cumsum_size_prob[n-1] + size_prob[n]; 
		}    
	}
}

void PackMimeOLPacketSizeRandomVariable::read_dist() {
	FILE *fp;
	double totalprob;
	int n;

	if((fp = fopen(pac_size_dist_file, "r")) == NULL) {
		fprintf(stderr, "Error: can't open %s\n", pac_size_dist_file);
		exit(1);
	} 

	n = 0;   /* line number - 1 */
	totalprob = 0;
	do {
		if(n == 0 ) {
			fscanf(fp, "%d\n", &n_size_dist);	
			size_dist_left = new int[n_size_dist];
			size_dist_right = new int[n_size_dist];
			size_prob = new double[n_size_dist];
		} else { 
			fscanf(fp, "%d %d %lf\n", &size_dist_left[n-1],
			       &size_dist_right[n-1], &size_prob[n-1]); 
			if(size_dist_right[n-1] < size_dist_left[n-1]) {
				fprintf(stderr, 
					"Error: size_dist_right must >= size_dist_left\n");
				exit(1);
			}
			if(n>1) {
				if(size_dist_left[n-1] != size_dist_right[n-2]+1){
					fprintf(stderr, 
						"Error: size_dist_left(n) must = size_dist_right(n-1)+1\n");
					exit(1);
				}
			}
			totalprob += size_prob[n-1];
		}
		n++;
	} while((n <= n_size_dist) && !feof(fp));
    
	if(totalprob>1){
		fprintf(stderr, "Error: sum of size_prob > 1, check!\n");
	} else {
		if(totalprob < 1-EPSILON) {
			fprintf(stderr, 
				"size_prob[%d] increase from %g to %g, so sum of prob. is 1\n",
				n_size_dist-1, size_prob[n_size_dist-1],
				size_prob[n_size_dist-1]+(1-totalprob));
		}
		size_prob[n_size_dist-1] = size_prob[n_size_dist-1]+(1-totalprob);
	}
	if(n <= n_size_dist) {
		fprintf(stderr, "Error: wrong value of n_size_dist in %s!\n", 
			pac_size_dist_file);
		exit(1);
	}
	fclose(fp);  
}

PackMimeOLPacketSizeRandomVariable::~PackMimeOLPacketSizeRandomVariable() {
	delete fARIMA;

	if(n_size_dist > 0){
		delete size_dist_left;
		delete size_dist_right;
		delete size_prob;
		delete cumsum_size_prob;
	}
}

/**
 * generate packet size according to the marginal distribution 
 */
int PackMimeOLPacketSizeRandomVariable::transform(double p) {
	int i, size;
	double p1;

	i = 0;
	while(p > cumsum_size_prob[i])  
		i++; 
  
	p1 = cumsum_size_prob[i]-cumsum_size_prob[i-1];
	size = (size_dist_left[i]-1) + (int) (ceil((p-cumsum_size_prob[i-1])/p1 * 
	       (size_dist_right[i]-(size_dist_left[i]-1))));
	return size;
}

/** 
 * compute the mean of packet size distribution
 */
double PackMimeOLPacketSizeRandomVariable::avg() {
	int i;
	double m = 0;
	
	if (fARIMA == NULL)
		init_dist();

	for(i=0; i<n_size_dist; i++)
		m += size_prob[i] * (size_dist_left[i]+size_dist_right[i])/2;
	
	return m;
}

/**
 * generate packet size according to Long-Range Dependent plus Noise (LRDN) model 
 */
double PackMimeOLPacketSizeRandomVariable::value(void) {
	double yt;

	if (fARIMA == NULL)
		initialize();
	
	yt = fARIMA->Next();
	yt = yt * sigmaEpsilon + myrng->rnorm() * sigmaNoise;
	return ((double)transform(myrng->pnorm(yt)));
}

