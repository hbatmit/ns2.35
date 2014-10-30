/*
 * Copyright (c) 2000 Nortel Networks
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
 *      This product includes software developed by Nortel Networks.
 * 4. The name of the Nortel Networks may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORTEL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NORTEL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Developed by: Farhan Shallwani, Jeremy Ethridge
 *               Peter Pieda, and Mandeep Baines
 * Maintainer: Peter Pieda <ppieda@nortelnetworks.com>
 */

#ifndef dsredq_h
#define dsredq_h

//#include "dsred.h"
// maximum number of virtual RED queues in one physical queue
#define MAX_PREC 3	

enum mredModeType {rio_c, rio_d, wred, dropTail};

// struct qParam
//   This structure specifies the parameters needed to be maintained for 
//   each RED queue.
struct qParam {
  edp edp_;		// early drop parameters (see red.h)
  edv edv_;		// early drop variables (see red.h)
  int qlen;		// actual (not weighted) queue length in packets
  double idletime_;	// needed to calculate avg queue
  bool idle_;		// needed to calculate avg queue
};

// class redQueue
//    This class provides specs for one physical queue.
class redQueue {
 public:
  int numPrec;	// the current number of precedence levels (or virtual queues)
  int qlim;
  mredModeType mredMode;
  redQueue();
  // configures one virtual RED queue
  void config(int prec, int argc, const char*const* argv);	

  void initREDStateVar(void);		// initializes RED state variables
  // Updates a virtual queue's length after dequeueing
  void updateVREDLen(int);
  //sets idle_ flag to 0
  // Patch contributed by Thilo Wagner <wagner@panasonic.de>
  void updateIdleFlag(int);

  // updates RED variables after enqueueing/dequing a packet
  void updateREDStateVar(int prec);	
  // enques packets into a physical queue
  int enque(Packet *pkt, int prec, int ecn);
  Packet* deque(void);			// deques packets
  double getWeightedLength();
  int getRealLength(void);		// queries length of a physical queue
  // sets packet time constant values
  //(needed for calc. avgQSize) for each virtual queue

  // queries average length of a virtual queue
  double getWeightedLength_v(int prec); 
  // queries length of a virtual queue
  int getRealLength_v(int prec); 

  void setPTC(double outLinkBW);
  // sets mean packet size (needed to calculate avg. queue size)
  void setMPS(int mps);
  
 private:
  // underlying FIFO queue
  PacketQueue *q_;		
  // used to maintain parameters for each of the virtual queues
  qParam qParam_[MAX_PREC];
  // to calculate avg. queue size of a virtual queue
  void calcAvg(int prec, int m); 
  
};
#endif

