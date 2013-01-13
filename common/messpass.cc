#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/common/messpass.cc,v 1.4 2010/03/08 05:54:49 tom_henderson Exp $";
#endif

#include "messpass.h"
#include "rtp.h"
#include "random.h"
#include "address.h"
#include "ip.h"


static class MessagePassingAgentClass : public TclClass {
public:
	MessagePassingAgentClass() : TclClass("Agent/MessagePassing") {}
	TclObject* create(int, const char*const*) {
		return (new MessagePassingAgent());
	}
} class_message_passing_agent;

MessagePassingAgent::MessagePassingAgent() : Agent(PT_MESSAGE), seqno_(-1)
{
	bind("packetSize_", &size_);
}

MessagePassingAgent::MessagePassingAgent(packet_t type) : Agent(type)
{
	bind("packetSize_", &size_);
}

void MessagePassingAgent::sendmsg(int nbytes, AppData* data, const char*)
{
	Packet *p;

	if (nbytes == -1) {
		printf("Error:  sendmsg() for MessagePassingAgent should not be -1\n");
		return;
	}	

	// check packet size (we don't fragment packets)
	if (nbytes > size_) {
		printf("Error: packet greater than maximum MessagePassingAgent packet size\n");
		return;
	}

	double local_time = Scheduler::instance().clock();
	p = allocpkt();
	hdr_cmn::access(p)->size() = nbytes;
	hdr_rtp* rh = hdr_rtp::access(p);
	rh->flags() = 0;
	rh->seqno() = ++seqno_;
	hdr_cmn::access(p)->timestamp() = 
	    (u_int32_t)(SAMPLERATE*local_time);
	p->setdata(data);
	target_->recv(p);
	idle();
}


void MessagePassingAgent::sendto(int nbytes, AppData *data, const char*, ns_addr_t dst)
{
	Packet *p;

	if (nbytes == -1) {
		printf("Error: packet size for MessagePassingAgent should not be -1\n");
		return;
	}	

	// check packet size (we don't fragment packets)
	if (nbytes > size_) {
		printf("Error: packet greater than maximum MessagePassingAgent packet size\n");
		return;
	}

	double local_time = Scheduler::instance().clock();
	p = allocpkt();
	hdr_ip* iph = hdr_ip::access(p);
	iph->daddr() = dst.addr_;
	iph->dport() = dst.port_;
	hdr_cmn::access(p)->size() = nbytes;
	hdr_rtp* rh = hdr_rtp::access(p);
	rh->flags() = 0;
	rh->seqno() = ++seqno_;
	hdr_cmn::access(p)->timestamp() = 
	    (u_int32_t)(SAMPLERATE*local_time);
	p->setdata(data);
	target_->recv(p);
	idle();
}


void MessagePassingAgent::recv(Packet* pkt, Handler*)
{
	if (app_ ) {
		// If an application is attached, pass the data to the app
		hdr_cmn* h = hdr_cmn::access(pkt);
		app_->process_data(h->size(), pkt->userdata());
	} else if (pkt->userdata() && pkt->userdata()->type() == PACKET_DATA) {
		// otherwise if it's just PacketData, pass it to Tcl

		PacketData* data = (PacketData*)pkt->userdata();

		hdr_ip* iph = hdr_ip::access(pkt);
                Tcl& tcl = Tcl::instance();
		tcl.evalf("%s recv %d %d %d {%s}", name(),
			  iph->saddr(), iph->sport(),
			  hdr_cmn::access(pkt)->size(), data->data());
	} else {
		// It wasn't PacketData, or userdata() was NULL
		// so pass an empty string to Tcl


		hdr_ip* iph = hdr_ip::access(pkt);
                Tcl& tcl = Tcl::instance();
		tcl.evalf("%s recv %d %d %d {}", name(),
			  iph->saddr(), iph->sport(),
			  hdr_cmn::access(pkt)->size());
		
	}
	Packet::free(pkt);
}


int MessagePassingAgent::command(int argc, const char*const* argv)
{
	PacketData* data;
	ns_addr_t dst;

	if (argc == 4) {
		if (strcmp(argv[1], "send") == 0) {
			data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data);
			return TCL_OK;
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "sendmsg") == 0) {
			data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data, argv[4]);
			return TCL_OK;
		} else if (strcmp(argv[1], "sendto") == 0) {
			dst.addr_ = atoi(argv[3]);
			dst.port_ = atoi(argv[4]);
			if (dst.port_ == 0) dst.port_ = here_.port_;
			sendto(atoi(argv[2]), 0, dst);
			return TCL_OK;
		}
	} else if (argc == 6) {
		if (strcmp(argv[1], "sendto") == 0) {
			data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			dst.addr_ = atoi(argv[4]);
			dst.port_ = atoi(argv[5]);
			if (dst.port_ == 0) dst.port_ = here_.port_;
			sendto(atoi(argv[2]), data, 0, dst);
			return TCL_OK;
		} else if (strcmp(argv[1], "sendmsgto") == 0) {
			dst.addr_ = atoi(argv[3]);
			dst.port_ = atoi(argv[4]);
			if (dst.port_ == 0) dst.port_ = here_.port_;
			sendto(atoi(argv[2]), argv[5], dst);
			return TCL_OK;
		}
	} else if (argc == 7) {
		if (strcmp(argv[1], "sendmsgto") == 0) {
			data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			dst.addr_ = atoi(argv[4]);
			dst.port_ = atoi(argv[5]);
			if (dst.port_ == 0) dst.port_ = here_.port_;
			sendto(atoi(argv[2]), argv[6], dst);
			return TCL_OK;
		}}
	return (Agent::command(argc, argv));
}
