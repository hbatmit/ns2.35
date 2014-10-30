
/*
 * ls.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: ls.h,v 1.10 2010/03/08 05:54:51 tom_henderson Exp $
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
// $Header: /cvsroot/nsnam/ns-2/linkstate/ls.h,v 1.10 2010/03/08 05:54:51 tom_henderson Exp $

#ifndef ns_ls_h
#define ns_ls_h

#include <sys/types.h> 
#include <list>
#include <map>
#include <utility>

#include "timer-handler.h"

const int LS_INVALID_COUNT = -1;
const int LS_INIT_ACCESS_COUNT = 3;
const int LS_INVALID_NODE_ID = 65535;
const int LS_INVALID_COST = 65535;
const int LS_MIN_COST = 0;
const int LS_MAX_COST = 65534;
const int LS_MESSAGE_CENTER_SIZE_FACTOR = 4; // times the number of nodes 
const int LS_DEFAULT_MESSAGE_SIZE = 100; // in bytes
const int LS_LSA_MESSAGE_SIZE = 100; // in bytes
const double LS_RTX_TIMEOUT = 0.002;  // default to 2ms to begin with
const int LS_TIMEOUT_FACTOR = 3;   // x times of one-way total delay
// Topo message is not too long due to incremental update
const int LS_TOPO_MESSAGE_SIZE = 200; // in bytes
const int LS_ACK_MESSAGE_SIZE = 20; // in bytes
const unsigned int LS_INVALID_MESSAGE_ID = 0;
const unsigned int LS_BIG_NUMBER = 1048576;
const unsigned int LS_WRAPAROUND_THRESHOLD = 1073741824; // 2^30
const unsigned int LS_MESSAGE_TYPES = 6;

enum ls_status_t { 
	LS_STATUS_DOWN = 0, 
	LS_STATUS_UP = 1
};

enum ls_message_type_t { 
	LS_MSG_INVALID = 0, 
	LS_MSG_LSA = 1,		// Link state advertisement
	LS_MSG_TPM = 2,		// Topology map message
	LS_MSG_LSAACK = 3,	// Link state advertisement ACK
	LS_MSG_TPMACK = 4, 
	LS_MSG_LSM = 5
};

template <class _Tp>
class LsList : public list<_Tp> {
public:
	typedef list<_Tp> baseList;
	LsList() : baseList() {}
	LsList(const _Tp& x) : baseList(1, x) {}
	void eraseAll() { 
		baseList::erase(baseList::begin(), baseList::end()); 
	}
	LsList<_Tp>& operator= (const LsList<_Tp> & x) {
		return (LsList<_Tp> &)baseList::operator= (x);
	}
};

template<class Key, class T>
class LsMap : public map<Key, T, less<Key> > {
public:
	typedef less<Key> less_key;
	typedef map<Key, T, less_key> baseMap;
	LsMap() : baseMap() {}

	// this next typedef of iterator seems extraneous but is required by gcc-2.96
	typedef typename map<Key, T, less<Key> >::iterator iterator;
	typedef pair<iterator, bool> pair_iterator_bool;
	iterator insert(const Key & key, const T & item) {
		typename baseMap::value_type v(key, item);
		pair_iterator_bool ib = baseMap::insert(v);
		return ib.second ? ib.first : baseMap::end();
	}

	void eraseAll() { this->erase(baseMap::begin(), baseMap::end()); }
	T* findPtr(Key key) {
		iterator it = baseMap::find(key);
		return (it == baseMap::end()) ? (T *)NULL : &((*it).second);
	}
};

/*
  LsNodeIdList -- A list of int 's. It manages its own memory
*/
class LsNodeIdList : public LsList<int> {
public:
	int appendUnique (const LsNodeIdList& x);
};  

/* -------------------------------------------------------------------*/
/* 
   LsLinkState -- representing a link, contains neighborId, cost and status
*/
struct LsLinkState {
	// public data
	int neighborId_;  
	ls_status_t status_;
	int cost_;
	u_int32_t sequenceNumber_;  

	// public methods
	LsLinkState() : neighborId_(LS_INVALID_NODE_ID),
		status_(LS_STATUS_DOWN), cost_(LS_INVALID_COST) {}
	LsLinkState(int id, ls_status_t s, int c) : neighborId_(id), 
		status_(s), cost_(c) {}

	void init (int nbId, ls_status_t s, int c){
		neighborId_ = nbId;
		status_ = s;
		cost_ =c;
	}
} ;

/* 
   LsLinkStateList 
*/
typedef LsList<LsLinkState> LsLinkStateList;

/* -------------------------------------------------------------------*/
/*
  LsTopoMap
  the Link State Database, the representation of the
  topology within the protocol 
*/
typedef LsMap<int, LsLinkStateList> LsLinkStateListMap;

class LsTopoMap : public LsLinkStateListMap {
public:
	// constructor / destructor 
	// the default ones 
	LsTopoMap() : LsLinkStateListMap() {}
  
	//   // map oeration
	//   iterator begin() { return LsLinkStateListMap::begin();}
	//   iterator end() { return LsLinkStateListMap::end();}
	//  iterator begin() { return baseMap::begin();}
	//   const_iterator begin() const { return  baseMap::begin();}
	//   iterator end() { return baseMap::end();}
	//   const_iterator end() const { return baseMap::end();}

	// insert one link state each time 
	LsLinkStateList* insertLinkState(int nodeId, 
					 const LsLinkState& linkState);
	// update returns true if there's change
	bool update(int nodeId, const LsLinkStateList& linkStateList);
	//   friend ostream & operator << ( ostream & os, LsTopoMap & x) ;
	void setNodeId(int id) { myNodeId_ = id ;}
private:
	int myNodeId_; // for update()
};

typedef LsTopoMap LsTopology;
typedef LsTopoMap* LsTopoMapPtr;

/*
  LsPath - A struct with destination, cost, nextHop
*/
struct LsPath {
	LsPath() : destination (LS_INVALID_NODE_ID) {}
	LsPath(int dest, int c, int nh)
		: destination (dest), cost(c), nextHop(nh) {}
	// methods
	bool isValid() { 
		return ((destination != LS_INVALID_NODE_ID) && 
			(cost != LS_INVALID_COST) && 
			(nextHop != LS_INVALID_COST));
	}

	// public data
	int destination;
	int cost;
	int nextHop;
};

/*
  LsEqualPaths -- A struct with one cost and a list of multiple next hops
  Used by LsPaths
*/
struct LsEqualPaths {
public:
	int  cost;
	LsNodeIdList nextHopList;

	// constructors
	LsEqualPaths() : cost(LS_INVALID_COST) {}
	LsEqualPaths(int c, int nh) : cost(c), nextHopList() {
		nextHopList.push_back(nh);
	}
	LsEqualPaths(const LsPath & path) : cost (path.cost), nextHopList() {
		nextHopList.push_back(path.nextHop);
	}
	LsEqualPaths(int c, const LsNodeIdList & nhList) 
		: cost(c), nextHopList(nhList) {}
  
	LsEqualPaths& operator = (const LsEqualPaths & x ) {
		cost = x.cost;
		nextHopList = x.nextHopList;
		return *this;
	}

	// copy 
	LsEqualPaths& copy(const LsEqualPaths & x) { return operator = (x) ;}

	// appendNextHopList 
	int appendNextHopList(const LsNodeIdList & nhl) {
		return nextHopList.appendUnique (nhl);
	}
};

/* 
   LsEqualPathsMap -- A map of LsEqualPaths
*/
typedef LsMap< int, LsEqualPaths > LsEqualPathsMap;

/*
  LsPaths -- enhanced LsEqualPathsMap, used in LsRouting
*/
class LsPaths : public LsEqualPathsMap {
public:
	LsPaths(): LsEqualPathsMap() {}
  
	// -- map operations 
	iterator begin() { return LsEqualPathsMap::begin();}
	iterator end() { return LsEqualPathsMap::end();}

	// -- specical methods that facilitates computeRoutes of LsRouting
	// lookupCost
	int lookupCost(int destId) {
		LsEqualPaths * pEP = findPtr (destId);
		if ( pEP == NULL ) return LS_MAX_COST + 1;
		// else
		return pEP->cost;
	}

	// lookupNextHopListPtr
	LsNodeIdList* lookupNextHopListPtr(int destId) {
		LsEqualPaths* pEP = findPtr(destId);
		if (pEP == NULL ) 
			return (LsNodeIdList *) NULL;
		// else
		return &(pEP->nextHopList);
	}
  
	// insertPath without checking validity
	iterator insertPathNoChecking(int destId, int cost, int nextHop) {
		LsEqualPaths ep(cost, nextHop);
		iterator itr = insert(destId, ep);
		return itr; // for clarity
	}     

	// insertPathNoChekcing()
	iterator insertPathNoChecking (const LsPath & path) {
		return insertPathNoChecking(path.destination, path.cost, 
					    path.nextHop);
	}
	// insertPath(), returns end() if error, else return iterator 
	iterator insertPath(int destId, int cost, int nextHop);
	iterator insertPath(const LsPath& path) {
		return insertPath(path.destination, path.cost, path.nextHop);
	}
	// insertNextHopList 
	iterator insertNextHopList(int destId, int cost, 
				   const LsNodeIdList& nextHopList);
};

/*
  LsPathsTentative -- Used in LsRouting, remembers min cost and location 
*/
class LsPathsTentative : public LsPaths {
public:
	LsPathsTentative() : LsPaths(), minCost(LS_MAX_COST+1) {
		minCostIterator = end();
	}
  
	// combining get and remove min path
	LsPath popShortestPath();
  
private:
	int minCost; // remembers the min cost
	iterator minCostIterator; // remembers where it's stored
	iterator findMinEqualPaths();
};

/* 
   LsMessage 
*/
struct LsMessage {
	LsMessage() : type_(LS_MSG_INVALID), contentPtr_(NULL) {}
	LsMessage(u_int32_t id, int nodeId, ls_message_type_t t) :
		type_(t), messageId_(id),
		sequenceNumber_(id), originNodeId_(nodeId), 
		contentPtr_(NULL) {}
	~LsMessage() {
		if ((type_ == LS_MSG_LSM) && (lslPtr_ != NULL)) {
			delete lslPtr_;
			lslPtr_ = NULL;
		}
	}
	ls_message_type_t type_;
	u_int32_t messageId_;
	u_int32_t sequenceNumber_; 
	int originNodeId_; 
	union {
		LsLinkStateList* lslPtr_;
		const LsTopoMap* topoPtr_;
		void* contentPtr_;
	};
};

// TODO -- not used, comment out
// Some time we just want the header, since the message has a content
// which will be destroyed with it goes out of scope, 
// used by ack manager
struct LsMessageInfo
{
	ls_message_type_t type_;
	int destId_; 
	u_int32_t msgId_;
	u_int32_t sequenceNumber_;    
	union {
		// for LSA, the originator of the msg
		int originNodeId_;  
		// for LSA_ACK, the originator of the lsa being acked
		int originNodeIdAck_; 
	};
	// constructor 
	LsMessageInfo() {}
	LsMessageInfo(int d, const LsMessage& msg ) :
		type_(msg.type_), destId_(d),  msgId_(msg.messageId_),
		sequenceNumber_(msg.sequenceNumber_),
		originNodeId_(msg.originNodeId_)  {}
	LsMessageInfo(int d , ls_message_type_t t, int o, 
		      u_int32_t seq, u_int32_t mId) :
		type_(t), destId_(d), msgId_(mId), sequenceNumber_(seq), 
		originNodeId_(o) {}
};

/* 
   LsMessageCenter -- Global storage of LsMessage's for retrieval
*/
class LsMessageCenter {
public:
	typedef LsMap <u_int32_t, LsMessage> baseMap;
	// constructor
	LsMessageCenter () 
		: current_lsa_id(LS_INVALID_MESSAGE_ID + 1), 
		current_other_id(LS_INVALID_MESSAGE_ID + 2),
		max_size(0), lsa_messages(), other_messages() {}
  
	void setNodeNumber (int number_of_nodes) {
		max_size = number_of_nodes * LS_MESSAGE_CENTER_SIZE_FACTOR;
	}
	LsMessage* newMessage (int senderNodeId, ls_message_type_t type);
	u_int32_t duplicateMessage( u_int32_t msgId) {
		return msgId;
	}
	u_int32_t duplicateMessage(const LsMessage& msg) {
		return duplicateMessage(msg.messageId_);
	}
	bool deleteMessage(u_int32_t msgId) ;
	bool deleteMessage (const LsMessage& msg) {
		return deleteMessage(msg.messageId_);
	}
	LsMessage* retrieveMessagePtr(u_int32_t msgId){
		if (isLSA(msgId)) {
			return lsa_messages.findPtr(msgId);
		} else {
			return other_messages.findPtr(msgId);
		}
	}
	static LsMessageCenter& instance() { 
		return msgctr_;
	}

private:
	static LsMessageCenter msgctr_;	// Singleton class

	u_int32_t current_lsa_id ;
	u_int32_t current_other_id;
	unsigned int max_size; // if size() greater than this number, erase begin().
	typedef LsMap <u_int32_t, LsMessage > message_storage;
	message_storage lsa_messages;
	message_storage other_messages;
	void init();
	int isLSA (u_int32_t msgId) {
		// to see if msgId's last bit is different from 
		// LS_INVALID_MESSAGE_ID
		return (0x1 & (msgId ^ LS_INVALID_MESSAGE_ID));
	}
};

/* 
   LsMessageHistory 
*/
typedef LsList<u_int32_t> LsMessageIdList;
typedef less<int> less_node_id;
class LsMessageHistory : public LsMap<int, u_int32_t> {
public:
	// isNewMessage, note: it saves this one in the history as well
	bool isNewMessage ( const LsMessage & msg ); 
};

class LsRetransmissionManager;
class LsRetransTimer : public TimerHandler {
public:
	LsRetransTimer() {}
	LsRetransTimer (LsRetransmissionManager *amp , int nbrId)
		: ackManagerPtr_(amp), neighborId_(nbrId) {}
	virtual void expire(Event *e);
protected:
	LsRetransmissionManager* ackManagerPtr_;
	int neighborId_;
};

struct LsIdSeq {
	u_int32_t msgId_;
	u_int32_t seq_;
	LsIdSeq() {}
	LsIdSeq(u_int32_t id, u_int32_t s) : msgId_(id), seq_(s) {}
};

/* LsUnackPeer 
   used in ackManager to keep record a peer who still haven't ack some of 
   its LSA or Topo packets
*/
struct LsUnackPeer {
	double rtxTimeout_; // time out value 
	LsRetransTimer timer_;
	u_int32_t tpmSeq_; // topo message Id

	LsMap<int, LsIdSeq> lsaMap_;

	// constructor
	LsUnackPeer() : tpmSeq_(LS_INVALID_MESSAGE_ID) {}
	LsUnackPeer(LsRetransmissionManager* amp, int nbrId, 
		    double timeout = LS_RTX_TIMEOUT) : 
		rtxTimeout_(timeout), timer_(amp, nbrId), 
		tpmSeq_(LS_INVALID_MESSAGE_ID) {}
};

/* 
   LsDelayMap
   store the estimated one-way total delay for eay neighbor, in second
*/
typedef LsMap< int, double > LsDelayMap; 

/* 
   LsRetransmissionManager -- handles retransmission and acknowledgement
*/
class LsRouting; 
class LsRetransmissionManager : public LsMap<int, LsUnackPeer> {
public:
	LsRetransmissionManager(LsRouting& lsr) : lsRouting_(lsr) {} 

	void initTimeout(LsDelayMap* delayMapPtr);
	void cancelTimer(int neighborId);

	// Called by LsRouting when a message is sent out 
	int messageOut(int peerId, const LsMessage& msg);
  
	// Called by LsRouting when an ack is received
	int ackIn(int peerId,  const LsMessage& ack);

	// Called by retransmit timer
	int resendMessages(int peerId);

private:
	// data
	LsRouting& lsRouting_;
};

inline void LsRetransTimer::expire(Event *)
{ 
	ackManagerPtr_->resendMessages(neighborId_); 
}
   
/* 
   LsNode -- represents the node environment interface 
   It serves as the interface between the Routing class and the actual 
   simulation enviroment 
   rtProtoLS will derive from LsNode as well as Agent
*/
class LsNode {
public:
        virtual ~LsNode () {}
	virtual bool sendMessage(int destId, u_int32_t msgId, 
				  int msgsz = LS_DEFAULT_MESSAGE_SIZE) = 0;
	virtual void receiveMessage(int sender, u_int32_t msgId) = 0;
	// TODO , maybe not, use one type of message, that's it. 
	// All go to message center.
	// sendAck 
	// receiveAck
	virtual int getNodeId() = 0;
	virtual LsLinkStateList* getLinkStateListPtr()= 0; 
	virtual LsNodeIdList* getPeerIdListPtr() = 0;
	virtual LsDelayMap* getDelayMapPtr() = 0;
};

/* 
   LsRouting -- The implementation of the Link State Routing protocol
*/
class LsRouting {
public:
	static int msgSizes[ LS_MESSAGE_TYPES ];
	friend class LsRetransmissionManager;

	// constructor and distructor
	LsRouting() : myNodePtr_(NULL),  myNodeId_(LS_INVALID_NODE_ID), 
		peerIdListPtr_(NULL), linkStateListPtr_(NULL),
		routingTablePtr_(NULL),
		linkStateDatabase_(), lsaHistory_(), ackManager_(*this) {}
	~LsRouting() {
		//delete pLinkStateDatabase;
		if (routingTablePtr_ != NULL)
			delete routingTablePtr_;
	}

	bool init(LsNode* nodePtr);
	void computeRoutes() {
	        if (routingTablePtr_ != NULL)
	                delete routingTablePtr_;
	        routingTablePtr_ = _computeRoutes();
	}
	LsEqualPaths* lookup(int destId) {
		return (routingTablePtr_ == NULL) ? 
			(LsEqualPaths *)NULL : 
			routingTablePtr_->findPtr(destId);
	}

	// to propogate LSA, all Links, called by node and self
	bool sendLinkStates(bool buffer = false); 
	void linkStateChanged();
	void sendBufferedMessages() ;

	// called by node when messages arrive
	bool receiveMessage(int senderId , u_int32_t msgId);

private:
	// most of these pointers should have been references, 
	// except routing table
	LsNode * myNodePtr_; // where I am residing in
	int myNodeId_; // who am I
	LsNodeIdList* peerIdListPtr_; // my peers
	LsLinkStateList* linkStateListPtr_; // My links
	LsMessageCenter* messageCenterPtr_; // points to static messageCenter
	LsPaths* routingTablePtr_; // the routing table
	LsTopoMap linkStateDatabase_; // topology;
	LsMessageHistory lsaHistory_; // Remember what we've seen
	LsMessageHistory tpmHistory_; 
	LsRetransmissionManager ackManager_; // Handles ack and retransmission

	struct IdMsgPtr {
		int peerId_;
		const LsMessage* msgPtr_;
		IdMsgPtr() {};
		IdMsgPtr(int id, const LsMessage* p) :
			peerId_(id), msgPtr_(p) {}
	};
	typedef LsList<IdMsgPtr> MessageBuffer;
	MessageBuffer messageBuffer_;

private:
	LsMessageCenter& msgctr() { return LsMessageCenter::instance(); }
	LsPaths* _computeRoutes();
	bool isUp(int neighborId);

	bool receiveAck (int neighborId, LsMessage* msgPtr) {
		ackManager_.ackIn(neighborId, *msgPtr);
		return true;
	}
	bool receiveLSA (int neighborId, LsMessage* msgPtr);
	bool receiveTopo(int neighborId, LsMessage* msgPtr);

	// send the entire topomap
	// don't worry, in simulation only the pointer is sent
	// in ospf, only the descrpition of it is sent first, 
	void sendTopo(int neighborId);
	void regenAndSend(int exception, int origin, 
			  const LsLinkStateList& lsl);
	bool sendAck(int nbrId, ls_message_type_t type, 
		     int originNodeIdAcked, u_int32_t originMsgIdAcked);
	void resendMessage(int neighborId, u_int32_t msgId, 
			    ls_message_type_t type) {
		myNodePtr_->sendMessage(neighborId, msgId, msgSizes[type]);
	}
	// just store the outgoing messages, and wait for cmd flushBuffer to 
	// actually send out 
	void bufferedSend (int peerId, const LsMessage* mp) {
		messageBuffer_.push_back(IdMsgPtr(peerId, mp));
	}
};

#endif // ns_ls_h
