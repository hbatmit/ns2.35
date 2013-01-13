#ifndef ns_messpass_h
#define ns_messpass_h

#include "agent.h"
#include "trafgen.h"
#include "packet.h"


#define SAMPLERATE 8000


class MessagePassingAgent : public Agent {
public:
	MessagePassingAgent();
	MessagePassingAgent(packet_t);
	virtual void sendmsg(int nbytes, const char *flags = 0)
	{
		sendmsg(nbytes, NULL, flags);
	}
	virtual void sendmsg(int nbytes, AppData* data, const char *flags = 0);
	virtual void recv(Packet* pkt, Handler*);
	virtual int command(int argc, const char*const* argv);
	virtual void sendto(int nbytes, const char* flags, ns_addr_t dst)
	{
		sendto(nbytes, NULL, flags, dst);
	}
	virtual void sendto(int nbytes, AppData *data, const char* flags, ns_addr_t dst);

protected:
	int seqno_;
};

#endif
