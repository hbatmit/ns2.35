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
 * $Id: dccp_opt.cc,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#include <stdio.h>
#include "dccp_opt.h"

/* Constructor 
 * arg: size - Maximum size for stored options
 * returns: a new DCCPOptions object
 */
DCCPOptions::DCCPOptions(int size): size_(0), max_size_(size), current_(DCCP_OPT_ERR_WALK){
	options_ = new u_char[max_size_];
}

/* Copy constructor */
DCCPOptions::DCCPOptions(const DCCPOptions& orig){
	options_ = new u_char[orig.max_size_];
	memcpy (options_, orig.options_, orig.size_);
	size_ = orig.size_;
	max_size_ = orig.max_size_;
	current_ = orig.current_;
}


/* Destructor */
DCCPOptions::~DCCPOptions(){
	delete [] options_;
}

/* Add an option
 * arg: type     - Option type
 *      data     - Option data
 *      dataSize - Size of option data
 * ret: DCCP_OPT_NO_ERR   if successful
 *      DCCP_OPT_ERR_SIZE if the size of the option is wrong
 *      DCCP_OPT_ERR_FULL if max_size_ is reached
 */
int DCCPOptions::addOption(u_int8_t type, const u_char* data, u_int8_t dataSize){
	int length = ((dataSize == 0) ? 1 : dataSize+2);
	if ((type < DCCP_OPT_SINGLE_BYTE_LIMIT && dataSize > 0) ||
	    (type >= DCCP_OPT_SINGLE_BYTE_LIMIT && dataSize == 0) ||
	    length > DCCP_OPT_MAX_LENGTH)
		return DCCP_OPT_ERR_SIZE;
	
	if (size_ + length > max_size_) //out of space
		return DCCP_OPT_ERR_FULL;

	options_[size_++] = type;
	if (dataSize > 0){
		options_[size_++] = length;
		for(int i = 0; i < dataSize; i++)
			options_[size_++] = data[i];
	}
	
	return DCCP_OPT_NO_ERR;
}

/* Add a feature option
 * arg: type     - Option type
 *      feat_num - Feature number
 *      data     - Feature data
 *      dataSize - Size of feature data
 * ret: like addOption() but also
 *      DCCP_OPT_ERR_TYPE if type is not ChangeR/L or ConfirmR/L
 */
int DCCPOptions::addFeatureOption(u_int8_t type, u_int8_t feat_num, const u_char *data,
				   u_int8_t dataSize){
	int length = dataSize + 3;
	
	if (dataSize == 0 || length > DCCP_OPT_MAX_LENGTH) //must include some data, but not too much
		return DCCP_OPT_ERR_SIZE;
	
	if (type < DCCP_OPT_CHANGEL || type > DCCP_OPT_CONFIRMR)
		return DCCP_OPT_ERR_TYPE;  //not an feature option
	
	if (size_ + length > max_size_) //out of space
		return DCCP_OPT_ERR_FULL;

	options_[size_++] = type;
	options_[size_++] = length;
	options_[size_++] = feat_num;

	for(int i = 0; i < dataSize; i++)
		options_[size_++] = data[i];

	return DCCP_OPT_NO_ERR;
}

/* Extract an option
 * arg: type     - Option type
 *      data     - Array to copy option data to
 *      maxSize  - Maximum size of data array
 * ret: DCCP_OPT_NO_ERR if successful (for single byte option)
 *      size of data obtained if successful for other options
 *      DCCP_OPT_ERR_SIZE if the size of the data array is to small
 *	DCCP_OPT_ERR_NOT_FOUND if the option is not found
 */
int DCCPOptions::getOption(u_int8_t type, u_char* data, u_int8_t maxSize){
	int walker = 0, dataSize = 0;

	while (walker < size_){
		if (options_[walker] < DCCP_OPT_SINGLE_BYTE_LIMIT) {  //single byte option
			if (options_[walker] == type)
				return DCCP_OPT_NO_ERR;
			walker++;
		} else {  //multi byte option
			dataSize = options_[walker+1]-2;
			walker += 2;
			if (options_[walker-2] == type) { 
				if (maxSize >= dataSize){
					for(int i = walker; i < walker+dataSize; i++)
						data[i-walker] = options_[i];
					return dataSize;
				} else
					return DCCP_OPT_ERR_SIZE;
				//not reached
			}
			
			walker += dataSize;
		}
		
	}
	return DCCP_OPT_ERR_NOT_FOUND;
}

/* Get the total size of options, including necessary padding.
 * ret: size of options
 */
u_int16_t DCCPOptions::getSize(){
	u_int16_t result = size_;
	if ((size_ % DCCP_OPT_ALIGNMENT) > 0) //add padding
		result += DCCP_OPT_ALIGNMENT - (size_ % DCCP_OPT_ALIGNMENT);
	return result;
}

/* Start a walk through the options. */	
void DCCPOptions::startOptionWalk(){
	current_ = 0;
}

/* Return the next option in the option walk
 * arg: type    - Option type found
 *      data    - Array to copy option data to
 *      maxSize - Maximum size of data array
 * ret: like getOption() but also
 *      DCCP_OPT_ERR_WALK if option walk is not initiated.
 */
int DCCPOptions::nextOption(u_int8_t* type, u_char* data, u_int8_t maxSize){
	if (current_ == DCCP_OPT_ERR_WALK)
		return DCCP_OPT_ERR_WALK;
	if (current_ >= size_)
		return DCCP_OPT_ERR_NOT_FOUND;

	if (options_[current_] < DCCP_OPT_SINGLE_BYTE_LIMIT) {  //single byte option
		*type = options_[current_];
		current_++;
	} else {  //multi byte option
		int dataSize = options_[current_+1]-2;
		current_ += 2;
		if (maxSize >= dataSize){
			*type = options_[current_-2];
			for(int i = current_; i < current_+dataSize; i++)
				data[i-current_] = options_[i];
			current_ += dataSize;
			return dataSize;
		} else {
			current_ -= 2;
			return DCCP_OPT_ERR_SIZE;
		}
	}
	return DCCP_OPT_NO_ERR;
}

/* Print all options and state (for debug) */
void DCCPOptions::print(){
	fprintf(stdout, "DCCPOptions :: size %d, max_size %d, current %d\n", size_, max_size_, current_);
	if (size_ == 0)
		fprintf(stdout, "No option is present\n");
	else {
		fprintf(stdout, "Option buffer:\n");
		for(int i = 0; i < size_; i++){
			fprintf(stdout, "%d ", options_[i]);
			if ( (i != 0 && (i+1) % DCCP_OPT_PRINT_ALIGNMENT == 0) || i == size_-1)
				fprintf(stdout, "\n");
		}
	}
	fflush(stdout);
}
