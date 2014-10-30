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

/* 
 *  Integrated into ns main distribution and reorganized by 
 *  Xuan Chen (xuanc@isi.edu). The main changes are:
 *
 *  1. Defined two seperated classes, PolicyClassifier and Policy, to handle 
 *     the work done by class Policy before.
 *     Class PolicyClassifier now only keeps states for each flow and pointers
 *     to certain policies. 
 *     The policies perform the diffserv related jobs as described
 *     below. (eg, traffic metering and packet marking.)
 *     class Policy functions like the class Classifer.
 *
 *  2. Created a general supper class Policy so that new policy can be added
 *     by just creating a subclass of Policy. Examples are given (eg, 
 *     DumbPolicy) to help people trying to add their own new policies.
 *
 *  TODO:
 *  1. implement the multiple policy support by applying the idea of 
 *     multi-policy.
 *
 */

#include "dsPolicy.h"
#include "ew.h"
#include "dewp.h"
#include "packet.h"
#include "tcp.h"
#include "random.h"

// The definition of class PolicyClassifier.
//Constructor.
PolicyClassifier::PolicyClassifier() {
  int i;

  policyTableSize = 0;
  policerTableSize = 0;

  for (i = 0; i < MAX_POLICIES; i++) 
    policy_pool[i] = NULL;
}

/*-----------------------------------------------------------------------------
void addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  Supported policies and their parameters
are:

Null          InitialCodePoint
TSW2CM        InitialCodePoint  CIR
TSW3CM        InitialCodePoint  CIR  PIR
TokenBucket   InitialCodePoint  CIR  CBS
srTCM         InitialCodePoint  CIR  CBS  EBS
trTCM         InitialCodePoint  CIR  CBS  PIR  PBS

    No error-checking is performed on the parameters.  CIR and PIR should be
specified in bits per second; CBS, EBS, and PBS should be specified in bytes.

    If the Policy Table is full, this method prints an error message.
-----------------------------------------------------------------------------*/
void PolicyClassifier::addPolicyEntry(int argc, const char*const* argv) {
  if (policyTableSize == MAX_POLICIES)
    printf("ERROR: Policy Table size limit exceeded.\n");
  else {
    policyTable[policyTableSize].sourceNode = atoi(argv[2]);
    policyTable[policyTableSize].destNode = atoi(argv[3]);
    policyTable[policyTableSize].codePt = atoi(argv[5]);
    policyTable[policyTableSize].arrivalTime = 0;
    policyTable[policyTableSize].winLen = 1.0;
    
    if ((strcmp(argv[4], "Dumb") == 0) || (strcmp(argv[4],"Null") == 0)) {
      if(!policy_pool[Null])
	policy_pool[Null] = new NullPolicy;
      policyTable[policyTableSize].policy_index = Null;   
      policyTable[policyTableSize].policer = nullPolicer;
      policyTable[policyTableSize].meter = nullMeter;
    } else if (strcmp(argv[4], "TSW2CM") == 0) {
      if(!policy_pool[TSW2CM])
	policy_pool[TSW2CM] = new TSW2CMPolicy;
      policyTable[policyTableSize].policy_index = TSW2CM;   
      policyTable[policyTableSize].policer = TSW2CMPolicer;
      policyTable[policyTableSize].meter = tswTagger;

      policyTable[policyTableSize].cir =
	policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
      if (argc == 8) policyTable[policyTableSize].winLen = (double) atof(argv[7]);/* mb */
    } else if (strcmp(argv[4], "TSW3CM") == 0) {
      if(!policy_pool[TSW3CM])
	policy_pool[TSW3CM] = new TSW3CMPolicy;
      policyTable[policyTableSize].policy_index = TSW3CM;   
      policyTable[policyTableSize].policer = TSW3CMPolicer;
      policyTable[policyTableSize].meter = tswTagger;

      policyTable[policyTableSize].cir =
	policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
      policyTable[policyTableSize].pir = (double) atof(argv[7]) / 8.0;
    } else if (strcmp(argv[4], "TokenBucket") == 0) {
      if(!policy_pool[TB])
	policy_pool[TB] = (Policy *) new TBPolicy;
      policyTable[policyTableSize].policy_index = TB;   
      policyTable[policyTableSize].policer = tokenBucketPolicer;
      policyTable[policyTableSize].meter = tokenBucketMeter;
      
      policyTable[policyTableSize].cir =
	policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
      policyTable[policyTableSize].cbs =
	policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
    } else if (strcmp(argv[4], "srTCM") == 0) {
      if(!policy_pool[SRTCM])
	policy_pool[SRTCM] = new SRTCMPolicy;
      policyTable[policyTableSize].policy_index = SRTCM;   
      policyTable[policyTableSize].policer = srTCMPolicer;
      policyTable[policyTableSize].meter = srTCMMeter;      

      policyTable[policyTableSize].cir =
	policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
      policyTable[policyTableSize].cbs =
	policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
      policyTable[policyTableSize].ebs =
	policyTable[policyTableSize].eBucket = (double) atof(argv[8]);
    } else if (strcmp(argv[4], "trTCM") == 0) {
      if(!policy_pool[TRTCM])
	policy_pool[TRTCM] = new TRTCMPolicy;
      policyTable[policyTableSize].policy_index = TRTCM;  
      policyTable[policyTableSize].policer = trTCMPolicer;
      policyTable[policyTableSize].meter = trTCMMeter;
      
      policyTable[policyTableSize].cir =
	policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
      policyTable[policyTableSize].cbs =
	policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
      policyTable[policyTableSize].pir = (double) atof(argv[8]) / 8.0;
      policyTable[policyTableSize].pbs =
	policyTable[policyTableSize].pBucket = (double) atof(argv[9]);
    } else if (strcmp(argv[4], "SFD") == 0) {
      if(!policy_pool[SFD])
	policy_pool[SFD] = new SFDPolicy;
      policyTable[policyTableSize].policy_index = SFD;
      policyTable[policyTableSize].policer = SFDPolicer;
      policyTable[policyTableSize].meter = sfdTagger;

      // Use cir as the transmission size threshold for the moment.
      policyTable[policyTableSize].cir = atoi(argv[6]);
    } else if (strcmp(argv[4], "EW") == 0) {
      if(!policy_pool[EW])
	policy_pool[EW] = new EWPolicy();
      
      ((EWPolicy *)policy_pool[EW])->
	init(atoi(argv[6]), atoi(argv[7]), atoi(argv[8]));

      policyTable[policyTableSize].policy_index = EW;
      policyTable[policyTableSize].policer = EWPolicer;
      policyTable[policyTableSize].meter = ewTagger;
  } else if (strcmp(argv[4], "DEWP") == 0) {
    if(!policy_pool[DEWP])
      policy_pool[DEWP] = new DEWPPolicy;

    ((DEWPPolicy *)policy_pool[DEWP])->
      init(atof(argv[6]));
    
    policyTable[policyTableSize].policy_index = DEWP;
    policyTable[policyTableSize].policer = DEWPPolicer;
    policyTable[policyTableSize].meter = dewpTagger;
  } else {
      printf("No applicable policy specified, exit!!!\n");
      exit(-1);
    }
    policyTableSize++;
  }
}

/*-----------------------------------------------------------------------------
policyTableEntry* PolicyClassifier::getPolicyTableEntry(long source, long dest)
Pre: policyTable holds exactly one entry for the specified source-dest pair.
Post: Finds the policyTable array that matches the specified source-dest pair.
Returns: On success, returns a pointer to the corresponding policyTableEntry;
  on failure, returns NULL.
Note: the source-destination pair could be one-any or any-any (xuanc)
-----------------------------------------------------------------------------*/
policyTableEntry* PolicyClassifier::getPolicyTableEntry(nsaddr_t source, nsaddr_t dest) {
  for (int i = 0; i <= policyTableSize; i++) {
    if ((policyTable[i].sourceNode == source) || (policyTable[i].sourceNode == ANY_HOST)) {
      if ((policyTable[i].destNode == dest) || (policyTable[i].destNode == ANY_HOST))
	return(&policyTable[i]);
    }
  }
  
  // !!! Could make a default code point for undefined flows:
  printf("ERROR: No Policy Table entry found for Source %d-Destination %d.\n", source, dest);
  printPolicyTable();
  return(NULL);
}

/*-----------------------------------------------------------------------------
void addPolicerEntry(int argc, const char*const* argv)
Pre: argv contains a valid command line for adding a policer entry.
Post: Adds an entry to policerTable according to the arguments in argv.  No
  error-checking is done on the arguments.  A policer type should be specified,
  consisting of one of the names {Null, TSW2CM, TSW3CM, TokenBucket,
  srTCM, trTCM}, followed by an initial code point.  Next should be an
  out-of-profile code point for policers with two-rate markers; or a yellow and
  a red code point for policers with three drop precedences.
      If policerTable is full, an error message is printed.
-----------------------------------------------------------------------------*/
void PolicyClassifier::addPolicerEntry(int argc, const char*const* argv) {
  //int cur_policy;


  if (policerTableSize == MAX_CP)
    printf("ERROR: Policer Table size limit exceeded.\n");
  else {
    if ((strcmp(argv[2], "Dumb") == 0) || (strcmp(argv[2],"Null") == 0)) {
      if(!policy_pool[Null])
	policy_pool[Null] = new NullPolicy;
      policerTable[policerTableSize].policer = nullPolicer;      
      policerTable[policerTableSize].policy_index = Null;      
    } else if (strcmp(argv[2], "TSW2CM") == 0) {
      if(!policy_pool[TSW2CM])
	policy_pool[TSW2CM] = new TSW2CMPolicy;
      policerTable[policerTableSize].policer = TSW2CMPolicer;
      policerTable[policerTableSize].policy_index = TSW2CM;      
    } else if (strcmp(argv[2], "TSW3CM") == 0) {
      if(!policy_pool[TSW3CM])
	policy_pool[TSW3CM] = new TSW3CMPolicy;
      policerTable[policerTableSize].policer = TSW3CMPolicer;
      policerTable[policerTableSize].policy_index = TSW3CM;      
    } else if (strcmp(argv[2], "TokenBucket") == 0) {
      if(!policy_pool[TB])
	policy_pool[TB] = new TBPolicy;
      policerTable[policerTableSize].policer = tokenBucketPolicer;
      policerTable[policerTableSize].policy_index = TB;      
    } else if (strcmp(argv[2], "srTCM") == 0) {
      if(!policy_pool[SRTCM])
	policy_pool[SRTCM] = new SRTCMPolicy;
      policerTable[policerTableSize].policer = srTCMPolicer;
      policerTable[policerTableSize].policy_index = SRTCM;      
    } else if (strcmp(argv[2], "trTCM") == 0){
      if(!policy_pool[TRTCM])
	policy_pool[TRTCM] = new TRTCMPolicy;
      policerTable[policerTableSize].policer = trTCMPolicer;
      policerTable[policerTableSize].policy_index = TRTCM;      
    } else if (strcmp(argv[2], "SFD") == 0) {
      if(!policy_pool[SFD])
	policy_pool[SFD] = new SFDPolicy;
      policerTable[policerTableSize].policer = SFDPolicer;
      policerTable[policerTableSize].policy_index = SFD;      
    } else if (strcmp(argv[2], "EW") == 0) {
      if(!policy_pool[EW])
	policy_pool[EW] = new EWPolicy;
      policerTable[policerTableSize].policer = EWPolicer;
      policerTable[policerTableSize].policy_index = EW;      
    } else if (strcmp(argv[2], "DEWP") == 0) {
      if(!policy_pool[DEWP])
	policy_pool[DEWP] = new DEWPPolicy;
      policerTable[policerTableSize].policer = DEWPPolicer;
      policerTable[policerTableSize].policy_index = DEWP;      
    } else {
      printf("No applicable policer specified, exit!!!\n");
      exit(-1);
    }
  };
  
  policerTable[policerTableSize].initialCodePt = atoi(argv[3]);
  if (policerTable[policerTableSize].policer == nullPolicer)
      policerTable[policerTableSize].downgrade1 = atoi(argv[3]);
    else
      policerTable[policerTableSize].downgrade1 = atoi(argv[4]);
  if (argc == 6)
    policerTable[policerTableSize].downgrade2 = atoi(argv[5]);
  policerTableSize++;
}

// Return the entry of Policer table with policerType and initCodePoint matched
policerTableEntry* PolicyClassifier::getPolicerTableEntry(int policy_index, int oldCodePt) {
  for (int i = 0; i < policerTableSize; i++)
    if ((policerTable[i].policy_index == policy_index) &&
	(policerTable[i].initialCodePt == oldCodePt))
      return(&policerTable[i]);

  printf("ERROR: No Policer Table entry found for initial code point %d.\n", oldCodePt);
  //printPolicerTable();
  return(NULL);
}

/*-----------------------------------------------------------------------------
int mark(Packet *pkt, double minRTT)
Pre: The source-destination pair taken from pkt matches a valid entry in
  policyTable.
Post: pkt is marked with an appropriate code point.
-----------------------------------------------------------------------------*/
int PolicyClassifier::mark(Packet *pkt) {
  policyTableEntry *policy;
  policerTableEntry *policer;
  int policy_index;
  int codePt;
  hdr_ip* iph;
  
  iph = hdr_ip::access(pkt);
  policy = getPolicyTableEntry(iph->saddr(), iph->daddr());
  if (policy) {
    codePt = policy->codePt;
    policy_index = policy->policy_index;
    policer = getPolicerTableEntry(policy_index, codePt);

    // bug pointed by Jason Kenney <jason@linear.engmath.dal.ca>
    if (policy_pool[policy_index]) {
      policy_pool[policy_index]->applyMeter(policy, pkt);
      codePt = policy_pool[policy_index]->applyPolicer(policy, policer, pkt);
    }
  } else {
    printf("The policy object doesn't exist, ERROR!!!\n");
    exit(-1);    
  }
  
  iph->prio_ = codePt;
  return(codePt);
}

/*-----------------------------------------------------------------------------
Pre: The command line specifies a source and destination node for which an
  RTT-Aware policy exists and a current RTT value for that policy.
Post: The aggRTT field of the appropriate policy is updated to a weighted
  average of the previous value and the new RTT value specified in the command
  line.  If no matching policy is found, an error message is printed.
-----------------------------------------------------------------------------*/
void PolicyClassifier::updatePolicyRTT(int , const char*const* argv) {
  policyTableEntry *policy;
  
  policy = getPolicyTableEntry(atoi(argv[2]), atoi(argv[3]));
  if (policy == NULL)
    printf("ERROR: cannot update RTT; no existing policy found for Source %d-Desination %d.\n",
	   atoi(argv[2]), atoi(argv[3]));
  else {
    policy->winLen = (double) atof(argv[4]);
  }
}

/*-----------------------------------------------------------------------------
Pre: The command line specifies a source and destination node for which a
  policy exists that uses a cBucket value.  That policy's cBucket parameter is
  currently valid.
Post: The policy's cBucket value is found and returned.
Returns: The value cBucket on success; or -1 on an error.
-----------------------------------------------------------------------------*/
double PolicyClassifier::getCBucket(const char*const* argv) {
  policyTableEntry *policy;
  
  policy = getPolicyTableEntry(atoi(argv[2]), atoi(argv[3]));
  if (policy == NULL) {
    printf("ERROR: cannot get bucket size; no existing policy found for Source %d-Desination %d.\n",
	   atoi(argv[2]), atoi(argv[3]));
    return(-1);
  }
  else {
    if ((policy->policer == tokenBucketPolicer) || (policy->policer == srTCMPolicer) || (policy->policer == trTCMPolicer))
      return(policy->cBucket);
    else {
      printf("ERROR: cannot get bucket size; the Source %d-Desination %d Policy does not include a Committed Bucket.\n", atoi(argv[2]), atoi(argv[3]));
      return(-1);
    }
  }
}

//    Prints the policyTable, one entry per line.
void PolicyClassifier::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++)
    {
      switch (policyTable[i].policer) {
      case nullPolicer:
	printf("Flow (%d to %d): Null policer, ",
               policyTable[i].sourceNode,policyTable[i].destNode);
	printf("initial code point %d\n", policyTable[i].codePt);
	break;
      case TSW2CMPolicer:
	printf("Flow (%d to %d): TSW2CM policer, ",
               policyTable[i].sourceNode,policyTable[i].destNode);
	printf("initial code point %d, CIR %.1f bps.\n",
               policyTable[i].codePt, policyTable[i].cir * 8);
	break;
      case TSW3CMPolicer:
	printf("Flow (%d to %d): TSW3CM policer, initial code ",
               policyTable[i].sourceNode, policyTable[i].destNode);
	printf("point %d, CIR %.1f bps, PIR %.1f bytes.\n",
               policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].pir * 8);
	break;
      case tokenBucketPolicer:
	printf("Flow (%d to %d): Token Bucket policer, ",
               policyTable[i].sourceNode,policyTable[i].destNode);
	printf("initial code  point %d, CIR %.1f bps, CBS %.1f bytes.\n",
               policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].cbs);
	break;
      case srTCMPolicer:
	printf("Flow (%d to %d): srTCM policer, initial code ",
               policyTable[i].sourceNode, policyTable[i].destNode);
	printf("point %d, CIR %.1f bps, CBS %.1f bytes, EBS %.1f bytes.\n",
               policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].cbs, policyTable[i].ebs);
	break;
      case trTCMPolicer:
	printf("Flow (%d to %d): trTCM policer, initial code ",
               policyTable[i].destNode, policyTable[i].sourceNode);
	printf("point %d, CIR %.1f bps, CBS %.1f bytes, PIR %.1f bps, ",
	       policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].cbs, policyTable[i].pir * 8);
	printf("PBS %.1f bytes.\n", policyTable[i].pbs);
	break;
      case SFDPolicer:
	printf("Flow (%d to %d): SFD policer, ",
	       policyTable[i].sourceNode,policyTable[i].destNode);
	printf("initial code point %d, TH %d bytes.\n",
	       policyTable[i].codePt, (int)policyTable[i].cir);
	break;
      case EWPolicer:
	printf("Flow (%d to %d): EW policer, ",
	       policyTable[i].sourceNode,policyTable[i].destNode);
	printf("initial code point %d.\n", policyTable[i].codePt);
	break;
      case DEWPPolicer:
	printf("Flow (%d to %d): DEWP policer, ",
	       policyTable[i].sourceNode,policyTable[i].destNode);
	printf("initial code point %d.\n", policyTable[i].codePt);
	break;
      default:
	printf("ERROR: Unknown policer type in Policy Table.\n");
      }
    }
  printf("\n");
}

// Prints the policerTable, one entry per line.
void PolicyClassifier::printPolicerTable() {
  bool threeColor;
  
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) {
    threeColor = false;
    switch (policerTable[i].policer) {
    case nullPolicer:
      printf("Null ");
      break;
    case TSW2CMPolicer:
      printf("TSW2CM ");
      break;
    case TSW3CMPolicer:
      printf("TSW3CM ");
      threeColor = true;
      break;
    case tokenBucketPolicer:
      printf("Token Bucket ");
      break;
    case srTCMPolicer:
      printf("srTCM ");
      threeColor = true;
      break;
    case trTCMPolicer:
      printf("trTCM ");
      threeColor = true;
      break;
    case SFDPolicer:
      printf("SFD ");
      //printFlowTable();
      break;
    case EWPolicer:
      printf("EW ");
      break;
    case DEWPPolicer:
      printf("DEWP ");
      break;
    default:
      printf("ERROR: Unknown policer type in Policer Table.");
    }
    
    if (threeColor) {
      printf("policer code point %d is policed to yellow ",
	     policerTable[i].initialCodePt);
      printf("code point %d and red code point %d.\n",
	     policerTable[i].downgrade1,
	     policerTable[i].downgrade2);
    } else
      printf("policer code point %d is policed to code point %d.\n",
	     policerTable[i].initialCodePt,
	     policerTable[i].downgrade1);
  }
  printf("\n");
}

// The beginning of the definition of the NullPolicy
// NullPolicy will do nothing, but is also a good example to show how to add 
// new policy.

/*-----------------------------------------------------------------------------
void NullPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Do nothing
-----------------------------------------------------------------------------*/
void NullPolicy::applyMeter(policyTableEntry *policy, Packet *) {
  policy->arrivalTime = Scheduler::instance().clock();  
}

/*-----------------------------------------------------------------------------
int DumbPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt)
Always return the initial codepoint.
-----------------------------------------------------------------------------*/
int NullPolicy::applyPolicer(policyTableEntry *, policerTableEntry *policer, Packet *) {
  return(policer->initialCodePt);
}

// The end of NullPolicy

// The beginning of the definition of TSW2CM
/*-----------------------------------------------------------------------------
void TSW2CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables avgRate, arrivalTime, and winLen hold valid values; and
  pkt points to a newly-arrived packet.
Post: Adjusts policy's TSW state variables avgRate and arrivalTime (also called
  tFront) according to the specified packet.
Note: See the paper "Explicit Allocation of Best effor Delivery Service" (David
  Clark and Wenjia Fang), Section 3.3, for a description of the TSW Tagger.
-----------------------------------------------------------------------------*/
void TSW2CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  double now, bytesInTSW, newBytes;
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  
  bytesInTSW = policy->avgRate * policy->winLen;
  newBytes = bytesInTSW + (double) hdr->size();
  now = Scheduler::instance().clock();
  policy->avgRate = newBytes / (now - policy->arrivalTime + policy->winLen);
  policy->arrivalTime = now;
}

/*-----------------------------------------------------------------------------
int TSW2CMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt)
Pre: policy points to a policytableEntry that is using the TSW2CM policer and
  whose state variables (avgRate and cir) are up to date.
Post: If policy's avgRate exceeds its CIR, this method returns an out-of-profile
  code point with a probability of ((rate - CIR) / rate).  If it does not
  downgrade the code point, this method simply returns the initial code point.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne().
-----------------------------------------------------------------------------*/
int TSW2CMPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *) {
  if ((policy->avgRate > policy->cir)
      && (Random::uniform(0.0, 1.0) <= (1-(policy->cir/policy->avgRate)))) {
    return(policer->downgrade1);
  }
  else {
    return(policer->initialCodePt);
  }
}

// The end of TSW2CM

// The Beginning of TSW3CM
/*-----------------------------------------------------------------------------
void TSW3CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables avgRate, arrivalTime, and winLen hold valid values; and
  pkt points to a newly-arrived packet.
Post: Adjusts policy's TSW state variables avgRate and arrivalTime (also called
  tFront) according to the specified packet.
Note: See the paper "Explicit Allocation of Best effor Delivery Service" (David
  Clark and Wenjia Fang), Section 3.3, for a description of the TSW Tagger.
-----------------------------------------------------------------------------*/
void TSW3CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  double now, bytesInTSW, newBytes;
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  
  bytesInTSW = policy->avgRate * policy->winLen;
  newBytes = bytesInTSW + (double) hdr->size();
  now = Scheduler::instance().clock();
  policy->avgRate = newBytes / (now - policy->arrivalTime + policy->winLen);
  policy->arrivalTime = now;
}

/*-----------------------------------------------------------------------------
int TSW3CMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt)
Pre: policy points to a policytableEntry that is using the TSW3CM policer and
  whose state variables (avgRate, cir, and pir) are up to date.
Post: Sets code points with the following probabilities when rate > PIR:

          red:    (rate - PIR) / rate
          yellow: (PIR - CIR) / rate
          green:  CIR / rate

and with the following code points when CIR < rate <= PIR:

          red:    0
          yellow: (rate - CIR) / rate
          green:  CIR / rate

    When rate is under CIR, a packet is always marked green.
Returns: A code point to apply to the current packet.
Uses: Methods downgradeOne() and downgradeTwo().
-----------------------------------------------------------------------------*/
int TSW3CMPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *) {
  double rand = policy->avgRate * (1.0 - Random::uniform(0.0, 1.0));
  
  if (rand > policy->pir)
    return (policer->downgrade2);
  else if (rand > policy->cir)
    return(policer->downgrade1);
  else
    return(policer->initialCodePt);
}
 
// End of TSW3CM

// Begin of Token Bucket.
/*-----------------------------------------------------------------------------
void TBPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables cBucket, cir, cbs, and arrivalTime hold valid values.
Post: Increments policy's Token Bucket state variable cBucket according to the
  elapsed time since the last packet arrival.  cBucket is filled at a rate equal  to CIR, capped at an upper bound of CBS.
  This method also sets arrivalTime equal to the current simulator time.
-----------------------------------------------------------------------------*/
void TBPolicy::applyMeter(policyTableEntry *policy, Packet *) {
  double now = Scheduler::instance().clock();
  double tokenBytes;

  tokenBytes = (double) policy->cir * (now - policy->arrivalTime);
  if (policy->cBucket + tokenBytes <= policy->cbs)
   policy->cBucket += tokenBytes;
  else
   policy->cBucket = policy->cbs;
  policy->arrivalTime = now;
}

/*----------------------------------------------------------------------------
int TBPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt,
        Packet* pkt)
Pre: policy points to a policytableEntry that is using the Token Bucket policer
and whose state variable (cBucket) is up to date.  pkt points to a
newly-arrived packet.
Post: If policy's cBucket is at least as large as pkt's size, cBucket is
decremented by that size and the initial code point is retained.  Otherwise,
the code point is downgraded.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne().
-----------------------------------------------------------------------------*/
int TBPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet* pkt) {
  hdr_cmn* hdr = hdr_cmn::access(pkt);

  double size = (double) hdr->size();

  if ((policy->cBucket - size) >= 0) {
    policy->cBucket -= size;
    return(policer->initialCodePt);
  } else{
    return(policer->downgrade1);
  }
}

// End of Tocken Bucket.

// Begining of SRTCM
/*-----------------------------------------------------------------------------
void SRTCMPolicy::applyMeter(policyTableEntry *policy)
Pre: policy's variables cBucket, eBucket, cir, cbs, ebs, and arrivalTime hold
  valid values.
Post: Increments policy's srTCM state variables cBucket and eBucket according
  to the elapsed time since the last packet arrival.  cBucket is filled at a
  rate equal to CIR, capped at an upper bound of CBS.  When cBucket is full
  (equal to CBS), eBucket is filled at a rate equal to CIR, capped at an upper
  bound of EBS.
      This method also sets arrivalTime equal to the current
  simulator time.
Note: See the Internet Draft, "A Single Rate Three Color Marker" (Heinanen et
  al; May, 1999) for a description of the srTCM.
-----------------------------------------------------------------------------*/
void SRTCMPolicy::applyMeter(policyTableEntry *policy, Packet *) {
  double now = Scheduler::instance().clock();
  double tokenBytes;
  
  tokenBytes = (double) policy->cir * (now - policy->arrivalTime);
  if (policy->cBucket + tokenBytes <= policy->cbs)
    policy->cBucket += tokenBytes;
  else {
    tokenBytes = tokenBytes - (policy->cbs - policy->cBucket);
    
    policy->cBucket = policy->cbs;
    if (policy->eBucket + tokenBytes <= policy->ebs)
      policy->eBucket += tokenBytes;
    else
      policy->eBucket = policy->ebs;
  }
  policy->arrivalTime = now;
}

/*-----------------------------------------------------------------------------
int SRTCMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt)
Pre: policy points to a policyTableEntry that is using the srTCM policer and
  whose state variables (cBucket and eBucket) are up to date.  pkt points to a
  newly-arrived packet.
Post: If policy's cBucket is at least as large as pkt's size, cBucket is
  decremented by that size and the initial code point is retained.  Otherwise,
  if eBucket is at least as large as the packet, eBucket is decremented and the
  yellow code point is returned.  Otherwise, the red code point is returned.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne() and downgradeTwo().
Note: See the Internet Draft, "A Single Rate Three Color Marker" (Heinanen et
  al; May, 1999) for a description of the srTCM.
-----------------------------------------------------------------------------*/
int SRTCMPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet* pkt) {

  hdr_cmn* hdr = hdr_cmn::access(pkt);
  double size = (double) hdr->size();
  
  if ((policy->cBucket - size) >= 0) {
    policy->cBucket -= size;
    return(policer->initialCodePt);
  } else {
    if ((policy->eBucket - size) >= 0) {
      policy->eBucket -= size;
      return(policer->downgrade1);
    } else
      return(policer->downgrade2);
  }
}

// End of SRTCM

// Beginning of TRTCM
/*----------------------------------------------------------------------------
void TRTCMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables cBucket, pBucket, cir, pir, cbs, pbs, and arrivalTime
  hold valid values.
Post: Increments policy's trTCM state variables cBucket and pBucket according
  to the elapsed time since the last packet arrival.  cBucket is filled at a
  rate equal to CIR, capped at an upper bound of CBS.  pBucket is filled at a
  rate equal to PIR, capped at an upper bound of PBS.
      This method also sets arrivalTime equal to the current simulator time.
Note: See the Internet Draft, "A Two Rate Three Color Marker" (Heinanen et al;
  May, 1999) for a description of the srTCM.
---------------------------------------------------------------------------*/
void TRTCMPolicy::applyMeter(policyTableEntry *policy, Packet *) {
  double now = Scheduler::instance().clock();
  double tokenBytes;
  tokenBytes = (double) policy->cir * (now - policy->arrivalTime);
  if (policy->cBucket + tokenBytes <= policy->cbs)
    policy->cBucket += tokenBytes;
  else
    policy->cBucket = policy->cbs;
  
  tokenBytes = (double) policy->pir * (now - policy->arrivalTime);
  if (policy->pBucket + tokenBytes <= policy->pbs)
    policy->pBucket += tokenBytes;
  else
    policy->pBucket = policy->pbs;
  
  policy->arrivalTime = now;
}

/*----------------------------------------------------------------------------
int TRTCMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt)
Pre: policy points to a policyTableEntry that is using the trTCM policer and
  whose state variables (cBucket and pBucket) are up to date.  pkt points to a
  newly-arrived packet.
Post: If policy's pBucket is smaller than pkt's size, the red code point is
  retained.  Otherwise, if cBucket is smaller than the packet size, the yellow
  code point is returned and pBucket is decremented.  Otherwise, the packet
  remains green and both buckets are decremented.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne() and downgradeTwo().
Note: See the Internet Draft, "A Two Rate Three Color Marker" (Heinanen et al;
  May, 1999) for a description of the srTCM.
-----------------------------------------------------------------------------*/
int TRTCMPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet* pkt) {
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  double size = (double) hdr->size();
  
  if ((policy->pBucket - size) < 0)
    return(policer->downgrade2);
  else {
    if ((policy->cBucket - size) < 0) {
      policy->pBucket -= size;
      return(policer->downgrade1);
    } else {
      policy->cBucket -= size;
      policy->pBucket -= size;
      return(policer->initialCodePt);
    }
  }
}
// End of TRTCM

// Beginning of SFD
//Constructor.
SFDPolicy::SFDPolicy() : Policy() {
  flow_table.head = NULL;
  flow_table.tail = NULL;
}

//Deconstructor.
SFDPolicy::~SFDPolicy(){
  struct flow_entry *p, *q;
  p = q = flow_table.head;
  while (p) {
    printf("free flow: %d\n", p->fid);
    q = p;
    p = p->next;
    free(q);
  }

  p = q = NULL;
  flow_table.head = flow_table.tail = NULL;
}

/*-----------------------------------------------------------------------------
 void SFDPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
 Flow states are kept in a linked list.
 Record how many bytes has been sent per flow and check if there is any flow
 timeout.
-----------------------------------------------------------------------------*/
void SFDPolicy::applyMeter(policyTableEntry *, Packet *pkt) {
  int fid, src_id, dst_id;
  struct flow_entry *p, *q, *new_entry;

  double now = Scheduler::instance().clock();
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  hdr_ip* iph = hdr_ip::access(pkt);
  fid = iph->flowid();
  dst_id = iph->daddr();
  src_id = iph->saddr();

  //  printf("enter applyMeter\n");
  //  printFlowTable();

  p = q = flow_table.head;
  while (p) {
    // Check if the flow has been recorded before.
    if (p->fid == fid) {
    //if (p->src_id == src_id && p->dst_id == dst_id ) {
      p->last_update = now;
      p->bytes_sent += hdr->size();
      return;
    } else if (p->last_update + FLOW_TIME_OUT < now){
      // The coresponding flow is expired.      
      if (p == flow_table.head){
	if (p == flow_table.tail) {
	  flow_table.head = flow_table.tail = NULL;
	  free(p);
	  p = q = NULL;
	} else {
	  flow_table.head = p->next;
	  free(p);
	  p = q = flow_table.head;
	}
      } else {
	q->next = p->next;
	if (p == flow_table.tail)
	  flow_table.tail = q;
	free(p);
	p = q->next;
      }
    } else {
      q = p;
      p = q->next;
    }
  }
  
  // This is the firt time the flow shown up
  if (!p) {
    new_entry = new flow_entry;
    new_entry->fid = fid;
    new_entry->src_id = src_id;
    new_entry->dst_id = dst_id;
    new_entry->last_update = now;
    new_entry->bytes_sent = hdr->size();
    new_entry->count = 0;
    new_entry->next = NULL;
    
    // always insert the new entry to the tail.
    if (flow_table.tail)
      flow_table.tail->next = new_entry;
    else
      flow_table.head = new_entry;
    flow_table.tail = new_entry;
  }
  
  //  printf("leave applyMeter\n");
  return;
}

/*-----------------------------------------------------------------------------
void SFDPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) 
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
int SFDPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt) {
  int fid, src_id, dst_id;
  struct flow_entry *p;

  hdr_ip* iph = hdr_ip::access(pkt);
  fid = iph->flowid();
  dst_id = iph->daddr();
  src_id = iph->saddr();

  //  printf("enter applyPolicer\n");
  //printFlowTable();
  
  p = flow_table.head;
  while (p) {
    // Check if the flow has been recorded before.
    if (p->fid == iph->flowid()) {
      //if (p->src_id == src_id && p->dst_id == dst_id) {
      if (p->bytes_sent > policy->cir) {
	// Use downgrade2 code to judge how to penalize out-profile packets.
	if (policer->downgrade2 == 0) {
	  // Penalize every packet beyond th.
	  //printf("leave applyPolicer  %d, every downgrade\n", p->fid);
	  return(policer->downgrade1);
	} else if (policer->downgrade2 == 1) {
	  // Randomized penalization.
	  if (Random::uniform(0.0, 1.0) > (1 - (policy->cir/p->bytes_sent))) {
	    //printf("leave applyPolicer %d, random initial.\n", p->fid);
	    return(policer->initialCodePt);
	  } else {
	    //printf("leave applyPolicer %d, random, downgrade\n", p->fid);
	    return(policer->downgrade1);
	  }
	} else {
	  // Simple scheduling on penalization.
	  if (p->count == 5) {
	    // Penalize 4 out of every 5 packets.
	    p->count = 0;
	    //printf("leave applyPolicer %d, initial, %d\n", p->fid, p->count);
	    return(policer->initialCodePt);
	  } else {
	    p->count++;
	    //printf("leave applyPolicer %d, downgrade, %d\n", p->fid, p->count);
	    return(policer->downgrade1);
	  }
	}
      } else {
	//	printf("leave applyPolicer, initial\n");
	return(policer->initialCodePt);
      }
    }
    p = p->next;
  }
  
  // Can't find the record for this flow.
  if (!p) {
    printf ("MISS: no flow %d (%d, %d) in the table\n", fid, src_id, dst_id);
    printFlowTable();
};
  
  //  printf("leave applyPolicer, init but problem...\n");
  return(policer->initialCodePt);
}

//    Prints the flowTable, one entry per line.
void SFDPolicy::printFlowTable() {
  struct flow_entry *p;
  printf("Flow table:\n");

  p = flow_table.head;
  while (p) {
    printf("flow id: %d [%d %d], bytesSent: %d, last_update: %f\n", 
	   p->fid, p->src_id, p->dst_id, p->bytes_sent, p->last_update);
    p = p-> next;
  }
  p = NULL;
  printf("\n");
}
// End of SFD
