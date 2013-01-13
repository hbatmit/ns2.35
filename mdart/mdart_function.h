/*******************************************************************************
 *                                                                             *
 *   M-DART Routing Protocol                                                   *
 *                                                                             *
 *   Copyright (C) 2006 by Marcello Caleffi                                    *
 *   marcello.caleffi@unina.it                                                 *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright notice,   *
 *    this list of conditions and the following disclaimer.                    *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. The name of the author may not be used to endorse or promote products    *
 *    derived from this software without specific prior written permission.    *
 *                                                                             *
 * This software is provided by the author ``as is'' and any express or        *
 * implied warranties, including, but not limited to, the implied warranties   *
 * of merchantability and fitness for a particular purpose are disclaimed.     *
 * in no event shall the author be liable for any direct, indirect,            *
 * incidental, special, exemplary, or consequential damages (including, but    *
 * not limited to, procurement of substitute goods or services; loss of use,   *
 * data, or profits; or business interruption) however caused and on any       *
 * theory of liability, whether in contract, strict liability, or tort         *
 * (including negligence or otherwise) arising in any way out of the use of    *
 * this software, even if advised of the possibility of such damage.           *
 *                                                                             *
 * The M-DART code has been developed by Marcello Caleffi during his Ph.D. at  *
 * the Department of Biomedical, Electronic and Telecommunications Engineering *
 * University of Naples Federico II, Italy.                                    *
 *                                                                             *
 * In order to give credit and recognition to the author, if you use M-DART    *
 * results or results obtained by modificating the M-DART source code, please  *
 * cite one of the following papers:                                           *
 * - M. Caleffi, L. Paura, "M-DART: Multi-Path Dynamic Address RouTing",       *
 *   Wireless Communications and Mobile Computing, 2010                        *
 * - M. Caleffi, G. Ferraiuolo, L. Paura, "Augmented Tree-based Routing        *
 *   Protocol for Scalable Ad Hoc Networks", Proc. of IEEE MASS '07: IEEE      *
 *   Internatonal Conference on Mobile Adhoc and Sensor Systems, Pisa (Italy), *
 *   October 8-11 2007.                                                        *
 *                                                                             *
 ******************************************************************************/



#ifndef __mdart_function__
#define __mdart_function__



#include <random.h>
#include <packet.h>
#include <agent.h>
#include <trace.h>
#include <timer-handler.h>
#include <classifier-port.h>
#include <cmu-trace.h>



#include <bitset>
#include <string>
#include <sstream>
#include <set>
#include <vector>



#include <mdart/mdart_packet.h>



//------------------------------------------------------------------------------
// Defining debug facilities
//------------------------------------------------------------------------------
//#define DEBUG
//#define DEBUG_PACKET_FORWARDING
//#define DEBUG_ADDRESS_ALLOCATION
//#define DEBUG_NDP
//#define DEBUG_NDP_LINK_QUALITY
//#define DEBUG_NEIGHBOR
//#define DEBUG_TABLE
//#define DEBUG_ADP
//#define DEBUG_IDP_MESSAGE
//#define DEBUG_DHT
//#define DEBUG_QUEUE
//#define DEBUG_PRINT_TABLE
//#define DEBUG_ROBY



//------------------------------------------------------------------------------
// Defining improvements control
//------------------------------------------------------------------------------
// To allow to recovery packet dropped by MAC layer
//#define MAC_FAILED_RECOVERY
// Recovering past invalidation before to invalidate  
//#define MAC_FAILED_RECOVERY_FAST
// Number of retransimission
#define MAC_FAILED_ALLOWED_LOSS		5


//------------------------------------------------------------------------------
// Defining some general constants
//------------------------------------------------------------------------------
#define ADDR_SIZE						16
#define INFINITO						0xFFFF
#define CURRENT_TIME					Scheduler::instance().clock()
#define JITTER							(Random::uniform() * 0.5)
#define MESH_SIZE						2
// Defining the number of events utilized by NDP in order to estimate link quality
#define NDP_MEMORY					5
// Defining the weigth associated to the events by NDP in order to estimate link quality
#define NDP_WEIGTH					0.5



//------------------------------------------------------------------------------
// Defining constants for Neighbor Discovery Protocol (NDP)
//------------------------------------------------------------------------------
//#define FAST_ADDRESS_CONVERGENCE

// Hello interval [second]
#define NDP_HELLO_INTERVAL			1.0
// Constants used for HELLO delay [second]
#define NDP_MAX_HELLO_INTERVAL	(1.25 * NDP_HELLO_INTERVAL)
#define NDP_MIN_HELLO_INTERVAL	(0.75 * NDP_HELLO_INTERVAL)
// Allowing HELLO packet losses [packet]
#define NDP_ALLOWED_HELLO_LOSS	3
// Timeout of a neighbor [second]
//#define NDP_NEIGHBOR_EXPIRE		(1.5 * NDP_ALLOWED_HELLO_LOSS * NDP_HELLO_INTERVAL)



//------------------------------------------------------------------------------
// Defining constants for route metric
//------------------------------------------------------------------------------
// Maximum value for ETX metric
#define RTR_ETX_MAX					512



//------------------------------------------------------------------------------
// Defining constants for Link Quality Estimation (LQE)
//------------------------------------------------------------------------------
// link quality threshold
#define LQE_THRESHOLD				0.6
// link quality reverse threshold
#define LQE_RV_THRESHOLD			0.4
// Hello packets expected for ETX link quality metric [packet]
#define LQE_MA_EXPECTED_HELLO		10.0
#define NDP_NEIGHBOR_EXPIRE		10.5


//------------------------------------------------------------------------------
// Defining constants for packet queue
//------------------------------------------------------------------------------
// Maximum number of buffered packets
#define MDART_QUEUE_LEN				64
// Maximum period of buffering time [seconds]
#define MDART_QUEUE_TIMEOUT			30
// Purge interval [second]
#define MDART_QUEUE_PURGE_INTERVAL	1.0



//------------------------------------------------------------------------------
// Defining constants for Address Discovery Process (ADP)
//------------------------------------------------------------------------------
// 
#define MDART_DARQ_RETRANSMIT	5
// DAUP packet frequency [second]
#define ADP_DAUP_FREQ				5.0
// Timeout of a DAUP packet [second]
#define ADP_DAUP_TIMEOUT			5.0
// Timeout of a DARQ packet [second]
#define ADP_DARQ_TIMEOUT			5.0
// DARQ packet check frequency [second]
#define ADP_DARQ_CHECK_FREQ			0.1
// Timeout of a DARP [second]
#define ADP_DARP_TIMEOUT			5.0
// Exponential backoff for DARP packets 
#define ADP_DARQ_EB
// Timeout of a DARP [second] with the exponential backoff 
#define ADP_DARQ_EB_TIMEOUT			5.0
// DABR packet to handle mobility
#define ADP_DABR_ENABLE
// Network address caching by means of hello packets
#define ADP_HELLO_CACHING
// Network address caching by means of ADP packets
#define ADP_DAXX_CACHING
// Network address caching by means of ADP packets
#define ADP_DATA_CACHING



//------------------------------------------------------------------------------
// Defining constants for DHT
//------------------------------------------------------------------------------
// DHT purge interval [second]
#define MDART_DHT_PURGE_INTERVAL		5.0
// Timeout of an entry [second]: allowing 10 DAUP packet losses
#define MDART_DHT_ENTRY_TIMEOUT		10 * ADP_DAUP_FREQ



//------------------------------------------------------------------------------
// General purpose functions
//------------------------------------------------------------------------------
inline const char* bitString(nsaddr_t add) {
	bitset<ADDR_SIZE> tempSet (add);
	string temp = tempSet.to_string();
//	tempSet.reset();
	return temp.c_str();
}

inline int DiffBit(nsaddr_t n1, nsaddr_t n2) {
	bitset<ADDR_SIZE> tempN1 (n1);
	bitset<ADDR_SIZE> tempN2 (n2);
	int i;
	for(i=ADDR_SIZE-1; i>-1; i--) {
		if (tempN1.test(i) != tempN2.test(i)) {
			break;
		}
	}
	return i;
}

inline nsaddr_t hash(nsaddr_t id) {
	bitset<ADDR_SIZE> tempAddress_ (id);
	bitset<ADDR_SIZE> address_;
	for(unsigned int i=0; i<ADDR_SIZE; i++) {
		if (tempAddress_.test(i)) {
			address_.set(ADDR_SIZE - 1 - i);
		}
	}
	address_.flip(ADDR_SIZE - 1);
	nsaddr_t temp = (nsaddr_t) address_.to_ulong();
#ifdef DEBUG
	fprintf(stdout, "\thash = %s\n", bitString(temp));
#endif
	return temp;
}

/*inline nsaddr_t idToAdd(string id) {
	hashwrapper *myWrapper = new md5wrapper();
	std::string hash = myWrapper->getHashFromString(id);
	delete myWrapper;	
	char hashChar[34];
	strcpy(hashChar, hash.c_str());
	int len = lround(ADDR_SIZE/4);
	hashChar[len] = '\0';
	unsigned long addrLong = strtoul(hashChar, '\0', 16);
	nsaddr_t addr = (nsaddr_t) addrLong;
	return addr;
}*/



#endif /*__mdart_function__*/
