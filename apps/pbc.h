/*
 * Copyright (C) 2007 
 * Mercedes-Benz Research & Development North America, Inc. and
 * University of Karlsruhe (TH)
 * All rights reserved.
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
 
/*
 * For further information see: 
 * http://dsn.tm.uni-karlsruhe.de/english/Overhaul_NS-2.php
 */



#ifndef ns_pbc_h
#define ns_pbc_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"



struct hdr_pbc {
	double 	send_time;
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_pbc* access(const Packet* p) {
		return (hdr_pbc*) p->access(offset_);
	}
};

class PBCAgent;



class PBCTimer : public Handler {
public:
	PBCTimer(PBCAgent* ag, double p = 1) : agent(ag), period(p) {
		started = 0;
		}
	void start(void);
	void stop(void);
	void setPeriod(double p);
	void setVariance(double v);
	void handle(Event *e);

protected:
	int		started;
	Event		intr;		
	PBCAgent	*agent;
	double		period;
	double		variance;
};

class PBCAgent : public Agent {
	friend class PBCTimer;
public:
	PBCAgent();
	~PBCAgent();
	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);
	void	singleBroadcast();
	void    singleUnicast(int addr);
	bool    periodicBroadcast;

	double msgInterval;
	double msgVariance;
private:
	PBCTimer  timer;
	int size;
	int modulationScheme;
};

#endif // ns_pbc_h
