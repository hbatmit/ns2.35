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



#include <mdart/mdart_dht.h>
#include <mdart/mdart_function.h>



//------------------------------------------------------------------------------
// DHT timer
//------------------------------------------------------------------------------
DHTTimer::DHTTimer(DHT* dht) {
	dht_ = dht;
	counter = 0;
	handle((Event*) 0);
}

void DHTTimer::handle(Event*) {
//	fprintf(stdout, "%.9f\DHTTimer::handle()\n", CURRENT_TIME);
	if (counter > 1)
		dht_->purge();
	else
		counter++;
	Scheduler::instance().schedule(this, &intr, MDART_DHT_PURGE_INTERVAL);
}



//------------------------------------------------------------------------------
// DHT
//------------------------------------------------------------------------------
DHT::DHT() {
	timer_ = new DHTTimer(this);
	table_ = new dhtTable;
}

DHT::~DHT() {
	clear();
}


//------------------------------------------------------------------------------
// DHT functions
//------------------------------------------------------------------------------
void DHT::addEntry(nsaddr_t id, nsaddr_t address) {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::addEntry(%d, %d)\texpire = %f\n", CURRENT_TIME, id, address, CURRENT_TIME + MDART_DHT_ENTRY_TIMEOUT);
	printDHT();
#endif

	DHT_Entry* entry_ = getEntry(id);
	if (entry_ == NULL) {
		entry_ = new DHT_Entry(id, address);
		table_->insert(entry_);
	}
	else {
		entry_->address(address);
		entry_->expire(CURRENT_TIME + MDART_DHT_ENTRY_TIMEOUT);
	}
#ifdef DEBUG_DHT
	printDHT();
#endif
}

void DHT::addEntry(nsaddr_t id, nsaddr_t address, double expire) {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::addEntry(%d, %d)\texpire = %f\n", CURRENT_TIME, id, address, CURRENT_TIME + MDART_DHT_ENTRY_TIMEOUT);
	printDHT();
#endif

	DHT_Entry* entry_ = getEntry(id);
	if (entry_ == NULL) {
		entry_ = new DHT_Entry(id, address);
		entry_->expire(expire);
		table_->insert(entry_);
	}
	else {
		entry_->address(address);
		entry_->expire(expire);
	}
#ifdef DEBUG_DHT
	printDHT();
#endif
}

void DHT::purge() {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::purge()\n", CURRENT_TIME);
	printDHT();
#endif
	dhtTable::iterator entry_ = table_->begin();
	while(entry_ != table_->end() && !table_->empty()) {
		dhtTable::iterator nextEntry_ = entry_;
		nextEntry_++;
		if ((*entry_)->expire() < CURRENT_TIME) {
#ifdef DEBUG_DHT
			fprintf(stdout, "\tpurging\tnode id = %d\tnode address = %d\texpire time = %f\n", (*entry_)->id(), (*entry_)->address(), (*entry_)->expire());
#endif
			table_->erase(entry_);
			delete (*entry_);
		}
		entry_ = nextEntry_;
	}
#ifdef DEBUG_DHT
	printDHT();
#endif
}

DHT_Entry* DHT::getEntry(u_int32_t pos) {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::getEntry(%d)\n", CURRENT_TIME, pos);
#endif
	dhtTable::iterator entry_ = table_->begin();
	while (pos  > 0) {
		entry_++;
		pos--;
	}
	return (*entry_);
}

nsaddr_t DHT::findAdd(nsaddr_t id) {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::findAdd(%d)\n", CURRENT_TIME, id);
#endif
	dhtTable::iterator entry_;
	for(entry_ = table_->begin(); entry_ != table_->end(); ++entry_) {
		if ((*entry_)->id() == id) {
#ifdef DEBUG_DHT
			fprintf(stdout, "\tfind address %d\n", (*entry_)->address());
#endif
			return (*entry_)->address();
		}
	}
	return IP_BROADCAST;
}
	
nsaddr_t DHT::findId(nsaddr_t address) {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::findId(%d)\n", CURRENT_TIME, address);
#endif
	dhtTable::iterator entry_;
	for(entry_ = table_->begin(); entry_ != table_->end(); ++entry_) {
		if ((*entry_)->address() == address)
#ifdef DEBUG_DHT
			fprintf(stdout, "\tfind id %d\n", (*entry_)->id());
#endif
			return (*entry_)->id();
	}
	return IP_BROADCAST;
}

u_int32_t DHT::DHTSize() {
	return table_->size();
}

PacketData* DHT::getDHTUpdate() {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::getDHTUpdate()\n", CURRENT_TIME);
#endif
	string string_;
	dhtTable::iterator entry_;
	for(entry_ = table_->begin(); entry_ != table_->end(); ++entry_) {
		ostringstream stream_;
		stream_ << (*entry_)->id();
		stream_ << ',';
		stream_ << (*entry_)->address();
		stream_ << ',';
		stream_ << (*entry_)->expire();
		stream_ << ';';
		string_ += stream_.str();
	}
#ifdef DEBUG_DHT
	fprintf(stdout, "\tdata = %s\n", string_.data());
#endif
//	PacketData* data_ = new PacketData(512);
	PacketData* data_ = new PacketData(string_.length() + 2);
	strcpy((char *)data_->data(), string_.data());
//	fprintf(stdout, "\tstring length = %d\n", (int) string_.length());
//	fprintf(stdout, "\tstring = %s\n", string_.data());
//	fprintf(stdout, "\tdata length = %d\n", data_->size());
//	fprintf(stdout, "\tdata = %s\n", data_->data());
	return data_;	
}

/*
void DHT::setDHTUpdate(PacketData* data) {
#ifdef DEBUG_DHT
	fprintf(stdout, "%.9f\tDHT::setDHTUpdate()\n", CURRENT_TIME);
	printDHT();
#endif
	ostringstream strStream_;
	strStream_ << data->data();
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
		lastPos_ = str_.find_first_not_of(";", pos_);
		pos_ = str_.find_first_of(";", lastPos_);
	}
#ifdef DEBUG_DHT
	printDHT();
#endif
}
*/



//------------------------------------------------------------------------------
// DHT debug functions
//------------------------------------------------------------------------------
void DHT::printDHT() {
	dhtTable::iterator entry_;
	fprintf(stdout, "\tDHT::printDHT()\n");
	for(entry_ = table_->begin(); entry_ != table_->end(); ++entry_) {
		fprintf(stdout, "\t\tnode id = %d\tnode address = %s\texpire time = %f\n", (*entry_)->id(), bitString((*entry_)->address()), (*entry_)->expire());
	}
}



//------------------------------------------------------------------------------
// DHT private functions
//------------------------------------------------------------------------------
DHT_Entry* DHT::getEntry(nsaddr_t id) {
	dhtTable::iterator entry_;
	for(entry_ = table_->begin(); entry_ != table_->end(); entry_++) {
		if ((*entry_)->id() == id) {
			return (*entry_);
		}
	}
	return NULL;
}

void DHT::removeEntry(nsaddr_t id) {
	dhtTable::iterator entry_ = table_->begin();
	while (entry_ != table_->end() && !table_->empty()) {
		dhtTable::iterator nextEntry_ = entry_;
		++nextEntry_;
		if ((*entry_)->id() == id) {
			table_->erase(entry_);
			delete (*entry_);
		}
		entry_ = nextEntry_;
	}
}

void DHT::clear() {
	dhtTable::iterator entry_ = table_->begin();
	while (entry_ != table_->end() && !table_->empty()) {
		dhtTable::iterator nextEntry_ = entry_;
		++nextEntry_;
		table_->erase(entry_);
		delete (*entry_);
		entry_ = nextEntry_;
	}
}

