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
 * $Header: /cvsroot/nsnam/ns-2/webcache/tcpapp.h,v 1.15 2005/08/26 05:05:32 tomh Exp $
 */

//
// TcpApp - Transmitting real application data via TCP
//
// XXX Model a TCP connection as a FIFO byte stream. It shouldn't be used 
// if this assumption is violated.

#ifndef ns_tcpapp_h
#define ns_tcpapp_h

#include "ns-process.h"
#include "app.h"

class CBuf { 
public:
	CBuf(AppData *c, int nbytes);
	~CBuf() {
		if (data_ != NULL)
			delete data_;
	}
	AppData* data() { return data_; }
	int size() { return size_; }
	int bytes() { return nbytes_; }

#ifdef TCPAPP_DEBUG	
	// debug only
	double& time() { return time_; }
#endif

protected:
	friend class CBufList;
	AppData *data_;
	int size_;
	int nbytes_; 	// Total length of this transmission
	CBuf *next_;

#ifdef TCPAPP_DEBUG
	// for debug only 
	double time_; 
#endif
};

// A FIFO queue
class CBufList {
public:	
#ifdef TCPAPP_DEBUG 
	CBufList() : head_(NULL), tail_(NULL), num_(0) {}
#else
	CBufList() : head_(NULL), tail_(NULL) {}
#endif
	~CBufList();

	void insert(CBuf *cbuf);
	CBuf* detach();

protected:
	CBuf *head_;
	CBuf *tail_;
#ifdef TCPAPP_DEBUG
	int num_;
#endif
};


class TcpApp : public Application {
public:
	TcpApp(Agent *tcp);
	~TcpApp();

	virtual void recv(int nbytes);
	virtual void send(int nbytes, AppData *data);

	void connect(TcpApp *dst) { dst_ = dst; }

	virtual void process_data(int size, AppData* data);
	virtual AppData* get_data(int&, AppData*) {
		// Not supported
		abort();
		return NULL;
	}

	// Do nothing when a connection is terminated
	virtual void resume();

protected:
	virtual int command(int argc, const char*const* argv);
	CBuf* rcvr_retrieve_data() { return cbuf_.detach(); }

	// We don't have start/stop
	virtual void start() { abort(); } 
	virtual void stop() { abort(); }

	TcpApp *dst_;
	CBufList cbuf_;
	CBuf *curdata_;
	int curbytes_;
};

#endif // ns_tcpapp_h
