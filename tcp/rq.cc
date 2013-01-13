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

#include "rq.h"

ReassemblyQueue::seginfo* ReassemblyQueue::freelist_ = NULL;

ReassemblyQueue::seginfo* ReassemblyQueue::newseginfo()
{
	seginfo *s;
	
	if( (s = freelist_) ){
		freelist_ = s->next_;
		return s;
	}else{
		return new seginfo;
	}
}

void ReassemblyQueue::deleteseginfo(ReassemblyQueue::seginfo* s)
{
	s->next_ = freelist_;
	freelist_ = s;
}

/*
 * unlink a seginfo from its FIFO
 */
void
ReassemblyQueue::fremove(seginfo* p)
{
	if (hint_ == p)
		hint_ = NULL;

	if (p->prev_)
		p->prev_->next_ = p->next_;
	else
		head_ = p->next_;
	if (p->next_)
		p->next_->prev_ = p->prev_;
	else
		tail_ = p->prev_;
}

/*
 * unlink a seginfo from its LIFO
 */
void
ReassemblyQueue::sremove(seginfo* p)
{
	if (hint_ == p)
		hint_ = NULL;

	if (p->sprev_)
		p->sprev_->snext_ = p->snext_;
	else
		top_ = p->snext_;
	if (p->snext_)
		p->snext_->sprev_ = p->sprev_;
	else
		bottom_ = p->sprev_;

}

/*
 * push a seginfo on the LIFO
 */
void
ReassemblyQueue::push(seginfo *p)
{
	p->snext_ = top_;
	p->sprev_ = NULL;
	top_ = p;
	if (p->snext_)
		p->snext_->sprev_ = p;
	else
		bottom_ = p;
}

/*
 * counts: return the # of blks and byte counts in
 * them starting at the given node
 */
void
ReassemblyQueue::cnts(seginfo *p, int& blkcnt, int& bytecnt)
{
	int blks = 0;
	int bytes = 0;

	while (p != NULL) {
		++blks;
		bytes += (p->endseq_ - p->startseq_);
		p = p->next_;
	}
	blkcnt = blks;
	bytecnt = bytes;
	return;
}


/*
 * clear out reassembly queue and stack
 */

void
ReassemblyQueue::clear()
{
	// clear stack and end of queue
	tail_ = top_ = bottom_ = hint_ = NULL;

	seginfo *p = head_;
	while (head_) {
		p = head_;
		head_= head_->next_;
		ReassemblyQueue::deleteseginfo(p);
	}
	tail_ = NULL;
	total_ = 0;
	return;
}

/*
 * clear out reassembly queue (and stack) up
 * to the given sequence number
 */

TcpFlag
ReassemblyQueue::clearto(TcpSeq seq)
{
	TcpFlag flag = 0;
	seginfo *p = head_, *q;
	while (p) {
		if (p->endseq_ <= seq) {
			q = p->next_;
			flag |= p->pflags_;
			total_ -= (p->endseq_ - p->startseq_);
			sremove(p);
			fremove(p);
			ReassemblyQueue::deleteseginfo(p);
			p = q;
		} else
			break;
	}
	/* we might be trimming in the middle */
	if (p && p->startseq_ <= seq && p->endseq_ > seq) {
		total_ -= (seq - p->startseq_);
		p->startseq_ = seq;
		flag |= p->pflags_;
	}
	return flag;
}

/*
 * gensack() -- generate 'maxsblock' sack blocks (start/end seq pairs)
 * at specified address
 * returns the number of blocks written into the buffer specified
 *
 * According to RFC2018, a sack block contains:
 *	left edge of block (first seq # of the block)
 *	right edge of block (seq# immed. following last seq# of the block)
 */

int
ReassemblyQueue::gensack(int *sacks, int maxsblock)
{
	seginfo *p = top_;
	int cnt = maxsblock;

	while (p && maxsblock) {
		*sacks++ = p->startseq_;
		*sacks++ = p->endseq_;
		--maxsblock;
		p = p->snext_;
	}
	return (cnt - maxsblock);
}

/*
 * dumplist -- print out FIFO and LIFO (for debugging)
 */

void
ReassemblyQueue::dumplist()
{

	printf("FIFO [size:%d]: ", total_);
	if (head_ == NULL) {
		printf("NULL\n");
	} else {
		register seginfo* p = head_;
		while (p != NULL) {
			if (p->rqflags_ & RQF_MARK) {
				printf("OOPS: LOOP1\n");
				abort();
			}
			printf("[->%d, %d<-<f:0x%x,c:%d>]",
				p->startseq_, p->endseq_, p->pflags_, p->cnt_);
			p->rqflags_ |= RQF_MARK;
			p = p->next_;
		}
		printf("\n");
		p = tail_;
		while (p != NULL) {
			printf("[->%d, %d<-]",
				p->startseq_, p->endseq_);
			p = p->prev_;
		}
		printf("\n");
	}

	printf("LIFO: ");
	if (top_ == NULL) {
		printf("NULL\n");
	} else {
		register seginfo* s = top_;
		while (s != NULL) {
			if (s->rqflags_ & RQF_MARK)
				s->rqflags_ &= ~RQF_MARK;
			else {
				printf("OOPS: LOOP2\n");
				abort();
			}
			printf("[->%d, %d<-]",
				s->startseq_, s->endseq_);
			s = s->snext_;
		}
		printf("\n");
		s = bottom_;
		while (s != NULL) {
			printf("[->%d, %d<-]",
				s->startseq_, s->endseq_);
			s = s->sprev_;
		}
		printf("\n");
	}
	printf("RCVNXT: %d\n", rcv_nxt_);
	printf("\n");
	fflush(stdout);
}

/*
 *
 * add() -- add a segment to the reassembly queue
 *	this is where the real action is...
 *	add the segment to both the LIFO and FIFO
 *
 * returns the aggregate header flags covering the block
 * just inserted (for historical reasons)
 *
 * add start/end seq to reassembly queue
 * start specifies starting seq# for segment, end specifies
 * last seq# number in the segment plus one
 */

TcpFlag
ReassemblyQueue::add(TcpSeq start, TcpSeq end, TcpFlag tiflags, RqFlag rqflags)
{

	int needmerge = FALSE;
	int altered = FALSE;
	int initcnt = 1;	// initial value of cnt_ for new blk

	if (end < start) {
		fprintf(stderr, "ReassemblyQueue::add() - end(%d) before start(%d)\n",
			end, start);
		abort();
	}

	if (head_ == NULL) {
		if (top_ != NULL) {
			fprintf(stderr, "ReassemblyQueue::add() - problem: FIFO empty, LIFO not\n");
			abort();
		}

		// nobody there, just insert this one


		tail_ = head_ = top_ = bottom_ =  ReassemblyQueue::newseginfo();
		head_->prev_ = head_->next_ = head_->snext_ = head_->sprev_ = NULL;
		head_->startseq_ = start;
		head_->endseq_ = end;
		head_->pflags_ = tiflags;
		head_->rqflags_ = rqflags;
		head_->cnt_ = initcnt;

		total_ = (end - start);

		//
		// this shouldn't really happen, but
		// do the right thing just in case
		if (rcv_nxt_ >= start)
			rcv_nxt_ = end;

		return (tiflags);
	} else {

again2:
		seginfo *p = NULL, *q = NULL, *n, *r;

		//
		// in the code below, arrange for:
		// q: points to segment after this one
		// p: points to segment before this one
		//

		if (start >= tail_->endseq_) {
			// at tail, no overlap
			p = tail_;
			if (start == tail_->endseq_)
				needmerge = TRUE;
			goto endfast;
		}

		if (end <= head_->startseq_) {
			// at head, no overlap
			q = head_;
			if (end == head_->startseq_)
				needmerge = TRUE;
			goto endfast;
		}

		//
		// search for segments before and after
		// the new one; could be overlapped
		//
		q = head_;
		while (q && q->startseq_ < end)
			q = q->next_;

		p = tail_;
		while (p && p->endseq_ > start)
			p = p->prev_;

#ifdef notdef
printf("Thinking of merging (s:%d, e:%d), p:%p (%d,%d), q:%p (%d,%d) into: \n",
start, end, p, q,
	p ? p->startseq_ : 0,
	p ? p->endseq_ : 0,
	q ? n->startseq_ : 0,
	q ? n->endseq_ : 0);
#endif


		//
		// kill anything that is completely overlapped
		//
		r = p ? p : head_;
		while (r && r != q)  {
			if (start == r->startseq_ && end == r->endseq_) {
				// exact overlap
				r->pflags_ |= tiflags;
				if (RQC_CNTDUPS == TRUE)
					r->cnt_++;
				return (r->pflags_);
			} else if (start <= r->startseq_ && end >= r->endseq_) {
				// complete overlap, not exact
				total_ -= (r->endseq_ - r->startseq_);
				n = r;
				r = r->next_;
				tiflags |= n->pflags_;
				initcnt += n->cnt_;
				fremove(n);
				sremove(n);
				ReassemblyQueue::deleteseginfo(n);
				altered = TRUE;
			} else
				r = r->next_;
		}


		//
		// if we completely overlapped everything, the list
		// will now be empty.  In this case, just add the new one
		///

		if (empty())
			goto endfast;

		if (altered) {
			altered = FALSE;
			goto again2;
		}

		// look for left-side merge
		// update existing seg's start seq with new start
		if (p == NULL || p->next_->startseq_ < start) {
			if (p == NULL)
				p = head_;
			else
				p = p->next_;
				
			if (start < p->startseq_) {
				total_ += (p->startseq_ - start);
				p->startseq_ = start;
			}
			start = p->endseq_;
			needmerge = TRUE;
			p->pflags_ |= tiflags;
			p->cnt_++;
			--initcnt;
		}

		// look for right-side merge
		// update existing seg's end seq with new end
		if (q == NULL || q->prev_->endseq_ > end) {
			if (q == NULL)
				q = tail_;
			else
				q = q->prev_;

			if (end > q->endseq_) {
				total_ += (end - q->endseq_);
				q->endseq_ = end;
			}
			end = q->startseq_;
			needmerge = TRUE;
			q->pflags_ |= tiflags;
			if (!needmerge) {
				// if needmerge is TRUE, that can
				// only be the case if we did a left-side
				// merge (above), which has already taken
				// accounting of the new segment
				q->cnt_++;
				--initcnt;
			}
		}


		if (end <= start) {
			if (rcv_nxt_ >= head_->startseq_)
				rcv_nxt_ = head_->endseq_;
			return (tiflags);
		}
			

		//
		// if p & q are adjacent and new one
		// fits between, that is an easy case
		//

		if (!needmerge && p->next_ == q && p->endseq_ <= start && q->startseq_ >= end) {
			if (p->endseq_ == start || q->startseq_ == end)
				needmerge = TRUE;
		}

endfast:
		n = ReassemblyQueue::newseginfo();
		n->startseq_ = start;
		n->endseq_ = end;
		n->pflags_ = tiflags;
		n->rqflags_ = rqflags;
		n->cnt_=  initcnt;

		n->prev_ = p;
		n->next_ = q;

		push(n);

		if (p)
			p->next_ = n;
		else
			head_ = n;

		if (q)
			q->prev_ = n;
		else
			tail_ = n;


		//
		// If there is an adjacency condition,
		// call coalesce to deal with it.
		// If not, there is a chance we inserted
		// at the head at the rcv_nxt_ point.  In
		// this case we ned to update rcv_nxt_ to
		// the end of the newly-inserted segment
		//

		total_ += (end - start);

		if (needmerge)
			return(coalesce(p, n, q));
		else if (rcv_nxt_ >= start) {
			rcv_nxt_ = end;
		}

		return tiflags;
	}
}
/*
 * We need to see if we can coalesce together the
 * blocks in and around the new block
 *
 * Assumes p is prev, n is new, p is after
 */
TcpFlag
ReassemblyQueue::coalesce(seginfo *p, seginfo *n, seginfo *q)
{
	TcpFlag flags = 0;

#ifdef RQDEBUG
printf("coalesce(%p,%p,%p)\n", p, n, q);
printf(" [(%d,%d),%d],[(%d,%d),%d],[(%d,%d),%d]\n",
	p ? p->startseq_ : 0,
	p ? p->endseq_ : 0,
	p ? p->cnt_ : -1000,
	n ? n->startseq_ : 0,
	n ? n->endseq_ : 0,
	n ? n->cnt_ : -1000,
	q ? n->startseq_ : 0,
	q ? n->endseq_ : 0,
	q ? n->cnt_ : -1000);
dumplist();
#endif

	if (p && q && p->endseq_ == n->startseq_ && n->endseq_ == q->startseq_) {
		// new block fills hole between p and q
		// delete the new block and the block after, update p
		sremove(n);
		fremove(n);
		sremove(q);
		fremove(q);
		p->endseq_ = q->endseq_;
		p->cnt_ += (n->cnt_ + q->cnt_);
		flags = (p->pflags_ |= n->pflags_);
		ReassemblyQueue::deleteseginfo(n);
		ReassemblyQueue::deleteseginfo(q);
	} else if (p && (p->endseq_ == n->startseq_)) {
		// new block joins p, but not q
		// update p with n's seq data, delete new block
		sremove(n);
		fremove(n);
		p->endseq_ = n->endseq_;
		flags = (p->pflags_ |= n->pflags_);
		p->cnt_ += n->cnt_;
		ReassemblyQueue::deleteseginfo(n);
	} else if (q && (n->endseq_ == q->startseq_)) {
		// new block joins q, but not p
		// update q with n's seq data, delete new block
		sremove(n);
		fremove(n);
		q->startseq_ = n->startseq_;
		flags = (q->pflags_ |= n->pflags_);
		q->cnt_ += n->cnt_;
		ReassemblyQueue::deleteseginfo(n);
		p = q;	// ensure p points to something
	}

	//
	// at this point, p points to the updated/coalesced
	// block.  If it advances the highest in-seq value,
	// update rcv_nxt_ appropriately
	//
	if (rcv_nxt_ >= p->startseq_)
		rcv_nxt_ = p->endseq_;
	return (flags);
}

/*
 * look for the next hole, starting with the given
 * sequence number.  If this seq number is contained in
 * a SACK block we have, return the ending sequence number
 * of the block.  Also, fill in the nxtcnt and nxtbytes fields
 * with the number and sum total size of the sack regions above
 * the block.
 */
int
ReassemblyQueue::nexthole(TcpSeq seq, int& nxtcnt, int& nxtbytes)
{

	nxtbytes = nxtcnt = -1;
	hint_ = head_;

	seginfo* p;
	for (p = hint_; p; p = p->next_) {
		// seq# is prior to SACK region
		// so seq# is a legit hole
		if (p->startseq_ > seq) {
			cnts(p, nxtcnt, nxtbytes);
			return (seq);
		}

		// seq# is covered by SACK region
		// so the hole is at the end of the region
		if ((p->startseq_ <= seq) && (p->endseq_ >= seq)) {
			if (p->next_) {
				cnts(p->next_, nxtcnt, nxtbytes);
			}
			return (p->endseq_);
		}
	}
	return (-1);
}


#ifdef RQDEBUG
main()
{
	int rcvnxt = 1;
	ReassemblyQueue rq(rcvnxt);

	static int blockstore[64];
	int *blocks = blockstore;
	int nblocks = 5;
	int i;

	printf("Simple---\n");
	rq.add(2, 4, 0, 0);
	rq.add(6, 8, 0, 0);	// disc
	printf("D1\n");
	rq.dumplist();	// [(2,4),1], [(6,8),1]

	rq.add(1,2, 0, 0);	// l merge
	printf("D2\n");
	rq.dumplist();	// [(1,4),2], [(6,8),1]

	rq.add(8, 10, 0, 00);	// r merge
	printf("D3\n");
	rq.dumplist();	// [(1,4),2], [(6, 10),2]

	rq.add(4, 6, 0, 0);	// m merge
	printf("Simple output:\n");
	rq.dumplist();	// [(1, 10),5]

	printf("X0:\n");
	rq.init(1);
	rq.add(5,10, 0, 0);
	rq.add(11,20, 0, 0);
	rq.add(5,10, 0, 0);	// dup left
	rq.dumplist();	// [(5,10),1], [(11,20),1]

	printf("X1:\n");
	rq.init(1);
	rq.add(5,10, 0, 0);
	rq.add(11,20, 0, 0);
	rq.add(11,20, 0, 0);	// dup rt
	rq.dumplist();	// [(5,10),1], [(11,20),1]

	printf("X2:\n");
	rq.init(1);
	rq.add(5,10, 0, 0);
	rq.add(11,20, 0, 0);
	rq.add(30, 40, 0, 0);
	rq.add(11,20, 0, 0);	// dup mid
	rq.dumplist();	// [(5,10),1], [(11,20),1], [(30,40),1]

	printf("X3\n");
	rq.add(30,50,0,0);	// dup rt
	rq.dumplist();	// [(5,10),1], [(11,20),1], [(30,50),2]

	printf("X4\n");
	rq.add(1,10,0,0);	// dup lt
	rq.dumplist();	// [(1,10),2], [(11,20),1], [(30,50),2]

	printf("C1:\n");
	rq.init(1);
	rq.add(2, 4, 0, 0);
	rq.add(1, 4, 0, 0);	// l overlap full
	rq.dumplist();	// [(1,4),2]

	printf("C2:\n");
	rq.init(1);
	rq.add(2, 4, 0, 0);
	rq.add(1, 3, 0, 0);	// l overlap part
	rq.dumplist();	// [(1,4),2]

	printf("C3:\n");
	rq.init(1);
	rq.add(2, 4, 0, 0);
	rq.add(2, 7, 0, 0);	// r overlap full
	rq.dumplist();	// [(2,7),2]

	printf("C4:\n");
	rq.init(1);
	rq.add(2, 4, 0, 0);
	rq.add(3, 7, 0, 0);	// r overlap part
	rq.dumplist();	// [(2,7),2]

	printf("C5:\n");
	rq.init(1);
	rq.add(2, 4, 0, 0);
	rq.add(6, 8, 0, 0);
	rq.add(1, 9, 0, 0);	// double olap - ends
	rq.dumplist();	// [(1,9),3]

	printf("C6:\n");
	rq.init(1);
	rq.add(2, 4, 0, 0);
	rq.add(6, 8, 0, 0);
	rq.add(15, 20, 0, 0);
	rq.dumplist();	// [(2,4),1], [(6,8),1], [(15,20),1]
	rq.add(5, 9, 0, 0);	// overlap middle
	rq.dumplist();	// [(2,4),1], [(5,9),2], [(15,20),1]

	printf("C7:\n");
	rq.init(1);
	rq.add(1, 2, 0, 0);
	rq.add(3, 5, 0, 0);
	rq.add(6, 8, 0, 0);
	rq.add(9, 10, 0, 0);
	rq.dumplist();	// [(1,2),1],[(3,5),1],[(6,8),1],[(9,10),1]
	rq.add(4, 7, 0, 0);	// double olap middle
	rq.dumplist();	// [(1,2),1], [(3,8),3], [(9,10),1]

	printf("C8:\n");
	rq.init(1);
	rq.add(1, 2, 0, 0);
	rq.add(3, 5, 0, 0);
	rq.add(10, 12, 0, 0);
	rq.add(20, 30, 0, 0);
	rq.dumplist();	// [(1,2),1], [(3,5),1], [(10,12),1], [(20,30),1]
	rq.add(4, 8, 0, 0);	// single olap middle
	rq.dumplist();	// [(1,2),1], [(3,8),2], [(10,12),1], [(20,30),1]

	rq.init(1);
	rq.add(1, 5, 0, 0);
	rq.add(10, 20, 0, 0);
	//rq.add(40321, 41281, 0, 0);
	//rq.add(42241, 43201, 0, 0);
	//rq.add(44161, 45121, 0, 0);
	rq.dumplist();	// [(1,5),1], [(10,20),1]
	//rq.add(40321, 41281, 0, 0);
	rq.add(1, 5, 0, 0);
	rq.dumplist();	// [(1,5),1], [(10,20),1]

	int x, y;
	printf("NH1: %d\n", rq.nexthole(3, x, y));
	printf("NH2: %d\n", rq.nexthole(5, x, y));
	printf("CLR to 4\n");
	rq.clearto(4);
	rq.dumplist();

	exit(0);
}
#endif

