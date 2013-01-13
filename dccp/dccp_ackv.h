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
 * $Id: dccp_ackv.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_ackv_h
#define ns_dccp_ackv_h

#include "config.h"

#include "bsd_queue.h"
#include "dccp_types.h"
#include "dccp_opt.h"

#define DCCP_RUN_LENGTH_BITS 6
#define DCCP_RUN_LENGTH_MASK 0x3F

#define DCCP_PACKET_STATE_BITS 2
#define DCCP_PACKET_STATE_MASK 0xC0

#define DCCP_MAX_ACKV_SIZE 0xFFFF

#define DCCP_ACKV_OK 0
#define DCCP_ACKV_EMPTY -1
#define DCCP_ACKV_ERR_SIZE -2

//ack history
STAILQ_HEAD(dccp_ackv_hist,dccp_ackv_hist_entry); 
 
struct dccp_ackv_hist_entry {
	STAILQ_ENTRY(dccp_ackv_hist_entry) linfo_;    
	u_int32_t seq_sent_;            
	u_int32_t ack_num_;
	u_int8_t ene_;
};

/* Class DCCPAckVector represents an ack vector.
   Implemented as the October 2003 draft suggests */
class DCCPAckVector {
private:
	//packet state names
	static char* packet_state_str_[DCCP_NUM_PACKET_STATES];

        u_char* vector_;       //the real ack vector as a byte array
	u_int16_t size_;       //current size of vector
	u_int16_t max_size_;   //maximum size of vector (can increase)
	u_int16_t head_,tail_; //head and tail in the array
	u_int32_t seq_head_, seq_tail_;  //seq num of head and tail
	u_int8_t ene_;         //ecn nonce echo

	//packet walking variables
	u_int16_t p_walk_;     //current item in the vector
	u_int32_t seq_p_walk_; //current seq num
	int delta_p_walk_;     //current location within an element
	bool p_walking_;       //true if a packet walk is initiated
	bool p_reverse_;       //true if an reverse packet walk is initiated

	//interval walking variables
	u_int16_t i_walk_;     //current item in the vector
	u_int32_t seq_i_walk_; //current seq num
	bool i_walking_;       //true if an interval walk is initiated

	//history of acknowledgents (ack vectors) sent
	struct dccp_ackv_hist ack_hist_;

	/* Add items to the head of the vector
	 * arg: state  - packet state to set on items
	 *      runlen - run length to set on items
	 *      count  - number of similar items to add
	 * ret: true if successful
	 *      false if more space is needed and doubleMaxSize() fails.
	 */
	bool addItems(dccp_packet_state state, u_int8_t runlen, int count = 1);

	/* Find where in the vector a info about a seqnum exists
	 * arg: seqnum    - sequence number to find
	 *      where     - resulting position in the vector
	 *      seq_start - starting sequence number corresponding to where
	 *      seq_end   - ending sequence number corresponding to where
	 * ret: true if found
	 *      false otherwise (where, seq_start, seq_end invalid)
	 */
	bool findSeqNum(u_int32_t seqnum, u_int16_t *where, u_int32_t *seq_start, u_int32_t *seq_end);

	/* Increase the maximum size of the vector by a factor two
	 * until DCCP_MAX_ACKV_SIZE is reached.
	 * ret:  true if successful, false if already at DCCP_MAX_ACKV_SIZE
	 */
	bool doubleMaxSize();

	/* Add an acknowledgment to ack history
	 * arg: seqnum - sequence number of the sent ack packet
	 */
	void addToAckHistory(u_int32_t seqnum);

	/* Remove all items in the ack history */
	void clearAckHistory();

public:
	//ack vector conistency matrix
	dccp_packet_state ackv_cons_[DCCP_NUM_PACKET_STATES][DCCP_NUM_PACKET_STATES];

	/* Return a string reprenting a packet state
	 * arg: state - packet state
	 * ret: a string representation of state
	 */
	static const char* packetStateAsStr(dccp_packet_state state);

	/* Constructor
	 * arg: initialSize - initial size for the ack vector
	 * ret: a new DCCPAckVector
	 */
	DCCPAckVector(u_int16_t initialSize);

	/* Destructor */
	~DCCPAckVector();

	/* Add a packet to the ack vector (using ackv_cons_ when needed)
	 * Will fill with NOT_RECV packets when packets are missing
	 * arg: seqnum - sequence number of packet
	 *      state  - packet state
	 * ret: true if successful
	 *      false if more space is needed and doubleMaxSize() fails
	 *            or if packet is older than the oldest in the vector
	 */
	bool addPacket(u_int32_t seqnum, dccp_packet_state state);

	/* Add a packet to the ack vector (using ackv_cons_ when needed)
	 * Will fill with NOT_RECV packets when packets are missing
	 * arg: seqnum   - sequence number of packet
	 *      ecn      - packets ecn code point
	 *      addNonce - if true, add packet nonce to ecn echo sum
	 * ret: true if successful
	 *      false if more space is needed and doubleMaxSize() fails
	 *            or if packet is older than the oldest in the vector
	 */
	bool addPacket(u_int32_t seqnum, dccp_ecn_codepoint ecn, bool addNonce);

	/* Remove packets older and including given seqnum from ack vector
	 * arg: seqnum - sequence number of the largest packet to remove
	 */
	void removePackets(u_int32_t seqnum);

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
	bool mergeWith(DCCPAckVector *ackv);
	
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
	bool alterState(u_int32_t seqnum, dccp_packet_state newstate, dccp_packet_state *oldstate = NULL);
	
	/* Returns the state of a packet
	 * arg: seqnum - packets sequence number
	 *      state  - the packets state (if found)
	 * ret: true if packet is found
	 *      false if not
	 */
	bool getState(u_int32_t seqnum, dccp_packet_state *state);


	/* Packet walk functions.
	 * Note you can't start a descending packet walk and then
	 * use prevPacket(), and similiar, you can't use nextPacket
	 * if you started an ascending packet walk.
	 */

	/* Start a walk through the ack vector packet by packet,
	 * descending on sequence number.
	 */
	void startPacketWalk();

	/* Next packet in walk
	 * arg: seqnum - sequence number of next packet
	 *      state  - packet state
	 * ret: true if packet is found
	 *      false if packet is not found or walk not initiated.
	 */
	bool nextPacket(u_int32_t *seqnum, dccp_packet_state *state);

	/* Start a walk through the ack vector packet by packet,
	 * ascending on sequence number.
	 */
	void startReversePacketWalk();

	/* Previous packet in walk
	 * arg: seqnum - sequence number of previous packet
	 *      state  - packet state
	 * ret: true if packet is found
	 *      false if packet is not found or walk not initiated.
	 */
	bool prevPacket(u_int32_t *seqnum, dccp_packet_state *state);

	/* Start a walk through the ack vector, item for item
	 * Most recent interval first.
	 */
	void startIntervalWalk();

	/* Next interval in walk
	 * arg: interval  - found interval
	 *      seq_start - the sequence number that started the interval
	 *      seq_end   - the sequence number that ended the interval
	 *      state     - the state of the packet in the interval
	 * ret: true if an interval is found
	 *      false if not or walk not initiated.
	 */
	bool nextInterval(u_char *interval, u_int32_t *seq_start, u_int32_t *seq_end, dccp_packet_state *state);

	/* Return the ECN Nonce Echo
	 * ret: ECN Nonce Echo
	 */
	u_int8_t getENE();

	/* Set the ECN Nonce Echo
	 * arg: ene - new ene
	 */
	void setENE(u_int8_t ene);

	/* Chech if ack vector is empty
	 * ret: true if empty, otherwise false
	 */
	bool isEmpty();

	/* Return the first sequence number represented in the ack vector
	 * ret: first sequence number in the vector
	 *      0 if empty
	 */
	u_int32_t getFirstSeqNum();

	/* Return the last sequence number represented in the ack vector
	 * ret: last sequence number in the vector
	 *      0 if empty
	 */
	u_int32_t getLastSeqNum();
	
	/* Return the current size of the ack vector
	 * ret: current size
	 */
	int getSize();
		
	/* Return the ack vector
	 * arg: vect - array to store ack vector in
	 *      size - size of vect
	 * ret: The size of returned ack vector (also in size) if successful
	 *      DCCP_ACKV_EMPTY if its empty
	 *      DCCP_ACKV_ERR_SIZE if size is too small
	 *	(will set size to the size needed)
	 */		
	int getAckVector(u_char* vect, u_int16_t *size);

	/* Import an ack vector (clearing the old one)
	 * arg: vect      - ack vector to set
	 *      size      - size of vect
	 *      seq_start - sequnce number of the first packet in ack vector
	 *      ene       - the current ecn nonce echo
	 * ret: true if successful
	 *      false if out of space
	 */
	bool setAckVector(const u_char* vect, u_int16_t size, u_int32_t seq_start, u_int8_t ene);
	
	/* Return and mark the ackvector as sent in history
	 * arg: seqnum - sequence number of packet on which ackvector is added
	 *      acknum - acknowledgement number on that packet (== seq_first_)
	 *      vect   - ack vector to send
	 *      size   - size of ackvector
	 * ret: same as getAckVector()
	 * Note: Fails hard if seq_first_ != acknum! 
	 */
	int sendAckVector(u_int32_t seqnum, u_int32_t acknum, u_char* vect, u_int16_t *size);

	/* Add ack vector options and mark as sent in history
	 * arg: seqnum  - sequence number of packet on which ackvector is added
	 *      acknum  - acknowledgement number on that packet (== seq_first_)
	 *      options - DCCPOptions object to add ackvector to
	 * ret: DCCP_OPT_NO_ERR on success
	 *      similar to getAckVector and addOptions()
	 */
	int sendAckVector(u_int32_t seqnum, u_int32_t acknum, DCCPOptions *options);
	
	/* Check if state may be cleared due to an received ack.
	 * If ackv is present, check in that to see if an ack has been acked
	 * otherwise check only on acknum
	 * arg: acknum - acknowledgement number recv
	 *      ackv - ackvector received (if any)
	 */
	void ackRecv(u_int32_t acknum, DCCPAckVector *ackv = NULL);

	/* Print ack vector state including intervals and ack history
	 * Note: Uses interval walk
	 */
	void print();

	/* Print all packets
	 * arg: asc - if true, print in ascending order, otherwise descending
	 * Note: uses packet walk
	 */
	void printPackets(bool asc = false);

	/* Print ack history */
	void printHistory();
}; 

#endif

