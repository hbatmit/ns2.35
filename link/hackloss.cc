/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
#ifndef lint
static const char rcsid[] =
	"@(#) $Header: /cvsroot/nsnam/ns-2/link/hackloss.cc,v 1.6 2000/09/01 03:04:05 haoboy Exp $";
#endif

#include "connector.h"

#include "packet.h"
#include "queue.h"

class HackLossyLink : public Connector {
public:
	HackLossyLink() : down_(0), src_(0), dst_(0), fid_(0), ctr_(0), nth_(0){
	}
protected:
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h);
	NsObject* down_;
	int src_, dst_, fid_;
	int ctr_, nth_;
};

static class HackLossyLinkClass : public TclClass {
public:
	HackLossyLinkClass() : TclClass("HackLossyLink") {}
	TclObject* create(int, const char*const*) {
		return (new HackLossyLink);
	}
} class_dynamic_link;


int HackLossyLink::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "down-target") == 0) {
		NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
		if (p == 0) {
			Tcl::instance().resultf("no object %s", argv[2]);
			return TCL_ERROR;
		}
		down_ = p;
		return TCL_OK;
	}
	if (strcmp(argv[1], "show-params") == 0) {
		Tcl::instance().resultf("src_ = %d, dst_ = %d, fid_ = %d, nth_ = %d",
					src_, dst_, fid_, nth_);
		return TCL_OK;
	}
	if (strcmp(argv[1], "set-params") == 0) {
		src_ = atoi(argv[2]);
		dst_ = atoi(argv[3]);
		fid_ = atoi(argv[4]);
		return TCL_OK;
	}
	if (strcmp(argv[1], "nth") == 0) {
		nth_ = atoi(argv[2]);
		return TCL_OK;
	}
	return Connector::command(argc, argv);
}

void HackLossyLink::recv(Packet* p, Handler* h)
{
	hdr_ip* iph = hdr_ip::access(p);
	if (nth_ && (iph->flowid() == fid_) &&
	    (iph->saddr() == src_) && (iph->daddr() == dst_) &&
	    ((++ctr_ % nth_) == 0))
		down_->recv(p);	// XXX  Why no handler?
	else
		target_->recv(p, h);
}
