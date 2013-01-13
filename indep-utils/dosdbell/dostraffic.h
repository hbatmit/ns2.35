#ifdef FLOWLISTH
#else
#include "flowlist.h"
#define FLOWLISTH
#endif

#ifdef MODELSH
#else
#include "models.h"
#define MODELSH
#endif

struct DDOS_Traffic{
int NoEvents;
struct flow_event *Events;
};

void CBRTrafficGen(int NetworkDia, double H, int NoAttackers, int Master, double rate, struct DDOS_Traffic *);

void CBRTrafficInet(int NetworkDia, int NoAttackers, int Master, double rate, struct DDOS_Traffic *);
