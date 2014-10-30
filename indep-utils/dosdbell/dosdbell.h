#ifdef DDOSTRAFFICH
#else
#include "dostraffic.h"
#define DDOSTRAFFICH
#endif

struct TCP_Node_Info{
double delayTo;
double delayFrom;
double startTime;
};

struct TCP_Nodes{
  int NoNodes;
  struct TCP_Node_Info *TCPNode;
};

enum DDOSType {Master, NoMaster};

struct DDOS_Traffic_info{
  struct DDOS_Traffic DDOSTraffic;
  double MaxTime;
  enum DDOSType master; 
  int NoAttackers;
  double DDOSRate;
};

struct Topology{
  struct TCP_Nodes TCPNodes;
  struct DDOS_Traffic_info CBRTraffic;
};

