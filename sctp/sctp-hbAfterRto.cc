/*
 * Copyright (c) 2006-2009 by the Protocol Engineering Lab, U of Delaware
 * All rights reserved.
 *
 * Protocol Engineering Lab web page : http://pel.cis.udel.edu/
 *
 * Paul D. Amer        <amer@@cis,udel,edu>
 * Armando L. Caro Jr. <acaro@@cis,udel,edu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* HbAfterRto extension sends a heartbeat immediately after timeout
 * retransmission. The idea is to give the destinations a chance to get an
 * RTT measurement after their RTO is backed off. The hope is to avoid
 * unnecessarily large RTOs (especially on the alternate destinations).
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp-hbAfterRto.cc,v 1.5 2009/11/16 05:51:27 tom_henderson Exp $ (UD/PEL)";
#endif

#include "ip.h"
#include "sctp-hbAfterRto.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "sctpDebug.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))

static class HbAfterRtoSctpClass : public TclClass 
{ 
public:
  HbAfterRtoSctpClass() : TclClass("Agent/SCTP/HbAfterRto") {}
  TclObject* create(int, const char*const*) 
  {
    return (new HbAfterRtoSctpAgent());
  }
} classSctpHbAfterRto;

HbAfterRtoSctpAgent::HbAfterRtoSctpAgent() : SctpAgent()
{
}

void HbAfterRtoSctpAgent::delay_bind_init_all()
{
  SctpAgent::delay_bind_init_all();
}

int HbAfterRtoSctpAgent::delay_bind_dispatch(const char *cpVarName, 
					  const char *cpLocalName, 
					  TclObject *opTracer)
{
  return SctpAgent::delay_bind_dispatch(cpVarName, cpLocalName, opTracer);
}

void HbAfterRtoSctpAgent::Timeout(SctpChunkType_E eChunkType, 
				  SctpDest_S *spDest)
{
  DBG_I(Timeout);
  DBG_PL(Timeout, "eChunkType=%s spDest=%p"), 
    (eChunkType == SCTP_CHUNK_DATA) ? "DATA" : "HEARTBEAT",
    spDest
    DBG_PR;

  double dCurrTime = Scheduler::instance().clock();

  DBG_PL(Timeout, "dCurrTime=%f"), dCurrTime DBG_PR;

  if(eChunkType == SCTP_CHUNK_DATA)
    {
      spDest->eRtxTimerIsRunning = FALSE;
      
      /* section 7.2.3 of rfc2960 (w/ implementor's guide)
       */
      if(spDest->iCwnd > 1 * (int) uiMaxDataSize)
	{
	  spDest->iSsthresh = MAX(spDest->iCwnd/2, 
				  iInitialCwnd * (int) uiMaxDataSize);
	  spDest->iCwnd = 1*uiMaxDataSize;
	  spDest->iPartialBytesAcked = 0; // reset
	  tiCwnd++; // trigger changes for trace to pick up
	}

      spDest->opCwndDegradeTimer->force_cancel();

      /* Cancel any pending RTT measurement on this destination. Stephan
       * Baucke suggested (2004-04-27) this action as a fix for the
       * following simple scenario:
       *
       * - Host A sends packets 1, 2 and 3 to host B, and choses 3 for
       *   an RTT measurement
       *
       * - Host B receives all packets correctly and sends ACK1, ACK2,
       *   and ACK3.
       *
       * - ACK2 and ACK3 are lost on the return path
       *
       * - Eventually a timeout fires for packet 2, and A retransmits 2
       *
       * - Upon receipt of 2, B sends a cumulative ACK3 (since it has
       *   received 2 & 3 before)
       *
       * - Since packet 3 has never been retransmitted, the SCTP code
       *   actually accepts the ACK for an RTT measurement, although it
       *   was sent in reply to the retransmission of 2, which results
       *   in a much too high RTT estimate. Since this case tends to
       *   happen in case of longer link interruptions, the error is
       *   often amplified by subsequent timer backoffs.
       */
      spDest->eRtoPending = FALSE; // cancel any pending RTT measurement
    }

  DBG_PL(Timeout, "was spDest->dRto=%f"), spDest->dRto DBG_PR;
  spDest->dRto *= 2;    // back off the timer
  if(spDest->dRto > dMaxRto)
    spDest->dRto = dMaxRto;
  tdRto++;              // trigger changes for trace to pick up
  DBG_PL(Timeout, "now spDest->dRto=%f"), spDest->dRto DBG_PR;

  spDest->iTimeoutCount++;
  spDest->iErrorCount++; // @@@ window probe timeouts should not be counted
  DBG_PL(Timeout, "now spDest->iErrorCount=%d"), spDest->iErrorCount DBG_PR;

  if(spDest->eStatus == SCTP_DEST_STATUS_ACTIVE)
    {  
      iAssocErrorCount++;
      DBG_PL(Timeout, "now iAssocErrorCount=%d"), iAssocErrorCount DBG_PR;

      // Path.Max.Retrans exceeded?
      if(spDest->iErrorCount > (int) uiPathMaxRetrans) 
	{
	  spDest->eStatus = SCTP_DEST_STATUS_INACTIVE;
	  if(spDest == spNewTxDest)
	    {
	      spNewTxDest = GetNextDest(spDest);
	      DBG_PL(Timeout, "failing over from %p to %p"),
		spDest, spNewTxDest DBG_PR;
	    }
	}
      if(iAssocErrorCount > (int) uiAssociationMaxRetrans)
	{
	  /* abruptly close the association!  (section 8.1)
	   */
	  DBG_PL(Timeout, "abruptly closing the association!") DBG_PR;
	  Close();
	  DBG_X(Timeout);
	  return;
	}
    }

  // trace it!
  tiTimeoutCount++;
  tiErrorCount++;       

  if(spDest->iErrorCount > (int) uiChangePrimaryThresh &&
     spDest == spPrimaryDest)
    {
      spPrimaryDest = spNewTxDest;
      DBG_PL(Timeout, "changing primary from %p to %p"),
	spDest, spNewTxDest DBG_PR;
    }

  if(eChunkType == SCTP_CHUNK_DATA)
    {
      TimeoutRtx(spDest);

      // BEGIN -- HbAfterRto changes to this function

      /* If there is an active alternate destination, then send a HB
       * immediately to the destination which timed out.
       */
      if(GetNextDest(spDest) != spDest)
	SendHeartbeat(spDest);  

      // END -- HbAfterRto changes to this function
    }
  else if(eChunkType == SCTP_CHUNK_HB)
    {
      if(uiHeartbeatInterval != 0)
	SendHeartbeat(spDest);
    }

  DBG_X(Timeout);  
}
