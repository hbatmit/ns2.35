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

/* 8/02 Tom Kelly - Made scoreboard a general interface to allow
 *                  easy swapping of scoreboard algorithms.  
 *                - Allow scoreboard buffer size to grow dynamically
 *                - Made ScoreBoardNode a more compact data structure
 */

#ifndef ns_scoreboard_h
#define ns_scoreboard_h

//  Definition of the scoreboard class:

#include "tcp.h"

class ScoreBoardNode {
public:
	int seq_no_;		/* Packet number */
	char ack_flag_;         /* Acked by cumulative ACK */
	char sack_flag_;        /* Acked by SACK block */
	char retran_;           /* Packet retransmitted */
	int snd_nxt_;		/* snd_nxt at time of retransmission */
};

class ScoreBoard {
  public:
	ScoreBoard(ScoreBoardNode* sbn, int sz): first_(0), length_(0), sbsize_(sz), changed_(0),SBN(sbn) {}
	virtual ~ScoreBoard(){if(SBN) delete[] SBN;}
	virtual int IsEmpty () {return (length_ == 0);}
	virtual void ClearScoreBoard (); 
	virtual int GetNextRetran ();
	virtual void Dump();
	virtual void MarkRetran (int retran_seqno);
	virtual void MarkRetran (int retran_seqno, int snd_nxt);
	virtual int UpdateScoreBoard (int last_ack_, hdr_tcp*);
	virtual int CheckUpdate() {return (changed_);}
	virtual int CheckSndNxt (hdr_tcp*);
	virtual int GetNextUnacked (int seqno);
        inline int IsChanged() { return changed_; }
	
  protected:
	int first_, length_, sbsize_, changed_;
	ScoreBoardNode * SBN; 
	void resizeSB(int sz);
};

#endif
