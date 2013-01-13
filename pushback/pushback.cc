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

#include "pushback.h"

#include "ident-tree.h"
#include "pushback-queue.h"
#include "rate-limit.h"
#include "pushback-message.h"

//#define DEBUG  

int hdr_pushback::offset_;

static class PushbackHeaderClass : public PacketHeaderClass {
public:
  PushbackHeaderClass() : PacketHeaderClass("PacketHeader/Pushback", 
					    sizeof(hdr_pushback)) {
    bind_offset(&hdr_pushback::offset_);
  }
} class_Pushback_hdr;


static class PushbackClass : public TclClass {
public:
  PushbackClass() : TclClass("Agent/Pushback") {}
  TclObject* create(int, const char*const*) {
    return (new PushbackAgent());
  }
} class_Pushback;

PushbackAgent::PushbackAgent() : Agent(PT_PUSHBACK), last_index_(0), intResult_(-1) {

  bind("last_index_", &last_index_);
  bind("intResult_", &intResult_);
  bind_bool("enable_pushback_", &enable_pushback_);
  bind_bool("verbose_", &verbose_);
  timer_ = new PushbackTimer(this);
  debugLevel = 3;
  //  debugLevel = 0;
}

int 
PushbackAgent::command(int argc, const char*const* argv) {
  
  Tcl& tcl = Tcl::instance();
  if (argc == 4 ) {
    if (strcmp(argv[1], "initialize") == 0) {
      //get the node and routeLogic object
      node_ = (Node *)TclObject::lookup(argv[2]);
      rtLogic_ = (RouteLogic *)TclObject::lookup(argv[3]);

      if (node_ == NULL || rtLogic_ == NULL) {
	if (verbose_) printf("Improper Initialization for Pushback Agent\n");
	return(TCL_ERROR);
      }
      
      sprintf(prnMsg, "node=%s rtLogic=%s id=%d address=%d\n", node_->name(), 
	     rtLogic_->name(), node_->nodeid(), node_->address());
      printMsg(prnMsg,0);

      return(TCL_OK);
    } 
  }  
  else if (argc == 3) { 
    //$pba add-queue $queue
    if (strcmp(argv[1], "add-queue") == 0) {
      if (last_index_==MAX_QUEUES) {
	printf("queue list size exhausted - recompile with a bigger MAX_QUEUES\n");
	exit(-1);
      }
      PushbackQueue * queue = (PushbackQueue *) TclObject::lookup(argv[2]);
      if (queue == NULL) {
	printf("NULL queue passed \n");
	exit(-1);
      }

      int index = last_index_++;
      queue_list_[index].pbq_ = queue;
      queue_list_[index].idTree_ = new IdentStruct();
      
      tcl.resultf("%d", index);
      return (TCL_OK);
    }
  }
  return (Agent::command(argc, argv));
}

void 
PushbackAgent::reportDrop(int qid, Packet * p) {

  if (!checkQID(qid)) {
    sprintf(prnMsg,"Got invalid qid %d\n", qid);
    printMsg(prnMsg,0);
    exit(-1);
  }
  
  hdr_ip * iph = hdr_ip::access(p);
  ns_addr_t src = iph->src();
  ns_addr_t dst = iph->dst();
  int fid = iph->flowid();

  sprintf(prnMsg,"DropDetails from queue %d: %d.%d -> %d.%d (%d)\n", qid, 
	   src.addr_, src.port_, dst.addr_, dst.port_, fid);
  printMsg(prnMsg, 5);
  
  queue_list_[qid].idTree_->registerDrop(p);
}

void 
PushbackAgent::calculateLowerBound(int qid, double arrRate) {
	
  if (!checkQID(qid)) {
    sprintf(prnMsg, "Got invalid id from queue in identifyAggregate\n");
    printMsg(prnMsg,0);
    exit(-1);
  }

  AggReturn * aggReturn = queue_list_[qid].idTree_->calculateLowerBound();
  if (aggReturn == NULL) {
	  //not sure what to do here.
	  //maybe lower bound should be left as it is
	  
	  return;
  }

  double lowerBound = 0;
  int i = 0;
  for (; i <= aggReturn->finalIndex_; i++) {
      cluster currCluster = aggReturn->clusterList_[i];
      AggSpec * aggSpec = new AggSpec(1, currCluster.prefix_, currCluster.bits_);
      RateLimitSession * rls1 = 
      queue_list_[qid].pbq_->rlsList_->containsLocalAggSpec(aggSpec, node_->nodeid());
      if (rls1 !=NULL) continue;
      lowerBound = (currCluster.count_)*(arrRate/aggReturn->totalCount_);
      sprintf(prnMsg, "LB: count: %d totalCount_: %d arrRate: %g\n", currCluster.count_, aggReturn->totalCount_, arrRate);
      printMsg(prnMsg,0);
      break;
  }
  
  if (i == aggReturn->finalIndex_+1) {
    sprintf(prnMsg, "Warning: All clusters being rate limited\n");
    printMsg(prnMsg,0);
    //exit(-1);
  }

  queue_list_[qid].idTree_->setLowerBound(lowerBound, 1);
  
  delete(aggReturn);
}

void
PushbackAgent::identifyAggregate(int qid, double arrRate, double linkBW) {

  //set up refresh timer for this queue, if this is the firstime you come here.
  if (!timer_->containsRefresh(qid)) {
    PushbackEvent * event = new PushbackEvent(PUSHBACK_CYCLE_TIME, PUSHBACK_REFRESH_EVENT, qid);
    timer_->insert(event);
  }

  // if (debug_) 
  sprintf(prnMsg, "identifyAggregate for %d\n",  qid);
  printMsg(prnMsg,0);

  if (!checkQID(qid)) {
    sprintf(prnMsg, "Got invalid id from queue in identifyAggregate\n");
    printMsg(prnMsg,0);
    exit(-1);
  }
  if (verbose_) queue_list_[qid].idTree_->traverse();

  //this is a quick way of achieving this.
  //but it can be justified on some grounds. will do a check with Sally later.
  int noSessions = queue_list_[qid].pbq_->rlsList_->noMySessions(node_->nodeid());
//   if (noSessions >= MAX_SESSIONS) {
// 	  sprintf(prnMsg, "My hands are full\n");
// 	  printMsg(prnMsg,0); 
// 	  return;
//   }
	  
  AggReturn * aggReturn = queue_list_[qid].idTree_->identifyAggregate(arrRate, linkBW);
  if (aggReturn == NULL) return;

  for (int i=0; i<=aggReturn->finalIndex_; i++) {
    cluster currCluster = aggReturn->clusterList_[i];
    AggSpec * aggSpec = new AggSpec(1, currCluster.prefix_, currCluster.bits_);
    
    //don't insert the same aggregate again.
    RateLimitSession * rls1 = 
      queue_list_[qid].pbq_->rlsList_->containsLocalAggSpec(aggSpec, node_->nodeid());
    if (rls1 != NULL) {
      sprintf(prnMsg, "got subset aggregate. Lowerbound = %g. agg = ", aggReturn->limit_);
      printMsg(prnMsg,0);
      aggSpec->print(); fflush(stdout);
      delete(aggSpec);
      //this could keep the lowerbound unnecessarily down.
      //but don't be sympathetic with aggregates, which have been identified again.
      if (aggReturn->limit_ < rls1->lowerBound_) {
	rls1->lowerBound_ = aggReturn->limit_;
      }
      //set the last misbehavior signal.
      rls1->refreshed();
      continue;
    }
    
    double estimate = (currCluster.count_)*(arrRate/aggReturn->totalCount_);
    
    if (noSessions >= MAX_SESSIONS) {
	int rank = queue_list_[qid].pbq_->rlsList_->rankRate(node_->nodeid(), estimate);
	if (rank >= MAX_SESSIONS) {
	    sprintf(prnMsg, "got rate <= minRate. agg = ");
	    printMsg(prnMsg,0);aggSpec->print(); fflush(stdout);
	    delete(aggSpec);	    
	    continue;
	} 
    }
    
    sprintf(prnMsg, "starting rate-limiting lower=%g estimate=%g agg ",  
	    aggReturn->limit_, estimate);
    printMsg(prnMsg,0);
    aggSpec->print();  fflush(stdout);
    
    double initialLimit = estimate; //*(1 - ambientDropRate);
    RateLimitSession * rls = new RateLimitSession(aggSpec, estimate, 1, initialLimit, 
						  node_->nodeid(), qid, 
						  RATE_LIMIT_TIME_DEFAULT, aggReturn->limit_,
						  node_, rtLogic_);
    queue_list_[qid].pbq_->rlsList_->insert(rls);
      
    PushbackEvent * event = new PushbackEvent(INITIAL_UPDATE_TIME, INITIAL_UPDATE_EVENT, rls);
    timer_->insert(event);
    //    }

    noSessions++;
  }

  queue_list_[qid].idTree_->setLowerBound(aggReturn->limit_, 0);
  delete(aggReturn);
}

void
PushbackAgent::resetDropLog(int qid) {

  sprintf(prnMsg, " drop log reset for qid %d\n",  qid);
  printMsg(prnMsg,5);
  
  if (!checkQID(qid)) {
    printf("Got invalid id from queue in resetDropLog\n");
    exit(-1);
  }

  queue_list_[qid].idTree_->reset();
}

void 
PushbackAgent::timeout(PushbackEvent * event) {
  
  sprintf(prnMsg, "  %s event for qid %d\n",  PushbackEvent::type(event), event->qid_);
  printMsg(prnMsg,0);
  switch (event->eventID_) {
  case PUSHBACK_CHECK_EVENT: pushbackCheck(event->rls_);
    break;
  case PUSHBACK_REFRESH_EVENT: pushbackRefresh(event->qid_);
    break;
  case PUSHBACK_STATUS_EVENT: pushbackStatus(event->rls_);
    break;
  case INITIAL_UPDATE_EVENT: initialUpdate(event->rls_);
    break;
  default: sprintf(prnMsg, " Unrecognized event %d\n",  event->eventID_);
    printMsg(prnMsg,0);
    break;
  }

}

void 
PushbackAgent::initialUpdate(RateLimitSession * rls) {
  
  if ( !rls->initialPhase_ ) {
    sprintf(prnMsg, " Error: Update when not in initialphase\n");
     printMsg(prnMsg,0);
     exit(-1);
  }

  double qdrop = queue_list_[rls->localQID_].pbq_->getDropRate();
  double dropRate = rls->getDropRate();
  double arrRate = rls->getArrivalRateForStatus();
  double newLimit = arrRate*(1 - 2*(dropRate+qdrop));
  
  sprintf(prnMsg,"Initial-Update: qdrop=%g dr=%g newL=%g oldTarget=%g lowerBound=%g arr=%g\n",
	  qdrop, dropRate, newLimit, rls->rlStrategy_->target_rate_, rls->lowerBound_, arrRate);
  printMsg(prnMsg,0);

  //cancel right now, if arrRate is significantly less than lower bound.
  if (arrRate < 0.75*rls->lowerBound_) {
      #ifdef DEBUG
        double now = Scheduler::instance().clock();
        printf("Cancel pushback A time: %5.3f\n", now);
      #endif
      pushbackCancel(rls);
      return;
  }

  if (newLimit > rls->lowerBound_) {
    rls->setLimit(newLimit);
    
    PushbackEvent * event = new PushbackEvent(INITIAL_UPDATE_TIME, INITIAL_UPDATE_EVENT, rls);
    timer_->insert(event);
  }
  else {
    rls->setLimit(rls->lowerBound_);
    rls->initialPhase_ = 0;
    
    if (rls->logData_->count_!=0 && enable_pushback_) {
      PushbackEvent * event = new PushbackEvent(PUSHBACK_CHECK_TIME, PUSHBACK_CHECK_EVENT, rls);
      timer_->insert(event);
    }
  }
}


void 
PushbackAgent::pushbackCheck(RateLimitSession * rls) {
  
  double dropRate = rls->getDropRate();

  if (dropRate >= DROP_RATE_FOR_PUSHBACK) {
    rls->pushbackOn();
    rls->heightInPTree_++;
    
    double totalRate =  rls->rlStrategy_->target_rate_;
    int count = rls->logData_->count_;
    double fairShare = totalRate/count;
    int done = count;
    
    //max-min allocation of limit.
    while (done != 0) {
      LoggingDataStructNode * lgdsNode = rls->logData_->first_;
      int countThisRound=0;
      while (lgdsNode != NULL) {
	double rate = lgdsNode->rateEstimator_->estRate_;
	if (rate <= fairShare && !lgdsNode->pushbackSent_) {
	  AggSpec * aggSpec = rls->aggSpec_->clone();
	  PushbackMessage * msg;
	  if (rate < fairShare/2.0) {
	    msg = new PushbackRequestMessage(node_->nodeid(), lgdsNode->nid_, rls->localQID_, 
					     rls->localID_, aggSpec, INFINITE_LIMIT, 
					     rls->depthInPTree_);
	    lgdsNode->pushbackSent(INFINITE_LIMIT, rate);
	  }	  
	  else {
	    msg = new PushbackRequestMessage(node_->nodeid(), lgdsNode->nid_, rls->localQID_, 
					     rls->localID_, aggSpec, rate, rls->depthInPTree_);
	    lgdsNode->pushbackSent(rate, rate);
	  }
	  sendMsg(msg);
	  countThisRound++;
	  done--;
	  totalRate -= rate;
	}
	lgdsNode = lgdsNode->next_;
      }
      if (done == 0) break;
      if (countThisRound==0) {
	//allocate fairshare to everyone and end.
	LoggingDataStructNode * lgdsNode = rls->logData_->first_;
	while (lgdsNode != NULL) {
	  if (!lgdsNode->pushbackSent_) {
	    AggSpec * aggSpec = rls->aggSpec_->clone();
	    PushbackMessage * msg = new PushbackRequestMessage(node_->nodeid(), lgdsNode->nid_, 
							       rls->localQID_, rls->localID_, 
							       aggSpec, fairShare, 
							       rls->depthInPTree_);
	    lgdsNode->pushbackSent(fairShare,lgdsNode->rateEstimator_->estRate_);
	    sendMsg(msg);
	    done--;
	    totalRate-=fairShare;
	  }
	  lgdsNode = lgdsNode->next_;
	}
      }
      else {
	fairShare= totalRate/done;
      }
    }
    
  }
  else {
    //set up pushback check for later.
    PushbackEvent * event = new PushbackEvent(PUSHBACK_CHECK_TIME, PUSHBACK_CHECK_EVENT, rls);
    timer_->insert(event);
  }
}

void 
PushbackAgent::pushbackStatus(RateLimitSession * rls) {

  if (rls->pushbackON_) {
    sprintf(prnMsg, " Warning: status timer expired for non-leaf node\n");
    printMsg(prnMsg,0);
     //exit(-1);
  }
  double rate = rls->getArrivalRateForStatus();
  rls->logData_->resetStatus();
  
  PushbackMessage * msg = new PushbackStatusMessage(node_->nodeid(), rls->origin_, 
						    rls->remoteQID_, rls->remoteID_, 
						    rate, rls->heightInPTree_);
  sendMsg(msg);
}

void 
PushbackAgent::pushbackRefresh(int qid) {
   
  PushbackQueue * pbq = queue_list_[qid].pbq_;
  int oldSessions = pbq->rlsList_->noMySessions(node_->nodeid());
  if (!oldSessions) {
    //set up refresh timers for a later time and return.
// PushbackEvent * event = new PushbackEvent(PUSHBACK_CYCLE_TIME, PUSHBACK_REFRESH_EVENT, qid);
// timer_->insert(event);
    return;
  }
  
  int noSessions = oldSessions;

  if (MERGER_MODE == 1) {
      pbq->rlsList_->mergeSessions(node_->nodeid());
      noSessions = pbq->rlsList_->noMySessions(node_->nodeid());
      
      if (noSessions!=oldSessions) {
	      sprintf(prnMsg, " Some sessions merged. old = %d new = %d\n",  oldSessions, noSessions);
	      printMsg(prnMsg,0);

		  //get rid of merged RLS's
		  RateLimitSession * listItem = pbq->rlsList_->first_;
		  while (listItem != NULL) {
			  if (listItem->origin_ == node_->nodeid() && listItem->merged_) {
				  pushbackCancel(listItem);
				  listItem = listItem->next_;
				  
			  }
		  }
	  } else {
	      sprintf(prnMsg, " No sessions merged. number = %d\n", noSessions);
	      printMsg(prnMsg,0);
      }
  } else {
	  sprintf(prnMsg, "Number of sessions = %d\n", noSessions);
	  printMsg(prnMsg,0);
  }

  double now = Scheduler::instance().clock();
  
  //check if some sessions need to be discarded because of rate-limiting too many sessions
  RateLimitSession * listItem1 = pbq->rlsList_->first_;
  while (noSessions > MAX_SESSIONS && listItem1 != NULL) {
      int rank = pbq->rlsList_->rankRate(node_->nodeid(), listItem1->getArrivalRateForStatus());
      if (listItem1->origin_ == node_->nodeid() && 
	  rank >= MAX_SESSIONS && (now - listItem1->startTime_) >= EARLIEST_TIME_TO_FREE) {
	  sprintf(prnMsg,"Releasing because of too many being rate-limited\n");
	  printMsg(prnMsg,0);
	  if (LOWER_BOUND_MODE == 1 && 
	      queue_list_[qid].idTree_->lowerBound_ < listItem1->getArrivalRateForStatus()) {
	      queue_list_[qid].idTree_->lowerBound_ = listItem1->getArrivalRateForStatus();
	  }
	  pushbackCancel(listItem1);
	  noSessions--;
      }
      listItem1 = listItem1->next_;
  }
  
  double linkBW = pbq->getBW();
  double arrRate = pbq->getRate();
  double targetRate = linkBW/(1 - TARGET_DROPRATE);
  
  double totalRateLimitedArrivalRate = 0;
  double totalLimit=0;
  double lowerBound=-1;
  RateLimitSession * listItem = pbq->rlsList_->first_;
  while (listItem != NULL) {
    if (listItem->origin_ == node_->nodeid() && !listItem->merged_) {
      double sessionArrRate = listItem->getArrivalRateForStatus();
      double sessionLimit = listItem->rlStrategy_->target_rate_;
      totalRateLimitedArrivalRate+= sessionArrRate;
      totalLimit+= (sessionArrRate > sessionLimit)? sessionLimit: sessionArrRate;
      if (listItem->lowerBound_ < lowerBound || lowerBound == -1) {
		  lowerBound = listItem->lowerBound_;
      }
    }
    listItem = listItem->next_;
  }

  if (LOWER_BOUND_MODE == 1) {
	  lowerBound = queue_list_[qid].idTree_->lowerBound_;
  }

  double excessRate = (arrRate - totalLimit + totalRateLimitedArrivalRate) - targetRate;
  
  sprintf(prnMsg,"arr=%g totalLimit=%g totalRateLimit=%g excess=%g\n",  arrRate, totalLimit, 
	  totalRateLimitedArrivalRate, excessRate);
  printMsg(prnMsg,0);
  
  if (excessRate < 0) {
	  sprintf(prnMsg, "Negative Excess Rate. Things maybe fine now.\n");
	  printMsg(prnMsg,0);
	  //this would make all sessions go away after a while.
#ifdef DEBUG
	  printf("Negative Excess Rate - time: %5.3f\n", now);
#endif
	  requiredLimit_ = 2*totalRateLimitedArrivalRate;
  } else {
	  //Should we allow such an abrupt increase when the number of sessions 
	  // changes?
	  // How about: Let L be the requiredLimit.
	  // We need Sum (session arrival rate - L ) = excessRate
	  requiredLimit_ = (totalRateLimitedArrivalRate - excessRate)/noSessions;
	  if (requiredLimit_ < lowerBound) {
		  requiredLimit_ = lowerBound;
	  }
#ifdef DEBUG
      printf("New requiredLimit - time: %5.3f limit: %5.3f lowerBound:%5.3f \n", now, requiredLimit_, lowerBound);
#endif
  }

  sprintf(prnMsg,"Refresh. target=%g limit=%g floor=%g\n", targetRate, requiredLimit_,
	  lowerBound);
  printMsg(prnMsg,0);

  //consider all sessions in ascending order of their arrival rate
  for (int i=0; i<noSessions; i++) {
	  listItem = pbq->rlsList_->first_;
	  while (listItem != NULL ) {
		  if (listItem->origin_ == node_->nodeid() && 
			  pbq->rlsList_->rankSession(node_->nodeid(),listItem) == i) 
			  break;
		 listItem = listItem->next_;
	  }
	  if (listItem == NULL) {
		  printf("Error: Rank %d not found\n", i);
		  exit(0);
	  }
	  
	  double oldLimit = listItem->rlStrategy_->target_rate_;
	  double sendRate = listItem->getArrivalRateForStatus();
#ifdef DEBUG
	  printf("time: %5.3f ID: %d sendRate %5.3f oldLimit %5.3f requiredLimit %5.3f\n", now,
			 listItem->localID_, sendRate, oldLimit, requiredLimit_);
#endif
	  //Session sending less than the limit.
	  if (sendRate < requiredLimit_) {
		  //if it has been sending less for "some" time.
		  if (now - listItem->refreshTime_ >= MIN_TIME_TO_FREE) {
#ifdef DEBUG
			printf("time: %5.3f ID: %d refreshTime %5.3f MIN %d Cancel pushback B \n", 
				   now, listItem->localID_, listItem->refreshTime_, MIN_TIME_TO_FREE);
#endif
			pushbackCancel(listItem);       //cancel rate-limiting
			requiredLimit_+= (requiredLimit_ - sendRate)/(noSessions - i - 1);
			i--; noSessions--;
		  } 
		  else {
			  //refresh upstream with double of max(sending rate, old limit)
			  //just using sending rate, limits the amount an aggregate can grow till next refresh
			  //using just old limit is tricky when different aggregates have different limits.
			  //at the same time, we would prefer not to loosen the hold too much in one step.
#ifdef DEBUG
			  printf("time: %5.3f ID: %d double limit\n", now, listItem->localID_);
#endif
			  double maxR = sendRate>oldLimit? sendRate: oldLimit;
			  if (now - listItem->refreshTime_ <= PRIMARY_WAITING_ZONE) {
				  sprintf(prnMsg,"Waiting Zone 1: sendRate=%g oldLimit=%g\n", sendRate, oldLimit);
				  printMsg(prnMsg,0);
			  }
			  else {
				  sprintf(prnMsg,"Waiting Zone 2: sendRate=%g oldLimit=%g\n", sendRate, oldLimit);
				  printMsg(prnMsg,0);
				  maxR *= 1.5;
			  }
			  if (maxR < requiredLimit_) {
				  listItem->setLimit(maxR);
				  if ((noSessions - i - 1) > 0 ) {
				  	requiredLimit_ += (requiredLimit_ - maxR)/(noSessions - i - 1);
				  }
			  } 
			  else {
				  listItem->setLimit(requiredLimit_);
			  }
			  
			  if (listItem->pushbackON_) 
				  refreshUpstreamLimits(listItem);
		  }
	  }
      else {
		  //change the rate limit most half way.
		  double newLimit;
		  if (oldLimit > 1.25 * requiredLimit_ || oldLimit ==0) 
			  newLimit = requiredLimit_;
		  else 
			  newLimit = 0.5*requiredLimit_ + 0.5*oldLimit;
		  
		  if (newLimit < lowerBound) 
			  newLimit = lowerBound;
		  
		  listItem->refreshed();
		  listItem->setLimit(newLimit);
		  if (listItem->pushbackON_) 
			  refreshUpstreamLimits(listItem);
#ifdef DEBUG
		  printf("time: %5.3f ID: %d newLimit %5.3f oldLimit %5.3f requiredLimit %5.3f\n", 
				 now, listItem->localID_, newLimit, oldLimit, requiredLimit_);
#endif
      }
  }    
  
  //setup refresh timer again
  noSessions = pbq->rlsList_->noMySessions(node_->nodeid());
  if (noSessions) {
    PushbackEvent * event = new PushbackEvent(PUSHBACK_CYCLE_TIME, PUSHBACK_REFRESH_EVENT, qid);
    timer_->insert(event);
  }
}


void
PushbackAgent::pushbackCancel(RateLimitSession * rls) {
 
  sprintf(prnMsg,"Stopping rate-limiting for aggregate: ");
  printMsg(prnMsg,0);
  rls->aggSpec_->print();
  fflush(stdout);

  #ifdef DEBUG
    double now = Scheduler::instance().clock();
    printf("time: %5.3f ID: %d Cancel pushback C\n", now, rls->localID_);
  #endif

  if (rls->pushbackON_) {
    LoggingDataStructNode * lgdsNode = rls->logData_->first_;
    while (lgdsNode != NULL) {
      PushbackMessage * msg = new PushbackCancelMessage(node_->nodeid(), lgdsNode->nid_, 
							rls->localQID_, rls->localID_);
      sendMsg(msg);
      lgdsNode = lgdsNode->next_;
    }
  }
  
  //remove all events that point to this rls.
  timer_->removeEvents(rls);
  //local cancellation here.
  queue_list_[rls->localQID_].pbq_->rlsList_->endSession(rls);
  
}

//######################## Message Receiving Code #####################

void 
PushbackAgent::recv(Packet * pkt, Handler * ) {
  
  hdr_pushback * hdr_push = ((hdr_pushback*)pkt)->access(pkt);
  PushbackMessage * msg = hdr_push->msg_;
  
  sprintf(prnMsg, " %s msg from %d\n",  PushbackMessage::type(msg), msg->senderID_);
  printMsg(prnMsg,0);
	 
  switch (msg->msgID_) {
  case PUSHBACK_REQUEST_MSG : processPushbackRequest((PushbackRequestMessage *)msg);
    break;
  case PUSHBACK_STATUS_MSG : processPushbackStatus((PushbackStatusMessage *) msg);
    break;
  case PUSHBACK_REFRESH_MSG : processPushbackRefresh((PushbackRefreshMessage *) msg);
    break;
  case PUSHBACK_CANCEL_MSG : processPushbackCancel((PushbackCancelMessage *) msg);
    break;
  default: fprintf(stderr,"PBA: %s Undefined Message ID %d\n", name(),msg->msgID_);
  } 
  
  delete(msg);
}

void 
PushbackAgent::processPushbackRequest(PushbackRequestMessage * msg) {
  
  int qid = getQID(msg->senderID_);
  sprintf(prnMsg, " pushback request from %d for qid=%d limit=%g\n", msg->senderID_, 
	 qid, msg->limit_);
  printMsg(prnMsg,0);
 
  AggSpec * aggSpec = msg->aggSpec_;
  if (queue_list_[qid].pbq_->rlsList_->containsAggSpec(aggSpec)) {
	  fprintf(stdout,"PBA: %s got a pushback req for agg I already rate-limit. \
Feature not yet Implemented\n",name()); 
	  exit(-1);
  }
  
  RateLimitSession * rls = new RateLimitSession(aggSpec, msg->limit_, msg->senderID_, qid, 
						msg->qid_, msg->rlsID_, msg->depth_+1, 
						RATE_LIMIT_TIME_DEFAULT, -1, node_, rtLogic_);
  queue_list_[qid].pbq_->rlsList_->insert(rls);

  //pushback propagation check if there are valid upstream neighbors && enable_pushback_
  if (rls->logData_->count_ && enable_pushback_) {
    PushbackEvent * event = new PushbackEvent(PUSHBACK_CHECK_TIME, PUSHBACK_CHECK_EVENT, rls);
    timer_->insert(event);
  }
}


void 
PushbackAgent::processPushbackStatus(PushbackStatusMessage * msg) {

  int qid = msg->qid_;

  if (!checkQID(qid)) {
    sprintf(prnMsg, " Got invalid qid from %d in status message\n",  msg->senderID_);
    printMsg(prnMsg,0);
    exit(-1);
  }

  RateLimitSession * rls = queue_list_[qid].pbq_->rlsList_->getSessionByLocalID(msg->rlsID_);
  
  if (rls == NULL) {
    sprintf(prnMsg, " session %d not found\n",  msg->rlsID_);
    printMsg(prnMsg,0);
    exit(-1);
  }
  
  //increase your height if you need to.
  if (msg->height_ + 1 > rls->heightInPTree_) {
    rls->heightInPTree_ = msg->height_ + 1;
    sprintf(prnMsg, " height increased to %d\n",  rls->heightInPTree_);
    printMsg(prnMsg,0);
  }

  rls->logData_->registerStatus(msg->senderID_, msg->arrivalRate_);
  sprintf(prnMsg, " got rate %g\n",  msg->arrivalRate_);
  printMsg(prnMsg,0);

  //send status if you are not root.
  if (rls->origin_!= node_->nodeid()) {
    // 1. check to see if status from all the upstream neighbors has arrived.
    // 2. if yes, then send status downstream.
    int gotAll = rls->logData_->consolidateStatus();
    if (gotAll==1) {
      //send status down
      double rate = rls->logData_->statusArrivalRateAll_;
      PushbackMessage * msg = new PushbackStatusMessage(node_->nodeid(), rls->origin_, 
							rls->remoteQID_, rls->remoteID_, 
						      rate, rls->heightInPTree_);
      sendMsg(msg);
      timer_->cancelStatus(rls);
      //reset status arrivals.
      rls->logData_->resetStatus();
    }
  }
}

void
PushbackAgent::processPushbackRefresh(PushbackRefreshMessage *msg) {

  int qid = getQID(msg->senderID_);
  sprintf(prnMsg, " pushback refresh from %d for qid=%d with limit=%g\n",  msg->senderID_, qid, msg->limit_);
  printMsg(prnMsg,0);

  RateLimitSession * rls = queue_list_[qid].pbq_->rlsList_->getSessionByRemoteID(msg->rlsID_);
  
  if (rls == NULL) {
    sprintf(prnMsg, " session %d not found\n",  msg->rlsID_);
    printMsg(prnMsg,0);
    exit(-1);
  }

  //1. change your own rate limit
  rls->setAggSpec(msg->aggSpec_);
  delete(msg->aggSpec_);
  double newLimit = msg->limit_;
  rls->setLimit(newLimit);

  //2. if pushback has been propagated send out refreshes upstream with new limits
  if (rls->pushbackON_) {
    refreshUpstreamLimits(rls);
  }
  
  //3. set up status timer.
  PushbackEvent * event = new PushbackEvent(PUSHBACK_CYCLE_TIME - 0.1*rls->depthInPTree_, 
					     PUSHBACK_STATUS_EVENT, rls);
  timer_->insert(event);
}

void 
PushbackAgent::processPushbackCancel(PushbackCancelMessage *msg) {
 
  int qid = getQID(msg->senderID_);
  sprintf(prnMsg, " pushback cancel from %d for queue index %d\n",  msg->senderID_, qid);
  printMsg(prnMsg,0);

  RateLimitSession * rls = queue_list_[qid].pbq_->rlsList_->getSessionByRemoteID(msg->rlsID_);
  
  if (rls == NULL) {
    sprintf(prnMsg, " session %d not found\n",  msg->rlsID_);
    printMsg(prnMsg,0);
    exit(-1);
  }
  pushbackCancel(rls);

}

void 
PushbackAgent::refreshUpstreamLimits(RateLimitSession * rls) {

  double totalRate =  rls->rlStrategy_->target_rate_;
  int count = rls->logData_->count_;
  double fairShare = totalRate/count;
  int done = count;
  double arrRate = rls->getArrivalRateForStatus();
  sprintf(prnMsg, "Sending refresh messages to %d nodes. Limit = %g arrRate = %g\n", count, totalRate, arrRate);
  printMsg(prnMsg,0); 
  
  int excess = 0;
  if (totalRate > arrRate) {
	  excess = 1;
  }

  //max-min allocation of limit.
  while (done != 0) {
    LoggingDataStructNode * lgdsNode = rls->logData_->first_;
    int countThisRound=0;
    while (lgdsNode != NULL) {
      double rate;
      rate = lgdsNode->statusArrivalRate_;
      if (rate <= fairShare && !lgdsNode->sentRefresh_) {
	AggSpec * aggSpec = rls->aggSpec_->clone();
	PushbackMessage * msg;
	if (rate < fairShare/2.0) {
	  msg = new PushbackRefreshMessage(node_->nodeid(), lgdsNode->nid_, rls->localQID_, 
					   rls->localID_, aggSpec, INFINITE_LIMIT);
	  lgdsNode->sentRefresh(INFINITE_LIMIT);
	}
	else if (!excess) {
	  msg = new PushbackRefreshMessage(node_->nodeid(), lgdsNode->nid_, rls->localQID_, 
					   rls->localID_, aggSpec, rate);
	  lgdsNode->sentRefresh(rate);
	}
	else {
	  msg = new PushbackRefreshMessage(node_->nodeid(), lgdsNode->nid_, rls->localQID_, 
					   rls->localID_, aggSpec, fairShare);
	  lgdsNode->sentRefresh(fairShare);
	  rate = fairShare;
	}
	sendMsg(msg);
	countThisRound++;
	done--;
	totalRate -= rate;
      }
      lgdsNode = lgdsNode->next_;
    }
    if (done == 0) break;
    if (countThisRound==0) {
      //allocate fairshare to everyone and end.
      LoggingDataStructNode * lgdsNode = rls->logData_->first_;
      while (lgdsNode != NULL) {
	if (!lgdsNode->sentRefresh_) {
	  AggSpec * aggSpec = rls->aggSpec_->clone();
	  PushbackMessage * msg = new PushbackRefreshMessage(node_->nodeid(), lgdsNode->nid_, 
							     rls->localQID_, rls->localID_, 
							     aggSpec, fairShare);
	    lgdsNode->sentRefresh(fairShare);
	    sendMsg(msg);
	    done--;
	    totalRate-=fairShare;
	}
	lgdsNode = lgdsNode->next_;
      }
    }
    else {
      fairShare = totalRate/done;
    }
  }
  
  //reset all the sentRefresh bits
  LoggingDataStructNode * lgdsNode = rls->logData_->first_;
  while (lgdsNode != NULL) {
    lgdsNode->sentRefresh_ = 0;
    lgdsNode = lgdsNode->next_;
  }

}

int
PushbackAgent::getQID(int sender) {
  
  Tcl& tcl = Tcl::instance();
  intResult_ = -1;
  int index = 0;
  // there gotta be better ways of doing this;  todoLater.
  // like make Tcl call you back and set a variable using command.
  for (; index <last_index_; index++) {
    tcl.evalf("%s set intResult_ [%s check-queue %d %d %s]", name(), name(), 
	      node_->nodeid(), sender , queue_list_[index].pbq_->name());
    if (intResult_ == 1) break;
  }
  
  if (index == last_index_) {
    sprintf(prnMsg, " right queue not found\n");
    printMsg(prnMsg,0);
    exit(-1);
  }

  return index;
}

void
PushbackAgent::sendMsg(PushbackMessage * msg) {
  
  Tcl& tcl = Tcl::instance();
  
  dst_.addr_ = msg->targetID_;
  //this assumes that all pushback agents have port zero.
  
  tcl.evalf("%s set intResult_ [%s get-pba-port %d]", name(), name(),dst_.addr_ );

  if ( intResult_ == -1 ) {
    fprintf(stderr,"PBA: %s Pushback Agent not found on Node %d\n", name(), dst_.addr_);
    return;
  }
  dst_.port_ = intResult_;
  Packet *pkt = allocpkt();
  hdr_pushback * hdr_push = ((hdr_pushback*)pkt)->access(pkt);
  hdr_push->msg_ = msg;
    
  sprintf(prnMsg, " sent %s message to %d.%d\n", PushbackMessage::type(msg), dst_.addr_, dst_.port_);
  printMsg(prnMsg,4);
  send(pkt,0);
}

void 
PushbackAgent::printMsg(char * msg, int msgLevel) {
  
  if (msgLevel < debugLevel) {
    if (verbose_) printf("PBA:%d (%g) %s", node_->nodeid(), Scheduler::instance().clock(), msg);
    fflush(stdout);
  }
}

int 
PushbackAgent::checkQID(int qid) {
  if (qid < 0 || qid >= last_index_) 
    return 0;
  else 
    return 1;
}

//decide whether to accept a merger involving "count" aggregates, 
//the number of bits in the resultant aggregate would be "bits"
//the aggregate is being broadended by "bitsDiff" (measured from shortest prefix)
int
PushbackAgent::mergerAccept(int , int , int ) {
  
  //todo: think of a smarter way.
  //currently merge if bits < some value.
  //return (bits <= MIN_BITS_FOR_MERGER);

  return 0;
}
  
// ############################### PushbackTimer Methods ############################

void 
PushbackTimer::expire(Event *) {
 
  if (firstEvent_ == NULL) {
    printf("PushbackTimer: No event found on expiry\n");
    exit(-1);
  }
  
  PushbackEvent * event = firstEvent_;
  firstEvent_= firstEvent_->next_;
  schedule();

  agent_->timeout(event);
  delete(event);
}
 

void
PushbackTimer::insert(PushbackEvent * event) {

  sprintf(agent_->prnMsg,"%s timer set\n", PushbackEvent::type(event));
  agent_->printMsg(agent_->prnMsg,4);
  if (firstEvent_ == NULL) {
    firstEvent_ = event;
    schedule();
    return;
  }

  if (event->time_ < firstEvent_->time_) {
    event->setSucc(firstEvent_);
    firstEvent_=event;
    schedule();
    return;
  }

  PushbackEvent * listItem = firstEvent_;
  while (listItem->next_!=NULL && listItem->next_->time_ <= event->time_) {
    listItem = listItem->next_;
  }
  
  event->setSucc(listItem->next_);
  listItem->setSucc(event);
 
  //comment the sanity check out later
  sanityCheck();
  
  return;
}

void
PushbackTimer::removeEvents(RateLimitSession * rls) {
  if (firstEvent_==NULL) return;
  while (firstEvent_!= NULL && firstEvent_->rls_==rls) {
    cancel();
    PushbackEvent * event = firstEvent_;
    firstEvent_=firstEvent_->next_;
    delete(event);
    schedule();
  }
  if (firstEvent_==NULL) return;

  PushbackEvent * previous = firstEvent_;
  PushbackEvent * current = firstEvent_->next_;
  while (current!=NULL) {
    if (current->rls_==rls) {
      previous->next_=current->next_;
      delete(current);
      current = previous->next_;
      continue;
    }
    previous=current;
    current=current->next_;
  }

}
      
void 
PushbackTimer::schedule() {

  if (firstEvent_== NULL) {
    sprintf(agent_->prnMsg,"Timer: Nothing to schedule\n");
    agent_->printMsg(agent_->prnMsg, 0);
    return;
  }

  resched(firstEvent_->time_ - Scheduler::instance().clock());
}

void 
PushbackTimer::cancelStatus(RateLimitSession * rls) {
  
  if (firstEvent_==NULL) {
    sprintf(agent_->prnMsg, " Error timer list empty\n");
    agent_->printMsg(agent_->prnMsg, 0);
    //return; 
    exit(-1);
  }
  
  if (firstEvent_->eventID_==PUSHBACK_STATUS_EVENT && firstEvent_->rls_==rls) {
    cancel();
    PushbackEvent * event = firstEvent_;
    firstEvent_=firstEvent_->next_;
    delete(event);
    schedule();
    return;
  }
  
  PushbackEvent * previous = firstEvent_;
  PushbackEvent * current = firstEvent_->next_;
  
  while (current!=NULL) {
    if (current->eventID_ == PUSHBACK_STATUS_EVENT && current->rls_==rls) {
      previous->next_=current->next_;
      delete(current);
      return;
    } 
    previous=current;
    current=current->next_;
  }

  sprintf(agent_->prnMsg, "Error status timer not found\n");
  agent_->printMsg(agent_->prnMsg, 0);
  exit(-1);
}

int 
PushbackTimer::containsRefresh(int qid) {
  PushbackEvent * listItem = firstEvent_;
  while (listItem!=NULL) {
    if (listItem->eventID_ == PUSHBACK_REFRESH_EVENT && listItem->qid_==qid) 
      return 1;
    listItem = listItem->next_;
  }
  return 0;
}

void 
PushbackTimer::sanityCheck() {

  if (firstEvent_==NULL || firstEvent_->next_ == NULL) return;
  
  PushbackEvent * listItem = firstEvent_;
  while (listItem->next_!=NULL) {
    if (listItem->time_ > listItem->next_->time_) {
      sprintf(agent_->prnMsg, "Sanity Check Failed\n");
      agent_->printMsg(agent_->prnMsg, 0);
      exit(-1);
    }
    listItem = listItem->next_;
  }

}
  
