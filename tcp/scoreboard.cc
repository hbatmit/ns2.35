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

/*  A quick hack version of the scoreboard  */
#include <stdlib.h>
#include <stdio.h>

#include "scoreboard.h"
#include "tcp.h"

#define ASSERT(x) if (!(x)) {printf ("Assert SB failed\n"); exit(1);}
#define ASSERT1(x) if (!(x)) {printf ("Assert1 SB (length)\n"); exit(1);}

#define SBNI SBN[i%sbsize_]

// last_ack = TCP last ack
int ScoreBoard::UpdateScoreBoard (int last_ack, hdr_tcp* tcph)
{
	int i, sack_index, sack_left, sack_right;
	int retran_decr = 0;
	
	changed_ = 0;
			
	//  Advance the left edge of the block.
	if (length_ && SBN[first_%sbsize_].seq_no_ <= last_ack) {
		for (i=SBN[first_%sbsize_].seq_no_; i<=last_ack; i++) {
			//  Advance the ACK
			if (SBNI.seq_no_ <= last_ack) {
				ASSERT(first_ == i);
				first_ = (first_+1);
				length_--;
				ASSERT1(length_ >= 0);
				SBNI.ack_flag_ = 1;
				SBNI.sack_flag_ = 1;
				if (SBNI.retran_) {
					SBNI.retran_ = 0;
					SBNI.snd_nxt_ = 0;
					retran_decr++;
				}
				changed_++;
				if (length_==0) 
					break;
			}
		}
	}

	//  If there is no scoreboard, create one.
	if (length_ == 0 && tcph->sa_length()) {
		i = last_ack+1;
		SBNI.seq_no_ = i;
		SBNI.ack_flag_ = 0;
		SBNI.sack_flag_ = 0;
		SBNI.retran_ = 0;
		SBNI.snd_nxt_ = 0;
		first_ = i;
		length_++;
		if (length_ >= sbsize_) {
			printf ("Error, scoreboard too large (increase sbsize_ for more space)\n");
			exit(1);
		}
		changed_++;
	}	

	for (sack_index=0; sack_index < tcph->sa_length(); sack_index++) {
		sack_left = tcph->sa_left(sack_index);
		sack_right = tcph->sa_right(sack_index);
		
		//  Create new entries off the right side.
		if (sack_right > SBN[(first_+length_+sbsize_-1)%sbsize_].seq_no_) {

			// Resize the scoreboard if it is going to overrun the length
			while((sack_right - last_ack) >= sbsize_ -1 ){
				resizeSB(sbsize_*2);
			}

			//  Create new entries
			for (i = SBN[(first_+length_+sbsize_-1)%sbsize_].seq_no_+1; i<sack_right; i++) {
				SBNI.seq_no_ = i;
				SBNI.ack_flag_ = 0;
				SBNI.sack_flag_ = 0;
				SBNI.retran_ = 0;
				SBNI.snd_nxt_ = 0;
				length_++;
				if (length_ >= sbsize_) {
					fprintf(stderr, "ERROR: Scoreboard got too large!!!\n");
					fprintf(stderr, " SBN[first (mod) sbsize_]: %i, sack_right: %i length_: %i\n", SBN[first_%sbsize_].seq_no_,sack_right , length_);
					fprintf(stderr, "last_ack: %i SBN[(first_+length_+sbsize_-1) (mod) sbsize_].seq_no_: %i, sbsize_: %i\n", last_ack, SBN[(first_+length_+sbsize_-1)%sbsize_].seq_no_ , sbsize_);
 					exit(1);
				}
				changed_++;
			}
		}
		
		for (i=SBN[(first_)%sbsize_].seq_no_; i<sack_right; i++) {
			//  Check to see if this segment is now covered by the sack block
			if (SBNI.seq_no_ >= sack_left && SBNI.seq_no_ < sack_right) {
				if (! SBNI.sack_flag_) {
					SBNI.sack_flag_ = 1;
					changed_++;
				}
				if (SBNI.retran_) {
					SBNI.retran_ = 0;
					retran_decr++;
				}
			}
		}
	}
	return (retran_decr);
}
int ScoreBoard::CheckSndNxt (hdr_tcp* tcph)
{
	int i, sack_index, sack_right;
	int force_timeout = 0;

	for (sack_index=0; sack_index < tcph->sa_length(); sack_index++) {
		sack_right = tcph->sa_right(sack_index);

		for (i=SBN[(first_)%sbsize_].seq_no_; i<sack_right; i++) {
			//  Check to see if this segment's snd_nxt_ is now covered by the sack block
			if (SBNI.retran_ && SBNI.snd_nxt_ < sack_right) {
				// the packet was lost again
				SBNI.retran_ = 0;
				SBNI.snd_nxt_ = 0;
				force_timeout = 1;
			}
		}
	}
	return (force_timeout);
}

void ScoreBoard::ClearScoreBoard()
{
	length_ = 0;
}

/*
 * GetNextRetran() returns "-1" if there is no packet that is
 *   not acked and not sacked and not retransmitted.
 */
int ScoreBoard::GetNextRetran()	// Returns sequence number of next pkt...
{
	int i;

	if (length_) {
		for (i=SBN[(first_)%sbsize_].seq_no_; 
		     i<SBN[(first_)%sbsize_].seq_no_+length_; i++) {
			if (!SBNI.ack_flag_ && !SBNI.sack_flag_ && !SBNI.retran_) {
				return (i);
			}
		}
	}
	return (-1);
}


/*
 * GetNextUnacked returns sequence number of next unacked pkt,
 * starting with seqno.
 * Returns -1 if there is no unacked packet in that range.
 */
int ScoreBoard::GetNextUnacked (int seqno)
{
	int i;
	if (!length_) {
		return (-1);
	} else if (seqno < SBN[(first_)%sbsize_].seq_no_ ||
		seqno >= SBN[(first_)%sbsize_].seq_no_+length_) {
		return (-1);
	} else {
		for (i=seqno; i<SBN[(first_)%sbsize_].seq_no_+length_; i++) {
			if (!SBNI.ack_flag_ && !SBNI.sack_flag_) {
				return (i);
			}
		}
	}
	return (-1);

}

void ScoreBoard::MarkRetran (int retran_seqno, int snd_nxt)
{
	SBN[retran_seqno%sbsize_].retran_ = 1;
	SBN[retran_seqno%sbsize_].snd_nxt_ = snd_nxt;
}

void ScoreBoard::MarkRetran (int retran_seqno)
{
	SBN[retran_seqno%sbsize_].retran_ = 1;
}

void ScoreBoard::resizeSB(int sz)
{
	ScoreBoardNode *newSBN = new ScoreBoardNode[sz+1];

	if(!newSBN){
		fprintf(stderr, "Unable to allocate new ScoreBoardNode[%i]\n", sz);
		exit(1);
	}

	for(int i = SBN[first_%sbsize_].seq_no_;
	    i<=SBN[(first_)%sbsize_].seq_no_+length_; i++) {
		newSBN[i%sz] = SBN[i%sbsize_];
	}

	delete[] SBN;
	SBN = newSBN;
	sbsize_ = sz;
}

void ScoreBoard::Dump()
{
       int i;

       printf("SB len: %d  ", length_);
       if (length_) {
               for (i=SBN[(first_)%sbsize_].seq_no_; 
                    i<SBN[(first_)%sbsize_].seq_no_+length_; i++) {
                       printf("seq: %d  [ ", i);
                       if(SBNI.ack_flag_)
                               printf("A");
                       if(SBNI.sack_flag_)
                               printf("S");
                       if(SBNI.retran_)
                               printf("R");
                       printf(" ]");
               }
       }
       printf("\n");
}

