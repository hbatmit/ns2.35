/*******************************************************************************
 *                                                                             *
 *   M-DART Routing Protocol                                                   *
 *                                                                             *
 *   Copyright (C) 2006 by Marcello Caleffi                                    *
 *   marcello.caleffi@unina.it                                                 *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright notice,   *
 *    this list of conditions and the following disclaimer.                    *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. The name of the author may not be used to endorse or promote products    *
 *    derived from this software without specific prior written permission.    *
 *                                                                             *
 * This software is provided by the author ``as is'' and any express or        *
 * implied warranties, including, but not limited to, the implied warranties   *
 * of merchantability and fitness for a particular purpose are disclaimed.     *
 * in no event shall the author be liable for any direct, indirect,            *
 * incidental, special, exemplary, or consequential damages (including, but    *
 * not limited to, procurement of substitute goods or services; loss of use,   *
 * data, or profits; or business interruption) however caused and on any       *
 * theory of liability, whether in contract, strict liability, or tort         *
 * (including negligence or otherwise) arising in any way out of the use of    *
 * this software, even if advised of the possibility of such damage.           *
 *                                                                             *
 * The M-DART code has been developed by Marcello Caleffi during his Ph.D. at  *
 * the Department of Biomedical, Electronic and Telecommunications Engineering *
 * University of Naples Federico II, Italy.                                    *
 *                                                                             *
 * In order to give credit and recognition to the author, if you use M-DART    *
 * results or results obtained by modificating the M-DART source code, please  *
 * cite one of the following papers:                                           *
 * - M. Caleffi, L. Paura, "M-DART: Multi-Path Dynamic Address RouTing",       *
 *   Wireless Communications and Mobile Computing, 2010                        *
 * - M. Caleffi, G. Ferraiuolo, L. Paura, "Augmented Tree-based Routing        *
 *   Protocol for Scalable Ad Hoc Networks", Proc. of IEEE MASS '07: IEEE      *
 *   Internatonal Conference on Mobile Adhoc and Sensor Systems, Pisa (Italy), *
 *   October 8-11 2007.                                                        *
 *                                                                             *
 ******************************************************************************/



#include <mdart/mdart_adp.h>
#include <mdart/mdart_dht.h>
#include <mdart/mdart.h>
#include <mdart/mdart_function.h>



//------------------------------------------------------------------------------
// Address Discovery Protocol (ADP)
//------------------------------------------------------------------------------
ADP::ADP(MDART* mdart) {
	mdart_ = mdart;
	timer_ = new ADPDaupTimer(this);
	timerRx_ = new ADPDarqTimer(this);
	dht_ = new DHT();
	pckSeqNum_ = 0;
 }

 ADP::~ADP() {
	delete timer_;
	delete dht_;
}


 
//------------------------------------------------------------------------------
// ADP network address function
//------------------------------------------------------------------------------
void ADP::updateAdd() {
	addEntry(mdart_->id_, mdart_->address_);
#ifdef ADP_DABR_ENABLE 
	if (DHTSize() > 0) {
		sendDabr();
	}
#endif
}



//------------------------------------------------------------------------------
// ADP message functions
//------------------------------------------------------------------------------
void ADP::checkDarq() {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tMDART::checkDarq()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	for (int i=0; i<mdart_->queue_->size(); i++) {
		if (mdart_->queue_->isExpired(i)) {
			Packet *p = mdart_->queue_->getPacket(i);
#ifdef DEBUG_ADP
			fprintf(stdout, "\tre-sending darq for node %d\n", HDR_IP(p)->daddr());
#endif
			sendDarq(HDR_IP(p)->daddr(), HDR_CMN(p)->uid());			
		}
	}
}

void ADP::sendDarq(nsaddr_t reqId, int reqpktId) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tADP::sendDarq(%d)\t\t\tin node %d\twith address %s\n", CURRENT_TIME, reqId, mdart_->id_, bitString(mdart_->address_));
#endif
	nsaddr_t dstAdd_ = hash(reqId);
#ifdef DEBUG_ADP
	fprintf(stdout, "\tsending darq for node %s\n", bitString(dstAdd_));
#endif
	if (dstAdd_ == mdart_->address_ ) {
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDARQ DROP_RTR_ADPMISS\n");
#endif
		return;
	}
	if (mdart_->routingTable_->DAGetEntry(dstAdd_) == (nsaddr_t)IP_BROADCAST) {
		bitset<ADDR_SIZE> oldAdd_ (dstAdd_);
#ifdef DEBUG_ADP
		fprintf(stdout, "\told dstAdd_ = %s\n", bitString(dstAdd_));
#endif
		int i = 0;
		while ((mdart_->routingTable_->DAGetEntry(dstAdd_) == (nsaddr_t)IP_BROADCAST) && (i < ADDR_SIZE)) {
			oldAdd_.reset(i);
			dstAdd_ = (nsaddr_t) oldAdd_.to_ulong();
//			fprintf(stdout, "\ttemp dstAdd_ = %s\n", bitString(dstAdd_));
			i++;
		}
#ifdef DEBUG_ADP
		fprintf(stdout, "\tnew dstAdd_ = %s\n", bitString(dstAdd_));
#endif
		if (mdart_->routingTable_->DAGetEntry(dstAdd_) == (nsaddr_t)IP_BROADCAST) {
#ifdef DEBUG_ADP
			fprintf(stdout, "\tDARQ DROP_RTR_ADPNRT\n");
#endif
			return;
		}
	}
	nsaddr_t nextHop_ = mdart_->routingTable_->DAGetEntry(dstAdd_);
#ifdef DEBUG_ADP
	fprintf(stdout, "\tselected nextHop %d\n", nextHop_);
#endif
	Packet *sendPkt_ = Packet::alloc();
	struct hdr_cmn *sendPktCh_ = HDR_CMN(sendPkt_);
	struct hdr_ip *sendPktIh_ = HDR_IP(sendPkt_);
	struct hdr_mdart_darq *sendPktRh_ = HDR_MDART_DARQ(sendPkt_);
	
	sendPktCh_->ptype() = PT_MDART;
	sendPktCh_->size() = IP_HDR_LEN + mdart_->size();
	sendPktCh_->iface() = -2;
	sendPktCh_->error() = 0;
	sendPktCh_->addr_type() = NS_AF_NONE;
	sendPktCh_->next_hop() = nextHop_;		
	sendPktCh_->ts_ = CURRENT_TIME;		
		
	sendPktIh_->saddr() = mdart_->id_;
	sendPktIh_->daddr() = nextHop_;
	sendPktIh_->sport() = RT_PORT;
	sendPktIh_->dport() = RT_PORT;
	sendPktIh_->ttl_ = IP_DEF_TTL;		
	
	sendPktRh_->type_ = MDART_TYPE_DARQ;
	sendPktRh_->srcId_ = mdart_->id_;
	sendPktRh_->srcAdd_ = mdart_->address_;
	sendPktRh_->forId_ = mdart_->id_;
	sendPktRh_->forAdd_ = mdart_->address_;
	sendPktRh_->dstId_ = findId(dstAdd_);
	sendPktRh_->dstAdd_ = dstAdd_;
	sendPktRh_->lifetime_ = ADP_DARQ_TIMEOUT;
	sendPktRh_->reqId_ = reqId;
	sendPktRh_->seqNum_ = pckSeqNum();
#ifdef DEBUG_ADP
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", sendPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", sendPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcId_ = %d\n", sendPktRh_->srcId_);
	fprintf(stdout, "\tsendPktRh_->srcAdd_ = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->forId_ = %d\n", sendPktRh_->forId_);
	fprintf(stdout, "\tsendPktRh_->forAdd_ = %s\n", bitString(sendPktRh_->forAdd_));
	fprintf(stdout, "\tsendPktRh_->dstId_ = %d\n", sendPktRh_->dstId_);
	fprintf(stdout, "\tsendPktRh_->dstAdd_ = %s\n", bitString(sendPktRh_->dstAdd_));
	fprintf(stdout, "\tsendPktRh_->reqId_ = %d\n", sendPktRh_->reqId_);
	fprintf(stdout, "\tsendPktRh_->seqNum_ = %d\n", sendPktRh_->seqNum_);
#endif
	Scheduler::instance().schedule(mdart_->getTarget(), sendPkt_, 0.0);
}

void ADP::recvDarq(Packet* recvPkt) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tADP::recvDarq()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	struct hdr_cmn *recvPktCh_ = HDR_CMN(recvPkt);
	struct hdr_ip *recvPktIh_ = HDR_IP(recvPkt);
	struct hdr_mdart_darq *recvPktRh_ = HDR_MDART_DARQ(recvPkt);
#ifdef DEBUG_ADP
	fprintf(stdout, "\treceveing darq for node %s\n", bitString(recvPktRh_->dstAdd_));
	mdart_->routingTable_->print();
#endif
#ifdef ADP_DAXX_CACHING
	addEntry(recvPktRh_->srcId_, recvPktRh_->srcAdd_);
	addEntry(recvPktRh_->forId_, recvPktRh_->forAdd_);
#endif
	nsaddr_t findAdd_ = findAdd(recvPktRh_->reqId_);
	bool changeAdd_ = false;		
	if (findAdd_ != (nsaddr_t) IP_BROADCAST) {
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDARQ received\n");
#endif
		sendDarp(recvPkt);
		return;
	}	
	if (recvPktRh_->dstAdd_ == mdart_->address_) {
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDARQ DROP_RTR_ADPMISS\n");
#endif
		mdart_->drop(recvPkt, "DROP_RTR_ADPMISS");
		return;
	}
	if (mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == (nsaddr_t)IP_BROADCAST) {
		changeAdd_ = true;	
		bitset<ADDR_SIZE> oldAdd_ (recvPktRh_->dstAdd_);
#ifdef DEBUG_ADP
		fprintf(stdout, "\told dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
#endif
		int i = 0;
		while (((mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == recvPktRh_->forId_) || (mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == (nsaddr_t)IP_BROADCAST)) && (i < ADDR_SIZE)) {
			oldAdd_.reset(i);
			recvPktRh_->dstAdd_ = (nsaddr_t) oldAdd_.to_ulong();
			i++;
		}
#ifdef DEBUG_ADP
		fprintf(stdout, "\tnew dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
#endif
		if (mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == (nsaddr_t)IP_BROADCAST) {
#ifdef DEBUG_ADP
			fprintf(stdout, "\tDARQ DROP_RTR_ADPNRT\n");
#endif
			mdart_->drop(recvPkt, "DROP_RTR_ADPNRT");
			return;
		}
	}
	nsaddr_t nextHop_ = mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_);
	if (nextHop_ == recvPktRh_->forId_ && !changeAdd_) {
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDARQ DROP_RTR_ADPLOOP\n");
#endif
		mdart_->drop(recvPkt, "DROP_RTR_ADPNRT");
		return;
	}
#ifdef DEBUG_ADP
	fprintf(stdout, "\t\tselected nextHop_ = %d\n", nextHop_);
#endif
	recvPktCh_->direction() = hdr_cmn::DOWN;
	recvPktCh_->next_hop() = nextHop_;		
	recvPktIh_->daddr() = nextHop_;
	recvPktRh_->forId_ = mdart_->id_;
	recvPktRh_->forAdd_ = mdart_->address_;
	recvPktRh_->dstId_ = findId(recvPktRh_->dstAdd_);
#ifdef DEBUG_ADP
	fprintf(stdout, "\trecvPktIh_->saddr = %d\n", recvPktIh_->saddr());
	fprintf(stdout, "\trecvPktIh_->daddr = %d\n", recvPktIh_->daddr());
	fprintf(stdout, "\trecvPktRh_->srcId_ = %d\n", recvPktRh_->srcId_);
	fprintf(stdout, "\trecvPktRh_->srcAdd_ = %s\n", bitString(recvPktRh_->srcAdd_));
	fprintf(stdout, "\trecvPktRh_->forId_ = %d\n", recvPktRh_->forId_);
	fprintf(stdout, "\trecvPktRh_->forAdd_ = %s\n", bitString(recvPktRh_->forAdd_));
	fprintf(stdout, "\trecvPktRh_->dstId_ = %d\n", recvPktRh_->dstId_);
	fprintf(stdout, "\trecvPktRh_->dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
	fprintf(stdout, "\trecvPktRh_->reqId_ = %d\n", recvPktRh_->reqId_);
#endif		
	Scheduler::instance().schedule(mdart_->getTarget(), recvPkt, 0.0);
}



void ADP::sendDarp(Packet* recvPkt) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tMDART::sendDarp()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	struct hdr_mdart_darq *recvPktRh_ = HDR_MDART_DARQ(recvPkt);
	nsaddr_t nextHop_ = mdart_->routingTable_->getEntry(recvPktRh_->srcAdd_);
#ifdef DEBUG_ADP
	fprintf(stdout, "\t\tselected nextHop_ = %d\n", nextHop_);
#endif
	Packet *sendPkt_ = Packet::alloc();
	struct hdr_cmn *sendPktCh_ = HDR_CMN(sendPkt_);
	struct hdr_ip *sendPktIh_ = HDR_IP(sendPkt_);
	struct hdr_mdart_darp *sendPktRh_ = HDR_MDART_DARP(sendPkt_);
	sendPktCh_->ptype() = PT_MDART;
	sendPktCh_->size() = IP_HDR_LEN + mdart_->size();
	sendPktCh_->iface() = -2;
	sendPktCh_->error() = 0;
	sendPktCh_->addr_type() = NS_AF_NONE;
//	sendPktCh_->next_hop() = recvPktRh_->forId_;
	sendPktCh_->next_hop() = nextHop_;
	
	sendPktIh_->saddr() = mdart_->id_;
//	sendPktIh_->daddr() = recvPktRh_->forId_;
	sendPktIh_->daddr() = nextHop_;
	sendPktIh_->sport() = RT_PORT;
	sendPktIh_->dport() = RT_PORT;
	sendPktIh_->ttl_ = IP_DEF_TTL;		
	
	sendPktRh_->type_ = MDART_TYPE_DARP;
	sendPktRh_->srcId_ = mdart_->id_;
	sendPktRh_->srcAdd_ = mdart_->address_;
	sendPktRh_->forId_ = mdart_->id_;
	sendPktRh_->forAdd_ = mdart_->address_;
	sendPktRh_->dstId_ = recvPktRh_->srcId_;
	sendPktRh_->dstAdd_ = recvPktRh_->srcAdd_;
	sendPktRh_->reqId_ = recvPktRh_->reqId_;
	sendPktRh_->reqAdd_ = findAdd(recvPktRh_->reqId_);
	sendPktRh_->lifetime_ = ADP_DARP_TIMEOUT;
	sendPktRh_->seqNum_ = pckSeqNum();
#ifdef DEBUG_ADP
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", sendPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", sendPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcId = %d\n", sendPktRh_->srcId_);
	fprintf(stdout, "\tsendPktRh_->srcAdd = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->forId = %d\n", sendPktRh_->forId_);
	fprintf(stdout, "\tsendPktRh_->forAdd = %s\n", bitString(sendPktRh_->forAdd_));
	fprintf(stdout, "\tsendPktRh_->dstId = %d\n", sendPktRh_->dstId_);
	fprintf(stdout, "\tsendPktRh_->dstAdd = %s\n", bitString(sendPktRh_->dstAdd_));
	fprintf(stdout, "\tsendPktRh_->reqId = %d\n", recvPktRh_->reqId_);
	fprintf(stdout, "\tsendPktRh_->reqAdd = %s\n", bitString(sendPktRh_->reqAdd_));
#endif
	Packet::free(recvPkt);
	Scheduler::instance().schedule(mdart_->getTarget(), sendPkt_, 0.0);
}

void ADP::recvDarp(Packet* recvPkt) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tMDART::recvDarp()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	struct hdr_cmn *recvPktCh_ = HDR_CMN(recvPkt);
	struct hdr_ip *recvPktIh_ = HDR_IP(recvPkt);
	struct hdr_mdart_darp* recvPktRh_ = HDR_MDART_DARP(recvPkt);
#ifdef DEBUG_ADP
	fprintf(stdout, "\treceveing darp from node %s\n", bitString(recvPktRh_->srcAdd_));
#endif
#ifdef ADP_DAXX_CACHING
	addEntry(recvPktRh_->srcId_, recvPktRh_->srcAdd_);
	addEntry(recvPktRh_->forId_, recvPktRh_->forAdd_);
	addEntry(recvPktRh_->dstId_, recvPktRh_->dstAdd_);
	addEntry(recvPktRh_->reqId_, recvPktRh_->reqAdd_);
#endif
	if (recvPktRh_->dstAdd_ == mdart_->address_) {
		addEntry(recvPktRh_->reqId_, recvPktRh_->reqAdd_);
		Packet::free(recvPkt);
		return;
	}
	nsaddr_t nextHop_ = mdart_->routingTable_->getEntry(recvPktRh_->dstAdd_);
	if (nextHop_ == (nsaddr_t)IP_BROADCAST) {
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDARP DROP_RTR_ADPRTR\n");
#endif
		Packet::free(recvPkt);
		return;
	}
	if (nextHop_ == recvPktRh_->forId_) {
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDARP DROP_RTR_ADPLOOP\n");
#endif
		Packet::free(recvPkt);
		return;
	}
#ifdef DEBUG_ADP
	fprintf(stdout, "\t\tselected nextHop_ = %d\n", nextHop_);
#endif
	recvPktCh_->direction() = hdr_cmn::DOWN;
	recvPktCh_->next_hop() = nextHop_;		
//	recvPktIh_->saddr() = mdart_->address_;
	recvPktIh_->daddr() = nextHop_;
//	recvPktRh_->dstId_ = findId(recvPktRh_->dstAdd_);
	recvPktRh_->forId_ = mdart_->id_;
	recvPktRh_->forAdd_ = mdart_->address_;
#ifdef DEBUG_ADP
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", recvPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", recvPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcId = %d\n", recvPktRh_->srcId_);
	fprintf(stdout, "\tsendPktRh_->srcAdd = %s\n", bitString(recvPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->forId = %d\n", recvPktRh_->forId_);
	fprintf(stdout, "\tsendPktRh_->forAdd = %s\n", bitString(recvPktRh_->forAdd_));
	fprintf(stdout, "\tsendPktRh_->dstId = %d\n", recvPktRh_->dstId_);
	fprintf(stdout, "\tsendPktRh_->dstAdd = %s\n", bitString(recvPktRh_->dstAdd_));
	fprintf(stdout, "\tsendPktRh_->reqId = %d\n", recvPktRh_->reqId_);
	fprintf(stdout, "\tsendPktRh_->reqAdd = %s\n", bitString(recvPktRh_->reqAdd_));
#endif		
	Scheduler::instance().schedule(mdart_->getTarget(), recvPkt, 0.0);
}

void ADP::sendDaup() {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tMDART::sendDaup()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
//	printDHT();
#endif
	nsaddr_t dstAdd_ = hash(mdart_->id_);
#ifdef DEBUG_ADP
	fprintf(stdout, "\tsending daup for node  %s\n", bitString(dstAdd_));
	mdart_->routingTable_->print();
#endif
	if (dstAdd_ == mdart_->address_) {
		addEntry(mdart_->id_, mdart_->address_);
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDAUP received\n");
#endif
		return;
	}
	if (mdart_->routingTable_->DAGetEntry(dstAdd_) == (nsaddr_t)IP_BROADCAST) {
		bitset<ADDR_SIZE> oldAdd_ (dstAdd_);
#ifdef DEBUG_ADP
		fprintf(stdout, "\told dstAdd_ = %s\n", bitString(dstAdd_));
#endif
		int i = 0;
		while ((mdart_->routingTable_->DAGetEntry(dstAdd_) == (nsaddr_t)IP_BROADCAST) && (i < ADDR_SIZE)) {
			oldAdd_.reset(i);
			dstAdd_ = (nsaddr_t) oldAdd_.to_ulong();
//			fprintf(stdout, "\ttemp dstAdd_ = %s\n", bitString(dstAdd_));
			i++;
		}
#ifdef DEBUG_ADP
		fprintf(stdout, "\tnew dstAdd_ = %s\n", bitString(dstAdd_));
#endif
		if (mdart_->routingTable_->DAGetEntry(dstAdd_) == (nsaddr_t)IP_BROADCAST) {
			addEntry(mdart_->id_, mdart_->address_);
#ifdef DEBUG_ADP
			fprintf(stdout, "\tDAUP DROP_RTR_ADPNRT\n");
#endif		
			return;
		}
	}
	nsaddr_t nextHop_ = mdart_->routingTable_->DAGetEntry(dstAdd_);
#ifdef DEBUG_ADP
	fprintf(stdout, "\tselected nextHop_ = %d\n", nextHop_);
#endif
	Packet *sendPkt_ = Packet::alloc();
	struct hdr_cmn *sendPktCh_ = HDR_CMN(sendPkt_);
	struct hdr_ip *sendPktIh_ = HDR_IP(sendPkt_);
	struct hdr_mdart_daup *sendPktRh_ = HDR_MDART_DAUP(sendPkt_);

	sendPktCh_->ptype() = PT_MDART;
	sendPktCh_->size() = IP_HDR_LEN + mdart_->size();
	sendPktCh_->iface() = -2;
	sendPktCh_->error() = 0;
	sendPktCh_->addr_type() = NS_AF_NONE;
	sendPktCh_->next_hop() = nextHop_;		

	sendPktIh_->saddr() = mdart_->id_;
	sendPktIh_->daddr() = nextHop_;
	sendPktIh_->sport() = RT_PORT;
	sendPktIh_->dport() = RT_PORT;
	sendPktIh_->ttl_ = IP_DEF_TTL;		

	sendPktRh_->type_ = MDART_TYPE_DAUP;
	sendPktRh_->srcId_ = mdart_->id_;
	sendPktRh_->srcAdd_ = mdart_->address_;
	sendPktRh_->forId_ = mdart_->id_;
	sendPktRh_->forAdd_ = mdart_->address_;
	sendPktRh_->dstId_ = IP_BROADCAST;
	sendPktRh_->dstAdd_ = dstAdd_;
	sendPktRh_->lifetime_ = ADP_DARP_TIMEOUT;
	sendPktRh_->seqNum_ = pckSeqNum();
#ifdef DEBUG_ADP
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", sendPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", sendPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcId_ = %d\n", sendPktRh_->srcId_);
	fprintf(stdout, "\tsendPktRh_->srcAdd_ = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->forId_ = %d\n", sendPktRh_->forId_);
	fprintf(stdout, "\tsendPktRh_->forAdd_ = %s\n", bitString(sendPktRh_->forAdd_));
	fprintf(stdout, "\tsendPktRh_->dstId_ = %d\n", sendPktRh_->dstId_);
	fprintf(stdout, "\tsendPktRh_->dstAdd_ = %s\n", bitString(sendPktRh_->dstAdd_));
#endif
	Scheduler::instance().schedule(mdart_->getTarget(), sendPkt_, 0.0);
}

void ADP::recvDaup(Packet* recvPkt) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tADP::recvDaup()\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
//	printDHT();
#endif
	struct hdr_cmn *recvPktCh_ = HDR_CMN(recvPkt);
	struct hdr_ip *recvPktIh_ = HDR_IP(recvPkt);
	struct hdr_mdart_daup *recvPktRh_ = HDR_MDART_DAUP(recvPkt);
	bool changeAdd_ = false;	
#ifdef DEBUG_ADP
	fprintf(stdout, "\treceveing daup for node %s\n", bitString(recvPktRh_->dstAdd_));
	mdart_->routingTable_->print();
#endif
#ifdef	ADP_DAXX_CACHING
	addEntry(recvPktRh_->srcId_, recvPktRh_->srcAdd_);
	addEntry(recvPktRh_->forId_, recvPktRh_->forAdd_);
#endif
	if (recvPktRh_->dstAdd_ == mdart_->address_) {
		addEntry(recvPktRh_->srcId_, recvPktRh_->srcAdd_);
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDAUP received\n");
#endif
		Packet::free(recvPkt);
		return;
	}
	if (mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == (nsaddr_t)IP_BROADCAST) {
		changeAdd_ = true;	
		bitset<ADDR_SIZE> oldAdd_ (recvPktRh_->dstAdd_);
#ifdef DEBUG_ADP
		fprintf(stdout, "\told dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
#endif
		int i = 0;
		while (((mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == recvPktRh_->forId_) || (mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == (nsaddr_t)IP_BROADCAST)) && (i < ADDR_SIZE)) {
			oldAdd_.reset(i);
			recvPktRh_->dstAdd_ = (nsaddr_t) oldAdd_.to_ulong();
			i++;
		}
#ifdef DEBUG_ADP
		fprintf(stdout, "\tnew dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
#endif
		if (mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_) == (nsaddr_t)IP_BROADCAST) {
			addEntry(recvPktRh_->srcId_, recvPktRh_->srcAdd_);
#ifdef DEBUG_ADP
			fprintf(stdout, "\tDAUP DROP_RTR_ADPNRT\n");
#endif		
			mdart_->drop(recvPkt, "DROP_RTR_ADPNRT");
			return;
		}
	}
	nsaddr_t nextHop_ = mdart_->routingTable_->DAGetEntry(recvPktRh_->dstAdd_);
	if (nextHop_ == recvPktRh_->forId_ && !changeAdd_) {
		addEntry(recvPktRh_->srcId_, recvPktRh_->srcAdd_);
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDAUP DROP_RTR_ADPLOOP\n");
#endif
		mdart_->drop(recvPkt, "DROP_RTR_ADPLOOP");
		return;
	}
#ifdef DEBUG_ADP
	fprintf(stdout, "\t\tselected nextHop_ = %d\n", nextHop_);
#endif
	recvPktCh_->direction() = hdr_cmn::DOWN;
	recvPktCh_->next_hop() = nextHop_;
	recvPktIh_->daddr() = nextHop_;
	recvPktRh_->forId_ = mdart_->id_;
	recvPktRh_->forAdd_ = mdart_->address_;
	recvPktRh_->dstId_ = findId(recvPktRh_->dstAdd_);
#ifdef DEBUG_ADP
	fprintf(stdout, "\trecvPktIh_->saddr = %d\n", recvPktIh_->saddr());
	fprintf(stdout, "\trecvPktIh_->daddr = %d\n", recvPktIh_->daddr());
	fprintf(stdout, "\trecvPktRh_->srcId_ = %d\n", recvPktRh_->srcId_);
	fprintf(stdout, "\trecvPktRh_->srcAdd_ = %s\n", bitString(recvPktRh_->srcAdd_));
	fprintf(stdout, "\trecvPktRh_->forId_ = %d\n", recvPktRh_->forId_);
	fprintf(stdout, "\trecvPktRh_->forAdd_ = %s\n", bitString(recvPktRh_->forAdd_));
	fprintf(stdout, "\trecvPktRh_->dstId_ = %d\n", recvPktRh_->dstId_);
	fprintf(stdout, "\trecvPktRh_->dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
#endif		
	Scheduler::instance().schedule(mdart_->getTarget(), recvPkt, 0.0);
}

void ADP::sendDabr() {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tMDART::sendDabr()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
	printDHT();
#endif
	Packet *sendPkt_ = Packet::alloc();
	struct hdr_cmn *sendPktCh_ = HDR_CMN(sendPkt_);
	struct hdr_ip *sendPktIh_ = HDR_IP(sendPkt_);
	struct hdr_mdart_dabr *sendPktRh_ = HDR_MDART_DABR(sendPkt_);

	sendPktCh_->ptype() = PT_MDART;
	sendPktCh_->size() = IP_HDR_LEN + mdart_->size();
	sendPktCh_->iface() = -2;
	sendPktCh_->error() = 0;
	sendPktCh_->addr_type() = NS_AF_NONE;
	sendPktCh_->next_hop() = IP_BROADCAST;

	sendPktIh_->saddr() = mdart_->id_;
	sendPktIh_->daddr() = IP_BROADCAST;
	sendPktIh_->sport() = RT_PORT;
	sendPktIh_->dport() = RT_PORT;
	sendPktIh_->ttl_ = IP_DEF_TTL;

	sendPktRh_->type_ = MDART_TYPE_DABR;
	sendPktRh_->srcId_ = mdart_->id_;
	sendPktRh_->srcAdd_ = mdart_->address_;
	sendPktRh_->dstAdd_ = DATYPE_BROADCAST;

#ifdef DEBUG_ADP
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", sendPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", sendPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcId_ = %d\n", sendPktRh_->srcId_);
	fprintf(stdout, "\tsendPktRh_->srcAdd_ = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->dstAdd_ = %s\n", bitString(sendPktRh_->dstAdd_));
#endif
	PacketData* data_ = dht_->getDHTUpdate();
	int size_ = data_->size();
	sendPkt_->setdata(data_);
	sendPktCh_->size() = IP_HDR_LEN + size_ + 2 + mdart_->size();
	Scheduler::instance().schedule(mdart_->getTarget(), sendPkt_, 0.0);
}

void ADP::recvDabr(Packet* recvPkt) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tMDART::recvDabr()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
	printDHT();
#endif
#ifdef DEBUG_ADP
	struct hdr_mdart_dabr *recvPktRh_ = HDR_MDART_DABR(recvPkt);
	struct hdr_ip *recvPktIh_ = HDR_IP(recvPkt);
	fprintf(stdout, "\trecvPktIh_->saddr = %d\n", recvPktIh_->saddr());
	fprintf(stdout, "\trecvPktIh_->daddr = %d\n", recvPktIh_->daddr());
	fprintf(stdout, "\trecvPktRh_->srcId_ = %d\n", recvPktRh_->srcId_);
	fprintf(stdout, "\trecvPktRh_->srcAdd_ = %s\n", bitString(recvPktRh_->srcAdd_));
	fprintf(stdout, "\trecvPktRh_->dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
	fprintf(stdout, "\trecvPktRh_->data = %s\n", ((PacketData*)recvPkt->userdata())->data());
	fprintf(stdout, "\trecvPktRh_->data length = %d\n", ((PacketData*)recvPkt->userdata())->size());
#endif
	ostringstream strStream_;
	strStream_ << ((PacketData*)recvPkt->userdata())->data();
	string str_ = strStream_.str();
	string::size_type lastPos_ = str_.find_first_not_of(";", 0);
	string::size_type pos_ = str_.find_first_of(";", lastPos_);
	while (string::npos != pos_ || string::npos != lastPos_) {
		string entry_ = str_.substr(lastPos_, pos_ - lastPos_);
		string::size_type idLastPos_ = entry_.find_first_not_of(",", 0);
		string::size_type idPos_ = entry_.find_first_of(",", idLastPos_);
		nsaddr_t id_  = atoi(entry_.substr(idLastPos_, idPos_ - idLastPos_).c_str());
		string::size_type addressLastPos_ = entry_.find_first_not_of(",", idPos_);
		string::size_type addressPos_ = entry_.find_first_of(",", addressLastPos_);
		nsaddr_t address_  = atoi(entry_.substr(addressLastPos_, addressPos_ - addressLastPos_).c_str());
		string::size_type expireLastPos_ = entry_.find_first_not_of(",", addressPos_);
		string::size_type expirePos_ = entry_.find_first_of(",", expireLastPos_);
		double expire_ = (atof(entry_.substr(expireLastPos_, expirePos_ - expireLastPos_).c_str()));
#ifdef DEBUG_DHT
		fprintf(stdout, "\tid %d\taddress %d\texpire %f\n", id_, address_, expire_);
#endif
		addEntry(id_, address_, expire_);
		lastPos_ = str_.find_first_not_of(";", pos_);
		pos_ = str_.find_first_of(";", lastPos_);
	}
	
	Packet::free(recvPkt);
#ifdef DEBUG_ADP
	printDHT();
#endif
}



//------------------------------------------------------------------------------
// ADP DHT functions
//------------------------------------------------------------------------------
void ADP::addEntry(nsaddr_t id, nsaddr_t address) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tADP::addEntry(%d, %s)\t\tin node %d\twith address %s\n", CURRENT_TIME, id, bitString(address), mdart_->id_, bitString(mdart_->address_));
#endif
	dht_->addEntry(id, address);
	Packet* p;
#ifdef DEBUG_ADP
	fprintf(stdout,"\tlooking for packet to %d\n", id);
	atr_->queue_->printQueue();
#endif
	while( (p = mdart_->queue_->deque(id)) ) {
#ifdef DEBUG_ADP
		fprintf(stdout,"\tsending data packet for node %d\n", id);
#endif
		nsaddr_t addr_ = findAdd(id);
		struct hdr_mdart_encp* rh_ = HDR_MDART_ENCP(p);		
		rh_->dstAdd_ = addr_;
		mdart_->forward(p);
	}
}

void ADP::addEntry(nsaddr_t id, nsaddr_t address, double expire) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tADP::addEntry(%d, %s, %f)\t\tin node %d\twith address %s\n", CURRENT_TIME, id, bitString(address), expire, mdart_->id_, bitString(mdart_->address_));
#endif
	dht_->addEntry(id, address, expire);
	Packet* p;
#ifdef DEBUG_ADP
	fprintf(stdout,"\tlooking for packet to %d\n", id);
	mdart_->queue_->printQueue();
#endif
	while( (p = mdart_->queue_->deque(id)) ) {
#ifdef DEBUG_ADP
		fprintf(stdout,"\tsending data packet for node %d\n", id);
#endif
		nsaddr_t addr_ = findAdd(id);
		struct hdr_mdart_encp* rh_ = HDR_MDART_ENCP(p);		
		rh_->dstAdd_ = addr_;
		mdart_->forward(p);
	}
}
