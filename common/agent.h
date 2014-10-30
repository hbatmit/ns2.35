/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1993-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/agent.h,v 1.38 2011/08/26 19:26:39 tom_henderson Exp $ (LBL)
 */

#ifndef ns_agent_h
#define ns_agent_h

#include "connector.h"
#include "packet.h"
#include "timer-handler.h"
#include "ns-process.h"
#include "app.h"
//#include "basetrace.h"
#define TIME_FORMAT "%.15g"
// TIME_FORMAT is in basetrace.h, but including that header leads to problems


#define TIMER_IDLE 0
#define TIMER_PENDING 1

/* 
 * Note that timers are now implemented using timer-handler.{cc,h}
 */

#define TRACEVAR_MAXVALUELENGTH 128

class Application;


// store old value of traced vars
// work only for TracedVarTcl
struct OldValue {
	TracedVar *var_;
	char val_[TRACEVAR_MAXVALUELENGTH];
	struct OldValue *next_;
};

class EventTrace;
class Agent : public Connector {
 public:
	Agent(packet_t pktType);
	virtual ~Agent();
	void recv(Packet*, Handler*);
	
	//added for edrop tracing - ratul
	void recvOnly(Packet *) {};

	/* These two functions aid Tmix one-way TCP agents */
	virtual int is_closed() {return 0;} 
	virtual void clr_closed() {}

	void send(Packet* p, Handler* h) { target_->recv(p, h); }
	virtual void timeout(int tno);

	virtual void sendmsg(int sz, AppData*, const char* flags = 0);
	virtual void send(int sz, AppData *data) { sendmsg(sz, data, 0); }
	virtual void sendto(int sz, AppData*, const char* flags,
			    nsaddr_t dst);
	virtual void sendto(int sz, AppData*, const char* flags,
			    ns_addr_t dst);

	virtual void sendmsg(int nbytes, const char *flags = 0);
	virtual void send(int nbytes) { sendmsg(nbytes); }
	virtual void sendto(int nbytes, const char* flags, nsaddr_t dst);
	virtual void sendto(int nbytes, const char* flags, ns_addr_t dst);
	virtual void connect(nsaddr_t dst);
	virtual void close();
	virtual void listen();
	virtual void attachApp(Application* app);
	virtual int& size() { return size_; }
	inline nsaddr_t& addr() { return here_.addr_; }
	inline nsaddr_t& port() { return here_.port_; }
	inline nsaddr_t& daddr() { return dst_.addr_; }
	inline nsaddr_t& dport() { return dst_.port_; }
	void set_pkttype(packet_t pkttype) { type_ = pkttype; }
	inline packet_t get_pkttype() { return type_; }

 protected:
	int command(int argc, const char*const* argv);
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);

	virtual void recvBytes(int bytes);
	virtual void idle();
	Packet* allocpkt() const;	// alloc + set up new pkt
	Packet* allocpkt(int) const;	// same, but w/data buffer
	void initpkt(Packet*) const;	// set up fields in a pkt

	ns_addr_t here_;		// address of this agent
	ns_addr_t dst_;			// destination address for pkt flow
	int size_;			// fixed packet size
	packet_t type_;			// type to place in packet header
	int fid_;			// for IPv6 flow id field
	int prio_;			// for IPv6 prio field
	int flags_;			// for experiments (see ip.h)
	int defttl_;			// default ttl for outgoing pkts

#ifdef notdef
	int seqno_;		/* current seqno */
	int class_;		/* class to place in packet header */
#endif

	static int uidcnt_;

	Tcl_Channel channel_;
	char *traceName_;		// name used in agent traces
	OldValue *oldValueList_; 

	Application *app_;		// ptr to application for callback

	virtual void trace(TracedVar *v);
	void deleteAgentTrace();
	void addAgentTrace(const char *name);
	void monitorAgentTrace();
	OldValue* lookupOldValue(TracedVar *v);
	void insertOldValue(TracedVar *v, const char *value);
	void dumpTracedVars();

	/* support for event-tracing */
        EventTrace *et_;
        virtual void trace_event(char *eventtype);

 private:
	void flushAVar(TracedVar *v);
};

#endif

