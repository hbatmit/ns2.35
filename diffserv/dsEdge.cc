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
 * Integrated into ns by Xuan Chen (xuanc@isi.edu) 
 * Add instance of policyClassifier.
 */

#include "dsEdge.h"
#include "ew.h"
#include "dewp.h"
#include "packet.h"
#include "tcp.h"
#include "random.h"


/*------------------------------------------------------------------------------
class edgeClass 
------------------------------------------------------------------------------*/
static class edgeClass : public TclClass {
public:
	edgeClass() : TclClass("Queue/dsRED/edge") {}
	TclObject* create(int, const char*const*) {
		return (new edgeQueue);
	}
} class_edge;


/*------------------------------------------------------------------------------
edgeQueue() Constructor.
------------------------------------------------------------------------------*/
edgeQueue::edgeQueue() {
  //  policy = NULL;
}


/*------------------------------------------------------------------------------
void enque(Packet* pkt) 
Post: The incoming packet pointed to by pkt is marked with an appropriate code
  point (as handled by the marking method) and enqueued in the physical and 
  virtual queue corresponding to that code point (as specified in the PHB 
  Table).
Uses: Methods Policy::mark(), lookupPHBTable(), and redQueue::enque(). 
------------------------------------------------------------------------------*/
void edgeQueue::enque(Packet* pkt) {

	// Mark the packet with the specified priority:
	//printf("before ,mark\n");
	policy.mark(pkt);
	//	printf("after ,mark\n");
	dsREDQueue::enque(pkt);
}


/*------------------------------------------------------------------------------
int command(int argc, const char*const* argv) 
    Commands from the ns file are interpreted through this interface.
------------------------------------------------------------------------------*/
int edgeQueue::command(int argc, const char*const* argv) {
  if (strcmp(argv[1], "addPolicyEntry") == 0) {
    // Note: the definition of policy has changed.
    policy.addPolicyEntry(argc, argv);
    return(TCL_OK);
  };

  if (strcmp(argv[1], "addPolicerEntry") == 0) {
    // Note: the definition of policy has changed.
    policy.addPolicerEntry(argc, argv);
    return(TCL_OK);
  };

  if (strcmp(argv[1], "couple") == 0) {
  /*
    printf("%d ", argc);
    for (int i = 1; i < argc; i++) 
      printf("%d(%s) ", i, argv[i]);
    printf("\n");   
  */

    DEWPPolicy *ewp = (DEWPPolicy *)(policy.policy_pool[DEWP]);

    // Get the pointer to the queue to be coupled (in c++)
    //Tcl& tcl = Tcl::instance();
    edgeQueue *cq = (edgeQueue*) TclObject::lookup(argv[2]);
    DEWPPolicy *ewpc = (DEWPPolicy *)((cq->policy).policy_pool[DEWP]);

    ewp->couple(ewpc); 

    return(TCL_OK);
  };

  // couple the EW on request and response links
  if (strcmp(argv[1], "coupleEW") == 0) {
    //printf("%d ", argc);
    //for (int i = 1; i < argc; i++) 
    //printf("%d(%s) ", i, argv[i]);
    //printf("\n");   

    EWPolicy *ewp = (EWPolicy *)(policy.policy_pool[EW]);

    // Get the pointer to the queue to be coupled (in c++)
    //Tcl& tcl = Tcl::instance();
    edgeQueue *cq = (edgeQueue*) TclObject::lookup(argv[2]);
    EWPolicy *ewpc = (EWPolicy *)((cq->policy).policy_pool[EW]);

    // couple the EW detector 
    if (argc > 3)
      ewp->coupleEW(ewpc, atof(argv[3])); 
    else
      ewp->coupleEW(ewpc); 

    return(TCL_OK);
  };

  // Set a rate limitor
  if (strcmp(argv[1], "limit") == 0) {
    //printf("%d ", argc);
    //for (int i = 1; i < argc; i++) 
    //  printf("%d(%s) ", i, argv[i]);
    //printf("\n");

    EWPolicy *ewp = (EWPolicy *)(policy.policy_pool[EW]);
    
    // Packet rate limitor
    if (strcmp(argv[2], "P") == 0) {
      ewp->limitPr(atoi(argv[3]));
      return(TCL_OK);
    } 

    // bits rate limitor
    if (strcmp(argv[2], "B") == 0) {
      ewp->limitBr(atoi(argv[3]));
      return(TCL_OK);
    }
  };

  // Setup an EW detector on a link
  if (strcmp(argv[1], "detect") == 0) {
    //printf("%d ", argc);
    //for (int i = 1; i < argc; i++) 
    //  printf("%d(%s) ", i, argv[i]);
    //printf("\n");

    EWPolicy *ewp = (EWPolicy *)(policy.policy_pool[EW]);
    
    if (strcmp(argv[2], "P") == 0) {
      if (argc > 4)
	ewp->detectPr(atoi(argv[3]), atoi(argv[4]));
      else if (argc > 3)
	ewp->detectPr(atoi(argv[3]));
      else
	ewp->detectPr();

      return(TCL_OK);
    } 

    if (strcmp(argv[2], "B") == 0) {
      if (argc > 4)
	ewp->detectBr(atoi(argv[3]), atoi(argv[4]));
      else if (argc > 3)
	ewp->detectBr(atoi(argv[3]));
      else
	ewp->detectBr();

      return(TCL_OK);
    }
  };

  if (strcmp(argv[1], "getCBucket") == 0) {
    Tcl& tcl = Tcl::instance();
    tcl.resultf("%f", policy.getCBucket(argv));
    return(TCL_OK);
  }
  if (strcmp(argv[1], "printPolicyTable") == 0) {
    policy.printPolicyTable();
    return(TCL_OK);
  }
  if (strcmp(argv[1], "printPolicerTable") == 0) {
    policy.printPolicerTable();
    return(TCL_OK);
  }
  
  return(dsREDQueue::command(argc, argv));
}
