
/*
 * Copyright (C) 2004-2005 by the University of Southern California
 * $Id: diffagent.h,v 1.12 2005/09/13 20:47:34 johnh Exp $
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

// DiffAppAgent - Wrapper Class for diffusion transport agent DR, ported from SCADDS's directed diffusion software. --Padma, nov 2001.  

#ifdef NS_DIFFUSION

#ifndef NS_DIFFAGENT
#define NS_DIFFAGENT

#include <stdlib.h>
#include <tclcl.h>
#include <agent.h>
#include <flags.h>
#include <mobilenode.h>
#include <dr.hh>

class DiffAppAgent;

class NsLocal : public DiffusionIO {
public:
	NsLocal(DiffAppAgent *agent) { agent_ = agent;}
	~NsLocal() {};
	DiffPacket recvPacket(int fd);
	void sendPacket(DiffPacket p, int len, int dst);
protected:
	DiffAppAgent *agent_;
};

class DiffAppAgent : public Agent {
public:
  
	DiffAppAgent();
	int command(int argc, const char*const* argv);
	void initpkt(Packet *p, Message* msg, int len);
	Packet* createNsPkt(Message *msg, int len);	
	void recv(Packet*, Handler*);
	void sendPacket(DiffPacket msg, int len, int dst);
	
	NR *dr() {return dr_; }
  
protected:
	
	// diffusion transport agent or DR
	NR *dr_;
	
};

#endif //diffagent
#endif // NS
