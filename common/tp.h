#ifndef ns_tp_h
#define ns_tp_h

#include "agent.h"
#include "trafgen.h"
#include "packet.h"

#define SAMPLERATE 8000

class TPAgent : public Agent {
public:
  TPAgent();
  TPAgent(packet_t);
  
  virtual void sendto(int nbytes, unsigned int saddr, int sport, unsigned int daddr, int dport);
    
  virtual void recv(Packet* pkt, Handler*);
  virtual int command(int argc, const char*const* argv);
  
protected:
  int seqno_;
};

#endif
