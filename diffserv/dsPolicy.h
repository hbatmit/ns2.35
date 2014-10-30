/*
 * Copyright (c) 2000 Nortel Networks
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
 *      This product includes software developed by Nortel Networks.
 * 4. The name of the Nortel Networks may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORTEL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NORTEL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Developed by: Farhan Shallwani, Jeremy Ethridge
 *               Peter Pieda, and Mandeep Baines
 * Maintainer: Peter Pieda <ppieda@nortelnetworks.com>
 */

#ifndef DS_POLICY_H
#define DS_POLICY_H
#include "dsred.h"

#define ANY_HOST -1		// Add to enable point to multipoint policy
#define FLOW_TIME_OUT 5.0      // The flow does not exist already.
#define MAX_POLICIES 20		// Max. size of Policy Table.

#define Null 0
#define TSW2CM 1
#define TSW3CM 2
#define TB 3
#define SRTCM 4
#define TRTCM 5
#define SFD 6
#define EW 7
#define DEWP 8

enum policerType {nullPolicer, TSW2CMPolicer, TSW3CMPolicer, tokenBucketPolicer, srTCMPolicer, trTCMPolicer, SFDPolicer, EWPolicer, DEWPPolicer};

enum meterType {nullMeter, tswTagger, tokenBucketMeter, srTCMMeter, trTCMMeter, sfdTagger, ewTagger, dewpTagger};

class Policy;
class TBPolicy;

//struct policyTableEntry
struct policyTableEntry {
  nsaddr_t sourceNode, destNode;	// Source-destination pair
  int policy_index;                     // Index to the policy table.
  policerType policer;
  meterType meter;
  int codePt;		     // In-profile code point
  double cir;		     // Committed information rate (bytes per s) 
  double cbs;		     // Committed burst size (bytes)		       
  double cBucket;	     // Current size of committed bucket (bytes)     
  double ebs;		     // Excess burst size (bytes)		       
  double eBucket;	     // Current size of excess bucket (bytes)	
  double pir;		     // Peak information rate (bytes per s)
  double pbs;		     // Peak burst size (bytes)			
  double pBucket;	     // Current size of peak bucket (bytes)	       
  double arrivalTime;	     // Arrival time of last packet in TSW metering
  double avgRate, winLen;    // Used for TSW metering
};
	

// This struct specifies the elements of a policer table entry.
struct policerTableEntry {
  policerType policer;
  int initialCodePt;
  int downgrade1;
  int downgrade2;
  int policy_index;
};

// Class PolicyClassifier: keep the policy and polier tables.
class PolicyClassifier : public TclObject {
 public:
  PolicyClassifier();
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);
  void updatePolicyRTT(int argc, const char*const* argv);
  double getCBucket(const char*const* argv);
  int mark(Packet *pkt);

  // prints the policy tables
  void printPolicyTable();		
  void printPolicerTable();
  
  // The table keeps pointers to the real policy
  // Added to support multiple policy per interface.
  Policy *policy_pool[MAX_POLICIES];

protected:
  // policy table and its pointer
  policyTableEntry policyTable[MAX_POLICIES];
  int policyTableSize;
  // policer table and its pointer
  policerTableEntry policerTable[MAX_CP];
  int policerTableSize;	

  policyTableEntry* getPolicyTableEntry(nsaddr_t source, nsaddr_t dest);
  policerTableEntry* getPolicerTableEntry(int policy_index, int oldCodePt);
};

// Below are actual policy classes.
// Supper class Policy can't do anything useful.
class Policy : public TclObject {
 public:
  Policy(){};

  // Metering and policing methods:
  // Have to initialize all the pointers before actually do anything with them!
  // If not, ok with gcc but not cc!!! Nov 29, xuanc
  virtual void applyMeter(policyTableEntry *policy, Packet *pkt) = 0;
  virtual int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt) = 0;
};

// NullPolicy will do nothing. But it is also a good example to show 
// how to add a new policy.
class NullPolicy : public Policy {
 public:
  NullPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TSW2CMPolicy : public Policy {
 public:
  TSW2CMPolicy() : Policy(){};

  // protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TSW3CMPolicy : public Policy {
 public:
  TSW3CMPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TBPolicy : public Policy {
 public:
  TBPolicy() : Policy(){};

  // protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class SRTCMPolicy : public Policy {
 public:
  SRTCMPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TRTCMPolicy : public Policy {
 public:
  TRTCMPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

struct flow_entry {
  int fid;
  int src_id;
  int dst_id;
  double last_update;
  int bytes_sent;
  int count;
  struct flow_entry *next;
};

struct flow_list {
  struct flow_entry *head;
  struct flow_entry *tail;
};

class SFDPolicy : public Policy {
public:
SFDPolicy();
~SFDPolicy();

// Metering and policing methods:
void applyMeter(policyTableEntry *policy, Packet *pkt);
int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);

void printFlowTable();

 protected:
  // The table to keep the flow states.
  struct flow_list flow_table;
};

#endif
