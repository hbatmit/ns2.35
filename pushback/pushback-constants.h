/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2000  International Computer Science Institute
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef ns_pushback_constants_h
#define ns_pushback_constants_h

/* #define DEBUG_LGDS */
/* #define DEBUG_LGDSN */
/* #define DEBUG_AS  */
/* #define DEBUG_RLSL */

#define NO_BITS 4
#define MAX_CLUSTER 20
#define CLUSTER_LEVEL 4
#define MIN_BITS_FOR_MERGER 2

//make it 0 to classify on the basis of dst addresses
//make it 1 to classify on the basis of flowid
#define AGGREGATE_CLASSIFICATION_MODE_FID 1

//0 for the old version
//1 for the dynamic version
#define LOWER_BOUND_MODE 1

//maximum number of rate-limiting sessions that a congested router can start.
#define MAX_SESSIONS 3

//0 for No Mergers of aggregate prefixes
//1 for Mergers
#define MERGER_MODE 0

// maximum no of queues on the node
#define MAX_QUEUES 10

//min time to release an aggregate after starting to rate-limit it.
#define EARLIEST_TIME_TO_FREE 10

//min time to release an aggregate after it goes below limit imposed on it.
#define MIN_TIME_TO_FREE 20
#define PRIMARY_WAITING_ZONE 10
#define RATE_LIMIT_TIME_DEFAULT 30    //in seconds
#define DEFAULT_BUCKET_DEPTH  5000           //in bytes


#define PUSHBACK_CHECK_EVENT 1
#define PUSHBACK_REFRESH_EVENT 2
#define PUSHBACK_STATUS_EVENT 3
#define INITIAL_UPDATE_EVENT 4

#define PUSHBACK_CHECK_TIME  5   //in seconds
#define PUSHBACK_CYCLE_TIME 5
#define INITIAL_UPDATE_TIME 0.5

#define DROP_RATE_FOR_PUSHBACK 0.10

#define SUSTAINED_CONGESTION_PERIOD  2      //in seconds
#define SUSTAINED_CONGESTION_DROPRATE 0.10  //fraction
#define TARGET_DROPRATE 0.05  

#define MAX_PACKET_TYPES 4
#define PACKET_TYPE_TIMER 2

#define INFINITE_LIMIT 100e+6 //100Mbps

#endif
