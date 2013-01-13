#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <iostream.h>

// Integration of Ashish's RED and asim
#define _RED_ROUTER_MAIN_
#include "asim.h"

#define sp " " 

// Optimization
#include<vector>
using namespace std;

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
	vector<int> theflows; // The flows through this link
	double scaled_lambda;
	double unscaled_lambda;
	
	double utput; // unscaled tput 
	double uc; // unscaled capacity
	
	// RED
	RedRouter * redrouter;
	
}link_stats;


class asim {

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

		if(rho==0)
			return 0;

		double t;
		// M/M/1/K
		t=((1-rho)*pow(rho,k))/(1-pow(rho,K+1));
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
				// We know how many links it will use
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
				
				
				// For cbr 
				// Treat almost like a short flow!
				
				if (t && !strcasecmp(t,"cbr")) {
					// There are short flows on this route.
					flows[i].is_sflow = 2;
					
					// read the rate
					t = strtok(NULL," \t");
					assert(t);
					double  tmp = atof(t);
					flows[i].slambda = tmp;
					flows[i].snopkts = 1;
				}      
				
				
				// Now, let us put the flows in persective
				// Insert the flow id trhough all the links
				int l_;
				for(int j=0;j<nAdj[i];j++){
					l_ = Adj[i][j];
					(links[l_].theflows).push_back(i);
					links[l_].nflows++;
					if(flows[i].is_sflow){
						links[l_].lambda+=flows[i].slambda*flows[i].snopkts;
					}
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
				
				links[i].nflows = 0; // init the num of flows
				
				continue;
				
			}
			
			assert(0);
		}
		
		// Check whether everything is all right 
		assert (nConnections > 0);
		assert (nLinks > 0);
		for (int i=0; i<nConnections; ++i)
			assert(nAdj[i] > 0);
		
  
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
	
		double t;
		if(qlength<minth)
			return 0;
		if(qlength>maxth)
			return 1;
		return pmin + (qlength-minth)/(pmax-pmin);
	
	}


	void  CalcLinkStats(int flag = 0){
	
		// flag = 1 means enable RED
	
		// Calculate Link delays ... basically queuing delays
	
		for(int i=0; i<nLinks; i++){
		
			double rho = links[i].lambda/links[i].mu;
			double qlength = Lq(rho,links[i].buffer);
		
			links[i].qdelay = qlength/links[i].mu; 
			links[i].drop = Pk(rho,links[i].buffer,links[i].buffer);
			// cout << "Link " << i << " has drop prob = " << links[i].drop << endl;

			// Special code for RED gateways
			if(flag){
				if(links[i].red){
					double minth, maxth, pmin, pmax, delay,p;
					minth = links[i].minth;
					maxth = links[i].maxth;
					pmin = links[i].pmin;
					pmax = links[i].pmax;

					// The RED approx.
					p=(links[i].redrouter)->ComputeProbability(rho, delay);
					links[i].drop = p;
					qlength = Lq(rho*(1-p), links[i].buffer);
					links[i].qdelay = delay;
				}
			}
//			cout << i << sp << "rho = " << rho << " delay = " << links[i].qdelay << " and drop = " << links[i].drop << endl;
		}
	
	}

	void CalcPerFlowStats(){
 
		for(int i=0; i<nConnections; i++){
			double d = 0, p = 1 ;
			// Calculate drops and delays
			for(int j=0;j<nAdj[i];j++){
				d += 2*links[Adj[i][j]].prop + links[Adj[i][j]].qdelay;
				p *= 1-links[Adj[i][j]].drop;
			}
			p = 1-p;
			//cout << "Flow " << i << " has drop prob = " << p << endl; 
		
			flows[i].no = nAdj[i];
			flows[i].delay = d;
			flows[i].drop = p;
			flows[i].t = flows[i].p_tput;    
		
			// p is the end2end drop prob
			// If its normal flow, calculate Padhye's stuff
			// If its short flow, use our approximations
			// Nothing more
		
		
			if(flows[i].is_sflow==1){
				// If k flows come and each each flow has n packets to 
				// send then 
				double t = (flows[i].slambda*flows[i].snopkts);
				flows[i].p_tput = t/(1-p);
			}
			else if(flows[i].is_sflow==2){
				// For CBR, dont divide by 1-p unlike short flows.
				// If rate is x and prob is p, net goodput is x(1-p)
				flows[i].p_tput = flows[i].slambda*(1-p);
				// cout << "cbr stuff - tput = " << flows[i].p_tput << endl;
			}
			else{
				// regular bulk TCP connections, Padhye et. al.
				if(!p){
				// cout << "Oops, something wrong";
				}
				flows[i].p_tput = padhye(d,p);
			}
		
			// cout << "connection " << sp << i << sp << d << sp << p; 
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
	
		for(int i=0;i<nLinks;i++){
			printf("l %d qdel %.5lf drop %.5lf lam %.3lf\n", i+1, links[i].qdelay, links[i].drop,links[i].lambda);
		}
	
		for(int i=0; i<nConnections; i++){
			printf("c %d gput %.5lf drop %.5lf e2edel %.5lf\n", i+1,
			       flows[i].p_tput,
			       flows[i].drop,
			       flows[i].delay);
		}
	
	}

	void UpdateHelper(int flag=0){

		// if flag = 1 then update only when link is unscaled as of now
		// if flag = 0 then do the usual update 

		for(int i=0; i<nLinks; i++){
			links[i].tlambda=0;
		}

		for(int i=0; i<nConnections; i++){
			if(!flag || !flows[i].scaled) 
				for(int j=0;j<nAdj[i];j++){
					if(flows[i].is_sflow==2){
						// cbr flow
						links[Adj[i][j]].tlambda += flows[i].slambda*(1-links[Adj[i][j]].drop);
						//cout << "cbr flow " << i << " adding " << flows[i].slambda*(1-links[Adj[i][j]].drop)
						//    << " to link " << j << " tlam = " << links[Adj[i][j]].tlambda << endl;
					}
					else
						links[Adj[i][j]].tlambda += flows[i].p_tput;
				}
			//    cout << flows[i].p_tput << "\n";
		}
		
	}


	void Update(int niter){

		UpdateHelper();
		
		for(int i=0; i<nLinks; i++){
			links[i].plambda = links[i].lambda;
			double t;
			double tk=links[i].mu*(1.05)+5;
		
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


	int allscaled(){

		//cout << nConnections;

		for(int i=0; i<nConnections; i++)
			if(!flows[i].is_sflow && !flows[i].scaled){
				//cout << "Connection " << i << " not scaled as yet\n";
				return 0;
			}

		cout << "All are scaled\n";

		return 1;

	}
  



	void newupdate(int niter){

		// 1st init all unscaled tputs and cap

		for (int i=0;i<nLinks;i++){
			links[i].uc = links[i].mu*(1.05);
			links[i].utput = 0;
		}


		// calc all the unscaled tputs and C set all short flows 
		// to be scaled already 
		for(int i=0; i<nConnections; i++){
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

		//for(int i =0; i<nLinks; i++ ){
			//cout << i << sp << links[i].uc << sp << links[i].utput << endl;
		//}

		double maxgamma; // most congested link
		int bneck;
		double t;

		bneck = -1;
		maxgamma = 0;
		for(int i=0; i<nLinks; i++){
			if(links[i].uc){
				t=links[i].utput/links[i].uc;
				if(t > maxgamma){
					bneck = i;
					maxgamma = t;
				}
			}
		}    

		while(bneck+1){

			//cout << "bneck = " << bneck << sp << links[bneck].uc << sp << links[bneck].utput << sp << maxgamma << sp << links[bneck].nflows <<endl;
			for(int i=0; i<links[bneck].nflows; i++){
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
    
			//cout << links[bneck].uc << sp << links[bneck].utput << endl;
    
			links[bneck].uc = 0;

			bneck = -1;
			maxgamma = 0;
			for(int i=0; i<nLinks; i++){
				if(links[i].uc){
					t=links[i].utput/links[i].uc;
					if(t > maxgamma){
						bneck = i;
						maxgamma = t;
					}
				}
			} 

		}

		Update(niter);

	}


	asim(){
		//cout << "Reached here\n";
	}

};


int main(int argc, char **argv) {

	int niter = 0;

	// error if usage is wrong 
		   
	if (argc != 2) {
		fprintf(stderr,"Usage: %s  <InputFile>\n", argv[0]);
		exit(-1); 
	}
	

	asim sim;
	sim.GetInputs(argv[1]);
	//sim.PrintResults();
	//cout << "Read the input .... \n";

	for(int i=0; i<3; i++){
		sim.CalcLinkStats(1);
		//cout << "Calculated link delays ... \n";
		sim.CalcPerFlowStats();
		//cout << "Calculated per flow delays ... \n";
		//cout << " ------------------------------\n";
		sim.newupdate(niter);
		//sim.PrintResults();
		//cout << " ------------------------------\n"; 
	}
	sim.PrintResults();
}














