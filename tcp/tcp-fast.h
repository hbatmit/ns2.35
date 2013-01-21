/************************************************************************\
 * TCP-FAST  NS2 Module                                         	*
 * University of Melbourne April 2005                         		*
 *                                                              	*
 * Coding: Tony Cui and Lachlan Andrew                          	*
 *                                                                      *
 * Revision History                                                     *
 * Version 1.1.2                					*
 *     Make FAST_CC_MIQ tunable						*
 *     (using a mi_threshold_ (tunable TCL variable)  to replace it) 	*
 *     Consider the calculation error when cwnd is an integer. Give	*
 *     user 3 choices the calculate cwnd.(use integer without		*
 *     considering calculation error (mode 0),  using integer 		*
 *     with considering calculatioin error (mode 1) and using double	*
 *     (mode 2).							*
 *     Fix a bug in fast_cc: add acks_per_rtt++ in the function. 	*
 *     Fix a bug in fast_cc: compare t_seqno and tcph->seqno() to       *
 *     prevent t_seqno greater than tcph->seqno().			*
 *     Allow user update cwnd every other rtt (default is every rtt)	*
 * Version 1.1.1                                                        *
 *     Set ECT bit correctly if ECN capable				*
 * Version 1.1                                                          *
 *     Add SACK function into Fast                                      *
 *     Fix bug in output function which didn't consider SYN packets.    *
 *     Fix bug in using INT_VARABLE_INVALID_VALUE instead of            *
 *     DOUBLE_VARABLE_INVALID_VALUE                                     *
 * Version 1.0.1 Released 13 November, 2004                             *
 *     Fix bug in baseRTT_ estimation with pk loss                      *
 * Version 1.0   Released 24 September, 2004                            *
\************************************************************************/
#ifndef ns_tcp_fast_h
#define ns_tcp_fast_h

#include "tcp.h"
#include "flags.h"
#include "scoreboard.h"
#include "scoreboard-rq.h"
#include "random.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#undef FASTTCPAGENT_DEBUG
#define UPDATE_CWND_EVERY_OTHER_RTT

class FastTcpAgent : public virtual TcpAgent {
public:
	FastTcpAgent();
	~FastTcpAgent();
	virtual void recv(Packet *pkt, Handler*);
	virtual void reset();
	void	fast_est(Packet *pkt,double rtt);
	void 	fast_cc(double rtt, double old_pif);
	double 	fast_calc_cwnd(double cwnd, double old_pif);
	void 	fast_recv_newack_helper(Packet *pkt);
	double 	fast_est_update_avgrtt(double rtt);	
	double 	fast_est_update_avg_cwnd(int inst_cwnd);	
	void 	fast_pace(TracedDouble *cwndp, int incre_4_cwnd);
	int 	fast_expire(Packet* pkt);
	virtual void timeout(int tno);
	virtual void output(int seqno, int reason);
#ifdef FASTTCPAGENT_DEBUG
	FILE * fasttcpagent_recordfps[10];
#endif
protected:
	virtual void 	delay_bind_init_all();
	virtual int 	delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer);
	virtual void 	traceVar(TracedVar* v);
	virtual void dupack_action();
	virtual void partial_ack_action();
	virtual void send_much(int force, int reason, int maxburst);
	
	TracedInt 	alpha_;
	TracedInt	beta_;
	TracedDouble mi_threshold_;
	TracedDouble 	avgRTT_;	
	TracedDouble 	avg_cwnd_last_RTT_;
	TracedDouble 	baseRTT_;
	unsigned char 	fast_opts;
	int		alpha_tuning_;
	TracedInt	high_accuracy_cwnd_;	//this variable is used to determine the calculation method used by NS2 FAST. Because in real network system, cwnd is an integral while here cwnd is a double, we provide 3 modes to get cwnd: 1. use double to store cwnd (high_accuracy_cwnd != 0 nor 1); 2. use int to store cwnd, ignore the calculation error(high_accuracy_cwnd = 0); 3. use int to store cwnd but also consider the calculation error(high_accuracy_cwnd = 1).
	double 		fast_calc_cwnd_end;
	int    		slowstart_;    		// # of pkts to send after slow-start, deflt(2)
	bool	on_first_rtt_;	//use this to identify if FAST should freeze or update its cwnd in this rtt.
	double cwnd_remainder;	//this variable is used to fix the calculation error using old_cwnd.
	double 		cwnd_update_time;

	double		fast_update_cwnd_interval_;	
	double	 	cwnd_increments;		
	unsigned int 	acks_per_rtt;
	unsigned int 	acks_last_rtt;		
	unsigned short 	bc_ack,bc_spacing;	
	double rtt_;			// current rtt;
	double newcwnd_;		// record un-inflated cwnd
	double* sendtime_;		// each unacked pkt's sendtime is recorded.
					// (fixes problems with RTT calc'n)
	double*	cwnd_array_;
	int*   transmits_;		// # of retx for an unacked pkt
	int    maxwnd_;			// maxwnd size for v_sendtime_[]

	double firstrecv_;		//first receive timestamp.
	double currentTime;		//the time when a recv event happens
	
	u_char timeout_;	/* boolean: sent pkt from timeout? */
	u_char fastrecov_;	/* boolean: doing fast recovery? */
	TracedInt pipe_;		/* estimate of pipe size (fast recovery) */ 
	int partial_ack_;	/* Set to "true" to ensure sending */
				/*  a packet on a partial ACK.     */
	int next_pkt_;		/* Next packet to transmit during Fast */
				/*  Retransmit as a result of a partial ack. */
	int firstpartial_;	/* First of a series of partial acks. */
	int last_natural_ack_number_;	/*added by me, the last 'natural' ack number */
					/* which is only updated when last_ack_ < tcph->seqno()*/
	ScoreBoard* scb_;
	static const int SBSIZE=64; /* Initial scoreboard size */

	double gamma_;
	
	double fasttime() {
		return(Scheduler::instance().clock() - firstsent_);
//		return (NOW);
	}
} ;


#endif // ns_tcp_fast_h

