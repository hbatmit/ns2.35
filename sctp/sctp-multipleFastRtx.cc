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

/* MultipleFastRtx extension implements the Caro Multiple Fast Retransmit
 * Algorithm. Caro's Algorithm introduces a fastRtxRecover state variable
 * per TSN in the send buffer. Any time a TSN is retransmitted, its
 * fastRtxRecover is set to the highest TSN outstanding at the time of
 * retransmit. That way, only missing reports triggered by TSNs beyond
 * fastRtxRecover may trigger yet another fast retransmit.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp-multipleFastRtx.cc,v 1.6 2011/10/02 22:32:34 tom_henderson Exp $ (UD/PEL)";
#endif

#include "ip.h"
#include "sctp-multipleFastRtx.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "sctpDebug.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))

static class MultipleFastRtxSctpClass : public TclClass 
{ 
public:
  MultipleFastRtxSctpClass() : TclClass("Agent/SCTP/MultipleFastRtx") {}
  TclObject* create(int, const char*const*) 
  {
    return (new MultipleFastRtxSctpAgent());
  }
} classSctpMultipleFastRtx;

MultipleFastRtxSctpAgent::MultipleFastRtxSctpAgent() : SctpAgent()
{
}

void MultipleFastRtxSctpAgent::delay_bind_init_all()
{
  delay_bind_init_one("mfrCount_");
  SctpAgent::delay_bind_init_all();
}

int MultipleFastRtxSctpAgent::delay_bind_dispatch(const char *cpVarName, 
					  const char *cpLocalName, 
					  TclObject *opTracer)
{
  if(delay_bind(cpVarName, cpLocalName, "mfrCount_", &tiMfrCount, opTracer))
    return TCL_OK;

  return SctpAgent::delay_bind_dispatch(cpVarName, cpLocalName, opTracer);
}

void MultipleFastRtxSctpAgent::TraceAll()
{
  char cpOutString[500];
  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;
  double dCurrTime = Scheduler::instance().clock();

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrDest = (SctpDest_S *) spCurrNode->vpData;
      SetSource(spCurrDest); // gives us the correct source addr & port
      sprintf(cpOutString,
	      "time: %-8.5f  "
	      "saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
	      "cwnd: %d pba: %d out: %d ssthresh: %d peerRwnd: %d "
	      "rto: %-6.3f srtt: %-6.3f rttvar: %-6.3f "
	      "assocErrors: %d pathErrors: %d dstatus: %s isPrimary: %s "
	      "frCount: %d mfrCount: %d timeoutCount: %d rcdCount: %d"
	      " availSwnd: %d\n",
	      dCurrTime,
	      addr(), port(), spCurrDest->iNsAddr, spCurrDest->iNsPort,
	      spCurrDest->iCwnd, spCurrDest->iPartialBytesAcked, 
	      spCurrDest->iOutstandingBytes, spCurrDest->iSsthresh, 
	      uiPeerRwnd,
	      spCurrDest->dRto, spCurrDest->dSrtt, 
	      spCurrDest->dRttVar,
	      iAssocErrorCount,
	      spCurrDest->iErrorCount,
	      spCurrDest->eStatus ? "ACTIVE" : "INACTIVE",
	      (spCurrDest == spPrimaryDest) ? "TRUE" : "FALSE",
	      int(tiFrCount),
	      // BEGIN -- MultipleFastRtx changes to this function  
	      int(tiMfrCount),
              // END -- MultipleFastRtx changes to this function  
	      spCurrDest->iTimeoutCount,
	      spCurrDest->iRcdCount,
	      uiAvailSwnd);
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }

  sprintf(cpOutString, "\n");
  if(channel_)
    (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
}

void MultipleFastRtxSctpAgent::TraceVar(const char* cpVar)
{
  char cpOutString[500];
  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;
  double dCurrTime = Scheduler::instance().clock();

  if(!strcmp(cpVar, "cwnd_"))
    for(spCurrNode = sDestList.spHead;
	spCurrNode != NULL;
	spCurrNode = spCurrNode->spNext)
      {
	spCurrDest = (SctpDest_S *) spCurrNode->vpData;
	SetSource(spCurrDest); // gives us the correct source addr & port
	sprintf(cpOutString,
		"time: %-8.5f  "
		"saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
		"cwnd: %d pba: %d out: %d ssthresh: %d peerRwnd: %d\n",
		dCurrTime, 
		addr(), port(), 
		spCurrDest->iNsAddr, spCurrDest->iNsPort,
		spCurrDest->iCwnd, spCurrDest->iPartialBytesAcked, 
		spCurrDest->iOutstandingBytes, spCurrDest->iSsthresh, 
		uiPeerRwnd);
	if(channel_)
	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
      }

  else if(!strcmp(cpVar, "rto_"))
    for(spCurrNode = sDestList.spHead;
	spCurrNode != NULL;
	spCurrNode = spCurrNode->spNext)
      {
	spCurrDest = (SctpDest_S *) spCurrNode->vpData;
	SetSource(spCurrDest); // gives us the correct source addr & port
	sprintf(cpOutString,
		"time: %-8.5f  "
		"saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
		"rto: %-6.3f srtt: %-6.3f rttvar: %-6.3f\n",
		dCurrTime, 
		addr(), port(), 
		spCurrDest->iNsAddr, spCurrDest->iNsPort,
		spCurrDest->dRto, spCurrDest->dSrtt, 
		spCurrDest->dRttVar);
	if(channel_)
	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
      }

  else if(!strcmp(cpVar, "errorCount_"))
    for(spCurrNode = sDestList.spHead;
	spCurrNode != NULL;
	spCurrNode = spCurrNode->spNext)
      {
	spCurrDest = (SctpDest_S *) spCurrNode->vpData;
	SetSource(spCurrDest); // gives us the correct source addr & port
	sprintf(cpOutString, 
		"time: %-8.5f  "
		"saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
		"assocErrors: %d pathErrors: %d dstatus: %s isPrimary: %s\n",
		dCurrTime, 
		addr(), port(), 
		spCurrDest->iNsAddr, spCurrDest->iNsPort,
		iAssocErrorCount,
		spCurrDest->iErrorCount,
		spCurrDest->eStatus ? "ACTIVE" : "INACTIVE",
		(spCurrDest == spPrimaryDest) ? "TRUE" : "FALSE");
	if(channel_)
	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
      }

  else if(!strcmp(cpVar, "frCount_"))
    {
      sprintf(cpOutString, 
	      "time: %-8.5f  "
	      "frCount: %d\n",
	      dCurrTime, 
	      int(*((TracedInt*) cpVar)) );
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }

  // BEGIN -- MultipleFastRtx changes to this function  
  else if(!strcmp(cpVar, "mfrCount_"))
    {
      sprintf(cpOutString, 
	      "time: %-8.5f  "
	      "mfrCount: %d\n",
	      dCurrTime, 
	      int(*((TracedInt*) cpVar)) );
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }
  // END -- MultipleFastRtx changes to this function  

  else if(!strcmp(cpVar, "timeoutCount_"))
    {
    for(spCurrNode = sDestList.spHead;
	spCurrNode != NULL;
	spCurrNode = spCurrNode->spNext)
      {
	spCurrDest = (SctpDest_S *) spCurrNode->vpData;
	SetSource(spCurrDest); // gives us the correct source addr & port
	sprintf(cpOutString, 
		"time: %-8.5f  "
		"saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
		"timeoutCount: %d\n",
		dCurrTime, 
		addr(), port(), 
		spCurrDest->iNsAddr, spCurrDest->iNsPort,
		spCurrDest->iTimeoutCount);
	if(channel_)
	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
      }
    }

  else if(!strcmp(cpVar, "rcdCount_"))
    {
    for(spCurrNode = sDestList.spHead;
	spCurrNode != NULL;
	spCurrNode = spCurrNode->spNext)
      {
	spCurrDest = (SctpDest_S *) spCurrNode->vpData;
	SetSource(spCurrDest); // gives us the correct source addr & port
	sprintf(cpOutString, 
		"time: %-8.5f  "
		"saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
		"rcdCount: %d\n",
		dCurrTime, 
		addr(), port(), 
		spCurrDest->iNsAddr, spCurrDest->iNsPort,
		spCurrDest->iRcdCount);
	if(channel_)
	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
      }
    }

  else
    {
      sprintf(cpOutString,
	      "time: %-8.5f  "
	      "saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d %s: %s\n",
	      dCurrTime, addr(), port(), daddr(), dport(),
	      cpVar, "ERROR (unepected trace variable)"); 
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }

  sprintf(cpOutString, "\n");
  if(channel_)
    (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
}


void MultipleFastRtxSctpAgent::AddToSendBuffer(SctpDataChunkHdr_S *spChunk, 
				int iChunkSize,
				u_int uiReliability,
				SctpDest_S *spDest)
{
  DBG_I(AddToSendBuffer);
  DBG_PL(AddToSendBuffer, "spDest=%p  iChunkSize=%d"), 
    spDest, iChunkSize DBG_PR;

  Node_S *spNewNode = new Node_S;
  spNewNode->eType = NODE_TYPE_SEND_BUFFER;
  spNewNode->vpData = new SctpSendBufferNode_S;

  SctpSendBufferNode_S * spNewNodeData 
    = (SctpSendBufferNode_S *) spNewNode->vpData;

  /* This can NOT simply be a 'new SctpDataChunkHdr_S', because we need to
   * allocate the space for the ENTIRE data chunk and not just the data
   * chunk header.  
   */
  spNewNodeData->spChunk = (SctpDataChunkHdr_S *) new u_char[iChunkSize];
  memcpy(spNewNodeData->spChunk, spChunk, iChunkSize);

  spNewNodeData->eAdvancedAcked = FALSE;
  spNewNodeData->eGapAcked = FALSE;
  spNewNodeData->eAddedToPartialBytesAcked = FALSE;
  spNewNodeData->iNumMissingReports = 0;
  spNewNodeData->iUnrelRtxLimit = uiReliability;
  spNewNodeData->eMarkedForRtx = NO_RTX;
  spNewNodeData->eIneligibleForFastRtx = FALSE;
  spNewNodeData->iNumTxs = 1;
  spNewNodeData->spDest = spDest;

  // BEGIN -- MultipleFastRtx changes to this function  
  spNewNodeData->uiFastRtxRecover = 0;
  // END -- MultipleFastRtx changes to this function  

  /* Is there already a DATA chunk in flight measuring an RTT? 
   * (6.3.1.C4 RTT measured once per round trip)
   */
  if(spDest->eRtoPending == FALSE)  // NO?
    {
      spNewNodeData->dTxTimestamp = Scheduler::instance().clock();
      spDest->eRtoPending = TRUE;   // ...well now there is :-)
    }
  else
    spNewNodeData->dTxTimestamp = 0; // don't use this check for RTT estimate

  InsertNode(&sSendBuffer, sSendBuffer.spTail, spNewNode, NULL);

  DBG_X(AddToSendBuffer);
}

/* Go thru the send buffer deleting all chunks which have a tsn <= the 
 * tsn parameter passed in. We assume the chunks in the rtx list are ordered by 
 * their tsn value. In addtion, for each chunk deleted:
 *   1. we add the chunk length to # newly acked bytes and partial bytes acked
 *   2. we update round trip time if appropriate
 *   3. stop the timer if the chunk's destination timer is running
 */
void MultipleFastRtxSctpAgent::SendBufferDequeueUpTo(u_int uiTsn)
{
  DBG_I(SendBufferDequeueUpTo);

  Node_S *spDeleteNode = NULL;
  Node_S *spCurrNode = sSendBuffer.spHead;
  SctpSendBufferNode_S *spCurrNodeData = NULL;

  /* PN: 5/2007. Simulate send window */
  u_short usChunkSize = 0;

  iAssocErrorCount = 0;

  while(spCurrNode != NULL &&
	((SctpSendBufferNode_S*)spCurrNode->vpData)->spChunk->uiTsn <= uiTsn)
    {
      spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

      /* Only count this chunk as newly acked and towards partial bytes
       * acked if it hasn't been gap acked or marked as ack'd due to rtx
       * limit.  
       */
      if((spCurrNodeData->eGapAcked == FALSE) &&
	 (spCurrNodeData->eAdvancedAcked == FALSE) )
	{
	  uiHighestTsnNewlyAcked = spCurrNodeData->spChunk->uiTsn;

	  spCurrNodeData->spDest->iNumNewlyAckedBytes 
	    += spCurrNodeData->spChunk->sHdr.usLength;

	  /* only add to partial bytes acked if we are in congestion
	   * avoidance mode and if there was cwnd amount of data
	   * outstanding on the destination (implementor's guide) 
	   */
	  if(spCurrNodeData->spDest->iCwnd >spCurrNodeData->spDest->iSsthresh &&
	     ( spCurrNodeData->spDest->iOutstandingBytes 
	       >= spCurrNodeData->spDest->iCwnd) )
	    {
	      spCurrNodeData->spDest->iPartialBytesAcked 
		+= spCurrNodeData->spChunk->sHdr.usLength;
	    }
	}


      // BEGIN -- MultipleFastRtx changes to this function  

      /* This is to ensure that Max.Burst is applied when a SACK
       * acknowledges a chunk which has been fast retransmitted. (If the
       * fast rtx recover variable is set, that can only be because it was
       * fast rtxed.) This is a proposed change to RFC2960 section 7.2.4
       */
      if(spCurrNodeData->uiFastRtxRecover > 0)
	eApplyMaxBurst = TRUE;

      // END -- MultipleFastRtx changes to this function  

	    
      /* We update the RTT estimate if the following hold true:
       *   1. RTO pending flag is set (6.3.1.C4 measured once per round trip)
       *   2. Timestamp is set for this chunk (ie, we were measuring this chunk)
       *   3. This chunk has not been retransmitted
       *   4. This chunk has not been gap acked already 
       *   5. This chunk has not been advanced acked (pr-sctp: exhausted rtxs)
       */
      if(spCurrNodeData->spDest->eRtoPending == TRUE &&
	 spCurrNodeData->dTxTimestamp > 0 &&
	 spCurrNodeData->iNumTxs == 1 &&
	 spCurrNodeData->eGapAcked == FALSE &&
	 spCurrNodeData->eAdvancedAcked == FALSE) 
	{

	  /* If the chunk is marked for timeout rtx, then the sender is an 
	   * ambigious state. Were the sacks lost or was there a failure?
	   * Since we don't clear the error counter below, we also don't
	   * update the RTT. This could be a problem for late arriving SACKs.
	   */
	  if(spCurrNodeData->eMarkedForRtx != TIMEOUT_RTX)
	    RttUpdate(spCurrNodeData->dTxTimestamp, spCurrNodeData->spDest);
	  spCurrNodeData->spDest->eRtoPending = FALSE;
	}

      /* if there is a timer running on the chunk's destination, then stop it
       */
      if(spCurrNodeData->spDest->eRtxTimerIsRunning == TRUE)
	StopT3RtxTimer(spCurrNodeData->spDest);

      /* We don't want to clear the error counter if it's cleared already;
       * otherwise, we'll unnecessarily trigger a trace event.
       *
       * Also, the error counter is cleared by SACKed data ONLY if the
       * TSNs are not marked for timeout retransmission and has not been
       * gap acked before. Without this condition, we can run into a
       * problem for failure detection. When a failure occurs, some data
       * may have made it through before the failure, but the sacks got
       * lost. When the sender retransmits the first outstanding, the
       * receiver will sack all the data whose sacks got lost. We don't
       * want these sacks to clear the error counter, or else failover
       * would take longer.
       */
      if(spCurrNodeData->spDest->iErrorCount != 0 &&
	 spCurrNodeData->eMarkedForRtx != TIMEOUT_RTX &&
	 spCurrNodeData->eGapAcked == FALSE)
	{
	  DBG_PL(SendBufferDequeueUpTo, 
		 "clearing error counter for %p with tsn=%lu"), 
	    spCurrNodeData->spDest, spCurrNodeData->spChunk->uiTsn DBG_PR;

	  spCurrNodeData->spDest->iErrorCount = 0; // clear error counter
	  tiErrorCount++;                          // ... and trace it too!
	  spCurrNodeData->spDest->eStatus = SCTP_DEST_STATUS_ACTIVE;
	  if(spCurrNodeData->spDest == spPrimaryDest &&
	     spNewTxDest != spPrimaryDest) 
	    {
	      DBG_PL(SendBufferDequeueUpTo,
		     "primary recovered... migrating back from %p to %p"),
		spNewTxDest, spPrimaryDest DBG_PR;
	      spNewTxDest = spPrimaryDest; // return to primary
	    }
	}

      /* PN: 5/2007. Simulate send window */
      usChunkSize = spCurrNodeData->spChunk->sHdr.usLength;
      
      spDeleteNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;

      DeleteNode(&sSendBuffer, spDeleteNode);
      spDeleteNode = NULL;

        /* PN: 5/2007. Simulate send window */
      if (uiInitialSwnd) 
      {
        uiAvailSwnd += usChunkSize;

	DBG_PL(SendBufferDequeueUpTo, "AvailSendwindow=%ld"), uiAvailSwnd DBG_PR;
      }

    }

  DBG_X(SendBufferDequeueUpTo);
}

/* returns a boolean of whether a fast retransmit is necessary
 */
Boolean_E MultipleFastRtxSctpAgent::ProcessGapAckBlocks(u_char *ucpSackChunk,
					 Boolean_E eNewCumAck)
{
  DBG_I(ProcessGapAckBlocks);

  Boolean_E eFastRtxNeeded = FALSE;
  u_int uiHighestTsnSacked = uiHighestTsnNewlyAcked;
  u_int uiStartTsn;
  u_int uiEndTsn;

  Node_S *spCurrNode = NULL;
  SctpSendBufferNode_S *spCurrNodeData = NULL;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;

  SctpSackChunk_S *spSackChunk = (SctpSackChunk_S *) ucpSackChunk;

  u_short usNumGapAcksProcessed = 0;

  /* NE: number of Non-Renegable Gap Ack Blocks processed */
  u_short usNumNonRenegGapAcksProcessed = 0;

  SctpGapAckBlock_S *spCurrGapAck = NULL;

  /* NE: 06/2009 */
  SctpGapAckBlock_S *spCurrNonRenegGapAck = NULL;

  Node_S *spPrevNode = sSendBuffer.spHead;
  Boolean_E eDoNotChangeCurrNode = FALSE;

  u_short usNumGapAckBlocks = spSackChunk->usNumGapAckBlocks;

  /* NE: number of Non-Renegable Gap Ack Blocks reported */
  u_short usNumNonRenegGapAckBlocks = 0;

  /* NE: let's decide which block (Gap Ack or Non-Renegable Gap Ack) 
     is used for the calculation */
  Boolean_E eUseGapAckBlock = FALSE;
  u_int uiGapAckStartTsn;
  u_int uiNonRenegGapAckStartTsn;

  /* NE: since the SctpSackChunk_S and SctpNonRenegSackChunk_S structure sizes 
     are different, the offset to point first Gap Ack Block should be calculated
     based on the chunk type */
  if (!eUseNonRenegSacks) { /* SACK Chunk */  
    spCurrGapAck  = 
      (SctpGapAckBlock_S *) (ucpSackChunk + sizeof(SctpSackChunk_S));

    DBG_PL(ProcessGapAckBlocks,"CumAck=%d, NumGapAckBlocks=%u"), 
      spSackChunk->uiCumAck, usNumGapAckBlocks DBG_PR;
  }
  else {                    /* NR-SACK Chunk*/
    usNumGapAckBlocks = 
      ((SctpNonRenegSackChunk_S *)ucpSackChunk)->usNumGapAckBlocks;
    usNumNonRenegGapAckBlocks = 
      ((SctpNonRenegSackChunk_S *)ucpSackChunk)->usNumNonRenegSackBlocks;

    DBG_PL(ProcessGapAckBlocks,
	   "CumAck=%d, NumGapAckBlocks=%u NumNonRenegGapAckBlocks=%u"), 
      spSackChunk->uiCumAck, usNumGapAckBlocks, usNumNonRenegGapAckBlocks DBG_PR;

    /* NE: initial Gap Ack and Non-Renegable Gap Ack Block offsets */
    spCurrGapAck  = 
      (SctpGapAckBlock_S *) (ucpSackChunk + sizeof(SctpNonRenegSackChunk_S));
    spCurrNonRenegGapAck = 
      (SctpGapAckBlock_S *) (ucpSackChunk + sizeof(SctpNonRenegSackChunk_S) + 
			     usNumGapAckBlocks * sizeof(SctpGapAckBlock_S));

    /* NE: if there is no reported Gap Ack Blocks, just use Non-Renegable Gap 
       Ack Blocks */
    if ((usNumGapAckBlocks == 0) && (usNumNonRenegGapAckBlocks > 0)) {
      spCurrGapAck = spCurrNonRenegGapAck; 
      
      eUseGapAckBlock = FALSE;
      
      uiNonRenegGapAckStartTsn = 
	spSackChunk->uiCumAck + spCurrNonRenegGapAck->usStartOffset;
      
      DBG_PL(ProcessGapAckBlocks,"uiNonRenegGapAckStartTsn=%u"), 
	uiNonRenegGapAckStartTsn DBG_PR;
    } /* NE: if there is no reported Non-Renegable Gap Ack Blocks, 
	 just use Gap Ack Blocks */
    else if (usNumNonRenegGapAckBlocks == 0 && (usNumGapAckBlocks > 0)) {
      eUseGapAckBlock = TRUE;

      uiGapAckStartTsn = spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
      
      DBG_PL(ProcessGapAckBlocks,"uiGapAckStartTsn=%u"), 
	uiGapAckStartTsn DBG_PR;
    } /* NE: If there is both Gap Ack and Non-Renegable Gap Ack Blocks, use 
	 the block with lower start TSN value*/
    else if ((usNumGapAckBlocks > 0) && (usNumNonRenegGapAckBlocks > 0)) {
      
      uiGapAckStartTsn = spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
      uiNonRenegGapAckStartTsn = 
	spSackChunk->uiCumAck + spCurrNonRenegGapAck->usStartOffset;

      if (uiGapAckStartTsn < uiNonRenegGapAckStartTsn) {
	eUseGapAckBlock = TRUE;

	DBG_PL(ProcessGapAckBlocks,"uiGapAckStartTsn=%u"), 
	  uiGapAckStartTsn DBG_PR;
      }
      else if (uiNonRenegGapAckStartTsn < uiGapAckStartTsn) {
	spCurrGapAck = spCurrNonRenegGapAck; 
	eUseGapAckBlock = FALSE;
	
	DBG_PL(ProcessGapAckBlocks,"uiNonRenegGapAckStartTsn=%u"), 
	  uiNonRenegGapAckStartTsn DBG_PR;
      }
      else {
	DBG_PL(ProcessGapAckBlocks,
	       "WARNING! This statement should not be reached!") DBG_PR;
      }
    }
    else {
      DBG_PL(ProcessGapAckBlocks,
	     "WARNING! This statement should not be reached!") DBG_PR;
    }
  }

  // BEGIN -- MultipleFastRtx changes to this function    
  u_int uiHighestOutstandingTsn = GetHighestOutstandingTsn();

  /* We want to track any time a regular FR and a MFR is invoked
   */
  Boolean_E eFrInvoked = FALSE;
  Boolean_E eMfrInvoked = FALSE;
  // END -- MultipleFastRtx changes to this function    

  if(sSendBuffer.spHead == NULL) // do we have ANYTHING in the rtx buffer?
    {
      /* This COULD mean that this sack arrived late, and a previous one
       * already cum ack'd everything. ...so, what do we do? nothing??
       */
    }
  
  else // we do have chunks in the rtx buffer
    {
      /* make sure we clear all the spFirstOutstanding pointers before
       * using them!  
       */
      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
	  spCurrDestNodeData->spFirstOutstanding = NULL;
	}

      /* NE: process all reported Gap Ack/Non-Renegable Ack Blocks */
      for(spCurrNode = sSendBuffer.spHead;
	  (spCurrNode != NULL) &&
	    ((usNumGapAcksProcessed != usNumGapAckBlocks) ||
	     (usNumNonRenegGapAcksProcessed != usNumNonRenegGapAckBlocks)); 
	  spCurrNode = spCurrNode->spNext)                
	{
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

	  /* is this chunk the first outstanding on its destination?
	   */
	  if(spCurrNodeData->spDest->spFirstOutstanding == NULL &&
	     spCurrNodeData->eGapAcked == FALSE &&
	     spCurrNodeData->eAdvancedAcked == FALSE)
	    {
	      /* yes, it is the first!
	       */
	      spCurrNodeData->spDest->spFirstOutstanding = spCurrNodeData;
	    }

	  DBG_PL(ProcessGapAckBlocks, "--> rtx list chunk begin") DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "    TSN=%d"), 
	    spCurrNodeData->spChunk->uiTsn 
	    DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "    %s=%s %s=%s"),
	    "eGapAcked", 
	    spCurrNodeData->eGapAcked ? "TRUE" : "FALSE",
	    "eAddedToPartialBytesAcked",
	    spCurrNodeData->eAddedToPartialBytesAcked ? "TRUE" : "FALSE" 
	    DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "    NumMissingReports=%d NumTxs=%d"),
	    spCurrNodeData->iNumMissingReports, 
	    spCurrNodeData->iNumTxs 
	    DBG_PR;

	  DBG_PL(ProcessGapAckBlocks, "<-- rtx list chunk end") DBG_PR;
	  
	  DBG_PL(ProcessGapAckBlocks,"GapAckBlock StartOffset=%d EndOffset=%d"),
	    spCurrGapAck->usStartOffset, spCurrGapAck->usEndOffset DBG_PR;

	  uiStartTsn = spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
	  uiEndTsn = spSackChunk->uiCumAck + spCurrGapAck->usEndOffset;

	  /* NE: Change the calculation of Highest TSN SACKed since the 
	     previous calculation is using the TSNs from the send buffer and 
	     with the introduction of NR-SACKs send buffer may not have
	     the Highest TSN SACKed in the send buffer */
	  if(uiHighestTsnSacked < uiEndTsn)
	    uiHighestTsnSacked = uiEndTsn;
	  
	  DBG_PL(ProcessGapAckBlocks, "GapAckBlock StartTsn=%d EndTsn=%d"),
	    uiStartTsn, uiEndTsn DBG_PR;

	  if(spCurrNodeData->spChunk->uiTsn < uiStartTsn)
	    {
	      /* This chunk is NOT being acked and is missing at the receiver
	       */

	      /* If this chunk was GapAcked before, then either the
	       * receiver has renegged the chunk (which our simulation
	       * doesn't do) or this SACK is arriving out of order.
	       */
	      if(spCurrNodeData->eGapAcked == TRUE)
		{
		  DBG_PL(ProcessGapAckBlocks, 
			 "out of order SACK? setting TSN=%d eGapAcked=FALSE"),
		    spCurrNodeData->spChunk->uiTsn DBG_PR;
		  spCurrNodeData->eGapAcked = FALSE;
		  spCurrNodeData->spDest->iOutstandingBytes 
		    += spCurrNodeData->spChunk->sHdr.usLength;

		  /* section 6.3.2.R4 says that we should restart the
		   * T3-rtx timer here if it isn't running already. In our
		   * implementation, it isn't necessary since
		   * ProcessSackChunk will restart the timer for any
		   * destinations which have outstanding data and don't
		   * have a timer running.
		   */
		}
	    }
	  else if((uiStartTsn <= spCurrNodeData->spChunk->uiTsn) && 
		  (spCurrNodeData->spChunk->uiTsn <= uiEndTsn) )
	    {
	      /* This chunk is being acked via a gap ack block
	       */
	      DBG_PL(ProcessGapAckBlocks, "gap ack acks this chunk: %s%s"),
		"eGapAcked=",
		spCurrNodeData->eGapAcked ? "TRUE" : "FALSE" 
		DBG_PR;

	      /* HTNA algorithm... we need to know the highest TSN sacked
	       * (even if it isn't new), so that when the sender is in
	       * Fast Recovery, the outstanding tsns beyond the last sack
	       * tsn do not have their missing reports incremented
	       */
	      if(uiHighestTsnSacked < spCurrNodeData->spChunk->uiTsn)
		uiHighestTsnSacked = spCurrNodeData->spChunk->uiTsn;

	      if(spCurrNodeData->eGapAcked == FALSE)
		{
		  DBG_PL(ProcessGapAckBlocks, "setting eGapAcked=TRUE") DBG_PR;
		  spCurrNodeData->eGapAcked = TRUE;

		  /* HTNA algorithm... we need to know the highest TSN
		   * newly acked
		   */
		  if(uiHighestTsnNewlyAcked < spCurrNodeData->spChunk->uiTsn)
		    uiHighestTsnNewlyAcked = spCurrNodeData->spChunk->uiTsn;

		  if(spCurrNodeData->eAdvancedAcked == FALSE)
		    {
		      spCurrNodeData->spDest->iNumNewlyAckedBytes 
			+= spCurrNodeData->spChunk->sHdr.usLength;
		    }

		  /* only increment partial bytes acked if we are in
		   * congestion avoidance mode, we have a new cum ack, and
		   * we haven't already incremented it for this sack
		   */
		  if(( spCurrNodeData->spDest->iCwnd 
		       > spCurrNodeData->spDest->iSsthresh) &&
		     eNewCumAck == TRUE &&
		     spCurrNodeData->eAddedToPartialBytesAcked == FALSE)
		    {
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting eAddedToPartiallyBytesAcked=TRUE") DBG_PR;

		      spCurrNodeData->eAddedToPartialBytesAcked = TRUE; // set

		      spCurrNodeData->spDest->iPartialBytesAcked 
			+= spCurrNodeData->spChunk->sHdr.usLength;
		    }
		  /* We update the RTT estimate if the following hold true:
		   *   1. RTO pending flag is set (6.3.1.C4)
		   *   2. Timestamp is set for this chunk 
		   *   3. This chunk has not been retransmitted
		   *   4. This chunk has not been gap acked already 
		   *   5. This chunk has not been advanced acked (pr-sctp)
		   */
		  if(spCurrNodeData->spDest->eRtoPending == TRUE &&
		     spCurrNodeData->dTxTimestamp > 0 &&
		     spCurrNodeData->iNumTxs == 1 &&
		     spCurrNodeData->eAdvancedAcked == FALSE) 
		    {
		      /* If the chunk is marked for timeout rtx, then the
		       * sender is an ambigious state. Were the sacks lost
		       * or was there a failure?  Since we don't clear the
		       * error counter below, we also don't update the
		       * RTT. This could be a problem for late arriving
		       * SACKs.
		       */
		      if(spCurrNodeData->eMarkedForRtx != TIMEOUT_RTX)
			RttUpdate(spCurrNodeData->dTxTimestamp, 
				  spCurrNodeData->spDest);
		      spCurrNodeData->spDest->eRtoPending = FALSE;
		    }

		  /* section 6.3.2.R3 - Stop the timer if this is the
		   * first outstanding for this destination (note: it may
		   * have already been stopped if there was a new cum
		   * ack). If there are still outstanding bytes on this
		   * destination, we'll restart the timer later in
		   * ProcessSackChunk() 
		   */
		  if(spCurrNodeData->spDest->spFirstOutstanding 
		     == spCurrNodeData)
		    
		    {
		      if(spCurrNodeData->spDest->eRtxTimerIsRunning == TRUE)
			StopT3RtxTimer(spCurrNodeData->spDest);
		    }
		  
		  iAssocErrorCount = 0;
		  
		  /* We don't want to clear the error counter if it's
		   * cleared already; otherwise, we'll unnecessarily
		   * trigger a trace event.
		   *
		   * Also, the error counter is cleared by SACKed data
		   * ONLY if the TSNs are not marked for timeout
		   * retransmission and has not been gap acked
		   * before. Without this condition, we can run into a
		   * problem for failure detection. When a failure occurs,
		   * some data may have made it through before the
		   * failure, but the sacks got lost. When the sender
		   * retransmits the first outstanding, the receiver will
		   * sack all the data whose sacks got lost. We don't want
		   * these sacks * to clear the error counter, or else
		   * failover would take longer.
		   */
		  if(spCurrNodeData->spDest->iErrorCount != 0  &&
		     spCurrNodeData->eMarkedForRtx != TIMEOUT_RTX)
		    {
		      DBG_PL(ProcessGapAckBlocks,
			     "clearing error counter for %p with tsn=%lu"), 
			spCurrNodeData->spDest, 
			spCurrNodeData->spChunk->uiTsn DBG_PR;

		      spCurrNodeData->spDest->iErrorCount = 0; // clear errors
		      tiErrorCount++;                       // ... and trace it!
		      spCurrNodeData->spDest->eStatus = SCTP_DEST_STATUS_ACTIVE;
		      if(spCurrNodeData->spDest == spPrimaryDest &&
			 spNewTxDest != spPrimaryDest) 
			{
			  DBG_PL(ProcessGapAckBlocks,
				 "primary recovered... "
				 "migrating back from %p to %p"),
			    spNewTxDest, spPrimaryDest DBG_PR;
			  spNewTxDest = spPrimaryDest; // return to primary
			}
		    }

		  spCurrNodeData->eMarkedForRtx = NO_RTX; // unmark
		}
	    }
	  else if(spCurrNodeData->spChunk->uiTsn > uiEndTsn)
	    {
	      /* This point in the rtx buffer is already past the tsns which are
	       * being acked by this gap ack block.  
	       */

	      /* NE: let's increment the number of Gap Ack/Non-Renegable Gap 
		 Ack Blocks processed*/
	      if (!eUseNonRenegSacks) {
		usNumGapAcksProcessed++;
	      }
	      else {
		if (eUseGapAckBlock) {
		  usNumGapAcksProcessed++;
		}
		else {
		  usNumNonRenegGapAcksProcessed++;
		}
	      }

	      /* Did we process all the gap ack blocks?
	       */
	      if((usNumGapAcksProcessed != usNumGapAckBlocks) || 
		 (usNumNonRenegGapAcksProcessed != usNumNonRenegGapAckBlocks))
		{ 
		  /* NE: since the SctpSackChunk_S and SctpNonRenegSackChunk_S 
		     structure sizes are different, the offset to point next 
		     Gap Ack Block should be calculated based on the chunk type 
		  */ 
		  if (!eUseNonRenegSacks) 
		    {
		      spCurrGapAck 
			= ((SctpGapAckBlock_S *)
			   (ucpSackChunk + sizeof(SctpSackChunk_S) 
			    +(usNumGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
		      
		      DBG_PL(ProcessGapAckBlocks, "jump to next gap ack block") 
			DBG_PR;
		    }
		  else 
		    {
		      spCurrGapAck 
			= ((SctpGapAckBlock_S *)
			   (ucpSackChunk + sizeof(SctpNonRenegSackChunk_S) + 
			    (usNumGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
		      
		      spCurrNonRenegGapAck 
			= ((SctpGapAckBlock_S *)
			   (ucpSackChunk + sizeof(SctpNonRenegSackChunk_S)
			    +(usNumGapAckBlocks * sizeof(SctpGapAckBlock_S)) 
			    +(usNumNonRenegGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
		      
		      /* NE: Let's determine which block to use next. The next 
			 Gap Ack Block or the next Non-Renegable Gap Ack Block.
			 If all Gap Ack Blocks are processed let's use 
			 Non-Renegable Gap Ack Blocks */
		      if ((usNumGapAckBlocks == usNumGapAcksProcessed) && 
			  (usNumNonRenegGapAckBlocks > usNumNonRenegGapAcksProcessed)) {
			
			spCurrGapAck = spCurrNonRenegGapAck; 
			
			eUseGapAckBlock = FALSE;

			uiNonRenegGapAckStartTsn = 
			  spSackChunk->uiCumAck + spCurrNonRenegGapAck->usStartOffset;			
			
			DBG_PL(ProcessGapAckBlocks, "jump to next nr gap ack block") 
			  DBG_PR;
			
			DBG_PL(ProcessGapAckBlocks,"uiNonRenegGapAckStartTsn=%u"), 
			  uiNonRenegGapAckStartTsn DBG_PR;

		      } /* NE: If all Non-Renegable Gap Ack Blocks are processed, let's 
			   use Gap Ack Blocks */
		      else if ((usNumNonRenegGapAckBlocks == usNumNonRenegGapAcksProcessed)
			       && (usNumGapAckBlocks > usNumGapAcksProcessed)) {
			
			eUseGapAckBlock = TRUE;

			uiGapAckStartTsn = 
			  spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
			
			DBG_PL(ProcessGapAckBlocks, "jump to next gap ack block") 
			  DBG_PR;
			
			DBG_PL(ProcessGapAckBlocks,"uiGapAckStartTsn=%u"), 
			  uiGapAckStartTsn DBG_PR;

		      } /* NE: If there is both Gap Ack and Non-Renegable Gap 
			   Ack Blocks, use the block with lower start TSN value*/
		      else if ((usNumGapAckBlocks > usNumGapAcksProcessed) && 
			       (usNumNonRenegGapAckBlocks > usNumNonRenegGapAcksProcessed)){
			
			uiGapAckStartTsn = 
			  spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
			uiNonRenegGapAckStartTsn = 
			  spSackChunk->uiCumAck + spCurrNonRenegGapAck->usStartOffset;
			
			if (uiGapAckStartTsn < uiNonRenegGapAckStartTsn) {
			  
			  eUseGapAckBlock = TRUE;

			  DBG_PL(ProcessGapAckBlocks, "jump to next gap ack block") 
			    DBG_PR;

			  DBG_PL(ProcessGapAckBlocks,"uiGapAckStartTsn=%u"), 
			    uiGapAckStartTsn DBG_PR;
			}
			else if (uiNonRenegGapAckStartTsn < uiGapAckStartTsn) {
			  
			  spCurrGapAck = spCurrNonRenegGapAck; 
			  
			  eUseGapAckBlock = FALSE;

			  DBG_PL(ProcessGapAckBlocks, "jump to next nr gap ack block") 
			    DBG_PR;

			  DBG_PL(ProcessGapAckBlocks,"uiNonRenegGapAckStartTsn=%u"), 
			    uiNonRenegGapAckStartTsn DBG_PR;
			}
			else {
			  DBG_PL(ProcessGapAckBlocks,
				 "WARNING! This statement should not be reached!") 
			    DBG_PR;
			}
		      }
		      
		      eDoNotChangeCurrNode=TRUE;
		    }
		}

	      /* If this chunk was GapAcked before, then either the
	       * receiver has renegged the chunk (which our simulation
	       * doesn't do) or this SACK is arriving out of order.
	       */
	      /* NE: with the new implementation of NR-SACKs, Gap Ack Blocks and
		 Non-Renegable Gap Ack Blocks are disjoint and may constitute 
		 consecutive blocks which may cause a TSN to be seem like 
		 RENEGED but it is not the case */
	      if(spCurrNodeData->eGapAcked == TRUE && !eUseNonRenegSacks)
		{
		  DBG_PL(ProcessGapAckBlocks, 
			 "out of order SACK? setting TSN=%d eGapAcked=FALSE"),
		    spCurrNodeData->spChunk->uiTsn DBG_PR;
		  spCurrNodeData->eGapAcked = FALSE;
		  spCurrNodeData->spDest->iOutstandingBytes 
		    += spCurrNodeData->spChunk->sHdr.usLength;
		  
		  /* section 6.3.2.R4 says that we should restart the
		   * T3-rtx timer here if it isn't running already. In our
		   * implementation, it isn't necessary since
		   * ProcessSackChunk will restart the timer for any
		   * destinations which have outstanding data and don't
		   * have a timer running.
		   */
		}
	    }
	  
	  /* 04/18: PN: With NR-SACKS tsns can be missing in the send buffer,
	   * Need to track previous sbuf node for correct gap ack processing
	     */
	  if (eDoNotChangeCurrNode == TRUE) {
	    spCurrNode = spPrevNode; //spPrevNode remains unchanged 
	  }
	  else {
	    spPrevNode = spCurrNode; 
	  }
	  eDoNotChangeCurrNode=FALSE;
	}

      /* By this time, either we have run through the entire send buffer or we
       * have run out of gap ack blocks. In the case that we have run out of gap
       * ack blocks before we finished running through the send buffer, we need
       * to mark the remaining chunks in the send buffer as eGapAcked=FALSE.
       * This final marking needs to be done, because we only trust gap ack info
       * from the last SACK. Otherwise, renegging (which we don't do) or out of
       * order SACKs would give the sender an incorrect view of the peer's rwnd.
       */
      for(; spCurrNode != NULL; spCurrNode = spCurrNode->spNext)
	{
	  /* This chunk is NOT being acked and is missing at the receiver
	   */
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

	  /* If this chunk was GapAcked before, then either the
	   * receiver has renegged the chunk (which our simulation
	   * doesn't do) or this SACK is arriving out of order.
	   */
	  if(spCurrNodeData->eGapAcked == TRUE)
	    {
	      DBG_PL(ProcessGapAckBlocks, 
		     "out of order SACK? setting TSN=%d eGapAcked=FALSE"),
		spCurrNodeData->spChunk->uiTsn DBG_PR;
	      spCurrNodeData->eGapAcked = FALSE;
	      spCurrNodeData->spDest->iOutstandingBytes 
		+= spCurrNodeData->spChunk->sHdr.usLength;

	      /* section 6.3.2.R4 says that we should restart the T3-rtx
	       * timer here if it isn't running already. In our
	       * implementation, it isn't necessary since ProcessSackChunk
	       * will restart the timer for any destinations which have
	       * outstanding data and don't have a timer running.
	       */
	    }
	}

      /* NE : We need to process all Gap Ack Blocks since the uiHighestTsnSacked 
	 variable is calculated using the Gap Ack Blocks instead of using TSNs 
	 in the send buffer. So, if there are Gap Ack Blocks which are not 
	 processed then use them to calculate uiHighestTsnSacked variable */ 
      /* Did we process all the gap ack blocks?
       */
      while(usNumGapAcksProcessed != usNumGapAckBlocks)
	{
	  DBG_PL(ProcessGapAckBlocks, "jump to next gap ack block") 
	    DBG_PR;
	  
	  /* NE: since the SctpSackChunk_S and SctpNonRenegSackChunk_S structure
	     sizes are different, the offset to point first Gap Ack Block should
	     be calculated based on the chunk type */
	  if (!eUseNonRenegSacks) 
	    {
	      spCurrGapAck 
		= ((SctpGapAckBlock_S *)
		   (ucpSackChunk + sizeof(SctpSackChunk_S)
		    +(usNumGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
	    }
	  else {
	    
	    spCurrGapAck 
	      = ((SctpGapAckBlock_S *)
		 (ucpSackChunk + sizeof(SctpNonRenegSackChunk_S)
		  +(usNumGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
	  }
	  
	  uiStartTsn = spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
	  uiEndTsn = spSackChunk->uiCumAck + spCurrGapAck->usEndOffset;

	  /* NE: Change the calculation of Highest TSN SACKed since the 
	     previous calculation is using the TSNs from the send buffer and 
	     with the introduction of NR-SACKs send buffer may not have
	     the Highest TSN SACKed in the send buffer */
	  if(uiHighestTsnSacked < uiEndTsn)
	    uiHighestTsnSacked = uiEndTsn;
	  
	  usNumGapAcksProcessed++;
	}

      /* NE : We need to process also all Non-Renegable Gap Ack Blocks since the
	 uiHighestTsnSacked variable is calculated using the Gap Ack Blocks 
	 instead of using TSNs in the send buffer. So, if there are Gap Ack 
	 Blocks which are not processed then use them to calculate 
	 uiHighestTsnSacked variable */ 
      /* Did we process all the non-renegable gap ack blocks?
       */
      if (eUseNonRenegSacks) 
	{
	  while(usNumNonRenegGapAcksProcessed != usNumNonRenegGapAckBlocks)
	    {
	      DBG_PL(ProcessGapAckBlocks, "jump to next nr-gap ack block") 
		DBG_PR;
	      
	      spCurrGapAck 
		= ((SctpGapAckBlock_S *)
		   (ucpSackChunk + sizeof(SctpNonRenegSackChunk_S)
		    +(usNumGapAckBlocks * sizeof(SctpGapAckBlock_S))
		    +(usNumNonRenegGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
	      
	      
	      uiStartTsn = spSackChunk->uiCumAck + spCurrGapAck->usStartOffset;
	      uiEndTsn = spSackChunk->uiCumAck + spCurrGapAck->usEndOffset;

	      /* NE: Change the calculation of Highest TSN SACKed since the 
		 previous calculation is using the TSNs from the send buffer and 
		 with the introduction of NR-SACKs send buffer may not have
		 the Highest TSN SACKed in the send buffer */
	      if(uiHighestTsnSacked < uiEndTsn)
		uiHighestTsnSacked = uiEndTsn;
	      
	      usNumNonRenegGapAcksProcessed++;
	    }
	}

      DBG_PL(ProcessGapAckBlocks, "now incrementing missing reports...") DBG_PR;
      DBG_PL(ProcessGapAckBlocks, "uiHighestTsnNewlyAcked=%d"), 
	     uiHighestTsnNewlyAcked DBG_PR;

      for(spCurrNode = sSendBuffer.spHead;
	  spCurrNode != NULL; 
	  spCurrNode = spCurrNode->spNext)
	{
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

	  DBG_PL(ProcessGapAckBlocks, "TSN=%d eGapAcked=%s"), 
	    spCurrNodeData->spChunk->uiTsn,
	    spCurrNodeData->eGapAcked ? "TRUE" : "FALSE"
	    DBG_PR;

	  if(spCurrNodeData->eGapAcked == FALSE)
	    {
	      // BEGIN -- MultipleFastRtx changes to this function  

	      /* Caro's Multiple Fast Rtx Algorithm 
	       * (in addition to existing HTNA algorithm)
	       */
	      if(( (spCurrNodeData->spChunk->uiTsn < uiHighestTsnNewlyAcked) ||
		   (eNewCumAck == TRUE && 
		    uiHighestTsnNewlyAcked <= uiRecover &&
		    spCurrNodeData->spChunk->uiTsn < uiHighestTsnSacked) ) &&
		 uiHighestTsnNewlyAcked > spCurrNodeData->uiFastRtxRecover)
		{
		  spCurrNodeData->iNumMissingReports++;
		  DBG_PL(ProcessGapAckBlocks, 
			 "incrementing missing report for TSN=%d to %d"), 
		    spCurrNodeData->spChunk->uiTsn,
		    spCurrNodeData->iNumMissingReports
		    DBG_PR;

		  if(spCurrNodeData->iNumMissingReports == iFastRtxTrigger &&
		     spCurrNodeData->eAdvancedAcked == FALSE)
		    {
		      if(spCurrNodeData->uiFastRtxRecover == 0)
			eFrInvoked = TRUE;
		      else
			eMfrInvoked = TRUE;

		      spCurrNodeData->iNumMissingReports = 0;
		      MarkChunkForRtx(spCurrNodeData, FAST_RTX);
		      eFastRtxNeeded = TRUE;
		      spCurrNodeData->uiFastRtxRecover =uiHighestOutstandingTsn;
			
		      DBG_PL(ProcessGapAckBlocks, 
			     "resetting missing report for TSN=%d to 0"), 
			spCurrNodeData->spChunk->uiTsn
			DBG_PR;
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting uiFastRtxRecover=%d"), 
			spCurrNodeData->uiFastRtxRecover
			DBG_PR;
		    }
		}
	      // END -- MultipleFastRtx changes to this function  
	    }
	}
    }

  // BEGIN -- MultipleFastRtx changes to this function    
  if(eFrInvoked == TRUE)
    tiFrCount++;
  if(eMfrInvoked == TRUE)
    tiMfrCount++;
  // END -- MultipleFastRtx changes to this function    

  DBG_PL(ProcessGapAckBlocks, "eFastRtxNeeded=%s"), 
    eFastRtxNeeded ? "TRUE" : "FALSE" DBG_PR;
  DBG_X(ProcessGapAckBlocks);
  return eFastRtxNeeded;
}
