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



#include <mdart/mdart.h>
#include <mdart/mdart_ndp.h>
#include <mdart/mdart_adp.h>



//------------------------------------------------------------------------------
// Static object definition
//------------------------------------------------------------------------------
// MDART packet header static offset definition
int hdr_mdart::offset_;
// MDART static Lookup Table definition
// DHT
//lookupTable MDART::lookupTable_ = lookupTable();



//------------------------------------------------------------------------------
// TCL Hooks
//------------------------------------------------------------------------------
static class MDARTHeaderClass : public PacketHeaderClass {
	public:
		MDARTHeaderClass() : PacketHeaderClass("PacketHeader/MDART", sizeof(hdr_all_mdart)) {
			bind_offset (&hdr_mdart::offset_);
		}
} class_rtProtoMDART_hdr;



static class MDARTClass : public TclClass {
	public:
		MDARTClass() : TclClass("Agent/MDART") {
		}
		TclObject* create(int argc, const char* const* argv) {
			assert(argc == 5);
			MDART* mdart_ = new MDART((nsaddr_t)Address::instance().str2addr(argv[4]));
			assert(mdart_);
			return (mdart_);
		}
} class_rtProtoMDART;



//------------------------------------------------------------------------------
// For cross-layer with layer 2
//------------------------------------------------------------------------------
inline static void macFailedCallback(Packet *recvPkt, void *arg) {
	((MDART*)arg)->macFailed(recvPkt);
}



//------------------------------------------------------------------------------
// MDART timer 
//------------------------------------------------------------------------------
void MDARTTimer::handle(Event*) {
//#ifdef DEBUG
//	fprintf(stdout, "%.9f\tMDARTTimer::handle\t\t\tin node %d\twith address %s\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
//#endif
	double interval_ = NDP_MIN_HELLO_INTERVAL + ((NDP_MAX_HELLO_INTERVAL - NDP_MIN_HELLO_INTERVAL) * Random::uniform());
	switch(numberOfCall_) {
		case 0:
			numberOfCall_++;
			interval_ = Random::uniform();
			Scheduler::instance().schedule(this, &intr, interval_);
			break;
		case 1:
			numberOfCall_++;
//			//check my neighbors regularly
//			mdart_->ndp_->startNeighborTimer();
			mdart_->ndp_->neighborPurge();
			mdart_->validateAddress();
			mdart_->ndp_->updateRoutingTable();
			//send hello packets regularly
			mdart_->ndp_->startHelloTimer();
			Scheduler::instance().schedule(this, &intr, interval_);
			break;
		default:
			mdart_->ndp_->neighborPurge();
			mdart_->validateAddress();
			mdart_->ndp_->updateRoutingTable();
			Scheduler::instance().schedule(this, &intr, interval_);
	}
}



//------------------------------------------------------------------------------
// Augmented Tree-based Routing (MDART)
//------------------------------------------------------------------------------
MDART::MDART(nsaddr_t id) : Agent(PT_MDART) {
//	bind("address_", &address_);
	bind_bool("macFailed_", &macFailed_);
	bind_bool("etxMetric_", &etxMetric_);
	id_ = id;
	bitset<ADDR_SIZE> bitAddress_ (0);
	address_ = (unsigned int) bitAddress_.to_ulong();
//	MDART::lookupTable_ [id_] = address_;
	// dht
//	lookupTableAddEntry(id_, address_);
// #ifdef DEBUG
// 	fprintf(stdout, "%.9f\tMDART::MDART()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitAddress_.to_string().c_str());
// #endif
	mdartTimer_ = new MDARTTimer(this);
	ndp_ = new NDP(this);
	adp_ = new ADP(this);
	routingTable_ = new RoutingTable(this);
	queue_ = new MDARTQueue();
	assert(mdartTimer_);
	assert(ndp_);
	assert(routingTable_);
}



MDART::~MDART() {
#ifdef DEBUG
	fprintf(stdout, "%.9f\tMDART::~MDART()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
#endif
	delete mdartTimer_;
	delete ndp_;
}



int MDART::command(int argc, const char* const* argv) {
#ifdef DEBUG
			fprintf(stdout, "%.9f\tMDART::command()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
#endif
	if (argc == 2) {
		if (strcasecmp(argv[1], "start") == 0) {
			startTimer();
			return TCL_OK;
		}
		else if (strcasecmp(argv[1], "tableSize") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart table -Hs %d -Ds %s -Rt %d", CURRENT_TIME, id_, bitString(address_), routingTable_->size());
				logtarget_->pt_->dump();
			}
			else {
#ifdef DEBUG
				fprintf(stdout, "%f_ %d_ If you want to print the routing table size you must create a trace file in your tcl script", CURRENT_TIME, address_);
#endif
			}
			return TCL_OK;
		}
		else if (strcasecmp(argv[1], "neighborDegree") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart neighbor -Hs %d -Ds %s -Nd %d", CURRENT_TIME, id_, bitString(address_), ndp_->neighborDegree());
				logtarget_->pt_->dump();
			}
			else {
#ifdef DEBUG
				fprintf(stdout, "%f_ %d_ If you want to print the neighbor degree you must create a trace file in your tcl script", CURRENT_TIME, address_);
#endif
			}
			return TCL_OK;
		}
		else if (strcasecmp(argv[1], "realNeighborDegree") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart realNeighbor -Hs %d -Ds %s -Nd %d", CURRENT_TIME, id_, bitString(address_), ndp_->realNeighborDegree());
				logtarget_->pt_->dump();
			}
			else {
#ifdef DEBUG
				fprintf(stdout, "%f_ %d_ If you want to print the real neighbor degree you must create a trace file in your tcl script", CURRENT_TIME, address_);
#endif
			}
			return TCL_OK;
		}
/*		else if (strcasecmp(argv[1], "lookupTablePrint") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart lookupTable -Hs %d -Ds %s", CURRENT_TIME, id_, bitString(address_));
				logtarget_->pt_->dump();
// DHT
								for (lookupTable::iterator it = MDART::lookupTable_.begin(); it != MDART::lookupTable_.end(); it++) {
					sprintf(logtarget_->pt_->buffer(), "\tid\t%d\taddress\t%s", (*it).first, bitString((*it).second));
					logtarget_->pt_->dump();
				}
			}
			else {
				fprintf(stdout, "%f_ %d_ If you want to print the lookup table you must create a trace file in your tcl script", CURRENT_TIME, address_);
			}
			return TCL_OK;
		}
*/		else if (strcasecmp(argv[1], "dhtTablePrint") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart dhtTable -Hs %d -Ds %s", CURRENT_TIME, id_, bitString(address_));
				logtarget_->pt_->dump();
				unsigned int i;
				for(i=0; i<adp_->DHTSize(); i++) {
					sprintf(logtarget_->pt_->buffer(), "\tid\t%d\taddress\t%s", adp_->getEntry(i)->id(), bitString(adp_->getEntry(i)->address()));
					logtarget_->pt_->dump();
				}
			}
			else {
				fprintf(stdout, "%f_ %d_ If you want to print the lookup table you must create a trace file in your tcl script", CURRENT_TIME, address_);
			}
			return TCL_OK;
		}
/*		else if (strcasecmp(argv[1], "dhtInfoPrint") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart dhtInfo -Hs %d -Ds %s", CURRENT_TIME, id_, bitString(address_));
				logtarget_->pt_->dump();
				DHTInfo::iterator it;
				for(it=idp_->dht_->begin(); it!=idp_->dht_->end(); ++it) {
					sprintf(logtarget_->pt_->buffer(), "\tinfo idInfo %s\ttextInfo %s", (*it).first->c_str(), (*it).second->c_str());
					logtarget_->pt_->dump();
				}
			}
			else {
				fprintf(stdout, "%f_ %d_ If you want to print the lookup table you must create a trace file in your tcl script", CURRENT_TIME, address_);
			}
			return TCL_OK;
		}*/
		else if (strcasecmp(argv[1], "routingTablePrint") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart routingTable -Hs %d -Ds %s", CURRENT_TIME, id_, bitString(address_));
				logtarget_->pt_->dump();
				unsigned int i;
				for (i=0; i<ADDR_SIZE; i++) {
					bitset<ADDR_SIZE> tempSibling_ (address_);
					tempSibling_.flip(i);
					sprintf(logtarget_->pt_->buffer(), "\tlevel sibling = %i\tsibling = %s", i, tempSibling_.to_string().c_str());
					logtarget_->pt_->dump();
					entrySet::iterator itEntry_;
					for (itEntry_ = routingTable_->getSibling(i)->begin(); itEntry_ != routingTable_->getSibling(i)->end(); ++itEntry_) {
						sprintf(logtarget_->pt_->buffer(), "\t\tnextHopId = %d\tnextHopAdd = %s\thopNumber = %d\tetxMetric = %f", (*itEntry_)->nextHopId(), bitString((*itEntry_)->nextHopAdd()), (*itEntry_)->hopNumber(), (*itEntry_)->etxMetric());
						logtarget_->pt_->dump();
					}
				}
			}
			else {
#ifdef DEBUG
				fprintf(stdout, "%f_ %d_ If you want to print the lookup table you must create a trace file in your tcl script", CURRENT_TIME, address_);
#endif
			}
			return TCL_OK;
		}
		else if (strcasecmp(argv[1], "neighborPrint") == 0) {
			if (logtarget_ != 0) {
				sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart neighborPrint -Hs %d -Ds %s", CURRENT_TIME, id_, bitString(address_));
				logtarget_->pt_->dump();
				neighborSet::iterator neighbor_;
				for (neighbor_ = ndp_->neighborSet_->begin(); neighbor_ != ndp_->neighborSet_->end(); ++neighbor_) {
					sprintf(logtarget_->pt_->buffer(), "\tneighbor id = %d\taddress = %s", (*neighbor_)->id(), bitString((*neighbor_)->address()));
					logtarget_->pt_->dump();
				}
			}
			else {
#ifdef DEBUG
				fprintf(stdout, "%f_ %d_ If you want to print the lookup table you must create a trace file in your tcl script", CURRENT_TIME, address_);
#endif
			}
			return TCL_OK;
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "port-dmux") == 0) {
			dmux_ = (PortClassifier*)TclObject::lookup(argv[2]);
			if (dmux_ == 0) {
				fprintf(stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		else if (strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
			logtarget_ = (Trace*)TclObject::lookup(argv[2]);
			if (logtarget_ == 0) {
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		else if(strcmp(argv[1], "drop-target") == 0) {
			int stat = queue_->command(argc,argv);
			if (stat != TCL_OK) {
				return stat;
			}
			return Agent::command(argc, argv);
		}
/*		else if(strcmp(argv[1], "requestInfo") == 0) {
			string idInfo = argv[2];
			sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart node %d requestInfo %s", CURRENT_TIME, id_, idInfo.c_str());
			logtarget_->pt_->dump();
			idp_->requestInfo(idInfo, 0);
			return TCL_OK;
		}*/
	}
/*	else if (argc == 4) {
		if(strcmp(argv[1], "storeInfo") == 0) {
			string idInfo = argv[2];
			string textInfo = argv[3];
			sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart node %d storeInfo %s", CURRENT_TIME, id_, idInfo.c_str());
			logtarget_->pt_->dump();
			idp_->storeInfo(idInfo, textInfo);
			return TCL_OK;
		}
	}*/
	return Agent::command(argc, argv);
}



//------------------------------------------------------------------------------
// Packet routines
//------------------------------------------------------------------------------
void MDART::recv(Packet* recvPkt_, Handler* h) {
#ifdef DEBUG
	fprintf(stdout, "%.9f\tMDART::recv()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
#endif
	struct hdr_cmn* ch = HDR_CMN(recvPkt_);
	struct hdr_ip* ih = HDR_IP(recvPkt_);
//	struct hdr_mdart_encp* rh = HDR_MDART_ENCP(recvPkt_);
	// Receveing a packet from the same node
	if (ih->saddr() == id_) {
		if (ch->num_forwards() == 0) {
			// Adding the ip header
			ch->size() += IP_HDR_LEN;
		}
		// Routing loop
		else {
			if (ch->ptype() != PT_MDART) {
#ifdef DEBUG_PACKET_FORWARDING
				fprintf(stdout, "%.9f\tMDART::recv()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
				fprintf(stdout, "DROP_RTR_ROUTE_LOOP\n");
#endif
				drop(recvPkt_, DROP_RTR_ROUTE_LOOP);
				return;			 
			}
		}
	}
	// Receveing a packet from another node
	// Decreasing the TTL fiels
	else {
		if (--ih->ttl_ == 0) {
#ifdef DEBUG_PACKET_FORWARDING
			fprintf(stdout, "%.9f\tMDART::recv()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
			fprintf(stdout, "DROP_RTR_TTL\n");
#endif
			drop(recvPkt_, DROP_RTR_TTL);
			return;
		}

	}
	if (ch->ptype() == PT_MDART) {
		recvMDART(recvPkt_);
		return;
	}
	else {
		// If I'm originating the packet
		if (ch->num_forwards() == 0) {
			format(recvPkt_);
		}
		else {
			forward(recvPkt_);
			return;
		}
	}
}


void MDART::macFailed(Packet* recvPkt_) {
#ifdef DEBUG_PACKET_FORWARDING
	struct hdr_cmn* recvPktCh_ = HDR_CMN(recvPkt_);
	struct hdr_ip *recvPktIh_ = HDR_IP(recvPkt_);
	struct hdr_mdart_encp *recvPktRh_ = HDR_MDART_ENCP(recvPkt_);
	fprintf(stdout, "%.9f\tMDART::macFailed())\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
	fprintf(stdout, "\trecvPktCh_->next_hop = %d\n", recvPktCh_->next_hop());
	fprintf(stdout, "\trecvPktIh_->saddr = %d\n", recvPktIh_->saddr());
	fprintf(stdout, "\trecvPktIh_->daddr = %d\n", recvPktIh_->daddr());
	fprintf(stdout, "\trecvPktRh_->srcId_ = %d\n", recvPktRh_->srcId_);
	fprintf(stdout, "\trecvPktRh_->srcAdd_ = %s\n", bitString(recvPktRh_->srcAdd_));
	fprintf(stdout, "\trecvPktRh_->dstId_ = %d\n", recvPktRh_->dstId_);
	fprintf(stdout, "\trecvPktRh_->dstAdd_ = %s\n", bitString(recvPktRh_->dstAdd_));
	fprintf(stdout, "\trecvPktRh_->txCount_ = %d\n", recvPktRh_->txCount_);
	fprintf(stdout, "\trecvPktCh_->xmit_reason_ = %d\n", recvPktCh_->xmit_reason_);
	routingTable_->print();
// DHT
//	lookupTablePrint();
#endif
	if (macFailed_ == 1) {
//#ifdef MAC_FAILED_RECOVERY
#ifdef DEBUG_PACKET_FORWARDING
		routingTable_->print();
#endif // DEBUG_PACKET_FORWARDING
		struct hdr_cmn* recvPktCh_ = HDR_CMN(recvPkt_);
		struct hdr_mdart_encp *recvPktRh_ = HDR_MDART_ENCP(recvPkt_);
		routingTable_->macFailed(recvPktCh_->next_hop());
		if (recvPktRh_->txCount_++ < MAC_FAILED_ALLOWED_LOSS)
			forward(recvPkt_);
		else
			drop(recvPkt_, "DROP_RTR_TX_COUNT");
	} else {
//#else // MAC_FAILED_RECOVERY
		drop(recvPkt_, DROP_RTR_MAC_CALLBACK);
	}
//#endif
}


void MDART::format(Packet* sendPkt_) {
#ifdef DEBUG_ADP
	fprintf(stdout, "%.9f\tMDART::format()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
#endif
	struct hdr_cmn* sendPktCh_ = HDR_CMN(sendPkt_);
	struct hdr_ip* sendPktIh_ = HDR_IP(sendPkt_);
	struct hdr_mdart_encp* sendPktRh_ = HDR_MDART_ENCP(sendPkt_);
	nsaddr_t dstAdd_ = (nsaddr_t) adp_->findAdd(sendPktIh_->daddr());
	sendPktRh_->dstAdd_ = dstAdd_;
	sendPktRh_->dstId_ = sendPktIh_->daddr();
	sendPktRh_->srcAdd_ = address_;
	sendPktRh_->srcId_ = id_;
#ifdef DEBUG_ADP
	fprintf(stdout, "\tsendPktRh_->dstId_ = %d\n", sendPktRh_->dstId_);
	fprintf(stdout, "\tsendPktRh_->srcAdd_ = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->dstAdd_ = %s\n", bitString(sendPktRh_->dstAdd_));
	fprintf(stdout, "\tsendPktRh_->srcId_ = %d\n", sendPktRh_->srcId_);
#endif
	if (dstAdd_ == (nsaddr_t) IP_BROADCAST) {
#ifdef DEBUG_PACKET_FORWARDING
		fprintf(stdout, "\tDestination dynamic address unknown for node %d\n", sendPktIh_->daddr());
#endif
		queue_->enque(sendPkt_);
		adp_->sendDarq(sendPktIh_->daddr(), sendPktCh_->uid());
		return;
	}
#ifdef DEBUG_PACKET_FORWARDING
	fprintf(stdout, "\tDestination dynamic address %s for node %d\n", bitString(dstAdd_), sendPktIh_->daddr());
#endif
	forward(sendPkt_);
}


void MDART::forward(Packet* sendPkt_) {
#ifdef DEBUG_PACKET_FORWARDING
	fprintf(stdout, "%.9f\tMDART::forward()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
#endif
	struct hdr_cmn* sendPktCh_ = HDR_CMN(sendPkt_);
	struct hdr_ip* sendPktIh_ = HDR_IP(sendPkt_);
	struct hdr_mdart_encp* sendPktRh_ = HDR_MDART_ENCP(sendPkt_);
#ifdef DEBUG_PACKET_FORWARDING
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", sendPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", sendPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcAdd_ = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->dstAdd_ = %s\n", bitString(sendPktRh_->dstAdd_));
	fprintf(stdout, "\tsendPktRh_->srcId_ = %d\n", sendPktRh_->srcId_);
	fprintf(stdout, "\tsendPktRh_->dstId_ = %d\n", sendPktRh_->dstId_);
	adp_->printDHT();
	routingTable_->print();
#endif
	if (sendPktCh_->direction() == hdr_cmn::UP && ((u_int32_t)sendPktIh_->daddr() == IP_BROADCAST || sendPktIh_->daddr() == id_)) {
#ifdef DEBUG_PACKET_FORWARDING
		fprintf(stdout, "%.9f\tMDART::forward()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
		fprintf(stdout, "!!!\tdmux\n");
#endif
		dmux_->recv(sendPkt_, (Handler *) 0);
		return;
	}
	sendPktCh_->direction() = hdr_cmn::DOWN;
	sendPktCh_->addr_type() = NS_AF_INET;
	if ((u_int32_t)sendPktIh_->daddr() == IP_BROADCAST) {
#ifdef DEBUG_PACKET_FORWARDING
		fprintf(stdout, "%.9f\tMDART::forward()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
		fprintf(stdout, "\tpacket dropped: DROP_IP_BROADCAST\n");
		adp_->printDHT();
		routingTable_->print();
#endif
		drop(sendPkt_, "DROP_IP_BROADCAST");
		return;
	}
#ifdef ADP_DATA_CACHING
	adp_->addEntry(sendPktRh_->dstId_, sendPktRh_->dstAdd_);
	adp_->addEntry(sendPktRh_->srcId_, sendPktRh_->srcAdd_);
	adp_->addEntry(sendPktRh_->forId_, sendPktRh_->forAdd_);
#endif
#ifdef DEBUG_PACKET_FORWARDING
	fprintf(stdout, "\tADP_DHT::findAdd(%d) = %s\n", sendPktIh_->daddr(), bitString(sendPktRh_->dstAdd_));
#endif
	if (sendPktRh_->dstAdd_ == address_) {
#ifdef DEBUG_PACKET_FORWARDING
		fprintf(stdout, "%.9f\tMDART::forward()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
		fprintf(stdout, "\tpacket dropped: DROP_RTR_DADD\n");
		adp_->printDHT();
		routingTable_->print();
#endif
		drop(sendPkt_, "DROP_RTR_DADD");
		return;
	}
	nsaddr_t nextHop_ = routingTable_->getEntry(sendPktRh_->dstAdd_);
	if ((u_int32_t) nextHop_ == IP_BROADCAST) {
#ifdef DEBUG_PACKET_FORWARDING
		fprintf(stdout, "%.9f\tMDART::forward()\t\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
		fprintf(stdout, "\tpacket dropped: DROP_RTR_ADPNRT\tDAddress_=%s\tnextHop=%d\n", bitString(sendPktRh_->dstAdd_), nextHop_);
		adp_->printDHT();
		routingTable_->print();
#endif
		drop(sendPkt_, "DROP_RTR_ADPNRT");
		return;
	}
	sendPktCh_->next_hop() = nextHop_;
	sendPktRh_->forAdd_ = address_;
	sendPktRh_->forId_ = id_;
	sendPktCh_->xmit_failure_ = macFailedCallback;
	sendPktCh_->xmit_failure_data_ = (void*)this;
#ifdef DEBUG_PACKET_FORWARDING
	fprintf(stdout, "\tselected nextHop = %d\n", sendPktCh_->next_hop());
	fprintf(stdout, "\tsendPktIh_->saddr = %d\n", sendPktIh_->saddr());
	fprintf(stdout, "\tsendPktIh_->daddr = %d\n", sendPktIh_->daddr());
	fprintf(stdout, "\tsendPktRh_->srcAdd_ = %s\n", bitString(sendPktRh_->srcAdd_));
	fprintf(stdout, "\tsendPktRh_->dstAdd_ = %s\n", bitString(sendPktRh_->dstAdd_));
	fprintf(stdout, "\tsendPktRh_->srcId_ = %d\n", sendPktRh_->srcId_);
#endif
	Scheduler::instance().schedule(target_, sendPkt_, 0.0);
}



void MDART::recvMDART(Packet* recvPkt_) {
#ifdef DEBUG
	fprintf(stdout, "%.9f\t%s\t\t\tin node %d\twith address %s\n",  CURRENT_TIME, __FUNCTION__, id_, bitString(address_));
#endif
	struct hdr_mdart* recvPktRh_ = HDR_MDART(recvPkt_);
	switch(recvPktRh_->type_) {
		case MDART_TYPE_HELLO:
		{
#ifdef ADP_HELLO_CACHING
			struct hdr_mdart_hello* recvPktRhHello_ = HDR_MDART_HELLO(recvPkt_);
			adp_->addEntry(recvPktRhHello_->srcId_, recvPktRhHello_->srcAdd_);
#endif
			ndp_->recvHello(recvPkt_);
			break;
		}
		case MDART_TYPE_DARQ:
			adp_->recvDarq(recvPkt_);
			break;
		case MDART_TYPE_DARP:
			adp_->recvDarp(recvPkt_);
			break;
		case MDART_TYPE_DAUP:
			adp_->recvDaup(recvPkt_);
			break;
		case MDART_TYPE_DABR:
			adp_->recvDabr(recvPkt_);
			break;
/*		case MDART_TYPE_INST:
			idp_->recvInfo(recvPkt_);
			break;
		case MDART_TYPE_INRQ:
			idp_->recvRequestInfo(recvPkt_);
			break;
		case MDART_TYPE_INRP:
			idp_->recvReplyInfo(recvPkt_);
			break;*/
		default:
#ifdef DEBUG
			fprintf(stderr, "!!!\tinvalid MDART type (%x)\n", recvPktRh_->type_);
#endif
			exit(1);
	}
}






/*******************************************************************************
 * MDART Lookup Table management functions
 ******************************************************************************/
/*void MDART::lookupTableClear() {
	MDART::lookupTable_.clear();
}

void MDART::lookupTableRmEntry(nsaddr_t uid_) {
	MDART::lookupTable_.erase(uid_);
}

void MDART::lookupTableAddEntry(nsaddr_t uid, nsaddr_t address) {
#ifdef DEBUG
	fprintf(stdout, "%.9f\tMDART::lookupTableAddEntry(%d, %s)\tin node %d\twith address %s\n", CURRENT_TIME, uid, bitString(address), id_, bitString(address_));
#endif
	MDART::lookupTable_[uid] = address;
}

nsaddr_t MDART::lookupTableLookEntry(nsaddr_t uid_) {
	lookupTable::iterator it = MDART::lookupTable_.find(uid_);
	if (it == MDART::lookupTable_.end())
		return DATYPE_BROADCAST;
	else
		return (*it).second;
}

nsaddr_t MDART::lookupTableLookUid(nsaddr_t address_) {
	lookupTable::const_iterator itEntry_;
	for(itEntry_ = MDART::lookupTable_.begin(); itEntry_ != MDART::lookupTable_.end(); ++itEntry_) {
		if ((*itEntry_).second == address_) {
			return (*itEntry_).first;
		}
	}
	return IP_BROADCAST;
}

u_int32_t MDART::lookupTableSize() {
	return MDART::lookupTable_.size();
}

void MDART::lookupTablePrint() {
	fprintf(stdout, "\tMDART::lookupTablePrint()\t\t\tin node %d\twith address %s\n", id_, bitString(address_));
	for (lookupTable::iterator it = MDART::lookupTable_.begin(); it != MDART::lookupTable_.end(); it++) {
		fprintf(stdout, "\t\tnode id = %d\tnode address = %s\n", (*it).first, bitString((*it).second));
	}
}
*/



//------------------------------------------------------------------------------
// Address & routing table functions
//------------------------------------------------------------------------------
void	MDART::validateAddress() {
#ifdef DEBUG_ADDRESS_ALLOCATION
	fprintf(stdout, "%.9f\tMDART::validateAddress()\t\t\tin node %d\twith address %s\n", CURRENT_TIME, id_, bitString(address_));
//	lookupTablePrint();
#endif
	if (!ndp_->validateAddress()) {
//		ndp_->neighborPrint();
		routingTable_->clear();
		oldAddress_ = address_;
		ndp_->selectAddress();
#ifdef DEBUG_ADDRESS_ALLOCATION		
		fprintf(stdout, "\tNew address is = %s\n", bitString(address_));
		routingTable_->print();
#endif
		if (logtarget_ != 0) {
			sprintf(logtarget_->pt_->buffer(), "x -t %.9f mdart invalidAddress -Hs %d -Ds %s -Rt %d", CURRENT_TIME, id_, bitString(address_), routingTable_->size());
			logtarget_->pt_->dump();
		}
#ifdef FAST_ADDRESS_CONVERGENCE
		ndp_->sendHello();
#endif
	}
	routingTable_->clear();
}
