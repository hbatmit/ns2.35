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

#include "packmime_ranvar.h" 

#ifndef OL_RANVAR_H
#define OL_RANVAR_H

#define LOG2 0.6931471806

struct OL_arima_params {
	double d;
	int    N;
	int    pAR, qMA;

	double varRatioParam0, varRatioParam1, varRatioParam2; 
	// 1-2^(p0-p1*log2(rate)^p2)
};

struct OL_weibull_params {
	double shapeParam0, shapeParam1, shapeParam2; 
	// 1-2^(p0-p1*log2(rate)^p2)
};

/*:::::::::::::::::::: PackMimeOL Packet Size RanVar :::::::::::::::::::::::*/

class PackMimeOLPacketSizeRandomVariable : public RandomVariable {
  private:
	double varRatio, sigmaNoise, sigmaEpsilon;	
	int n_size_dist, *size_dist_left, *size_dist_right;
	double connum, bitrate, conbps, conpacrate;
	double *size_prob;
	double *cumsum_size_prob;
	char *pac_size_dist_file;
	FARIMA *fARIMA;
	RNG* myrng;
	virtual double value();
	virtual double avg();
	int transform(double p);  
	void initialize();
	void init_dist();
	void read_dist();
	void bind_vars();
	static int def_size_dist_left[];
	static int def_size_dist_right[];
	static double def_size_prob[];

  public:
	PackMimeOLPacketSizeRandomVariable();
	PackMimeOLPacketSizeRandomVariable(double bitrate); 
	PackMimeOLPacketSizeRandomVariable(double bitrate, RNG* rng);
	PackMimeOLPacketSizeRandomVariable(double bitrate, RNG* rng, 
					   const char *pac_size_dist_file);
	~PackMimeOLPacketSizeRandomVariable();	
};

/*::::::::::::: PackMimeOL Packet Interarrival RanVar :::::::::::::::::::::*/

class PackMimeOLPacketIARandomVariable : public RandomVariable {
  private:
	double varRatio, sigmaNoise, sigmaEpsilon;
	double shape, scale;
	double connum, bitrate, pacrate, conbps, conpacrate;
	FARIMA *fARIMA;
	RNG* myrng;
	RandomVariable *sizerng;
	virtual double value();
	virtual double avg();
	double transform(double p);  
	void initialize();
	void bind_vars();

  public:
	PackMimeOLPacketIARandomVariable();
	PackMimeOLPacketIARandomVariable(double bitrate_);  
	PackMimeOLPacketIARandomVariable(double bitrate_, RNG* rng_);  
	void setPktSizeRng(RandomVariable *sizerng_) {sizerng = sizerng_;};
	~PackMimeOLPacketIARandomVariable();
};


#endif /* OL_RANVAR_H */





