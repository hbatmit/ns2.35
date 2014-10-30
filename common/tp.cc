#include "tp.h"
#include "rtp.h"
#include "random.h"
#include "address.h"
#include "ip.h"


static class TPAgentClass : public TclClass {
public:
	TPAgentClass() : TclClass("Agent/TP") {}
	TclObject* create(int, const char*const*) {
		return (new TPAgent());
	}
} class_tp_agent;

TPAgent::TPAgent() : Agent(PT_MESSAGE), seqno_(-1) {
  size_ = 2000;
}

TPAgent::TPAgent(packet_t type) : Agent(type) {
  //bind("packetSize_", &size_);
}

void TPAgent::sendto(int nbytes, unsigned int saddr, int sport, unsigned int daddr, int dport) {
  double local_time = Scheduler::instance().clock();
  printf("send %f %u %d %u %d %d\n",
  	local_time, saddr, sport, daddr, dport, nbytes);

  if (nbytes == -1) {
    printf("Error: packet size for TPAgent should not be -1\n");
    return;
  }	
  
  // check packet size (we don't fragment packets)
  if (nbytes > size_) {
    printf("Error: packet greater than maximum TPAgent packet size\n");
    return;
  }
  
  Packet *p = allocpkt();
  // fill IP header
  hdr_ip* iph = hdr_ip::access(p);
  iph->saddr() = saddr;
  iph->sport() = sport;
  iph->daddr() = daddr;
  iph->dport() = dport;

  hdr_cmn::access(p)->size() = nbytes;
  hdr_rtp* rh = hdr_rtp::access(p);
  rh->flags() = 0;
  rh->seqno() = ++seqno_;
  
  hdr_cmn::access(p)->timestamp() = 
    (u_int32_t)(SAMPLERATE*local_time);

  target_->recv(p);
  idle();
}

void TPAgent::recv(Packet* pkt, Handler*) {
  if (app_ ) {
    // If an application is attached, pass the data to the app
    hdr_cmn* h = hdr_cmn::access(pkt);
    app_->process_data(h->size(), pkt->userdata());
  }

  hdr_ip* iph = hdr_ip::access(pkt);
  printf("recv %f %d %u %d %u %d %d\n",
         Scheduler::instance().clock(), addr(),
         iph->saddr(), iph->sport(),
         iph->daddr(), iph->dport(),
         hdr_cmn::access(pkt)->size());

  // recyle packets received
  Packet::free(pkt);
}

int TPAgent::command(int argc, const char*const* argv) {
  if (argc == 7) {
    if (strcmp(argv[1], "sendto") == 0) {
      sendto(atoi(argv[6]), atoi(argv[2]), atoi(argv[3]),
             atoi(argv[4]), atoi(argv[5]));
      return TCL_OK;
    }
  }
  
  return (Agent::command(argc, argv));
}
