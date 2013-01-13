/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * ns-process.h
 * Copyright (C) 1997 by the University of Southern California
 * $Id: ns-process.h,v 1.6 2005/08/25 18:58:02 johnh Exp $
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

// ADU and ADU processor
//
// $Header: /cvsroot/nsnam/ns-2/common/ns-process.h,v 1.6 2005/08/25 18:58:02 johnh Exp $

#ifndef ns_process_h
#define ns_process_h

#include <assert.h>
#include <string.h>
#include "config.h"

// Application-level data unit types
enum AppDataType {
	// Illegal type
	ADU_ILLEGAL,

	// Old packet data ADU
	PACKET_DATA,

	// HTTP ADUs
	HTTP_DATA,
	HTTP_INVALIDATION,	// Heartbeat that may contain invalidation
	HTTP_UPDATE,		// Pushed page updates (version 1)
	HTTP_PROFORMA,		// Pro forma sent when a direct request is sent
	HTTP_JOIN,
	HTTP_LEAVE,
	HTTP_PUSH,		// Selectively pushed pages 
	HTTP_NORMAL,		// Normal req/resp packets

	// TcpApp ADU
	TCPAPP_STRING,

	// Multimedia ADU
	MEDIA_DATA,
	MEDIA_REQUEST,

	// pub/sub ADU
	PUBSUB,
	
	//Diffusion ADU
	DIFFUSION_DATA,

	// Last ADU
	ADU_LAST

};

// Interface for generic application-level data unit. It should know its 
// size and how to make itself persistent.
class AppData {
private:
	AppDataType type_;  	// ADU type
public:
	AppData(AppDataType type) { type_ = type; }
	AppData(AppData& d) { type_ = d.type_; }
	virtual ~AppData() {}

	AppDataType type() const { return type_; }

	// The following two methods MUST be rewrited for EVERY derived classes
	virtual int size() const { return sizeof(AppData); }
	virtual AppData* copy() = 0;
};

// Models any entity that is capable of process an ADU. 
// The basic functionality of this entity is to (1) process data, 
// (2) pass data to another entity, (3) request data from another entity.
class Process : public TclObject {
public: 
	Process() : target_(0) {}
	inline Process*& target() { return target_; }

	// Process incoming data
	virtual void process_data(int size, AppData* data);

	// Request data from the previous application in the chain
	virtual AppData* get_data(int& size, AppData* req_data = 0);

	// Send data to the next application in the chain
	virtual void send_data(int size, AppData* data = 0) {
		if (target_)
			target_->process_data(size, data);
	}

protected:
	virtual int command(int argc, const char*const* argv);
	Process* target_;
};

#endif // ns_process_h
