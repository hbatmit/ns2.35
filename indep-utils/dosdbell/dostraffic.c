
#include "dostraffic.h"

void CBRTrafficGen(int NetworkDia, double H, int NoAttackers, int Master, double rate, struct DDOS_Traffic *traffic){
  struct flow_list *listroot = NULL;

  int no_hops;
  double flow,delay,total_bw;
  int i,cntr;

  for(i=0;i<NoAttackers;i++){
    no_hops = Power_law_no_hops(NetworkDia,H);
    if(Master==1)
      no_hops+=Power_law_no_hops(NetworkDia,H);
    delay = (double) no_hops;
    flow = rate;
    flow_list_insert(delay,flow,&listroot);
    cntr++;    
  }

   traffic->Events = get_all_events(listroot);
   traffic->NoEvents = get_no_events(listroot);
}


void CBRTrafficInet(int NetworkDia, int NoAttackers, int Master, double rate, struct DDOS_Traffic *traffic){
  struct flow_list *listroot = NULL;
  
  int no_hops;
  double flow,delay,total_bw;
  int i,cntr;
  
  for(i=0;i<NoAttackers;i++){
    no_hops = Inet_default_no_hops(NetworkDia);
    if(Master==1)
      no_hops+=Inet_default_no_hops(NetworkDia);
    delay = (double) no_hops;
    flow = rate;
    flow_list_insert(delay,flow,&listroot);
    cntr++;    
  }

   traffic->Events = get_all_events(listroot);
   traffic->NoEvents = get_no_events(listroot);

}
