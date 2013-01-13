
/*
 * ls.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: ls.cc,v 1.6 2009/12/30 22:06:34 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
//  Copyright (C) 1998 by Mingzhou Sun. All rights reserved.
//  This software is developed at Rensselaer Polytechnic Institute under 
//  DARPA grant No. F30602-97-C-0274
//  Redistribution and use in source and binary forms are permitted
//  provided that the above copyright notice and this paragraph are
//  duplicated in all such forms and that any documentation, advertising
//  materials, and other materials related to such distribution and use
//  acknowledge that the software was developed by Mingzhou Sun at the
//  Rensselaer  Polytechnic Institute.  The name of the University may not 
//  be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
// $Header: /cvsroot/nsnam/ns-2/linkstate/ls.cc,v 1.6 2009/12/30 22:06:34 tom_henderson Exp $

#include "config.h"
#ifdef HAVE_STL

#include "ls.h"

// a global variable
LsMessageCenter LsMessageCenter::msgctr_;

int LsRouting::msgSizes[LS_MESSAGE_TYPES];

static class initRouting {
public:
	initRouting() {
		LsRouting::msgSizes[LS_MSG_LSA] = LS_LSA_MESSAGE_SIZE;
		LsRouting::msgSizes[LS_MSG_TPM] =  LS_TOPO_MESSAGE_SIZE;
		LsRouting::msgSizes[LS_MSG_LSAACK] = LS_ACK_MESSAGE_SIZE;
		LsRouting::msgSizes[LS_MSG_TPMACK] = LS_ACK_MESSAGE_SIZE;
	}
} lsRoutingInitializer;

static void ls_error(char* msg) 
{ 
	fprintf(stderr, "%s\n", msg);
	abort();
}

/*
  LsNodeIdList methods
*/
int LsNodeIdList::appendUnique (const LsNodeIdList & x) 
{
	int newHopCount = 0;
	for (LsNodeIdList::const_iterator itr1 = x.begin(); 
	     itr1 != x.end(); itr1++) {
		LsNodeIdList::iterator itr2;
		for (itr2 = begin(); itr2 != end(); itr2++)
			// check for duplicate
			if ((*itr1) == (*itr2))
				break;
		if (itr2 == end()) {
			// no duplicate, insert it 
			push_back(*itr1);
			newHopCount++; // forgot what newHopCount is used for
		}
	}
	return newHopCount;
}

/*
  LsTopoMap methods
*/
LsLinkStateList * 
LsTopoMap::insertLinkState (int nodeId, const LsLinkState& linkState)
{
	LsLinkStateList* lsp = LsLinkStateListMap::findPtr(nodeId);
	if (lsp != NULL) {
		// there's a node with other linkState, not checking if there's
		// duplicate
		lsp->push_back(linkState);
		return lsp;
	}
	// else new node
	LsLinkStateList lsl; // an empty one
	iterator itr = insert(nodeId, lsl);
	if (itr !=end()){
		// successful
		(*itr).second.push_back(linkState);
		return &((*itr).second);
	}
	// else something is wrong
	ls_error("LsTopoMap::insertLinkState failed\n"); // debug
	return (LsLinkStateList *) NULL;
}

// -- update --, return true if anything's changed 
bool LsTopoMap::update(int nodeId, 
		       const LsLinkStateList& linkStateList)
{
	LsLinkStateList * LSLptr = findPtr (nodeId);
	if (LSLptr == NULL) {
		insert(nodeId, linkStateList);
		return true;
	}

	bool retCode = false;
	LsLinkStateList::iterator itrOld;
	for (LsLinkStateList::const_iterator itrNew = linkStateList.begin();
	     itrNew != linkStateList.end(); itrNew++ ) {
  
		for (itrOld = LSLptr->begin();
		     itrOld != LSLptr->end(); itrOld++) {

			if ((*itrNew).neighborId_ == (*itrOld).neighborId_) {
				// old link state found
				if (nodeId != myNodeId_)
					// update the sequence number, if 
					// it's not  my own link state
					(*itrOld).sequenceNumber_ = 
						(*itrNew).sequenceNumber_;
				if ((*itrNew).status_ != (*itrOld).status_) {
					(*itrOld).status_ = (*itrNew).status_;
					retCode = true;
				}
				if ((*itrNew).cost_ != (*itrOld).cost_) {
					(*itrOld).cost_ = (*itrNew).cost_;
					retCode = true;
				}
				break; // no need to search for more old link state;
			} // end if old link state found
		} // end for old link states
		if (itrOld == LSLptr->end()) {
			// no old link found
			LSLptr->push_back(*itrNew);
			retCode = true;
		}
	}// end for new link states 
	return retCode;
}

/*
  LsPaths methods
*/

// insertPath(), returns end() if error, else return iterator 
LsPaths::iterator LsPaths::insertPath(int destId, int cost, int nextHop) 
{
	iterator itr = LsEqualPathsMap::find(destId);
	// if new path, insert it and return iterator
	if (itr == end())
		return insertPathNoChecking(destId, cost, nextHop);

	LsEqualPaths * ptrEP = &(*itr).second;
	// if the old path is better, ignore it, return end() 
	// to flag the error
	if (ptrEP->cost < cost)
		return end(); 
	// else if the new path is better, erase the old ones and save the new one
	LsNodeIdList & nhList = ptrEP->nextHopList;
	if (ptrEP->cost > cost) {
		ptrEP->cost = cost;
		nhList.erase(nhList.begin(), nhList.end());
		nhList.push_back(nextHop);
		return itr;
	}
	// else the old path is the same cost, check for duplicate
	for (LsNodeIdList::iterator itrList = nhList.begin();
	     itrList != nhList.end(); itrList++)
		// if duplicate found, return 0 to flag the error 
		if ((*itrList) == nextHop)
			return end(); 
	// else, the new path is installed indeed, the total number of nextHops
	// is returned. 
	nhList.push_back(nextHop);
	return itr;
}

LsPaths::iterator 
LsPaths::insertNextHopList(int destId, int cost, 
			   const LsNodeIdList& nextHopList) 
{
	iterator itr = LsEqualPathsMap::find(destId);
	// if new path, insert it and return iterator 
	if (itr == end()) {
		LsEqualPaths ep(cost, nextHopList);
		itr = insert(destId, ep);
		return itr;
	}
	// else get the old ep
	LsEqualPaths* ptrOldEp = &(*itr).second;
	// if the old path is better, ignore it, return end() to flag error
	if (ptrOldEp->cost < cost)
		return end();
	// else if the new path is better, replace the old one with the new one
	if (ptrOldEp->cost > cost) {
		ptrOldEp->cost = cost;
		ptrOldEp->nextHopList = nextHopList ; // copy
		return itr;
	}
	// equal cost: append the new next hops with checking for duplicates
	ptrOldEp->appendNextHopList(nextHopList);
	return itr;
}

/*
  LsPathsTentative methods
*/
LsPath LsPathsTentative::popShortestPath() 
{
	findMinEqualPaths();
	LsPath path;
	if (empty() || (minCostIterator == end()))
		return path; // an invalid one

	LsNodeIdList& nhList = (*minCostIterator).second.nextHopList;
	if (nhList.empty() && (findMinEqualPaths() == end()))
		return path;
	path.destination = (*minCostIterator).first;
	path.cost=(*minCostIterator).second.cost;

	// the first 'nextHop'
	path.nextHop = nhList.front();
	nhList.pop_front();

	// if this pops out the last nextHop in the EqualPaths, find a new one.
	if (nhList.empty()) {
		erase(minCostIterator);
		findMinEqualPaths();
	}

	return path;
}

LsPathsTentative::iterator LsPathsTentative::findMinEqualPaths()
{
	minCost = LS_MAX_COST + 1; 
	minCostIterator = end();
	for (iterator itr = begin(); itr != end(); itr++){
		if ((minCost > (*itr).second.cost) && 
		    !(*itr).second.nextHopList.empty()) {
			minCost = (*itr).second.cost;
			minCostIterator = itr;
		}
	}
	return minCostIterator;
}

/*
  LsMessageCenter methods
*/

LsMessage* LsMessageCenter::newMessage(int nodeId, ls_message_type_t type) 
{
	// check if max_message_number is invalid, call init ()
	if (max_size == 0)
		init(); 

	// if max_size reached, recycle
	message_storage* storagePtr;
	u_int32_t currentId;

	// current_lsa_id and current_other_id are odd and even, respectively
	if ((type == LS_MSG_LSA) || (type == LS_MSG_TPM)) { 
		storagePtr = & lsa_messages;
		currentId = current_lsa_id;
		if ((current_lsa_id += 2) == LS_INVALID_MESSAGE_ID)  
			current_lsa_id += 2;
	} else {
		storagePtr = &other_messages;
		currentId = current_other_id;
		if ((current_other_id += 2) == LS_INVALID_MESSAGE_ID)  
			current_other_id +=2;
	}

	if (storagePtr->size() >= max_size)
		storagePtr->erase(storagePtr->begin());

	LsMessage* msgPtr = (LsMessage *)NULL;
	message_storage::iterator itr = 
		storagePtr->insert(currentId, 
				   LsMessage(currentId, nodeId, type));
	if (itr == storagePtr->end())
		ls_error("Can't insert new message in "
			 "LsMessageCenter::newMessage.\n");
	else
		msgPtr = &((*itr).second);
	return msgPtr;
}

bool LsMessageCenter::deleteMessage(u_int32_t msgId)
{
	message_storage::iterator itr = other_messages.find (msgId); 
	if (itr == other_messages.end())
		return false;
	other_messages.erase(itr);
	return true;
}

// init()
void LsMessageCenter::init() 
{
	// only when nothing is provided by tcl code
	max_size = 300;
}

/*
  LsMessageHistory methods 
*/

// TODO, rewrite this method for topo message
bool LsMessageHistory::isNewMessage ( const LsMessage& msg ) 
{
	iterator itr = find(msg.originNodeId_);
	if (itr != end()) {
		if (((*itr).second < msg.sequenceNumber_) || 
		    ((*itr).second - msg.sequenceNumber_ > 
		     LS_WRAPAROUND_THRESHOLD)) {
			// The new message is more up-to-date than the old one
			(*itr).second = msg.sequenceNumber_;
			return true;
		} else {
			// We had a more up-to-date or same message from 
			// this node before
			return false;
		}
	} else {
		// We never received message from this node before 
		insert(msg.originNodeId_, msg.sequenceNumber_);
		return true;
	}
 
}

/* LsRetransmissionManager Methods */

void LsRetransmissionManager::initTimeout(LsDelayMap * delayMapPtr) 
{
	if (delayMapPtr == NULL)
		return;
	for (LsDelayMap::iterator itr = delayMapPtr->begin();
	     itr != delayMapPtr->end(); itr++)
		// timeout is LS_TIMEOUT_FACTOR*one-way-delay estimate
		insert((*itr).first, 
		       LsUnackPeer(this, (*itr).first, 
				   LS_TIMEOUT_FACTOR*(*itr).second));
}

void LsRetransmissionManager::cancelTimer (int nbrId) 
{
	LsUnackPeer* peerPtr = findPtr(nbrId);
	if (peerPtr == NULL) 
		return;
	peerPtr->tpmSeq_ = LS_INVALID_MESSAGE_ID;
	peerPtr->lsaMap_.eraseAll();
	peerPtr->timer_.force_cancel();
}

// Called by LsRouting when a message is sent out
int LsRetransmissionManager::messageOut(int peerId, 
					const LsMessage& msg)
{
	LsUnackPeer* peerPtr = findPtr(peerId);
	if (peerPtr == NULL) {
		iterator itr = insert(peerId, LsUnackPeer(this, peerId));
		if (itr == end()) { 
			ls_error ("Can't insert."); 
		}
		peerPtr = &((*itr).second);
	}
	LsIdSeq* idSeqPtr;
	switch (msg.type_) {
	case LS_MSG_TPM:
		peerPtr->tpmSeq_ = msg.sequenceNumber_;
		break;
	case LS_MSG_LSA:
		idSeqPtr =  peerPtr->lsaMap_.findPtr(msg.originNodeId_);
		if (idSeqPtr == NULL)
			peerPtr->lsaMap_.insert(msg.originNodeId_, 
				 LsIdSeq(msg.messageId_, msg.sequenceNumber_));
		else {
			idSeqPtr->msgId_ = msg.messageId_;
			idSeqPtr->seq_ = msg.sequenceNumber_;
		}
		break;
	case LS_MSG_TPMACK:
	case LS_MSG_LSAACK:
	case LS_MSG_LSM:
	default:
		// nothing, just to avoid compiler warning
		break;
	}
 
	// reschedule timer to allow account for this latest message
	peerPtr->timer_.resched(peerPtr->rtxTimeout_);
  
	return 0;
}
  
// Called by LsRouting,  when an ack is received
// Or by messageIn, some the message can serve as ack
int LsRetransmissionManager::ackIn(int peerId, const LsMessage& msg)
{
	LsUnackPeer* peerPtr = findPtr(peerId);
	if ((peerPtr == NULL) ||
	    ((peerPtr->tpmSeq_ == LS_INVALID_MESSAGE_ID) &&
	    peerPtr->lsaMap_.empty()))
		// no pending ack for this neighbor 
		return 0;

	LsMap<int, LsIdSeq>::iterator itr;
	switch (msg.type_) {
	case LS_MSG_TPMACK:
		if (peerPtr->tpmSeq_ == msg.sequenceNumber_)
			// We've got the right ack, so erase the unack record
			peerPtr->tpmSeq_ = LS_INVALID_MESSAGE_ID;
		break;
	case LS_MSG_LSAACK:
		itr = peerPtr->lsaMap_.find(msg.originNodeId_);
		if ((itr != peerPtr->lsaMap_.end()) && 
		    ((*itr).second.seq_ == msg.sequenceNumber_)) 
			// We've got the right ack, so erase the unack record
			peerPtr->lsaMap_.erase(itr);
		break;
	case LS_MSG_TPM:
	case LS_MSG_LSA: 
	case LS_MSG_LSM:
	default:
		break;
	}

	if ((peerPtr->tpmSeq_ == LS_INVALID_MESSAGE_ID) &&
	    (peerPtr->lsaMap_.empty()))
		// No more pending ack, cancel timer
		peerPtr->timer_.cancel();

	// ack deleted in calling function LsRouting::receiveMessage
	return 0;
}

// resendMessage
int LsRetransmissionManager::resendMessages (int peerId) 
{
	LsUnackPeer* peerPtr = findPtr (peerId);
	if (peerPtr == NULL) 
		// That's funny, We should never get in here
		ls_error ("Wait a minute, nothing to send for this neighbor");

	// resend topo map
	if (peerPtr->tpmSeq_ != LS_INVALID_MESSAGE_ID) 
		lsRouting_.resendMessage(peerId, peerPtr->tpmSeq_, LS_MSG_TPM);
  
	// resend all other unack'ed LSA
	for (LsMap<int, LsIdSeq>::iterator itr = peerPtr->lsaMap_.begin();
	     itr != peerPtr->lsaMap_.end(); ++itr)
		lsRouting_.resendMessage(peerId, (*itr).second.msgId_, 
					 LS_MSG_LSA);

	// reschedule retransmit timer
	peerPtr->timer_.resched(peerPtr->rtxTimeout_);
	return 0;
} 

/*
  LsRouting methods
*/

bool LsRouting::init(LsNode * nodePtr) 
{
	if (nodePtr == NULL) 
		return false;

	myNodePtr_ = nodePtr;
	myNodeId_ = myNodePtr_->getNodeId();
	linkStateDatabase_.setNodeId(myNodeId_);
	peerIdListPtr_ = myNodePtr_->getPeerIdListPtr();
	linkStateListPtr_ = myNodePtr_->getLinkStateListPtr();
	if (linkStateListPtr_ != NULL) 
		linkStateDatabase_.insert(myNodeId_, *linkStateListPtr_);

	LsDelayMap* delayMapPtr = myNodePtr_->getDelayMapPtr();
	if (delayMapPtr != NULL)
		ackManager_.initTimeout(delayMapPtr) ;
	return true;
}  

void LsRouting::linkStateChanged () 
{
	if (linkStateListPtr_ == NULL)
		ls_error("LsRouting::linkStateChanged: linkStateListPtr null\n");
   
	LsLinkStateList* oldLsPtr = linkStateDatabase_.findPtr(myNodeId_);
	if (oldLsPtr == NULL) 
		// Should never happen,something's wrong, we didn't 
		// initialize properly
		ls_error("LsRouting::linkStateChanged: oldLsPtr null!!\n");

	// save the old link state for processing
	LsLinkStateList oldlsl(*oldLsPtr);

	// if there's any changes, compute new routes and send link states
	// Note: we do want to send link states before topo
	bool changed=linkStateDatabase_.update(myNodeId_, *linkStateListPtr_);
	if (changed) {
		computeRoutes();
		sendLinkStates(/* buffer before sending */ true); 
		// tcl code will call sendBufferedMessage
	}

	// Check if there's need to send topo or cancel timer
	LsLinkStateList::iterator oldLsItr = oldlsl.begin();
	for (LsLinkStateList::iterator newLsItr = linkStateListPtr_->begin();
	      newLsItr != linkStateListPtr_->end();
	      newLsItr++, oldLsItr++) {
		// here we are assuming the two link state list are the same 
		// except the status and costs, buggy    
		if ((*newLsItr).neighborId_ != (*oldLsItr).neighborId_)
			// something's wrong
			ls_error("New and old link State list are not aligned.\n");
		// if link goes down, clear neighbor's not ack'ed entry
		if ((*newLsItr).status_ == LS_STATUS_DOWN)
			ackManager_.cancelTimer((*newLsItr).neighborId_);

		if (((*newLsItr).status_ == LS_STATUS_DOWN) || 
		    ((*oldLsItr).status_ == LS_STATUS_UP))
			// Don't have to send out tpm if the link goes from 
			// up to down, or it was originally up
			continue;
		// else we have to set out the whole topology map that we have 
		// to our neighbor that just resumes peering with us
		// the messages are buffered, will flush the buffer after the 
		// routes are installed in the node
		// But don't worry, only a const ptr to our topo map is sent
		sendTopo( (*newLsItr).neighborId_);
	}
}

/* -- isUp -- */
// a linear search, but not too bad if most nodes will have 
// less than a couple of interfaces
bool LsRouting::isUp(int neighborId) 
{
	if (linkStateListPtr_ == NULL) 
		return false;

	for (LsLinkStateList::iterator itr = linkStateListPtr_->begin();
	     itr!= linkStateListPtr_->end(); itr++)
		if ((*itr).neighborId_ == neighborId)
			return ((*itr).status_ == LS_STATUS_UP) ? true : false;
	return false;
}

/* -- receiveMessage -- */
/* return true if there's a need to re-compute routes */
bool LsRouting::receiveMessage (int senderId, u_int32_t msgId)
{
	if (senderId == LS_INVALID_NODE_ID)
		return false;
	LsMessage* msgPtr = msgctr().retrieveMessagePtr(msgId);
	if (msgPtr == NULL)
		return false;

	// A switch statement to see the type.
	// and handle differently
	bool retCode = false;
	switch (msgPtr->type_){
	case LS_MSG_LSM:
		// not supported yet
		break;
	case LS_MSG_TPM:
		retCode = receiveTopo (senderId, msgPtr);
		break;
	case LS_MSG_LSA:  // Link State Update 
		retCode = receiveLSA (senderId, msgPtr);
		break;
	case LS_MSG_LSAACK: 
	case LS_MSG_TPMACK: 
		receiveAck(senderId, msgPtr);
		msgctr().deleteMessage(msgId);
		break;
	default:
		break;
	}
	return retCode;
}

// LsRouting::receiveAck is in-line in header file
bool LsRouting::receiveLSA(int senderId, LsMessage* msgPtr)
{
	if (msgPtr == NULL) 
		return false;
	u_int32_t msgId = msgPtr->messageId_;
	int originNodeId = msgPtr->originNodeId_;

	sendAck(senderId, LS_MSG_LSAACK, originNodeId, msgId);
       
	if ((originNodeId == myNodeId_) ||
	     !lsaHistory_.isNewMessage(*msgPtr)) 
		return false; 

	// looks like we've got a new message LSA
	int peerId;
	if ((peerIdListPtr_ != NULL) && (myNodePtr_ != NULL)) {
		// forwards to peers whose links are up, except the sender 
		// and the originator 
		for (LsNodeIdList::iterator itr = peerIdListPtr_->begin();
		     itr != peerIdListPtr_->end(); itr++) {
			peerId = *itr;
			if ((peerId == originNodeId) || (peerId == senderId))
				continue; 
			if (isUp(peerId) && ((peerId) != senderId)) {
				ackManager_.messageOut(peerId, *msgPtr);
				myNodePtr_->sendMessage(peerId, msgId);
			}
		}
	}

	// Get the content of the message 
	if (msgPtr->contentPtr_ == NULL) 
		return false;
	bool changed = linkStateDatabase_.update(msgPtr->originNodeId_,
						 *(msgPtr->lslPtr_));
	if (changed)
		// linkstate database has changed, re-compute routes
		computeRoutes();
	return changed;
}


// -- sendLinkStates --
bool LsRouting::sendLinkStates(bool buffer /* = false */) 
{
	if (myNodePtr_ == NULL)
		return false;
	if ((peerIdListPtr_ == NULL) || peerIdListPtr_->empty())
		return false;

	LsLinkStateList* myLSLptr = linkStateDatabase_.findPtr(myNodeId_);
	if ((myLSLptr == NULL) || myLSLptr->empty())
		return false;

	LsMessage* msgPtr = msgctr().newMessage(myNodeId_, LS_MSG_LSA);
	if (msgPtr == NULL) 
		return false; // can't get new message

	u_int32_t msgId = msgPtr->messageId_;
	u_int32_t seq = msgPtr->sequenceNumber_;
	// update the sequence number in my own data base
	for (LsLinkStateList::iterator itr = myLSLptr->begin();
	     itr != myLSLptr->end(); itr++)
		(*itr).sequenceNumber_ = seq;
  
	LsLinkStateList* newLSLptr = new LsLinkStateList( *myLSLptr);
	if (newLSLptr == NULL) {
		ls_error ("Can't get new link state list, in LsRouting::sendLinkStates\n");
		// can't get new link state list
		msgctr().deleteMessage(msgId);
		return false;
	}

	msgPtr->lslPtr_ = newLSLptr;
	for (LsNodeIdList::iterator itr = peerIdListPtr_->begin();
	     itr != peerIdListPtr_->end(); itr++) {
		if (!isUp(*itr))
			continue; // link is down 
		if (!buffer) {
			myNodePtr_->sendMessage((*itr), msgId);
			ackManager_.messageOut((*itr), *msgPtr);
		} else {
			// put in buffer for later sending
			bufferedSend((*itr), msgPtr);
		}
	}
	return true;
}

// send acknowledgment, called self
bool LsRouting::sendAck (int nbrId, ls_message_type_t type, 
			 int originNodeIdAcked, u_int32_t seqAcked) 
{
	// Get a new message fom messageCenter
	LsMessage * msgPtr = msgctr().newMessage(myNodeId_, type);
	if (msgPtr == NULL)
		return false; // can't get new message

	u_int32_t msgId = msgPtr->messageId_;
	// fill in the blank
	// msgPtr->type = type;
	msgPtr->originNodeId_ = originNodeIdAcked;
	msgPtr->sequenceNumber_ = seqAcked;

	// Call node to send out message
	myNodePtr_->sendMessage (nbrId, msgId, LS_ACK_MESSAGE_SIZE);
	return true;
}

// After a link comes up, receive Topology update from the 
// corresponding neighbor
bool LsRouting::receiveTopo(int neighborId, LsMessage * msgPtr)
{
	// TODO
	bool changed = false;
	// send Ack 
	sendAck(neighborId, LS_MSG_TPMACK, neighborId, 
		msgPtr->sequenceNumber_);
	// check if it's a new message
	if (!tpmHistory_.isNewMessage(*msgPtr))
		return false;

	if (msgPtr->topoPtr_ == NULL)
		return false;
	// Compare with my own database
	for (LsTopoMap::const_iterator recItr = msgPtr->topoPtr_->begin();
	     recItr != msgPtr->topoPtr_->end(); recItr++) {

		if ((*recItr).first == myNodeId_)
			// Don't need peer to tell me my own link state 
			continue; 
		// find my own record of the LSA of the node being examined
		LsLinkStateList* myRecord = 
			linkStateDatabase_.findPtr((*recItr).first);

		if ((myRecord == NULL) || // we don't have it
		    myRecord->empty() ||
		    // or we have an older record
		    ((*(myRecord->begin())).sequenceNumber_ <
		     (*((*recItr).second.begin())).sequenceNumber_) ||
		    ((*(myRecord->begin())).sequenceNumber_ - 
		     (*((*recItr).second.begin())).sequenceNumber_ > 
		     LS_WRAPAROUND_THRESHOLD)) {

			// Update our database
			changed = true;
			if (myRecord == NULL)
				linkStateDatabase_.insert((*recItr).first, 
							 (*recItr).second);
			else 
				*myRecord = (*recItr).second ;
			// Regenerate the LSA message and send to my peers, 
			// except the sender of the topo and the 
			// originator of the LSA
			regenAndSend (/* to except */neighborId, 
				      /* originator */(*recItr).first, 
				      /* the linkstateList */(*recItr).second);
		}
	}
	if (changed)
		computeRoutes();
	// if anything relevant has changed, recompute routes
	return changed;
}

// replicate a LSA and send it out
void LsRouting::regenAndSend(int exception, int origin, 
			     const LsLinkStateList& lsl)
{
	if (lsl.empty())
		ls_error("lsl is empty, in LsRouting::regenAndSend.\n");
	LsLinkStateList* newLSLptr = new LsLinkStateList(lsl);
	if (newLSLptr == NULL) { 
		ls_error("Can't get new link state list, in "
			 "LsRouting::sendLinkStates\n");
		return;
	}

	// replicate the LSA 
	LsMessage* msgPtr =  msgctr().newMessage(origin, LS_MSG_LSA);
	msgPtr->sequenceNumber_ = (*lsl.begin()).sequenceNumber_;
	msgPtr->originNodeId_ = origin;

	for (LsNodeIdList::iterator itr = peerIdListPtr_->begin();
	     itr != peerIdListPtr_->end(); itr++) {
		if ((*itr == origin) || (*itr == exception) || !isUp(*itr))
			continue; 
		bufferedSend(*itr, msgPtr);
		// debug 
//  		cout << "Node " << myNodeId << " regenAndSend " 
//  		     << (*(lsl.begin())).sequenceNumber << " from origin  " << origin 
//  		     << " to peer " << *itr << endl;
	}
}
		       
// After a link comes up, receive Topo with the corresponding neighbor
void LsRouting::sendTopo(int neighborId) 
{
	// if we've gone so far, messageCenterPtr should not be null, 
	// don't check
	LsMessage* msgPtr= msgctr().newMessage(myNodeId_, LS_MSG_TPM);
	// XXX, here we are going to send the pointer that points
	// to my own topo, because sending the who topomap is too costly in
	// simulation
	msgPtr->contentPtr_ = &linkStateDatabase_;
	bufferedSend(neighborId, msgPtr);
}

void LsRouting::sendBufferedMessages()
{
	for (MessageBuffer::iterator itr = messageBuffer_.begin();
	     itr != messageBuffer_.end(); itr++) {
		ackManager_.messageOut((*itr).peerId_, *((*itr).msgPtr_));
		myNodePtr_->sendMessage((*itr).peerId_, 
					(*itr).msgPtr_->messageId_,
					msgSizes[(*itr).msgPtr_->type_]);
	}
	messageBuffer_.eraseAll();
}

// private _computeRoutes, called by public computeRoutes
LsPaths* LsRouting::_computeRoutes () 
{
	LsPathsTentative* pTentativePaths = new LsPathsTentative();
	LsPaths* pPaths = new LsPaths() ; // to be returned; 
 
	// step 1. put myself in path
	LsPath toSelf(myNodeId_, 0, myNodeId_); // zero cost, nextHop is myself
	pPaths->insertPathNoChecking(toSelf);
	int newNodeId = myNodeId_;
	LsLinkStateList * ptrLSL = linkStateDatabase_.findPtr(newNodeId);
	if (ptrLSL == NULL )
		// don't have my own linkState
		return pPaths;

	bool done = false;
	// start the loop
	while (! done) {
		// Step 2. for the new node just put in path
		// find the next hop to the new node
		LsNodeIdList nhl;
		LsNodeIdList *nhlp = &nhl; // nextHopeList pointer
    
		if (newNodeId != myNodeId_) {
			// if not looking at my own links, find the next hop 
			// to new node
			nhlp = pPaths->lookupNextHopListPtr(newNodeId);
			if (nhlp == NULL)
				ls_error("computeRoutes: nhlp == NULL \n");
		}
		// for each of it's links
		for (LsLinkStateList::iterator itrLink = ptrLSL->begin();
		     itrLink != ptrLSL->end(); itrLink++) {
			if ((*itrLink).status_ != LS_STATUS_UP)
				// link is not up, skip this link
				continue;
			int dest = (*itrLink).neighborId_;
			int path_cost = (*itrLink).cost_ + 
				pPaths->lookupCost(newNodeId);
			if (pPaths->lookupCost(dest) < path_cost)
				// better path already in paths, 
				// move on to next link
				continue;
			else {
				// else we have a new or equally good path, 
				// LsPathsTentative::insertPath(...) will 
				// take care of checking if the new path is
				// a better or equally good one, etc.
				LsNodeIdList nextHopList;
				if (newNodeId == myNodeId_) {
					// destination is directly connected, 
					// nextHop is itself
					nextHopList.push_back(dest);
					nhlp = &nextHopList;
				}
				pTentativePaths->insertNextHopList(dest, 
						   path_cost, *nhlp);
			}
		}
		done = true;
		// if tentatives empty, terminate;
		while (!pTentativePaths->empty()) {
			// else pop shortest path triple from tentatives
			LsPath sp = pTentativePaths->popShortestPath();
			if (!sp.isValid())
				ls_error (" popShortestPath() failed\n");
			// install the newly found shortest path among 
			// tentatives
			pPaths->insertPath(sp);
			newNodeId = sp.destination;
			ptrLSL = linkStateDatabase_.findPtr(newNodeId);
			if (ptrLSL != NULL) {
				// if we have the link state for the new node
				// break out of inner do loop to continue 
				// computing routes
				done = false;
				break;
			} 
			// else  we don't have linkstate for this new node, 
			// we need to keep popping shortest paths
		}

	} // endof while ( ! done );
	delete pTentativePaths;
	return pPaths;
}

#endif //HAVE_STL
