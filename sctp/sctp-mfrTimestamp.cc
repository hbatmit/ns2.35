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
"@(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp-mfrTimestamp.cc,v 1.3 2009/11/16 05:51:27 tom_henderson Exp $ (UD/PEL)";
#endif

#include "ip.h"
#include "sctp-mfrTimestamp.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "sctpDebug.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))

static class MfrTimestampSctpClass : public TclClass 
{ 
public:
  MfrTimestampSctpClass() : TclClass("Agent/SCTP/MfrTimestamp") {}
  TclObject* create(int, const char*const*) 
  {
    return (new MfrTimestampSctpAgent());
  }
} classSctpMfrTimestamp;

MfrTimestampSctpAgent::MfrTimestampSctpAgent() : SctpAgent()
{
}

void MfrTimestampSctpAgent::delay_bind_init_all()
{
  delay_bind_init_one("mfrCount_");
  TimestampSctpAgent::delay_bind_init_all();
}

int MfrTimestampSctpAgent::delay_bind_dispatch(const char *cpVarName, 
					  const char *cpLocalName, 
					  TclObject *opTracer)
{
  if(delay_bind(cpVarName, cpLocalName, "mfrCount_", &tiMfrCount, opTracer))
    return TCL_OK;

  return TimestampSctpAgent::delay_bind_dispatch(cpVarName, 
						 cpLocalName, opTracer);
}

void MfrTimestampSctpAgent::TraceAll()
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
	      "frCount: %d mfrCount: %d timeoutCount: %d rcdCount: %d\n",
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
	      spCurrDest->iRcdCount);
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }

  sprintf(cpOutString, "\n");
  if(channel_)
    (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
}

void MfrTimestampSctpAgent::TraceVar(const char* cpVar)
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


void MfrTimestampSctpAgent::AddToSendBuffer(SctpDataChunkHdr_S *spChunk, 
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

  // BEGIN -- Timestamp changes to this function
  spNewNodeData->dTxTimestamp = Scheduler::instance().clock();
  // END -- Timestamp changes to this function

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
void MfrTimestampSctpAgent::SendBufferDequeueUpTo(u_int uiTsn)
{
  DBG_I(SendBufferDequeueUpTo);

  Node_S *spDeleteNode = NULL;
  Node_S *spCurrNode = sSendBuffer.spHead;
  SctpSendBufferNode_S *spCurrNodeData = NULL;

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

	    
      // BEGIN -- Timestamp changes to this function

      /* We update the RTT estimate if the following hold true:
       *   1. Timestamp set for this chunk matches echoed timestamp
       *   2. This chunk has not been gap acked already 
       *   3. This chunk has not been advanced acked (pr-sctp: exhausted rtxs)
       */
      if(fInTimestampEcho == (float) spCurrNodeData->dTxTimestamp &&
	 spCurrNodeData->eGapAcked == FALSE &&
	 spCurrNodeData->eAdvancedAcked == FALSE) 
	{
	  RttUpdate(spCurrNodeData->dTxTimestamp, spCurrNodeData->spDest);
	}

      // END -- Timestamp changes to this function


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

      spDeleteNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;
      DeleteNode(&sSendBuffer, spDeleteNode);
      spDeleteNode = NULL;
    }

  DBG_X(SendBufferDequeueUpTo);
}

/* returns a boolean of whether a fast retransmit is necessary
 */
Boolean_E MfrTimestampSctpAgent::ProcessGapAckBlocks(u_char *ucpSackChunk,
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
  SctpGapAckBlock_S *spCurrGapAck 
    = (SctpGapAckBlock_S *) (ucpSackChunk + sizeof(SctpSackChunk_S));

  // BEGIN -- MultipleFastRtx changes to this function    
  u_int uiHighestOutstandingTsn = GetHighestOutstandingTsn();

  /* We want to track any time a regular FR and a MFR is invoked
   */
  Boolean_E eFrInvoked = FALSE;
  Boolean_E eMfrInvoked = FALSE;
  // END -- MultipleFastRtx changes to this function    

  DBG_PL(ProcessGapAckBlocks,"CumAck=%d"), spSackChunk->uiCumAck DBG_PR;

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

      for(spCurrNode = sSendBuffer.spHead;
	  (spCurrNode != NULL) &&
	    (usNumGapAcksProcessed != spSackChunk->usNumGapAckBlocks);
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

		  // BEGIN -- Timestamp changes to this function

		  /* We update the RTT estimate if the following hold true:
		   *   1. Timestamp set for this chunk matches echoed timestamp
		   *   2. This chunk has not been gap acked already 
		   *   3. This chunk has not been advanced acked
		   */
		  if(fInTimestampEcho == (float) spCurrNodeData->dTxTimestamp &&
		     spCurrNodeData->eAdvancedAcked == FALSE) 
		    {
		      RttUpdate(spCurrNodeData->dTxTimestamp, 
				spCurrNodeData->spDest);
		    }

		  // END -- Timestamp changes to this function


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
	      usNumGapAcksProcessed++; 

	      /* Did we process all the gap ack blocks?
	       */
	      if(usNumGapAcksProcessed != spSackChunk->usNumGapAckBlocks)
		{
		  DBG_PL(ProcessGapAckBlocks, "jump to next gap ack block") 
		    DBG_PR;

		  spCurrGapAck 
		    = ((SctpGapAckBlock_S *)
		       (ucpSackChunk + sizeof(SctpSackChunk_S)
			+ (usNumGapAcksProcessed * sizeof(SctpGapAckBlock_S))));
		}

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
