/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 The Regents of the University of California.
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
 * TCP-Linux module for NS2 
 *
 * May 2006
 *
 * Author: Xiaoliang (David) Wei  (DavidWei@acm.org)
 *
 * NetLab, the California Institute of Technology 
 * http://netlab.caltech.edu
 *
 * Module: scoreboard1.cc
 *      This is the Scoreboard implementation for TCP-Linux in NS-2.
 *      We loosely follow the Linux implementation (clearn_rtx_queue and sacktag_write_queue in tcp_input.c) in Scoreboard Update.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "scoreboard1.h"
#include "tcp.h"
#include "linux/ns-linux-util.h"

#define ASSERT(x) if (!(x)) {printf ("Assert SB failed\n"); exit(1);}
#define ASSERT1(x) if (!(x)) {printf ("Assert1 SB (length)\n"); exit(1);}
#define ASSERT2(x,y...) if (!(x)) {printf(y); exit(1); }

bool ScoreBoard1::CleanRtxQueue(int last_ack, unsigned char* flag) 
{
	int cmp;
	bool changed_acked_rtx = false;
	//clean the acked packets
	while ((head_) && ((cmp=head_->ShouldClean(last_ack))!=0)) {
		int pkt_num = (cmp<0) ? head_->GetLength(): cmp;
		switch (head_->GetFlag()) {
		case SKB_FLAG_SACKED: 
			sack_out_ -= pkt_num;
			ASSERT2((sack_out_>=0), "SACK_OUT %d get negative when advancing edge, last_ack %d\n", sack_out_, last_ack);
			break;
		case SKB_FLAG_LOST:
			fack_out_ -= pkt_num;
			ASSERT2((fack_out_>=0), "fack_out %d get negative when advancing edge, last_ack %d\n", fack_out_, last_ack);
			break;
		}
		//printf("%d: %d %d\n", last_ack, cmp, head_->GetFlag());

		if (cmp<0) {
			ScoreBoardNode1* tmp=head_;
			head_=head_->GetNext();
			if (tmp->GetFlag()==SKB_FLAG_RETRANSMITTED) {
				if (tmp->GetRtx()>acked_rtx_id_) {
					changed_acked_rtx = true;
					acked_rtx_id_ = tmp->GetRtx();
				}
				(*flag) |= FLAG_UNSURE_TSTAMP;
			}
			if (tmp == nxt_to_retrx_) nxt_to_retrx_ = head_;
			delete tmp;
		} else {
			head_->SetBegin(last_ack+1);
			break;
		}
	}
	return changed_acked_rtx;
}
// last_ack = TCP last ack
int ScoreBoard1::UpdateScoreBoard (int last_ack, hdr_tcp* tcph, int dupack_threshold)
{
	// This one pass process is not completely accurate:
	//
	// There is some case such that the acked_rtx_id_ is increased in the later part of the process,
	// and this change may mark some earlier packet in the queue to be lost 
	// One example:
	// 1 (retran=1) 2-5 (sacked) 6(retran=5) 
	// if this sack block has 6 in its sack, 1 won't be marked lost in the round.
	// However, 1 will be marked as lost when the next SACK arrives
	//
	// Also, if there is some new packet loss happens after the right most seq of this SACK, this
	// pass will not find the loss since it stops at the right most seq of this SACK.
	// However, the lost packets will be marked when there is new SACK come back. 
	//printf("clean rtx\n");
	unsigned char flag = 0;
	bool thorough = CleanRtxQueue(last_ack, &flag);

	if (head_==NULL) {
		ClearScoreBoard();
		thorough=false;
		// if there is no outstanding lost packets, we might not need to do the rest
	}

	if ((!thorough) && (tcph->sa_length()==0)) return (flag);

	// SACK pre-processing
	// we filter D-SACK and sort the SACKs below
	int sa_length = 0;
	int left[NSA+1];
	int right[NSA+1];
	for (int i=0; i < tcph->sa_length(); i++) {
		int sack_left = tcph->sa_left(i);
		int sack_right = tcph->sa_right(i);
		if (sack_left >= last_ack) {	//here we filter the D-SACK
			int j = sa_length;
			sa_length++;
			while ((j>0) && (left[j-1] > sack_left)) { 
				left[j] = left[j-1]; 
				right[j] = right[j-1]; 
				j--;
			}
			//insert to the right place to keep increasing order
			left[j] = sack_left;
			right[j] = sack_right;
		}
	}
	if ((!thorough)&&(sa_length == 0)) return (flag);	//same reason as the first return 0

	if ((sa_length > 0) && (fack_ < (right[sa_length-1]-1))) {
	// advance fack_. right[sa_length-1]-1 is the right most of all the sacks in the packet. 
		fack_ = right[sa_length-1]-1;
		thorough = true;
	}

	//  If there is no scoreboard, create one.
	if (head_==NULL) {
		head_ = new ScoreBoardNode1(last_ack+1, fack_,0);
		//initially, create a block from snd_una to facked, 
		//  with flag==0 meaning that packets are in flight
	};

	// From now, we process the score board
	nxt_to_retrx_ = NULL;


//	printf("start traverse\n");
	int lost_seq_threshold = fack_ - (dupack_threshold+1);
	ScoreBoardNode1* now = head_;	// the block we are processing
	ScoreBoardNode1* prev = NULL;
	int sack_index = 0;	//the sack that is currently focused 
	while ((now != NULL) && ((sack_index<sa_length)||thorough)) {
		if (now->GetFlag()!=SKB_FLAG_SACKED) {
			while ((sack_index<sa_length) && (right[sack_index]<=now->GetStart()) )
				sack_index++;
			//find the sack that match "now"
			if (sack_index<sa_length) {
//printf("Sack block: [%d %d], now block: %d[%d %d]\n", left[sack_index], right[sack_index], now->GetFlag(), now->GetStart(), now->GetEnd());
				if (left[sack_index]<=now->GetEnd()) {
				// We share some intersection between this SACK and this "now"
					if (left[sack_index]>now->GetStart()) {
						//printf("split to make some sackes\n");
						// the first part of "now" is not SACKED
						now->Split(left[sack_index]);
					} else {
						if (right[sack_index]<=now->GetEnd()) {
							//partially sacked
							//printf("going to split\n");
							now->Split(right[sack_index]);
							//printf("done split\n");
						};
						int pktnum = now->GetLength();
						switch (now->GetFlag()) {
						case SKB_FLAG_LOST:
							fack_out_ -= pktnum;
							break;
						case SKB_FLAG_RETRANSMITTED:
							acked_rtx_id_ = now->GetRtx();
							flag |= FLAG_UNSURE_TSTAMP;
							thorough = true;
							break;			
						}
						sack_out_ += pktnum;
						flag |= FLAG_DATA_SACKED;
						now->SetFlag(SKB_FLAG_SACKED);
					}
				};
			}
			if ((sack_index>=sa_length)|| (left[sack_index]>now->GetEnd())) {
//printf("now block: %d[%d %d]\n", now->GetFlag(),now->GetStart(), now->GetEnd());

				switch (now->GetFlag()) {
				case SKB_FLAG_RETRANSMITTED:
					// check if it is lost again
					if (((now->GetSndNxt() <= fack_)
					||((now->GetRtx() + dupack_threshold+1) < acked_rtx_id_))) {
						now->SetFlag(SKB_FLAG_LOST);
						fack_out_++;
						flag |= FLAG_DATA_LOST;
						if (!nxt_to_retrx_) nxt_to_retrx_ = now;
					} 
					break;
				case SKB_FLAG_INFLIGHT:
					//check if it is lost
					if (now->GetStart() <= lost_seq_threshold) {
					//some packets might be lost
						if (lost_seq_threshold<now->GetEnd()) {
							//partial loss
							now->Split(lost_seq_threshold+1);
						}
						now->SetFlag(SKB_FLAG_LOST);
						fack_out_ += now->GetLength();
						flag |= FLAG_DATA_LOST;
						if (!nxt_to_retrx_) nxt_to_retrx_ = now;
					}
					break;
				case SKB_FLAG_LOST:
					if (!nxt_to_retrx_) nxt_to_retrx_ = now;
					break;
				}
			}
		}
//printf("now block: %d[%d %d]\n", now->GetFlag(),now->GetStart(), now->GetEnd());

		if ((prev) && (prev->Mergable())) {
			//Try to merge with previous node
			//printf("Merging %d [%d,%d] + %d [%d, %d]\n", prev->GetFlag(), prev->GetStart(), prev->GetEnd(), now->GetFlag(), now->GetStart(), now->GetEnd());
			prev->Merge();
		} else {
			prev = now;
		}

		if ((prev->GetNext()==NULL) && (prev->GetEnd()<fack_)) {
			//printf("appending %d %d\n",prev->GetEnd()+1, fack_);
			prev->Append(new ScoreBoardNode1(prev->GetEnd()+1, fack_,0));
			//extend the scoreboard
		}
		now = prev -> GetNext();
	};

	return (flag);

}

void ScoreBoard1::MarkLoss(int snd_una, int snd_nxt) {
	nxt_to_retrx_ = NULL;
	ScoreBoardNode1* now = head_;
	ScoreBoardNode1* prev = NULL;
	bool last_lost = false;
	while (now) {
		if (!(now->GetFlag() & (SKB_FLAG_SACKED))) {
			if (! (now->GetFlag() & SKB_FLAG_LOST)) 
				fack_out_ += now->GetEnd() - now->GetStart() + 1;
			now->SetFlag(SKB_FLAG_LOST);
			if (last_lost) {
				prev->Merge();
				now=prev;
			} else {
				if (nxt_to_retrx_==NULL) nxt_to_retrx_ = now;
			}
			last_lost = true;
		} else {
			last_lost = false;
		}
		prev = now;
		now = now->GetNext();
		if ((now==NULL) && ((prev->GetEnd()+1) < snd_nxt)) {
			//all the packets that we sent have lost
			now = new ScoreBoardNode1(prev->GetEnd()+1, snd_nxt-1, SKB_FLAG_INFLIGHT);
			prev->Append(now);
		}
	}
	if ((prev==NULL) && (snd_una<snd_nxt)) {
		//the score board is empty
		head_ = new ScoreBoardNode1(snd_una, snd_nxt-1, SKB_FLAG_LOST);
		nxt_to_retrx_ = head_;
		fack_out_ += snd_nxt - snd_una;
	}
	rtx_id_ = 0;
	acked_rtx_id_ = -1;	
}

void ScoreBoard1::ClearScoreBoard()
{
	rtx_id_ = 0;
	acked_rtx_id_ = -1;
        fack_out_ = 0;
        sack_out_ = 0;
	fack_ = 0;
	nxt_to_retrx_ = NULL;
	while (head_) {
		ScoreBoardNode1* temp = head_;
		head_ = head_->GetNext();
		delete temp;
	}
}

/*
 * GetNextRetran() returns "-1" if there is no packet that is
 *   not acked and not sacked and not retransmitted.
 */
int ScoreBoard1::GetNextRetran()
{
	while ((nxt_to_retrx_) && (nxt_to_retrx_->GetFlag() != SKB_FLAG_LOST))
		nxt_to_retrx_ = nxt_to_retrx_->GetNext();
	if (nxt_to_retrx_) {
		return nxt_to_retrx_->GetStart();
	} else
		return -1;
}

void ScoreBoard1::MarkRetran (int retran_seqno, int snd_nxt)
{
	ASSERT2(((nxt_to_retrx_) && (nxt_to_retrx_->Inside(retran_seqno))), "Trying to mark a retransmission outside the recommended one: %p %d, snd_nxt: %d\n", nxt_to_retrx_, retran_seqno, snd_nxt);
	fack_out_ --;
	ASSERT2((fack_out_>=0), "fack_out gets negative in Mark Retran, %d, snd_nxt: %d, rtx_seq:%d\n", fack_out_, snd_nxt, retran_seqno);
	nxt_to_retrx_->MarkRetran(snd_nxt, rtx_id_);
	rtx_id_++;
	last_rtx_seq_=retran_seqno;
}


void ScoreBoard1::Dump()
{
       if (head_ == NULL ) { printf("SB: No entry\n"); return; };

       printf("SB len:%d fack:%d sack_out:%d fack_out:%d acked_rtx_id:%d next_rtx_id:%d\n", 
		fack_-head_->GetStart(), 
		fack_, 
		sack_out_,
		fack_out_,
		acked_rtx_id_,
		rtx_id_
	);

	ScoreBoardNode1* now=head_;
	while (now) {
		if (now->GetFlag()==SKB_FLAG_RETRANSMITTED) 
			printf("seq:%d (%d snd_nxt:%d rtxid:%d)\n", now->GetStart(), now->GetFlag(), now->GetSndNxt(), now->GetRtx());
		else
			printf("seq:[%d,%d] (%d)\n", now->GetStart(), now->GetEnd(), now->GetFlag());
		now=now->GetNext();
       }
       printf("\n");
}

void ScoreBoard1::test() 
{
	ClearScoreBoard();
	hdr_tcp test;
	test.sa_length_ =  1;
	test.sack_area_[0][0]= 4;
	test.sack_area_[0][1]= 5;
	
	printf(" 1 x x 4  \n");
	UpdateScoreBoard(1, &test); Dump();
        test.sack_area_[0][0]= 7;
        test.sack_area_[0][1]= 8;
	
	printf(" 1 x x 4 x x 7 \n");
	UpdateScoreBoard(1, &test); Dump();

	printf(" 1 R x 4 x x 7\n");
	MarkRetran(GetNextRetran(),8); Dump();
	
	test.sack_area_[0][0]= 9;
	test.sack_area_[0][1]= 10;
	printf(" 1 x x 4 x x 7 x 9 \n");
	UpdateScoreBoard(1, &test); Dump();

	test.sack_area_[0][0]= 7;
        test.sack_area_[0][1]= 9;
        printf(" 1 x x 4 x x 7 8 9\n");
        UpdateScoreBoard(1, &test); Dump();

	printf(" 1 R x 4 x x 7 8 9\n");
        MarkRetran(GetNextRetran(),60); Dump();

	printf(" 1 R R 4 x x 7 8 9\n");
        MarkRetran(GetNextRetran(),60); Dump();

	test.sack_area_[0][0]= 3;
        test.sack_area_[0][1]= 4;
        printf(" 1 R 3 4 x x 7 8 9\n");
        UpdateScoreBoard(1, &test); Dump();

	test.sack_area_[0][0]= 15;
        test.sack_area_[0][1]= 19;
        printf(" 1 R 3 4 x x 7 8 9 x x x x x 15 - 19\n");
        UpdateScoreBoard(1, &test); Dump();

	printf(" 1 R R 4 R x 7 8 9 x x x x x 15 - 19\n");
        MarkRetran(GetNextRetran(),60); Dump();

	printf(" 1 R 3 4 R R 7 8 9 x x x x x 15 - 19\n");
        MarkRetran(GetNextRetran(),60); Dump();

	printf(" 1 R R 4 R R 7 8 9 R x x x x 15 - 19\n");
        MarkRetran(GetNextRetran(),60); Dump();

        printf(" 1 R 3 4 R R 7 8 9 R R x x x 15 16\n");
        MarkRetran(GetNextRetran(),60); Dump();

        printf(" 1 R 3 4 R R 7 8 9 R R R x x 15 16\n");
        MarkRetran(GetNextRetran(),60); Dump();

        printf(" 1 R 3 4 R R 7 8 9 R R R R x 15 16\n");
        MarkRetran(GetNextRetran(),60); Dump();

	printf(" 1 R 3 4 R R 7 8 9 R R R R R 15 16\n");
        MarkRetran(GetNextRetran(),60); Dump();

        test.sack_area_[0][0]= 5;
        test.sack_area_[0][1]= 11;
        printf(" 1 R 3 4 5 6 7 8 9 10 R R R R 15 16\n");
        UpdateScoreBoard(1, &test); Dump();

	test.sack_area_[0][0]= 25;
        test.sack_area_[0][1]= 40;
        printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16 25-40\n");
        UpdateScoreBoard(1, &test); Dump();

	printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16 R\n");
        MarkRetran(GetNextRetran(),90); Dump();
	printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16 R R\n");
        MarkRetran(GetNextRetran(),20); Dump();
	printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16 R R R\n");
        MarkRetran(GetNextRetran(),20); Dump();
	printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16 R R R R\n");
        MarkRetran(GetNextRetran(),20); Dump();
	printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16 R R R R R\n");
        MarkRetran(GetNextRetran(),20); Dump();
	printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16 R R R R R R\n");
        MarkRetran(GetNextRetran(),20); Dump();

        test.sack_area_[0][0]= 11;
        test.sack_area_[0][1]= 16;
        printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 14 15 16 25-40\n");
        UpdateScoreBoard(1, &test); Dump();

	test.sack_area_[0][0]= 11;
        test.sack_area_[0][1]= 16;
        printf(" 1 x 3 4 5 6 7 8 9 10 11 12 13 14 15 16 25-40\n");
        UpdateScoreBoard(1, &test); Dump();


	printf(" 1 R 3 4 5 6 7 8 9 10 11 12 13 R 15 16\n");
	MarkRetran(GetNextRetran(),20); Dump();

	UpdateScoreBoard(3, &test); Dump();
}
