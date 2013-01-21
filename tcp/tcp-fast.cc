/************************************************************************\
 * TCP-FAST  NS2 Module                                         	*
 * University of Melbourne April 2005                         		*
 *                                                              	*
 * Coding: Tony Cui and Lachlan Andrew                          	*
 *                                                                      *
 * Revision History                                                     *
 * Version 1.1.3 (14 Jan, 2007)        					*
 *     Make updating every other RTT the default, as in Linux		*
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

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /nfs/jade/vint/CVSROOT/ns-2/tcp/tcp-fast.cc,v 1.35 2004/12/14 05:05:21 xuanc Exp $ (NCSU/IBM)";
#endif

#define FAST_ALLOW_CC 		0x1
#define FAST_ALLOW_MI		0x2
#define INT_VARABLE_INVALID_VALUE	999999999
#define DOUBLE_VARABLE_INVALID_VALUE 	99999999999.0

#include "ip.h"
#include "tcp.h"
#include "tcp-fast.h"
#include "flags.h"
#include "random.h"

/******************************************************************************/
static class FastTcpClass : public TclClass {
public:
	FastTcpClass() : TclClass("Agent/TCP/Fast") {}
	TclObject* create(int, const char*const*) {
		return (new FastTcpAgent());
	}
} class_fast;


/******************************************************************************/
FastTcpAgent::~FastTcpAgent()
{
#ifdef FASTTCPAGENT_DEBUG
	fclose(fasttcpagent_recordfps[0]);
	fclose(fasttcpagent_recordfps[1]);
#endif

	delete scb_;

	if (sendtime_)
		delete []sendtime_;
	if (transmits_)
		delete []transmits_;
	if (cwnd_array_)
		delete []cwnd_array_;
}

/******************************************************************************/
FastTcpAgent::FastTcpAgent() : TcpAgent(),
	avgRTT_(0), baseRTT_(0), avg_cwnd_last_RTT_(1),
	alpha_(0), beta_(0), fastrecov_(FALSE),
	pipe_(-1), next_pkt_(0), firstpartial_(0),
	last_natural_ack_number_(-1), on_first_rtt_(true),
	gamma_(0.5)
{
	sendtime_ = NULL;
	transmits_ = NULL;
	cwnd_array_ = NULL;
	bind_bool("partial_ack_", &partial_ack_);
	/* Use the Reassembly Queue based scoreboard as
	 * ScoreBoard is O(cwnd) which is bad for HSTCP
	 * scb_ = new ScoreBoard(new ScoreBoardNode[SBSIZE],SBSIZE);
	 */
	scb_ = new ScoreBoardRQ();

#ifdef TCP_DELAY_BIND_ALL
#else /* ! TCP_DELAY_BIND_ALL */
	/*bind tunable parameters*/
	bind("fast_update_cwnd_interval_", &cwnd_update_period_);
	bind("avg_cwnd_last_RTT_", &avg_cwnd_last_RTT_);
	bind("avgRTT_", &avgRTT_);
	bind("baseRTT_", &baseRTT_);
	bind("alpha_", &alpha_);
	bind("beta_", &beta_);
	bind("high_accuracy_cwnd_", &high_accuracy_cwnd_);
	bind("mi_threshold_", &mi_threshold_);

//	bind("pipe_", &pipe_);		//weixl
//	bind("sack_num_", &sack_num_);	//weixl
//	bind("sack_len_", &sack_len_);  //weixl
//	bind("alpha_tuning_", &alpha_tuning_);
	bind("gamma_", &gamma_);
#endif /* TCP_DELAY_BIND_ALL */

#ifdef FASTTCPAGENT_DEBUG
	static unsigned int s_agent_ID = 0;
	char strTmp[30];
	sprintf(strTmp, "agent%d_%s.txt", s_agent_ID, "record0");
	fasttcpagent_recordfps[0] = fopen(strTmp, "w+");
	sprintf(strTmp, "agent%d_%s.txt", s_agent_ID, "record1");
	fasttcpagent_recordfps[1] = fopen(strTmp, "w+");
	s_agent_ID++;
#endif

}

/******************************************************************************/
void
FastTcpAgent::delay_bind_init_all()
{
#ifdef TCP_DELAY_BIND_ALL
        // Defaults for bound variables should be set in ns-default.tcl.
	delay_bind_init_one("cwnd_update_period_");
	delay_bind_init_one("avg_cwnd_last_RTT_");
	delay_bind_init_one("avgRTT_");
	delay_bind_init_one("baseRTT_");
	delay_bind_init_one("alpha_");
	delay_bind_init_one("beta_");
	delay_bind_init_one("high_accuracy_cwnd_");
	delay_bind_init_one("mi_threshold_");

//	delay_bind_init_one("pipe_"); 	//weixl
//	delay_bind_init_one("sack_num_");//weixl
//	delay_bind_init_one("sack_len_");
//	delay_bind_init_one("alpha_tuning_");
	delay_bind_init_one("gamma_");
#endif /* TCP_DELAY_BIND_ALL */
	TcpAgent::delay_bind_init_all();

	reset();
}

/******************************************************************************/
int
FastTcpAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer)
{
#ifdef TCP_DELAY_BIND_ALL
	if (delay_bind(varName, localName, "cwnd_update_period_", &fast_update_cwnd_interval_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "avg_cwnd_last_RTT_", &avg_cwnd_last_RTT_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "avgRTT_", &avgRTT_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "baseRTT_", &baseRTT_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "alpha_", &alpha_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "beta_", &beta_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "gamma_", &gamma_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "high_accuracy_cwnd_", &high_accuracy_cwnd_, tracer))
		return TCL_OK;
	if (delay_bind(varName, localName, "mi_threshold_", &mi_threshold_, tracer))
		return TCL_OK;

//	if (delay_bind(varName, localName, "pipe_", &pipe_, tracer))
//		return TCL_OK;		//weixl
//	if (delay_bind(varName, localName, "sack_num_", &sack_num_, tracer))
//		return TCL_OK;		//weixl
//        if (delay_bind(varName, localName, "sack_len_", &sack_len_, tracer))
//                return TCL_OK;          //weixl
//	if (delay_bind(varName, localName, "alpha_tuning_", &alpha_tuning_, tracer))
//		return TCL_OK;
#endif /* TCP_DELAY_BIND_ALL */

	return TcpAgent::delay_bind_dispatch(varName, localName, tracer);
}

/******************************************************************************/
/* Print out just the variable that is modified */
void
FastTcpAgent::traceVar(TracedVar* v) 
{
	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[500];
	int n;

	curtime = &s ? s.clock() : 0;
	if (!strcmp(v->name(), "avgRTT_")
	 || !strcmp(v->name(), "baseRTT_")
	 || !strcmp(v->name(), "mi_threshold_")
	 )
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), double(*((TracedDouble*) v))); 
	else if (!strcmp(v->name(), "avg_cwnd_last_RTT_")
	      || !strcmp(v->name(), "alpha_")
	      || !strcmp(v->name(), "beta_")
	      || !strcmp(v->name(), "high_accuracy_cwnd_" )
	      )
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %d",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), int(*((TracedInt*) v))); 
	else
	{
		TcpAgent::traceVar(v);
		return;
	}

	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	if (channel_)
		(void)Tcl_Write(channel_, wrk, n+1);
	wrk[n] = 0;

	return;
}

/******************************************************************************/
void
FastTcpAgent::reset ()
{
	fast_opts = 0;
	fast_opts |= FAST_ALLOW_CC;
	fast_opts |= FAST_ALLOW_MI;
	cwnd_update_time = fasttime();
	cwnd_increments = 0;
	firstrecv_ = -1.0;
	slowstart_ = 2;
	acks_per_rtt = 0;
	acks_last_rtt = 0;
	bc_ack = 0;
	bc_spacing = 0;
	t_backoff_=1;
	baseRTT_ = DOUBLE_VARABLE_INVALID_VALUE;
	newcwnd_ = DOUBLE_VARABLE_INVALID_VALUE;
	currentTime = 0;
	fast_calc_cwnd_end = 0;
	scb_->ClearScoreBoard();
	cwnd_remainder = 0;

	TcpAgent::reset ();
}

#define CWND_USE_INT	0
#define CWND_USE_INT_CONSIDER_ERROR	1
#define CWND_USE_DOUBLE	2
/******************************************************************************/
/*this function should be used to calculate the new cwnd_ size in every update*/
double
FastTcpAgent::fast_calc_cwnd(double cwnd,double old_cwnd)
{
        double q = avgRTT_ - baseRTT_;
        double cwnd_array_q = old_cwnd * q;
        double target_cwnd = cwnd;

        if ( avgRTT_ == 0 || baseRTT_ == DOUBLE_VARABLE_INVALID_VALUE )
        {
                return cwnd;
        }

        if ( fast_opts & FAST_ALLOW_CC )
        {
                if ( cwnd_array_q < alpha_ * avgRTT_ ||
		     cwnd_array_q >= beta_ * avgRTT_ )
                {
                        // target_cwnd = (cwnd + old_cwnd * (baseRTT_/avgRTT_) + alpha_) / 2;
                        target_cwnd = ( (1-gamma_) * cwnd + (old_cwnd * (baseRTT_/avgRTT_) + alpha_) * gamma_);
			double gamma=gamma_;
			double alpha=alpha_;
			double beta=beta_;
			//printf("gamma:%lf alpha;%lf beta:%lf target:%lf, now:%lf, new:%lf\n", gamma, alpha, beta, (old_cwnd * (baseRTT_/avgRTT_) + alpha_), cwnd, target_cwnd);
			if (target_cwnd<2) target_cwnd=2;
			if ( high_accuracy_cwnd_ == CWND_USE_INT_CONSIDER_ERROR ||
			     high_accuracy_cwnd_ == CWND_USE_INT )	//we use intergal to store cwnd.
			{
				if (high_accuracy_cwnd_ == CWND_USE_INT_CONSIDER_ERROR)	//we consider the calculation error.
				{
					cwnd_remainder += (target_cwnd - (int)target_cwnd);
					if (cwnd_remainder > 1)
					{
						target_cwnd++;
						cwnd_remainder--;
					}
				}
				target_cwnd = (int)target_cwnd;
			//printf("gamma:%lf alpha;%lf beta:%lf target:%lf, now:%lf, new:%lf\n", gamma, alpha, beta, (old_cwnd * (baseRTT_/avgRTT_) + alpha_), cwnd, target_cwnd);
			}
                }
        }

        return target_cwnd;
}

/******************************************************************************/
/* in this function, we can get the new average rtt which */
/* will be used to get the new cwnd_ size */
double
FastTcpAgent::fast_est_update_avgrtt(double rtt)
{
	double scale = avg_cwnd_last_RTT_/3;
	double avgRTT=avgRTT_;
	if ( !avgRTT)
		avgRTT = rtt;
	else if ( scale > 256 )
	{
		avgRTT = ((scale-1) * avgRTT + rtt)/scale;
	}
	else
	{
		avgRTT = (avgRTT*255 + rtt)/256;
	}
	return (double)avgRTT;
}

/******************************************************************************/
/* We get average cwnd size which will be used as a history record. */
double
FastTcpAgent::fast_est_update_avg_cwnd(int inst_cwnd)
{
#if 0
	unsigned int scale = avg_cwnd_last_RTT_/3;
	double avg_cwnd=avg_cwnd_last_RTT_;

	if ( !avg_cwnd)
		avg_cwnd = inst_cwnd;
	else if ( scale > 256 )
	{
		avg_cwnd = ((scale-1) * avg_cwnd + inst_cwnd)/scale;
	}
	else
	{
		avg_cwnd = (avg_cwnd*255 + inst_cwnd)/256;
	}
	return avg_cwnd;
#endif
	return (double)inst_cwnd;
}

/******************************************************************************/
/* parameter estimation */
void
FastTcpAgent::fast_est(Packet *pkt,double rtt)
{			
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	if(rtt < baseRTT_)
		baseRTT_ = rtt;
	avg_cwnd_last_RTT_ = (fast_est_update_avg_cwnd(t_seqno_ - tcph->seqno() - dupacks_));
	avgRTT_ = fast_est_update_avgrtt(rtt);
}

/******************************************************************************/
/* congestion control */
void
FastTcpAgent::fast_cc(double rtt, double old_cwnd)
{
	// We update acks_last_rtt every RTT.
	if ( fast_calc_cwnd_end + rtt < currentTime ) {
		fast_calc_cwnd_end = currentTime;
		acks_last_rtt = acks_per_rtt;
		acks_per_rtt = 0;
		if ( on_first_rtt_ == true )	on_first_rtt_ = false;
		else	on_first_rtt_ = true;
	}
	acks_per_rtt++;

	// We use MI to increase cwnd_ when there is little queueing delay 
	if ( avgRTT_ - baseRTT_ < mi_threshold_ && (fast_opts & FAST_ALLOW_MI) ) {
		cwnd_++;
		cwnd_update_time = currentTime;
		return;
	}

	// If queueing delay is large, we use fast algorithm
	if ( currentTime >= cwnd_update_time + fast_update_cwnd_interval_ 
#ifdef	UPDATE_CWND_EVERY_OTHER_RTT
	&& on_first_rtt_ == true
#endif
	) {
		double updated_cwnd;
		cwnd_increments = 0;
		updated_cwnd = fast_calc_cwnd(cwnd_,old_cwnd);
		if ( updated_cwnd >= cwnd_ && baseRTT_ >= 0.004 && baseRTT_ != DOUBLE_VARABLE_INVALID_VALUE ) {
			fast_pace(&cwnd_, (int)(updated_cwnd-cwnd_));
		}
		else
			cwnd_ = updated_cwnd;
		cwnd_update_time = currentTime;
	}
	else if ( baseRTT_ >= 0.004 && baseRTT_ != DOUBLE_VARABLE_INVALID_VALUE )
		fast_pace(&cwnd_, 0);
}

#define MIN(x, y) ((x)<(y) ? (x) : (y))
/******************************************************************************/
void
FastTcpAgent::recv(Packet *pkt, Handler *)
{
	currentTime = fasttime();
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	hdr_flags *flagh = hdr_flags::access(pkt);
	int valid_ack = 0;

	if (qs_approved_ == 1 && tcph->seqno() > last_ack_)
		endQuickStart();
	if (qs_requested_ == 1)
		processQuickStart(pkt);

	/* W.N.: check if this is from a previous incarnation */
	if (tcph->ts() < lastreset_) {
		// Remove packet and do nothing
 		Packet::free(pkt);
		return;
	}
	++nackpack_;
	if(firstrecv_<0) { // init fast rtt vars
		firstrecv_ = currentTime;
		baseRTT_ = avgRTT_ = rtt_ = firstrecv_;
	}

	if (flagh->ecnecho())
		ecn(tcph->seqno());

	int ecnecho = hdr_flags::access(pkt)->ecnecho();
	if (ecnecho && ecn_)
		ecn(tcph->seqno());
	// Check if ACK is valid.  Suggestion by Mark Allman.
        if (tcph->seqno() >= last_ack_)
		valid_ack = 1;

#ifdef FASTTCPAGENT_DEBUG
	if (cwnd_ <= 0)
		printf("%8.3f : cwnd is not positive! cwnd is %f .\n", (double)currentTime, (double)cwnd_);
#endif	
	/*
	 * If DSACK is being used, check for DSACK blocks here.
	 * Possibilities:  Check for unnecessary Fast Retransmits.
	 */
	if (!fastrecov_) {
		/* normal... not fast recovery */
		if ((int)tcph->seqno() > last_ack_) {
			if (last_ack_ == 0 ) {
				cwnd_ = initial_window();
			}

			/* check if cwnd has been inflated */
#if 1
			if(dupacks_ > numdupacks_ &&  cwnd_ > newcwnd_) {
				//check t_seqno_ before changing cwnd.
        			if (t_seqno_ < tcph->seqno())
				        t_seqno_ = tcph->seqno();
				//cwnd becomes a negative number in some case.
				
				cwnd_ = t_seqno_ - tcph->seqno() + 1;
				dupacks_ = 0;
				for (int i=0;i<maxwnd_;i++)
					cwnd_array_[i]=cwnd_;
			}
#endif

			firstpartial_ = 0;

			// reset sendtime for acked pkts and incr transmits_
			double sendTime = sendtime_[tcph->seqno()%maxwnd_];
			double old_pif=cwnd_array_[tcph->seqno()%maxwnd_];
			int transmits = transmits_[tcph->seqno()% maxwnd_];
			for(int k= (last_natural_ack_number_+1); k<=tcph->seqno(); ++k) {
				sendtime_[k%maxwnd_] = -1.0;
				transmits_[k%maxwnd_] = 0;
				cwnd_array_[k%maxwnd_]=-1;
			}
			if (t_seqno_ > tcph->seqno()){
				if ((transmits == 1) && (currentTime - sendTime > 0))
					rtt_ = currentTime - sendTime;
				else
					rtt_ = avgRTT_;
			}else rtt_ = avgRTT_;

			fast_recv_newack_helper(pkt);
			timeout_ = FALSE;
			scb_->ClearScoreBoard();
			fast_est(pkt, rtt_);
			fast_cc(rtt_, old_pif);
			last_natural_ack_number_ = tcph->seqno();
		} else if ((int)tcph->seqno() < last_ack_) {
			/*NOTHING*/
		//the follows are if (int)tcph->seqno() == last_ack_
		} else if (timeout_ == FALSE) {
			if (tcph->seqno() != last_ack_) {
				fprintf(stderr, "pkt seq %d should be %d\n", tcph->seqno(), last_ack_);
				abort();
			}
			scb_->UpdateScoreBoard (highest_ack_, tcph);
			/*
		 	 * Check for a duplicate ACK.
			 * Check that the SACK block actually
			 *  acknowledges new data.
 			 */
			if(scb_->CheckUpdate()) {
				if (++dupacks_ == numdupacks(cwnd_)) {
 				/*
 				 * Assume we dropped just one packet.
 				 * Retransmit last ack + 1
 				 * and try to resume the sequence.
 				 */
 				   	dupack_action();
 				} else if (dupacks_ < numdupacks(cwnd_) && singledup_ ) {
					send_one();
 				}
			}
		}
        	if (valid_ack || aggressive_maxburst_)
			if (dupacks_ == 0)
				send_much(FALSE, 0, maxburst_);
	} else {
		/* we are in fast recovery */
		cwnd_update_time = currentTime;
		--pipe_;
		if ((int)tcph->seqno() >= recover_) {
			/* ACK indicates fast recovery is over */
			recover_ = 0;
			fastrecov_ = FALSE;
			newack(pkt);
			/* if the connection is done, call finish() */
			if ((highest_ack_ >= curseq_-1) && !closed_) {
				closed_ = 1;
				finish();
			}
			timeout_ = FALSE;
			scb_->ClearScoreBoard();

			/* New window: W/2 - K or W/2? */
		} else if ((int)tcph->seqno() > highest_ack_) {
			/* Not out of fast recovery yet.
			 * Update highest_ack_, but not last_ack_. */
			highest_ack_ = (int)tcph->seqno();
			scb_->UpdateScoreBoard (highest_ack_, tcph);
			if (partial_ack_) {
			  /* partial_ack_ is needed to guarantee that */
			  /*  a new packet is sent in response to a   */
			  /*  partial ack.                            */
				partial_ack_action();
				++pipe_;
				if (firstpartial_ == 0) {
					newtimer(pkt);
					t_backoff_ = 1;
					firstpartial_ = 1;
				}
			} else {
				--pipe_;
				newtimer(pkt);
				t_backoff_ = 1;
 			 /* If this partial ACK is from a retransmitted pkt,*/
 			 /* then we decrement pipe_ again, so that we never */
 			 /* do worse than slow-start.  If this partial ACK  */
 			 /* was instead from the original packet, reordered,*/
 			 /* then this might be too aggressive. */
			}
		} else if (timeout_ == FALSE) {
			/* got another dup ack */
			scb_->UpdateScoreBoard (highest_ack_, tcph);
 		        if(scb_->CheckUpdate()) {
 				if (dupacks_ > 0)
 			        	dupacks_++;
 			}
		}
        	if (valid_ack || aggressive_maxburst_)
			send_much(FALSE, 0, maxburst_);
	}

	Packet::free(pkt);
}

/******************************************************************************/
void
FastTcpAgent::dupack_action()
{
	int recovered = (highest_ack_ > recover_);
	if (recovered || (!bug_fix_ && !ecn_)) {
		goto sack_action;
	}
 
	if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		last_cwnd_action_ = CWND_ACTION_DUPACK;
		/*
		 * What if there is a DUPACK action followed closely by ECN
		 * followed closely by a DUPACK action?
		 * The optimal thing to do would be to remember all
		 * congestion actions from the most recent window
		 * of data.  Otherwise "bugfix" might not prevent
		 * all unnecessary Fast Retransmits.
		 */
		reset_rtx_timer(1,0);
		/*
		 * There are three possibilities: 
		 * (1) pipe_ = int(cwnd_) - numdupacks_;
		 * (2) pipe_ = window() - numdupacks_;
		 * (3) pipe_ = maxseq_ - highest_ack_ - numdupacks_;
		 * equation (2) takes into account the receiver's
		 * advertised window, and equation (3) takes into
		 * account a data-limited sender.
		 */
		if (singledup_ && LimTransmitFix_) {
		  pipe_ = maxseq_ - highest_ack_ - 1;
		}
		else {
		  // numdupacks(cwnd_) packets have left the pipe
		  pipe_ = maxseq_ - highest_ack_ - numdupacks(cwnd_);
		}
		fastrecov_ = TRUE;
		scb_->MarkRetran(highest_ack_+1);
		output(last_ack_ + 1, TCP_REASON_DUPACK);
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

sack_action:
	// we are now going into fast_recovery and will trace that event
	trace_event("FAST_RECOVERY");

	recover_ = maxseq_;
	last_cwnd_action_ = CWND_ACTION_DUPACK;
	if (oldCode_) {
	 	pipe_ = int(cwnd_) - numdupacks(cwnd_);
	} else if (singledup_ && LimTransmitFix_) {
		  pipe_ = maxseq_ - highest_ack_ - 1;
	}
	else {
		  // numdupacks(cwnd_) packets have left the pipe
		  pipe_ = maxseq_ - highest_ack_ - numdupacks(cwnd_);
	}
	slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
	reset_rtx_timer(1,0);
	fastrecov_ = TRUE;
	scb_->MarkRetran(highest_ack_+1);
	output(last_ack_ + 1, TCP_REASON_DUPACK);	// from top
	/*
	 * If dynamically adjusting numdupacks_, record information
	 *  at this point.
	 */
	return;
}

/*
 * Process a packet that acks previously unacknowleges data, but
 * does not take us out of Fast Retransmit.
 *
 * The need for a mechanism to ensure that Sack TCP sends a packet in
 * response to a partial ACK has been discussed in
 * "Challenges to Reliable Data Transport over Heterogeneous
 * Wireless Networks", Hari Balakrishnan, 1998, and in
 * "Responding to Spurious Timeouts in TCP", Andrei Gurtov and 
 * Reiner Ludwig, 2003. 
 */
void
FastTcpAgent::partial_ack_action()
{
	if (next_pkt_ < highest_ack_ + 1) {
		next_pkt_ = highest_ack_ + 1;
	}
	// Output two packets in response to a partial ack,
	//   so as not to do worse than slow-start.
	int i;
	for (i = 1; i<=2; i++) {
		int getNext = scb_->GetNextUnacked(next_pkt_);

		if (getNext > next_pkt_) {
			next_pkt_ = getNext;
		}
		if (t_seqno_ < next_pkt_) {
			t_seqno_ = next_pkt_;
		}
		output(next_pkt_, TCP_REASON_PARTIALACK);	
		scb_->MarkRetran(next_pkt_);
		++next_pkt_; 
	}
	return;
}

void FastTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		/*
		 * IF DSACK and dynamic adjustment of numdupacks_,
		 *  check whether a smaller value of numdupacks_
		 *  would have prevented this retransmit timeout.
		 * If DSACK and detection of premature retransmit
		 *  timeouts, then save some info here.
		 */ 
		dupacks_ = 0;
		fastrecov_ = FALSE;
		timeout_ = TRUE;
		if (highest_ack_ > last_ack_)
			last_ack_ = highest_ack_;
		recover_ = maxseq_;
		scb_->ClearScoreBoard();
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.
			 *
			 * Note:  in the USC implementation,
			 * slow_start_restart_ == 0.
			 * I don't know what the U. Arizona implementation
			 * defaults to.
			 */
			return;
		};
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
		reset_rtx_timer(0, 0);
		++nrexmit_;
		slowdown(CLOSE_CWND_RESTART|CLOSE_SSTHRESH_HALF);
		cwnd_ = double(slowstart_);
		newcwnd_ = 0;
		send_much(0, TCP_REASON_TIMEOUT, maxburst_);
	} else {
		/* delayed-sent timer, with random overhead to avoid
		 * phase effect. */
		send_much(1, TCP_REASON_TIMEOUT, 3);
	}
}

void FastTcpAgent::send_much(int force, int reason, int maxburst)
{
	register int found, npacket = 0;
	int win = window();
	int xmit_seqno;

	found = 1;
	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	/* Save time when first packet was sent, for newreno  --Allman */
	if (t_seqno_ == 0)
		firstsent_ = Scheduler::instance().clock();
	/*
	 * as long as the pipe is open and there is app data to send...
	 */
	while (((!fastrecov_ && (t_seqno_ <= last_ack_ + win)) ||
			(fastrecov_ && (pipe_ < int(cwnd_)))) 
			&& t_seqno_ < curseq_ && found) {

		if (overhead_ == 0 || force) {
			found = 0;
			int oldest_unacked_pkt = scb_->GetNextUnacked(last_ack_);
			if (oldest_unacked_pkt != -1 &&
			fasttime() - sendtime_[oldest_unacked_pkt%maxwnd_] > 2*avgRTT_){
				xmit_seqno = oldest_unacked_pkt;
			}
			else
				xmit_seqno = scb_->GetNextRetran ();
			if (xmit_seqno == -1) { 
				if ((!fastrecov_ && t_seqno_<=highest_ack_+win)||
					(fastrecov_ && t_seqno_<=highest_ack_+int(wnd_))) {
					found = 1;
					xmit_seqno = t_seqno_++;
				}
			} else if (recover_>0 && xmit_seqno<=highest_ack_+int(wnd_)) {
				found = 1;
				scb_->MarkRetran (xmit_seqno);
				win = window();
			}
			if (found) {
				output(xmit_seqno, reason);
				if (t_seqno_ <= xmit_seqno)
					t_seqno_ = xmit_seqno + 1;
				npacket++;
				pipe_++;
				if (QOption_)
					process_qoption_after_send () ;
	                        if (qs_approved_ == 1) {
	                                double delay = (double) t_rtt_ * tcp_tick_ / cwnd_;
	                                delsnd_timer_.resched(delay);
	                                return;
	                        }
			}
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 * This is only for the simulator,to add some
			 *   randomization if speficied.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if (maxburst && npacket == maxburst)
			break;
	} /* while */
}

void
FastTcpAgent::output(int seqno, int reason)
{
	Packet* p = allocpkt();
	hdr_tcp *tcph = hdr_tcp::access(p);
	double now = Scheduler::instance().clock();
	hdr_flags* hf = hdr_flags::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	int databytes = hdr_cmn::access(p)->size();
	tcph->seqno() = seqno;
	tcph->ts() = now;
	tcph->reason() = reason;
	/* if this is the 1st pkt, setup senttime[] and transmits[]
	 * I alloc mem here, instrad of in the constructor, to cover
	 * cases which windows get set by each different tcp flows */
	if (seqno==0) {
		maxwnd_ = int(wnd_);
		if (sendtime_)
			delete []sendtime_;
        	if (transmits_)
			delete []transmits_;
        	if (cwnd_array_)
			delete []cwnd_array_;
		sendtime_ = new double[maxwnd_];
		transmits_ = new int[maxwnd_];
		cwnd_array_= new double[maxwnd_];
		for(int i=0;i<maxwnd_;i++) {
			sendtime_[i] = -1.;
			transmits_[i] = 0;
			cwnd_array_[i]=-1;
		}
	}

	if (ecn_) {
		hf->ect() = 1; // ECN capable transport.
	}

	/* Check if this is the initial SYN packet. */
	if (seqno == 0) {
		if (syn_) {
			databytes= 0;
			curseq_ += 1;
			hdr_cmn::access(p)->size() = tcpip_base_hdr_size_;
		}
		if (ecn_) {
			hf->ecnecho() = 1;
//			hf->cong_action() = 1;
			hf->ect() = 0;
		}
	}
	else if (useHeaders_ == true) {
		hdr_cmn::access(p)->size() += headersize();
	}

	// record a find grained send time and # of transmits 
	int index = seqno % maxwnd_;
	sendtime_[index] = fasttime(); 
	cwnd_array_[index]=avg_cwnd_last_RTT_; 
	++transmits_[index];
	/* support ndatabytes_ in output - Lloyd Wood 14 March 2000 */
	int bytes = hdr_cmn::access(p)->size(); 
	ndatabytes_ += bytes; 
	ndatapack_++; // Added this - Debojyoti 12th Oct 2000
	send(p, 0);

	if (seqno == curseq_ && seqno > maxseq_)
		idle();  // Tell application I have sent everything so far

	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_) {
				rtt_seq_ = seqno;
				rtt_ts_ = now;
			}
		}
	} else {
		++nrexmitpack_;
       		nrexmitbytes_ += bytes;
    	}

	if (!(rtx_timer_.status() == TIMER_PENDING))
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

/******************************************************************************/
/* Space out increments to cwnd as acknowledegements arrive */
/* cwndp points to the actual cwnd value */
/* cwnd_incre (0 or 1) specifies the desired amount to increase cwnd by */
/* (eventually) */
void
FastTcpAgent::fast_pace(TracedDouble *cwndp, int incre_4_cwnd)
{
	if ( !avgRTT_ )
		return;

	double acks_per_period = (double)acks_last_rtt * fast_update_cwnd_interval_ / avgRTT_;

	if ( incre_4_cwnd >= 1 )
	{
		cwnd_increments += incre_4_cwnd;
		/* bc_spacing: target number of ACKs between increments */
		bc_spacing = (short unsigned int)(acks_per_period/cwnd_increments);
		/* bc_ack: number of ACKs since last increment */
		bc_ack = 1;
	}
	else { 
		bc_ack ++;
	}

	if (cwnd_increments)
	{
		/* if cwnd small, increment immediately */
		if (*cwndp <= 4) {
		/* if increment would more than double cwnd, do it in stages */
			if (*cwndp < cwnd_increments) {
				cwnd_increments -= (unsigned int)*cwndp;
				*cwndp += *cwndp;
			}
			else {
				*cwndp += cwnd_increments;
				cwnd_increments=0;
				bc_spacing=0;
			}
			bc_ack=0;
		}
		else if (bc_ack >= bc_spacing)
		{
			(*cwndp)++;
			cwnd_increments--;
			bc_ack = 0;
		}
	}
}

/******************************************************************************/
/*
 * return -1 if the oldest sent pkt has not been timeout (based on
 * fine grained timer).
 */
int
FastTcpAgent::fast_expire(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	double elapse = fasttime() - sendtime_[(tcph->seqno()+1)%maxwnd_];
	if (elapse >= 2 * avgRTT_) {
		return(tcph->seqno()+1);
	}
	return(-1);
}

/******************************************************************************/
void
FastTcpAgent::fast_recv_newack_helper(Packet *pkt) {
	newack(pkt);

	if (ect_) {
		if (!hdr_flags::access(pkt)->ecnecho())
			ecn_backoff_ = 0;
		if (!ecn_burst_ && hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = TRUE;
		else if (ecn_burst_ && ! hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = FALSE;
	}
	if (!ect_ && hdr_flags::access(pkt)->ecnecho() &&
		!hdr_flags::access(pkt)->cong_action())
		ect_ = 1;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
	if (QOption_ && curseq_ == highest_ack_ +1) {
		cancel_rtx_timer();
	}
}

