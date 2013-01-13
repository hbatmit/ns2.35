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
 * $Id: dccp_opt.h,v 1.1 2010/04/06 05:09:01 tom_henderson Exp $ */

#ifndef ns_dccp_opt_h
#define ns_dccp_opt_h

#include "config.h"
#include "dccp_types.h"

//options types less than this limit consist of a single byte
#define DCCP_OPT_SINGLE_BYTE_LIMIT 32
#define DCCP_OPT_CC_START 128         //CCID specific options starts here

//total size is padded to this byte boundary
#define DCCP_OPT_ALIGNMENT 4          
#define DCCP_OPT_MAX_LENGTH 255       //maximum length of one option

#define DCCP_OPT_PRINT_ALIGNMENT 16   //print this number of option data/row

//options errors
#define DCCP_OPT_NO_ERR 0
#define DCCP_OPT_ERR_SIZE -1
#define DCCP_OPT_ERR_FULL -2
#define DCCP_OPT_ERR_NOT_FOUND -3
#define DCCP_OPT_ERR_TYPE -4
#define DCCP_OPT_ERR_WALK -5

//options
#define DCCP_OPT_PADDING 0
#define DCCP_OPT_QUIESCENCE 30    //experimental!!!
#define DCCP_OPT_CHANGEL 33
#define DCCP_OPT_CONFIRML 34
#define DCCP_OPT_CHANGER 35
#define DCCP_OPT_CONFIRMR 36
#define DCCP_OPT_ACK_VECTOR_N0 37
#define DCCP_OPT_ACK_VECTOR_N1 38
#define DCCP_OPT_ELAPSED_TIME 46

//units
#define DCCP_OPT_ELAPSED_TIME_UNIT 0.0001

/* Class DCCPOptions models dccp options in packets */
class DCCPOptions {
private:
        u_char* options_;      //array with options
	u_int16_t size_;       //size used in the array
	u_int16_t max_size_;   //the maximum size of the array
	int current_;          //current position in walk
public:
	/* Constructor 
	 * arg: size - Maximum size for stored options
	 * returns: a new DCCPOptions object
	 */
	DCCPOptions(int size);

	/* Copy constructor */
	DCCPOptions(const DCCPOptions&);

	/* Destructor */
	~DCCPOptions();

	/* Add an option
	 * arg: type     - Option type
	 *      data     - Option data
	 *      dataSize - Size of option data
	 * ret: DCCP_OPT_NO_ERR   if successful
	 *      DCCP_OPT_ERR_SIZE if the size of the option is wrong
	 *      DCCP_OPT_ERR_FULL if max_size_ is reached
	 */
        int addOption(u_int8_t type, const u_char* data, u_int8_t dataSize);

	/* Add a feature option
	 * arg: type     - Option type
	 *      feat_num - Feature number
	 *      data     - Feature data
	 *      dataSize - Size of feature data
	 * ret: like addOption() but also
	 *      DCCP_OPT_ERR_TYPE if type is not ChangeR/L or ConfirmR/L
	 */
	int addFeatureOption(u_int8_t type, u_int8_t feat_num, const u_char *data, u_int8_t dataSize);

	/* Extract an option
	 * arg: type     - Option type
	 *      data     - Array to copy option data to
	 *      maxSize  - Maximum size of data array
	 * ret: DCCP_OPT_NO_ERR if successful (for single byte option)
	 *      size of data obtained if successful for other options
	 *      DCCP_OPT_ERR_SIZE if the size of the data array is to small
	 *	DCCP_OPT_ERR_NOT_FOUND if the option is not found
	 */	    
	int getOption(u_int8_t type, u_char* data, u_int8_t maxSize);

	/* Get the total size of options, including necessary padding.
	 * ret: size of options
	 */
	u_int16_t getSize();

	/* Start a walk through the options. */
	void startOptionWalk();
	
	/* Return the next option in the option walk
	 * arg: type    - Option type found
	 *      data    - Array to copy option data to
	 *      maxSize - Maximum size of data array
	 * ret: like getOption() but also
	 *      DCCP_OPT_ERR_WALK if option walk is not initiated.
	 */
	int nextOption(u_int8_t* type, u_char* data, u_int8_t maxSize);

	/* Print all options and state (for debug) */
	void print();
};

#endif
