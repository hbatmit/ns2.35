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
 * @(#) $Header: /cvsroot/nsnam/ns-2/tcp/scoreboard-rh.h,v 1.2 2000/08/12 21:46:10 sfloyd Exp $
 */

#ifndef ns_scoreboard_h
#define ns_scoreboard_h

//  Definition of the scoreboard class:

#define SBSIZE 1024

#include "tcp.h"

class ScoreBoardRH {
  public:
        ScoreBoardRH(int *numdupacks) : numdupacks_(numdupacks) {first_=0; length_=0; retran_occured_=0;}
	int IsEmpty () {return (length_ == 0);}
	void ClearScoreBoard (); 
	int GetNextRetran ();
	void MarkRetran (int retran_seqno, int snd_nxt, int rh_id);
	int UpdateScoreBoard (int last_ack_, hdr_tcp*, int rh_id);
	int CheckSndNxt (int sack_max);
	int GetFack (int last_ack);
	int GetNewHoles ();
	void TimeoutScoreBoard (int snd_nxt);
	int FakeSack (int last_ack, int num_dupacks);
	int RetranSacked (int rh_id);
	int RetranOccurred (int rh_id) {return (retran_occured_ == rh_id);}
	
  protected:
	int first_, length_;
	int retran_occured_;             /* This variable stores the last adjustment interval
					    in which a retransmission occured  */
	int retran_sacked_;              /* This variable stores the last adjustment interval
					    in which a retransmission was sacked  */
        int *numdupacks_;		 /* I know, it stinks...  But this is numdupacks_ from
					  * our parent TCP stack. */
   
	struct ScoreBoardNode {
		int seq_no_;		/* Packet number */
		int ack_flag_;		/* Acked by cumulative ACK */
		int sack_flag_;		/* Acked by SACK block */
		int retran_;		/* Packet retransmitted */
		int snd_nxt_;		/* snd_nxt at time of retransmission */
		int sack_cnt_;		/* number of reports for this hole */
		int rh_id_;             /* The id of the rate-halving adjustment interval */
	} SBN[SBSIZE+1];
};

#endif
