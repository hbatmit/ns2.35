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



#include <mdart/mdart_queue.h>



//------------------------------------------------------------------------------
// Queue timer
//------------------------------------------------------------------------------
MDARTQueueTimer::MDARTQueueTimer(MDARTQueue* queue) {
	queue_ = queue;
	counter = 0;
	handle((Event*) 0);
}

void MDARTQueueTimer::handle(Event*) {
//	fprintf(stdout, "%.9f\MDARTQueueTimer::handle()\n", CURRENT_TIME);
	if (counter > 0)
		queue_->purge();
	else
		counter++;
	Scheduler::instance().schedule(this, &intr, MDART_QUEUE_PURGE_INTERVAL);
}



//------------------------------------------------------------------------------
// Queue
//------------------------------------------------------------------------------
MDARTQueue::MDARTQueue() {
	queue_ = new queue;
	timer_ = new MDARTQueueTimer(this); 
/*	head_		= tail_ = 0;
	len_		= 0;
	limit_	= MDART_QUEUE_LEN;
	timeout_	= MDART_QUEUE_TIMEOUT;
	*/		
}

MDARTQueue::~MDARTQueue() {
	clear();
}

		
		
//------------------------------------------------------------------------------
// Queue functions
//------------------------------------------------------------------------------
void MDARTQueue::enque(Packet *p) {
#ifdef DEBUG_QUEUE
	fprintf(stdout, "%.9f\tMDARTQueue::enque(%d)\n",CURRENT_TIME, HDR_IP(p)->daddr());
	printQueue();
#endif
	struct hdr_cmn* ch_ = HDR_CMN(p);
	struct hdr_mdart_encp* rh_ = HDR_MDART_ENCP(p);
	if (queue_->size() >= MDART_QUEUE_LEN)
		removeHead();
#ifdef ADP_DARP_EB
	ch_->ts_ = CURRENT_TIME + MDART_QUEUE_TIMEOUT;
	rh_->lifetime_ = CURRENT_TIME + ADP_DARQ_EB_TIMEOUT;
	rh_->darqCount_++;
#else
	ch_->ts_ = CURRENT_TIME + MDART_QUEUE_TIMEOUT;
	rh_->lifetime_ = CURRENT_TIME + ADP_DARQ_TIMEOUT;
#endif	
	queue_->push_back(p);
}

Packet* MDARTQueue::deque() {
#ifdef DEBUG_QUEUE
		fprintf(stdout, "%.9f\tMDARTQueue::deque()\n", CURRENT_TIME);
#endif
	queue::iterator entry_ = queue_->begin();
	if (entry_ != queue_->end()) {
#ifdef DEBUG_QUEUE
		fprintf(stdout, "\tfind\tdst=%d\n", HDR_IP(*entry_)->daddr());
#endif
		Packet *p = (*entry_);
		queue_->erase(entry_);
		return p;
	}
	return NULL;
}

Packet* MDARTQueue::deque(nsaddr_t dst) {
#ifdef DEBUG_QUEUE
//			fprintf(stdout, "%.9f\tMDARTQueue::deque(%d)\n", CURRENT_TIME, dst);
#endif
	queue::iterator entry_ = queue_->begin();
	while (entry_ != queue_->end()) {
#ifdef DEBUG_QUEUE
		fprintf(stdout, "\t\tqueue packet to %d\n", dst);
#endif			
		if (HDR_IP(*entry_)->daddr() == dst) {
#ifdef DEBUG_QUEUE
			fprintf(stdout, "\tfind\n");
#endif			
			Packet *p = (*entry_);
			queue_->erase(entry_);
			return p;
		}
		entry_++;
	}
	return NULL;
}

bool MDARTQueue::isExpired(int pos) {
#ifdef DEBUG_QUEUE
	fprintf(stdout, "%.9f\tMDARTQueue::isExpired(%d)\n", CURRENT_TIME, pos);
#endif
	if (pos <= (int) queue_->size()) {
		struct hdr_mdart_encp* rh_ = HDR_MDART_ENCP(queue_->at(pos));
		if (rh_->lifetime_ <= CURRENT_TIME) {
#ifdef DEBUG_QUEUE
			fprintf(stdout, "\texpired \n");
#endif
#ifdef ADP_DARQ_EB
			rh_->lifetime_ = CURRENT_TIME + rh_->darqCount_ * ADP_DARQ_EB_TIMEOUT;
			rh_->darqCount_++;
#else
			rh_->lifetime_ = CURRENT_TIME + ADP_DARQ_TIMEOUT;
#endif
			return true;
		}
	}
	return false;
}

Packet* MDARTQueue::getPacket(int pos) {
#ifdef DEBUG_QUEUE
	fprintf(stdout, "%.9f\tMDARTQueue::get(%d)\n", CURRENT_TIME, pos);
#endif
	if (pos <= (int) queue_->size()) {
		return queue_->at(pos);
	}
	return NULL;
}

/*
bool MDARTQueue::expired() {
#ifdef DEBUG_QUEUE
	fprintf(stdout, "%.9f\tMDARTQueue::expired()\n",CURRENT_TIME);
#endif
	queue::iterator entry_ = queue_->begin();
	while (entry_ != queue_->end()) {
		if (HDR_CMN(*entry_)->ts_ <= CURRENT_TIME - MDART_QUEUE_TIMEOUT + MDART_DARQ_TIMEOUT) {
			return true;
		}
	}
	return false;
}
*/

void MDARTQueue::purge() {
#ifdef DEBUG_QUEUE
	fprintf(stdout, "%.9f\tMDARTQueue::purge()\n",CURRENT_TIME);
	printQueue();
#endif
	queue::iterator entry_ = queue_->begin();
	while (entry_ != queue_->end()) {
#ifdef DEBUG_QUEUE
		fprintf(stdout, "\tchecking for %d\n", HDR_IP(*entry_)->daddr());
#endif
		queue::iterator nextEntry_ = entry_;
		nextEntry_++;
		if (HDR_CMN(*entry_)->ts_ <= CURRENT_TIME) {
#ifdef DEBUG_QUEUE
			fprintf(stdout, "\tdropped packet for %d\n", HDR_IP(*entry_)->daddr());
#endif
			drop((*entry_), DROP_RTR_QTIMEOUT);
			queue_->erase(entry_);
		}
//		La funzione erase() mi incrementa di suo l'iteratore
		else 
			entry_ = nextEntry_;
	}
#ifdef DEBUG_QUEUE
	printQueue();
#endif
}



//------------------------------------------------------------------------------
// Queue debug functions
//------------------------------------------------------------------------------
void MDARTQueue::printNumPacket(nsaddr_t dst){
	int num = 0;
	queue::iterator entry_;
	for(entry_ = queue_->begin(); entry_ != queue_->end(); entry_++) {
		if (HDR_IP(*entry_)->daddr() == dst) {
			num++;
		}
	}
	fprintf(stdout, "\tMDARTQueue::numPacket(%d) = %d\n", dst, num);
}

void MDARTQueue::printQueue() {
	queue::iterator entry_;
	fprintf(stdout, "\tMDARTQueue::printQueue()\n");
//	fprintf(stdout, "%.9f\tMDARTQueue::printQueue()\n", CURRENT_TIME);
	for(entry_ = queue_->begin(); entry_ != queue_->end(); ++entry_) {
		fprintf(stdout, "\t\tpacket dst = %d\texpire time = %f\n", HDR_IP(*entry_)->daddr(), HDR_CMN(*entry_)->ts_);
	}
}



//------------------------------------------------------------------------------
// Queue private functions
//------------------------------------------------------------------------------
void MDARTQueue::removeHead() {
#ifdef DEBUG_QUEUE
	fprintf(stdout, "%.9f\tMDARTQueue::removeHead()\n", CURRENT_TIME);
	printQueue();
#endif
	queue::iterator entry_ = queue_->begin();
	if (entry_ != queue_->end()) {
		if(HDR_CMN(*entry_)->ts_ <= CURRENT_TIME) {
#ifdef DEBUG_QUEUE
			fprintf(stdout, "\tDROP_RTR_QTIMEOUT\n");
#endif
			drop(*entry_, DROP_RTR_QTIMEOUT);
		}
		else {
#ifdef DEBUG_QUEUE
			fprintf(stdout, "\tDROP_RTR_QFULL\n");
#endif
			drop(*entry_, DROP_RTR_QFULL);
		}
		queue_->erase(entry_);
	}
#ifdef DEBUG_QUEUE
	printQueue();
#endif
}

void MDARTQueue::clear() {
	queue::iterator entry_ = queue_->begin();
	while (entry_ != queue_->end()) {
		queue::iterator nextEntry_ = entry_;
		nextEntry_++;
		queue_->erase(entry_);
		delete (*entry_);
		entry_ = nextEntry_;
	}
}

/*	
void MDARTQueue::enque(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
#ifdef DEBUG_QUEUE
	struct hdr_mdart_encp *Rh = HDR_MDART_ENCP(p);
	nsaddr_t dest= Rh->dstId_;
	fprintf(stdout, "%.9f\tMDARTQueue::enque(%d)\n",CURRENT_TIME, dest);
#endif
	// Purge any expired packet
	p->next_ = 0;
	ch->ts_ = CURRENT_TIME + MDART_QUEUE_TIMEOUT;
	if (len_ == limit_) {
		// Remove the older packet
		Packet *p0 = remove_head();	// decrements len_
		assert(p0);
		if(HDR_CMN(p0)->ts_ > CURRENT_TIME) {
			drop(p0, DROP_RTR_QFULL);
		}
		else {
			drop(p0, DROP_RTR_QTIMEOUT);
		}
	}
	if(head_ == 0) {
		head_ = tail_ = p;
	}
	else {
		tail_->next_ = p;
		tail_ = p;
	}
	len_++;
#ifdef DEBUG_QUEUE
	verifyQueue();
#endif
}
*/

/*
Packet* MDARTQueue::deque() {
	purge();
  	Packet *p;
	// Purge any expired packet
	purge();
	p = remove_head();
#ifdef DEBUG_QUEUE
	verifyQueue();
#endif
	return p;
}
*/

/*	
Packet* MDARTQueue::deque(nsaddr_t dst) {
	Packet *p, *prev;
	// Purge any expired packet
	purge();

	find(dst, p, prev);
	assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));
	if(p == 0) {
		return 0;
	}
	if (p == head_) {
		p = remove_head();
	}
	else if (p == tail_) {
		prev->next_ = 0;
		tail_ = prev;
		len_--;
	}
	else {
		prev->next_ = p->next_;
		len_--;
	}
#ifdef DEBUG_QUEUE
	verifyQueue();
#endif
	return p;
}
*/

/*
bool MDARTQueue::findDAExpired(Packet*& p) {
	p = 0;
	if (head_ == 0)
		return false;
	for(p = head_; p; p = p->next_)
	{
		if(HDR_CMN(p)->ts_ < CURRENT_TIME + 25)
		{
			return true;
		}
	}
	return false;
}
*/

/*
Packet* MDARTQueue::findExpiredDarq(Packet* start) {
	Packet* p;
	if (!start) {
		p = head_;
	}
	else {
		if (start != tail_){
			p=start->next_;
		}
		else {
		return 0;
		}
			
	}
	
	for (; p!= tail_; p= p->next_) {
		if(HDR_CMN(p)->ts_ < CURRENT_TIME + 25) {
			return p;
		}
	}
	return 0;
}
*/

/*
bool MDARTQueue::findExpired(Packet*& p, Packet*& prev) {
	p = prev = 0;
	for(p = head_; p; p = p->next_)
	{
		if(HDR_CMN(p)->ts_ < CURRENT_TIME)
		{
			return true;
		}
		prev = p;
	}
	return false;
}
*/

/*
void MDARTQueue::printNumPacket(nsaddr_t dst){
	Packet *p;
	unsigned int count_;
	count_=0; 
	for(p = head_; p; p = p->next_)	{
		if(HDR_IP(p)->daddr() == dst)	{
			count_++;
		}
	}
#ifdef DEBUG_QUEUE
	fprintf(stdout, "ci sono %d pacchetti nella coda del nodo per la destinazione %d\n",count_, dst);
#endif
}
*/

/*
Packet* MDARTQueue::removeHead()
{
	Packet *p = head_;
	if(head_ == tail_) {
		head_ = tail_ = 0;
	}
	else {
		head_ = head_->next_;
	}
	if(p) {
		 len_--;
	}
	return p;
}
*/

/*
void MDARTQueue::find(nsaddr_t dst, Packet*& p, Packet*& prev)
{
	p = prev = 0;
	for(p = head_; p; p = p->next_)
	{
		if(HDR_IP(p)->daddr() == dst)	{
			return;
		}
		prev = p;
	}
}
*/

/*
void MDARTQueue::verifyQueue() {
	Packet *p, *prev = 0;
	int cnt = 0;
	for(p = head_; p; p = p->next_) {
		cnt++;
		prev = p;
	}
	assert(cnt == len_);
	assert(prev == tail_);
}
*/

/*
void MDARTQueue::purge() {
	Packet *p, *prev;
	while ( findExpired(p, prev) ) {
		assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));
		if(p == 0) {
			return;
		}
		if (p == head_) {
			p = remove_head();
		}
		else if (p == tail_) {
			prev->next_ = 0;
			tail_ = prev;
			len_--;
		}
		else {
			prev->next_ = p->next_;
			len_--;
		}
#ifdef DEBUG_QUEUE
		verifyQueue();
#endif
		drop(p, DROP_RTR_QTIMEOUT);
		p = prev = 0;
	}
}
*/
