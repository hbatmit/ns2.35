/*
 * Copyright (c) 2002 Tom Kelly, University of Cambridge
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

#ifndef ns_scoreboard_rq_h
#define ns_scoreboard_rq_h

// Definition of the scoreboardRQ class 
// - a shim over the FullTCP reassembly queue code

#include "scoreboard.h"
#include "rq.h"

class ScoreBoardRQ : public ScoreBoard {
public:
	ScoreBoardRQ(): ScoreBoard(NULL, 0), h_seqno_(-1),sack_min(-1), rq_(sack_min){};
	virtual ~ScoreBoardRQ(){delete[] SBN;}
	virtual int IsEmpty ();
	virtual void ClearScoreBoard () {rq_.clear(); h_seqno_ = -1;}; 
	virtual int GetNextRetran ();
	virtual void MarkRetran (int retran_seqno){
		if (retran_seqno >= h_seqno_) 
			h_seqno_ = retran_seqno+1;
	};
	virtual void MarkRetran (int retran_seqno, int snd_nxt);
	virtual int UpdateScoreBoard (int last_ack_, hdr_tcp*);
	virtual int CheckUpdate() {return (changed_);}
	virtual int CheckSndNxt (hdr_tcp*);
	virtual int GetNextUnacked (int seqno);
	virtual void Dump();
protected:	
	int h_seqno_;
	int sack_min;	// highest cumulative ack seen so far, plus one
	
	ReassemblyQueue rq_;	
};

#endif
