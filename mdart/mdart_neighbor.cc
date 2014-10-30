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



#include <mdart/mdart_neighbor.h>



//------------------------------------------------------------------------------
// Neighbor Routing Table update function
//------------------------------------------------------------------------------
void NeighborTable::update(string str_) {
#ifdef DEBUG_NEIGHBOR
	fprintf(stdout, "%.9f\tNeighborTable::update()\n", CURRENT_TIME);
	fprintf(stdout, "\tdata = %s\n", str_.c_str());
#endif
/*	for (int i=ADDR_SIZE-1; i>-1; i--) {
		networkId_[i] = INFINITO;
		hopNumber_[i] = INFINITO;
		etxMetric_[i] = RTR_ETX_MAX;
		bitset<ADDR_SIZE> routeLog;
		routeLog.reset();
		routeLog_[i] = routeLog;		
	}
	*/
	int i=ADDR_SIZE;
	string::size_type lastPos_ = str_.find_first_not_of(";", 0);
	string::size_type pos_ = str_.find_first_of(";", lastPos_);
//	if ((int) pos_ > -1) {
		while (string::npos != pos_ || string::npos != lastPos_) {
			string entry_ = str_.substr(lastPos_, pos_ - lastPos_);
//			string::size_type iLastPos_ = entry_.find_first_not_of(",", 0);
//			string::size_type iPos_ = entry_.find_first_of(",", iLastPos_);
//			int i = atoi(entry_.substr(iLastPos_, iPos_ - iLastPos_).c_str());
//			string::size_type networkIdLastPos_ = entry_.find_first_not_of(",", iPos_);
			string::size_type networkIdLastPos_ = entry_.find_first_not_of(",", 0);
			string::size_type networkIdPos_ = entry_.find_first_of(",", networkIdLastPos_);
			nsaddr_t networkId = atoi(entry_.substr(networkIdLastPos_, networkIdPos_ - networkIdLastPos_).c_str());
			string::size_type hopLastPos_ = entry_.find_first_not_of(",", networkIdPos_);
			string::size_type hopPos_ = entry_.find_first_of(",", hopLastPos_);
			u_int32_t hopNumber = atoi(entry_.substr(hopLastPos_, hopPos_ - hopLastPos_).c_str());
			string::size_type qualityLastPos_ = entry_.find_first_not_of(",", hopPos_);
			string::size_type qualityPos_ = entry_.find_first_of(",", qualityLastPos_);
			double etxMetric = atof(entry_.substr(qualityLastPos_, qualityPos_ - qualityLastPos_).c_str());
			string::size_type routeLogLastPos_ = entry_.find_first_not_of(",", qualityPos_);
			string::size_type routeLogPos_ = entry_.find_first_of(",", routeLogLastPos_);
			bitset<ADDR_SIZE> routeLog(atoi(entry_.substr(routeLogLastPos_, routeLogPos_ - routeLogLastPos_).c_str()));
			i--;
			networkId_[i] = networkId;
			hopNumber_[i] = hopNumber;
			etxMetric_[i] = etxMetric;
			routeLog_[i] = routeLog;
	#ifdef DEBUG_NEIGHBOR
			fprintf(stdout, "\t\tentry %d:\t%d\t%d\t%f\t%s\n", i, networkId_[i], hopNumber_[i], etxMetric_[i], routeLog_[i].to_string().c_str());
	#endif
			lastPos_ = str_.find_first_not_of(";", pos_);
			pos_ = str_.find_first_of(";", lastPos_);
//		}
	}
}



//------------------------------------------------------------------------------
// Neighbor routing table debug functions
//------------------------------------------------------------------------------
void NeighborTable::print(nsaddr_t address_) const {
	unsigned int i;
	for (i=0; i<ADDR_SIZE; i++) {
		if (hopNumber_[i] < INFINITO) {
			bitset<ADDR_SIZE> tempSibling_ (address_);
			tempSibling_.flip(i);
//#ifdef DEBUG_NEIGHBOR
			fprintf(stdout, "\t\t\tlevel sibling = %i\tsibling = %s\tnetworkId = %d\thopNumber = %d\tetxMetric = %f\trouteLog = %s\n", i, tempSibling_.to_string().c_str(), networkId_[i], hopNumber_[i], etxMetric_[i], routeLog_[i].to_string().c_str());
//#endif
		}
	}
}



//------------------------------------------------------------------------------
// Neighbor
//------------------------------------------------------------------------------
Neighbor::Neighbor(nsaddr_t id, nsaddr_t address, u_int32_t helloSeqNum) {
	id_ = id;
	address_ = address;
	expire_ = CURRENT_TIME + NDP_NEIGHBOR_EXPIRE;
	table_ = new NeighborTable();
	forLinkQuality_ = 0;
	addHello(helloSeqNum);
}



//------------------------------------------------------------------------------
// Neighbor functions
//------------------------------------------------------------------------------
/*
void Neighbor::updateDirectLinkQuality(string quality) {
	string::size_type lastPos_ = quality.find_first_not_of(";", 0);
	string::size_type pos_ = quality.find_first_of(";", lastPos_);
	while (string::npos != pos_ || string::npos != lastPos_) {
		string neighbor_ = quality.substr(lastPos_, pos_ - lastPos_);
		string::size_type idLastPos_ = neighbor_.find_first_not_of(",", 0);
		string::size_type idPos_ = neighbor_.find_first_of(",", idLastPos_);
		nsaddr_t id  = atoi(neighbor_.substr(idLastPos_, idPos_ - idLastPos_).c_str());
		string::size_type qualityLastPos_ = neighbor_.find_first_not_of(",", idPos_);
		string::size_type qualityPos_ = neighbor_.find_first_of(",", qualityLastPos_);
		double quality_ = atof(neighbor_.substr(qualityLastPos_, qualityPos_ - qualityLastPos_).c_str());
	#ifdef DEBUG_NDP
		fprintf(stdout, "\tneigh id %d\tquality %f\n", id_, quality_);
	#endif
		if (id_ == id) {
	#ifdef DEBUG_NDP
		fprintf(stdout, "\t\tinsert\n");
	#endif
			directLinkQuality_ = quality_;
			return;
		}
		lastPos_ = quality.find_first_not_of(";", pos_);
		pos_ = quality.find_first_of(";", lastPos_);
	}
}
*/

void Neighbor::purgeHello() {
#ifdef DEBUG_NEIGHBOR
	fprintf(stdout, "%.9f\tNeighbor::purgeHello()\n",CURRENT_TIME);
#endif
	helloVector::iterator entry_ = helloVector_.begin();
	while (entry_ != helloVector_.end()) {
		helloVector::iterator nextEntry_ = entry_;
		nextEntry_++;
		if ((*entry_).time_ + NDP_NEIGHBOR_EXPIRE <= CURRENT_TIME) {
			helloVector_.erase(entry_);
		}
		else 
			entry_ = nextEntry_;
	}
}



//------------------------------------------------------------------------------
// Neighbor g debufunctions
//------------------------------------------------------------------------------
void Neighbor::printHello() {
	helloVector::iterator entry_;
	fprintf(stdout, "\tNeighbor::printHello()\n");
	for(entry_ = helloVector_.begin(); entry_ != helloVector_.end(); ++entry_) {
		fprintf(stdout, "\t\thello num = %d\ttime = %f\n", (*entry_).seqNum_, (*entry_).time_);
	}
}
