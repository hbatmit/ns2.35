#include "agg-spec.h"

/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2000  International Computer Science Institute
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
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/pushback/agg-spec.cc,v 1.6 2001/01/20 21:41:45 sfloyd Exp $ (ACIRI)
 */

#include "ip.h"
#include "ident-tree.h"

AggSpec::AggSpec(int dstON, int dstPrefix, int dstBits) {

  dstON_ = dstON;
  dstPrefix_ = dstPrefix;
  dstBits_ = dstBits;
  
  ptype_=-1;
  ptypeShare_=0;
}

AggSpec::AggSpec(AggSpec * orig) {
  dstON_ = orig->dstON_;
  dstPrefix_ = orig->dstPrefix_;
  dstBits_ = orig->dstBits_;

  ptype_ = orig->ptype_;
  ptypeShare_=orig->ptypeShare_;
}

int
AggSpec::member(Packet * pkt) {
  
  hdr_ip * iph = hdr_ip::access(pkt);
  ns_addr_t dst = iph->dst();
  int fid = iph->flowid();

  if (dstON_) {
    int prefix;
    if (AGGREGATE_CLASSIFICATION_MODE_FID) 
      prefix = getPrefix(fid);
    else 
      prefix = getPrefix(dst.addr_);

    if (prefix == dstPrefix_) {
      return 1;
    }
  }

#ifdef DEBUG_AS
  printf("AS: non-member packet with dst %d at %g. Agg: ", dst.addr_, Scheduler::instance().clock());
  print();
#endif
  return 0;
}

int 
AggSpec::getPrefix(int addr) {
  
  int andAgent = ((1 << dstBits_) - 1) << (NO_BITS - dstBits_);
  return (addr &  andAgent);
}
 
int 
AggSpec::equals(AggSpec * another) {
  
  return (dstON_ == another->dstON_ && dstPrefix_ == another->dstPrefix_ && 
	  dstBits_ == another->dstBits_);
}

int 
AggSpec::contains(AggSpec * another) {
  
  if (another->dstBits_ < dstBits_) return 0; 
  if (dstON_ != another->dstON_) return 0;

  int prefix1 = PrefixTree::getPrefixBits(dstPrefix_, dstBits_);
  int prefix2 = PrefixTree::getPrefixBits(another->dstPrefix_, dstBits_);
  
  return (prefix1 == prefix2);
}

void 
AggSpec::expand(int prefix, int bits) {
  
  dstPrefix_ = prefix;
  dstBits_ = bits;
}

int 
AggSpec::subsetOfDst(int prefix, int bits) {
  
  if (!dstON_ || dstBits_ < bits) return 0;

  int myPrefix = PrefixTree::getPrefixBits(dstPrefix_, bits);
  return (prefix==myPrefix);
}

AggSpec *
AggSpec::clone() {
  return new AggSpec(this);
}

int 
AggSpec::getSampleAddress() {
  
  //for now, return the prefix itself
  if (AGGREGATE_CLASSIFICATION_MODE_FID) 
    return 1;
  else 
    return dstPrefix_;
}

int 
AggSpec::prefixBitsForMerger(AggSpec * agg1, AggSpec * agg2) {
  
  int bitsNow = (agg1->dstBits_ < agg2->dstBits_)? agg1->dstBits_: agg2->dstBits_;
  for (int i=bitsNow; i>=0; i--) {
    int prefix1 = PrefixTree::getPrefixBits(agg1->dstPrefix_, i);
    int prefix2 = PrefixTree::getPrefixBits(agg2->dstPrefix_, i);
    if (prefix1==prefix2) {
      return i;
    }
  }

  printf("AS: Error: Should never come here\n");
  exit(-1);
}

void 
AggSpec::print() {
  //printf("Prefix = %d Bits = %d\n", dstPrefix_, dstBits_);
}
  
