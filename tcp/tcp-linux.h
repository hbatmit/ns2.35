/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 *      This product includes software developed by the Network Research
 *      Group at Lawrence Berkeley National Laboratory.
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
 */

/* 
 * TCP-Linux module for NS2 
 *
 * May 2006
 *
 * Author: Xiaoliang (David) Wei  (DavidWei@acm.org)
 *
 * NetLab, the California Institute of Technology 
 * http://netlab.caltech.edu
 *
 * Module: tcp-linux.h
 *      This is the header file for TCP-Linux in NS-2.
 *
 *
 * See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
 *
 */


#ifndef ns_tcp_linux_h
#define ns_tcp_linux_h

#include "tcp.h"
#include "scoreboard1.h"
#include "linux/ns-linux-util.h"


#define DEBUG_LEVEL 0
#define DEBUG(level, ...) if (DEBUG_LEVEL>=level) printf(__VA_ARGS__);

/* A list to store the parameters */
class ParamList {
private:
	struct paramNode {
		int* addr;
		int value;
		int default_value;
		struct paramNode* next;
	};
	struct paramNode* head;
public:
	ParamList():head(NULL) {};
	~ParamList();
	/** Add this parameter to the list and set the value */
	void set_param(int* address, int value);
	bool get_param(int* address, int* valuep);

	/** Refresh all the values in the list */
	void refresh_default();
	
	/** Restore the */ 
	void restore_default();
	void load_local();
};

//This class provide C++ interface to access the Linux parameters for specific congestion control algorithm
/* The manager of Linux parameters for each TCP */
class LinuxParamManager {
private:
	ParamList localValues;
	static struct cc_list* find_cc_by_proto(const char* proto);
	static struct cc_param_list* find_param_by_proto_name(const char* proto, const char* name);
public:
	LinuxParamManager(){};
	static bool set_default_param(const char* proto, const char* param, const int value);
	static bool get_default_param(const char* proto, const char* param, int* valuep);
	static bool query_param(const char* proto);
	bool set_param(const char* proto, const char* param, const int value);
	bool get_param(const char* proto, const char* param, int* valuep);
	void load_local() {localValues.load_local();};
	void restore_default() {localValues.restore_default();};
};


/* TCP Linux */
class LinuxTcpAgent : public TcpAgent {
private:	
	LinuxParamManager paramManager;
public:
	LinuxTcpAgent();
	virtual ~LinuxTcpAgent();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
	virtual int window();
	virtual double windowd();
	void oldack (Packet* pkt);
	int maxsack (Packet* pkt); 
	void plot();
	void reset();
	virtual void send_much(int force, int reason, int maxburst = 0);
	virtual int packets_in_flight();
	virtual int command(int argc, const char*const* argv);

protected:
	ScoreBoard1 *scb_;
	struct tcp_sock linux_;		// Main data structure of a Linux TCP flow
	bool initialized_;		// a flag to record if a congestion control algorithm is initialized or not
					// ca_ops->init shall be run the first time an acknowledgment is processed (at least one RTT sample recorded).
	TracedInt next_pkts_in_flight_;	//the # of packets in flight allowed, if we need rate halving


	virtual bool is_congestion();	// whether the network is congested?

	void rtt_update(double tao, unsigned long pkt_seq_no=0);		//rewrite the tcp.cc functions

	unsigned char ack_processing(Packet* pkt, unsigned char flag);		// process the ack: sequence#
	void time_processing(Packet* pkt, unsigned char flag,s32* seq_urtt_p);	// process the ack for timestamp, timer
										//     these two processing functions replace 
										// the combination of newack() and oldack()



	void touch_cwnd();				// called whenever cwnd_ is changed, mark linux_.snd_cwnd_stamp

	void enter_loss();
//	void enter_frto();		We don't have FRTO yet
//	void enter_frto_loss();		We don't hvae FRTO yet
	void tcp_fastretrans_alert(unsigned char flag);
	void tcp_moderate_cwnd();

	void load_to_linux();				// the variables that shall be loaded to Linux every acks
	void load_to_linux_once();			// the variables that shall be loaded to Linux at boot or reset 

	void save_from_linux();				// the variables that shall be saved from Linux every ack

	char install_congestion_control(const char* name);
	void remove_congestion_control();

	inline void tcp_set_ca_state(const u8 ca_state) {
		if ((linux_.icsk_ca_ops)&&(linux_.icsk_ca_ops->set_state))
			linux_.icsk_ca_ops->set_state(&linux_, ca_state);
		//printf("%lf: %d State: %d->%d cwnd:%d ssthresh:%d\n",Scheduler::instance().clock(), this, linux_.icsk_ca_state, ca_state, linux_.snd_cwnd, linux_.snd_ssthresh);
		linux_.icsk_ca_state = ca_state;

	};
	inline void tcp_ca_event(const enum tcp_ca_event event) {
		if ((linux_.icsk_ca_ops)&&(linux_.icsk_ca_ops->cwnd_event))
			linux_.icsk_ca_ops->cwnd_event(&linux_, event);
		//printf("%lf: %d Event: %d\n",Scheduler::instance().clock(), this, event);
	};
};

class CongestionControlManager
{

public:
	CongestionControlManager();
//	int Register(struct tcp_congestion_ops* new_ops);
	struct tcp_congestion_ops* get_ops(const char* name);
	void dump();
	void scan();
private:
	int num_;
	struct tcp_congestion_ops** ops_list;
};
extern CongestionControlManager cong_ops_manager;



#endif
