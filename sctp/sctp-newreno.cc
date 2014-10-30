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

/* NewReno extension introduces a recover state variable per destination
 * similar to avoid multiple cwnd cuts in a single window (ie, round
 * trip). The concept is borrowed from TCP NewReno.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp-newreno.cc,v 1.5 2009/11/16 05:51:27 tom_henderson Exp $ (UD/PEL)";
#endif

#include "ip.h"
#include "sctp-newreno.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "sctpDebug.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))

static class NewRenoSctpClass : public TclClass 
{ 
public:
  NewRenoSctpClass() : TclClass("Agent/SCTP/Newreno") {}
  TclObject* create(int, const char*const*) 
  {
    return (new NewRenoSctpAgent());
  }
} classSctpNewReno;

NewRenoSctpAgent::NewRenoSctpAgent() : SctpAgent()
{
}

void NewRenoSctpAgent::delay_bind_init_all()
{
  SctpAgent::delay_bind_init_all();
}

int NewRenoSctpAgent::delay_bind_dispatch(const char *cpVarName, 
					  const char *cpLocalName, 
					  TclObject *opTracer)
{
  return SctpAgent::delay_bind_dispatch(cpVarName, cpLocalName, opTracer);
}

void NewRenoSctpAgent::OptionReset()
{
  DBG_I(OptionReset);
  uiRecover = 0;
  DBG_X(OptionReset);
}

/* Go thru the send buffer deleting all chunks which have a tsn <= the 
 * tsn parameter passed in. We assume the chunks in the rtx list are ordered by 
 * their tsn value. In addtion, for each chunk deleted:
 *   1. we add the chunk length to # newly acked bytes and partial bytes acked
 *   2. we update round trip time if appropriate
 *   3. stop the timer if the chunk's destination timer is running
 */
void NewRenoSctpAgent::SendBufferDequeueUpTo(u_int uiTsn)
{
  DBG_I(SendBufferDequeueUpTo);

  Node_S *spDeleteNode = NULL;
  Node_S *spCurrNode = sSendBuffer.spHead;
  SctpSendBufferNode_S *spCurrNodeData = NULL;

  /* Only the first TSN that is being dequeued can be used to reset the
   * error cunter on a destination. Why? Well, suppose there are some
   * chunks that were gap acked before the primary had errors. Then the
   * gap gets filled with a retransmission using an alternate path. The
   * filled gap will cause the cum ack to move past the gap acked TSNs,
   * but does not mean that the they can reset the errors on the primary.
   */
  
  uiAssocErrorCount = 0;

  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

  /* trigger trace ONLY if it was previously NOT 0 */
  if(spCurrNodeData->spDest->uiErrorCount != 0)
    {
      spCurrNodeData->spDest->uiErrorCount = 0; // clear error counter
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
	  // BEGIN -- NewReno changes to this function
	  uiHighestTsnNewlyAcked = spCurrNodeData->spChunk->uiTsn;
	  // END -- NewReno changes to this function

	  spCurrNodeData->spDest->uiNumNewlyAckedBytes 
	    += spCurrNodeData->spChunk->sHdr.usLength;

	  /* only add to partial bytes acked if we are in congestion
	   * avoidance mode and if there was cwnd amount of data
	   * outstanding on the destination (implementor's guide) 
	   */
	  if(spCurrNodeData->spDest->uiCwnd >spCurrNodeData->spDest->uiSsthresh &&
	     ( spCurrNodeData->spDest->uiOutstandingBytes 
	       >= spCurrNodeData->spDest->uiCwnd) )
	    {
	      spCurrNodeData->spDest->uiPartialBytesAcked 
		+= spCurrNodeData->spChunk->sHdr.usLength;
	    }
	}


      /* This is to ensure that Max.Burst is applied when a SACK
       * acknowledges a chunk which has been fast retransmitted. If it is
       * ineligible for fast rtx, that can only be because it was fast
       * rtxed or it timed out. If it timed out, a burst shouldn't be
       * possible, but shouldn't hurt either. The fast rtx case is what we
       * are really after. This is a proposed change to RFC2960 section
       * 7.2.4
       */
      if(spCurrNodeData->eIneligibleForFastRtx == TRUE)
	eApplyMaxBurst = TRUE;
	    
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
	  RttUpdate(spCurrNodeData->dTxTimestamp, spCurrNodeData->spDest);
	  spCurrNodeData->spDest->eRtoPending = FALSE;
	}

      /* if there is a timer running on the chunk's destination, then stop it
       */
      if(spCurrNodeData->spDest->eRtxTimerIsRunning == TRUE)
	StopT3RtxTimer(spCurrNodeData->spDest);

      spDeleteNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;
      DeleteNode(&sSendBuffer, spDeleteNode);
      spDeleteNode = NULL;
    }

  DBG_X(SendBufferDequeueUpTo);
}

/* This function is called as soon as we are done processing a SACK which 
 * notifies the need of a fast rtx. Following RFC2960, we pack as many chunks
 * as possible into one packet (PTMU restriction). The remaining marked packets
 * are sent as soon as cwnd allows.
 */
void NewRenoSctpAgent::FastRtx()
{
  DBG_I(FastRtx);

  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestData = NULL;
  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffData = NULL;
  SctpSendBufferNode_S *spTailData = NULL;

  /* be sure we clear all the eCcApplied flags before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestData->eCcApplied = FALSE;
    }
  
  /* go thru chunks marked for rtx and cut back their ssthresh, cwnd, and
   * partial bytes acked. make sure we only apply congestion control once
   * per destination and once per window (ie, round-trip).
   */
  for(spCurrBuffNode = sSendBuffer.spHead;
      spCurrBuffNode != NULL;
      spCurrBuffNode = spCurrBuffNode->spNext)
    {
      spCurrBuffData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;


      // BEGIN -- NewReno changes to this function

      /* If the chunk has been either marked for rtx or advanced ack, we want
       * to apply congestion control (assuming we didn't already).
       *
       * Why do we do it for advanced ack chunks? Well they were advanced ack'd
       * because they were lost. The ONLY reason we are not fast rtxing them is
       * because the chunk has run out of retransmissions (u-sctp). So we need
       * to still account for the fact they were lost... so apply congestion
       * control!
       */
      if((spCurrBuffData->eMarkedForRtx == TRUE ||
	  spCurrBuffData->eAdvancedAcked == TRUE) &&
	 spCurrBuffData->spDest->eCcApplied == FALSE &&
	 spCurrBuffData->spChunk->uiTsn > uiRecover)
	 
	{ 
	  /* section 7.2.3 of rfc2960 (w/ implementor's guide v.02) 
	   */
	  spCurrBuffData->spDest->uiSsthresh 
	    = MAX(spCurrBuffData->spDest->uiCwnd/2, 2*uiMaxPayloadSize);
	  spCurrBuffData->spDest->uiCwnd = spCurrBuffData->spDest->uiSsthresh;
	  spCurrBuffData->spDest->uiPartialBytesAcked = 0; //reset
	  tiCwnd++; // trigger changes for trace to pick up
	  spCurrBuffData->spDest->eCcApplied = TRUE;

	  /* Set the recover variable to avoid multiple cwnd cuts for losses
	   * in the same window (ie, round-trip).
	   */
	  spTailData = (SctpSendBufferNode_S *) sSendBuffer.spTail->vpData;
	  uiRecover = spTailData->spChunk->uiTsn;
	}

      // END -- NewReno changes to this function
    }

  /* possible that no chunks are pending retransmission since they could be 
   * advanced ack'd 
   */
  if(eMarkedChunksPending == TRUE)  
    RtxMarkedChunks(RTX_LIMIT_ONE_PACKET);

  DBG_X(FastRtx);
}

/* returns a boolean of whether a fast retransmit is necessary
 */
Boolean_E NewRenoSctpAgent::ProcessGapAckBlocks(u_char *ucpSackChunk,
					 Boolean_E eNewCumAck)
{
  DBG_I(ProcessGapAckBlocks);

  Boolean_E eFastRtxNeeded = FALSE;
  u_int uiHighestTsnSacked = uiHighestTsnNewlyAcked; // NewReno changes
  u_int uiStartTsn;
  u_int uiEndTsn;
  Node_S *spCurrNode = NULL;
  SctpSendBufferNode_S *spCurrNodeData = NULL;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;
  Boolean_E eFirstOutstanding = FALSE;  

  SctpSackChunk_S *spSackChunk = (SctpSackChunk_S *) ucpSackChunk;

  u_short usNumGapAcksProcessed = 0;
  SctpGapAckBlock_S *spCurrGapAck 
    = (SctpGapAckBlock_S *) (ucpSackChunk + sizeof(SctpSackChunk_S));

  DBG_PL(ProcessGapAckBlocks,"CumAck=%d"), spSackChunk->uiCumAck DBG_PR;

  if(sSendBuffer.spHead == NULL) // do we have ANYTHING in the rtx buffer?
    {
      /* This COULD mean that this sack arrived late, and a previous one
       * already cum ack'd everything. ...so, what do we do? nothing??
       */
    }
  
  else // we do have chunks in the rtx buffer
    {
      /* make sure we clear all the eSeenFirstOutstanding flags before
       * using them!  
       */
      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
	  spCurrDestNodeData->eSeenFirstOutstanding = FALSE;
	}

      for(spCurrNode = sSendBuffer.spHead;
	  (spCurrNode != NULL) &&
	    (usNumGapAcksProcessed != spSackChunk->usNumGapAckBlocks);
	  spCurrNode = spCurrNode->spNext)
	{
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

	  DBG_PL(ProcessGapAckBlocks, "eSeenFirstOutstanding=%s"),
	    spCurrNodeData->spDest->eSeenFirstOutstanding ? "TRUE" : "FALSE"
	    DBG_PR;
		 
	  /* is this chunk the first outstanding on its destination?
	   */
	  if(spCurrNodeData->spDest->eSeenFirstOutstanding == FALSE &&
	     spCurrNodeData->eGapAcked == FALSE &&
	     spCurrNodeData->eAdvancedAcked == FALSE)
	    {
	      /* yes, it is the first!
	       */
	      eFirstOutstanding = TRUE;
	      spCurrNodeData->spDest->eSeenFirstOutstanding = TRUE;
	    }
	  else
	    {
	      /* nope, not the first...
	       */
	      eFirstOutstanding = FALSE;
	    }

	  DBG_PL(ProcessGapAckBlocks, "eFirstOutstanding=%s"),
	    eFirstOutstanding ? "TRUE" : "FALSE" DBG_PR;

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
		  spCurrNodeData->spDest->uiOutstandingBytes 
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

	      // BEGIN -- NewReno changes to this function

	      /* modified HTNA algorithm... we need to know the highest TSN
	       * sacked (even if it isn't new), so that when the sender
	       * is in Fast Recovery, the outstanding tsns beyond the 
	       * last sack tsn do not have their missing reports incremented
	       */
	      if(uiHighestTsnSacked < spCurrNodeData->spChunk->uiTsn)
		uiHighestTsnSacked = spCurrNodeData->spChunk->uiTsn;

	      // END -- NewReno changes to this function

	      if(spCurrNodeData->eGapAcked == FALSE)
		{
		  DBG_PL(ProcessGapAckBlocks, "setting eGapAcked=TRUE") DBG_PR;
		  spCurrNodeData->eGapAcked = TRUE;
		  spCurrNodeData->eMarkedForRtx = FALSE; // unmark

		  /* HTNA algorithm... we need to know the highest TSN
		   * newly acked
		   */
		  if(uiHighestTsnNewlyAcked < spCurrNodeData->spChunk->uiTsn)
		    uiHighestTsnNewlyAcked = spCurrNodeData->spChunk->uiTsn;

		  if(spCurrNodeData->eAdvancedAcked == FALSE)
		    {
		      spCurrNodeData->spDest->uiNumNewlyAckedBytes 
			+= spCurrNodeData->spChunk->sHdr.usLength;
		    }
		  
		  /* only increment partial bytes acked if we are in
		   * congestion avoidance mode, we have a new cum ack, and
		   * we haven't already incremented it for this TSN
		   */
		  if(( spCurrNodeData->spDest->uiCwnd 
		       > spCurrNodeData->spDest->uiSsthresh) &&
		     eNewCumAck == TRUE &&
		     spCurrNodeData->eAddedToPartialBytesAcked == FALSE)
		    {
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting eAddedToPartiallyBytesAcked=TRUE") DBG_PR;
		      
		      spCurrNodeData->eAddedToPartialBytesAcked = TRUE; // set

		      spCurrNodeData->spDest->uiPartialBytesAcked 
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
		  if(eFirstOutstanding == TRUE 
		     && spCurrNodeData->spDest->eRtxTimerIsRunning == TRUE)
		    StopT3RtxTimer(spCurrNodeData->spDest);
		  
		  uiAssocErrorCount = 0;
		  
		  /* trigger trace ONLY if it was previously NOT 0
		   */
		  if(spCurrNodeData->spDest->uiErrorCount != 0)
		    {
		      spCurrNodeData->spDest->uiErrorCount = 0; // clear errors
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
		  spCurrNodeData->spDest->uiOutstandingBytes 
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
		  spCurrNodeData->spDest->uiOutstandingBytes 
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
	      // BEGIN -- NewReno changes to this function

	      /* HTNA (Highest TSN Newly Acked) algorithm from
	       * implementer' guide
	       *
	       * NewReno modifies HTNA to also increment the missing reports 
	       * when the sender is in Fast Recovery, the cum ack changes, and
	       * the new cum ack is less than recover.
	       */
	      if( (spCurrNodeData->spChunk->uiTsn < uiHighestTsnNewlyAcked) ||
		  (eNewCumAck == TRUE && 
		   uiHighestTsnNewlyAcked <= uiRecover &&
		   spCurrNodeData->spChunk->uiTsn < uiHighestTsnSacked))
		{
		  spCurrNodeData->iNumMissingReports++;
		  DBG_PL(ProcessGapAckBlocks, 
			 "incrementing missing report for TSN=%d to %d"), 
		    spCurrNodeData->spChunk->uiTsn,
		    spCurrNodeData->iNumMissingReports
		    DBG_PR;

		  if(spCurrNodeData->iNumMissingReports >= FAST_RTX_TRIGGER &&
		     spCurrNodeData->eIneligibleForFastRtx == FALSE &&
		     spCurrNodeData->eAdvancedAcked == FALSE)
		    {
		      MarkChunkForRtx(spCurrNodeData);
		      eFastRtxNeeded = TRUE;
		      spCurrNodeData->eIneligibleForFastRtx = TRUE;
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting eFastRtxNeeded = TRUE") DBG_PR;
		    }
		}
	      // END -- NewReno changes to this function
	    }
	}
    }

  DBG_PL(ProcessGapAckBlocks, "eFastRtxNeeded=%s"), 
    eFastRtxNeeded ? "TRUE" : "FALSE" DBG_PR;
  DBG_X(ProcessGapAckBlocks);
  return eFastRtxNeeded;
}

void NewRenoSctpAgent::ProcessSackChunk(u_char *ucpSackChunk)
{
  DBG_I(ProcessSackChunk);

  SctpSackChunk_S *spSackChunk = (SctpSackChunk_S *) ucpSackChunk;

  DBG_PL(ProcessSackChunk, "cum=%d arwnd=%d #gapacks=%d #duptsns=%d"),
    spSackChunk->uiCumAck, spSackChunk->uiArwnd, 
    spSackChunk->usNumGapAckBlocks, spSackChunk->usNumDupTsns 
    DBG_PR;

  Boolean_E eFastRtxNeeded = FALSE;
  Boolean_E eNewCumAck = FALSE;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;
  u_int uiTotalOutstanding = 0;
  int i = 0;

  /* make sure we clear all the uiNumNewlyAckedBytes before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->uiNumNewlyAckedBytes = 0;
      spCurrDestNodeData->eSeenFirstOutstanding = FALSE;
    }

  if(spSackChunk->uiCumAck < uiCumAckPoint) 
    {
      /* this cumAck's a previously cumAck'd tsn (ie, it's out of order!)
       * ...so ignore!
       */
      DBG_PL(ProcessSackChunk, "ignoring out of order sack!") DBG_PR;
      DBG_X(ProcessSackChunk);
      return;
    }
  else if(spSackChunk->uiCumAck > uiCumAckPoint)
    {
      eNewCumAck = TRUE; // incomding SACK's cum ack advances the cum ack point
      SendBufferDequeueUpTo(spSackChunk->uiCumAck);
      uiCumAckPoint = spSackChunk->uiCumAck; // Advance the cumAck pointer
    }

  if(spSackChunk->usNumGapAckBlocks != 0) // are there any gaps??
    {
      eFastRtxNeeded = ProcessGapAckBlocks(ucpSackChunk, eNewCumAck);
    } 

  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;

      // BEGIN -- NewReno changes to this function

      /* Only adjust cwnd if:
       *    1. sack advanced the cum ack point 
       *    2. this destination has newly acked bytes
       *    3. the cum ack is at or beyond the recovery window
       *
       * Also, we MUST adjust our congestion window BEFORE we update the
       * number of outstanding bytes to reflect the newly acked bytes in
       * received SACK.
       */
      if(eNewCumAck == TRUE &&
	 spCurrDestNodeData->uiNumNewlyAckedBytes > 0 &&
	 spSackChunk->uiCumAck >= uiRecover)
	{
	  AdjustCwnd(spCurrDestNodeData);
	}

      // END -- NewReno changes to this function

      /* The number of outstanding bytes is reduced by how many bytes this sack 
       * acknowledges.
       */
      if(spCurrDestNodeData->uiNumNewlyAckedBytes <=
	 spCurrDestNodeData->uiOutstandingBytes)
	{
	  spCurrDestNodeData->uiOutstandingBytes 
	    -= spCurrDestNodeData->uiNumNewlyAckedBytes;
	}
      else
	spCurrDestNodeData->uiOutstandingBytes = 0;

      DBG_PL(ProcessSackChunk,"Dest #%d (%d:%d) (%p): outstanding=%d, cwnd=%d"),
	++i, spCurrDestNodeData->iNsAddr, spCurrDestNodeData->iNsPort,
	spCurrDestNodeData, spCurrDestNodeData->uiOutstandingBytes, 
	spCurrDestNodeData->uiCwnd DBG_PR;

      if(spCurrDestNodeData->uiOutstandingBytes == 0)
	{
	  /* All outstanding data has been acked
	   */
	  spCurrDestNodeData->uiPartialBytesAcked = 0;  // section 7.2.2

	  /* section 6.3.2.R2
	   */
	  if(spCurrDestNodeData->eRtxTimerIsRunning == TRUE)
	    {
	      DBG_PL(ProcessSackChunk, "Dest #%d (%p): stopping timer"), 
		i, spCurrDestNodeData DBG_PR;
	      StopT3RtxTimer(spCurrDestNodeData);
	    }
	}

      /* section 6.3.2.R3 - Restart timers for destinations that have
       * acknowledged their first outstanding (ie, no timer running) and
       * still have outstanding data in flight.  
       */
      if(spCurrDestNodeData->uiOutstandingBytes > 0 &&
	 spCurrDestNodeData->eRtxTimerIsRunning == FALSE)
	{
	  StartT3RtxTimer(spCurrDestNodeData);
	}
    }

  DBG_F(ProcessSackChunk, DumpSendBuffer());

  AdvancePeerAckPoint();

  if(eFastRtxNeeded == TRUE)  // section 7.2.4
    FastRtx();

  /* Let's see if after process this sack, there are still any chunks
   * pending... If so, rtx all allowed by cwnd.
   */
  else if( (eMarkedChunksPending = AnyMarkedChunks()) == TRUE)
    {
      /* section 6.1.C) When the time comes for the sender to
       * transmit, before sending new DATA chunks, the sender MUST
       * first transmit any outstanding DATA chunks which are marked
       * for retransmission (limited by the current cwnd).  
       */
      RtxMarkedChunks(RTX_LIMIT_CWND);
    }

  /* (6.2.1.D.ii) Adjust PeerRwnd based on total oustanding bytes on all
   * destinations. We need to this adjustment after any
   * retransmissions. Otherwise the sender's view of the peer rwnd will be
   * off, because the number outstanding increases again once a marked
   * chunk gets retransmitted (when marked, outstanding is decreased).
   */
  uiTotalOutstanding = TotalOutstanding();
  if(uiTotalOutstanding <= spSackChunk->uiArwnd)
    uiPeerRwnd = (spSackChunk->uiArwnd  - uiTotalOutstanding);
  else
    uiPeerRwnd = 0;
  
  DBG_PL(ProcessSackChunk, "uiPeerRwnd=%d, uiArwnd=%d"), uiPeerRwnd, 
    spSackChunk->uiArwnd DBG_PR;
  DBG_X(ProcessSackChunk);
}


