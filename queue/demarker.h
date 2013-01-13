/*
 * Copyright (c) 2000-2002, by the Rector and Board of Visitors of the 
 * University of Virginia.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer. 
 *
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 *
 * Neither the name of the University of Virginia nor the names 
 * of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written 
 * permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *                                                                     
 * Marker module for JoBS (and WTP).
 *                                                                     
 * Authors: Constantinos Dovrolis <dovrolis@mail.eecis.udel.edu>, 
 *          Nicolas Christin <nicolas@cs.virginia.edu>, 2000-2002       
 *								      
 * $Id: demarker.h,v 1.1 2003/02/02 22:18:22 xuanc Exp $
 */

#ifndef DEMARKER_H
#define DEMARKER_H

#define START_STATISTICS 0.0	// When do we start writing the delays? (sec)
#define NO_CLASSES 	4	// This number cannot be changed 
				// w/o modifying the code. 
				// This is, again, a prototype 
				// implementation...
#define VERBOSE	1		// Write e2e delay of packet in trace file 
				// (per class)
#define QUIET	2		// Do not do anything basically


class Demarker: public Queue {
public:
	Demarker();
	virtual int command(int argc, const char*const* argv); 
	void 	enque(Packet*);
	Packet* deque();
	double  demarker_arrvs_[NO_CLASSES+1];
protected:
	int 	demarker_type_;		// Demarker Type
	PacketQueue	*q_;		// Underlying FIFO queue 
	FILE*	delay_tr_[NO_CLASSES+1];// Delay trace per class
	char*	file_name_;		// Trace filename
private:
	int link_id_;
	double arrived_Bits_[NO_CLASSES+1];
	double monitoring_window_;
	double last_monitor_update_;
};

#endif /* DEMARKER_H */
