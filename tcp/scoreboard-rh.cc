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


/* 9/96 Pittsburgh Supercomputing Center
 *      UpdateScoreBoard, CheckSndNxt, MarkRetran modified for fack
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tcp/scoreboard-rh.cc,v 1.2 2000/08/12 21:45:39 sfloyd Exp $ (LBL)";
#endif

/*  A quick hack version of the scoreboard  */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <math.h>

#include "packet.h"
#include "scoreboard-rh.h"
#include "tcp.h"

#define ASSERT(x) if (!(x)) {printf ("Assert SB failed\n"); exit(1);}
#define ASSERT1(x) if (!(x)) {printf ("Assert1 SB (length)\n"); exit(1);}

#define SBNI SBN[i%SBSIZE]

// last_ack = TCP last ack
int ScoreBoardRH::UpdateScoreBoard (int last_ack, hdr_tcp* tcph, int rh_id)
{
	int i, sack_index, sack_left, sack_right;
	int sack_max = 0;
	int retran_decr = 0;

	/* Can't do this, because we need to process out the retran_decr  */
#if 0
	if (tcph->sa_length() == 0) {
		// There are no SACK blocks, so clear the scoreboard.
		this->ClearScoreBoard();
		return(0);
	}
#endif

	/*  What we do need to do is not create a scoreboard if we don't need one.  */
	if ((tcph->sa_length() == 0) && (length_ == 0)) {
		return(0);
	}
	

	//  If there is no scoreboard, create one.
	if (length_ == 0) {
		i = last_ack+1;
		SBNI.seq_no_ = i;
		SBNI.ack_flag_ = 0;
		SBNI.sack_flag_ = 0;
		SBNI.retran_ = 0;
		SBNI.snd_nxt_ = 0;
		SBNI.sack_cnt_ = 0;
		SBNI.rh_id_ = 0;
		first_ = i%SBSIZE;
		length_++;
		if (length_ >= SBSIZE) {
			printf ("Error, scoreboard too large (increase SBSIZE for more space)\n");
			exit(1);
		}
	}	

	//  Advance the left edge of the block.
	if (SBN[first_].seq_no_ <= last_ack) {
		for (i=SBN[(first_)%SBSIZE].seq_no_; i<=last_ack; i++) {
			//  Advance the ACK
			if (SBNI.seq_no_ <= last_ack) {
				ASSERT(first_ == i%SBSIZE);
				first_ = (first_+1)%SBSIZE; 
				length_--;
				ASSERT1(length_ >= 0);
				SBNI.ack_flag_ = 1;
				SBNI.sack_flag_ = 1;
				if (SBNI.retran_) {
					SBNI.retran_ = 0;
					SBNI.snd_nxt_ = 0;
					retran_decr++;
					retran_sacked_ = rh_id;
				}
				if (length_==0) 
					break;
			}
		}
	}

	for (sack_index=0; sack_index < tcph->sa_length(); sack_index++) {
		sack_left = tcph->sa_left(sack_index);
		sack_right = tcph->sa_right(sack_index);

		/*  Remember the highest segment SACKed by this packet  */
		if (sack_right > sack_max) {
			sack_max = sack_right;
		}

		//  Create new entries off the right side.
		if (sack_right > SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_) {
			//  Create new entries
			for (i = SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_+1; i<sack_right; i++) {
				SBNI.seq_no_ = i;
				SBNI.ack_flag_ = 0;
				SBNI.sack_flag_ = 0;
				SBNI.retran_ = 0;
				SBNI.snd_nxt_ = 0;
				SBNI.sack_cnt_ = 0;
				SBNI.rh_id_ = 0;
				length_++;
				if (length_ >= SBSIZE) {
					printf ("Error, scoreboard too large (increase SBSIZE for more space)\n");
					exit(1);
				}
			}
		}
		
		for (i=SBN[(first_)%SBSIZE].seq_no_; i<sack_right; i++) {
			//  Check to see if this segment is now covered by the sack block
			if (SBNI.seq_no_ >= sack_left && SBNI.seq_no_ < sack_right) {
				if (! SBNI.sack_flag_) {
					SBNI.sack_flag_ = 1;
				}
				if (SBNI.retran_) {
					SBNI.retran_ = 0;
					SBNI.snd_nxt_ = 0;
					retran_decr++;
					retran_sacked_ = rh_id;
				}
			}
		}
	}

	/*  Now go through the whole scoreboard and update sack_cnt
	    on holes which still exist.  */
	if (length_ != 0) {
		for (i=SBN[(first_)%SBSIZE].seq_no_; i<sack_max; i++) {
			//  Check to see if this segment is a hole
			if (!SBNI.ack_flag_ && !SBNI.sack_flag_) {
				SBNI.sack_cnt_++;
			}
		}
	}

	retran_decr += CheckSndNxt(sack_max);

	return (retran_decr);
}
int ScoreBoardRH::CheckSndNxt (int sack_max)
{
	int i;
	int num_lost = 0;

	if (length_ != 0) {
		for (i=SBN[(first_)%SBSIZE].seq_no_; i<sack_max; i++) {
			//  Check to see if this segment's snd_nxt_ is now covered by the sack block
			if (SBNI.retran_ && SBNI.snd_nxt_ < sack_max) {
				// the packet was lost again
				SBNI.retran_ = 0;
				SBNI.snd_nxt_ = 0;
				SBNI.sack_cnt_ = 1;
				num_lost++;
			}
		}
	}
	return (num_lost);


}

void ScoreBoardRH::ClearScoreBoard()
{
	length_ = 0;
}

/*
 * GetNextRetran() returns "-1" if there is no packet that is
 *   not acked and not sacked and not retransmitted.
 */
int ScoreBoardRH::GetNextRetran()	// Returns sequence number of next pkt...
{
	int i;

	if (length_) {
		for (i=SBN[(first_)%SBSIZE].seq_no_; 
		     i<SBN[(first_)%SBSIZE].seq_no_+length_; i++) {
			if (!SBNI.ack_flag_ && !SBNI.sack_flag_ && !SBNI.retran_
			    && (SBNI.sack_cnt_ >= *numdupacks_)) {
				return (i);
			}
		}
	}
	return (-1);
}


void ScoreBoardRH::MarkRetran (int retran_seqno, int snd_nxt, int rh_id)
{
	SBN[retran_seqno%SBSIZE].retran_ = 1;
	SBN[retran_seqno%SBSIZE].snd_nxt_ = snd_nxt;
	SBN[retran_seqno%SBSIZE].rh_id_ = rh_id;
	retran_occured_ = rh_id;
}

int ScoreBoardRH::GetFack (int last_ack)
{
	if (length_) {
		return(SBN[(first_)%SBSIZE].seq_no_+length_-1);
	}
	else {
		return(last_ack);
	}
}

int ScoreBoardRH::GetNewHoles ()
{
	int i, new_holes=0;

	for (i=SBN[(first_)%SBSIZE].seq_no_;
	     i<SBN[(first_)%SBSIZE].seq_no_+length_; i++) {
		//  Check to see if this segment is a new hole
#if 1
		if (!SBNI.ack_flag_ && !SBNI.sack_flag_ && SBNI.sack_cnt_ == 1) {
			new_holes++;
		}
#else
		if (!SBNI.ack_flag_ && !SBNI.sack_flag_ && SBNI.sack_cnt_ == *numdupacks_) {
			new_holes++;
		}
#endif
	}
	return (new_holes);
}

void ScoreBoardRH::TimeoutScoreBoard (int snd_nxt)
{
	int i, sack_right;

	if (length_ == 0) {
		// No need to do anything!  
		return;
	}

	sack_right = snd_nxt;  // Use this to know how far to extend.

	//  Create new entries off the right side.
	if (sack_right > SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_) {
		//  Create new entries
		for (i = SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_+1; i<sack_right; i++) {
			SBNI.seq_no_ = i;
			SBNI.ack_flag_ = 0;
			SBNI.sack_flag_ = 0;
			SBNI.retran_ = 0;
			SBNI.snd_nxt_ = 0;
			SBNI.sack_cnt_ = 0;
			SBNI.rh_id_ = 0;
			length_++;
			if (length_ >= SBSIZE) {
				printf ("Error, scoreboard too large (increase SBSIZE for more space)\n");
				exit(1);
			}
		}
	}

	/*  Now go through the whole scoreboard and update sack_cnt on holes;
	    clear retran flag on everything.  */
	for (i=SBN[(first_)%SBSIZE].seq_no_;
	     i<SBN[(first_)%SBSIZE].seq_no_+length_; i++) {
		//  Check to see if this segment is a hole
		if (!SBNI.ack_flag_ && !SBNI.sack_flag_) {
			SBNI.retran_ = 0;
			SBNI.snd_nxt_ = 0;
			SBNI.sack_cnt_ = *numdupacks_;  // This forces all holes to be retransmitted.
		}
	}

	/*  And, finally, check the first segment in case of a renege.  */
	i=SBN[(first_)%SBSIZE].seq_no_;
	if (!SBNI.ack_flag_ && SBNI.sack_flag_) {
		printf ("Renege!!! seqno = %d\n", SBNI.seq_no_);
		SBNI.sack_flag_ = 0;
		SBNI.retran_ = 0;
		SBNI.snd_nxt_ = 0;
		SBNI.sack_cnt_ = *numdupacks_;  // This forces it to be retransmitted.
	}
}

#if 0
/*  This routine inserts a fake sack block of length num_dupacks,
    starting at last_ack+1.  It is for use during NewReno recovery.
*/
int ScoreBoardRH::FakeSack (int last_ack, int num_dupacks)
{
	int i, sack_left, sack_right;
	int retran_decr = 0;

	//  If there is no scoreboard, create one.
	if (length_ == 0) {
		i = last_ack+1;
		SBNI.seq_no_ = i;
		SBNI.ack_flag_ = 0;
		SBNI.sack_flag_ = 0;
		SBNI.retran_ = 0;
		SBNI.snd_nxt_ = 0;
		SBNI.sack_cnt_ = 0;
		SBNI.rh_id_ = 0;
		first_ = i%SBSIZE;
		length_++;
		if (length_ >= SBSIZE) {
			printf ("Error, scoreboard too large (increase SBSIZE for more space)\n");
			exit(1);
		}
	}	

	//  Advance the left edge of the scoreboard.
	if (SBN[first_].seq_no_ <= last_ack) {
		for (i=SBN[(first_)%SBSIZE].seq_no_; i<=last_ack; i++) {
			//  Advance the ACK
			if (SBNI.seq_no_ <= last_ack) {
				ASSERT(first_ == i%SBSIZE);
				first_ = (first_+1)%SBSIZE; 
				length_--;
				ASSERT1(length_ >= 0);
				SBNI.ack_flag_ = 1;
				SBNI.sack_flag_ = 1;
				if (SBNI.retran_) {
					SBNI.retran_ = 0;
					SBNI.snd_nxt_ = 0;
					retran_decr++;
				}
				if (length_==0) 
					break;
			}
		}
		/*  Now create a new hole in the first position  */
		i=SBN[(first_)%SBSIZE].seq_no_;
		SBNI.ack_flag_ = 0;
		SBNI.sack_flag_ = 0;
		SBNI.retran_ = 0;
		SBNI.snd_nxt_ = 0;
		SBNI.rh_id_ = 0;
		SBNI.sack_cnt_ = num_dupacks;
	}

	sack_left = last_ack + 1;
	sack_right = sack_left + num_dupacks - 1;

	//  Create new entries off the right side.
	if (sack_right > SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_) {
		//  Create new entries
		for (i = SBN[(first_+length_+SBSIZE-1)%SBSIZE].seq_no_+1; i<sack_right; i++) {
			SBNI.seq_no_ = i;
			SBNI.ack_flag_ = 0;
			SBNI.sack_flag_ = 0;
			SBNI.retran_ = 0;
			SBNI.snd_nxt_ = 0;
			SBNI.sack_cnt_ = 0;
			SBNI.rh_id_ = 0;
			length_++;
			if (length_ >= SBSIZE) {
				printf ("Error, scoreboard too large (increase SBSIZE for more space)\n");
				exit(1);
			}
		}
	}
		
	for (i=SBN[(first_)%SBSIZE].seq_no_; i<sack_right; i++) {
		//  Check to see if this segment is now covered by the sack block
		if (SBNI.seq_no_ >= sack_left && SBNI.seq_no_ < sack_right) {
			if (! SBNI.sack_flag_) {
				SBNI.sack_flag_ = 1;
			}
			if (SBNI.retran_) {
				SBNI.retran_ = 0;
				retran_decr++;
			}
		}
	}

	/*  Now go through the whole scoreboard and update sack_cnt
	    on holes which still exist.  In this case the only possible 
	    case is the first hole.  */
        i=SBN[(first_)%SBSIZE].seq_no_;
	//  Check to see if this segment is a hole
	if (!SBNI.ack_flag_ && !SBNI.sack_flag_) {
		SBNI.sack_cnt_++;
	}

	return (retran_decr);
}

#endif

int ScoreBoardRH::RetranSacked (int rh_id) {
	return (retran_sacked_ == rh_id);
}
