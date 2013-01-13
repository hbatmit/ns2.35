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
 *  $Header: /cvsroot/nsnam/ns-2/webcache/tcpapp.cc,v 1.16 2005/08/26 05:05:32 tomh Exp $
 *
 */

//
// Tcp application: transmitting real application data
// 
// Allows only one connection. Model underlying TCP connection as a 
// FIFO byte stream, use this to deliver user data

#include "agent.h"
#include "app.h"
#include "tcpapp.h"


// Buffer management stuff.

CBuf::CBuf(AppData *c, int nbytes)
{
	nbytes_ = nbytes;
	size_ = c->size();
  	if (size_ > 0) 
		data_ = c;
  	else 
  		data_ = NULL;
	next_ = NULL;
}

CBufList::~CBufList() 
{
	while (head_ != NULL) {
		tail_ = head_;
		head_ = head_->next_;
		delete tail_;
	}
}

void CBufList::insert(CBuf *cbuf) 
{
	if (tail_ == NULL) 
		head_ = tail_ = cbuf;
	else {
		tail_->next_ = cbuf;
		tail_ = cbuf;
	}
#ifdef TCPAPP_DEBUG
	num_++;
#endif
}

CBuf* CBufList::detach()
{
	if (head_ == NULL)
		return NULL;
	CBuf *p = head_;
	if ((head_ = head_->next_) == NULL)
		tail_ = NULL;
#ifdef TCPAPP_DEBUG
	num_--;
#endif
	return p;
}


// ADU for plain TcpApp, which is by default a string of otcl script
// XXX Local to this file
class TcpAppString : public AppData {
private:
	int size_;
	char* str_; 
public:
	TcpAppString() : AppData(TCPAPP_STRING), size_(0), str_(NULL) {}
	TcpAppString(TcpAppString& d) : AppData(d) {
		size_ = d.size_;
		if (size_ > 0) {
			str_ = new char[size_];
			strcpy(str_, d.str_);
		} else
			str_ = NULL;
	}
	virtual ~TcpAppString() { 
		if (str_ != NULL) 
			delete []str_; 
	}

	char* str() { return str_; }
	virtual int size() const { return AppData::size() + size_; }

	// Insert string-contents into the ADU
	void set_string(const char* s) {
		if ((s == NULL) || (*s == 0)) 
			str_ = NULL, size_ = 0;
		else {
			size_ = strlen(s) + 1;
			str_ = new char[size_];
			assert(str_ != NULL);
			strcpy(str_, s);
		}
	}
	virtual AppData* copy() {
		return new TcpAppString(*this);
	}
};

// TcpApp
static class TcpCncClass : public TclClass {
public:
	TcpCncClass() : TclClass("Application/TcpApp") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc != 5)
			return NULL;
		Agent *tcp = (Agent *)TclObject::lookup(argv[4]);
		if (tcp == NULL) 
			return NULL;
		return (new TcpApp(tcp));
	}
} class_tcpcnc_app;

TcpApp::TcpApp(Agent *tcp) : 
	Application(), curdata_(0), curbytes_(0)
{
	agent_ = tcp;
	agent_->attachApp(this);
}

TcpApp::~TcpApp()
{
	// XXX Before we quit, let our agent know what we no longer exist
	// so that it won't give us a call later...
	agent_->attachApp(NULL);
}

// Send with callbacks to transfer application data
void TcpApp::send(int nbytes, AppData *cbk)
{
	CBuf *p = new CBuf(cbk, nbytes);
#ifdef TCPAPP_DEBUG
	p->time() = Scheduler::instance().clock();
#endif
	cbuf_.insert(p);
	Application::send(nbytes);
}

// All we need to know is that our sink has received one message
void TcpApp::recv(int size)
{
	// If it's the start of a new transmission, grab info from dest, 
	// and execute callback
	if (curdata_ == 0)
		curdata_ = dst_->rcvr_retrieve_data();
	if (curdata_ == 0) {
		fprintf(stderr, "[%g] %s receives a packet but no callback!\n",
			Scheduler::instance().clock(), name_);
		return;
	}
	curbytes_ += size;
#ifdef TCPAPP_DEBUG
	fprintf(stderr, "[%g] %s gets data size %d, %s\n", 
		Scheduler::instance().clock(), name(), curbytes_, 
		curdata_->data());
#endif
	if (curbytes_ == curdata_->bytes()) {
		// We've got exactly the data we want
		// If we've received all data, execute the callback
		process_data(curdata_->size(), curdata_->data());
		// Then cleanup this data transmission
		delete curdata_;
		curdata_ = NULL;
		curbytes_ = 0;
	} else if (curbytes_ > curdata_->bytes()) {
		// We've got more than we expected. Must contain other data.
		// Continue process callbacks until the unfinished callback
		while (curbytes_ >= curdata_->bytes()) {
			process_data(curdata_->size(), curdata_->data());
			curbytes_ -= curdata_->bytes();
#ifdef TCPAPP_DEBUG
			fprintf(stderr, 
				"[%g] %s gets data size %d(left %d)\n", 
				Scheduler::instance().clock(), 
				name(),
				curdata_->bytes(), curbytes_);
				//curdata_->data());
#endif
			delete curdata_;
			curdata_ = dst_->rcvr_retrieve_data();
			if (curdata_ != 0)
				continue;
			if ((curdata_ == 0) && (curbytes_ > 0)) {
				fprintf(stderr, "[%g] %s gets extra data!\n",
					Scheduler::instance().clock(), name_);
				Tcl::instance().eval("[Simulator instance] flush-trace");
				abort();
			} else
				// Get out of the look without doing a check
				break;
		}
	}
}

void TcpApp::resume()
{
	// Do nothing
}

int TcpApp::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (strcmp(argv[1], "connect") == 0) {
		dst_ = (TcpApp *)TclObject::lookup(argv[2]);
		if (dst_ == NULL) {
			tcl.resultf("%s: connected to null object.", name_);
			return (TCL_ERROR);
		}
		dst_->connect(this);
		return (TCL_OK);
	} else if (strcmp(argv[1], "send") == 0) {
		/*
		 * <app> send <size> <tcl_script>
		 */
		int size = atoi(argv[2]);
		if (argc == 3)
			send(size, NULL);
		else {
			TcpAppString *tmp = new TcpAppString();
			tmp->set_string(argv[3]);
			send(size, tmp);
		}
		return (TCL_OK);
	} else if (strcmp(argv[1], "dst") == 0) {
		tcl.resultf("%s", dst_->name());
		return TCL_OK;
	}
	return Application::command(argc, argv);
}

void TcpApp::process_data(int size, AppData* data) 
{
	if (data == NULL)
		return;
	// XXX Default behavior:
	// If there isn't a target, use tcl to evaluate the data
	if (target())
		send_data(size, data);
	else if (data->type() == TCPAPP_STRING) {
		TcpAppString *tmp = (TcpAppString*)data;
		Tcl::instance().eval(tmp->str());
	}
}
