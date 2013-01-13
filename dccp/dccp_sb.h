/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* Copyright (c) 2004  Nils-Erik Mattsson 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: dccp_sb.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_sb_h
#define ns_dccp_sb_h

/* A (circular) send buffer consisting of packet sizes */
class DCCPSendBuffer {
private:
	int* buf_;                 //buffer
	int head_,tail_;           //head and tail of circular buffer
	int size_;                 //maximum size of buffer
	bool empty_;               //if its empty or not
public:
	/* Constructor
	 * arg: size - Maximum size of buffer
	 * ret: a new DCCPSendBuffer
	 */
	DCCPSendBuffer(int size);

	/* Destructor */
	~DCCPSendBuffer();

	/* Add an element to the buffer
	 * arg: size - size of element
	 * ret: true if successful, false if full
	 */
	bool add(int size);

	/* Check if the buffer is full
	 * ret: true if full, otherwise false
	 */
	bool full();

	/* Check if the buffer is empty
	 * ret: true if empty, otherwise false
	 */
	bool empty();

	/* Return the top element (without removing it)
	 * ret: the top element. If empty, return 0.
	 */
	int top();

	/* Return and remove the top element
	 * ret: the top element. If empty, return 0.
	 */
	int remove();

	/* Print send buffer contents and state (for debug) */
	void print();
};

#endif

