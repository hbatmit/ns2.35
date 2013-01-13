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



#include <mdart/mdart_ndp.h>
#include <mdart/mdart.h>



//------------------------------------------------------------------------------
// Neigbor Discovery Protocol (NDP)
//------------------------------------------------------------------------------
NDP::NDP(MDART* mdart) {
	mdart_ = mdart;
	neighborSet_ = new neighborSet;
	helloTimer_ = new NDPHelloTimer(this);
	neighborTimer_ = new NDPNeighborTimer(this);
	helloSeqNum_ = 0;
}

NDP::~NDP() {
	delete helloTimer_;
	delete neighborTimer_;
	delete neighborSet_;
}



/*******************************************************************************
 * NDP message functions
 ******************************************************************************/
void NDP::sendHello() {
#ifdef DEBUG_NDP
	fprintf(stdout, "%.9f\tNDP::sendHello()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	Packet *sendPkt_ = Packet::alloc();
	struct hdr_cmn *sendPktCh_ = HDR_CMN(sendPkt_);
	struct hdr_ip *sendPktIh_ = HDR_IP(sendPkt_);
	struct hdr_mdart_hello *sendPktRh_ = HDR_MDART_HELLO(sendPkt_);

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

	sendPktRh_->type_ = MDART_TYPE_HELLO;
	sendPktRh_->dstAdd_ = DATYPE_BROADCAST;
	sendPktRh_->srcAdd_ = mdart_->address_;
	sendPktRh_->srcId_ = mdart_->id_;
	sendPktRh_->seqNum_ = helloSeqNum();

	//	PacketData* data_ = mdart_->routingTable_->getUpdate();
	//	assert(data_);
	string string_ = mdart_->routingTable_->getUpdate();
	string_ += "-";
 	if (mdart_->etxMetric_ == 1) {
		string_ += getNeighborQuality();
	}
	PacketData* data_ = new PacketData(string_.length() + 2);
	strcpy((char *)data_->data(), string_.data());
	sendPkt_->setdata(data_);
	sendPktCh_->size() = IP_HDR_LEN + string_.length() + 2 + mdart_->size();

#ifdef DEBUG_NDP
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", sendPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", sendPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcId_ = %d\n", sendPktRh_->srcId_);
	fprintf(stdout, "\tsendPktRh_->srcAdd_ = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->dstAdd_ = %s\n", bitString(sendPktRh_->dstAdd_));
	fprintf(stdout, "\tsendPktRh_->seqNum_ = %d\n", sendPktRh_->seqNum_);
	fprintf(stdout, "\tsendPktRh_->data() = %s\n", ((PacketData*)sendPkt_->userdata())->data());
//	if (mdart_->logtarget_ != 0) {
//		sprintf(mdart_->logtarget_->pt_->buffer(), "x -t %.9f mdart sendHello srcId %d srcAdd %s dstAdd %s seqNum %d", CURRENT_TIME, sendPktRh_->srcId_, bitString(sendPktRh_->srcAdd_), bitString(sendPktRh_->dstAdd_), sendPktRh_->seqNum_);
//		mdart_->logtarget_->pt_->dump();
//	}
#endif
	Scheduler::instance().schedule(mdart_->getTarget(), sendPkt_, 0.0);
}

void NDP::recvHello(Packet* recvPkt_) {
	struct hdr_mdart_hello *recvPktRh_ = HDR_MDART_HELLO(recvPkt_);
#ifdef DEBUG_NDP
	fprintf(stdout, "%.9f\tNDP::recvHello()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
	struct hdr_ip *recvPktIh_ = HDR_IP(recvPkt_);
	fprintf(stdout, "\trecvPktIh_->saddr = %d\n", recvPktIh_->saddr());
	fprintf(stdout, "\trecvPktIh_->daddr = %d\n", recvPktIh_->daddr());
	fprintf(stdout, "\trecvPktRh_->dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
	fprintf(stdout, "\trecvPktRh_->srcAdd_ = %s\n", bitString(recvPktRh_->srcAdd_));
	fprintf(stdout, "\trecvPktRh_->srcId_ = %d\n", recvPktRh_->srcId_);
	fprintf(stdout, "\trecvPktRh_->seqNum_ = %d\n", recvPktRh_->seqNum_);
	fprintf(stdout, "\tsendPktRh_->data() = %s\n", ((PacketData*)recvPkt_->userdata())->data());
#endif
	ostringstream strStream_;
	strStream_ << ((PacketData*)recvPkt_->userdata())->data();
	string string_ = strStream_.str();
	string::size_type start_ = string_.find_first_not_of("-", 0);
	string::size_type end_ = string_.find_first_of("-", start_);
//	string table_ = "-";
//	if ((int) end_ > -1)
//		table_ = string_.substr(start_, end_);
//	else
//		end_ = (size_t) 0;
	string table_ = string_.substr(start_, end_);
	double forQuality_ = (double) 0.0;
#ifdef DEBUG_NDP
	fprintf(stdout, "\ttable_ = %s\n", table_.c_str());
#endif
	if (mdart_->etxMetric_ == 1) {
		string quality_ = string_.substr(end_ + 1, string_.size() - end_ - 1);
#ifdef DEBUG_NDP
		fprintf(stdout, "\tquality_ = %s\n", quality_.c_str());
#endif
		string::size_type lastPos_ = quality_.find_first_not_of(";", 0);
		string::size_type pos_ = quality_.find_first_of(";", lastPos_);
		while (string::npos != pos_ || string::npos != lastPos_) {
			string neighbor_ = quality_.substr(lastPos_, pos_ - lastPos_);
			string::size_type idLastPos_ = neighbor_.find_first_not_of(",", 0);
			string::size_type idPos_ = neighbor_.find_first_of(",", idLastPos_);
			nsaddr_t id_  = atoi(neighbor_.substr(idLastPos_, idPos_ - idLastPos_).c_str());
			string::size_type qualityLastPos_ = neighbor_.find_first_not_of(",", idPos_);
			string::size_type qualityPos_ = neighbor_.find_first_of(",", qualityLastPos_);
		#ifdef DEBUG_NDP
			fprintf(stdout, "\tneigh id %d\tquality %f\n", id_, forQuality_);
		#endif
			if (id_ == mdart_->id_) {
		#ifdef DEBUG_NDP
				fprintf(stdout, "\t\tinsert\n");
		#endif
				forQuality_ = atof(neighbor_.substr(qualityLastPos_, qualityPos_ - qualityLastPos_).c_str());
			}
			lastPos_ = quality_.find_first_not_of(";", pos_);
			pos_ = quality_.find_first_of(";", lastPos_);
		}
	}
	neighborInsert(recvPktRh_->srcId_, recvPktRh_->srcAdd_, recvPktRh_->seqNum_, table_, forQuality_);
	Packet::free(recvPkt_);
}



/*******************************************************************************
 * NDP timers functions
 ******************************************************************************/
void NDP::startHelloTimer() {
	helloTimer_->handle((Event*) 0);
}

void NDP::startNeighborTimer() {
	neighborTimer_->handle((Event*) 0);
}



/*******************************************************************************
 * NDP neighbor list functions
 ******************************************************************************/
Neighbor* NDP::neighborLookup(nsaddr_t id) {
// #ifdef DEBUG
// 	fprintf(stdout, "%.9f\tNDP::neighborLookup(%d)\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id, mdart_->id_, bitString(mdart_->address_));
// #endif
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
		if((*neighbor_)->id() == id) {
// #ifdef DEBUG
// 			fprintf(stdout, "\tId found\n");
// #endif
			return (*neighbor_);
		}
	}
	return 0;
}

void NDP::neighborInsert(nsaddr_t id, nsaddr_t address, u_int32_t helloSeqNum, string table, double quality) {
#ifdef DEBUG_NDP
	fprintf(stdout, "%.9f\tNDP::neighborInsert(%d, %s, %f)\t\tin node %d\twith address %s\n", CURRENT_TIME, id, bitString(address), quality, mdart_->id_, bitString(mdart_->address_));
#endif
	Neighbor *neighbor_ = neighborLookup(id);
	if (neighbor_ == 0) {
//		fprintf(stdout, "\tNeighbor inserted\n");
		neighbor_ = new Neighbor(id, address, helloSeqNum);
		neighborSet_->insert(neighbor_);
	}
	else {
		neighbor_->address(address, helloSeqNum);
//		neighbor_->expire(expire);
	}
//#ifdef LQE_ALGORITHM_MA
//	if (quality != 0)
	neighbor_->forLinkQuality(quality);
//#endif
	neighbor_->updateTable(table);
}

/*
void NDP::neighborDelete(nsaddr_t id) {
#ifdef DEBUG_NDP
	fprintf(stdout, "%.9f\tNDP::neighborDelete(%d)\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id, mdart_->id_, bitString(mdart_->address_));
#endif
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
		if((*neighbor_)->id() == id) {
			neighborSet_->erase(neighbor_);
#ifdef DEBUG_NDP
 			fprintf(stdout, "\tNeighbor removed\n");
#endif
			break;
		}
	}
}
*/

void NDP::neighborPurge(void) {
 #ifdef DEBUG_NDP
 	fprintf(stdout, "%.9f\tNDP::neighborPurge()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
 #endif
	double now = CURRENT_TIME;
	neighborSet::iterator neighbor_ = neighborSet_->begin();
	while (neighbor_ != neighborSet_->end()) {
		neighborSet::iterator nextNeighbor_ = neighbor_;
		++nextNeighbor_;
		if((*neighbor_)->expire() <= now) {
//			mdart_->routingTable_->purge((*neighbor_)->id_);
			neighborSet_->erase(neighbor_);
			delete (*neighbor_);
#ifdef DEBUG_NDP
 			fprintf(stdout, "\tNeighbor purged\n");
#endif
		}
		else {
			(*neighbor_)->purgeHello();
		}
		neighbor_ = nextNeighbor_;
	}

/*	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
#ifdef DEBUG
			fprintf(stdout, "\tNeighbor id = %d\texpire = %.9f\n", (*neighbor_)->id_, (*neighbor_)->expire());
#endif
		if((*neighbor_)->expire() <= now) {
//			mdart_->routingTable_->purge((*neighbor_)->id_);
			neighborSet_->erase(neighbor_);
			delete (*neighbor_);
#ifdef DEBUG
 			fprintf(stdout, "\tNeighbor purged\n");
#endif
		}
	}
*/}

void NDP::neighborPrint() {
#ifdef DEBUG_ADDRESS_ALLOCATION
 	fprintf(stdout, "\tNDP::neighborPrint()\t\t\tin node %d\twith address %s\n", mdart_->id_, bitString(mdart_->address_));
#endif
	neighborSet::iterator neighbor_;
//#ifdef DEBUG_ADDRESS_ALLOCATION
   for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
      fprintf(stdout, "\t\tNeighbor id=%d\taddress=%s\n", (*neighbor_)->id(), bitString((*neighbor_)->address()));

//		(*neighbor_)->printTable();
// ora
   }
//#endif
}

int NDP::neighborDegree() {
	int degree = 0;
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
		degree += 1;
	}
	return degree;
}

int NDP::realNeighborDegree() {
	int degree = 0;
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
		if(((*neighbor_)->expire() - (1.5 * NDP_ALLOWED_HELLO_LOSS * NDP_HELLO_INTERVAL) + NDP_HELLO_INTERVAL) >= CURRENT_TIME) {
			degree += 1;
		}
	}
	return degree;
}

/*
void NDP::setNeighborQuality(string str_) {
	string::size_type lastPos_ = str_.find_first_not_of(";", 0);
	string::size_type pos_ = str_.find_first_of(";", lastPos_);
	while (string::npos != pos_ || string::npos != lastPos_) {
		string neighbor_ = str_.substr(lastPos_, pos_ - lastPos_);
		string::size_type idLastPos_ = neighbor_.find_first_not_of(",", 0);
		string::size_type idPos_ = neighbor_.find_first_of(",", idLastPos_);
		nsaddr_t id_  = atoi(neighbor_.substr(idLastPos_, idPos_ - idLastPos_).c_str());
		string::size_type qualityLastPos_ = neighbor_.find_first_not_of(",", idPos_);
		string::size_type qualityPos_ = neighbor_.find_first_of(",", qualityLastPos_);
		double quality_ = atoi(neighbor_.substr(qualityLastPos_, qualityPos_ - qualityLastPos_).c_str());
#ifdef DEBUG_NDP
		fprintf(stdout, "\tneigh id %d\tquality %f\n", id_, quality_);
#endif
//		addEntry(id_, address_, expire_);
		lastPos_ = str_.find_first_not_of(";", pos_);
		pos_ = str_.find_first_of(";", lastPos_);
	}
	
//	dht_->setDHTUpdate((PacketData*)recvPkt->userdata());
}
*/

string NDP::getNeighborQuality() {
#ifdef DEBUG_NDP
	fprintf(stdout, "%.9f\tNDP::getNeighborQuality()\n", CURRENT_TIME);
#endif
	string str_;
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
/*		if((*neighbor_)->revLinkQuality() < LQE_RV_THRESHOLD) {
			fprintf(stdout, "%.9f\tNDP::getNeighborQuality()\n", CURRENT_TIME);
			fprintf(stdout, "\tid = %d\trevLinkQuality() = %f\n", (*neighbor_)->id(), (*neighbor_)->revLinkQuality());
//			(*neighbor_)->printHello();
		}*/
//		if((*neighbor_)->revLinkQuality() >= LQE_RV_THRESHOLD) {
			ostringstream stream_;
			stream_ << (*neighbor_)->id();
			stream_ << ',';
			stream_ << (*neighbor_)->revLinkQuality();
			stream_ << ';';
			str_ += stream_.str();
//		}
	}
#ifdef DEBUG_NDP
	fprintf(stdout, "\tdata = %s\n", str_.data());
#endif
	return str_;
}



/*******************************************************************************
 * NDP management functions
 ******************************************************************************/
void NDP::selectAddress() {
#ifdef DEBUG_NDP
	fprintf(stdout, "\tNDP::selectAddress()\t\t\tin node %d\twith address %s\n", mdart_->id_, bitString(mdart_->address_));
#endif
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
#ifdef DEBUG_NDP_LINK_QUALITY
		if (mdart_->etxMetric_ == 1 && (*neighbor_)->linkQuality() < LQE_THRESHOLD) {
			fprintf(stdout, "%.9f\tNDP::selectAddress()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
			fprintf(stdout, "\tlinkQuality() = %f\trevLinkQuality() = %f\n", (*neighbor_)->linkQuality(), (*neighbor_)->revLinkQuality());
		}
#endif
		if ((mdart_->etxMetric_ == 1 && (*neighbor_)->linkQuality() > LQE_THRESHOLD) || mdart_->etxMetric_ == 0) {
#ifdef DEBUG_NDP
			fprintf(stdout, "\t\tselected neighbor id=%d\taddress=%s\n", (*neighbor_)->id(), bitString((*neighbor_)->address()));
#endif
			int i;
			for(i=(*neighbor_)->insertionPoint(); i>-1; i--) {
				std::bitset<ADDR_SIZE>	bitNewAddress_((*neighbor_)->address());
				bitNewAddress_.flip(i);
				mdart_->address_ = bitNewAddress_.to_ulong();
#ifdef DEBUG_ADDRESS_ALLOCATION
				fprintf(stdout, "\t\ttrying address=%s\n", bitString(mdart_->address_));
#endif
				mdart_->routingTable_->setUpdate((*neighbor_));
				if (validateAddress()) {
#ifdef DEBUG_ADDRESS_ALLOCATION
					fprintf(stdout, "\t\taddress selected is %s\n", bitNewAddress_.to_string().c_str());
#endif 
					mdart_->adp_->updateAdd();
					return;
				}
				else {
#ifdef DEBUG_NDP
					fprintf(stdout, "\t\taddress selected %s is not valid\n", bitNewAddress_.to_string().c_str());
#endif
					mdart_->address_ = mdart_->oldAddress_;
					mdart_->routingTable_->clear();
				}
			}
		}
	}
#ifdef DEBUG_NDP
	fprintf(stdout, "\t\tcouldn't select a valid address\n");
#endif
}



bool NDP::validateAddress() {
#ifdef DEBUG_ADDRESS_ALLOCATION
	fprintf(stdout, "\tNDP::validateAddress()\t\t\tin node %d\twith address %s\n", mdart_->id_, bitString(mdart_->address_));
	mdart_->routingTable_->print();
#endif
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
#ifdef DEBUG_NDP_LINK_QUALITY
		fprintf(stdout, "\tid = %d\tlinkQuality() = %f\trevLinkQuality() = %f\n", (*neighbor_)->id(), (*neighbor_)->linkQuality(), (*neighbor_)->revLinkQuality());
#endif
		if ((mdart_->etxMetric_ == 1 && (*neighbor_)->linkQuality() >= LQE_THRESHOLD) || mdart_->etxMetric_ == 0) {
			int levelSibling_ = DiffBit(mdart_->address_, (*neighbor_)->address());
#ifdef DEBUG_ADDRESS_ALLOCATION
			fprintf(stdout, "\t\tNeighbor id=%d\taddress = %s\tLevel sibling = %d\n", (*neighbor_)->id(), bitString((*neighbor_)->address()), levelSibling_);
#endif
			if (levelSibling_ == -1) {
#ifdef DEBUG_ADDRESS_ALLOCATION
				if ((*neighbor_)->id() >= mdart_->id_) {
					fprintf(stdout, "\t\t\tNeighbor id=%d >= node id=%d\n", (*neighbor_)->id(), mdart_->id_);
				}
#endif
				if ((*neighbor_)->id() < mdart_->id_) {
#ifdef DEBUG_ADDRESS_ALLOCATION
					fprintf(stdout, "\t\t\tNeighbor id=%d < node id=%d\n", (*neighbor_)->id(), mdart_->id_);
					fprintf(stdout, "\t\t\tinvalid address\n");
#endif
					return false;
				}
			}
			else {
#ifdef DEBUG_ADDRESS_ALLOCATION
				if (!(*neighbor_)->entryPresent(levelSibling_)) {
					fprintf(stdout, "\t\t\tNeighbor Entry not present\n");
				}
#endif
				if ((*neighbor_)->entryPresent(levelSibling_)) {
#ifdef DEBUG_ADDRESS_ALLOCATION
					if ((*neighbor_)->networkId(levelSibling_) >= mdart_->routingTable_->levelId(levelSibling_-1)) {
						fprintf(stdout, "\t\t\tNeighborTableEntry->networkId(%d)=%d >= node levelId(%d)=%d\n", levelSibling_, (*neighbor_)->networkId(levelSibling_), levelSibling_ -1 , mdart_->routingTable_->levelId(levelSibling_-1));
					}
#endif
					if ((*neighbor_)->networkId(levelSibling_) < mdart_->routingTable_->levelId(levelSibling_-1)) {
#ifdef DEBUG_ADDRESS_ALLOCATION
						fprintf(stdout, "\t\t\tNeighborTableEntry->networkId(%d)=%d < node levelId(%d)=%d\n", levelSibling_, (*neighbor_)->networkId(levelSibling_), levelSibling_ -1 , mdart_->routingTable_->levelId(levelSibling_-1));
						fprintf(stdout, "\t\t\tinvalid address\n");
#endif
						return false;
					}
				}
			}
#ifdef DEBUG_ADDRESS_ALLOCATION
			fprintf(stdout, "\t\t\tvalid address\n");
#endif
		}
	}
	return true;
}

void NDP::updateRoutingTable() {
#ifdef DEBUG_NDP_LINK_QUALITY
	fprintf(stdout, "%.9f\tNDP::updateRoutingTable()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	neighborSet::iterator neighbor_;
	for (neighbor_ = neighborSet_->begin(); neighbor_ != neighborSet_->end(); ++neighbor_) {
#ifdef DEBUG_NDP_LINK_QUALITY
		if (mdart_->etxMetric_ == 1 && (*neighbor_)->linkQuality() < LQE_THRESHOLD) {
			fprintf(stdout, "\tid = %d\tlinkQuality() = %f\trevLinkQuality() = %f\n", (*neighbor_)->id(), (*neighbor_)->linkQuality(), (*neighbor_)->revLinkQuality());
//			neighborPrint();
		}
#endif
		if ((mdart_->etxMetric_ == 1 && (*neighbor_)->linkQuality() >= LQE_THRESHOLD) || mdart_->etxMetric_ == 0)
			mdart_->routingTable_->setUpdate(*neighbor_);
	}
}

