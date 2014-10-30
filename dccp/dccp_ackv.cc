/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* Copyright (c) 2004  Nils-Erik Mattsson 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: dccp_ackv.cc,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#include <stdio.h>
#include "dccp_ackv.h"

/* Static string representation of packet states */
char* DCCPAckVector::packet_state_str_[DCCP_NUM_PACKET_STATES] =
{"Received", "ECN-marked", "Reserved", "Not Received" };

//private methods

/* Add items to the head of the vector
 * arg: state  - packet state to set on items
 *      runlen - run length to set on items
 *      count  - number of similar items to add
 * ret: true if successful
 *      false if more space is needed and doubleMaxSize() fails.
 */
bool DCCPAckVector::addItems(dccp_packet_state state, u_int8_t runlen, int count){
	u_int8_t temp = state;	
	int new_head, walker;
	u_int8_t item = (temp << DCCP_RUN_LENGTH_BITS | (runlen & DCCP_RUN_LENGTH_MASK));
	if (size_+count > max_size_){
		//increase size and try again
		return (doubleMaxSize() && addItems(state,runlen,count));
	}

	if(size_ != 0)
		seq_head_ +=(runlen+1)*count;
	
	new_head = (head_ - count + max_size_) % max_size_;
	walker = new_head;
	
	while(walker != head_){
		vector_[walker] = item;
		walker = (walker + 1) % max_size_;
	}
	
	size_ += count;
	head_ = new_head;

	return true;
}

/* Find where in the vector a info about a seqnum exists
 * arg: seqnum    - sequence number to find
 *      where     - resulting position in the vector
 *      seq_start - starting sequence number corresponding to where
 *      seq_end   - ending sequence number corresponding to where
 * ret: true if found
 *      false otherwise (where, seq_start, seq_end invalid)
 */
bool DCCPAckVector::findSeqNum(u_int32_t seqnum, u_int16_t *where, u_int32_t *seq_start, u_int32_t *seq_end){
	if (size_ == 0 || seqnum > seq_head_ || seqnum < seq_tail_ )
		return false;
	
	*where = head_;
	*seq_start = seq_head_;
	*seq_end = 0;
	int stop = (tail_ + 1) % max_size_;
	
	do{
		*seq_end = *seq_start - (vector_[*where] & DCCP_RUN_LENGTH_MASK);
		
		if (*seq_end <= seqnum && *seq_start >= seqnum){
			return true;
		}
		
		*where = (*where + 1) % max_size_;
		*seq_start = *seq_end - 1;
	} while(*where != stop);
	
	//should never get here!!
	return false;
}

/* Increase the maximum size of the vector by a factor two
 * until DCCP_MAX_ACKV_SIZE is reached.
 * ret:  true if successful, false if already at DCCP_MAX_ACKV_SIZE
 */
bool DCCPAckVector::doubleMaxSize(){
	if (max_size_ == DCCP_MAX_ACKV_SIZE)
		return false;
    
	int new_max_size = max_size_ * 2;

	if (new_max_size > DCCP_MAX_ACKV_SIZE)
		new_max_size = DCCP_MAX_ACKV_SIZE;

	u_char *new_vector = new u_char[new_max_size];
	int walker = head_;

	for (int i = 0; i < size_; i++){
		new_vector[i] = vector_[walker];
		walker = (walker + 1) % max_size_;
	}
	max_size_ = new_max_size;

	if (size_ > 0) {
		head_ = 0;
		tail_ = size_-1;
	}
	
	delete [] vector_;
	vector_ = new_vector;
	return true;
}

/* Add an acknowledgment to ack history
 * arg: seqnum - sequence number of the sent ack packet
 */
void DCCPAckVector::addToAckHistory(u_int32_t seqnum){
	struct dccp_ackv_hist_entry *elm = STAILQ_FIRST(&ack_hist_);
	if(elm != NULL && elm->seq_sent_ >= seqnum){
		fprintf(stderr, "DCCPAckVector::addToAckHistory - Sequence number to send is old! (seqnum %d < seq_sent_ %d)\n",seqnum, elm->seq_sent_);
		fflush(stdout);
		abort();
	} else if (elm != NULL && elm->ack_num_ > seq_head_) {
		fprintf(stderr, "DCCPAckVector::addToAckHistory - Last Ack number %d is larger than seq_head_ %d\n", elm->ack_num_, seq_head_);
		fflush(stdout);
		abort();
	} else {
		/*if (elm != NULL && elm->ack_num_ == seq_head_) {
			fprintf(stderr, "DCCPAckVector::addToAckHistory - Last Ack number %d is equal to seq_head_ %d\n", elm->ack_num_, seq_head_);
			fflush(stdout);
			abort();
			}*/
		struct dccp_ackv_hist_entry *new_entry = new struct dccp_ackv_hist_entry;
		new_entry->seq_sent_ = seqnum;
		new_entry->ack_num_ = seq_head_;
		new_entry->ene_ = ene_;
		STAILQ_INSERT_HEAD(&ack_hist_, new_entry, linfo_);
	}
}

/* Remove all items in the ack history */
void DCCPAckVector::clearAckHistory(){
	struct dccp_ackv_hist_entry *elm, *elm2;

	/* Empty packet history */
	elm = STAILQ_FIRST(&ack_hist_);
	while (elm != NULL) {
		elm2 = STAILQ_NEXT(elm, linfo_);
		delete elm;
		elm = elm2;
	}
	
	STAILQ_INIT(&ack_hist_);
}

//public methods

/* Return a string reprenting a packet state
 * arg: state - packet state
 * ret: a string representation of state
 */
const char* DCCPAckVector::packetStateAsStr(dccp_packet_state state){
        if (state < DCCP_NUM_PACKET_STATES && state >= 0)
		return packet_state_str_[state];
	else
		return "Unknown";
}

/* Constructor
 * arg: initialSize - initial size for the ack vector
 * ret: a new DCCPAckVector
 */
DCCPAckVector::DCCPAckVector(u_int16_t initialSize){
	vector_ = new u_char[initialSize];
	size_ = 0;
	max_size_ = initialSize;
	ene_ = 0;
	head_ = 0;
	tail_ = 0;
	seq_head_ = 0;
	seq_tail_ = 0;
	p_walk_ = 0;
	seq_p_walk_ = 0;
	delta_p_walk_ = 0;
	p_walking_ = false;
	p_reverse_ = false;
	i_walk_ = 0;
	seq_i_walk_ = 0;
	i_walking_ = false;

	ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_RECV]         = DCCP_PACKET_RECV;
	ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_ECN]          = DCCP_PACKET_RECV;
	ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_RESERVED]     = DCCP_PACKET_RESERVED;
	ackv_cons_[DCCP_PACKET_RECV][DCCP_PACKET_NOT_RECV]     = DCCP_PACKET_RECV;
	ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_RECV]          = DCCP_PACKET_ECN;
	ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_ECN]           = DCCP_PACKET_ECN;
	ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_RESERVED]      = DCCP_PACKET_RESERVED;
	ackv_cons_[DCCP_PACKET_ECN][DCCP_PACKET_NOT_RECV]      = DCCP_PACKET_ECN;
	ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_RECV]     = DCCP_PACKET_RESERVED;
	ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_ECN]      = DCCP_PACKET_RESERVED;
	ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_RESERVED] = DCCP_PACKET_RESERVED;
	ackv_cons_[DCCP_PACKET_RESERVED][DCCP_PACKET_NOT_RECV] = DCCP_PACKET_RESERVED;
	ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_RECV]     = DCCP_PACKET_RECV;
	ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_ECN]      = DCCP_PACKET_ECN;
	ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_RESERVED] = DCCP_PACKET_RESERVED;
	ackv_cons_[DCCP_PACKET_NOT_RECV][DCCP_PACKET_NOT_RECV] = DCCP_PACKET_NOT_RECV;
	
	STAILQ_INIT(&ack_hist_);
}

/* Destructor */
DCCPAckVector::~DCCPAckVector(){
	clearAckHistory();
	delete [] vector_;
}

/* Add a packet to the ack vector (using ackv_cons_ when needed)
 * Will fill with NOT_RECV packets when packets are missing
 * arg: seqnum - sequence number of packet
 *      state  - packet state
 * ret: true if successful
 *      false if more space is needed and doubleMaxSize() fails
 *            or if packet is older than the oldest in the vector
 */
bool DCCPAckVector::addPacket(u_int32_t seqnum, dccp_packet_state state){
	int dist = seqnum - seq_head_;
	u_int8_t runlen = 0;
	
	if (size_ == 0){
		seq_head_ = seqnum;
		seq_tail_ = seqnum;
		head_ = 0;
		tail_ = max_size_-1;
	} else if (dist > 1){  //some packets are missing
		int num_fulls = (dist-2) / (DCCP_RUN_LENGTH_MASK+1);
		int last_runlen = (dist-2) % (DCCP_RUN_LENGTH_MASK+1);
	
		if (size_ + num_fulls + 1 + 1 > max_size_)
			return doubleMaxSize() && addPacket(seqnum, state);

		if (num_fulls > 0)
			addItems(DCCP_PACKET_NOT_RECV, DCCP_RUN_LENGTH_MASK, num_fulls);

		addItems(DCCP_PACKET_NOT_RECV,last_runlen);

	} else if (dist == 1) { //expected packet
		u_int8_t last_runlen = vector_[head_] & DCCP_RUN_LENGTH_MASK;
		u_int8_t last_state = vector_[head_] >> DCCP_RUN_LENGTH_BITS;
		if (last_runlen < DCCP_RUN_LENGTH_MASK && last_state == (u_int8_t) state){
			vector_[head_] += 1;
			seq_head_ = seqnum;
			return true;
		}

	} else {
		dccp_packet_state curr_state;
		if (getState(seqnum, &curr_state))
			if (alterState(seqnum,ackv_cons_[curr_state][state])){
				return true;
			}
		
		return false;
	}

	if (!addItems(state, runlen)){
		return false;
	}

	return true;
}

/* Add a packet to the ack vector (using ackv_cons_ when needed)
 * Will fill with NOT_RECV packets when packets are missing
 * arg: seqnum   - sequence number of packet
 *      ecn      - packets ecn code point
 *      addNonce - if true, add packet nonce to ecn echo sum
 * ret: true if successful
 *      false if more space is needed and doubleMaxSize() fails
 *            or if packet is older than the oldest in the vector
 */
bool DCCPAckVector::addPacket(u_int32_t seqnum, dccp_ecn_codepoint ecn, bool addNonce){
	dccp_packet_state state = DCCP_PACKET_RECV;
	if (ecn == ECN_CE)
		state = DCCP_PACKET_ECN;

	bool result = addPacket(seqnum, state);
	if (result && addNonce && (ecn == ECN_ECT0 || ecn == ECN_ECT1))
		ene_ = ene_ ^ ecn;
	
	return result;
}

/* Remove packets older and including given seqnum from ack vector
 * arg: seqnum - sequence number of the largest packet to remove
 */
void DCCPAckVector::removePackets(u_int32_t seqnum){
	if (size_ == 0 || seqnum < seq_tail_)
		return;
	else if (seqnum > seq_head_) { 
		size_ = 0;
		seq_tail_ = seq_head_;
		tail_ = head_;
		ene_ = 0;
		return;
	}

	u_int16_t where;
	u_int32_t seq_start;
	u_int32_t seq_end;

	u_int32_t dist = 0;

	if (findSeqNum(seqnum, &where, &seq_start, &seq_end)){
		dist = seq_start-seq_end;
		if (dist > 0 && seq_start != seqnum){
			vector_[where] = vector_[where] - (seqnum-seq_end+1);
			size_ = ((where - head_ + max_size_) % max_size_)+1;
			tail_ = where;
			seq_tail_ = seqnum + 1;
		} else if (where == head_){
			size_ = 0;
			seq_tail_ = seq_head_;
			tail_ = head_;
			ene_ = 0;
		} else {
			where = ((where -1 + max_size_) % max_size_);
			size_ = ((where - head_ + max_size_) % max_size_)+1;
			tail_ = where;
			seq_tail_ = seqnum +1;
		}


	}
	
}

/* Merge two ack vectors.
 * Will read packet states from ackv and add them to this vector
 * using ackv_cons_.
 * If (ackv->seq_first_ > this->seq_first_)
 *    pad with DCCP_PACKET_NOT_RECV until they match
 * Discards all packets older than this->seq_last_.
 * If this->getSize() == 0
 *    use setAckVector()
 * arg: ackv - ack vector to merge with
 * ret: true if successful
 *      false if ackv == NULL or the upper limit on size is exceeded
 * Note: Adds packet in seqnum order until either successful
 *       or out of space.
 *       will NOT change ecn nonce echo as needed by the state change
 */
bool DCCPAckVector::mergeWith(DCCPAckVector *ackv){
	bool result = false;
	
	if (ackv == NULL)
		return false;
	else if (ackv->getSize() == 0)
		return true;

	if (size_ == 0){
		u_int16_t size;
		size = ackv->getSize();
		u_char vect[size];
		if (ackv->getAckVector(vect, &size) > 0)
			if (setAckVector(vect,size,ackv->getFirstSeqNum(),ackv->getENE()))
				result = true;
	} else {
		u_int32_t seqnum;
		dccp_packet_state state;
		ackv->startReversePacketWalk();
		while(ackv->prevPacket(&seqnum, &state)){
			if (!addPacket(seqnum,state) && seqnum >= seq_tail_){
				fprintf(stdout,"DCCPAckVector::mergeWith() failed\n");
				result = false;
				break;
			}
		}
	}

	
	return result;
}

/* Alter state of one packet.
 * Will split items as needed. Note will not alter 
 * arg: seqnum   - sequence number of packet to add
 *      newstate - packets new state
 *      oldstate - the packets old state
 * ret: true if successful
 *      false if packet is not in vector or size
 *            if the upper limit on size is exceeded
 * Note: will not alter the vector on failure.
 *       will NOT change ecn nonce echo as needed by the state change
 */
bool DCCPAckVector::alterState(u_int32_t seqnum, dccp_packet_state newstate, dccp_packet_state *oldstate){
	u_int16_t where;
	u_int32_t seq_start;
	u_int32_t seq_end;
	u_char items[3];
	int num_to_add = 0;
	dccp_packet_state old_state;
	u_int8_t old_runlen;
	bool result = false;
	
	if (findSeqNum(seqnum, &where, &seq_start, &seq_end)){
		old_state = (dccp_packet_state) (vector_[where] >> DCCP_RUN_LENGTH_BITS);

		if (oldstate != NULL)
			*oldstate = old_state;
		
		if (old_state != newstate){			
			old_runlen = vector_[where] & DCCP_RUN_LENGTH_MASK;
			if (old_runlen == 0){
				vector_[where] = ((u_int8_t) newstate) << DCCP_RUN_LENGTH_BITS;
				result = true;
			} else if (seqnum == seq_start || seqnum == seq_end){
				if (size_ + 1 > max_size_){
				        //increase size and try again
					result = (doubleMaxSize() && alterState(seqnum, newstate));
				} else {
					if(seqnum == seq_start){
						items[0] = ((u_int8_t) newstate) << DCCP_RUN_LENGTH_BITS;
						items[1] = (((u_int8_t) old_state) << DCCP_RUN_LENGTH_BITS)
							+ (old_runlen-1);
					} else {
						items[0] = (((u_int8_t) old_state) << DCCP_RUN_LENGTH_BITS)
							+ (old_runlen-1);
						items[1] = ((u_int8_t) newstate) << DCCP_RUN_LENGTH_BITS;
					}
					num_to_add = 2;
				}
			} else {
				if (size_ + 2 > max_size_){
				        //increase size and try again
					result = (doubleMaxSize() && alterState(seqnum, newstate));
				} else {
					items[0] = (((u_int8_t) old_state) << DCCP_RUN_LENGTH_BITS)
							+ (old_runlen-1-(seqnum-seq_end));
					items[1] = ((u_int8_t) newstate) << DCCP_RUN_LENGTH_BITS;
					items[2] = (((u_int8_t) old_state) << DCCP_RUN_LENGTH_BITS)
							+ (old_runlen-1-(seq_start-seqnum));
					num_to_add = 3;
				}
			}
		} else
			result = true;
	}
	
	if (num_to_add > 1){
		if(where == tail_){
			tail_ = (tail_ + (num_to_add - 1)) % max_size_;			
		} else if (where == head_){
			head_ = (head_ - (num_to_add - 1) + max_size_) % max_size_;
			where = head_;
		} else {
			int from = head_;
			int to = (head_ - (num_to_add - 1) + max_size_) % max_size_;
			head_ = to;
			while (from != where){
				vector_[to] = vector_[from];
				from = (from + 1) % max_size_;
				to = (to + 1) % max_size_;
			}
			where = (where - (num_to_add - 1) + max_size_) % max_size_;
		}
		for(int i = 0; i < num_to_add; i++){
			vector_[where] = items[i];
			where = (where + 1 ) % max_size_;
		}
		size_ += num_to_add - 1;
		result = true;
	}
	
	return result;
	
}

/* Returns the state of a packet
 * arg: seqnum - packets sequence number
 *      state  - the packets state (if found)
 * ret: true if packet is found
 *      false if not
 */
bool DCCPAckVector::getState(u_int32_t seqnum, dccp_packet_state *state){
	if (size_ == 0 || seqnum > seq_head_ || seqnum < seq_tail_)
		return false;
	bool result = false;
	u_int16_t where;
	u_int32_t seq_start;
	u_int32_t seq_end;

	if(findSeqNum(seqnum, &where, &seq_start, &seq_end)){
		*state = (dccp_packet_state) (vector_[where] >> DCCP_RUN_LENGTH_BITS);
		result = true;
	}

	return result;
}

/* Start a walk through the ack vector packet by packet,
 * descending on sequence number.
 */
void DCCPAckVector::startPacketWalk(){
	p_walk_ = head_;
	seq_p_walk_ = seq_head_;
	delta_p_walk_ = 0;
	p_walking_ = (size_ != 0);
	p_reverse_ = false;
}

/* Next packet in walk
 * arg: seqnum - sequence number of next packet
 *      state  - packet state
 * ret: true if packet is found
 *      false if packet is not found or walk not initiated.
 */
bool DCCPAckVector::nextPacket(u_int32_t *seqnum, dccp_packet_state *state){
	if (!p_walking_ || p_reverse_)
		return false;

	u_int8_t runlen = vector_[p_walk_] & DCCP_RUN_LENGTH_MASK;

	if (delta_p_walk_ > runlen){
		p_walk_ = (p_walk_+1) % max_size_;
		delta_p_walk_ = 0;
	}
		
	*seqnum = seq_p_walk_;
	*state = (dccp_packet_state) (vector_[p_walk_] >> DCCP_RUN_LENGTH_BITS);

	delta_p_walk_++;

	p_walking_ = (seq_p_walk_ != seq_tail_);

	seq_p_walk_--;

	return true;
}

/* Start a walk through the ack vector packet by packet,
 * ascending on sequence number.
 */
void DCCPAckVector::startReversePacketWalk(){
	p_walk_ = tail_;
	seq_p_walk_ = seq_tail_;
	delta_p_walk_ = 0;
	p_walking_ = (size_ != 0);
	p_reverse_ = true;
}

/* Previous packet in walk
 * arg: seqnum - sequence number of previous packet
 *      state  - packet state
 * ret: true if packet is found
 *      false if packet is not found or walk not initiated.
 */
bool DCCPAckVector::prevPacket(u_int32_t *seqnum, dccp_packet_state *state){
	if (!p_walking_ || !p_reverse_)
		return false;

	u_int8_t runlen = vector_[p_walk_] & DCCP_RUN_LENGTH_MASK;

	if (delta_p_walk_ > runlen){
		p_walk_ = (p_walk_+max_size_-1) % max_size_;
		delta_p_walk_ = 0;
	}
		
	*seqnum = seq_p_walk_;
	*state = (dccp_packet_state) (vector_[p_walk_] >> DCCP_RUN_LENGTH_BITS);

	delta_p_walk_++;

	p_walking_ = (seq_p_walk_ != seq_head_);

	seq_p_walk_++;

	return true;
}

/* Start a walk through the ack vector, item for item
 * Most recent interval first.
 */
void DCCPAckVector::startIntervalWalk(){
	i_walk_ = head_;
	seq_i_walk_ = seq_head_;
	i_walking_ = size_ != 0;
}

/* Next interval in walk
 * arg: interval  - found interval
 *      seq_start - the sequence number that started the interval
 *      seq_end   - the sequence number that ended the interval
 *      state     - the state of the packet in the interval
 * ret: true if an interval is found
 *      false if not or walk not initiated.
 */
bool DCCPAckVector::nextInterval(u_char *interval, u_int32_t *seq_start, u_int32_t *seq_end, dccp_packet_state *state){
	if (!i_walking_)
		return false;

	*interval = vector_[i_walk_];
	*seq_start = seq_i_walk_;
	*seq_end = seq_i_walk_ - (vector_[i_walk_] & DCCP_RUN_LENGTH_MASK);
	*state = (dccp_packet_state) (vector_[i_walk_] >> DCCP_RUN_LENGTH_BITS);
	
	i_walking_ = (i_walk_ != tail_);
	i_walk_ = (i_walk_ +1) % max_size_;
	seq_i_walk_ = *seq_end - 1;
	
	return true;
}

/* Return the ECN Nonce Echo
 * ret: ECN Nonce Echo
 */
u_int8_t DCCPAckVector::getENE(){
	return ene_;
}

/* Set the ECN Nonce Echo
 * arg: ene - new ene
 */
void DCCPAckVector::setENE(u_int8_t ene){
	ene_ = ene;
}

/* Chech if ack vector is empty
 * ret: true if empty, otherwise false
 */
bool DCCPAckVector::isEmpty(){
	return (size_ == 0);
}

/* Return the first sequence number represented in the ack vector
 * ret: first sequence number in the vector
 *      0 if empty
 */
u_int32_t DCCPAckVector::getFirstSeqNum(){
	return seq_head_;
}

/* Return the current size of the ack vector
 * ret: current size
 */
u_int32_t DCCPAckVector::getLastSeqNum(){
	return seq_tail_;
}

/* Return the current size of the ack vector
 * ret: current size
 */
int DCCPAckVector::getSize(){
	return size_;
}

/* Return the ack vector
 * arg: vect - array to store ack vector in
 *      size - size of vect
 * ret: The size of returned ack vector (also in size) if successful
 *      DCCP_ACKV_EMPTY if its empty
 *      DCCP_ACKV_ERR_SIZE if size is too small
 *	(will set size to the size needed)
 */	
int DCCPAckVector::getAckVector(u_char* vect, u_int16_t *size){
	if (size_ == 0)
		return DCCP_ACKV_EMPTY;
	else if (size_ > *size){
		*size = size_;
		return DCCP_ACKV_ERR_SIZE;
	}

	int walker = head_;

	for (int i = 0; i < size_; i++){
		vect[i] = vector_[walker];
		walker = (walker + 1) % max_size_;
	}
	*size = size_;
	
	return size_;
}

/* Import an ack vector (clearing the old one)
 * arg: vect      - ack vector to set
 *      size      - size of vect
 *      seq_start - sequnce number of the first packet in ack vector
 *      ene       - the current ecn nonce echo
 * ret: true if successful
 *      false if out of space
 */
bool DCCPAckVector::setAckVector(const u_char* vect, u_int16_t size, u_int32_t seq_start, u_int8_t ene){
	if (size > max_size_) {
		size_ = 0;
		return (doubleMaxSize() && setAckVector(vect, size, seq_start, ene));
	}

	u_int32_t seq_end = seq_start;
	
	for (int i = 0; i < size; i++){
		vector_[i] = vect[i];
		if (i == size-1)
			seq_end -= vect[i] & DCCP_RUN_LENGTH_MASK;
		else
			seq_end -= (vect[i] & DCCP_RUN_LENGTH_MASK) + 1;
	}

	size_ = size;
	head_ = 0;
	tail_ = size_-1;
	seq_head_ = seq_start;
	seq_tail_ = seq_end;
	ene_ = ene;

	clearAckHistory();
	return true;
}


/* Return and mark the ackvector as sent in history
 * arg: seqnum - sequence number of packet on which ackvector is added
 *      acknum - acknowledgement number on that packet (== seq_first_)
 *      vect   - ack vector to send
 *      size   - size of ackvector
 * ret: same as getAckVector()
 * Note: Fails hard if seq_first_ != acknum! 
 */
int DCCPAckVector::sendAckVector(u_int32_t seqnum, u_int32_t acknum, u_char* vect, u_int16_t *size){
	if (acknum != seq_head_){
		fprintf(stdout, "DCCPAckVector::sendAckVector acknum %d != seq_head %d\n",acknum,seq_head_);
		fflush(stdout);
		abort();
	}

	int result = getAckVector(vect, size);

	if (result > 0) {
		addToAckHistory(seqnum);
	}

	return result;
}

/* Add ack vector options and mark as sent in history
 * arg: seqnum  - sequence number of packet on which ackvector is added
 *      acknum  - acknowledgement number on that packet (== seq_first_)
 *      options - DCCPOptions object to add ackvector to
 * ret: DCCP_OPT_NO_ERR on success
 *      similar to getAckVector and addOptions()
 */
int DCCPAckVector::sendAckVector(u_int32_t seqnum, u_int32_t acknum, DCCPOptions *options){
	if (acknum != seq_head_){
		fprintf(stdout, "DCCPAckVector::sendAckVector acknum %d != seq_head %d\n",acknum,seq_head_);
		fflush(stdout);
		abort();
	} else if (size_ == 0){
		fprintf(stdout, "DCCPAckVector::sendAckVector size_ == 0\n");
		fflush(stdout);
		abort();
	}
	u_char vect[size_];
	u_int16_t size;
	size = size_;
        int result = getAckVector(vect,&size);
	if (result > 0){
		//for now, assume that the ackvector can be contained in one option
		if (ene_ == 0)
			result = options->addOption(DCCP_OPT_ACK_VECTOR_N0, vect, size);
		else
			result = options->addOption(DCCP_OPT_ACK_VECTOR_N1, vect, size);
		if (result == DCCP_OPT_NO_ERR){
			addToAckHistory(seqnum);
		} else {
			fprintf(stderr, "DCCPAckVector::sendAckVector - Failed to add ack vector to option: err %d, size %d\n",result, size);
			fflush(stdout);
			abort();
		}

	} else {
		fprintf(stderr, "DCCPAckVector::sendAckVector - Get vector returned %d\n",result);
		fflush(stdout);
		abort();
	}
       
	return result;
}

/* Check if state may be cleared due to an received ack.
 * If ackv is present, check in that to see if an ack has been acked
 * otherwise check only on acknum
 * arg: acknum - acknowledgement number recv
 *      ackv - ackvector received (if any)
 */
void DCCPAckVector::ackRecv(u_int32_t acknum, DCCPAckVector *ackv){
	struct dccp_ackv_hist_entry *elm = NULL, *next_elm = NULL;
	dccp_packet_state state;
	u_int8_t ene_sub = 0;
	
	elm = STAILQ_FIRST(&ack_hist_);
	while (elm != NULL){
		if (ackv == NULL) {
			if (acknum == elm->seq_sent_)
				break;
		} else if (ackv->getState(elm->seq_sent_, &state) && (state == DCCP_PACKET_RECV || state == DCCP_PACKET_ECN)){
			break;
		}
		elm = STAILQ_NEXT(elm, linfo_);
	}

	if (elm != NULL){
		ene_sub = elm->ene_;
		ene_ = ene_ ^ (elm->ene_);
		removePackets(elm->ack_num_);
		do{
		       next_elm = STAILQ_NEXT(elm, linfo_);
		       STAILQ_REMOVE(&ack_hist_,elm,dccp_ackv_hist_entry,linfo_);
		       delete elm;
		       elm = next_elm;
		} while (elm != NULL);

		//subtract ene from later acks
		elm = STAILQ_FIRST(&ack_hist_);
		while (elm != NULL) {
			elm->ene_ = (elm->ene_) ^ ene_sub;
			elm = STAILQ_NEXT(elm, linfo_);
		}
	}
	
}

/* Print ack vector state including intervals and ack history
 * Note: Uses interval walk
 */
void DCCPAckVector::print(){
	if (isEmpty()) {
		fprintf(stdout, "DCCPAckVector :: Ack vector is empty (max_size_ %d)\n",max_size_);
	} else {
		fprintf(stdout, "DCCPAckVector :: size_ %d, max_size_ %d, head_ %d, tail_ %d, seq_head_ %d, seq_tail_ %d, ene_ %d\n", size_, max_size_,head_,tail_,seq_head_,seq_tail_, ene_);
		startIntervalWalk();
		u_char interval;
		u_int32_t seq_start;
		u_int32_t seq_end;
		dccp_packet_state state;

		while(nextInterval(&interval, &seq_start, &seq_end, &state)){
			fprintf(stdout, "interval 0x%X (%d): state %s, runlen %d, (seq %d - %d)\n",interval, interval, packetStateAsStr(state), seq_start-seq_end,seq_start,seq_end);
		}
	}
	printHistory();
}

/* Print all packets
 * arg: asc - if true, print in ascending order, otherwise descending
 * Note: uses packet walk
 */
void DCCPAckVector::printPackets(bool asc){
	if (isEmpty()) {
		fprintf(stdout, "DCCPAckVector :: Ack vector is empty (max_size_ %d)\n",max_size_);
	} else {
		fprintf(stdout, "DCCPAckVector :: size_ %d, max_size_ %d, head_ %d, tail_ %d, seq_head_ %d, seq_tail_ %d, ene_ %d\n", size_, max_size_,head_,tail_,seq_head_,seq_tail_, ene_);
		u_int32_t seq_num;
		dccp_packet_state state;

		if (asc){
			startReversePacketWalk();
			while(prevPacket(&seq_num,&state)){
				fprintf(stdout, "Packet : seq %d state %s\n",seq_num, packetStateAsStr(state));
			}
		} else {
			startPacketWalk();
			while(nextPacket(&seq_num,&state)){
				fprintf(stdout, "Packet : seq %d state %s\n",seq_num, packetStateAsStr(state));
			}
		}
	}
}

/* Print ack history */
void DCCPAckVector::printHistory(){
	struct dccp_ackv_hist_entry *elm = STAILQ_FIRST(&ack_hist_);
	if (elm == NULL)
		fprintf(stdout, "DCCPAckVector :: Ack history is empty\n");
	else {
		fprintf(stdout, "DCCPAckVector :: Ack history:\n");
		while (elm != NULL){
			fprintf(stdout, "Seq %d sent with ack %d and ene %d\n", elm->seq_sent_, elm->ack_num_,elm->ene_);
			elm = STAILQ_NEXT(elm, linfo_);
		}
	}
}

