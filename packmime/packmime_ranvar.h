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

#ifndef ns_pm_ranvar_h
#define ns_pm_ranvar_h

#include "ranvar.h"

#define PACKMIME_XMIT_CLIENT 0
#define PACKMIME_XMIT_SERVER 1
#define PACKMIME_REQ_SIZE 0
#define PACKMIME_RSP_SIZE 1

struct arima_params {
  double d;
  int N;
  double varRatioParam0, varRatioParam1;
  int pAR, qMA;
};

/*:::::::::::::::::::::::::::::::: FX  :::::::::::::::::::::::::::::::::::::*/

// FX generator based on interpolation
class FX {
public:
  FX(const double* x, const double* y, int n);
  ~FX();
  double LinearInterpolate(double xnew);
private:
  double* x_, *y_;
  int nsteps_;
  double* slope_; // avoid real-time slope computation
};

/*:::::::::::::::::::::::::::: Fractional ARIMA  :::::::::::::::::::::::::::*/

class FARIMA {
public:
  FARIMA(RNG* rng, double d, int N, int pAR=0, int qMA=1);
  ~FARIMA();

  double Next();
private:
  RNG* rng_;
  int t_, N_, pAR_, qMA_, tmod_;
  double* AR_, *MA_, *x_, *y_, *phi_, d_;
  double NextLow();
};

/*:::::::::::::::::::::: PackMimeHTTP Transmission Delay RanVar ::::::::::::*/

class PackMimeHTTPXmitRandomVariable : public RandomVariable {

public:
  virtual double value();
  virtual double avg();
  PackMimeHTTPXmitRandomVariable();
  PackMimeHTTPXmitRandomVariable(double rate, int type);
  PackMimeHTTPXmitRandomVariable(double rate, int type, RNG* rng);
  ~PackMimeHTTPXmitRandomVariable();
  double* ratep() {return &rate_;}
  double rate() {return rate_;}
  void setrate(double r) {rate_ = r;}
  int* typep() {return &type_;}
  int type() {return type_;}
  void settype(int t) {type_ = t;}

private:
  void initialize();
  double rate_;
  int type_;
  double varRatio_, sigmaNoise_, sigmaEpsilon_;
  double const_;
  double mean_;
  FARIMA* fARIMA_;
  static const double SHORT_RTT_INVCDF_X[];
  static const double SHORT_RTT_INVCDF_Y[];
  static const double LONG_RTT_INVCDF_X[];
  static const double LONG_RTT_INVCDF_Y[];
  static struct arima_params rtt_arima_params[]; 
  static FX rtt_invcdf[2];
  static const double WEIBULL_SHAPE[2];
};

/*:::::::::::::::::::::: PackMimeHTTP Flow Arrival RanVar :::::::::::::::::*/

class PackMimeHTTPFlowArriveRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg();
  virtual double avg(int gap_type_);
  PackMimeHTTPFlowArriveRandomVariable();
  PackMimeHTTPFlowArriveRandomVariable(double rate);
  PackMimeHTTPFlowArriveRandomVariable(double rate, RNG* rng);
  ~PackMimeHTTPFlowArriveRandomVariable();
  double* ratep() {return &rate_;}
  double rate() {return rate_;}
  void setrate(double r) {rate_ = r;}

private:
  void initialize();
  double rate_;
  double const_;
  double mean_;
  double varRatio_, sigmaNoise_, sigmaEpsilon_, weibullShape_, weibullScale_;
  FARIMA *fARIMA_;
  static struct arima_params flowarrive_arima_params;
};

/*:::::::::::::::::::::: PackMimeHTTP File Size RanVar :::::::::::::::::::::*/

class PackMimeHTTPFileSizeRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg();
  PackMimeHTTPFileSizeRandomVariable();
  PackMimeHTTPFileSizeRandomVariable(double rate, int type);  
  PackMimeHTTPFileSizeRandomVariable(double rate, int type, RNG* rng);  
  ~PackMimeHTTPFileSizeRandomVariable(); 
  double* ratep() {return &rate_;}
  double rate() {return rate_;}
  void setrate(double r) {rate_ = r;}
  int* typep() {return &type_;}
  int type() {return type_;}
  void settype(int t) {type_ = t;}

private:
  void initialize();
  double rate_;
  int type_;
  double const_;
  double mean_;
  double varRatio_, sigmaNoise_, sigmaEpsilon_;
  FARIMA* fARIMA_;
  int runlen_, state_;
  double shape_[2], scale_[2];
  double loc_;
  double scale2_;
  int rfsize0(int state);
  int qfsize1(double p);

  /* fitted inverse cdf curves for file sizes  */
  static const double FSIZE0_INVCDF_A_X[];
  static const double FSIZE0_INVCDF_A_Y[];
  static const double FSIZE0_INVCDF_B_X[];
  static const double FSIZE0_INVCDF_B_Y[];
  static const double FSIZE1_INVCDF_A_X[];
  static const double FSIZE1_INVCDF_A_Y[];
  static const double FSIZE1_INVCDF_B_X[];
  static const double FSIZE1_INVCDF_B_Y[];
  static const double FSIZE1_PROB_A;
  static const double FSIZE1_D;
  static const double FSIZE1_VARRATIO_INTERCEPT;
  static const double FSIZE1_VARRATIO_SLOPE;

  /* server file size run length for downloaded and cache validation */
  static const double WEIBULLSCALECACHERUN;
  static const double WEIBULLSHAPECACHERUN_ASYMPTOE;
  static const double WEIBULLSHAPECACHERUN_PARA1;
  static const double WEIBULLSHAPECACHERUN_PARA2;
  static const double WEIBULLSHAPECACHERUN_PARA3;
  static const double WEIBULLSCALEDOWNLOADRUN;
  static const double WEIBULLSHAPEDOWNLOADRUN;
  static const double FSIZE0_PARA[];
  static struct arima_params filesize_arima_params;
  static const double* P;
  static FX fsize_invcdf[2][2]; 

  /* cut off of file size for cache validation */
  static const int FSIZE0_CACHE_CUTOFF; 
  static const int FSIZE0_STRETCH_THRES;
  /* mean of log2(fsize0) for non cache validation flows */
  static const double M_FSIZE0_NOTCACHE;
  /* variance of log2(fsize0) for non cache validation flows */ 
  static const double V_FSIZE0_NOTCACHE; 

  /* fsize0 for non-cache validation flow */
  static const double M_LOC;        /* mean of location */
  static const double V_LOC;        /* variance of location */
  static const double SHAPE_SCALE2; /* Gamma shape parameter of scale^2*/
  static const double RATE_SCALE2;  /* Gamma rate parameter of scale^2 */
  static const double V_ERROR;      /* variance of error */
};

/*:::::::::::::::::::::: PackMimeHTTP Persistent Rsp Size RanVar :::::::::::*/

class PackMimeHTTPPersistRspSizeRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg();
  PackMimeHTTPPersistRspSizeRandomVariable();
  PackMimeHTTPPersistRspSizeRandomVariable(RNG* rng);
  ~PackMimeHTTPPersistRspSizeRandomVariable(); 
  inline void reset_loc_scale() {loc_ = -1; scale_ = -1;}

  /* cut off of file size for cache validation */
  static const int FSIZE_CACHE_CUTOFF; 

private:
  double loc_;
  double scale_;

  /* fsize for non-cache validation flow */
  static const double M_LOC;        /* mean of location */
  static const double V_LOC;        /* variance of location */
  static const double SHAPE_SCALE2; /* Gamma shape parameter of scale^2*/
  static const double RATE_SCALE2;  /* Gamma rate parameter of scale^2 */
  static const double V_ERROR;      /* variance of error */
};

/*:::::::::::::::::::::: PackMimeHTTP Persistent RanVar ::::::::::::::::::::*/

class PackMimeHTTPPersistentRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg() {return 0;}
  PackMimeHTTPPersistentRandomVariable();
  PackMimeHTTPPersistentRandomVariable(double prob);  
  PackMimeHTTPPersistentRandomVariable(double prob, RNG* rng);  

  static const double P_PERSISTENT;   /* P(persistent=1) */

private:
  double probability_;    // probability that the connection is persistent

};

/*:::::::::::::::::::::: PackMimeHTTP NumPages RanVar ::::::::::::::::::::::*/

class PackMimeHTTPNumPagesRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg() {return 0;}
  PackMimeHTTPNumPagesRandomVariable();
  PackMimeHTTPNumPagesRandomVariable(double prob, double shape, double scale);
  PackMimeHTTPNumPagesRandomVariable(double prob, double shape, double scale, 
				     RNG* rng);  

  static const double P_1PAGE;       /* P(num reqs=1) */
  static const double SHAPE_NPAGE;   /* shape param for (#page reqs-1)*/
  static const double SCALE_NPAGE;   /* scale param for (#page reqs-1) */

private:
  double probability_;  // probability that the connection has just one page
  double shape_;        // shape of Weibull for number of pages in connection
  double scale_;        // scale of Weibull for number of pages in connection
};

/*:::::::::::::::::::::: PackMimeHTTP SingleObj RanVar :::::::::::::::::::::*/

class PackMimeHTTPSingleObjRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg() {return 0;}
  PackMimeHTTPSingleObjRandomVariable();
  PackMimeHTTPSingleObjRandomVariable(double prob);
  PackMimeHTTPSingleObjRandomVariable(double prob, RNG* rng);  

  static const double P_1TRANSFER; /* P(#xfers=1 | #page req>=2) */

private:
  double probability_;    // probability that the num of objs is 1
};

/*:::::::::::::::::::::: PackMimeHTTP ObjsPerPage RanVar :::::::::::::::::::*/

class PackMimeHTTPObjsPerPageRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg() {return 0;}
  PackMimeHTTPObjsPerPageRandomVariable();
  PackMimeHTTPObjsPerPageRandomVariable(double shape, double scale);
  PackMimeHTTPObjsPerPageRandomVariable(double shape, double scale, RNG* rng);
  static const double SHAPE_NTRANSFER; /* shape param for (#xfers-1)*/
  static const double SCALE_NTRANSFER; /* scale param for (#xfers-1) 1.578 */

private:
  double shape_;          // shape of Weibull for number of objs in page
  double scale_;          // scale of Weibull for number of objs in page
};

/*:::::::::::::::::::::: PackMimeHTTP TimeBtwnPages RanVar :::::::::::::::::*/

class PackMimeHTTPTimeBtwnPagesRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg() {return 0;}
  PackMimeHTTPTimeBtwnPagesRandomVariable();
  PackMimeHTTPTimeBtwnPagesRandomVariable(RNG* rng);

  /* time gap between page requests */
  static const double M_LOC_B;        /* mean of location */
  static const double V_LOC_B;        /* variance of location */
  static const double SHAPE_SCALE2_B; /* Gamma shape param of scale^2*/
  static const double RATE_SCALE2_B;  /* Gamma rate param of scale^2 */
  static const double V_ERROR_B;      /* variance of error */

private:
  double loc_b_;
  double scale2_b_;
};

/*:::::::::::::::::::::: PackMimeHTTP TimeBtwnObjs RanVar ::::::::::::::::::*/

class PackMimeHTTPTimeBtwnObjsRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg() {return 0;}
  PackMimeHTTPTimeBtwnObjsRandomVariable();
  PackMimeHTTPTimeBtwnObjsRandomVariable(RNG* rng);

  /* time gap within page requests */
  static const double M_LOC_W;        /* mean of location */
  static const double V_LOC_W;        /* variance of location */
  static const double SHAPE_SCALE2_W; /* Gamma shape param of scale^2*/
  static const double RATE_SCALE2_W;  /* Gamma rate param of scale^2 */
  static const double V_ERROR_W;      /* variance of error */

private:
  double loc_w_;
  double scale2_w_;
};

/*:::::::::::::::::::::: PackMimeHTTP ServerDelay RanVar :::::::::::::::::::*/

class PackMimeHTTPServerDelayRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg() {return 0;}
  PackMimeHTTPServerDelayRandomVariable();
  PackMimeHTTPServerDelayRandomVariable(double shape, double scale);
  PackMimeHTTPServerDelayRandomVariable(double shape, double scale, RNG* rng);
  static const double SERVER_DELAY_SHAPE;
  static const double SERVER_DELAY_SCALE;
  static const double SERVER_DELAY_DIV;

private:
  double shape_; 
  double scale_; 
  double const_;
  double mean_;
};

#endif
