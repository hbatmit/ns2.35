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
 * $Id: dccp_sb.cc,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#include <stdio.h>
#include "dccp_sb.h"

/* Constructor
 * arg: size - Maximum size of buffer
 * ret: a new DCCPSendBuffer
 */
DCCPSendBuffer::DCCPSendBuffer(int size): head_(0), tail_(0),
	size_(size), empty_(true) {
	if (size_ <= 0)
		size_ = 1;
	
	buf_ = new int[size_];
}

/* Destructor */
DCCPSendBuffer::~DCCPSendBuffer(){
	delete [] buf_;
}

/* Add an element to the buffer
 * arg: size - size of element
 * ret: true if successful, false if full
 */
bool DCCPSendBuffer::add(int packet_size){
	if (empty_) {
		buf_[head_] = packet_size;
		empty_ = false;
	} else if ( (tail_+1) % size_ == head_)  //buffer is full
		return false;
	else {
		tail_ = (tail_ + 1) % size_;
		buf_[tail_] = packet_size;
	}
	return true;
}

/* Check if the buffer is full
 * ret: true if full, otherwise false
 */
bool DCCPSendBuffer::full(){
	return (!empty_ && ((tail_+1) % size_ == head_));
}

/* Check if the buffer is empty
 * ret: true if empty, otherwise false
 */
bool DCCPSendBuffer::empty(){
	return empty_;
}

/* Return the top element (without removing it)
 * ret: the top element. If empty, return 0.
 */
int DCCPSendBuffer::top(){
	int result = 0;
	if (!empty_)
		result = buf_[head_];
	return result;
}

/* Return and remove the top element
 * ret: the top element. If empty, return 0.
 */
int DCCPSendBuffer::remove(){
	int result = 0;
	if (!empty_){
		result = buf_[head_];
		//remove element
		if (head_ != tail_)
			head_ = (head_+1) % size_;
		else
			empty_ = true;
	}
	return result;
}

/* Print send buffer contents and state (for debug) */
void DCCPSendBuffer::print(){
	int walker_ = head_;
	if (empty_)
		fprintf(stdout, "DCCPSendBuffer :: Buffer is empty (size %d, head %d, tail %d)\n", size_, head_, tail_);
	else {
		fprintf(stdout, "DCCPSendBuffer :: Buffer (size %d, head %d, tail %d): ", size_, head_, tail_);
		while(walker_ != tail_){
			fprintf(stdout, "%d ",buf_[walker_]);
			walker_ = (walker_ + 1) % size_;
		}
		fprintf(stdout, "%d\n",buf_[walker_]);
	}
	fflush(stdout);
}

