
/*
 * asim.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: asim.cc,v 1.13 2011/10/02 22:32:34 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <iostream>

#include "agent.h"

// Integration of Ashish's RED and asim
#define _RED_ROUTER_MAIN_
#include "asim.h"

#define sp " " 

typedef struct c{
  int no; // no of edges in the connection
  double delay; // total delay;
  double drop; // total drop prob
  double p_tput;
  double t;
  // The short flow stuff
  int is_sflow; // boolean to indicate whether there is a short flow
  double slambda; // The arrival rate of the connections
  int snopkts; // average of no of packets each short flow givies
  RedRouter * red;
  int scaled; // Whether this flow has been scaled or not
}flow_stats;

typedef struct n{
  int red; // flag to notify whether its a red queue or not
  double pmin, pmax, minth, maxth; // RED parameters
  double lambda; // Arrival rate - Packets per second
  double plambda; // Temp lambda value. previous lambda
  double tlambda;
  double mu; // Consumption rate - Packets per second
  double prop; // Propagation delay of the link
  double qdelay; // Store the queuing delay for each link
  int    buffer; // Total buffer
  double drop; // probability of drop
  int nflows; // Number of flows through this link
  int *theflows; // The flows through this link
  double scaled_lambda;
  double unscaled_lambda;

  double utput; // unscaled tput 
  double uc; // unscaled capacity

  // For ashish
  RedRouter * redrouter;

}link_stats;


class asim : public NsObject{

public:

  // data structures 
  int nConnections; // Number of connections
  int K, MaxHops; // 
  int nLinks; // Number of links 
  int **Adj; // Stores the edge list of each connection
  int *nAdj; // Stores the no of edges per connection
  
  link_stats* links;
  flow_stats* flows;
  
  double min(double x, double y){
    return (x<y)?x:y;
  }

  double padhye(double rtt, double p){
    
    double rto = 1;
    double t=1;
    t = rtt*sqrt(2*p/3)+rto*min(1,(3*sqrt(3*p/8)))*p*(1+32*p*p);
    return min(20/rtt,1/t);
    
  }
  
  double Po(double rho, int K){
    
    if(rho==1)
      return 1.0/(K+1);
    
    double t;
    t=(1.0*(1-rho))/(1.0-pow(rho,K));
    return t;
    
  }

  double Pk(double rho, int K, int k){
    
    if(rho==1)
      return 1.0/(K+1);
    
    double t;
    t=(1-rho)*pow(rho,k);
    t/=1-pow(rho,K+1);
    return t;
  }  

  double Lq(double rho, int K){

    double t1,t2;
    
    if(rho==1){
      return (1.0*K*(K-1))/(2.0*(K+1));
    }
    
    t1=rho*1.0/(1-rho);
    t2=rho*1.0/(1-pow(rho,K+1));
    t2*=K*pow(rho,K)+1;
    return (t1-t2)/2;
    
  }
  
  int command (int , const char*const* argv){

    if (strcmp(argv[1], "run") == 0) {
      int niter=0;
      for(int i=0; i<20; i++){
	CalcLinkDelays(1);
	CalcPerFlowDelays();
	newupdate(niter);
      }
      //PrintResults();  
      return (TCL_OK);
    }
    
    if (strcmp(argv[1], "readinput") == 0) {
      GetInputs((char*)argv[2]);
      //cout << "All inputs properly obtained from " << argv[2] <<endl ; 
      return (TCL_OK);
    }

    if (strcmp(argv[1], "get-link-drop") == 0) {
      cout << "Hi";
      Tcl& tcl = Tcl::instance();
      tcl.resultf("%lf",get_link_drop(atoi(argv[2])));
      return (TCL_OK);
    }

    if (strcmp(argv[1], "get-link-delay") == 0) {
      Tcl& tcl = Tcl::instance();
      tcl.resultf("%lf",get_link_delay(atoi(argv[2])));
      return (TCL_OK);
    }  

    if (strcmp(argv[1], "get-link-tput") == 0) {
      Tcl& tcl = Tcl::instance();
      tcl.resultf("%lf",get_link_tput(atoi(argv[2])));
      return (TCL_OK);
    }  

    if (strcmp(argv[1], "get-flow-tput") == 0) {
      Tcl& tcl = Tcl::instance();
      tcl.resultf("%lf",get_flow_tput(atoi(argv[2])));
      return (TCL_OK);
    }

    if (strcmp(argv[1], "get-flow-delay") == 0) {
      Tcl& tcl = Tcl::instance();
      tcl.resultf("%lf",get_flow_delay(atoi(argv[2])));
      return (TCL_OK);
    }      

    if (strcmp(argv[1], "get-flow-drop") == 0) {
      Tcl& tcl = Tcl::instance();
      tcl.resultf("%lf",get_flow_drop(atoi(argv[2])));
      return (TCL_OK);
    }
	return 0;
  }
  
  double get_link_drop(int x){
    assert(x<nLinks);
    return links[x].drop;
  }

  double get_link_delay(int x){
    assert(x<nLinks);
    return links[x].qdelay + links[x].prop ;
  }

  double get_link_qdelay(int x){
    assert(x<nLinks);
    return links[x].qdelay;
  }

  double get_link_pdelay(int x){
    assert(x<nLinks);
    return links[x].prop;
  }

  double get_link_tput(int x){
    assert(x<nLinks);
    return links[x].lambda;
  }

  double get_flow_delay(int x){
    assert(x<nConnections);
    return flows[x].delay;
  }

  double get_flow_tput(int x){
    assert(x<nConnections);
    return flows[x].p_tput;
  }

  double get_flow_drop(int x){
    assert(x<nConnections);
    return flows[x].drop;
  }

  void GetInputs(char *argv) {
    
    // error if usage is wrong 
    /*    
    if (argc != 2) {
      fprintf(stderr,"Usage: %s  <InputFile>\n", argv[0]);
      exit(-1); 
      }*/
    
  // No error 
  MaxHops = 0;
  // K = atoi(argv[1]);
  // assert(K >= 1);


  // Init links and connections 
  nConnections = 0;
  nLinks = 0;

  // Start the reading process
  FILE *f;
  f = fopen(argv,"r");
  assert(f);

  char s[256];
  while (fgets(s, 255, f)) {

    // Read a token 
    char *t;
    t = strtok(s, " \t\n");

    // Ignore comments 
    if (!t || !t[0] || (t[0] == '#') || !strncasecmp(t, "comment", 6))
      continue;
    
    // Define the number of connections
    if (!strcasecmp(t,"n")) {
      t = strtok(NULL," \t");
      assert(t);
      nConnections = atoi(t);
      assert(nConnections > 0);
      assert(nConnections >= 0);
      nAdj = new int[nConnections];
      Adj = new int*[nConnections];
      flows = new flow_stats[nConnections];
      for (int i=0; i<nConnections; ++i)
	nAdj[i] = -1;
      continue;
    }

    // Define the number of links
    else if (!strcasecmp(t,"m")) {

      t = strtok(NULL," \t");
      assert(t);
      // #of links defined
      nLinks = atoi(t);
      assert(nLinks > 0);
      // Allocate space for sotring lambdas and mus
      links = new link_stats[nLinks];
      continue;
    }

    // Enter each route 
    else if (!strcasecmp(t,"route")) {

      assert (nConnections > 0);
      assert (nLinks > 0);
      t = strtok(NULL," \t");
      assert(t);
      int i = atoi(t);
      assert(i > 0 && i<= nConnections);
      i--;
 
     // We dunno whether this will be short flow specs
      flows[i].is_sflow = 0; // Lets assume its a normal flow
      flows[i].drop = 0; // Assume ideal case to start off
      flows[i].scaled = 0; // Not scaled as yet

      t = strtok(NULL," \t");
      assert(t);
      nAdj[i] = atoi(t);
      assert(nAdj[i] > 0 && nAdj[i] <= nLinks);
      Adj[i] = new int[nAdj[i]];
      for (int j=0; j<nAdj[i]; ++j) {
	t = strtok(NULL," \t");
	assert(t);
	int l = atoi(t);
	assert(l > 0 && l <= nLinks);
	l--;
	Adj[i][j] = l;
      }

      if (MaxHops < nAdj[i]) MaxHops = nAdj[i];

      
      t = strtok(NULL," \t");
      // assert(t);
    
      // Short flows stuff 

      if (t && !strcasecmp(t,"sh")) {
	// There are short flows on this route.
	flows[i].is_sflow = 1;
      
	// read the slambda
	t = strtok(NULL," \t");
	assert(t);
	double  tmp = atof(t);
	flows[i].slambda = tmp;

	// read the snopkts
	t = strtok(NULL," \t");
	assert(t);
	int  tmpi = atoi(t);
	flows[i].snopkts = tmpi;
      }
      
      continue;
    }

    else if(!strcasecmp(t,"link")){

      assert (nLinks > 0);

      // Get the link number
      t = strtok(NULL," \t");
      assert(t);
      int i = atoi(t);
      assert(i > 0 && i<= nLinks);
      i--;

      // Get the prop delay
      t = strtok(NULL," \t");
      assert(t);
      double p = atof(t);
      assert(p>=0); 
      links[i].prop = p;

      // Get the lambda for this link
      t = strtok(NULL," \t");
      assert(t);
      p = atof(t);
      assert(p>=0);
      links[i].lambda = 0;
      links[i].tlambda = p;
      links[i].plambda = p;

      // Get the mu for this link
      t = strtok(NULL," \t");
      assert(t);
      p = atof(t);
      assert(p>=0);
      links[i].mu = p;

      // Get the buffer for this link
      t = strtok(NULL," \t");
      assert(t);
      int t1 = atoi(t);
      assert(t1>0);
      links[i].buffer = t1;

      // Check for RED Q or not
      t = strtok(NULL," \t");
      if(t && !strcasecmp(t,"red")){

	// must be a red queue
	// input red parameters
	// all parameters between 0 and 1
	links[i].red=1;
	// get minth
	t = strtok(NULL," \t");
	double dt = atof(t); 
	//assert(dt>=0 && dt<=1);
	links[i].minth=dt;

	// get pmin
	t = strtok(NULL," \t");
	dt = atof(t); 
	//assert(dt>=0 && dt<=1);
	links[i].pmin=dt;

	// get maxth
	t = strtok(NULL," \t");
	dt = atof(t); 
	//assert(dt>=0 && dt<=1);
	links[i].maxth=dt;

	// get pmax
	t = strtok(NULL," \t");
	dt = atof(t); 
	//assert(dt>=0 && dt<=1);
	links[i].pmax=dt;

	// Invoke Ashish's RED module ... ignore pmin .....

	links[i].redrouter = new RedRouter((int)links[i].minth, 
					   (int)links[i].maxth,
					   links[i].pmax);
	assert(links[i].red);

      }
      else{
	links[i].red=0;
      }
	
      continue;

    }

    assert(0);
  }

  // Check whether everything is all right 
  assert (nConnections > 0);
  assert (nLinks > 0);
  int i;
  for (i=0; i<nConnections; ++i)
    assert(nAdj[i] > 0);

  
  // check all the edges and store all the connections that flow 
  // through a particular link
  
  for(i=0;i<nLinks;i++){

    //    cout << i << sp;
    int c=0; links[i].tlambda=0;

    for(int j=0;j<nConnections;j++){
      for(int k=0;k<nAdj[j];k++){
	if(Adj[j][k]==i){
	  c++;
	}
      }
    }
    links[i].nflows=c;
    //cout << c << sp;

    if(c){
      links[i].theflows = new int[c];
      c = 0;
      // Store teh flows
      for(int j=0;j<nConnections;j++){
	for(int k=0;k<nAdj[j];k++){
	  if(Adj[j][k]==i){
	    if(flows[j].is_sflow){
	      links[i].lambda+=flows[j].slambda*flows[j].snopkts;
	    }
	    links[i].theflows[c++]=j;
	    //	    cout << links[i].theflows[c-1] << sp;
	  }
	}
      }
      // cout << " slambda = " << links[i].lambda;
    }

    else links[i].theflows = 0; // no connection passing through this edge
    //cout << endl;



  }

  /*
  char c= getchar();

  for(int i=0;i<nConnections;i++){
    cout << "connection" << sp << i << sp << "-"; 
    for(int j=0;j<nAdj[i];j++){
      cout << sp << Adj[i][j];
    }
    cout << endl;
  }
  */
  
}



double redFn(double minth, double pmin,
	     double maxth, double pmax, double qlength){

  assert(qlength>=0 && qlength<=1);
  assert(pmax>=0 && pmax<=1);
  assert(pmin>=0 && pmin<=1);
  assert(minth>=0 && minth<=1);
  assert(maxth>=0 && maxth<=1);
  assert(maxth>=minth);
  assert(pmax>pmin);

  //Double t;
  if(qlength<minth)
    return 0;
  if(qlength>maxth)
    return 1;
  return pmin + (qlength-minth)/(pmax-pmin);

}


void  CalcLinkDelays(int flag = 0){

  // flag = 1 means enable RED

  // Calculate Link delays ... basically queuing delays

  for(int i=0; i<nLinks; i++){

    double rho = links[i].lambda/links[i].mu;
    double qlength = Lq(rho,links[i].buffer);

    links[i].qdelay = qlength/links[i].mu; 
    links[i].drop = Pk(rho,links[i].buffer,links[i].buffer);
//cout << "rho = " << rho << " drop = " << links[i].drop << endl;

    // Special code for RED gateways
    if(flag){
      if(links[i].red){
	double p, delay;
	/* Debo's RED approx
	double minth, maxth, pmin, pmax;
	minth = links[i].minth;
	maxth = links[i].maxth;
	pmin = links[i].pmin;
	pmax = links[i].pmax;
	links[i].drop = redFn(minth,pmin,maxth,pmax,qlength/links[i].buffer);
	qlength = (1-links[i].drop)*links[i].buffer;
	links[i].qdelay = qlength/links[i].mu; 
	*/
	
	// Ashish's RED approx.
	p=(links[i].redrouter)->ComputeProbability(rho, delay);
	links[i].drop = p;
	qlength = Lq(rho*(1-p), links[i].buffer);
	links[i].qdelay = delay;
      }
    }


    //cout << "delay = " << links[i].qdelay << " and drop = " << links[i].drop << endl;


  }

}

void CalcPerFlowDelays(){
  for(int i=0; i<nConnections; i++){
    double d = 0, p = 1 ;
    // Calculate drops and delays
    for(int j=0;j<nAdj[i];j++){
      d += 2*links[Adj[i][j]].prop + links[Adj[i][j]].qdelay;
      p *= 1-links[Adj[i][j]].drop;
    }
    p = 1-p;

    flows[i].no = nAdj[i];
    flows[i].delay = d;
    flows[i].drop = p;
    flows[i].t = flows[i].p_tput;    

    // p is the end2end drop prob
    // If its normal flow, calculate Padhye's stuff
    // If its short flow, use our approximations
    // Nothing more

    
    if(flows[i].is_sflow){
      // If k flows come and each each flow has n packets to 
      // send then 
      double t = (flows[i].slambda*flows[i].snopkts);
      flows[i].p_tput = t/(1-p);
    }
    else{
      // regular bulk TCP connections, Padhye et. al.
      if(!p){
	// cout << "Oops, something wrong";
      }
      flows[i].p_tput = padhye(d,p);
    }

    //    cout << "connection " << sp << i << sp << d << sp << p; 
    //cout << sp << flows[i].p_tput << endl;
   

  }
}

void PrintData(){
  for(int i=0;i<nLinks;i++){
    cout << i << sp << links[i].lambda << sp << links[i].mu;
    cout << sp << links[i].buffer << endl;
  }
}

void PrintResults(){
  int i;

  for(i=0;i<nLinks;i++){
    // cout << i << sp << links[i].qdelay << sp << links[i].drop;
    cout << sp << "Qdelay = " << links[i].prop << sp << links[i].lambda;
    cout << sp << links[i].drop << endl;
  }

  for(i=0; i<nConnections; i++){
    cout << i << sp << flows[i].delay << sp;
    cout << flows[i].drop << sp << flows[i].p_tput << sp;
    cout << sp <<  endl;
  }

}

void UpdateHelper(int flag=0){

  // if flag = 1 then update only when link is unscaled as of now
  // if flag = 0 then do the usual update 

  int i;
  for(i=0; i<nLinks; i++){
    links[i].tlambda=0;
  }

  for(i=0; i<nConnections; i++){
    if(!flag || !flows[i].scaled) 
      for(int j=0;j<nAdj[i];j++)
	links[Adj[i][j]].tlambda += flows[i].p_tput;
    //    cout << flows[i].p_tput << "\n";
  }

}


void Update(int niter){

  UpdateHelper();

  for(int i=0; i<nLinks; i++){
    links[i].plambda = links[i].lambda;
    double t;
    double tk=links[i].mu*(1.1);
    
    if(niter){
      if(links[i].tlambda>tk)
	//t = pow((sqrt(links[i].lambda)+sqrt(links[i].mu+5))/2,2);
	t = ((links[i].lambda)+tk)/2;
      // t = exp((log(links[i].lambda)+log(links[i].mu+5))/2);
      else
	//t = pow((sqrt(links[i].tlambda)+sqrt(links[i].lambda))/2,2);
	t= ((links[i].tlambda)+(links[i].lambda))/2;
      // t = exp((log(links[i].tlambda)+log(links[i].lambda))/2);
    }
    else t = links[i].tlambda;
    links[i].lambda = t; // Update the lambda ..........
  }

}  


void Update2(){

  UpdateHelper();

  for(int i=0; i<nLinks; i++){
    links[i].plambda = links[i].lambda;
    links[i].lambda = (links[i].lambda + links[i].tlambda)/2;
  }
}

int allscaled(){

  //cout << nConnections;

  for(int i=0; i<nConnections; i++)
    if(!flows[i].is_sflow && !flows[i].scaled){
      cout << "Connection " << i << " not scaled as yet\n";
      return 0;
    }

  //cout << "All are scaled\n";

  return 1;

}
  

void Update3(int flag = 0){

// flag = 1 means dont touch short flows in your scaling algo


  double maxtlambda = -1e7;
  int bneck = -1;
  int i;

  // 1st get set scaled var of all flows to 0
  for(i=0; i<nConnections; i++)
    flows[i].scaled = 0;

  // Calculate the tlambdas
  UpdateHelper();

  // Find out the link with the max throughput

  for(i=0; i<nLinks; i++){
    //cout << "after updatehelper link #" << i << " " << links[i].tlambda << "\n";
    if(links[i].tlambda>maxtlambda){
      bneck = i;
      maxtlambda = links[i].tlambda;
    }
  }

  cout << "bottleneck = " << bneck << sp << maxtlambda <<endl;

  double tk = links[bneck].mu*(1+links[bneck].drop)+5; 
  // We cant go above this tk ......

  while((maxtlambda > tk + 1) && ! allscaled()){

    cout << "Maxtlambda = " << maxtlambda << " bneck = " << bneck << endl;

    //    cout << "tk =  "<< tk << " maxlambda = " << maxtlambda << endl;

    // Now lets reduce this to tk
    assert(bneck>=0 && bneck<=nLinks);
    int i;
    for(i=0; i<links[bneck].nflows; i++){
 
     // For all the connections passing through this link
      int t = links[bneck].theflows[i]; // get a connection id
      // Now reduce its p_tput iff its not a short flow
      // For short flows we dont do scaling
      if(flag){
	if(!flows[t].is_sflow && !flows[t].scaled){
	  flows[t].p_tput *= (tk)/maxtlambda;
	  //cout << "Flow " << t << " getting scaled to  << " << flows[t].p_tput <<" \n";
	  flows[t].scaled = 1; // we have scaled this flow already
	}
      }
      else
	flows[t].p_tput *= (tk)/maxtlambda;
      flows[t].scaled = 1; // we have scaled this flow already 

    }

    for (i=0; i<nLinks;i++){
      cout << "Link " << i << " tlambda = " << links[i].tlambda << endl;
    }

    //Char x =getchar();

    // Recalculate the flows' stats
    UpdateHelper(0);

    // Find out the link with the max throughput
    bneck = -1;
    maxtlambda = -1e7;
    for(i=0; i<nLinks; i++){
      if(links[i].tlambda>maxtlambda){
	bneck = i;
	maxtlambda = links[i].tlambda;
      }
    }    


    tk = links[bneck].mu*(1+links[bneck].drop)+5;


  }

  Update(0);
  cout << "Out of the converge loop\n";
    for (i =0; i<nLinks;i++){
      cout << "Link " << i << " tlambda = " << links[i].tlambda << endl;
    }

}


void newupdate(int niter){

  int i;

  // 1st init all unscaled tputs and cap
  for (i=0;i<nLinks;i++){
    links[i].uc = links[i].mu*(1.05);
    links[i].utput = 0;
  }


  // calc all the unscaled tputs and C set all short flows 
  // to be scaled already 
  for(i=0; i<nConnections; i++){
    if(flows[i].is_sflow)
      flows[i].scaled = 1;
    else 
      flows[i].scaled = 0;
    for(int j=0;j<nAdj[i];j++){
      if(flows[i].is_sflow)
	links[Adj[i][j]].uc -= flows[i].p_tput;
      else
	links[Adj[i][j]].utput += flows[i].p_tput;
    }
  }

  //  for(i=0; i<nLinks; i++ ){
  //cout << i << sp << links[i].uc << sp << links[i].utput << endl;
  //}

  double maxgamma; // most congested link
  int bneck;
  double t;

  bneck = -1;
  maxgamma = 0;
  for(i=0; i<nLinks; i++){
    if(links[i].uc){
      t=links[i].utput/links[i].uc;
      if(t > maxgamma){
	bneck = i;
	maxgamma = t;
      }
    }
  }    

  //cout << bneck << endl;

  //char c= getchar();
  /*
  for(i=0;i<nConnections;i++){
    cout << "connection" << sp << i << sp << "-"; 
    for(int j=0;j<nAdj[i];j++){
      cout << sp << Adj[i][j];
    }
    cout << endl;
  }
  */
  // c= getchar();

  while(bneck+1){

    // cout << "bneck = " << bneck << sp << links[bneck].uc << sp << links[bneck].utput << sp << maxgamma << sp << links[bneck].nflows <<endl;

    for(i=0; i<links[bneck].nflows; i++){
     // For all the connections passing through this link
      int t = links[bneck].theflows[i]; // get a connection id
      //     cout << i<< sp << t << sp ;
      // Now reduce its p_tput iff its not a short flow
      // For short flows we dont do scaling
      if(!flows[t].is_sflow && !flows[t].scaled){
	flows[t].p_tput /= maxgamma;
	//cout << "Flow " << t << " getting scaled to  << " << flows[t].p_tput;
	flows[t].scaled = 1; // we have scaled this flow already
	for(int j=0;j<nAdj[t];j++){
	  // subtract this scaled throughout from all teh links that
	  // have this flow. 
	  links[Adj[t][j]].uc -= flows[t].p_tput;
	  links[Adj[t][j]].utput -= flows[t].p_tput*maxgamma;
	  // cout << sp << Adj[i][j];
	}
	//cout << endl;
      }
    }
    
    // cout << links[bneck].uc << sp << links[bneck].utput << endl;
    
    links[bneck].uc = 0;

    bneck = -1;
    maxgamma = 0;
    for(i=0; i<nLinks; i++){
      if(links[i].uc){
	t=links[i].utput/links[i].uc;
	if(t > maxgamma){
	  bneck = i;
	  maxgamma = t;
	}
      }
    } 
    /*
    c = getchar();

    for(i=0;i<nConnections;i++){
      cout << "connection" << sp << i << sp << "-"; 
      for(int j=0;j<nAdj[i];j++){
	cout << sp << Adj[i][j];
      }
      cout << endl;
      }*/
    
    // c=getchar();

  }

  Update(niter);

}


  asim(){
    // cout << "Reached here\n";
  }

  void recv(Packet *, Handler * = 0){}

};



static class AsimClass : public TclClass {
public:
  AsimClass(): TclClass("Asim"){ }
  TclObject * create(int, const char*const*) {
    return (new asim());
  }
} class_asim;

/*

int main(int argc, char **argv) {

  int niter = 0;

  asim sim;
  sim.GetInputs(argc, argv);
  //  PrintResults();
  cout << "Read the input .... \n";

  for(int i=0; i<10; i++){
    sim.CalcLinkDelays(1);
    cout << "Calculated link delays ... \n";
    //    PrintResults();

    sim.CalcPerFlowDelays();
    cout << "Calculated per flow delays ... \n";
    cout << " ------------------------------\n";
    sim.newupdate(niter);
    //sim.PrintResults();
    cout << " ------------------------------\n"; 
  }
  sim.PrintResults();
}


*/











