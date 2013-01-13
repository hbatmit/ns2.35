/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (c) Intel Corporation 2001. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (c) 1997, 1998 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 */

/*
 *
 * Full-TCP : A two-way TCP very similar to the 4.4BSD version of Reno TCP.
 * This version also includes variants Tahoe, NewReno, and SACK.
 *
 * This code below has received a fairly major restructuring (Aug. 2001).
 * The ReassemblyQueue structure is now removed to a separate module and
 * entirely re-written.
 * Also, the SACK functionality has been re-written (almost) entirely.
 * -KF [kfall@intel.com]
 *
 * This code below was motivated in part by code contributed by
 * Kathie Nichols (nichols@baynetworks.com).  The code below is based primarily
 * on the 4.4BSD TCP implementation. -KF [kfall@ee.lbl.gov]
 *
 * Kathie Nichols and Van Jacobson have contributed significant bug fixes,
 * especially with respect to the the handling of sequence numbers during
 * connection establishment/clearin.  Additional fixes have followed
 * theirs.
 *
 * Fixes for gensack() and ReassemblyQueue::add() contributed by Richard 
 * Mortier <Richard.Mortier@cl.cam.ac.uk>
 *
 * Some warnings and comments:
 *	this version of TCP will not work correctly if the sequence number
 *	goes above 2147483648 due to sequence number wrap
 *
 *	this version of TCP by default sends data at the beginning of a
 *	connection in the "typical" way... That is,
 *		A   ------> SYN ------> B
 *		A   <----- SYN+ACK ---- B
 *		A   ------> ACK ------> B
 *		A   ------> data -----> B
 *
 *	there is no dynamic receiver's advertised window.   The advertised
 *	window is simulated by simply telling the sender a bound on the window
 *	size (wnd_).
 *
 *	in real TCP, a user process performing a read (via PRU_RCVD)
 *		calls tcp_output each time to (possibly) send a window
 *		update.  Here we don't have a user process, so we simulate
 *		a user process always ready to consume all the receive buffer
 *
 * Notes:
 *	wnd_, wnd_init_, cwnd_, ssthresh_ are in segment units
 *	sequence and ack numbers are in byte units
 *
 * Futures:
 *      there are different existing TCPs with respect to how
 *      ack's are handled on connection startup.  Some delay
 *      the ack for the first segment, which can cause connections
 *      to take longer to start up than if we be sure to ack it quickly.
 *
 *      some TCPs arrange for immediate ACK generation if the incoming segment
 *      contains the PUSH bit
 *
 *
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp-full.cc,v 1.130 2010/03/08 05:54:54 tom_henderson Exp $ (LBL)";
#endif

#include "ip.h"
#include "tcp-full.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#ifndef TRUE
#define	TRUE 	1
#endif

#ifndef FALSE
#define	FALSE 	0
#endif

/*
 * Tcl Linkage for the following:
 *	Agent/TCP/FullTcp, Agent/TCP/FullTcp/Tahoe,
 *	Agent/TCP/FullTcp/Newreno, Agent/TCP/FullTcp/Sack
 *
 * See tcl/lib/ns-default.tcl for init methods for
 *	Tahoe, Newreno, and Sack
 */

static class FullTcpClass : public TclClass { 
public:
	FullTcpClass() : TclClass("Agent/TCP/FullTcp") {}
	TclObject* create(int, const char*const*) { 
		return (new FullTcpAgent());
	}
} class_full;

static class TahoeFullTcpClass : public TclClass { 
public:
	TahoeFullTcpClass() : TclClass("Agent/TCP/FullTcp/Tahoe") {}
	TclObject* create(int, const char*const*) { 
		// ns-default sets reno_fastrecov_ to false
		return (new TahoeFullTcpAgent());
	}
} class_tahoe_full;

static class NewRenoFullTcpClass : public TclClass { 
public:
	NewRenoFullTcpClass() : TclClass("Agent/TCP/FullTcp/Newreno") {}
	TclObject* create(int, const char*const*) { 
		// ns-default sets open_cwnd_on_pack_ to false
		return (new NewRenoFullTcpAgent());
	}
} class_newreno_full;

static class SackFullTcpClass : public TclClass { 
public:
	SackFullTcpClass() : TclClass("Agent/TCP/FullTcp/Sack") {}
	TclObject* create(int, const char*const*) { 
		// ns-default sets reno_fastrecov_ to false
		// ns-default sets open_cwnd_on_pack_ to false
		return (new SackFullTcpAgent());
	}
} class_sack_full;

/*
 * Delayed-binding variable linkage
 */

void
FullTcpAgent::delay_bind_init_all()
{
        delay_bind_init_one("segsperack_");
        delay_bind_init_one("segsize_");
        delay_bind_init_one("tcprexmtthresh_");
        delay_bind_init_one("iss_");
        delay_bind_init_one("nodelay_");
        delay_bind_init_one("data_on_syn_");
        delay_bind_init_one("dupseg_fix_");
        delay_bind_init_one("dupack_reset_");
        delay_bind_init_one("close_on_empty_");
        delay_bind_init_one("signal_on_empty_");
        delay_bind_init_one("interval_");
        delay_bind_init_one("ts_option_size_");
        delay_bind_init_one("reno_fastrecov_");
        delay_bind_init_one("pipectrl_");
        delay_bind_init_one("open_cwnd_on_pack_");
        delay_bind_init_one("halfclose_");
        delay_bind_init_one("nopredict_");
        delay_bind_init_one("ecn_syn_");
        delay_bind_init_one("ecn_syn_wait_");
        delay_bind_init_one("debug_");
        delay_bind_init_one("spa_thresh_");

	TcpAgent::delay_bind_init_all();
       
      	reset();
}

int
FullTcpAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer)
{
        if (delay_bind(varName, localName, "segsperack_", &segs_per_ack_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "segsize_", &maxseg_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "tcprexmtthresh_", &tcprexmtthresh_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "iss_", &iss_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "spa_thresh_", &spa_thresh_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "nodelay_", &nodelay_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "data_on_syn_", &data_on_syn_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "dupseg_fix_", &dupseg_fix_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "dupack_reset_", &dupack_reset_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "close_on_empty_", &close_on_empty_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "signal_on_empty_", &signal_on_empty_, tracer)) return TCL_OK;
        if (delay_bind_time(varName, localName, "interval_", &delack_interval_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ts_option_size_", &ts_option_size_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "reno_fastrecov_", &reno_fastrecov_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "pipectrl_", &pipectrl_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "open_cwnd_on_pack_", &open_cwnd_on_pack_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "halfclose_", &halfclose_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "nopredict_", &nopredict_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "ecn_syn_", &ecn_syn_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ecn_syn_wait_", &ecn_syn_wait_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "debug_", &debug_, tracer)) return TCL_OK;

        return TcpAgent::delay_bind_dispatch(varName, localName, tracer);
}

void
SackFullTcpAgent::delay_bind_init_all()
{
        delay_bind_init_one("clear_on_timeout_");
        delay_bind_init_one("sack_rtx_cthresh_");
        delay_bind_init_one("sack_rtx_bthresh_");
        delay_bind_init_one("sack_block_size_");
        delay_bind_init_one("sack_option_size_");
        delay_bind_init_one("max_sack_blocks_");
        delay_bind_init_one("sack_rtx_threshmode_");
	FullTcpAgent::delay_bind_init_all();
}

int
SackFullTcpAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer)
{
        if (delay_bind_bool(varName, localName, "clear_on_timeout_", &clear_on_timeout_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "sack_rtx_cthresh_", &sack_rtx_cthresh_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "sack_rtx_bthresh_", &sack_rtx_bthresh_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "sack_rtx_threshmode_", &sack_rtx_threshmode_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "sack_block_size_", &sack_block_size_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "sack_option_size_", &sack_option_size_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "max_sack_blocks_", &max_sack_blocks_, tracer)) return TCL_OK;
        return FullTcpAgent::delay_bind_dispatch(varName, localName, tracer);
}

int
FullTcpAgent::command(int argc, const char*const* argv)
{
	// would like to have some "connect" primitive
	// here, but the problem is that we get called before
	// the simulation is running and we want to send a SYN.
	// Because no routing exists yet, this fails.
	// Instead, see code in advance().
	//
	// listen can happen any time because it just changes state_
	//
	// close is designed to happen at some point after the
	// simulation is running (using an ns 'at' command)

	if (argc == 2) {
		if (strcmp(argv[1], "listen") == 0) {
			// just a state transition
			listen();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "close") == 0) {
			usrclosed();
			return (TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "advance") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advanceby") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advance-bytes") == 0) {
			advance_bytes(atoi(argv[2]));
			return (TCL_OK);
		}
	}
	if (argc == 4) {
		if (strcmp(argv[1], "sendmsg") == 0) {
			sendmsg(atoi(argv[2]), argv[3]);
			return (TCL_OK);
		}
	}
	return (TcpAgent::command(argc, argv));
}

/*
 * "User Interface" Functions for Full TCP
 *	advanceby(number of packets)
 *	advance_bytes(number of bytes)
 *	sendmsg(int bytes, char* buf)
 *	listen
 *	close
 */

/*
 * the 'advance' interface to the regular tcp is in packet
 * units.  Here we scale this to bytes for full tcp.
 *
 * 'advance' is normally called by an "application" (i.e. data source)
 * to signal that there is something to send
 *
 * 'curseq_' is the sequence number of the last byte provided
 * by the application.  In the case where no data has been supplied
 * by the application, curseq_ is the iss_.
 */
void
FullTcpAgent::advanceby(int np)
{
	// XXX hack:
	//	because np is in packets and a data source
	//	may pass a *huge* number as a way to tell us
	//	to go forever, just look for the huge number
	//	and if it's there, pre-divide it
	if (np >= 0x10000000)
		np /= maxseg_;

	advance_bytes(np * maxseg_);
	return;
}

/*
 * the byte-oriented interface: advance_bytes(int nbytes)
 */

void
FullTcpAgent::advance_bytes(int nb)
{

	//
	// state-specific operations:
	//	if CLOSED or LISTEN, reset and try a new active open/connect
	//	if ESTABLISHED, queue and try to send more
	//	if SYN_SENT or SYN_RCVD, just queue
	//	if above ESTABLISHED, we are closing, so don't allow
	//

	switch (state_) {

	case TCPS_CLOSED:
	case TCPS_LISTEN:
                reset();
                curseq_ = iss_ + nb;
                connect();              // initiate new connection
		break;

	case TCPS_ESTABLISHED:
	case TCPS_SYN_SENT:
	case TCPS_SYN_RECEIVED:
                if (curseq_ < iss_) 
                        curseq_ = iss_; 
                curseq_ += nb;
		break;

	default:
            if (debug_) 
	            fprintf(stderr, "%f: FullTcpAgent::advance(%s): cannot advance while in state %s\n",
		         now(), name(), statestr(state_));

	}

	if (state_ == TCPS_ESTABLISHED)
		send_much(0, REASON_NORMAL, maxburst_);

  	return;
}

/*
 * If MSG_EOF is set, by setting close_on_empty_ to TRUE, we ensure that
 * a FIN will be sent when the send buffer emptys.
 * If DAT_EOF is set, the callback function done_data is called
 * when the send buffer empty
 * 
 * When (in the future?) FullTcpAgent implements T/TCP, avoidance of 3-way 
 * handshake can be handled in this function.
 */
void
FullTcpAgent::sendmsg(int nbytes, const char *flags)
{
	if (flags && strcmp(flags, "MSG_EOF") == 0) 
		close_on_empty_ = TRUE;	
	if (flags && strcmp(flags, "DAT_EOF") == 0) 
		signal_on_empty_ = TRUE;	

	if (nbytes == -1) {
		infinite_send_ = TRUE;
		advance_bytes(0);
	} else
		advance_bytes(nbytes);
}

/*
 * do an active open
 * (in real TCP, see tcp_usrreq, case PRU_CONNECT)
 */
void
FullTcpAgent::connect()
{
	newstate(TCPS_SYN_SENT);	// sending a SYN now
	sent(iss_, foutput(iss_, REASON_NORMAL));
	return;
}

/*
 * be a passive opener
 * (in real TCP, see tcp_usrreq, case PRU_LISTEN)
 * (for simulation, make this peer's ptype ACKs)
 */
void
FullTcpAgent::listen()
{
	newstate(TCPS_LISTEN);
	type_ = PT_ACK;	// instead of PT_TCP
}


/*
* This function is invoked when the sender buffer is empty. It in turn
* invokes the Tcl done_data procedure that was registered with TCP.
*/
 
void
FullTcpAgent::bufferempty()
{
   	signal_on_empty_=FALSE;
	Tcl::instance().evalf("%s done_data", this->name());
}


/*
 * called when user/application performs 'close'
 */

void
FullTcpAgent::usrclosed()
{
	curseq_ = maxseq_ - 1;	// now, no more data
	infinite_send_ = FALSE;	// stop infinite send

	switch (state_) {
	case TCPS_CLOSED:
	case TCPS_LISTEN:
		cancel_timers();
		newstate(TCPS_CLOSED);
		finish();
		break;
	case TCPS_SYN_SENT:
		newstate(TCPS_CLOSED);
		/* fall through */
	case TCPS_LAST_ACK:
		flags_ |= TF_NEEDFIN;
		send_much(1, REASON_NORMAL, maxburst_);
		break;
	case TCPS_SYN_RECEIVED:
	case TCPS_ESTABLISHED:
		newstate(TCPS_FIN_WAIT_1);
		flags_ |= TF_NEEDFIN;
		send_much(1, REASON_NORMAL, maxburst_);
		break;
	case TCPS_CLOSE_WAIT:
		newstate(TCPS_LAST_ACK);
		flags_ |= TF_NEEDFIN;
		send_much(1, REASON_NORMAL, maxburst_);
		break;
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSING:
		/* usr asked for a close more than once [?] */
                if (debug_)
		         fprintf(stderr,
		           "%f FullTcpAgent(%s): app close in bad state %s\n",
		           now(), name(), statestr(state_));
		break;
	default:
                if (debug_)
		        fprintf(stderr,
		        "%f FullTcpAgent(%s): app close in unknown state %s\n",
		        now(), name(), statestr(state_));
	}

	return;
}

/*
 * Utility type functions
 */

void
FullTcpAgent::cancel_timers()
{

	// cancel: rtx, burstsend, delsnd
	TcpAgent::cancel_timers();      
	// cancel: delack
	delack_timer_.force_cancel();
}

void
FullTcpAgent::newstate(int state)
{
//printf("%f(%s): state changed from %s to %s\n",
//now(), name(), statestr(state_), statestr(state));

	state_ = state;
}

void
FullTcpAgent::prpkt(Packet *pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);	// TCP header
	hdr_cmn *th = hdr_cmn::access(pkt);	// common header (size, etc)
	//hdr_flags *fh = hdr_flags::access(pkt);	// flags (CWR, CE, bits)
	hdr_ip* iph = hdr_ip::access(pkt);
	int datalen = th->size() - tcph->hlen(); // # payload bytes

	fprintf(stdout, " [%d:%d.%d>%d.%d] (hlen:%d, dlen:%d, seq:%d, ack:%d, flags:0x%x (%s), salen:%d, reason:0x%x)\n",
		th->uid(),
		iph->saddr(), iph->sport(),
		iph->daddr(), iph->dport(),
		tcph->hlen(),
		datalen,
		tcph->seqno(),
		tcph->ackno(),
		tcph->flags(), flagstr(tcph->flags()),
		tcph->sa_length(),
		tcph->reason());
}

char *
FullTcpAgent::flagstr(int hflags)
{
	// update this if tcp header flags change
	static char *flagstrs[28] = {
		"<null>", "<FIN>", "<SYN>", "<SYN,FIN>",	// 0-3
		"<?>", "<?,FIN>", "<?,SYN>", "<?,SYN,FIN>",	// 4-7
		"<PSH>", "<PSH,FIN>", "<PSH,SYN>", "<PSH,SYN,FIN>", // 0x08-0x0b
		/* do not use <??, in next line because that's an ANSI trigraph */
		"<?>", "<?,FIN>", "<?,SYN>", "<?,SYN,FIN>",	    // 0x0c-0x0f
		"<ACK>", "<ACK,FIN>", "<ACK,SYN>", "<ACK,SYN,FIN>", // 0x10-0x13
		"<ACK>", "<ACK,FIN>", "<ACK,SYN>", "<ACK,SYN,FIN>", // 0x14-0x17
		"<PSH,ACK>", "<PSH,ACK,FIN>", "<PSH,ACK,SYN>", "<PSH,ACK,SYN,FIN>", // 0x18-0x1b
	};
	if (hflags < 0 || (hflags > 28)) {
		/* Added strings for CWR and ECE  -M. Weigle 6/27/02 */
		if (hflags == 72) 
	 		return ("<ECE,PSH>");
	 	else if (hflags == 80)
	 		return ("<ECE,ACK>");
	 	else if (hflags == 88) 
	 		return ("<ECE,PSH,ACK>");
	 	else if (hflags == 152) 
	 		return ("<CWR,PSH,ACK>");
		else if (hflags == 153)
			return ("<CWR,PSH,ACK,FIN>");
		else
			return ("<invalid>");
	}
	return (flagstrs[hflags]);
}

char *
FullTcpAgent::statestr(int state)
{
	static char *statestrs[TCP_NSTATES] = {
		"CLOSED", "LISTEN", "SYN_SENT", "SYN_RCVD",
		"ESTABLISHED", "CLOSE_WAIT", "FIN_WAIT_1", "CLOSING",
		"LAST_ACK", "FIN_WAIT_2"
	};
	if (state < 0 || (state >= TCP_NSTATES))
		return ("INVALID");
	return (statestrs[state]);
}

void
DelAckTimer::expire(Event *) {
        a_->timeout(TCP_TIMER_DELACK);
}

/*
 * reset to starting point, don't set state_ here,
 * because our starting point might be LISTEN rather
 * than CLOSED if we're a passive opener
 */
void
FullTcpAgent::reset()
{
	cancel_timers();	// cancel timers first
      	TcpAgent::reset();	// resets most variables
	rq_.clear();		// clear reassembly queue
	rtt_init();		// zero rtt, srtt, backoff

	last_ack_sent_ = -1;
	rcv_nxt_ = -1;
	pipe_ = 0;
	rtxbytes_ = 0;
	flags_ = 0;
	t_seqno_ = iss_;
	maxseq_ = -1;
	irs_ = -1;
	last_send_time_ = -1.0;
	if (ts_option_)
		recent_ = recent_age_ = 0.0;
	else
		recent_ = recent_age_ = -1.0;

	fastrecov_ = FALSE;

	closed_ = 0;
	close_on_empty_ = FALSE;

        if (ecn_syn_)
                ecn_syn_next_ = 1;
        else
                ecn_syn_next_ = 0;

}

/*
 * This function is invoked when the connection is done. It in turn
 * invokes the Tcl finish procedure that was registered with TCP.
 * This function mimics tcp_close()
 */

void
FullTcpAgent::finish()
{
	Tcl::instance().evalf("%s done", this->name());
}
/*
 * headersize:
 *	how big is an IP+TCP header in bytes; include options such as ts
 * this function should be virtual so others (e.g. SACK) can override
 */
int
FullTcpAgent::headersize()
{
	int total = tcpip_base_hdr_size_;
	if (total < 1) {
		fprintf(stderr,
		    "%f: FullTcpAgent(%s): warning: tcpip hdr size is only %d bytes\n",
			now(), name(), tcpip_base_hdr_size_);
	}

	if (ts_option_)
		total += ts_option_size_;

	return (total);
}

/*
 * flags that are completely dependent on the tcp state
 * these are used for the next outgoing packet in foutput()
 * (in real TCP, see tcp_fsm.h, the "tcp_outflags" array)
 */
int
FullTcpAgent::outflags()
{
	// in real TCP an RST is added in the CLOSED state
	static int tcp_outflags[TCP_NSTATES] = {
		TH_ACK,          	/* 0, CLOSED */  
		0,                      /* 1, LISTEN */ 
		TH_SYN,                 /* 2, SYN_SENT */
		TH_SYN|TH_ACK,          /* 3, SYN_RECEIVED */
		TH_ACK,                 /* 4, ESTABLISHED */
		TH_ACK,                 /* 5, CLOSE_WAIT */
		TH_FIN|TH_ACK,          /* 6, FIN_WAIT_1 */
		TH_FIN|TH_ACK,          /* 7, CLOSING */
		TH_FIN|TH_ACK,          /* 8, LAST_ACK */
		TH_ACK,                 /* 9, FIN_WAIT_2 */
		/* 10, TIME_WAIT --- not used in simulator */
	};

	if (state_ < 0 || (state_ >= TCP_NSTATES)) {
		fprintf(stderr, "%f FullTcpAgent(%s): invalid state %d\n",
			now(), name(), state_);
		return (0x0);
	}

	return (tcp_outflags[state_]);
}

/*
 * reaass() -- extract the appropriate fields from the packet
 *	and pass this info the ReassemblyQueue add routine
 *
 * returns the TCP header flags representing the "or" of
 *	the flags contained in the adjacent sequence # blocks
 */

int
FullTcpAgent::reass(Packet* pkt)
{  
        hdr_tcp *tcph =  hdr_tcp::access(pkt);
        hdr_cmn *th = hdr_cmn::access(pkt);
   
        int start = tcph->seqno();
        int end = start + th->size() - tcph->hlen();
        int tiflags = tcph->flags();
	int fillshole = (start == rcv_nxt_);
	int flags;
   
	// end contains the seq of the last byte of
	// in the packet plus one

	if (start == end && (tiflags & TH_FIN) == 0) {
		fprintf(stderr, "%f: FullTcpAgent(%s)::reass() -- bad condition - adding non-FIN zero-len seg\n",
			now(), name());
		abort();
	}

	flags = rq_.add(start, end, tiflags, 0);

	//present:
	//
	// If we've never received a SYN (unlikely)
	// or this is an out of order addition, no reason to coalesce
	//

	if (TCPS_HAVERCVDSYN(state_) == 0 || !fillshole) {
	 	return (0x00);
	}
	//
	// If we get some data in SYN_RECVD, no need to present to user yet
	//
	if (state_ == TCPS_SYN_RECEIVED && (end > start))
		return (0x00);

	// clear out data that has been passed, up to rcv_nxt_,
	// collects flags

	flags |= rq_.cleartonxt();

        return (flags);
}

/*
 * utility function to set rcv_next_ during inital exchange of seq #s
 */

int
FullTcpAgent::rcvseqinit(int seq, int dlen)
{
	return (seq + dlen + 1);
}

/*
 * build a header with the timestamp option if asked
 */
int
FullTcpAgent::build_options(hdr_tcp* tcph)
{
	int total = 0;
	if (ts_option_) {
		tcph->ts() = now();
		tcph->ts_echo() = recent_;
		total += ts_option_size_;
	} else {
		tcph->ts() = tcph->ts_echo() = -1.0;
	}
	return (total);
}

/*
 * pack() -- is the ACK a partial ACK? (not past recover_)
 */

int
FullTcpAgent::pack(Packet *pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	/* Added check for fast recovery.  -M. Weigle 5/2/02 */
	return (fastrecov_ && tcph->ackno() >= highest_ack_ &&
		tcph->ackno() < recover_);
}

/*
 * baseline reno TCP exists fast recovery on a partial ACK
 */

void
FullTcpAgent::pack_action(Packet*)
{
	if (reno_fastrecov_ && fastrecov_ && cwnd_ > double(ssthresh_)) {
		cwnd_ = double(ssthresh_); // retract window if inflated
	}
	fastrecov_ = FALSE;
//printf("%f: EXITED FAST RECOVERY\n", now());
	dupacks_ = 0;
}

/*
 * ack_action -- same as partial ACK action for base Reno TCP
 */

void
FullTcpAgent::ack_action(Packet* p)
{
	FullTcpAgent::pack_action(p);
}


/*
 * sendpacket: 
 *	allocate a packet, fill in header fields, and send
 *	also keeps stats on # of data pkts, acks, re-xmits, etc
 *
 * fill in packet fields.  Agent::allocpkt() fills
 * in most of the network layer fields for us.
 * So fill in tcp hdr and adjust the packet size.
 *
 * Also, set the size of the tcp header.
 */
void
FullTcpAgent::sendpacket(int seqno, int ackno, int pflags, int datalen, int reason, Packet *p)
{
        if (!p) p = allocpkt();
        hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_flags *fh = hdr_flags::access(p);

	/* build basic header w/options */

        tcph->seqno() = seqno;
        tcph->ackno() = ackno;
        tcph->flags() = pflags;
        tcph->reason() |= reason; // make tcph->reason look like ns1 pkt->flags?
	tcph->sa_length() = 0;    // may be increased by build_options()
        tcph->hlen() = tcpip_base_hdr_size_;
	tcph->hlen() += build_options(tcph);

	/*
	 * Explicit Congestion Notification (ECN) related:
	 * Bits in header:
	 * 	ECT (EC Capable Transport),
	 * 	ECNECHO (ECHO of ECN Notification generated at router),
	 * 	CWR (Congestion Window Reduced from RFC 2481)
	 * States in TCP:
	 *	ecn_: I am supposed to do ECN if my peer does
	 *	ect_: I am doing ECN (ecn_ should be T and peer does ECN)
	 */

	if (datalen > 0 && ecn_ ){
	        // set ect on data packets 
		fh->ect() = ect_;	// on after mutual agreement on ECT
        } else if (ecn_ && ecn_syn_ && ecn_syn_next_ && (pflags & TH_SYN) && (pflags & TH_ACK)) {
                // set ect on syn/ack packet, if syn packet was negotiating ECT
               	fh->ect() = ect_;
	} else {
		/* Set ect() to 0.  -M. Weigle 1/19/05 */
		fh->ect() = 0;
	}
	if (ecn_ && ect_ && recent_ce_ ) { 
		// This is needed here for the ACK in a SYN, SYN/ACK, ACK
		// sequence.
		pflags |= TH_ECE;
	}
        // fill in CWR and ECE bits which don't actually sit in
        // the tcp_flags but in hdr_flags
        if ( pflags & TH_ECE) {
                fh->ecnecho() = 1;
        } else {
                fh->ecnecho() = 0;
        }
        if ( pflags & TH_CWR ) {
                fh->cong_action() = 1;
        }
	else {
		/* Set cong_action() to 0  -M. Weigle 1/19/05 */
		fh->cong_action() = 0;
	}

	/* actual size is data length plus header length */

        hdr_cmn *ch = hdr_cmn::access(p);
        ch->size() = datalen + tcph->hlen();

        if (datalen <= 0)
                ++nackpack_;
        else {
                ++ndatapack_;
                ndatabytes_ += datalen;
		last_send_time_ = now();	// time of last data
        }
        if (reason == REASON_TIMEOUT || reason == REASON_DUPACK || reason == REASON_SACK) {
                ++nrexmitpack_;
                nrexmitbytes_ += datalen;
        }

	last_ack_sent_ = ackno;

//if (state_ != TCPS_ESTABLISHED) {
//printf("%f(%s)[state:%s]: sending pkt ", now(), name(), statestr(state_));
//prpkt(p);
//}

	send(p, 0);

	return;
}

//
// reset_rtx_timer: called during a retransmission timeout
// to perform exponential backoff.  Also, note that because
// we have performed a retransmission, our rtt timer is now
// invalidated (indicate this by setting rtt_active_ false)
//
void
FullTcpAgent::reset_rtx_timer(int /* mild */)
{
	// cancel old timer, set a new one
	/* if there is no outstanding data, don't back off rtx timer *
	 * (Fix from T. Kelly.) */
	if (!(highest_ack_ == maxseq_ && restart_bugfix_)) {
        	rtt_backoff();		// double current timeout
	}
        set_rtx_timer();	// set new timer
        rtt_active_ = FALSE;	// no timing during this window
}

/*
 * see if we should send a segment, and if so, send it
 * 	(may be ACK or data)
 * return the number of data bytes sent (count a SYN or FIN as 1 each)
 *
 * simulator var, desc (name in real TCP)
 * --------------------------------------
 * maxseq_, largest seq# we've sent plus one (snd_max)
 * flags_, flags regarding our internal state (t_state)
 * pflags, a local used to build up the tcp header flags (flags)
 * curseq_, is the highest sequence number given to us by "application"
 * highest_ack_, the highest ACK we've seen for our data (snd_una-1)
 * seqno, the next seq# we're going to send (snd_nxt)
 */
int
FullTcpAgent::foutput(int seqno, int reason)
{
	// if maxseg_ not set, set it appropriately
	// Q: how can this happen?

	if (maxseg_ == 0) 
	   	maxseg_ = size_ - headersize();
	else
		size_ =  maxseg_ + headersize();

	int is_retransmit = (seqno < maxseq_);
	int quiet = (highest_ack_ == maxseq_);
	int pflags = outflags();
	int syn = (seqno == iss_);
	int emptying_buffer = FALSE;
	int buffered_bytes = (infinite_send_) ? TCP_MAXSEQ :
				curseq_ - highest_ack_ + 1;

	int win = window() * maxseg_;	// window (in bytes)
	int off = seqno - highest_ack_;	// offset of seg in window
	int datalen;
	//int amtsent = 0;

	// be careful if we have not received any ACK yet
	if (highest_ack_ < 0) {
		if (!infinite_send_)
			buffered_bytes = curseq_ - iss_;;
		off = seqno - iss_;
	}

	if (syn && !data_on_syn_)
		datalen = 0;
	else if (pipectrl_)
		datalen = buffered_bytes - off;
	else
		datalen = min(buffered_bytes, win) - off;

        if ((signal_on_empty_) && (!buffered_bytes) && (!syn))
	                bufferempty();

	//
	// in real TCP datalen (len) could be < 0 if there was window
	// shrinkage, or if a FIN has been sent and neither ACKd nor
	// retransmitted.  Only this 2nd case concerns us here...
	//
	if (datalen < 0) {
		datalen = 0;
	} else if (datalen > maxseg_) {
		datalen = maxseg_;
	}

	//
	// this is an option that causes us to slow-start if we've
	// been idle for a "long" time, where long means a rto or longer
	// the slow-start is a sort that does not set ssthresh
	//

	if (slow_start_restart_ && quiet && datalen > 0) {
		if (idle_restart()) {
			slowdown(CLOSE_CWND_INIT);
		}
	}

	//
	// see if sending this packet will empty the send buffer
	// a dataless SYN packet counts also
	//

	if (!infinite_send_ && ((seqno + datalen) > curseq_ || 
	    (syn && datalen == 0))) {
		emptying_buffer = TRUE;
		//
		// if not a retransmission, notify application that 
		// everything has been sent out at least once.
		//
		if (!syn) {
			idle();
			if (close_on_empty_ && quiet) {
				flags_ |= TF_NEEDCLOSE;
			}
		}
		pflags |= TH_PUSH;
		//
		// if close_on_empty set, we are finished
		// with this connection; close it
		//
	} else  {
		/* not emptying buffer, so can't be FIN */
		pflags &= ~TH_FIN;
	}
	if (infinite_send_ && (syn && datalen == 0))
		pflags |= TH_PUSH;  // set PUSH for dataless SYN

	/* sender SWS avoidance (Nagle) */

	if (datalen > 0) {
		// if full-sized segment, ok
		if (datalen == maxseg_)
			goto send;
		// if Nagle disabled and buffer clearing, ok
		if ((quiet || nodelay_)  && emptying_buffer)
			goto send;
		// if a retransmission
		if (is_retransmit)
			goto send;
		// if big "enough", ok...
		//	(this is not a likely case, and would
		//	only happen for tiny windows)
		if (datalen >= ((wnd_ * maxseg_) / 2.0))
			goto send;
	}

	if (need_send())
		goto send;

	/*
	 * send now if a control packet or we owe peer an ACK
	 * TF_ACKNOW can be set during connection establishment and
	 * to generate acks for out-of-order data
	 */

	if ((flags_ & (TF_ACKNOW|TF_NEEDCLOSE)) ||
	    (pflags & (TH_SYN|TH_FIN))) {
		goto send;
	}

        /*      
         * No reason to send a segment, just return.
         */      
	return 0;

send:

	// is a syn or fin?

	syn = (pflags & TH_SYN) ? 1 : 0;
	int fin = (pflags & TH_FIN) ? 1 : 0;

        /* setup ECN syn and ECN SYN+ACK packet headers */
        if (ecn_ && syn && !(pflags & TH_ACK)){
                pflags |= TH_ECE;
                pflags |= TH_CWR;
        }
        if (ecn_ && syn && (pflags & TH_ACK)){
                pflags |= TH_ECE;
                pflags &= ~TH_CWR;
        }
	else if (ecn_ && ect_ && cong_action_ && 
	             (!is_retransmit || SetCWRonRetransmit_)) {
		/* 
		 * Don't set CWR for a retranmitted SYN+ACK (has ecn_ 
		 * and cong_action_ set).
		 * -M. Weigle 6/19/02
                 *
                 * SetCWRonRetransmit_ was changed to true,
                 * allowing CWR on retransmitted data packets.  
                 * See test ecn_burstyEcn_reno_full 
                 * in test-suite-ecn-full.tcl.
		 * - Sally Floyd, 6/5/08.
		 */
		/* set CWR if necessary */
		pflags |= TH_CWR;
		/* Turn cong_action_ off: Added 6/5/08, Sally Floyd. */
		cong_action_ = FALSE;
	}

	/* moved from sendpacket()  -M. Weigle 6/19/02 */
	//
	// although CWR bit is ordinarily associated with ECN,
	// it has utility within the simulator for traces. Thus, set
	// it even if we aren't doing ECN
	//
	if (datalen > 0 && cong_action_ && !is_retransmit) {
		pflags |= TH_CWR;
	}
  
        /* set ECE if necessary */
        if (ecn_ && ect_ && recent_ce_ ) {
		pflags |= TH_ECE;
	}

        /* 
         * Tack on the FIN flag to the data segment if close_on_empty_
         * was previously set-- avoids sending a separate FIN
         */ 
        if (flags_ & TF_NEEDCLOSE) {
                flags_ &= ~TF_NEEDCLOSE;
                if (state_ <= TCPS_ESTABLISHED && state_ != TCPS_CLOSED)
                {
                    pflags |=TH_FIN;
                    fin = 1;  /* FIN consumes sequence number */
                    newstate(TCPS_FIN_WAIT_1);
                }
        }
	sendpacket(seqno, rcv_nxt_, pflags, datalen, reason);

        /*      
         * Data sent (as far as we can tell).
         * Any pending ACK has now been sent.
         */      
	flags_ &= ~(TF_ACKNOW|TF_DELACK);

	/*
	 * if we have reacted to congestion recently, the
	 * slowdown() procedure will have set cong_action_ and
	 * sendpacket will have copied that to the outgoing pkt
	 * CWR field. If that packet contains data, then
	 * it will be reliably delivered, so we are free to turn off the
	 * cong_action_ state now  If only a pure ACK, we keep the state
	 * around until we actually send a segment
	 */

	int reliable = datalen + syn + fin; // seq #'s reliably sent
	/* 
	 * Don't reset cong_action_ until we send new data.
	 * -M. Weigle 6/19/02
	 */
	if (cong_action_ && reliable > 0 && !is_retransmit)
		cong_action_ = FALSE;

	// highest: greatest sequence number sent + 1
	//	and adjusted for SYNs and FINs which use up one number

	int highest = seqno + reliable;
	if (highest > maxseq_) {
		maxseq_ = highest;
		//
		// if we are using conventional RTT estimation,
		// establish timing on this segment
		//
		if (!ts_option_ && rtt_active_ == FALSE) {
			rtt_active_ = TRUE;	// set timer
			rtt_seq_ = seqno; 	// timed seq #
			rtt_ts_ = now();	// when set
		}
	}

	/*
	 * Set retransmit timer if not currently set,
	 * and not doing an ack or a keep-alive probe.
	 * Initial value for retransmit timer is smoothed
	 * round-trip time + 2 * round-trip time variance.
	 * Future values are rtt + 4 * rttvar.
	 */
	if (rtx_timer_.status() != TIMER_PENDING && reliable) {
		set_rtx_timer();  // no timer pending, schedule one
	}

	return (reliable);
}

/*
 *
 * send_much: send as much data as we are allowed to.  This is
 * controlled by the "pipectrl_" variable.  If pipectrl_ is set
 * to FALSE, then we are working as a normal window-based TCP and
 * we are allowed to send whatever the window allows.
 * If pipectrl_ is set to TRUE, then we are allowed to send whatever
 * pipe_ allows us to send.  One tricky part is to make sure we
 * do not overshoot the receiver's advertised window if we are
 * in (pipectrl_ == TRUE) mode.
 */
  
void
FullTcpAgent::send_much(int force, int reason, int maxburst)
{
	int npackets = 0;	// sent so far

//if ((int(t_seqno_)) > 1)
//printf("%f: send_much(f:%d, win:%d, pipectrl:%d, pipe:%d, t_seqno:%d, topwin:%d, maxseq_:%d\n",
//now(), force, win, pipectrl_, pipe_, int(t_seqno_), topwin, int(maxseq_));

	if (!force && (delsnd_timer_.status() == TIMER_PENDING))
		return;

	while (1) {

		/*
		 * note that if output decides to not actually send
		 * (e.g. because of Nagle), then if we don't break out
		 * of this loop, we can loop forever at the same
		 * simulated time instant
		 */
		int amt;
		int seq = nxt_tseq();
		if (!force && !send_allowed(seq))
			break;
		// Q: does this need to be here too?
		if (!force && overhead_ != 0 &&
		    (delsnd_timer_.status() != TIMER_PENDING)) {
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if ((amt = foutput(seq, reason)) <= 0)
			break;
		if ((outflags() & TH_FIN))
			--amt;	// don't count FINs
		sent(seq, amt);
		force = 0;

		if ((outflags() & (TH_SYN|TH_FIN)) ||
		    (maxburst && ++npackets >= maxburst))
			break;
	}
	return;
}

/*
 * base TCP: we are allowed to send a sequence number if it
 * is in the window
 */
int
FullTcpAgent::send_allowed(int seq)
{
        int win = window() * maxseg_;
        int topwin = curseq_; // 1 seq number past the last byte we can send

        if ((topwin > highest_ack_ + win) || infinite_send_)
                topwin = highest_ack_ + win; 

	return (seq < topwin);
}
/*
 * Process an ACK
 *	this version of the routine doesn't necessarily
 *	require the ack to be one which advances the ack number
 *
 * if this ACKs a rtt estimate
 *	indicate we are not timing
 *	reset the exponential timer backoff (gamma)
 * update rtt estimate
 * cancel retrans timer if everything is sent and ACK'd, else set it
 * advance the ack number if appropriate
 * update segment to send next if appropriate
 */
void
FullTcpAgent::newack(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	register int ackno = tcph->ackno();
	int progress = (ackno > highest_ack_);

	if (ackno == maxseq_) {
		cancel_rtx_timer();	// all data ACKd
	} else if (progress) {
		set_rtx_timer();
	}

	// advance the ack number if this is for new data
	if (progress)
		highest_ack_ = ackno;

	// if we have suffered a retransmit timeout, t_seqno_
	// will have been reset to highest_ ack.  If the
	// receiver has cached some data above t_seqno_, the
	// new-ack value could (should) jump forward.  We must
	// update t_seqno_ here, otherwise we would be doing
	// go-back-n.

	if (t_seqno_ < highest_ack_)
		t_seqno_ = highest_ack_; // seq# to send next

        /*
         * Update RTT only if it's OK to do so from info in the flags header.
         * This is needed for protocols in which intermediate agents

         * in the network intersperse acks (e.g., ack-reconstructors) for
         * various reasons (without violating e2e semantics).
         */
        hdr_flags *fh = hdr_flags::access(pkt);

	if (!fh->no_ts_) {
                if (ts_option_) {
			recent_age_ = now();
			recent_ = tcph->ts();
			rtt_update(now() - tcph->ts_echo());
			if (ts_resetRTO_ && (!ect_ || !ecn_backoff_ ||
		           !hdr_flags::access(pkt)->ecnecho())) { 
				// From Andrei Gurtov
				//
                         	// Don't end backoff if still in ECN-Echo with
                         	// a congestion window of 1 packet.
				t_backoff_ = 1;
			}
		} else if (rtt_active_ && ackno > rtt_seq_) {
			// got an RTT sample, record it
			// "t_backoff_ = 1;" deleted by T. Kelly.
			rtt_active_ = FALSE;
			rtt_update(now() - rtt_ts_);
                }
		if (!ect_ || !ecn_backoff_ ||
		    !hdr_flags::access(pkt)->ecnecho()) {
			/*
			 * Don't end backoff if still in ECN-Echo with
			 * a congestion window of 1 packet.
			 * Fix from T. Kelly.
			 */
			t_backoff_ = 1;
			ecn_backoff_ = 0;
		}

        }
	return;
}

/*
 * this is the simulated form of the header prediction
 * predicate.  While not really necessary for a simulation, it
 * follows the code base more closely and can sometimes help to reveal
 * odd behavior caused by the implementation structure..
 *
 * Here's the comment from the real TCP:
 *
 * Header prediction: check for the two common cases
 * of a uni-directional data xfer.  If the packet has
 * no control flags, is in-sequence, the window didn't
 * change and we're not retransmitting, it's a
 * candidate.  If the length is zero and the ack moved
 * forward, we're the sender side of the xfer.  Just
 * free the data acked & wake any higher level process
 * that was blocked waiting for space.  If the length
 * is non-zero and the ack didn't move, we're the
 * receiver side.  If we're getting packets in-order
 * (the reassembly queue is empty), add the data to
 * the socket buffer and note that we need a delayed ack.
 * Make sure that the hidden state-flags are also off.
 * Since we check for TCPS_ESTABLISHED above, it can only
 * be TF_NEEDSYN.
 */

int
FullTcpAgent::predict_ok(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
        hdr_flags *fh = hdr_flags::access(pkt);

	/* not the fastest way to do this, but perhaps clearest */

	int p1 = (state_ == TCPS_ESTABLISHED);		// ready
	int p2 = ((tcph->flags() & (TH_SYN|TH_FIN|TH_ACK)) == TH_ACK); // ACK
	int p3 = ((flags_ & TF_NEEDFIN) == 0);		// don't need fin
	int p4 = (!ts_option_ || fh->no_ts_ || (tcph->ts() >= recent_)); // tsok
	int p5 = (tcph->seqno() == rcv_nxt_);		// in-order data
	int p6 = (t_seqno_ == maxseq_);			// not re-xmit
	int p7 = (!ecn_ || fh->ecnecho() == 0);		// no ECN
	int p8 = (tcph->sa_length() == 0);		// no SACK info

	return (p1 && p2 && p3 && p4 && p5 && p6 && p7 && p8);
}

/*
 * fast_retransmit using the given seqno
 *	perform fast RTX, set recover_, set last_cwnd_action
 */

int
FullTcpAgent::fast_retransmit(int seq)
{
	// we are now going to fast-retransmit and willtrace that event
	trace_event("FAST_RETX");
	
	recover_ = maxseq_;	// recovery target
	last_cwnd_action_ = CWND_ACTION_DUPACK;
	return(foutput(seq, REASON_DUPACK));	// send one pkt
}

/*
 * real tcp determines if the remote
 * side should receive a window update/ACK from us, and often
 * results in sending an update every 2 segments, thereby
 * giving the familiar 2-packets-per-ack behavior of TCP.
 * Here, we don't advertise any windows, so we just see if
 * there's at least 'segs_per_ack_' pkts not yet acked
 *
 * also, provide for a segs-per-ack "threshold" where
 * we generate 1-ack-per-seg until enough stuff
 * (spa_thresh_ bytes) has been received from the other side
 * This idea came from vj/kmn in BayTcp.  Added 8/21/01.
 */

int
FullTcpAgent::need_send()
{
	if (flags_ & TF_ACKNOW)
		return TRUE;

	int spa = (spa_thresh_ > 0 && ((rcv_nxt_ - irs_)  < spa_thresh_)) ?
		1 : segs_per_ack_;
		
	return ((rcv_nxt_ - last_ack_sent_) >= (spa * maxseg_));
}

/*
 * determine whether enough time has elapsed in order to
 * conclude a "restart" is necessary (e.g. a slow-start)
 *
 * for now, keep track of this similarly to how rtt_update() does
 */

int
FullTcpAgent::idle_restart()
{
	if (last_send_time_ < 0.0) {
		// last_send_time_ isn't set up yet, we shouldn't
		// do the idle_restart
		return (0);
	}

	double tao = now() - last_send_time_;
	if (!ts_option_) {
                double tickoff = fmod(last_send_time_ + boot_time_,
			tcp_tick_);
                tao = int((tao + tickoff) / tcp_tick_) * tcp_tick_;
	}

	return (tao > t_rtxcur_);  // verify this CHECKME
}

/*
 * tcp-full's version of set_initial_window()... over-rides
 * the one in tcp.cc
 */
void
FullTcpAgent::set_initial_window()
{
	syn_ = TRUE;	// full-tcp always models SYN exchange
	TcpAgent::set_initial_window();
}       

/*
 * main reception path - 
 * called from the agent that handles the data path below in its muxing mode
 * advance() is called when connection is established with size sent from
 * user/application agent
 *
 * This is a fairly complex function.  It operates generally as follows:
 *	do header prediction for simple cases (pure ACKS or data)
 *	if in LISTEN and we get a SYN, begin initializing connection
 *	if in SYN_SENT and we get an ACK, complete connection init
 *	trim any redundant data from received dataful segment
 *	deal with ACKS:
 *		if in SYN_RCVD, complete connection init then go on
 *		see if ACK is old or at the current highest_ack
 *		if at current high, is the threshold reached or not
 *		if so, maybe do fast rtx... otherwise drop or inflate win
 *	deal with incoming data
 *	deal with FIN bit on in arriving packet
 */
void
FullTcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);	// TCP header
	hdr_cmn *th = hdr_cmn::access(pkt);	// common header (size, etc)
	hdr_flags *fh = hdr_flags::access(pkt);	// flags (CWR, CE, bits)

	int needoutput = FALSE;
	int ourfinisacked = FALSE;
	int dupseg = FALSE;			// recv'd dup data segment
	int todrop = 0;				// duplicate DATA cnt in seg

	last_state_ = state_;

	int datalen = th->size() - tcph->hlen(); // # payload bytes
	int ackno = tcph->ackno();		 // ack # from packet
	int tiflags = tcph->flags() ; 		 // tcp flags from packet

//if (state_ != TCPS_ESTABLISHED || (tiflags&(TH_SYN|TH_FIN))) {
//fprintf(stdout, "%f(%s)in state %s recv'd this packet: ", now(), name(), statestr(state_));
//prpkt(pkt);
//}

	/* 
	 * Acknowledge FIN from passive closer even in TCPS_CLOSED state
	 * (since we lack TIME_WAIT state and RST packets,
	 * the loss of the FIN packet from the passive closer will make that
	 * endpoint retransmit the FIN forever)
	 * -F. Hernandez-Campos 8/6/00
	 */
	if ( (state_ == TCPS_CLOSED) && (tiflags & TH_FIN) ) {
		goto dropafterack;
	}

	/*
	 * Don't expect to see anything while closed
	 */

	if (state_ == TCPS_CLOSED) {
                if (debug_) {
		        fprintf(stderr, "%f: FullTcp(%s): recv'd pkt in CLOSED state: ",
			    now(), name());
		        prpkt(pkt);
                }
		goto drop;
	}

        /*
         * Process options if not in LISTEN state,
         * else do it below
         */
	if (state_ != TCPS_LISTEN)
		dooptions(pkt);

	/*
	 * if we are using delayed-ACK timers and
	 * no delayed-ACK timer is set, set one.
	 * They are set to fire every 'interval_' secs, starting
	 * at time t0 = (0.0 + k * interval_) for some k such
	 * that t0 > now
	 */
	if (delack_interval_ > 0.0 &&
	    (delack_timer_.status() != TIMER_PENDING)) {
		int last = int(now() / delack_interval_);
		delack_timer_.resched(delack_interval_ * (last + 1.0) - now());
	}

	/*
	 * Try header prediction: in seq data or in seq pure ACK
	 *	with no funny business
	 */
	if (!nopredict_ && predict_ok(pkt)) {
                /*
                 * If last ACK falls within this segment's sequence numbers,
                 * record the timestamp.
		 * See RFC1323 (now RFC1323 bis)
                 */
                if (ts_option_ && !fh->no_ts_ &&
		    tcph->seqno() <= last_ack_sent_) {
			/*
			 * this is the case where the ts value is newer than
			 * the last one we've seen, and the seq # is the one
			 * we expect [seqno == last_ack_sent_] or older
			 */
			recent_age_ = now();
			recent_ = tcph->ts();
                }

		//
		// generate a stream of ecnecho bits until we see a true
		// cong_action bit
		//

	    	if (ecn_) {
	    		if (fh->ce() && fh->ect()) {
	    			// no CWR from peer yet... arrange to
	    			// keep sending ECNECHO
	    			recent_ce_ = TRUE;
	    		} else if (fh->cwr()) {
	    			// got CWR response from peer.. stop
	    			// sending ECNECHO bits
	    			recent_ce_ = FALSE;
	    		}
	    	}

		// Header predication basically looks to see
		// if the incoming packet is an expected pure ACK
		// or an expected data segment

		if (datalen == 0) {
			// check for a received pure ACK in the correct range..
			// also checks to see if we are wnd_ limited
			// (we don't change cwnd at all below), plus
			// not being in fast recovery and not a partial ack.
			// If we are in fast
			// recovery, go below so we can remember to deflate
			// the window if we need to
			if (ackno > highest_ack_ && ackno < maxseq_ &&
			    cwnd_ >= wnd_ && !fastrecov_) {
				newack(pkt);	// update timers,  highest_ack_
				send_much(0, REASON_NORMAL, maxburst_);
				Packet::free(pkt);
				return;
			}
		} else if (ackno == highest_ack_ && rq_.empty()) {
			// check for pure incoming segment
			// the next data segment we're awaiting, and
			// that there's nothing sitting in the reassem-
			// bly queue
			// 	give to "application" here
			//	note: DELACK is inspected only by
			//	tcp_fasttimo() in real tcp.  Every 200 ms
			//	this routine scans all tcpcb's looking for
			//	DELACK segments and when it finds them
			//	changes DELACK to ACKNOW and calls tcp_output()
			rcv_nxt_ += datalen;
			flags_ |= TF_DELACK;
			recvBytes(datalen); // notify application of "delivery"
			//
			// special code here to simulate the operation
			// of a receiver who always consumes data,
			// resulting in a call to tcp_output
			Packet::free(pkt);
			if (need_send())
				send_much(1, REASON_NORMAL, maxburst_);
			return;
		}
	} /* header prediction */


	//
	// header prediction failed
	// (e.g. pure ACK out of valid range, SACK present, etc)...
	// do slow path processing

	//
	// the following switch does special things for these states:
	//	TCPS_LISTEN, TCPS_SYN_SENT
	//

	switch (state_) {

        /*
         * If the segment contains an ACK then it is bad and do reset.
         * If it does not contain a SYN then it is not interesting; drop it.
         * Otherwise initialize tp->rcv_nxt, and tp->irs, iss is already
	 * selected, and send a segment:
         *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
         * Initialize tp->snd_nxt to tp->iss.
         * Enter SYN_RECEIVED state, and process any other fields of this
         * segment in this state.
         */

	case TCPS_LISTEN:	/* awaiting peer's SYN */

		if (tiflags & TH_ACK) {
                        if (debug_) {
		    	        fprintf(stderr,
		    		"%f: FullTcpAgent(%s): warning: recv'd ACK while in LISTEN: ",
				    now(), name());
			        prpkt(pkt);
                        }
			// don't want ACKs in LISTEN
			goto dropwithreset;
		}
		if ((tiflags & TH_SYN) == 0) {
                        if (debug_) {
		    	        fprintf(stderr, "%f: FullTcpAgent(%s): warning: recv'd NON-SYN while in LISTEN\n",
				now(), name());
			        prpkt(pkt);
                        }
			// any non-SYN is discarded
			goto drop;
		}

		/*
		 * must by a SYN (no ACK) at this point...
		 * in real tcp we would bump the iss counter here also
		 */
		dooptions(pkt);
		irs_ = tcph->seqno();
		t_seqno_ = iss_; /* tcp_sendseqinit() macro in real tcp */
		rcv_nxt_ = rcvseqinit(irs_, datalen);
		flags_ |= TF_ACKNOW;

		// check for a ECN-SYN with ECE|CWR
		if (ecn_ && fh->ecnecho() && fh->cong_action()) {
			ect_ = TRUE;
		}


		if (fid_ == 0) {
			// XXX: sort of hack... If we do not
			// have a special flow ID, pick up that
			// of the sender (active opener)
			hdr_ip* iph = hdr_ip::access(pkt);
			fid_ = iph->flowid();
		}

		newstate(TCPS_SYN_RECEIVED);
		goto trimthenstep6;

        /*
         * If the state is SYN_SENT:
         *      if seg contains an ACK, but not for our SYN, drop the input.
         *      if seg does not contain SYN, then drop it.
         * Otherwise this is an acceptable SYN segment
         *      initialize tp->rcv_nxt and tp->irs
         *      if seg contains ack then advance tp->snd_una
         *      if SYN has been acked change to ESTABLISHED else SYN_RCVD state
         *      arrange for segment to be acked (eventually)
         *      continue processing rest of data/controls, beginning with URG
         */

	case TCPS_SYN_SENT:	/* we sent SYN, expecting SYN+ACK (or SYN) */

		/* drop if it's a SYN+ACK and the ack field is bad */
		if ((tiflags & TH_ACK) &&
			((ackno <= iss_) || (ackno > maxseq_))) {
			// not an ACK for our SYN, discard
                        if (debug_) {
			       fprintf(stderr, "%f: FullTcpAgent::recv(%s): bad ACK for our SYN: ",
			        now(), name());
			        prpkt(pkt);
                        }
			goto dropwithreset;
		}

		if ((tiflags & TH_SYN) == 0) {
                        if (debug_) {
			        fprintf(stderr, "%f: FullTcpAgent::recv(%s): no SYN for our SYN: ",
			        now(), name());
			        prpkt(pkt);
                        }
			goto drop;
		}

		/* looks like an ok SYN or SYN+ACK */
                // If ecn_syn_wait is set to 2:
		// Check if CE-marked SYN/ACK packet, then just send an ACK
                //  packet with ECE set, and drop the SYN/ACK packet.
                //  Don't update TCP state. 
		if (tiflags & TH_ACK) 
		{
                        if (ecn_ && fh->ecnecho() && !fh->cong_action() && ecn_syn_wait_ == 2) 
                        // if SYN/ACK packet and ecn_syn_wait_ == 2
			{
	    		        if ( fh->ce() ) 
                                // If SYN/ACK packet is CE-marked
				{
					//cancel_rtx_timer();
					//newack(pkt);
					set_rtx_timer();
					sendpacket(t_seqno_, rcv_nxt_, TH_ACK|TH_ECE, 0, 0);
					goto drop;
				}
	    		}
		}


#ifdef notdef
cancel_rtx_timer();	// cancel timer on our 1st SYN [does this belong!?]
#endif
		irs_ = tcph->seqno();	// get initial recv'd seq #
		rcv_nxt_ = rcvseqinit(irs_, datalen);

		if (tiflags & TH_ACK) {
			// SYN+ACK (our SYN was acked)
                        if (ecn_ && fh->ecnecho() && !fh->cong_action()) {
                                ect_ = TRUE;
	    		        if ( fh->ce() ) 
	    				recent_ce_ = TRUE;
	    		}
			highest_ack_ = ackno;
			cwnd_ = initial_window();

#ifdef notdef
/*
 * if we didn't have to retransmit the SYN,
 * use its rtt as our initial srtt & rtt var.
 */
if (t_rtt_) {
	double tao = now() - tcph->ts();
	rtt_update(tao);
}
#endif

			/*
			 * if there's data, delay ACK; if there's also a FIN
			 * ACKNOW will be turned on later.
			 */
			if (datalen > 0) {
				flags_ |= TF_DELACK;	// data there: wait
			} else {
				flags_ |= TF_ACKNOW;	// ACK peer's SYN
			}

                        /*
                         * Received <SYN,ACK> in SYN_SENT[*] state.
                         * Transitions:
                         *      SYN_SENT  --> ESTABLISHED
                         *      SYN_SENT* --> FIN_WAIT_1
                         */

			if (flags_ & TF_NEEDFIN) {
				newstate(TCPS_FIN_WAIT_1);
				flags_ &= ~TF_NEEDFIN;
				tiflags &= ~TH_SYN;
			} else {
				newstate(TCPS_ESTABLISHED);
			}

			// special to ns:
			//  generate pure ACK here.
			//  this simulates the ordinary connection establishment
			//  where the ACK of the peer's SYN+ACK contains
			//  no data.  This is typically caused by the way
			//  the connect() socket call works in which the
			//  entire 3-way handshake occurs prior to the app
			//  being able to issue a write() [which actually
			//  causes the segment to be sent].
			sendpacket(t_seqno_, rcv_nxt_, TH_ACK, 0, 0);
		} else {
			// Check ECN-SYN packet
                        if (ecn_ && fh->ecnecho() && fh->cong_action())
                                ect_ = TRUE;

			// SYN (no ACK) (simultaneous active opens)
			flags_ |= TF_ACKNOW;
			cancel_rtx_timer();
			newstate(TCPS_SYN_RECEIVED);
			/*
			 * decrement t_seqno_: we are sending a
			 * 2nd SYN (this time in the form of a
			 * SYN+ACK, so t_seqno_ will have been
			 * advanced to 2... reduce this
			 */
			t_seqno_--;	// CHECKME
		}

trimthenstep6:
		/*
		 * advance the seq# to correspond to first data byte
		 */
		tcph->seqno()++;

		if (tiflags & TH_ACK)
			goto process_ACK;

		goto step6;

	case TCPS_LAST_ACK:
		/* 
		 * The only way we're in LAST_ACK is if we've already
		 * received a FIN, so ignore all retranmitted FINS.
		 * -M. Weigle 7/23/02
		 */
		if (tiflags & TH_FIN) {
			goto drop;
		}
		break;
	case TCPS_CLOSING:
		break;
	} /* end switch(state_) */

        /*
         * States other than LISTEN or SYN_SENT.
         * First check timestamp, if present.
         * Then check that at least some bytes of segment are within
         * receive window.  If segment begins before rcv_nxt,
         * drop leading data (and SYN); if nothing left, just ack.
         *
         * RFC 1323 PAWS: If we have a timestamp reply on this segment
         * and it's less than ts_recent, drop it.
         */

	if (ts_option_ && !fh->no_ts_ && recent_ && tcph->ts() < recent_) {
		if ((now() - recent_age_) > TCP_PAWS_IDLE) {
			/*
			 * this is basically impossible in the simulator,
			 * but here it is...
			 */
                        /*
                         * Invalidate ts_recent.  If this segment updates
                         * ts_recent, the age will be reset later and ts_recent
                         * will get a valid value.  If it does not, setting
                         * ts_recent to zero will at least satisfy the
                         * requirement that zero be placed in the timestamp
                         * echo reply when ts_recent isn't valid.  The
                         * age isn't reset until we get a valid ts_recent
                         * because we don't want out-of-order segments to be
                         * dropped when ts_recent is old.
                         */
			recent_ = 0.0;
		} else {
			fprintf(stderr, "%f: FullTcpAgent(%s): dropped pkt due to bad ts\n",
				now(), name());
			goto dropafterack;
		}
	}

	// check for redundant data at head/tail of segment
	//	note that the 4.4bsd [Net/3] code has
	//	a bug here which can cause us to ignore the
	//	perfectly good ACKs on duplicate segments.  The
	//	fix is described in (Stevens, Vol2, p. 959-960).
	//	This code is based on that correction.
	//
	// In addition, it has a modification so that duplicate segments
	// with dup acks don't trigger a fast retransmit when dupseg_fix_
	// is enabled.
	//
	// Yet one more modification: make sure that if the received
	//	segment had datalen=0 and wasn't a SYN or FIN that
	//	we don't turn on the ACKNOW status bit.  If we were to
	//	allow ACKNOW to be turned on, normal pure ACKs that happen
	//	to have seq #s below rcv_nxt can trigger an ACK war by
	//	forcing us to ACK the pure ACKs
	//
	// Update: if we have a dataless FIN, don't really want to
	// do anything with it.  In particular, would like to
	// avoid ACKing an incoming FIN+ACK while in CLOSING
	//
	todrop = rcv_nxt_ - tcph->seqno();  // how much overlap?

	if (todrop > 0 && ((tiflags & (TH_SYN)) || datalen > 0)) {
//printf("%f(%s): trim 1..todrop:%d, dlen:%d\n",now(), name(), todrop, datalen);
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			tcph->seqno()++;
			th->size()--;	// XXX Must decrease packet size too!!
					// Q: Why?.. this is only a SYN
			todrop--;
		}
		//
		// see Stevens, vol 2, p. 960 for this check;
		// this check is to see if we are dropping
		// more than this segment (i.e. the whole pkt + a FIN),
		// or just the whole packet (no FIN)
		//
		if ((todrop > datalen) ||
		    (todrop == datalen && ((tiflags & TH_FIN) == 0))) {
//printf("%f(%s): trim 2..todrop:%d, dlen:%d\n",now(), name(), todrop, datalen);
                        /*
                         * Any valid FIN must be to the left of the window.
                         * At this point the FIN must be a duplicate or out
                         * of sequence; drop it.
                         */

			tiflags &= ~TH_FIN;

			/*
			 * Send an ACK to resynchronize and drop any data.
			 * But keep on processing for RST or ACK.
			 */

			flags_ |= TF_ACKNOW;
			todrop = datalen;
			dupseg = TRUE;	// *completely* duplicate

		}

		/*
		 * Trim duplicate data from the front of the packet
		 */

		tcph->seqno() += todrop;
		th->size() -= todrop;	// XXX Must decrease size too!!
					// why? [kf]..prob when put in RQ
		datalen -= todrop;

	} /* data trim */

	/*
	 * If we are doing timstamps and this packet has one, and
	 * If last ACK falls within this segment's sequence numbers,
	 * record the timestamp.
	 * See RFC1323 (now RFC1323 bis)
	 */
	if (ts_option_ && !fh->no_ts_ && tcph->seqno() <= last_ack_sent_) {
		/*
		 * this is the case where the ts value is newer than
		 * the last one we've seen, and the seq # is the one we expect
		 * [seqno == last_ack_sent_] or older
		 */
		recent_age_ = now();
		recent_ = tcph->ts();
	}

	if (tiflags & TH_SYN) {
                if (debug_) {
		        fprintf(stderr, "%f: FullTcpAgent::recv(%s) received unexpected SYN (state:%d): ",
		        now(), name(), state_);
		        prpkt(pkt);
                }
		goto dropwithreset;
	}

	if ((tiflags & TH_ACK) == 0) {
		/*
		 * Added check for state != SYN_RECEIVED.  We will receive a 
		 * duplicate SYN in SYN_RECEIVED when our SYN/ACK was dropped.
		 * We should just ignore the duplicate SYN (our timeout for 
		 * resending the SYN/ACK is about the same as the client's 
		 * timeout for resending the SYN), but give no error message. 
		 * -M. Weigle 07/24/01
		 */
		if (state_ != TCPS_SYN_RECEIVED) {
                        if (debug_) {
			        fprintf(stderr, "%f: FullTcpAgent::recv(%s) got packet lacking ACK (state:%d): ",
				now(), name(), state_);
			        prpkt(pkt);
                        }
		}
		goto drop;
	}

	/*
	 * Ack processing.
	 */

	switch (state_) {
	case TCPS_SYN_RECEIVED:	/* want ACK for our SYN+ACK */
		if (ackno < highest_ack_ || ackno > maxseq_) {
			// not in useful range
                        if (debug_) {
		    	        fprintf(stderr, "%f: FullTcpAgent(%s): ack(%d) not in range while in SYN_RECEIVED: ",
			 	now(), name(), ackno);
			        prpkt(pkt);
                        }
			goto dropwithreset;
		}

		if (ecn_ && ect_ && ecn_syn_ && fh->ecnecho() && ecn_syn_wait_ == 2) 
		{
		// The SYN/ACK packet was ECN-marked.
		// Reset the rtx timer, send another SYN/ACK packet
                //  immediately, and drop the ACK packet.
                // Do not move to TCPS_ESTB state or update TCP variables.
			cancel_rtx_timer();
			ecn_syn_next_ = 0;
			foutput(iss_, REASON_NORMAL);
			wnd_init_option_ = 1;
                        wnd_init_ = 1;
			goto drop;
		} 
		if (ecn_ && ect_ && ecn_syn_ && fh->ecnecho() && ecn_syn_wait_ < 2) {
		// The SYN/ACK packet was ECN-marked.
			if (ecn_syn_wait_ == 1) {
				// A timer will be called in ecn().
				cwnd_ = 1;
				use_rtt_ = 1; //KK, wait for timeout() period
			} else {
			        // Congestion window will be halved in ecn().
				cwnd_ = 2;
			}
		} else  {
			cwnd_ = initial_window();
		}
	
                /*
                 * Make transitions:
                 *      SYN-RECEIVED  -> ESTABLISHED
                 *      SYN-RECEIVED* -> FIN-WAIT-1
                 */
                if (flags_ & TF_NEEDFIN) {
			newstate(TCPS_FIN_WAIT_1);
                        flags_ &= ~TF_NEEDFIN;
                } else {
                        newstate(TCPS_ESTABLISHED);
                }

		/* fall into ... */


        /*
         * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
         * ACKs.  If the ack is in the range
         *      tp->snd_una < ti->ti_ack <= tp->snd_max
         * then advance tp->snd_una to ti->ti_ack and drop
         * data from the retransmission queue.
	 *
	 * note that state TIME_WAIT isn't used
	 * in the simulator
         */

        case TCPS_ESTABLISHED:
        case TCPS_FIN_WAIT_1:
        case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
        case TCPS_CLOSING:
        case TCPS_LAST_ACK:

		//
		// look for ECNs in ACKs, react as necessary
		//

		if (fh->ecnecho() && (!ecn_ || !ect_)) {
			fprintf(stderr,
			    "%f: FullTcp(%s): warning, recvd ecnecho but I am not ECN capable!\n",
				now(), name());
		}

                //
                // generate a stream of ecnecho bits until we see a true
                // cong_action bit
                // 
                if (ecn_) {
                        if (fh->ce() && fh->ect())
                                recent_ce_ = TRUE;
                        else if (fh->cwr()) 
                                recent_ce_ = FALSE;
                }

		//
		// If ESTABLISHED or starting to close, process SACKS
		//

		if (state_ >= TCPS_ESTABLISHED && tcph->sa_length() > 0) {
			process_sack(tcph);
		}

		//
		// ACK indicates packet left the network
		//	try not to be fooled by data
		//

		if (fastrecov_ && (datalen == 0 || ackno > highest_ack_))
			pipe_ -= maxseg_;

		// look for dup ACKs (dup ack numbers, no data)
		//
		// do fast retransmit/recovery if at/past thresh
		if (ackno <= highest_ack_) {
			// a pure ACK which doesn't advance highest_ack_
			if (datalen == 0 && (!dupseg_fix_ || !dupseg)) {

                                /*
                                 * If we have outstanding data
                                 * this is a completely
                                 * duplicate ack,
                                 * the ack is the biggest we've
                                 * seen and we've seen exactly our rexmt
                                 * threshhold of them, assume a packet
                                 * has been dropped and retransmit it.
                                 *
                                 * We know we're losing at the current
                                 * window size so do congestion avoidance.
                                 *
                                 * Dup acks mean that packets have left the
                                 * network (they're now cached at the receiver)
                                 * so bump cwnd by the amount in the receiver
                                 * to keep a constant cwnd packets in the
                                 * network.
                                 */

				if ((rtx_timer_.status() != TIMER_PENDING) ||
				    ackno < highest_ack_) {
					// Q: significance of timer not pending?
					// ACK below highest_ack_
					oldack();
				} else if (++dupacks_ == tcprexmtthresh_) {
					// ACK at highest_ack_ AND meets threshold
					//trace_event("FAST_RECOVERY");
					dupack_action(); // maybe fast rexmt
					goto drop;

				} else if (dupacks_ > tcprexmtthresh_) {
					// ACK at highest_ack_ AND above threshole
					//trace_event("FAST_RECOVERY");
					extra_ack();

					// send whatever window allows
					send_much(0, REASON_DUPACK, maxburst_);
					goto drop;
				}
			} else {
				// non zero-length [dataful] segment
				// with a dup ack (normal for dataful segs)
				// (or window changed in real TCP).
				if (dupack_reset_) {
					dupacks_ = 0;
					fastrecov_ = FALSE;
				}
			}
			break;	/* take us to "step6" */
		} /* end of dup/old acks */

		/*
		 * we've finished the fast retransmit/recovery period
		 * (i.e. received an ACK which advances highest_ack_)
		 * The ACK may be "good" or "partial"
		 */

process_ACK:

		if (ackno > maxseq_) {
			// ack more than we sent(!?)
                        if (debug_) {
			        fprintf(stderr, "%f: FullTcpAgent::recv(%s) too-big ACK (maxseq:%d): ",
				now(), name(), int(maxseq_));
			        prpkt(pkt);
                        }
			goto dropafterack;
		}

                /*
                 * If we have a timestamp reply, update smoothed
                 * round trip time.  If no timestamp is present but
                 * transmit timer is running and timed sequence
                 * number was acked, update smoothed round trip time.
                 * Since we now have an rtt measurement, cancel the
                 * timer backoff (cf., Phil Karn's retransmit alg.).
                 * Recompute the initial retransmit timer.
		 *
                 * If all outstanding data is acked, stop retransmit
                 * If there is more data to be acked, restart retransmit
                 * timer, using current (possibly backed-off) value.
                 */
		newack(pkt);	// handle timers, update highest_ack_

		/*
		 * if this is a partial ACK, invoke whatever we should
		 * note that newack() must be called before the action
		 * functions, as some of them depend on side-effects
		 * of newack()
		 */

		int partial = pack(pkt);

		if (partial)
			pack_action(pkt);
		else
			ack_action(pkt);

		/*
		 * if this is an ACK with an ECN indication, handle this
		 * but not if it is a syn packet
		 */
		if (fh->ecnecho() && !(tiflags&TH_SYN) )
		if (fh->ecnecho()) {
			// Note from Sally: In one-way TCP,
			// ecn() is called before newack()...
			ecn(highest_ack_);  // updated by newack(), above
			// "set_rtx_timer();" from T. Kelly.
			if (cwnd_ < 1)
			 	set_rtx_timer();
		}
		// CHECKME: handling of rtx timer
		if (ackno == maxseq_) {
			needoutput = TRUE;
		}

		/*
		 * If no data (only SYN) was ACK'd,
		 *    skip rest of ACK processing.
		 */
		if (ackno == (highest_ack_ + 1))
			goto step6;

		// if we are delaying initial cwnd growth (probably due to
		// large initial windows), then only open cwnd if data has
		// been received
		// Q: check when this happens
                /*
                 * When new data is acked, open the congestion window.
                 * If the window gives us less than ssthresh packets
                 * in flight, open exponentially (maxseg per packet).
                 * Otherwise open about linearly: maxseg per window
                 * (maxseg^2 / cwnd per packet).
                 */
		if ((!delay_growth_ || (rcv_nxt_ > 0)) &&
		    last_state_ == TCPS_ESTABLISHED) {
			if (!partial || open_cwnd_on_pack_) {
                           if (!ect_ || !hdr_flags::access(pkt)->ecnecho())
				opencwnd();
                        }
		}

		if ((state_ >= TCPS_FIN_WAIT_1) && (ackno == maxseq_)) {
			ourfinisacked = TRUE;
		}

		//
		// special additional processing when our state
		// is one of the closing states:
		//	FIN_WAIT_1, CLOSING, LAST_ACK

		switch (state_) {
                /*
                 * In FIN_WAIT_1 STATE in addition to the processing
                 * for the ESTABLISHED state if our FIN is now acknowledged
                 * then enter FIN_WAIT_2.
                 */
		case TCPS_FIN_WAIT_1:	/* doing active close */
			if (ourfinisacked) {
				// got the ACK, now await incoming FIN
				newstate(TCPS_FIN_WAIT_2);
				cancel_timers();
				needoutput = FALSE;
			}
			break;

                /*
                 * In CLOSING STATE in addition to the processing for
                 * the ESTABLISHED state if the ACK acknowledges our FIN
                 * then enter the TIME-WAIT state, otherwise ignore
                 * the segment.
                 */
		case TCPS_CLOSING:	/* simultaneous active close */;
			if (ourfinisacked) {
				newstate(TCPS_CLOSED);
				cancel_timers();
			}
			break;
                /*
                 * In LAST_ACK, we may still be waiting for data to drain
                 * and/or to be acked, as well as for the ack of our FIN.
                 * If our FIN is now acknowledged,
                 * enter the closed state and return.
                 */
		case TCPS_LAST_ACK:	/* passive close */
			// K: added state change here
			if (ourfinisacked) {
				newstate(TCPS_CLOSED);
				finish(); // cancels timers, erc
				reset(); // for connection re-use (bug fix from ns-users list)
				goto drop;
			} else {
				// should be a FIN we've seen
                                if (debug_) {
                                        fprintf(stderr, "%f: FullTcpAgent(%s)::received non-ACK (state:%d): ",
                                        now(), name(), state_);
				        prpkt(pkt);
                                }
                        }
			break;

		/* no case for TIME_WAIT in simulator */
		}  // inner state_ switch (closing states)
	} // outer state_ switch (ack processing)

step6:

	/*
	 * Processing of incoming DATAful segments.
	 * 	Code above has already trimmed redundant data.
	 *
	 * real TCP handles window updates and URG data here also
	 */

/* dodata: this label is in the "real" code.. here only for reference */

	if ((datalen > 0 || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(state_) == 0) {

		//
		// the following 'if' implements the "real" TCP
		// TCP_REASS macro
		//

		if (tcph->seqno() == rcv_nxt_ && rq_.empty()) {
			// got the in-order packet we were looking
			// for, nobody is in the reassembly queue,
			// so this is the common case...
			// note: in "real" TCP we must also be in
			// ESTABLISHED state to come here, because
			// data arriving before ESTABLISHED is
			// queued in the reassembly queue.  Since we
			// don't really have a process anyhow, just
			// accept the data here as-is (i.e. don't
			// require being in ESTABLISHED state)
			flags_ |= TF_DELACK;
			rcv_nxt_ += datalen;
			tiflags = tcph->flags() & TH_FIN;

			// give to "application" here
			// in "real" TCP, this is sbappend() + sorwakeup()
			if (datalen)
				recvBytes(datalen); // notify app. of "delivery"
			needoutput = need_send();
		} else {
			// see the "tcp_reass" function:
			// not the one we want next (or it
			// is but there's stuff on the reass queue);
			// do whatever we need to do for out-of-order
			// segments or hole-fills.  Also,
			// send an ACK (or SACK) to the other side right now.
			// Note that we may have just a FIN here (datalen = 0)
			int rcv_nxt_old_ = rcv_nxt_; // notify app. if changes
			tiflags = reass(pkt);
			if (rcv_nxt_ > rcv_nxt_old_) {
				// if rcv_nxt_ has advanced, must have
				// been a hole fill.  In this case, there
				// is something to give to application
				recvBytes(rcv_nxt_ - rcv_nxt_old_);
			}
			flags_ |= TF_ACKNOW;

			if (tiflags & TH_PUSH) {
				//
				// ???: does this belong here
				// K: APPLICATION recv
				needoutput = need_send();
			}
		}
	} else {
		/*
		 * we're closing down or this is a pure ACK that
		 * wasn't handled by the header prediction part above
		 * (e.g. because cwnd < wnd)
		 */
		// K: this is deleted
		tiflags &= ~TH_FIN;
	}

	/*
	 * if FIN is received, ACK the FIN
	 * (let user know if we could do so)
	 */

	if (tiflags & TH_FIN) {
		if (TCPS_HAVERCVDFIN(state_) == 0) {
			flags_ |= TF_ACKNOW;
 			rcv_nxt_++;
		}
		switch (state_) {
                /*
                 * In SYN_RECEIVED and ESTABLISHED STATES
                 * enter the CLOSE_WAIT state.
		 * (passive close)
                 */
                case TCPS_SYN_RECEIVED:
                case TCPS_ESTABLISHED:
			newstate(TCPS_CLOSE_WAIT);
                        break;

                /*
                 * If still in FIN_WAIT_1 STATE FIN has not been acked so
                 * enter the CLOSING state.
		 * (simultaneous close)
                 */
                case TCPS_FIN_WAIT_1:
			newstate(TCPS_CLOSING);
                        break;
                /*
                 * In FIN_WAIT_2 state enter the TIME_WAIT state,
                 * starting the time-wait timer, turning off the other
                 * standard timers.
		 * (in the simulator, just go to CLOSED)
		 * (completion of active close)
                 */
                case TCPS_FIN_WAIT_2:
                        newstate(TCPS_CLOSED);
			cancel_timers();
                        break;
		}
	} /* end of if FIN bit on */

	if (needoutput || (flags_ & TF_ACKNOW))
		send_much(1, REASON_NORMAL, maxburst_);
	else if (curseq_ >= highest_ack_ || infinite_send_)
		send_much(0, REASON_NORMAL, maxburst_);
	// K: which state to return to when nothing left?

	if (!halfclose_ && state_ == TCPS_CLOSE_WAIT && highest_ack_ == maxseq_)
		usrclosed();

	Packet::free(pkt);

	// haoboy: Is here the place for done{} of active close? 
	// It cannot be put in the switch above because we might need to do
	// send_much() (an ACK)
	if (state_ == TCPS_CLOSED) 
		Tcl::instance().evalf("%s done", this->name());

	return;

	//
	// various ways of dropping (some also ACK, some also RST)
	//

dropafterack:
	flags_ |= TF_ACKNOW;
	send_much(1, REASON_NORMAL, maxburst_);
	goto drop;

dropwithreset:
	/* we should be sending an RST here, but can't in simulator */
	if (tiflags & TH_ACK) {
		sendpacket(ackno, 0, 0x0, 0, REASON_NORMAL);
	} else {
		int ack = tcph->seqno() + datalen;
		if (tiflags & TH_SYN)
			ack--;
		sendpacket(0, ack, TH_ACK, 0, REASON_NORMAL);
	}
drop:
   	Packet::free(pkt);
	return;
}

/*  
 * Dupack-action: what to do on a DUP ACK.  After the initial check
 * of 'recover' below, this function implements the following truth
 * table:
 *  
 *      bugfix  ecn     last-cwnd == ecn        action  
 *  
 *      0       0       0                       full_reno_action
 *      0       0       1                       full_reno_action [impossible]
 *      0       1       0                       full_reno_action
 *      0       1       1                       1/2 window, return 
 *      1       0       0                       nothing 
 *      1       0       1                       nothing         [impossible]
 *      1       1       0                       nothing 
 *      1       1       1                       1/2 window, return
 */ 
    
void
FullTcpAgent::dupack_action()
{   

        int recovered = (highest_ack_ > recover_);

	fastrecov_ = TRUE;
	rtxbytes_ = 0;

        if (recovered || (!bug_fix_ && !ecn_) 
            || (last_cwnd_action_ == CWND_ACTION_DUPACK)
            || ( highest_ack_ == 0)) {
                goto full_reno_action;
        }       
    
        if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
                slowdown(CLOSE_CWND_HALF);
		cancel_rtx_timer();
		rtt_active_ = FALSE;
		(void)fast_retransmit(highest_ack_);
                return; 
        }      
    
        if (bug_fix_) {
                /*
                 * The line below, for "bug_fix_" true, avoids
                 * problems with multiple fast retransmits in one
                 * window of data.
                 */      
                return;  
        }
    
full_reno_action:    
        slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
	cancel_rtx_timer();
	rtt_active_ = FALSE;
	recover_ = maxseq_;
	(void)fast_retransmit(highest_ack_);
	// we measure cwnd in packets,
	// so don't scale by maxseg_
	// as real TCP does
	cwnd_ = double(ssthresh_) + double(dupacks_);
        return;
}

void
FullTcpAgent::timeout_action()
{
	recover_ = maxseq_;

	if (cwnd_ < 1.0) {
                if (debug_) {
	            fprintf(stderr, "%f: FullTcpAgent(%s):: resetting cwnd from %f to 1\n",
		    now(), name(), double(cwnd_));
                }
		cwnd_ = 1.0;
	}

	if (last_cwnd_action_ == CWND_ACTION_ECN) {
		slowdown(CLOSE_CWND_ONE);
	} else {
		slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
	}
	reset_rtx_timer(1);
	t_seqno_ = (highest_ack_ < 0) ? iss_ : int(highest_ack_);
	fastrecov_ = FALSE;
	dupacks_ = 0;
}
/*
 * deal with timers going off.
 * 2 types for now:
 *	retransmission timer (rtx_timer_)
 *  delayed ack timer (delack_timer_)
 *	delayed send (randomization) timer (delsnd_timer_)
 *
 * real TCP initializes the RTO as 6 sec
 *	(A + 2D, where A=0, D=3), [Stevens p. 305]
 * and thereafter uses
 *	(A + 4D, where A and D are dynamic estimates)
 *
 * note that in the simulator t_srtt_, t_rttvar_ and t_rtt_
 * are all measured in 'tcp_tick_'-second units
 */

void
FullTcpAgent::timeout(int tno)
{

	/*
	 * Due to F. Hernandez-Campos' fix in recv(), we may send an ACK
	 * while in the CLOSED state.  -M. Weigle 7/24/01
	 */
	if (state_ == TCPS_LISTEN) {
	 	// shouldn't be getting timeouts here
                if (debug_) {
		        fprintf(stderr, "%f: FullTcpAgent(%s): unexpected timeout %d in state %s\n",
			now(), name(), tno, statestr(state_));
                }
		return;
	}

	switch (tno) {

	case TCP_TIMER_RTX:
                /* retransmit timer */
                ++nrexmit_;
                timeout_action();
		/* fall thru */
	case TCP_TIMER_DELSND:
		/* for phase effects */
                send_much(1, PF_TIMEOUT, maxburst_);
		break;

	case TCP_TIMER_DELACK:
                if (flags_ & TF_DELACK) {
                        flags_ &= ~TF_DELACK;
                        flags_ |= TF_ACKNOW;
                        send_much(1, REASON_NORMAL, 0);
                }
                delack_timer_.resched(delack_interval_);
		break;
	default:
		fprintf(stderr, "%f: FullTcpAgent(%s) Unknown Timeout type %d\n",
			now(), name(), tno);
	}
	return;
}

void
FullTcpAgent::dooptions(Packet* pkt)
{
	// interesting options: timestamps (here),
	//	CC, CCNEW, CCECHO (future work perhaps?)

        hdr_flags *fh = hdr_flags::access(pkt);
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	if (ts_option_ && !fh->no_ts_) {
		if (tcph->ts() < 0.0) {
			fprintf(stderr,
			    "%f: FullTcpAgent(%s) warning: ts_option enabled in this TCP, but appears to be disabled in peer\n",
				now(), name());
		} else if (tcph->flags() & TH_SYN) {
			flags_ |= TF_RCVD_TSTMP;
			recent_ = tcph->ts();
			recent_age_ = now();
		}
	}

	return;
}

//
// this shouldn't ever happen
//
void
FullTcpAgent::process_sack(hdr_tcp*)
{
	fprintf(stderr, "%f: FullTcpAgent(%s) Non-SACK capable FullTcpAgent received a SACK\n",
		now(), name());
	return;
}


/*
 * ****** Tahoe ******
 *
 * for TCP Tahoe, we force a slow-start as the dup ack
 * action.  Also, no window inflation due to multiple dup
 * acks.  The latter is arranged by setting reno_fastrecov_
 * false [which is performed by the Tcl init function for Tahoe in
 * ns-default.tcl].
 */

/* 
 * Tahoe
 * Dupack-action: what to do on a DUP ACK.  After the initial check
 * of 'recover' below, this function implements the following truth
 * table:
 * 
 *      bugfix  ecn     last-cwnd == ecn        action  
 * 
 *      0       0       0                       full_tahoe_action
 *      0       0       1                       full_tahoe_action [impossible]
 *      0       1       0                       full_tahoe_action
 *      0       1       1                       1/2 window, return
 *      1       0       0                       nothing 
 *      1       0       1                       nothing         [impossible]
 *      1       1       0                       nothing 
 *      1       1       1                       1/2 window, return
 */

void
TahoeFullTcpAgent::dupack_action()
{  
        int recovered = (highest_ack_ > recover_);

	fastrecov_ = TRUE;
	rtxbytes_ = 0;

        if (recovered || (!bug_fix_ && !ecn_) || highest_ack_ == 0) {
                goto full_tahoe_action;
        }
   
        if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		// slow start on ECN
		last_cwnd_action_ = CWND_ACTION_DUPACK;
                slowdown(CLOSE_CWND_ONE);
		set_rtx_timer();
                rtt_active_ = FALSE;
		t_seqno_ = highest_ack_;
                return; 
        }
   
        if (bug_fix_) {
                /*
                 * The line below, for "bug_fix_" true, avoids
                 * problems with multiple fast retransmits in one
                 * window of data.
                 */      
                return;  
        }
   
full_tahoe_action:
	// slow-start and reset ssthresh
	trace_event("FAST_RETX");
	recover_ = maxseq_;
	last_cwnd_action_ = CWND_ACTION_DUPACK;
        slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_ONE);	// cwnd->1
	set_rtx_timer();
        rtt_active_ = FALSE;
	t_seqno_ = highest_ack_;
	send_much(0, REASON_NORMAL, 0);
        return; 
}  

/*
 * ****** Newreno ******
 *
 * for NewReno, a partial ACK does not exit fast recovery,
 * and does not reset the dup ACK counter (which might trigger fast
 * retransmits we don't want).  In addition, the number of packets
 * sent in response to an ACK is limited to recov_maxburst_ during
 * recovery periods.
 */

NewRenoFullTcpAgent::NewRenoFullTcpAgent() : save_maxburst_(-1)
{
	bind("recov_maxburst_", &recov_maxburst_);
}

void
NewRenoFullTcpAgent::pack_action(Packet*)
{
	(void)fast_retransmit(highest_ack_);
	cwnd_ = double(ssthresh_);
	if (save_maxburst_ < 0) {
		save_maxburst_ = maxburst_;
		maxburst_ = recov_maxburst_;
	}
	return;
}

void
NewRenoFullTcpAgent::ack_action(Packet* p)
{
	if (save_maxburst_ >= 0) {
		maxburst_ = save_maxburst_;
		save_maxburst_ = -1;
	}
	FullTcpAgent::ack_action(p);
	return;
}

/*
 *
 * ****** SACK ******
 *
 * for Sack, receiver part must report SACK data
 * sender part maintains a 'scoreboard' (sq_) that
 * records what it hears from receiver
 * sender fills holes during recovery and obeys
 * "pipe" style control until recovery is complete
 */

void
SackFullTcpAgent::reset()
{
	sq_.clear();			// no SACK blocks
	/* Fixed typo.  -M. Weigle 6/17/02 */
	sack_min_ = h_seqno_ = -1;	// no left edge of SACK blocks
	FullTcpAgent::reset();
}


int
SackFullTcpAgent::hdrsize(int nsackblocks)
{
	int total = FullTcpAgent::headersize();
	// use base header size plus SACK option size
        if (nsackblocks > 0) {
                total += ((nsackblocks * sack_block_size_)
                        + sack_option_size_);
	}
	return (total);
}

void
SackFullTcpAgent::dupack_action()
{

        int recovered = (highest_ack_ > recover_);

	fastrecov_ = TRUE;
	rtxbytes_ = 0;
	pipe_ = maxseq_ - highest_ack_ - sq_.total();

//printf("%f: SACK DUPACK-ACTION:pipe_:%d, sq-total:%d, bugfix:%d, cwnd:%d, highest_ack:%d, recover_:%d\n",
//now(), pipe_, sq_.total(), bug_fix_, int(cwnd_), int(highest_ack_), recover_);

        if (recovered || (!bug_fix_ && !ecn_)) {
                goto full_sack_action;
        }           

        if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		/* 
		 * Received ECN notification and 3 DUPACKs in same 
		 * window. Don't cut cwnd again, but retransmit lost
		 * packet.   -M. Weigle  6/19/02
		 */
		last_cwnd_action_ = CWND_ACTION_DUPACK;
		cancel_rtx_timer();
		rtt_active_ = FALSE;
		int amt = fast_retransmit(highest_ack_);
		pipectrl_ = TRUE;
		h_seqno_ = highest_ack_ + amt;
		send_much(0, REASON_DUPACK, maxburst_);
		return; 
	}
   
        if (bug_fix_) {
                /*                              
                 * The line below, for "bug_fix_" true, avoids
                 * problems with multiple fast retransmits in one
                 * window of data.
                 */      

//printf("%f: SACK DUPACK-ACTION BUGFIX RETURN:pipe_:%d, sq-total:%d, bugfix:%d, cwnd:%d\n",
//now(), pipe_, sq_.total(), bug_fix_, int(cwnd_));
                return;  
        }
   
full_sack_action:                               
	trace_event("FAST_RECOVERY");
        slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
        cancel_rtx_timer();
        rtt_active_ = FALSE;

	// these initiate SACK-style "pipe" recovery
	pipectrl_ = TRUE;
	recover_ = maxseq_;	// where I am when recovery starts

	int amt = fast_retransmit(highest_ack_);
	h_seqno_ = highest_ack_ + amt;

//printf("%f: FAST-RTX seq:%d, h_seqno_ is now:%d, pipe:%d, cwnd:%d, recover:%d\n",
//now(), int(highest_ack_), h_seqno_, pipe_, int(cwnd_), recover_);

	send_much(0, REASON_DUPACK, maxburst_);

        return;
}

void
SackFullTcpAgent::pack_action(Packet*)
{
	if (!sq_.empty() && sack_min_ < highest_ack_) {
		sack_min_ = highest_ack_;
		sq_.cleartonxt();
	}
	pipe_ -= maxseg_;	// see comment in tcp-sack1.cc
	if (h_seqno_ < highest_ack_)
		h_seqno_ = highest_ack_;
}

void
SackFullTcpAgent::ack_action(Packet*)
{
//printf("%f: EXITING fast recovery, recover:%d\n",
//now(), recover_);
	fastrecov_ = pipectrl_ = FALSE;
        if (!sq_.empty() && sack_min_ < highest_ack_) {
                sack_min_ = highest_ack_;
                sq_.cleartonxt();
        }
	dupacks_ = 0;

	/* 
	 * Update h_seqno_ on new ACK (same as for partial ACKS)
	 * -M. Weigle 6/3/05
	 */
	if (h_seqno_ < highest_ack_)
		h_seqno_ = highest_ack_;
}

//
// receiver side: if there are things in the reassembly queue,
// build the appropriate SACK blocks to carry in the SACK
//
int
SackFullTcpAgent::build_options(hdr_tcp* tcph)
{
	int total = FullTcpAgent::build_options(tcph);

        if (!rq_.empty()) {
                int nblk = rq_.gensack(&tcph->sa_left(0), max_sack_blocks_);
                tcph->sa_length() = nblk;
		total += (nblk * sack_block_size_) + sack_option_size_;
        } else {
                tcph->sa_length() = 0;
        }
	return (total);
}

void
SackFullTcpAgent::timeout_action()
{
	FullTcpAgent::timeout_action();

	//
	// original SACK spec says the sender is
	// supposed to clear out its knowledge of what
	// the receiver has in the case of a timeout
	// (on the chance the receiver has renig'd).
	// Here, this happens when clear_on_timeout_ is
	// enabled.
	//

	if (clear_on_timeout_) {
		sq_.clear();
		sack_min_ = highest_ack_;
	}

	return;
}

void
SackFullTcpAgent::process_sack(hdr_tcp* tcph)
{
	//
	// Figure out how many sack blocks are
	// in the pkt.  Insert each block range
	// into the scoreboard
	//

	if (max_sack_blocks_ <= 0) {
		fprintf(stderr,
		    "%f: FullTcpAgent(%s) warning: received SACK block but I am not SACK enabled\n",
			now(), name());
		return;
	}	

	int slen = tcph->sa_length(), i;
	for (i = 0; i < slen; ++i) {
		/* Added check for FIN   -M. Weigle 5/21/02 */
		if (((tcph->flags() & TH_FIN) == 0) && 
		    tcph->sa_left(i) >= tcph->sa_right(i)) {
			fprintf(stderr,
			    "%f: FullTcpAgent(%s) warning: received illegal SACK block [%d,%d]\n",
				now(), name(), tcph->sa_left(i), tcph->sa_right(i));
			continue;
		}
		sq_.add(tcph->sa_left(i), tcph->sa_right(i), 0);  
	}

	return;
}

int
SackFullTcpAgent::send_allowed(int seq)
{
	// not in pipe control, so use regular control
	if (!pipectrl_)
		return (FullTcpAgent::send_allowed(seq));

	// don't overshoot receiver's advertised window
	int topawin = highest_ack_ + int(wnd_) * maxseg_;
	if (seq >= topawin) {
//printf("%f: SEND(%d) NOT ALLOWED DUE TO AWIN:%d, pipe:%d, cwnd:%d\n",
//now(), seq, topawin, pipe_, int(cwnd_));
		return FALSE;
	}

	/*
	 * If not in ESTABLISHED, don't send anything we don't have
	 *   -M. Weigle 7/18/02
	 */
	if (state_ != TCPS_ESTABLISHED && seq > curseq_)
		return FALSE;

	// don't overshoot cwnd_
	int cwin = int(cwnd_) * maxseg_;
	return (pipe_ < cwin);
}


//
// Calculate the next seq# to send by send_much.  If we are recovering and
// we have learned about data cached at the receiver via a SACK,
// we may want something other than new data (t_seqno)
//

int
SackFullTcpAgent::nxt_tseq()
{

	int in_recovery = (highest_ack_ < recover_);
	int seq = h_seqno_;

	if (!in_recovery) {
//if (int(t_seqno_) > 1)
//printf("%f: non-recovery nxt_tseq called w/t_seqno:%d\n",
//now(), int(t_seqno_));
//sq_.dumplist();
		return (t_seqno_);
	}

	int fcnt;	// following count-- the
			// count field in the block
			// after the seq# we are about
			// to send
	int fbytes;	// fcnt in bytes

//if (int(t_seqno_) > 1)
//printf("%f: recovery nxt_tseq called w/t_seqno:%d, seq:%d, mode:%d\n",
//now(), int(t_seqno_), seq, sack_rtx_threshmode_);
//sq_.dumplist();

	while ((seq = sq_.nexthole(seq, fcnt, fbytes)) > 0) {
		// if we have a following block
		// with a large enough count
		// we should use the seq# we get
		// from nexthole()
		if (sack_rtx_threshmode_ == 0 ||
		    (sack_rtx_threshmode_ == 1 && fcnt >= sack_rtx_cthresh_) ||
		    (sack_rtx_threshmode_ == 2 && fbytes >= sack_rtx_bthresh_) ||
		    (sack_rtx_threshmode_ == 3 && (fcnt >= sack_rtx_cthresh_ || fbytes >= sack_rtx_bthresh_)) ||
		    (sack_rtx_threshmode_ == 4 && (fcnt >= sack_rtx_cthresh_ && fbytes >= sack_rtx_bthresh_))) {

//if (int(t_seqno_) > 1)
//printf("%f: nxt_tseq<hole> returning %d\n",
//now(), int(seq));
			// adjust h_seqno, as we may have
			// been "jumped ahead" by learning
			// about a filled hole
			if (seq > h_seqno_)
				h_seqno_ = seq;
			return (seq);
		} else if (fcnt <= 0)
			break;
		else {
			seq += maxseg_;
		}
	}
//if (int(t_seqno_) > 1)
//printf("%f: nxt_tseq<top> returning %d\n",
//now(), int(t_seqno_));
	return (t_seqno_);
}
