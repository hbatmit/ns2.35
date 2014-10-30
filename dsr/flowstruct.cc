
/*
 * flowstruct.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: flowstruct.cc,v 1.5 2010/03/08 05:54:50 tom_henderson Exp $
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
// Ported from CMU/Monarch's code, appropriate copyright applies.  


#include "flowstruct.h"

FlowTable::FlowTable(int size_) : DRTab(size_) {
	assert (size_ > 0);
	size = 0;
	maxSize = size_;
	table = new TableEntry[size_];
	counter = 0;
}

FlowTable::~FlowTable() {
	delete table;
}

TableEntry &FlowTable::operator[](int index) {
	assert(index >= 0 && index < size);
	return table[index];
}

int FlowTable::find(nsaddr_t source, nsaddr_t destination, u_int16_t flow) {
	register int i;
	for (i=size-1; i>=0; i--)
		if (table[i].sourceIP == source &&
		    table[i].destinationIP == destination &&
		    table[i].flowId == flow)
			break;

	return i;
}

int FlowTable::find(nsaddr_t source, nsaddr_t destination, const Path &route) {
	register int i;
	for (i=size-1; i>=0; i--)
		if (table[i].sourceIP == source &&
		    table[i].destinationIP == destination &&
		    table[i].sourceRoute == route)
			break;

	return i;
}

void FlowTable::grow() {
	assert(0);
	TableEntry *temp;
	if (!(temp = new TableEntry[maxSize*2]))
		return;
	bcopy(table, temp, sizeof(TableEntry)*maxSize);
	delete table;
	maxSize *= 2;
	table = temp;
}

u_int16_t FlowTable::generateNextFlowId(nsaddr_t , bool allowDefault) {
	if ((counter&1)^allowDefault) // make sure parity is correct
		counter++;

	assert((counter & 1) == allowDefault);
	return counter++;
}

int FlowTable::createEntry(nsaddr_t source, nsaddr_t destination, 
			   u_int16_t flow) {
	if (find(source, destination, flow) != -1)
		return -1;
	if (size == maxSize)
		grow();
	if (size == maxSize)
		return -1;

	table[size].sourceIP = source;
	table[size].destinationIP = destination;
	table[size].flowId = flow;

	if (flow & 1)
		DRTab.insert(source, destination, flow);

	return size++;
}

void FlowTable::noticeDeadLink(const ID &from, const ID &to) {
	double now = Scheduler::instance().clock();

	for (int i=0; i<size; i++)
		if (table[i].timeout >= now && table[i].sourceIP == net_addr)
			for (int n=0; n < (table[i].sourceRoute.length()-1); n++)
				if (table[i].sourceRoute[n] == from &&
				    table[i].sourceRoute[n+1] == to) {
					table[i].timeout = now - 1;
					// XXX ych rediscover??? 5/2/01
				}
}

// if ent represents a default flow, bad things are going down and we need
// to rid the default flow table of them.
static void checkDefaultFlow(DRTable &DRTab, const TableEntry &ent) {
	u_int16_t flow;
	if (!DRTab.find(ent.sourceIP, ent.destinationIP, flow))
		return;
	if (flow == ent.flowId)
		DRTab.flush(ent.sourceIP, ent.destinationIP);
}

void FlowTable::cleanup() {
	int front, back;
	double now = Scheduler::instance().clock();

	return; // it's messing up path orders...

	// init front to the first expired entry
	for (front=0; (front<size) && (table[front].timeout >= now); front++)
		;

	// init back to the last unexpired entry
	for (back = size-1; (front<back) && (table[back].timeout < now); back--)
		checkDefaultFlow(DRTab, table[back]);

	while (front < back) {
		checkDefaultFlow(DRTab, table[front]);
		bcopy(table+back, table+front, sizeof(TableEntry)); // swap
		back--;

		// next expired entry
		while ((front<back) && (table[front].timeout >= now))
			front++;
		while ((front<back) && (table[back].timeout < now)) {
			checkDefaultFlow(DRTab, table[back]);
			back--;
		}
	}

	size = back+1;
}

void FlowTable::setNetAddr(nsaddr_t net_id) {
	net_addr = net_id;
}

bool FlowTable::defaultFlow(nsaddr_t source, nsaddr_t destination, 
			    u_int16_t &flow) {
	return DRTab.find(source, destination, flow);
}

DRTable::DRTable(int size_) {
	assert (size_ > 0);
	size = 0;
	maxSize = size_;
	table = new DRTabEnt[size_];
}

DRTable::~DRTable() {
	delete table;
}

bool DRTable::find(nsaddr_t src, nsaddr_t dst, u_int16_t &flow) {
	for (int i = 0; i < size; i++)
		if (src == table[i].src && dst == table[i].dst) {
			flow = table[i].fid;
			return true;
		}
	return false;
}

void DRTable::grow() {
	assert(0);
	DRTabEnt *temp;
	if (!(temp = new DRTabEnt[maxSize*2]))
		return;
	bcopy(table, temp, sizeof(DRTabEnt)*maxSize);
	delete table;
	maxSize *= 2;
	table = temp;
}

void DRTable::insert(nsaddr_t src, nsaddr_t dst, u_int16_t flow) {
	assert(flow & 1);
	for (int i = 0; i < size; i++) {
		if (src == table[i].src && dst == table[i].dst) {
			if ((short)((flow) - (table[i].fid)) > 0) {
				table[i].fid = flow;
			} else {
			}
			return;
		}
	}

	if (size == maxSize)
		grow();

	assert(size != maxSize);

	table[size].src = src;
	table[size].dst = dst;
	table[size].fid = flow;
	size++;
}

void DRTable::flush(nsaddr_t src, nsaddr_t dst) {
	for (int i = 0; i < size; i++)
		if (src == table[i].src && dst == table[i].dst) {
			table[i].src = table[size-1].src;
			table[i].dst = table[size-1].dst;
			table[i].fid = table[size-1].fid;
			size--;
			return;
		}
	assert(0);
}

ARSTable::ARSTable(int size_) {
	size = size_;
	victim = 0;
	table = new ARSTabEnt[size_];
	bzero(table, sizeof(ARSTabEnt)*size_);
}

ARSTable::~ARSTable() {
	delete table;
}

void ARSTable::insert(int uid, u_int16_t fid, int hopsEarly) {
	int i = victim;
	assert(hopsEarly);
	
	do {
		if (!table[i].hopsEarly)
			break; // we found a victim
		i = (i+1)%size;
	} while (i != victim);

	if (table[i].hopsEarly) // full. need extreme measures.
		victim = (victim+1)%size;

	table[i].hopsEarly = hopsEarly;
	table[i].uid       = uid;
	table[i].fid	   = fid;
}

int ARSTable::findAndClear(int uid, u_int16_t fid) {
	int i, retval;

	for (i=0; i<size; i++) {
	        if (table[i].hopsEarly && table[i].uid == uid) {
		        if (table[i].fid == fid) {
			        retval = table[i].hopsEarly;
				table[i].hopsEarly = 0;
				return retval;
			} else {
				table[i].hopsEarly = 0;
				return 0;
			}
		}
	}

	return 0;
}
