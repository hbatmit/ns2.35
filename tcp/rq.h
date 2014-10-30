/*
 * Copyright (c) Intel Corporation 2001. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 *
 */

#ifndef ns_rq_h
#define ns_rq_h

#define	RQF_MARK	1	// for debugging

typedef int	TcpSeq;		// a TCP sequence number
typedef	int	TcpFlag;	// holds flags from TCP hdr
typedef int	RqFlag;		// meta data (owned by ReassemblyQueue)

#ifndef TRUE
#define	TRUE	1
#endif

#ifndef FALSE
#define	FALSE	0
#endif

#define	RQC_CNTDUPS	FALSE	// count exact dups on add()?

#include <stdio.h>
#include <stdlib.h>

/*
 * ReassemblyQueue: keeps both a stack and linked list of segments
 *	FIFO maintains list in sequence # order (very often a FIFO)
 *	LIFO maintains list in insert order (used for generation of (D)SACKS
 *
 * Note that this code attempts to be largely independent of all
 * other code (no include files from the rest of the simulator)
 *
 * July, 2001
 * kfall@intel.com
 */

/* 01/03 Tom Kelly <ctk21@cam.ac.uk>
 * 
 * Made the newseginfo and deleteseginfo calls to avoid new/delete 
 * overhead in generating SACK blocks good for HSTCP; see scoreboard-rq
 */ 

class ReassemblyQueue {
	struct seginfo {
		seginfo* next_;	// next on FIFO list
		seginfo* prev_;	// prev on FIFO list
		seginfo* snext_;	// next on LIFO list
		seginfo* sprev_;	// prev on LIFO list

		TcpSeq	startseq_;	// starting seq
		TcpSeq	endseq_;	// ending seq + 1
		TcpFlag	pflags_;	// flags derived from tcp hdr
		RqFlag	rqflags_;	// book-keeping flags
		int	cnt_;		// refs to this block
	};

public:
	ReassemblyQueue(TcpSeq& rcvnxt) :
		head_(NULL), tail_(NULL), top_(NULL), bottom_(NULL), hint_(NULL), total_(0), rcv_nxt_(rcvnxt) { };
	int empty() { return (head_ == NULL); }
	int add(TcpSeq sseq, TcpSeq eseq, TcpFlag pflags, RqFlag rqflags = 0);
	int maxseq() { return (tail_ ? (tail_->endseq_) : -1); }
	int minseq() { return (head_ ? (head_->startseq_) : -1); }
	int total() { return total_; }
	int nexthole(TcpSeq seq, int&, int&);	// find next hole above seq, also
						// include cnt of following blk

	int gensack(int *sacks, int maxsblock);

	void clear();		// clear FIFO, LIFO
	void init(TcpSeq rcvnxt) { rcv_nxt_ = rcvnxt; clear(); }
	TcpFlag clearto(TcpSeq);	// clear FIFO, LIFO up to seq #
	TcpFlag cleartonxt() { 		// clear FIFO, LIFO to rcv_nxt_
	    return (clearto(rcv_nxt_));
	}
	void dumplist();	// for debugging

	// cache of allocated seginfo blocks
	static seginfo* newseginfo();
	static void deleteseginfo(seginfo*);	
	
protected:
	static seginfo* freelist_; // cache of free seginfo blocks
	
	seginfo* head_;		// head of segs linked list
	seginfo* tail_;		// end of segs linked list

	seginfo* top_;		// top of stack
	seginfo* bottom_;	// bottom of stack
	seginfo* hint_;	// hint for nexthole() function
	int total_;	// # bytes in Reassembly Queue

	// rcv_nxt_ is a reference to an externally allocated TcpSeq
	// (aka integer)that will be updated with the highest in-sequence sequence
	// number added [plus 1] by the user.  This is the value ordinarily used
	// within TCP to set rcv_nxt and thus to set the ACK field.  It is also
	// used in the SACK sender as sack_min_

	TcpSeq& rcv_nxt_;	// start seq of next expected thing
	TcpFlag coalesce(seginfo*, seginfo*, seginfo*);
	void fremove(seginfo*);	// remove from FIFO
	void sremove(seginfo*); // remove from LIFO
	void push(seginfo*); // add to LIFO
	void cnts(seginfo *, int&, int&); // byte/blk counts
};

#endif
