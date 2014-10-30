#include "models.h"

int Power_law_no_hops(double delta, double H){
  int hop;
  double ran;
  ran = drand48();
  hop = (int) ceil(delta*pow(ran,1/H) );
  return(hop);
}


int Inet_default_no_hops(int diameter){
  int i;
  double *table;
  double sum,ran;
  int hops=-1;
  switch(diameter){
  case 20 : { table = table20;break;}
  case 16 : { table = table16;break;} 
  case 15 : { table = table15;break;}
  case 12 : { table = table12;break;}
  default : { table = table20; diameter=20; break;}
  }
  ran = drand48();
  sum=0;
  for(i=1;i<=diameter;i++){
    sum+=table[i-1];
    if(ran<sum){
      hops=i;
      break;
    }
  }
  if(hops==-1)
    hops=diameter;
  return(hops);
}

double Binomial_aggregations(double mu, double p){
return(mu*ceil(log10(1-drand48())/log10(p)));
}
