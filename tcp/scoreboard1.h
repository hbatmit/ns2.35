/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) @ Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
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
 * Module: scoreboard1.h
 *      This is the header file for Scoreboard1 in TCP-Linux.
 *	We define the states of a packet in here (SKB_FLAG_*).
 *
 */

#ifndef ns_scoreboard1_h
#define ns_scoreboard1_h

//  Definition of the scoreboard class:
#include "tcp.h"

#define SKB_FLAG_INFLIGHT 0		/* in flight, new packets */
#define SKB_FLAG_SACKED 1		/* Acked by SACK block */
#define SKB_FLAG_LOST 2			/* lost by FACK signal */
#define SKB_FLAG_RETRANSMITTED 4	/* in flight, but is retransmitted packets */

class ScoreBoardNode1 {
public:
	ScoreBoardNode1(int start, int end, char flag):
		flag_(flag),
		seq_(start),
		nxt_(end),
		next_in_queue_(NULL)
	{}
	inline char GetFlag() { return flag_; }

	inline int GetStart() { return seq_; }
	inline int GetEnd() { return (flag_==SKB_FLAG_RETRANSMITTED)?seq_:nxt_; }
	inline int GetLength() {return (flag_==SKB_FLAG_RETRANSMITTED)?1:(nxt_+1-seq_); }
	inline int GetRtx() { return retran_;}
	inline int GetSndNxt() { return nxt_; }
	inline void SetFlag(char flag) { if (flag_==SKB_FLAG_RETRANSMITTED) nxt_=seq_; flag_ = flag;}
	inline void SetBegin(int seq) { seq_=seq; }
	inline void Append(ScoreBoardNode1* next) {next_in_queue_ = next;}
	inline bool Inside(int seq) { return (flag_==SKB_FLAG_RETRANSMITTED)?(seq==seq_):((seq>=seq_) && (seq<=nxt_)); }
//        inline int Compare(int seq) { if (flag_==SKB_FLAG_RETRANSMITTED) return (seq-seq_); else if (seq<seq_) return -1; else if (seq>nxt_) return 1; else return 0;}
	inline int ShouldClean(int ackseq) 
	// return: 0: this block should not be touched; -1: the whole block should be deleted; 
	//	>0: the first # of packets should be deleted
	{
		if (ackseq<seq_) 
			return 0;
		else if ((flag_==SKB_FLAG_RETRANSMITTED)||(ackseq>=nxt_)) return -1; 
		else return ackseq+1-seq_;
	}

	inline ScoreBoardNode1* GetNext() { return next_in_queue_; }

	inline bool Mergable() { return ((next_in_queue_) && (next_in_queue_->flag_!=SKB_FLAG_RETRANSMITTED) && (next_in_queue_->flag_ == flag_)); }
	//to check if the next packet can be merged to this packet

	void Merge() 
	{ 
	//merge with the next packet: WARNING: call this function ONLY WHEN you check the validity by Mergable.	
		ScoreBoardNode1* next = next_in_queue_;
		nxt_ = next->nxt_;
		next_in_queue_ = next->next_in_queue_;
		delete next;
	}


	inline bool Splittable(int seq) { return (flag_!=SKB_FLAG_RETRANSMITTED) && (seq>seq_) && (seq<=nxt_);}
	//to check if a node can be split from seq.

	ScoreBoardNode1* Split(int seq) 
	{
	// split the current node by the seq; the new node starts is [seq, end of the block]; return the new node
	// WARNING: Make sure it is Splittable before this functionis called.
		ScoreBoardNode1* next = new ScoreBoardNode1(seq, nxt_, flag_);
		next->next_in_queue_ = next_in_queue_;
		next_in_queue_ = next;
		nxt_ = seq-1;
		return next;
	}

	void MarkRetran(int snd_nxt, int retrans_id) 
	// Mark the first packet of this block as a retransmission packet
	// WARNING: Make sure it is a lost block before calling this function
	{
		if (nxt_ > seq_ ) Split(seq_+1);
		flag_ = SKB_FLAG_RETRANSMITTED; 
		nxt_ = snd_nxt; 
		retran_ = retrans_id; 
	}

private:
	char flag_; 
	int seq_;		/* Packet number */
	int nxt_;		
	/* This member has two different meanings: 
	1. If the flag_ is not SKB_FLAG_RETRANSMITTED, this means the right edge (included) of the block: [seq_, nxt_]
	2. If the flag_ is SKB_FLAG_RETRANSMITTED, this means the snd_nxt_ at the time of retransmission (retransmitted packet is one packet per node)
	*/
	int retran_;  /* Packet retransmitted or not. If retran_ == 0: not retransmitted, if retran_>0, it's the rtx_id_ when it is retransmitted. */
	/* the combination of (retran_, snd_nxt_) can detect the loss of retransmitted packet in an accurate way */
//	double when;		//We don't have head timeout yet
	ScoreBoardNode1* next_in_queue_;
};


class ScoreBoard1 {
  public:
	ScoreBoard1(): head_(NULL), last_rtx_seq_(-1) {ClearScoreBoard();} 
	virtual ~ScoreBoard1(){ClearScoreBoard ();}
	virtual int IsEmpty () {return (head_ == NULL);}
	virtual void ClearScoreBoard (); 
	virtual int GetNextRetran ();
	virtual void Dump();
//	virtual void MarkRetran (int retran_seqno);
	virtual void MarkRetran (int retran_seqno, int snd_nxt);
	virtual int UpdateScoreBoard (int last_ack_, hdr_tcp*, int dupack_threshold=3);
	virtual void MarkLoss(int snd_una, int snd_nxt);
	//the return value: flags
	inline int FackOut() { return fack_out_; }
	inline int SackOut() { return sack_out_; } 
	inline int packets_in_flight(int snd_una, int snd_nxt){return snd_nxt - snd_una - fack_out_ - sack_out_;}
	inline int fack() {return fack_;}
//	inline bool sure_timestamp(int seq) { return seq>last_rtx_seq_;}
	void test();

  protected:
	bool CleanRtxQueue(int last_ack, unsigned char* flag);

	ScoreBoardNode1* head_;
	ScoreBoardNode1* nxt_to_retrx_;
	//the next packet to be retransmitted. if nxt_to_retrx_==NULL: no packet can be retransmitted.


	int acked_rtx_id_, rtx_id_;	//the current id of retransmitted packet
	int fack_;		// the furthest sacked seq#
	int fack_out_;		//# of packets that are in the scoreboard, which are deemed to be lost
	int sack_out_;		//# of packets that are in the scoreboard, which are sacked.
	int last_rtx_seq_;	// the seqno that we cleaned scoreboard last time. 

};

#endif
