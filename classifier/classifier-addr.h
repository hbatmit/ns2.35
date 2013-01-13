/*
 * Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Sun Microsystems, Inc.
 *
 * 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
 *      promote products derived from this software without specific prior
 *      written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
 * SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

#ifndef ns_classifier_addr_h
#define ns_classifier_addr_h

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

#define BCAST_ADDR_MASK	mask_

class AddressClassifier : public Classifier {
protected:
	virtual int classify(Packet * p);
};

/* addr. classifier that enforces reserved ports */
class ReserveAddressClassifier : public AddressClassifier {
public:
	ReserveAddressClassifier() : AddressClassifier(), reserved_(0) {}
protected:
	virtual int classify(Packet *p);
	virtual void clear(int slot);
	virtual int getnxt(NsObject *);
	virtual int command(int argc, const char*const* argv);
	int reserved_;
};

/* addr. classifier that supports limited broadcast */
class BcastAddressClassifier : public AddressClassifier {
public:
	BcastAddressClassifier() : AddressClassifier(), bcast_recver_(0) {}
	NsObject *find(Packet *);
protected:
	virtual int command(int argc, const char*const* argv);
	NsObject *bcast_recver_;
};

#endif
