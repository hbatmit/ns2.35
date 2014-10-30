/* -*-  Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */
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
 */

#include "rate-limit.h"

#include "random.h"
#include "ident-tree.h"
#include "pushback.h"

// ######################### RateLmitSessionList Methods #####################

int
RateLimitSessionList::filter(Packet * pkt, int lowDemand) {

  RateLimitSession * next = first_;
  double dropP = -1;
  
  while (next != NULL) {
    double p = next->log(pkt, lowDemand);
    if (p >= dropP) dropP = p;
    next = next->next_;
  }

#ifdef DEBUG_RLSL
  if (dropP == -1) {
    printf("RLSList: Found a non-member packet at %g\n", Scheduler::instance().clock());
    fflush(stdout);
  }
#endif

  double u = Random::uniform();
  if (u <= dropP) {
#ifdef DEBUG_RLSL
    printf("RLSList:%d Dropping Packet in filter. \n", first_->logData_->myID_);
    fflush(stdout);
#endif
   return 1;
  }
  
  //not dropped 
  return 0;
}

int 
RateLimitSessionList::insert(RateLimitSession * session) {
  RateLimitSession * listItem = first_;
  
  while (listItem != NULL) {
    if ( listItem->aggSpec_->equals(session->aggSpec_) ) {
      return 0;
    }
    listItem = listItem->next_;
  }
  
  session->setSucc(first_);
  first_ = session;
  session->localID_ = IDCounter_++;
  noSessions_++;
  return 1;
}
  
RateLimitSession *
RateLimitSessionList::containsLocalAggSpec(AggSpec * spec, int myID) {
  RateLimitSession * listItem = first_;
  while (listItem != NULL) {
    if ( listItem->origin_== myID ) {
      if ( listItem->aggSpec_->contains(spec) ) {
	return listItem;
      } 
      //  else if ( spec->contains(listItem->aggSpec_) ) {
      //	return 2;
      // }
    }
    listItem = listItem->next_;
  }
  return NULL;
}

int 
RateLimitSessionList::containsAggSpec(AggSpec * spec) {
  RateLimitSession * listItem = first_;
  while (listItem != NULL) {
    if ( listItem->aggSpec_->contains(spec) ) {
      return 1;
    }
    listItem = listItem->next_;
  }
  return 0;
}

//merger will only take place for aggregates started at this node.
void 
RateLimitSessionList::mergeSessions(int myID) {
  RateLimitSession * session1 = first_;
  while (session1 != NULL) {
    AggSpec * agg1 = session1->aggSpec_;
    RateLimitSession * session2 = session1->next_;
    while (session2 != NULL) {
      AggSpec * agg2 = session2->aggSpec_;
      if (session1->origin_== myID && session2->origin_ == myID &&
	  agg1->dstON_  && agg2->dstON_) {
	int bits = AggSpec::prefixBitsForMerger(agg1, agg2);
	if (bits==0) {
		
	    //a measure of how much are we broadening.
	    int bitsDiff = ((agg1->dstBits_<agg2->dstBits_)? agg1->dstBits_: agg2->dstBits_) - bits;
	    int prefix = PrefixTree::getPrefixBits(agg1->dstPrefix_, bits);
	    int count = getMySubsetCount(prefix, bits, myID);
	    if (count <2) {
		    printf("Error: Anomaly \n");
		    exit(-1);
	    }
	    if (PushbackAgent::mergerAccept(count, bits, bitsDiff)) {
		    merge(prefix, bits, myID);
	    }
	}
      }
      session2 = session2->next_;
    }
    session1 = session1->next_;
  }
}

void
RateLimitSessionList::merge(int prefix, int bits, int myID) {
  RateLimitSession * listItem = first_;
  RateLimitSession * firstItem = NULL;
  while (listItem != NULL) {
    if ( listItem->origin_== myID && 
	 listItem->aggSpec_->subsetOfDst(prefix, bits) &&
	 !listItem->merged_) {
      if (firstItem == NULL) {
	firstItem = listItem;
      } else {
	//merge here with firstItem
	firstItem = RateLimitSession::merge(firstItem, listItem, bits);
      }
    }
    listItem = listItem->next_;
  }
  if (firstItem == NULL) {
    printf("Error: Anomaly no 2\n");
    exit(-1);
  }
  
  firstItem->aggSpec_->expand(prefix, bits);
}
  
int 
RateLimitSessionList::getMySubsetCount(int prefix, int bits, int myID) {
 RateLimitSession * listItem = first_;
 int count=0;
 while (listItem != NULL) {
    if ( listItem->origin_== myID &&
	 listItem->aggSpec_->subsetOfDst(prefix, bits))
      count++;
    
    listItem = listItem->next_;
  }
 return count;
}

int 
RateLimitSessionList::noMySessions(int myID) {
  RateLimitSession * listItem = first_;
  int count=0;
  while (listItem != NULL) {
    if ( listItem->origin_==myID &&
	 !listItem->merged_ ) {
      count++;
    }
    listItem = listItem->next_;
  }
  return count;
}
  
RateLimitSession *
RateLimitSessionList::getSessionByLocalID(int localID) {
  
  RateLimitSession * listItem = first_;
  while (listItem != NULL) {
    if (listItem->localID_ == localID) {
      return listItem;
    }
    listItem = listItem->next_;
  }
  
  return NULL;
}

RateLimitSession *
RateLimitSessionList::getSessionByRemoteID(int remoteID) {
  
  RateLimitSession * listItem = first_;
  while (listItem != NULL) {
    if (listItem->remoteID_ == remoteID) {
      return listItem;
    }
    listItem = listItem->next_;
  }
  
  return NULL;
}

void 
RateLimitSessionList::endSession(RateLimitSession * rls) {
  
  if (first_==NULL) {
    printf("RLSL: Error. No session in progress\n");
    exit(-1);
  }

  if (first_==rls) {
    first_=rls->next_;
    noSessions_--;
    delete(rls);
    return;
  } 

  RateLimitSession * previous = first_;
  RateLimitSession * current = first_->next_;
  while (current!=NULL) {
    if (current == rls) {
      previous->next_=current->next_;
      noSessions_--;
      delete(rls);
      return;
    }
    previous = current;
    current=current->next_;
  }
  
  printf("RLSL: Error. The correct RLS not found\n");
  exit(-1);
}

//descending order
int 
RateLimitSessionList::rankRate(int myID, double rate) {
    int rank=0;
    RateLimitSession * listItem = first_;
    while (listItem != NULL) {
		if (listItem->origin_ == myID && listItem->getArrivalRateForStatus() > rate) {
			rank++;
		}
		listItem = listItem->next_;
	}
    
    return rank;
}

//ascending order
int 
RateLimitSessionList::rankSession(int myID, RateLimitSession * session) {
    int rank=0;
    RateLimitSession * listItem = first_;
    while (listItem != NULL) {
		if (listItem->origin_ == myID) {
			if (listItem->getArrivalRateForStatus() < session->getArrivalRateForStatus()) {
				rank++;
			}
			//to enforce deterministic ordering between sessions with same rate
			else if (listItem->getArrivalRateForStatus() == session->getArrivalRateForStatus() &&
					 listItem < session) {
				rank++;
			}
		}
		listItem = listItem->next_;
	}
    
    return rank;
}


// ############################# RateLmitSession Methods #####################

//local constructor
RateLimitSession::RateLimitSession(AggSpec * aggSpec, double rateEstimate, int initial, 
				   double limit, int origin, int locQID, 
				   double delay, double lowerBound, Node * node, RouteLogic * rtLogic):
  pushbackON_(0), merged_(0), next_(NULL) {
  aggSpec_ = aggSpec;
  origin_ = origin;
  remoteID_ = -1;
  localQID_ = locQID;
  remoteQID_ = -1;
  heightInPTree_ = 0; //always begin as a leaf.
  depthInPTree_ = 0;   
  startTime_ = Scheduler::instance().clock();
  expiryTime_ = startTime_ + delay;
  refreshTime_ = startTime_;
  lowerBound_ = lowerBound;
  initialPhase_=initial;
  rlStrategy_ = new RateLimitStrategy(limit, aggSpec_->ptype_, aggSpec->ptypeShare_, rateEstimate);
  logData_ = new LoggingDataStruct(node, rtLogic, aggSpec->getSampleAddress(), rateEstimate);
}

//remote constructor
RateLimitSession::RateLimitSession(AggSpec * aggSpec, double limit, int origin, int locQID, 
				   int remoteQID, int remoteID, int depth, double delay, 
				   double lowerBound, Node * node, RouteLogic * rtLogic):
  pushbackON_(0), merged_(0), initialPhase_(0), next_(NULL) {
  aggSpec_ = aggSpec;
  origin_ = origin;
  remoteID_ = remoteID;
  localQID_ = locQID;
  remoteQID_ = remoteQID;
  heightInPTree_ = 0; //always begin as a leaf.
  depthInPTree_ = depth;   
  startTime_ = Scheduler::instance().clock();
  expiryTime_ = startTime_ + delay;
  lowerBound_ = lowerBound;
  rlStrategy_ = new RateLimitStrategy(limit, aggSpec_->ptype_, aggSpec->ptypeShare_, 0);
  logData_ = new LoggingDataStruct(node, rtLogic, aggSpec->getSampleAddress(), 0);
}

double 
RateLimitSession::log(Packet *pkt, int lowDemand) {
  
  int member = aggSpec_->member(pkt); 
  
  if (member == 0) {
    //printf("RLS: Found a non-member packet at %g\n", Scheduler::instance().clock());
    return 0;
  }

  //expired session
  //   if (expiryTime_ < Scheduler::instance().clock()) {
  //     printf("RLS: Session expired at %g. expiryTime = %g\n", 
  // 	   Scheduler::instance().clock(), expiryTime_);
  //     return 0;
  //   }

  logData_->log(pkt);    //log the packet
  int mine = (origin_ == logData_->myID_); 
  double prob = rlStrategy_->process(pkt, mine, lowDemand);  //rate limit it.

  return prob;
}

double
RateLimitSession::getDropRate() {
  return rlStrategy_->getDropRate();
}

void
RateLimitSession::pushbackOn() {
  pushbackON_ = 1;
  rlStrategy_->reset();
}

void
RateLimitSession::refreshed() {
  refreshTime_ = Scheduler::instance().clock();
}

void 
RateLimitSession::setAggSpec(AggSpec * aggSpec) {
  aggSpec_->dstON_ = aggSpec->dstON_;
  aggSpec_->dstPrefix_ = aggSpec->dstPrefix_;
  aggSpec_->dstBits_ = aggSpec->dstBits_;
}

void
RateLimitSession::setLimit(double limit) {
  rlStrategy_->target_rate_=limit;
}
 
double
RateLimitSession::getArrivalRateForStatus()  {

  // for a leaf PBA, this is the rate seen at the rlStrategy_;
  // for non-leaf PBAs it is the sum of the rates reported by upstream PBAs 
  // in their status messages.
  
  double rate;
  
  if (pushbackON_) {
    logData_->consolidateStatus();
    rate = logData_->statusArrivalRateAll_;
  } 
  else {
    rate = rlStrategy_->getArrivalRate();
  }

  return rate;
}

RateLimitSession *
RateLimitSession::merge(RateLimitSession * session1, RateLimitSession * session2, int bits) {

  RateLimitSession *winner, *loser;
  
  if (session1->pushbackON_) {
    winner = session1;
    loser = session2;
  } else {
    winner = session2;
    loser = session1;
  }
  loser->merged_=1;
  
  int envelope;
  if (session1->aggSpec_->dstBits_==bits) 
    envelope = 1;
  else if (session2->aggSpec_->dstBits_==bits) 
    envelope=2;
  else 
    envelope=0;
  
  double lowerBound = pick4merge(session1->lowerBound_, session2->lowerBound_, envelope);
  winner->lowerBound_=lowerBound;

  double target_rate = pick4merge(session1->rlStrategy_->target_rate_,
				   session2->rlStrategy_->target_rate_, 
				   envelope);
  winner->setLimit(target_rate);

  double estRate = pick4merge(session1->rlStrategy_->rateEstimator_->estRate_,
			      session2->rlStrategy_->rateEstimator_->estRate_,
			      envelope);
  winner->rlStrategy_->rateEstimator_->estRate_=estRate;

  LoggingDataStruct * log1 = session1->logData_;
  LoggingDataStruct * log2 = session2->logData_;
  if (log1->count_ != log2->count_ || log1->myID_ != log2->myID_) {
    printf("RLS: Error: logdata count or ID anomaly\n");
    exit(-1);
  }
  
  estRate = pick4merge(log1->rateEstimator_->estRate_,
		       log2->rateEstimator_->estRate_,
		       envelope);
  winner->logData_->rateEstimator_->estRate_ = estRate;
  
  LoggingDataStructNode * node1 = log1->first_;
  LoggingDataStructNode * node2 = log2->first_;
  LoggingDataStructNode * nodew = winner->logData_->first_;
  
  while (node1 != NULL && node2!= NULL && nodew != NULL) {
    if (node1->nid_ != node2->nid_) {
      printf("RLS: Error: Out of order log nodes. Or something more sinister\n");
      exit(-1);
    }

    estRate =  pick4merge(node1->rateEstimator_->estRate_,
			  node2->rateEstimator_->estRate_,
			  envelope);
    nodew->rateEstimator_->estRate_ = estRate;
    
    double statusArrivalRate = pick4merge(node1->statusArrivalRate_,
					  node2->statusArrivalRate_,
					  envelope);
    nodew->statusArrivalRate_ = statusArrivalRate;

    node1=node1->next_;
    node2=node2->next_;
    nodew=nodew->next_;
  }
  
  if (node1 != NULL || node2 !=NULL || nodew != NULL) {
    printf("RLS: Error: Different chains\n");
    exit(-1);
  }
  
  return winner;
}

double
RateLimitSession::pick4merge(double q1, double q2, int envelope) {
  
  if (envelope == 1) {
    return q1;
  } else if (envelope == 2) {
    return q2;
  }
  return q1+q2;
}

RateLimitSession::~RateLimitSession() {
  delete(aggSpec_);
  delete(rlStrategy_);
  delete(logData_);
}
