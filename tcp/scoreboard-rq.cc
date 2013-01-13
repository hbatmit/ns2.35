/* Copyright (c) 2002 Tom Kelly, University of Cambridge
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "scoreboard-rq.h"

// Implementation of a ScoreBoard shim for 
// the FullTCP reassembly queue code

int ScoreBoardRQ::IsEmpty(){
	printf("ScoreBoardRQ::IsEmpty not implemented\n");
	exit(1);
	return 0;
}

int ScoreBoardRQ::GetNextRetran(){
	int seq;
	int fcnt;
	int fbytes;

	if(h_seqno_ < sack_min) h_seqno_ = sack_min;
	seq = h_seqno_;
	
	if((seq = rq_.nexthole(seq, fcnt, fbytes)) > 0) {
		// adjust h_seqno, as we may have
		// been "jumped ahead" by learning
		// about a filled hole
		
		if(fcnt <= 0) return (-1); // no holes above

		//printf("%.4f ", Scheduler::instance().clock());
		//printf("GetNextRetran sack_min: %i h_seqno: %i seq: %i\n", sack_min, h_seqno_, seq);
		if(seq > h_seqno_)
			h_seqno_ = seq;

		return (seq);
	}
	return (-1);
}

int ScoreBoardRQ::UpdateScoreBoard(int last_ack_, hdr_tcp* tcph){
	int old_total = rq_.total();
	changed_ = 0;
	
	if(sack_min <= last_ack_){
		// beginning of retransmission queue is one beyond last_ack
		sack_min = last_ack_+1; 
		if(!rq_.empty()){
			rq_.cleartonxt();
			changed_ = 1;
		}
	}

	for(int i = 0 ; i < tcph->sa_length() ; i++){
		//printf("l: %i r: %i\n", tcph->sa_left(i), tcph->sa_right(i));
		rq_.add(tcph->sa_left(i), tcph->sa_right(i), 0); 
	}
	changed_ = changed_  || (old_total != rq_.total());
	return 0;
	
	//printf("UpdateScoreBoard dump changed_: %i\n", changed_);
	//printf("%.4f ", Scheduler::instance().clock());
	//rq_.dumplist();
	
}


/*
 * GetNextUnacked returns sequence number of next unacked pkt,
 * starting with seqno.
 * Returns -1 if there is no unacked packet in that range.
 */
int ScoreBoardRQ::GetNextUnacked (int seqno)
{
      int nxtcnt;	// not used
      int nxtbytes;	// not used
      int unacked = rq_.nexthole(seqno, nxtcnt, nxtbytes);
      return (unacked);
}

int ScoreBoardRQ::CheckSndNxt(hdr_tcp* ) {
	printf("ScoreBoardRQ::CheckSndNxt not implemented\n");
	exit(1);
	return 0;
}

void ScoreBoardRQ::Dump() {
  rq_.dumplist();
}

void ScoreBoardRQ::MarkRetran (int retran_seqno, int)
{
	if (retran_seqno >= h_seqno_) 
		h_seqno_ = retran_seqno+1;
}

