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



#include <mdart/mdart_table.h>
#include <mdart/mdart_function.h>



//------------------------------------------------------------------------------
// MDART routing table management functions
//------------------------------------------------------------------------------
void RoutingTable::clear() {
#ifdef DEBUG_TABLE
	fprintf(stdout, "\t\tRoutingTable::clear\t\t\tin node %d\twith address %s.\n", mdart_->id_, bitString(mdart_->address_));
#endif
	unsigned int i;
	assert(table_);
	for(i=0; i<ADDR_SIZE; i++) {
		entrySet::iterator itEntry_;
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
			delete (*itEntry_);
			table_->at(i)->erase(itEntry_);
		}
	}
}

void RoutingTable::clear(u_int32_t levelSibling_) {
#ifdef DEBUG_TABLE
	fprintf(stdout, "\t\tclear(%d)\t\t\tin node %d\twith address %s.\n", levelSibling_, mdart_->id_, bitString(mdart_->address_));
#endif
	assert(table_);
	entrySet::iterator itEntry_ = table_->at(levelSibling_)->begin();
	while (itEntry_ != table_->at(levelSibling_)->end()) {
		entrySet::iterator nextEntry_ = itEntry_;
		++nextEntry_;
		table_->at(levelSibling_)->erase(itEntry_);
		delete (*itEntry_);
		itEntry_ = nextEntry_;
	}
}

void RoutingTable::purge(nsaddr_t id) {
#ifdef DEBUG_TABLE
	fprintf(stdout, "%.9f\tRoutingTable::purge()\t\t\tin node %d\twith address %s.\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	unsigned int i;
	assert(table_);
	for(i=0; i<ADDR_SIZE; i++) {
		entrySet::iterator itEntry_;
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
#ifdef DEBUG_TABLE
			fprintf(stdout, "\tlevel = %d\tnextHop add = %s\n", i, bitString((*itEntry_)->nextHopAdd()));
#endif
			if ((*itEntry_)->nextHopId() == id) {
#ifdef DEBUG_TABLE
				fprintf(stdout, "\tEntry deleted\n");
#endif
				delete (*itEntry_);
				table_->at(i)->erase(itEntry_);
			}
		}
	}
}

string RoutingTable::getUpdate() {
#ifdef DEBUG_TABLE
	fprintf(stdout, "%.9f\tRoutingTable::getUpdate()\t\t\tin node %d\twith address %s.\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
#endif
	string string_;
	int i;
	for (i=ADDR_SIZE-1; i>-1; i--) {
		ostringstream stream_;
		nsaddr_t networkId_ = INFINITO;
		u_int32_t hopNumber_ = INFINITO;
		double etxMetric_ = (double) RTR_ETX_MAX;
		bitset<ADDR_SIZE> routeLog_;
		routeLog_.reset();
		entrySet::const_iterator itEntry_;
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
			if (((*itEntry_)->networkId() < networkId_) || (((*itEntry_)->networkId() == networkId_) && ((mdart_->etxMetric_ == 1 && (*itEntry_)->etxMetric() < etxMetric_) || (mdart_->etxMetric_ == 0 && (*itEntry_)->hopNumber() < hopNumber_)))) {			
				networkId_ = (*itEntry_)->networkId();
				hopNumber_ = (*itEntry_)->hopNumber();
				etxMetric_ = (*itEntry_)->etxMetric();
				routeLog_ = (*itEntry_)->routeLog();
			}
		}
//		if (networkId_ != INFINITO) {
//			stream_ << i;
//			stream_ << ',';
			stream_ << networkId_;
			stream_ << ',';
			stream_ << hopNumber_;
			stream_ << ',';
			stream_ << etxMetric_;
			stream_ << ',';
			stream_ << routeLog_.to_ulong();
			stream_ << ';';
			string_ += stream_.str();
//		}
	}
#ifdef DEBUG_TABLE
	print();
	fprintf(stdout, "\tdata = %s\n", string_.c_str());
#endif
	return string_;
}


void RoutingTable::setUpdate(Neighbor* neighbor_) {
#ifdef DEBUG_TABLE
	fprintf(stdout, "\tRoutingTable::setUpdate\t\t\tin node %d\twith address %s.\n", mdart_->id_, bitString(mdart_->address_));
#endif
	int levelSibling_ = DiffBit(mdart_->address_, neighbor_->address());
#ifdef DEBUG_TABLE
	fprintf(stdout, "\t\tneighbor id = %d\taddress = %s\tlevelSibling = %d\n", neighbor_->id(), bitString(neighbor_->address()), levelSibling_);
#endif
	if (levelSibling_ == -1) {
#ifdef DEBUG_TABLE
		fprintf(stdout, "RoutingTable::setUpdate\t\t\tin node %d\twith address %s.\n", mdart_->id_, bitString(mdart_->address_));
		fprintf(stdout, "\tDADD address with the neighbor");
#endif
		return;
	}
#ifdef DEBUG_TABLE
	if(neighbor_->levelId(levelSibling_ - 1) > id(levelSibling_)) {
		fprintf(stdout, "\t\t\tneighbor_->levelId(%d)=%d > id(%d)=%d\n", levelSibling_ - 1, neighbor_->levelId(levelSibling_ - 1), levelSibling_, id(levelSibling_));
	}
#endif
	if (neighbor_->levelId(levelSibling_ - 1) <= id(levelSibling_)) {
#ifdef DEBUG_TABLE
		fprintf(stdout, "\t\t\tneighbor_->levelId(%d)=%d <= id(%d)=%d\n", levelSibling_ - 1, neighbor_->levelId(levelSibling_ - 1), levelSibling_, id(levelSibling_));
#endif
		bool entryPresent_ = false;
		entrySet::iterator itEntry_ = table_->at(levelSibling_)->begin();
		for (itEntry_ = table_->at(levelSibling_)->begin(); itEntry_ != table_->at(levelSibling_)->end(); ++itEntry_) {
#ifdef DEBUG_TABLE
			if ((*itEntry_)->nextHopAdd() != neighbor_->address()) {
				fprintf(stdout, "\t\t\tEntry not present\n");
			}
			else if (neighbor_->levelId(levelSibling_ - 1) < id(levelSibling_)) {
				fprintf(stdout, "\t\t\tEntry with id(%d)=%d < neighbor_->levelId(%d)=%d already present\n", levelSibling_, id(levelSibling_), levelSibling_ - 1, neighbor_->levelId(levelSibling_ - 1));
			}
#endif
			if ((*itEntry_)->nextHopAdd() == neighbor_->address() && neighbor_->levelId(levelSibling_ - 1) >= id(levelSibling_)) {
#ifdef DEBUG_TABLE
				fprintf(stdout, "\t\t\tEntry with id(%d)=%d <= neighbor_->levelId(%d)=%d already present\n", levelSibling_, id(levelSibling_), levelSibling_ - 1, neighbor_->levelId(levelSibling_ - 1));
#endif
				entryPresent_ = true;
			}
		}
/*#ifdef RTR_METRIC_ETX
//		if (!entryPresent_ && neighbor_->etxMetric() < RTR_ETX_MAX) {
		if (!entryPresent_) {
#else
		if (!entryPresent_) {
#endif*/
		if (!entryPresent_) {
			if ((mdart_->etxMetric_ == 1 && neighbor_->etxMetric() + 1 < RTR_ETX_MAX) || mdart_->etxMetric_ == 0) {
#ifdef DEBUG_TABLE
				fprintf(stdout, "\t\t\tneighbor_->etxMetric() = %f\n", neighbor_->etxMetric());
#endif
				bitset<ADDR_SIZE> routeLog_;
				routeLog_.reset();
				routeLog_.set(levelSibling_);
				addEntry(levelSibling_, neighbor_->address(), neighbor_->id(), neighbor_->levelId(levelSibling_ - 1), 1, neighbor_->etxMetric(), routeLog_);
			}
		}
	}
	int i;
	for(i=ADDR_SIZE-1; i>levelSibling_; i--) {
#ifdef DEBUG_TABLE
		fprintf(stdout, "\t\ti = %d\n", i);
#endif
		if ((neighbor_->routeLog(i, levelSibling_) == 0) && neighbor_->entryPresent(i)) {
#ifdef DEBUG_TABLE
			if(neighbor_->networkId(i) > networkId(i)) {
				fprintf(stdout, "\t\t\tneighbor_->networkId(%d)=%d > networkId(%d)=%d\n", i, neighbor_->networkId(i), i, networkId(i));
			}
#endif
			if (neighbor_->networkId(i) < networkId(i)) {
#ifdef DEBUG_TABLE
				fprintf(stdout, "\t\t\tneighbor_->networkId(%d)=%d < networkId(%d)=%d\trouteLog = %s\n", i, neighbor_->networkId(i), i, networkId(i), neighbor_->routeLog(i).to_string().c_str());
#endif
				clear(i);
			}
/*#ifdef RTR_METRIC_ETX
//			if(neighbor_->networkId(i) <= networkId(i) && (neighbor_->linkQuality(i) + neighbor_->etxMetric() < RTR_ETX_MAX)) {
			if(neighbor_->networkId(i) <= networkId(i)) {
#else
			if(neighbor_->networkId(i) <= networkId(i)) {
#endif*/
			if(neighbor_->networkId(i) <= networkId(i)) {
				if ((mdart_->etxMetric_ == 1 && neighbor_->etxMetric() + neighbor_->etxMetric(i) < RTR_ETX_MAX) || mdart_->etxMetric_ == 0) {
#ifdef DEBUG_TABLE
					fprintf(stdout, "\t\t\tneighbor_->networkId(%d)=%d <= networkId(%d)=%d && neighbor_->linkQuality(%d) + neighbor_->etxMetric() = %f\n", i, neighbor_->networkId(i), i, networkId(i), i, neighbor_->linkQuality(i) + neighbor_->etxMetric());
#endif
					bitset<ADDR_SIZE> routeLog_ = neighbor_->routeLog(i);
					routeLog_.set(levelSibling_);
					int j;
					for(j=0; j<levelSibling_; j++) {
						routeLog_.reset(j);
					}
					addEntry(i, neighbor_->address(), neighbor_->id(), neighbor_->networkId(i), neighbor_->hopNumber(i) + 1, neighbor_->etxMetric(i) + neighbor_->etxMetric(), routeLog_);
				}
			}
		}
	}
#ifdef DEBUG_TABLE
	print();
#endif
}

void RoutingTable::macFailed(nsaddr_t id) {
#ifdef DEBUG_PACKET_FORWARDING
	fprintf(stdout, "%.9f\tRoutingTable::macFailed(%d)\t\tin node %d\twith address %s\n", CURRENT_TIME, id, mdart_->id_, bitString(mdart_->address_));
#endif
	unsigned int i;
	for(i=0; i<ADDR_SIZE; i++) {
		entrySet::iterator itEntry_;
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
#ifdef DEBUG_PACKET_FORWARDING
			fprintf(stdout, "\tlevel = %d\tentry nextHopAdd = %s\n", i, bitString((*itEntry_)->nextHopAdd()));
#endif
			if ((*itEntry_)->nextHopId() == id) {
#ifdef DEBUG_PACKET_FORWARDING
				fprintf(stdout, "\tEntry invalided\n");
#endif
				(*itEntry_)->macFailed(true);
				(*itEntry_)->upLinkQuality();
			}
			else {
#ifdef MAC_FAILED_RECOVERY_FAST
				(*itEntry_)->macFailed(false);
#endif
			}
		}
	}
}


//------------------------------------------------------------------------------
// Routing table data access functions
//------------------------------------------------------------------------------
nsaddr_t RoutingTable::levelId(u_int32_t levelSibling) {
	nsaddr_t id = mdart_->id_;
	int i;
	for (i=levelSibling; i>-1; i--) {
		entrySet::iterator itEntry_;
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
//			if ((*itEntry_)->id() < mdart_->id_)
//				id_ = (*itEntry_)->id();
			if ((*itEntry_)->networkId() < id)
				id = (*itEntry_)->networkId();
		}
	}
	return id;
}



nsaddr_t RoutingTable::id(u_int32_t levelSibling) {
//	nsaddr_t id_ = mdart_->id();
	nsaddr_t id_ = INFINITO;
	entrySet::iterator itEntry_;
	for (itEntry_ = table_->at(levelSibling)->begin(); itEntry_ != table_->at(levelSibling)->end(); ++itEntry_) {
//		nsaddr_t id_ = mdart_->id();
		if ((*itEntry_)->networkId() < id_)
			id_ = (*itEntry_)->networkId();
	}
	return id_;
}

nsaddr_t RoutingTable::networkId(u_int32_t levelSibling) {
	nsaddr_t networkId_ = INFINITO;
	entrySet::iterator itEntry_;
	for (itEntry_ = table_->at(levelSibling)->begin(); itEntry_ != table_->at(levelSibling)->end(); ++itEntry_) {
		if ((*itEntry_)->networkId() < networkId_)
			networkId_ = (*itEntry_)->networkId();
	}
	return networkId_;
}

u_int32_t RoutingTable::hopNumber(u_int32_t levelSibling) {
//#ifdef DEBUG
//	fprintf(stdout, "%.9f\tRoutingTable::cost(%d)\t\t\tin node %d\twith address %s.\n", CURRENT_TIME, levelSibling, mdart_->id(), bitString(mdart_->address()));
//#endif
	u_int32_t hopNumber_ = INFINITO;
	entrySet::iterator itEntry_;
	for (itEntry_ = table_->at(levelSibling)->begin(); itEntry_ != table_->at(levelSibling)->end(); ++itEntry_) {
		if ((*itEntry_)->hopNumber() < hopNumber_)
			hopNumber_ = (*itEntry_)->hopNumber();
	}
//#ifdef DEBUG
//	fprintf(stdout, "\tCost is %d.\n", cost_);
//#endif
	return hopNumber_;
}

double RoutingTable::etxMetric(u_int32_t levelSibling) {
	double etxMetric_ = RTR_ETX_MAX;
	entrySet::iterator itEntry_;
	for (itEntry_ = table_->at(levelSibling)->begin(); itEntry_ != table_->at(levelSibling)->end(); ++itEntry_) {
		if ((*itEntry_)->etxMetric() < etxMetric_)
			etxMetric_ = (*itEntry_)->etxMetric();
	}
//#ifdef DEBUG
//	fprintf(stdout, "\tCost is %d.\n", cost_);
//#endif
	return etxMetric_;
}

bitset<ADDR_SIZE> RoutingTable::routeLog(u_int32_t levelSibling) {
//#ifdef DEBUG
//	fprintf(stdout, "%.9f\tRoutingTable::routeLog(%d)\t\t\tin node %d\twith address %s.\n", CURRENT_TIME, levelSibling, mdart_->id(), bitString(mdart_->address()));
//#endif
	bitset<ADDR_SIZE> routeLog_;
	routeLog_.reset();
	entrySet::iterator itEntry_;
	for (itEntry_ = table_->at(levelSibling)->begin(); itEntry_ != table_->at(levelSibling)->end(); ++itEntry_) {
		routeLog_ = (*itEntry_)->routeLog();
	}
	return routeLog_;
}



//------------------------------------------------------------------------------
// MDART Routing Table entries management functions
//------------------------------------------------------------------------------
void RoutingTable::addEntry(int levelSibling, nsaddr_t nextHop, nsaddr_t id, nsaddr_t networkId, u_int32_t hopNumber, double linkQuality, bitset<ADDR_SIZE> routeLog) {
#ifdef DEBUG_TABLE
	fprintf(stdout, "%.9f\tRoutingTable::addEntry()\t\tin node %d\twith address %s.\n", CURRENT_TIME, mdart_->id_, bitString(mdart_->address_));
	fprintf(stdout, "\tnextHop = %s\tid = %d\tnetworkId = %d\thopNumber = %d\tlinkQuality = %f\trouteLog = %s\n", bitString(nextHop), id, networkId, hopNumber, linkQuality, routeLog.to_string().c_str());
#endif
	if (linkQuality < 1.0) {
		fprintf(stdout, "\t\tlinkQuality() = %f\n", linkQuality);
	}
	RoutingEntry *tempEntry_ = new RoutingEntry(nextHop, id, networkId, hopNumber, linkQuality, routeLog);
	assert(tempEntry_);
	table_->at(levelSibling)->insert(tempEntry_);
}

nsaddr_t RoutingTable::getEntry(nsaddr_t dst_) {
#ifdef DEBUG_PACKET_FORWARDING
	fprintf(stdout, "\tRoutingTable::getEntry(%s)\tin node %d\twith address %s.\n", bitString(dst_), mdart_->id_, bitString(mdart_->address_));
#endif
	int levelSibling_ = ADDR_SIZE;
	nsaddr_t nextHopId_ = IP_BROADCAST;
#ifdef DEBUG_PACKET_FORWARDING
	nsaddr_t nextHopAdd_ = IP_BROADCAST;
#endif
	u_int32_t hopNumber_ = INFINITO;
	double etxMetric_ = (double) RTR_ETX_MAX;
	if (DiffBit(mdart_->address_, dst_) <0) {
#ifdef DEBUG_PACKET_FORWARDING
		fprintf(stdout, "\tDADD address with the destination");
#endif
		return nextHopId_;
	}
	entrySet::const_iterator itEntry_;
	for(int i=DiffBit(mdart_->address_, dst_); i<ADDR_SIZE; i++) {
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
#ifdef DEBUG_PACKET_FORWARDING
			fprintf(stdout, "\t\ttemp\ti = %d\tnexthop id_ = %d\tnexthop add_ = %s\tlevelSibling = %d\n", i, (*itEntry_)->nextHopId(), bitString((*itEntry_)->nextHopAdd()), DiffBit(dst_, (*itEntry_)->nextHopAdd()));
#endif
#ifdef DEBUG_PACKET_FORWARDING
			if (DiffBit(dst_, (*itEntry_)->nextHopAdd()) > levelSibling_) {
				fprintf(stdout, "\t\t\t(DiffBit(%s, %s)=%d > %d\n", bitString(dst_), bitString((*itEntry_)->nextHopAdd()), DiffBit(dst_, (*itEntry_)->nextHopAdd()), levelSibling_);
			}
			if ((*itEntry_)->macFailed()) {
				fprintf(stdout, "\t\t\t(*itEntry_)->macFailed()\n");
			}
			if ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->hopNumber() < hopNumber_)) {
				fprintf(stdout, "\t\t\t(*itEntry_)->hopNumber()=%d < hopNumber_=%d)\n", (*itEntry_)->hopNumber(), hopNumber_);
			}
#endif
/*#ifdef RTR_METRIC_ETX
//			if (((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->etxMetric() < etxMetric_))) && !((*itEntry_)->macFailed())) {
			if ((*itEntry_)->etxMetric() < etxMetric_ && !(*itEntry_)->macFailed()) {
#else
			if (((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->hopNumber() < hopNumber_))) && !((*itEntry_)->macFailed())) {
#endif*/
			if ((mdart_->etxMetric_ == 1 && ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && (*itEntry_)->etxMetric() < etxMetric_)) && !(*itEntry_)->macFailed()) || (mdart_->etxMetric_ == 0 && ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->hopNumber() < hopNumber_))) && !((*itEntry_)->macFailed()))) {
				levelSibling_ = DiffBit(dst_, (*itEntry_)->nextHopAdd());
				nextHopId_ = (*itEntry_)->nextHopId();
#ifdef DEBUG_PACKET_FORWARDING
				nextHopAdd_ = (*itEntry_)->nextHopAdd();
#endif
				hopNumber_ = (*itEntry_)->hopNumber();
				etxMetric_ = (*itEntry_)->etxMetric();
#ifdef DEBUG_PACKET_FORWARDING
				fprintf(stdout, "\t\t\ttemp selected nextHop_ id = %d address = %s\n", nextHopId_, bitString(nextHopAdd_));
#endif
			}
		}
#ifdef DEBUG_PACKET_FORWARDING
			fprintf(stdout, "\t\tselected nextHop_ id = %d address = %s\n", nextHopId_, bitString(nextHopAdd_));
#endif
		if ((u_int32_t) nextHopId_ != IP_BROADCAST) {
			return nextHopId_;
		}
	}
	return nextHopId_;
}

nsaddr_t RoutingTable::DAGetEntry(nsaddr_t dst_) {
#ifdef DEBUG_ADP
	fprintf(stdout, "\tRoutingTable::DaGetEntry(%s)\tin node %d\twith address %s.\n", bitString(dst_), mdart_->id_, bitString(mdart_->address_));
#endif
	int levelSibling_ = ADDR_SIZE;
	nsaddr_t nextHopId_ = IP_BROADCAST;
#ifdef DEBUG_ADP
	nsaddr_t nextHopAdd_ = IP_BROADCAST;
#endif
	u_int32_t hopNumber_ = INFINITO;
	double etxMetric_ = (double) RTR_ETX_MAX;
	if (DiffBit(mdart_->address_, dst_) <0) {
#ifdef DEBUG_ADP
		fprintf(stdout, "\tDADD address with the destination");
#endif
		return nextHopId_;
	}
	entrySet::const_iterator itEntry_;
	int i=DiffBit(mdart_->address_, dst_);
	for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
#ifdef DEBUG_ADP
		if (mdart_->id_ == 19)
			fprintf(stdout, "\t\ttemp\ti = %d\tnexthop id_ = %d\tnexthop add_ = %s\tlevelSibling = %d\n", i, (*itEntry_)->nextHopId(), bitString((*itEntry_)->nextHopAdd()), DiffBit(dst_, (*itEntry_)->nextHopAdd()));
#endif
#ifdef DEBUG_ADP
		if (DiffBit(dst_, (*itEntry_)->nextHopAdd()) > levelSibling_) {
			fprintf(stdout, "\t\t\t(DiffBit(%s, %s)=%d > %d\n", bitString(dst_), bitString((*itEntry_)->nextHopAdd()), DiffBit(dst_, (*itEntry_)->nextHopAdd()), levelSibling_);
		}
		if ((*itEntry_)->macFailed()) {
			fprintf(stdout, "\t\t\t(*itEntry_)->macFailed()\n");
		}
		if ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->hopNumber() < hopNumber_)) {
			fprintf(stdout, "\t\t\t(*itEntry_)->hopNumber()=%d < hopNumber_=%d)\n", (*itEntry_)->hopNumber(), hopNumber_);
		}
#endif
/*#ifdef RTR_METRIC_ETX
		if (((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->etxMetric() < etxMetric_))) && !((*itEntry_)->macFailed())) {
#else
		if (((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->hopNumber() < hopNumber_))) && !((*itEntry_)->macFailed())) {
#endif*/
		if ((mdart_->etxMetric_ == 1 && ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && (*itEntry_)->etxMetric() < etxMetric_)) && !(*itEntry_)->macFailed()) || (mdart_->etxMetric_ == 0 && ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) < levelSibling_) || ((DiffBit(dst_, (*itEntry_)->nextHopAdd()) == levelSibling_) && ((*itEntry_)->hopNumber() < hopNumber_))) && !((*itEntry_)->macFailed()))) {
			levelSibling_ = DiffBit(dst_, (*itEntry_)->nextHopAdd());
			nextHopId_ = (*itEntry_)->nextHopId();
#ifdef DEBUG_ADP
			nextHopAdd_ = (*itEntry_)->nextHopAdd();
#endif
			hopNumber_ = (*itEntry_)->hopNumber();
			etxMetric_ = (*itEntry_)->etxMetric();
#ifdef DEBUG_ADP
			if (mdart_->id_ == 19)
				fprintf(stdout, "\t\t\ttemp selected nextHop_ id = %d address = %s\n", nextHopId_, bitString(nextHopAdd_));
#endif
		}
	}
#ifdef DEBUG_ADP
	if (mdart_->id_ == 19)
		fprintf(stdout, "\t\tselected nextHop_ id = %d address = %s\n", nextHopId_, bitString(nextHopAdd_));
#endif
	if ((u_int32_t) nextHopId_ != IP_BROADCAST) {
		return nextHopId_;
	}
	return nextHopId_;
}



//------------------------------------------------------------------------------
// MDART Routing Table debug functions
//------------------------------------------------------------------------------
void RoutingTable::print() {
//#ifdef DEBUG_PRINT_TABLE
   fprintf(stdout, "\t\tRoutingTable::print()\t\tin node %d\twith address %s.\n", mdart_->id_, bitString(mdart_->address_));
//#endif
	unsigned int i;
	for (i=0; i<ADDR_SIZE; i++) {
		bitset<ADDR_SIZE> tempSibling_ (mdart_->address_);
		tempSibling_.flip(i);
//#ifdef DEBUG_PRINT_TABLE
		fprintf(stdout, "\t\t\tlevel sibling = %i\tsibling = %s\n", i, tempSibling_.to_string().c_str());
//#endif
		entrySet::iterator itEntry_;
//#ifdef DEBUG_PRINT_TABLE
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
			fprintf(stdout, "\t\t\t\tnextHop id = %d\tadd = %s\tnetworkId = %d\thopNumber = %d\tlinkQuality = %.9f\trouteLog = %s\tmacFailed = %d\n", (*itEntry_)->nextHopId(), bitString((*itEntry_)->nextHopAdd()), (*itEntry_)->networkId(), (*itEntry_)->hopNumber(), (*itEntry_)->etxMetric(), (*itEntry_)->routeLog().to_string().c_str(), (*itEntry_)->macFailed());
		}
//#endif
	}
}

u_int32_t RoutingTable::size() {
	unsigned int i;
	int size_ = 0;
	for(i=0; i<ADDR_SIZE; i++) {
		entrySet::iterator itEntry_;
		for (itEntry_ = table_->at(i)->begin(); itEntry_ != table_->at(i)->end(); ++itEntry_) {
			size_++;
		}
	}
//#ifdef DEBUG_TABLE
//			fprintf(stdout, "%.9f\tRoutingTable::size()\t\t\tin node %d\twith address %s is %d.\n", CURRENT_TIME, mdart_->id(), bitString(mdart_->address()), size_);
//#endif
	return size_;
}

