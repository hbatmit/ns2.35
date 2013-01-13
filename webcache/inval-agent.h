/*
 * Copyright (c) Xerox Corporation 1998. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linking this file statically or dynamically with other modules is making
 * a combined work based on this file.  Thus, the terms and conditions of
 * the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of this file
 * give you permission to combine this file with free software programs or
 * libraries that are released under the GNU LGPL and with code included in
 * the standard release of ns-2 under the Apache 2.0 license or under
 * otherwise-compatible licenses with advertising requirements (or modified
 * versions of such code, with unchanged license).  You may copy and
 * distribute such a system following the terms of the GNU GPL for this
 * file and the licenses of the other code concerned, provided that you
 * include the source code of that other code when and as the GNU GPL
 * requires distribution of source code.
 *
 * Note that people who make modified versions of this file are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception.
 *
 * $Header: /cvsroot/nsnam/ns-2/webcache/inval-agent.h,v 1.12 2005/08/26 05:05:31 tomh Exp $
 *
 */

//
// Definition of Agent/Invalidation
// 

#ifndef ns_invalagent_h
#define ns_invalagent_h

#include <string.h>
#include <tclcl.h>
#include "config.h"
#include "packet.h"
#include "agent.h"
#include "tcpapp.h"

struct hdr_inval {
	int size_;
	int& size() { return size_; }

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_inval* access(const Packet* p) {
		return (hdr_inval*) p->access(offset_);
	}
};

// XXX If we have an interface declaration, we could define a common 
// interface for the two implementations below. But in C++ the only way
// to define an interface is through abstract base class, and use 
// multiple inheritance to get different implementations with the same 
// interface. 
// Because multiple inheritance prohibits conversion from 
// base class pointer to derived class pointer, and TclObject::lookup 
// must do such a conversion, we cannot declare a common interface here. :(

class HttpApp;

// Implementation 1: multicast, this is a real agent
class HttpInvalAgent : public Agent {
public: 
	HttpInvalAgent();

	virtual void recv(Packet *, Handler *);
	virtual void send(int realsize, AppData* data);

protected:
	int inval_hdr_size_;
};

// Implementation 2: unicast, actually an application on top of TCP
class HttpUInvalAgent : public TcpApp {
public:
	HttpUInvalAgent(Agent *a) : TcpApp(a) {}

	void send(int realsize, AppData* data) {
		TcpApp::send(realsize, data);
	}
	virtual void process_data(int size, AppData* data);
	virtual AppData* get_data(int&, AppData*) {
		abort(); 
		return NULL;
	}
protected:
	virtual int command(int argc, const char*const* argv);
};

#endif // ns_invalagent_h
