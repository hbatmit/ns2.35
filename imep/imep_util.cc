/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *      This product includes software developed by the Computer Systems
 *      Engineering Group at Lawrence Berkeley Laboratory.
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
/* Ported from CMU/Monarch's code*/

/*
  imep_util.cc
  $Id: imep_util.cc,v 1.7 2006/02/21 15:20:18 mahrenho Exp $
  */

#include "packet.h"
#include "tora/tora_packet.h"
#include "imep/imep.h"

#define CURRENT_TIME	Scheduler::instance().clock()

static const int verbose = 0;

// ======================================================================
// Utility routines to manipulate IMEP packets.

imep_ack_block*
imepAgent::findAckBlock(Packet *p)
{
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_ack_block *ab;

	assert(im->imep_version == IMEP_VERSION);

	if((im->imep_block_flags & BLOCK_FLAG_ACK) == 0)
		return 0;

	ab = (struct imep_ack_block*) (im + 1);

	assert(ab->ab_num_acks > 0);

	return ab;
}

imep_hello_block*
imepAgent::findHelloBlock(Packet *p)
{
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_hello_block *hb;

	assert(im->imep_version == IMEP_VERSION);

	if((im->imep_block_flags & BLOCK_FLAG_HELLO) == 0)
		return 0;

	if(im->imep_block_flags & BLOCK_FLAG_ACK) {
		struct imep_ack_block *ab = findAckBlock(p);
		struct imep_ack *ack = (struct imep_ack*) (ab + 1);

		hb = (struct imep_hello_block*) (ack + ab->ab_num_acks);
	}
	else {
		hb = (struct imep_hello_block*) (im + 1);
	}

	assert(hb->hb_num_hellos > 0);

	return hb;
}

imep_object_block*
imepAgent::findObjectBlock(Packet *p)
{
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_ack_block *ab;
	struct imep_hello_block *hb;
	struct imep_object_block *ob;
	char *ptr;

	assert(im->imep_version == IMEP_VERSION);

	if((im->imep_block_flags & BLOCK_FLAG_OBJECT) == 0)
		return 0;	
	
	ptr = (char *) (im + 1);

	if (im->imep_block_flags & BLOCK_FLAG_ACK) {
	  ab = (struct imep_ack_block*) ptr;
	  ptr += ab->ab_num_acks * sizeof(struct imep_ack)
	    + sizeof(struct imep_ack_block);
	  assert(ab->ab_num_acks > 0);
	}
	if (im->imep_block_flags & BLOCK_FLAG_HELLO) {
	  hb = (struct imep_hello_block *) ptr;
	  ptr += hb->hb_num_hellos * sizeof(struct imep_hello) 
	    + sizeof(struct imep_hello_block);
	  assert(hb->hb_num_hellos > 0);
	}

	ob = (struct imep_object_block*) ptr;

	assert(ob->ob_protocol_type == PROTO_TORA);
	// for debugging purposes only

	assert(ob->ob_num_objects > 0);

	return ob;
}

struct imep_response*
imepAgent::findResponseList(Packet *p)
{
	struct imep_object_block *ob;
	struct imep_object *object;

	assert(HDR_IMEP (p)->imep_version == IMEP_VERSION);

	if((ob = findObjectBlock(p)) == 0)
		return 0;

	if(ob->ob_num_responses <= 0)
		return 0;

	object = (struct imep_object*) (ob + 1);
	for(int i = 0; i < ob->ob_num_objects; i++) {
		object = (struct imep_object*) ((char*) object + 
			   IMEP_OBJECT_SIZE + object->o_length);
	}

	return (struct imep_response*) object;
}

// ======================================================================
// ======================================================================

void
imepAgent::aggregateAckBlock(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_ack_block *ab = (struct imep_ack_block*) (im + 1);
	struct imep_ack *ack = (struct imep_ack*) (ab + 1);
	int num_acks = ackQueue.length();

	if(num_acks == 0) return;

	ch->size() += sizeof(struct imep_ack_block);
	U_INT16_T(im->imep_length) += sizeof(struct imep_ack_block);
	im->imep_block_flags |= BLOCK_FLAG_ACK;

	ab->ab_num_acks = 0;

	for(int i = 0; i < num_acks; i++) {

	        if (U_INT16_T(im->imep_length) + sizeof(struct imep_ack)
		    > IMEP_HDR_LEN) break;

		Packet *p0 = ackQueue.deque();
		assert(p0);

		struct imep_ack_block *ab0 = findAckBlock(p0);
		assert(ab0);

		struct imep_ack *ack0 = (struct imep_ack*) (ab0 + 1);

		ack->ack_seqno = ack0->ack_seqno;
		U_INT32_T(ack->ack_ipaddr) = U_INT32_T(ack0->ack_ipaddr);

		if (verbose) trace("T %0.9f _%d_ ack   %d:%d",
				   CURRENT_TIME, ipaddr, 
				   U_INT32_T(ack->ack_ipaddr),
				   ack->ack_seqno);

		ch->size() += sizeof(struct imep_ack);
		U_INT16_T(im->imep_length) += sizeof(struct imep_ack);
		ab->ab_num_acks++;
		ack++;

		Packet::free(p0);
	}
	assert(U_INT16_T(im->imep_length) <= IMEP_HDR_LEN);
}

void
imepAgent::aggregateHelloBlock(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_hello_block *hb;
	struct imep_hello *hello;
	int num_hellos = helloQueue.length();
	int inserted_hellos = 0;

	if (num_hellos == 0) return;
	if (U_INT16_T(im->imep_length) + sizeof(struct imep_hello_block)
	    + sizeof(struct imep_hello) > IMEP_HDR_LEN) return;

	hb = (struct imep_hello_block*)
		(((char*) im) + U_INT16_T(im->imep_length));
	hello = (struct imep_hello *) (hb + 1);

	ch->size() += sizeof(struct imep_hello_block);
	U_INT16_T(im->imep_length) += sizeof(struct imep_hello_block);

	for(int i = 0; i < num_hellos; i++) {

		if ( U_INT16_T(im->imep_length) + sizeof(struct imep_hello)
		     > IMEP_HDR_LEN ) break; // no more room

		Packet *p0 = helloQueue.deque();
		assert(p0);

		struct imep_hello_block *hb0 = findHelloBlock(p0);
		assert(hb0);

		struct imep_hello *hello0 = (struct imep_hello*) (hb0 + 1);

		imepLink *l = findLink(U_INT32_T(hello0->hello_ipaddr));

		if (l && CURRENT_TIME != l->lastEcho())
		  {
		    if (verbose) trace("T %0.9f _%d_ hello %d",
				       CURRENT_TIME, ipaddr, 
				       U_INT32_T(hello0->hello_ipaddr));
		    U_INT32_T(hello->hello_ipaddr) =
		      U_INT32_T(hello0->hello_ipaddr);
		    hello++;
		    l->lastEcho() = CURRENT_TIME;

		    inserted_hellos++;
		    ch->size() += sizeof(struct imep_hello);
		    U_INT16_T(im->imep_length) += sizeof(struct imep_hello);
		  }
		else 
		  { // this dest is already in this hello block, 
		    // or it was removed from our adj list since the 
		    // Hello was scheduled for it, so skip the dest
		  }

		Packet::free(p0);
	}

	if (0 == inserted_hellos) 
	  {
	    ch->size() -= sizeof(struct imep_hello_block);
	    U_INT16_T(im->imep_length) -= sizeof(struct imep_hello_block);
	    return;
	  }

	hb->hb_num_hellos = inserted_hellos;
	im->imep_block_flags |= BLOCK_FLAG_HELLO;

	assert(U_INT16_T(im->imep_length) <= IMEP_HDR_LEN);
}


// XXX: Objects generatd by TORA or other upper layer protocols
// don't contain a "response list".  Since, there is only one "response list"
// for potentially many objects, we generate the response list when
// the packet is sent.  I can't think of a case where the response
// list would NOT be "all of my neighbors."
void
imepAgent::aggregateObjectBlock(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_object_block *ob;
	struct imep_object *object;
	int num_objects = objectQueue.length();
	int response_list_len;
	int num_objects_inserted;

	if(num_objects == 0) return;
	if (U_INT16_T(im->imep_length) >= IMEP_HDR_LEN) return;

	ob = (struct imep_object_block*)
		((char*) im + U_INT16_T(im->imep_length));
	U_INT16_T(im->imep_length) += IMEP_OBJECT_BLOCK_SIZE;
	ch->size() += IMEP_OBJECT_BLOCK_SIZE;

	object = (struct imep_object*) (ob + 1);	

	ob->ob_sequence = 0;
	// initialized when the "response list" is created.
	ob->ob_protocol_type = PROTO_TORA; // XXX
	ob->ob_num_responses = 0;
	// initialized when the "response list" is created.

	response_list_len = getResponseListSize();

	for(num_objects_inserted = 0; 
	    num_objects_inserted < num_objects; 
	    num_objects_inserted++) {

		Packet *p0 = objectQueue.deque();
		assert(p0);

		struct hdr_cmn *ch0 = HDR_CMN(p0);
		struct hdr_tora *t = HDR_TORA(p0);

		int obj_length = toraHeaderLength(t);
		int new_len = U_INT16_T(im->imep_length) 
		  + IMEP_OBJECT_SIZE + obj_length 
		  + response_list_len;
		if (new_len > IMEP_HDR_LEN)
		  { // object won't fit, stop now
		    objectQueue.enqueHead(p0);
		    break;
		  }

		switch(ch0->ptype()) {

		case PT_TORA:
   		        toraExtractHeader(p0, ((char*) object) 
					  + IMEP_OBJECT_SIZE);
			break;

		default:
			fprintf(stderr, "Invalid Packet Type  %d\n",
				ch0->ptype());
			abort();
		}

		ch->size() += IMEP_OBJECT_SIZE + obj_length;

		U_INT16_T(im->imep_length) +=
			IMEP_OBJECT_SIZE + obj_length;

		object->o_length = obj_length;

		object = (struct imep_object*) ((char*) object + 
			   IMEP_OBJECT_SIZE + obj_length);

		Packet::free(p0);
	}

	assert(U_INT16_T(im->imep_length) <= IMEP_HDR_LEN);

	if (0 == num_objects_inserted)
	  { // remove the object block hdr we just worked so hard to put in
	    U_INT16_T(im->imep_length) -= IMEP_OBJECT_BLOCK_SIZE;
	    ch->size() -= IMEP_OBJECT_BLOCK_SIZE;
	    return;
	  }

	im->imep_block_flags |= BLOCK_FLAG_OBJECT;
	ob->ob_num_objects = num_objects_inserted;
	assert(ob->ob_protocol_type == PROTO_TORA);

	// add the response list
	createResponseList(p);

	stats.num_objects_created += num_objects_inserted;
	stats.num_object_pkts_created++;
	stats.sum_response_list_sz += ob->ob_num_responses;
	
	if (verbose) 
	  trace("T %.9f _%d_ # obj %d seq %d #resp %d RL %s", CURRENT_TIME, ipaddr,
		ob->ob_num_objects, ob->ob_sequence, ob->ob_num_responses, dumpResponseList(p));

	if (ob->ob_num_responses > 0)
	  { // now make a packet to save on the rexmit queue with only the
	    // object block and response list in it
	    Packet *sp = p->copy();
	    struct hdr_imep *sim = HDR_IMEP(sp);
	    struct hdr_cmn *sch = HDR_CMN(sp);
	    sim->imep_block_flags &= ~(BLOCK_FLAG_ACK | BLOCK_FLAG_HELLO);
	    int objrep_len = U_INT16_T(im->imep_length) 
	      - (int) ((char*) ob - (char*) im);
	    bcopy(ob, (char *)(sim + 1), objrep_len);
	    U_INT16_T(sim->imep_length) = sizeof(struct hdr_imep) + objrep_len;
	    sch->size() = IP_HDR_LEN  + sizeof(struct hdr_imep) + objrep_len;
	    stats.num_rexmitable_pkts++;

	    scheduleReXmit(sp);
	  }
}

// ======================================================================
// ======================================================================

int 
imepAgent::getResponseListSize()
{
  int size = 0;
  for(imepLink *l = imepLinkHead.lh_first; l; l = l->link.le_next) {

    if (l->status() != LINK_BI) continue;
    size += sizeof(struct imep_response);
  }
  return size;
}

void
imepAgent::createResponseList(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_object_block *ob;

	if((im->imep_block_flags & BLOCK_FLAG_OBJECT) == 0)
		return;

	ob = findObjectBlock(p);
	assert(ob);

	ob->ob_sequence = nextSequence();

	struct imep_response *r = (struct imep_response*)
		((char*) im + U_INT16_T(im->imep_length));

	for(imepLink *l = imepLinkHead.lh_first; l; l = l->link.le_next) {

	        if (l->status() != LINK_BI) continue;

		ch->size() += sizeof(struct imep_response);
		U_INT16_T(im->imep_length) += sizeof(struct imep_response);
		ob->ob_num_responses += 1;

		INT32_T(r->resp_ipaddr) = l->index();
		r++;
	}
}

