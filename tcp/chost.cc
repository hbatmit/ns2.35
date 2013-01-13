/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 * 	This product includes software developed by the Daedalus Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * $Header: /cvsroot/nsnam/ns-2/tcp/chost.cc,v 1.14 2011/10/02 22:32:34 tom_henderson Exp $
 */

/*
 * Functions invoked by TcpSessionAgent
 */
#include "nilist.h"
#include "chost.h"
#include "tcp-int.h"
#include "random.h"

CorresHost::CorresHost() : slink(), TcpFsAgent(),
	lastackTS_(0), dontAdjustOwnd_(0), dontIncrCwnd_(0), rexmtSegCount_(0),
	connWithPktBeforeFS_(NULL)
{
	nActive_ = nTimeout_ = nFastRec_ = 0;
	ownd_ = 0;
	owndCorrection_ = 0;
	closecwTS_ = 0;
	connIter_ = new Islist_iter<IntTcpAgent> (conns_);
	rtt_seg_ = NULL;
}


/*
 * Open up the congestion window.
 */
void 
CorresHost::opencwnd(int /*size*/, IntTcpAgent *sender)
{
	if (cwnd_ < ssthresh_) {
		/* slow-start (exponential) */
		cwnd_ += 1;
	} else {
		/* linear */
		//double f;
		if (!proxyopt_) {
			switch (wnd_option_) {
			case 0:
				if ((count_ = count_ + winInc_) >= cwnd_) {
					count_ = 0;
					cwnd_ += winInc_;
				}
				break;
			case 1:
				/* This is the standard algorithm. */
				cwnd_ += winInc_ / cwnd_;
				break;
			default:
#ifdef notdef
				/*XXX*/
				error("illegal window option %d", wnd_option_);
#endif
				abort();
			}
		} else {	// proxy
			switch (wnd_option_) {
			case 0: 
			case 1:
				if (sender->highest_ack_ >= sender->wndIncSeqno_) {
					cwnd_ += winInc_;
					sender->wndIncSeqno_ = 0;
				}
				break;
			default:
#ifdef notdef
				/*XXX*/
				error("illegal window option %d", wnd_option_);
#endif
				abort();
			}
		}
	}
	// if maxcwnd_ is set (nonzero), make it the cwnd limit
	if (maxcwnd_ && (int(cwnd_) > maxcwnd_))
		cwnd_ = maxcwnd_;
	
	return;
}

void 
CorresHost::closecwnd(int how, double ts, IntTcpAgent *sender)
{
	if (proxyopt_) {
		if (!sender || ts > sender->closecwTS_)
			closecwnd(how, sender);
	}
	else {
		if (ts > closecwTS_)
			closecwnd(how, sender);
	}
}

void 
CorresHost::closecwnd(int how, IntTcpAgent *sender)
{
	int sender_ownd = 0;
	if (sender)
		sender_ownd = sender->maxseq_ - sender->highest_ack_;
	closecwTS_ = Scheduler::instance().clock();
	if (proxyopt_) {
		if (sender)
			sender->closecwTS_ = closecwTS_;
		how += 10;
	}
	switch (how) {
	case 0:
	case 10:
		/* timeouts */

		/* 
		 * XXX we use cwnd_ instead of ownd_ here, which may not be
		 * appropriate if the sender does not fully utilize the
		 * available congestion window (ownd_ < cwnd_).
		 */
		ssthresh_ = int(cwnd_ * winMult_);
		cwnd_ = int(wndInit_);
		break;
		
	case 1:
		/* Reno dup acks, or after a recent congestion indication. */

		/* 
		 * XXX we use cwnd_ instead of ownd_ here, which may not be
		 * appropriate if the sender does not fully utilize the
		 * available congestion window (ownd_ < cwnd_).
		 */
		cwnd_ *= winMult_;
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		break;

	case 11:
		/* Reno dup acks, or after a recent congestion indication. */
		/* XXX fix this -- don't make it dependent on ownd */
		cwnd_ = ownd_ - sender_ownd*(1-winMult_);
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		break;

	case 3:
	case 13:
		/* idle time >= t_rtxcur_ */
		cwnd_ = wndInit_;
		break;
		
	default:
		abort();
	}
	fcnt_ = 0.;
	count_ = 0;
	if (sender)
		sender->count_ = 0;
}

Segment* 
CorresHost::add_pkts(int /*size*/, int seqno, int sessionSeqno, int daddr, int dport, 
		     int sport, double ts, IntTcpAgent *sender)
{
	class Segment *news;

	ownd_ += 1;
	news = new Segment;
	news->seqno_ = seqno;
	news->sessionSeqno_ = sessionSeqno;
	news->daddr_ = daddr;
	news->dport_ = dport;
	news->sport_ = sport;
	news->ts_ = ts;
	news->size_ = 1;
	news->dupacks_ = 0;
	news->later_acks_ = 0;
	news->thresh_dupacks_ = 0;
	news->partialack_ = 0;
	news->rxmitted_ = 0;
	news->sender_ = sender;
	seglist_.append(news);
	return news;
}

void
CorresHost::adjust_ownd(int size) 
{
	if (double(owndCorrection_) < size)
		ownd_ -= min(double(ownd_), size - double(owndCorrection_));
	owndCorrection_ -= min(double(owndCorrection_),size);
	if (double(ownd_) < -0.5 || double(owndCorrection_ < -0.5))
		printf("In adjust_ownd(): ownd_ = %g  owndCorrection_ = %g\n", double(ownd_), double(owndCorrection_));
}

int
CorresHost::clean_segs(int /*size*/, Packet *pkt, IntTcpAgent *sender, int sessionSeqno, int amt_data_acked)
{ 
    Segment *cur, *prev=NULL, *newseg;
    int i;
    //int rval = -1;

    /* remove all acked pkts from list */
    int latest_susp_loss = rmv_old_segs(pkt, sender, amt_data_acked);
    /* 
     * XXX If no new data is acked and the last time we shrunk the window 
     * covers the most recent suspected loss, update the estimate of the amount
     * of outstanding data.
     */
    if (amt_data_acked == 0 && latest_susp_loss <= recover_ && 
	!dontAdjustOwnd_ && last_cwnd_action_ != CWND_ACTION_TIMEOUT) {
	    owndCorrection_ += min(double(ownd_),1);
	    ownd_ -= min(double(ownd_),1);
    }
    /*
     * A pkt is a candidate for retransmission if it is the leftmost one in the
     * unacked window for the connection AND has at least numdupacks_
     * dupacks/later acks AND (at least one dupack OR a later packet also
     * with the threshold number of dup/later acks). A pkt is also a candidate
     * for immediate retransmission if it has partialack_ set, indicating that
     * a partial new ack has been received for it.
     */

    for (i=0; i < rexmtSegCount_; i++) {
	    int remove_flag = 0;
	    /* 
	     * curArray_ only contains segments that are the first oldest
	     * unacked segments of their connection (i.e., they are at the left
	     * edge of their window) and have either received a
	     * partial ack and/or have received at least cur->numdupacks_
	     * dupacks/later acks. Thus, segments in curArray_ have a high
	     * probability (but not certain) of being eligible for 
	     * retransmission. Using curArray_ avoids having to scan
	     * through all the segments.
	     */
	    cur = curArray_[i];
	    prev = prevArray_[i];
	    if (cur->partialack_ || cur->dupacks_ > 0 || 
		cur->sender_->num_thresh_dupack_segs_ > 1 ) {
		    if (cur->thresh_dupacks_) {
			    cur->thresh_dupacks_ = 0;
			    cur->sender_->num_thresh_dupack_segs_--;
		    }
		    if (cur->sessionSeqno_ <= recover_ && 
			last_cwnd_action_ != CWND_ACTION_TIMEOUT /* XXX 2 */)
			    dontAdjustOwnd_ = 1;
		    if ((cur->sessionSeqno_ > recover_) || 
			(last_cwnd_action_ == CWND_ACTION_TIMEOUT /* XXX 2 */)
			|| (proxyopt_ && cur->seqno_ > cur->sender_->recover_)
			|| (proxyopt_ && cur->sender_->last_cwnd_action_ == CWND_ACTION_TIMEOUT /* XXX 2 */)) { 
			    /* new loss window */
			    closecwnd(1, cur->ts_, cur->sender_);
			    recover_ = sessionSeqno - 1;
			    last_cwnd_action_ = CWND_ACTION_DUPACK /* XXX 1 */;
			    cur->sender_->recover_ = cur->sender_->maxseq_;
			    cur->sender_->last_cwnd_action_ = 
				    CWND_ACTION_DUPACK /* XXX 1 */;
			    dontAdjustOwnd_ = 0;
		    }
		    if ((newseg = cur->sender_->rxmit_last(TCP_REASON_DUPACK, 
			   cur->seqno_, cur->sessionSeqno_, cur->ts_))) {
			    newseg->rxmitted_ = 1;
			    adjust_ownd(cur->size_);
			    if (!dontAdjustOwnd_) {
				    owndCorrection_ += 
					    min(double(ownd_),cur->dupacks_);
				    ownd_ -= min(double(ownd_),cur->dupacks_);
			    }
			    seglist_.remove(cur, prev);
			    remove_flag = 1;
			    delete cur;
		    }
		    /* 
		     * if segment just removed used to be the one just previous
		     * to the next segment in the array, update prev for the
		     * next segment
		     */
		    if (remove_flag && cur == prevArray_[i+1])
			    prevArray_[i+1] = prev;
	    }
    }
    rexmtSegCount_ = 0;
    return(0);
}
		    

int
CorresHost::rmv_old_segs(Packet *pkt, IntTcpAgent *sender, int amt_data_acked)
{
	Islist_iter<Segment> seg_iter(seglist_);
	Segment *cur, *prev=0;
	int done = 0;
	int new_data_acked = 0;
	int partialack = 0;
	int latest_susp_loss = -1;
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	if (tcph->ts_echo() > lastackTS_)
		lastackTS_ = tcph->ts_echo();

	while (((cur = seg_iter()) != NULL) &&
	       (!done || tcph->ts_echo() > cur->ts_)) {
		int remove_flag = 0;
		/* ack for older pkt of another connection */
		if (sender != cur->sender_ && tcph->ts_echo() > cur->ts_) { 
			if (!disableIntLossRecov_)
				cur->later_acks_++;
			latest_susp_loss = 
				max(latest_susp_loss,cur->sessionSeqno_);
			dontIncrCwnd_ = 1;
		} 
		/* ack for same connection */
		else if (sender == cur->sender_) {
			/* higher ack => clean up acked packets */
			if (tcph->seqno() >= cur->seqno_) {
				adjust_ownd(cur->size_);
				seglist_.remove(cur, prev);
				remove_flag = 1;
				new_data_acked += cur->size_;
				if (new_data_acked >= amt_data_acked)
					done = 1;
				if (prev == cur) 
					prev = NULL;
				if (cur == rtt_seg_)
					rtt_seg_ = NULL;
				delete cur;
				if (seg_iter.get_cur() && prev)
					seg_iter.set_cur(prev);
				else if (seg_iter.get_cur())
					seg_iter.set_cur(seg_iter.get_last());
			} 
			/* partial new ack => rexmt immediately */
			/* XXX should we check recover for session? */
			else if (amt_data_acked > 0 && 
				 tcph->seqno() == cur->seqno_-1 &&
				 cur->seqno_ <= sender->recover_ &&
				 sender->last_cwnd_action_ == CWND_ACTION_DUPACK) {
				cur->partialack_ = 1;
				partialack = 1;
				latest_susp_loss = 
					max(latest_susp_loss,cur->sessionSeqno_);
				if (new_data_acked >= amt_data_acked)
					done = 1;
				dontIncrCwnd_ = 1;
			}
			/* 
			 * If no new data has been acked AND this segment has
			 * not been retransmitted before AND the ack indicates 
			 * that this is the next segment to be acked, then
			 * increment dupack count.
			 */
			else if (!amt_data_acked && !cur->rxmitted_ &&
				 tcph->seqno() == cur->seqno_-1) {
				cur->dupacks_++;
				latest_susp_loss = 
					max(latest_susp_loss,cur->sessionSeqno_);
				done = 1;
				dontIncrCwnd_ = 1;
			} 
		}
		if (cur->dupacks_+cur->later_acks_ >= sender->numdupacks_ &&
		    !cur->thresh_dupacks_) {
			cur->thresh_dupacks_ = 1;
			cur->sender_->num_thresh_dupack_segs_++;
		}

		if (amt_data_acked==0 && tcph->seqno()==cur->seqno_-1)
			done = 1;
		/* XXX we could check for rexmt candidates here if we ignore 
		   the num_thresh_dupack_segs_ check */
		if (!remove_flag &&
		    cur->seqno_ == cur->sender_->highest_ack_ + 1 &&
		    (cur->dupacks_ + cur->later_acks_ >= sender->numdupacks_ ||
		     cur->partialack_)) {
			curArray_[rexmtSegCount_] = cur;
			prevArray_[rexmtSegCount_] = prev;
			rexmtSegCount_++;
		}
		if (!remove_flag)
			prev = cur;
	}
	/* partial ack => terminate fast start mode */
	if (partialack && fs_enable_ && fs_mode_) {
		timeout(TCP_TIMER_RESET);
		rexmtSegCount_ = 0;
	}
	return latest_susp_loss;
}
	
void
CorresHost::add_agent(IntTcpAgent *agent, int /*size*/, double winMult, 
		      int winInc, int /*ssthresh*/)
{
	if (nActive_ >= MAX_PARALLEL_CONN) {
		printf("In add_agent(): reached limit of number of parallel conn (%d); returning\n", nActive_);
		return;
	}
	nActive_++;
	if ((!fixedIw_ && nActive_ > 1) || cwnd_ == 0)
		cwnd_ += 1; /* XXX should this be done? */
	wndInit_ = 1;
	winMult_ = winMult;
	winInc_ = winInc;
/*	ssthresh_ = ssthresh;*/
	conns_.append(agent);
}

int
CorresHost::ok_to_snd(int /*size*/)
{
	if (ownd_ <= -0.5)
		printf("In ok_to_snd(): ownd_ = %g  owndCorrection_ = %g\n", double(ownd_), double(owndCorrection_));
	return (cwnd_ >= ownd_+1);
}





