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

/* Timestamp extension adds a TIMESTAMP chunk into every packet with DATA
 * or SACK chunk(s). The timestamps allow RTT measurements to be made per
 * packet on both original transmissiosn and retransmissions. Thus, Karn's
 * algorithm is no longer needed.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp-timestamp.cc,v 1.6 2009/11/16 05:51:27 tom_henderson Exp $ (UD/PEL)";
#endif

#include "ip.h"
#include "sctp-timestamp.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "sctpDebug.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))

static class TimestampSctpClass : public TclClass 
{ 
public:
  TimestampSctpClass() : TclClass("Agent/SCTP/Timestamp") {}
  TclObject* create(int, const char*const*) 
  {
    return (new TimestampSctpAgent());
  }
} classSctpTimestamp;

TimestampSctpAgent::TimestampSctpAgent() : SctpAgent()
{
}

void TimestampSctpAgent::delay_bind_init_all()
{

  SctpAgent::delay_bind_init_all();
}

int TimestampSctpAgent::delay_bind_dispatch(const char *cpVarName, 
					    const char *cpLocalName, 
					    TclObject *opTracer)
{
  return SctpAgent::delay_bind_dispatch(cpVarName, cpLocalName, opTracer);
}

void TimestampSctpAgent::OptionReset()
{
  DBG_I(OptionReset);

  eNeedTimestampEcho = FALSE;

  DBG_X(OptionReset);
}

/* This function returns the reserved size needed for timestamp chunks.
 */
u_int TimestampSctpAgent::ControlChunkReservation()
{
  DBG_I(ControlChunkReservation);
  DBG_PL(ControlChunkReservation, "returning %d"), 
    sizeof(SctpTimestampChunk_S) DBG_PR;
  DBG_X(ControlChunkReservation);
  return sizeof(SctpTimestampChunk_S);
}

/* This function bundles control chunks with data chunks. We copy the timestamp
 * chunk into the outgoing packet and return the size of the timestamp chunk.
 */
int TimestampSctpAgent::BundleControlChunks(u_char *ucpOutData)
{
  DBG_I(BundleControlChunks);
  SctpTimestampChunk_S *spTimestampChunk = (SctpTimestampChunk_S *) ucpOutData;

  spTimestampChunk->sHdr.usLength = sizeof(SctpTimestampChunk_S);

  /* We need to send a timestamp in the timestamp chunk when sending data
   * chunks (and not sack or foward tsn chunks)... otherwise, we may only
   * need the timestamp echo to be set.
   */
  if((eSendNewDataChunks == TRUE || eMarkedChunksPending == TRUE) && 
     eSackChunkNeeded == FALSE  && 
     eForwardTsnNeeded == FALSE)
    {
      spTimestampChunk->sHdr.ucType = SCTP_CHUNK_TIMESTAMP;
      spTimestampChunk->sHdr.ucFlags |= SCTP_TIMESTAMP_FLAG_TS;
      spTimestampChunk->fTimestamp = (float) Scheduler::instance().clock();
    }
  if(eNeedTimestampEcho == TRUE)
    {
      spTimestampChunk->sHdr.ucType = SCTP_CHUNK_TIMESTAMP;
      spTimestampChunk->sHdr.ucFlags |= SCTP_TIMESTAMP_FLAG_ECHO;
      spTimestampChunk->fEcho = fOutTimestampEcho;
      eNeedTimestampEcho = FALSE;
    }

  DBG_PL(BundleControlChunks, "returning %d"), 
    spTimestampChunk->sHdr.usLength DBG_PR;
  DBG_X(BundleControlChunks);
  return spTimestampChunk->sHdr.usLength;
}

void TimestampSctpAgent::AddToSendBuffer(SctpDataChunkHdr_S *spChunk, 
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
void TimestampSctpAgent::SendBufferDequeueUpTo(u_int uiTsn)
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

/* This function goes through the entire send buffer filling a packet with 
 * chunks marked for retransmission. Once a packet is full (according to MTU)
 * it is transmittted. If the eLimit is one packet, than that is all that is
 * done. If the eLimit is cwnd, then packets full of marked tsns are sent until
 * cwnd is full.
 */
void TimestampSctpAgent::RtxMarkedChunks(SctpRtxLimit_E eLimit)
{
  DBG_I(RtxMarkedChunks);

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  u_char *ucpCurrOutData = ucpOutData;
  int iBundledControlChunkSize = 0;
  int iCurrSize = 0;
  int iOutDataSize = 0;
  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffNodeData = NULL;
  SctpDataChunkHdr_S  *spCurrChunk;
  SctpDest_S *spRtxDest = NULL;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;
  Boolean_E eControlChunkBundled = FALSE;
  int iNumPacketsSent = 0;

  memset(ucpOutData, 0, uiMaxPayloadSize);

  uiBurstLength = 0;

  /* make sure we clear all the spFirstOutstanding pointers before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->spFirstOutstanding = NULL;  // reset
    }

  /* We need to set the destination address for the retransmission(s). We assume
   * that on a given call to this function, all should all be sent to the same
   * address (should be a reasonable assumption). So, to determine the address,
   * we find the first marked chunk and determine the destination it was last 
   * sent to. 
   *
   * Also, we temporarily count all marked chunks as not outstanding. Why? Well,
   * if we try retransmitting on the same dest as used previously, the cwnd may
   * never let us retransmit because the outstanding is counting marked chunks
   * too. At the end of this function, we'll count all marked chunks as 
   * outstanding again. (ugh... there has to be a better way!)
   */
  for(spCurrBuffNode = sSendBuffer.spHead; 
      spCurrBuffNode != NULL;
      spCurrBuffNode = spCurrBuffNode->spNext)
    {
      spCurrBuffNodeData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;
	  
      if(spCurrBuffNodeData->eMarkedForRtx != NO_RTX)
	{
	  spCurrChunk = spCurrBuffNodeData->spChunk;

	  if(spRtxDest == NULL)
	    {
	      /* RFC2960 says that retransmissions should go to an
	       * alternate destination when available, which is the
	       * default behavior characterized by
	       * eRtxToAlt=RTX_TO_ALT_ON. 
	       *
	       * We add two experimental options:
	       *    1. rtx all data to same destination (RTX_TO_ALT_OFF)
	       *    2. rtx only timeouts to alt dest (RTX_TO_ALT_TIMEOUTS_ONLY)
	       *
	       * Note: Even with these options, if the same dest is inactive,
	       * then alt dest is used.
	       */
	      switch(eRtxToAlt)
		{
		case RTX_TO_ALT_OFF:
		  if(spCurrBuffNodeData->spDest->eStatus 
		     == SCTP_DEST_STATUS_ACTIVE)
		    {
		      spRtxDest = spCurrBuffNodeData->spDest;
		    }
		  else
		    {
		      spRtxDest = GetNextDest(spCurrBuffNodeData->spDest);
		    }
		  break;
		  
		case RTX_TO_ALT_ON:
		  spRtxDest = GetNextDest(spCurrBuffNodeData->spDest);
		  break;

		case RTX_TO_ALT_TIMEOUTS_ONLY:
		  if(spCurrBuffNodeData->eMarkedForRtx == FAST_RTX &&
		     spCurrBuffNodeData->spDest->eStatus 
		     == SCTP_DEST_STATUS_ACTIVE)
		    {
		      spRtxDest = spCurrBuffNodeData->spDest; 
		    }
		  else
		    {
		      spRtxDest = GetNextDest(spCurrBuffNodeData->spDest);
		    }
		  break;
		}
	    }

	  spCurrBuffNodeData->spDest->iOutstandingBytes
	    -= spCurrChunk->sHdr.usLength;
	}
    }

  spCurrBuffNode = sSendBuffer.spHead;

  while( (eLimit == RTX_LIMIT_ONE_PACKET && 
	  iNumPacketsSent < 1 && 
	  spCurrBuffNode != NULL) ||
	 (eLimit == RTX_LIMIT_CWND &&
	  spRtxDest->iOutstandingBytes < spRtxDest->iCwnd &&
	  spCurrBuffNode != NULL) )
    {
      DBG_PL(RtxMarkedChunks, 
	     "eLimit=%s pktsSent=%d out=%d cwnd=%d spCurrBuffNode=%p"),
	(eLimit == RTX_LIMIT_ONE_PACKET) ? "ONE_PACKET" : "CWND",
	iNumPacketsSent, spRtxDest->iOutstandingBytes, spRtxDest->iCwnd,
	spCurrBuffNode
	DBG_PR;
      
      /* section 7.2.4.3
       *
       * continue filling up the packet with chunks which are marked for
       * rtx. exit loop when we have either run out of chunks or the
       * packet is full.
       *
       * note: we assume at least one data chunk fits in the packet.  
       */
      for(eControlChunkBundled = FALSE; 
	  spCurrBuffNode != NULL; 
	  spCurrBuffNode = spCurrBuffNode->spNext)
	{
	  spCurrBuffNodeData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;
	  
	  /* is this chunk the first outstanding on its destination?
	   */
	    if(spCurrBuffNodeData->spDest->spFirstOutstanding == NULL &&
	     spCurrBuffNodeData->eGapAcked == FALSE &&
	     spCurrBuffNodeData->eAdvancedAcked == FALSE)
	    {
	      /* yes, it is the first!
	       */
	      spCurrBuffNodeData->spDest->spFirstOutstanding 
		= spCurrBuffNodeData;
	    }

	  /* Only retransmit the chunks which have been marked for rtx.
	   */
	  if(spCurrBuffNodeData->eMarkedForRtx != NO_RTX)
	    {
	      spCurrChunk = spCurrBuffNodeData->spChunk;

	      /* bundle the control chunk before any data chunks and only
	       * once per packet
	       */
	      if(eControlChunkBundled == FALSE)
		{
		  eControlChunkBundled = TRUE;
		  iBundledControlChunkSize =BundleControlChunks(ucpCurrOutData);
		  ucpCurrOutData += iBundledControlChunkSize;
		  iOutDataSize += iBundledControlChunkSize;
		}

	      /* can we fit this chunk into the packet without exceeding MTU?? 
	       */
  	      if((iOutDataSize + spCurrChunk->sHdr.usLength) 
		 > (int) uiMaxPayloadSize)
		  break;  // doesn't fit in packet... jump out of the for loop

	      /* If this chunk was being used to measure the RTT, stop using it.
	       */
	      if(spCurrBuffNodeData->spDest->eRtoPending == TRUE &&
		 spCurrBuffNodeData->dTxTimestamp > 0)
		{
		  spCurrBuffNodeData->dTxTimestamp = 0;
		  spCurrBuffNodeData->spDest->eRtoPending = FALSE;
		}

	      /* section 7.2.4.4 (condition 2) - is this the first
	       * outstanding for the destination and are there still
	       * outstanding bytes on the destination? if so, restart
	       * timer.  
	       */
	      if(spCurrBuffNodeData->spDest->spFirstOutstanding 
		 == spCurrBuffNodeData)
		{
		  if(spCurrBuffNodeData->spDest->iOutstandingBytes > 0)
		    StartT3RtxTimer(spCurrBuffNodeData->spDest);
		}

	      /* JRI, ALC - Bugfix 2004/01/21 - section 6.1 - Whenever a
	       * transmission or retransmission is made to any address, if
	       * the T3-rtx timer of that address is not currently
	       * running, the sender MUST start that timer.  If the timer
	       * for that address is already running, the sender MUST
	       * restart the timer if the earliest (i.e., lowest TSN)
	       * outstanding DATA chunk sent to that address is being
	       * retransmitted.  Otherwise, the data sender MUST NOT
	       * restart the timer.  
	       */
	      if(spRtxDest->spFirstOutstanding == NULL ||
		 spCurrChunk->uiTsn <
		 spRtxDest->spFirstOutstanding->spChunk->uiTsn)
		{
		  /* This chunk is now the first outstanding on spRtxDest.
		   */
		  spRtxDest->spFirstOutstanding = spCurrBuffNodeData;
		  StartT3RtxTimer(spRtxDest);
		}
	      
	      memcpy(ucpCurrOutData, spCurrChunk, spCurrChunk->sHdr.usLength);
	      iCurrSize = spCurrChunk->sHdr.usLength;

	      /* the chunk length field does not include the padded bytes,
	       * so we need to account for these extra bytes.
	       */
	      if( (iCurrSize % 4) != 0 ) 
		iCurrSize += 4 - (iCurrSize % 4);

	      ucpCurrOutData += iCurrSize;
	      iOutDataSize += iCurrSize;
	      spCurrBuffNodeData->spDest = spRtxDest;
	      spCurrBuffNodeData->iNumTxs++;
	      spCurrBuffNodeData->eMarkedForRtx = NO_RTX;
	      
	      // BEGIN -- Timestamp changes to this function
	      spCurrBuffNodeData->dTxTimestamp = Scheduler::instance().clock();
	      // END -- Timestamp changes to this function

	      /* fill in tracing fields too
	       */
	      spSctpTrace[uiNumChunks].eType = SCTP_CHUNK_DATA;
	      spSctpTrace[uiNumChunks].uiTsn = spCurrChunk->uiTsn;
	      spSctpTrace[uiNumChunks].usStreamId = spCurrChunk->usStreamId;
	      spSctpTrace[uiNumChunks].usStreamSeqNum 
		= spCurrChunk->usStreamSeqNum;
	      uiNumChunks++;

	      /* the chunk is now outstanding on the alternate destination
	       */
	      spCurrBuffNodeData->spDest->iOutstandingBytes
		+= spCurrChunk->sHdr.usLength;
	      uiPeerRwnd -= spCurrChunk->sHdr.usLength; // 6.2.1.B
	      DBG_PL(RtxMarkedChunks, "spDest->iOutstandingBytes=%d"), 
		spCurrBuffNodeData->spDest->iOutstandingBytes DBG_PR;

	      DBG_PL(RtxMarkedChunks, "TSN=%d"), spCurrChunk->uiTsn DBG_PR;
	    }
	  else if(spCurrBuffNodeData->eAdvancedAcked == TRUE)
	    {
	      if(spCurrBuffNodeData->spDest->spFirstOutstanding 
		 == spCurrBuffNodeData)
		{
		  /* This WAS considered the first outstanding chunk for
		   * the destination, then stop the timer if there are no
		   * outstanding chunks waiting behind this one in the
		   * send buffer.  However, if there ARE more outstanding
		   * chunks on this destination, we need to restart timer
		   * for those.
		   */
		  if(spCurrBuffNodeData->spDest->iOutstandingBytes > 0)
		    StartT3RtxTimer(spCurrBuffNodeData->spDest);
		  else
		    StopT3RtxTimer(spCurrBuffNodeData->spDest);
		}
	    }
	}

      /* Transmit the packet now...
       */
      if(iOutDataSize > 0)
	{
	  SendPacket(ucpOutData, iOutDataSize, spRtxDest);
	  if(spRtxDest->eRtxTimerIsRunning == FALSE)
	    StartT3RtxTimer(spRtxDest);
	  iNumPacketsSent++;	  
	  iOutDataSize = 0; // reset
	  ucpCurrOutData = ucpOutData; // reset
	  memset(ucpOutData, 0, uiMaxPayloadSize); // reset

	  spRtxDest->opCwndDegradeTimer->resched(spRtxDest->dRto);

	  /* This addresses the proposed change to RFC2960 section 7.2.4,
	   * regarding using of Max.Burst. We have an option which allows
	   * to control if Max.Burst is applied.
	   */
	  if(eUseMaxBurst == MAX_BURST_USAGE_ON)
	    if( (eApplyMaxBurst == TRUE) && (uiBurstLength++ >= MAX_BURST) )
	      {
		/* we've reached Max.Burst limit, so jump out of loop
		 */
		eApplyMaxBurst = FALSE; // reset before jumping out of loop
		break;
	      }
	}
    } 

  /* Ok, let's count all marked chunks as outstanding again. (ugh... there
   * has to be a better way!)  
   */
  for(spCurrBuffNode = sSendBuffer.spHead; 
      spCurrBuffNode != NULL;
      spCurrBuffNode = spCurrBuffNode->spNext)
    {
      spCurrBuffNodeData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;
	  
      if(spCurrBuffNodeData->eMarkedForRtx != NO_RTX)
	{
	  spCurrChunk = spCurrBuffNodeData->spChunk;
	  spCurrBuffNodeData->spDest->iOutstandingBytes
	    += spCurrChunk->sHdr.usLength;
	}
    }
    
  /* If we made it here, either our limit was only one packet worth of
   * retransmissions or we hit the end of the list and there are no more
   * marked chunks. If we didn't hit the end, let's see if there are more marked
   * chunks.
   */
  eMarkedChunksPending = AnyMarkedChunks();

  delete[] ucpOutData;

  DBG_X(RtxMarkedChunks);
}

/* returns a boolean of whether a fast retransmit is necessary
 */
Boolean_E TimestampSctpAgent::ProcessGapAckBlocks(u_char *ucpSackChunk,
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

	      /* HTNA algorithm... we need to know the highest TSN
	       * sacked (even if it isn't new), so that when the sender
	       * is in Fast Recovery, the outstanding tsns beyond the 
	       * last sack tsn do not have their missing reports incremented
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
		  if(spCurrNodeData->spDest->iErrorCount != 0 &&
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
	      /* HTNA (Highest TSN Newly Acked) algorithm from
	       * implementer's guide. The HTNA increments missing reports
	       * for TSNs not GapAcked when one of the following
	       * conditions hold true:
	       *
	       *    1. The TSN is less than the highest TSN newly acked.
	       *
	       *    2. The TSN is less than the highest TSN sacked so far
	       *    (not necessarily newly acked), the sender is in Fast
	       *    Recovery, the cum ack changes, and the new cum ack is less
	       *    than recover.
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

		  if(spCurrNodeData->iNumMissingReports >= iFastRtxTrigger &&
		     spCurrNodeData->eIneligibleForFastRtx == FALSE &&
		     spCurrNodeData->eAdvancedAcked == FALSE)
		    {
		      MarkChunkForRtx(spCurrNodeData, FAST_RTX);
		      eFastRtxNeeded = TRUE;
		      spCurrNodeData->eIneligibleForFastRtx = TRUE;
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting eFastRtxNeeded = TRUE") DBG_PR;
		    }
		}
	    }
	}
    }

  if(eFastRtxNeeded == TRUE)
    tiFrCount++;

  DBG_PL(ProcessGapAckBlocks, "eFastRtxNeeded=%s"), 
    eFastRtxNeeded ? "TRUE" : "FALSE" DBG_PR;
  DBG_X(ProcessGapAckBlocks);
  return eFastRtxNeeded;
}

/* This function is left as a hook for extensions to process chunk types not
 * supported in the base sctp. Here, we can only expect the timestamp chunk.
 */
void TimestampSctpAgent::ProcessOptionChunk(u_char *ucpInChunk)
{
  DBG_I(ProcessOptionChunk);
  double dCurrTime = Scheduler::instance().clock();

  switch( ((SctpChunkHdr_S *)ucpInChunk)->ucType)
    {
    case SCTP_CHUNK_TIMESTAMP:
      ProcessTimestampChunk((SctpTimestampChunk_S *) ucpInChunk);
      break;
      
    default:
      DBG_PL(ProcessOptionChunk, "unexpected chunk type (unknown: %d) at %f"),
	((SctpChunkHdr_S *)ucpInChunk)->ucType, dCurrTime DBG_PR;
      printf("[ProcessOptionChunk] unexpected chunk type (unknown: %d) at %f\n",
	     ((SctpChunkHdr_S *)ucpInChunk)->ucType, dCurrTime);
      break;
    }

  DBG_X(ProcessOptionChunk);
}

void TimestampSctpAgent::ProcessTimestampChunk(SctpTimestampChunk_S 
					       *spTimestampChunk)
{
  DBG_I(ProcessTimestampChunk);

  /* Only echo the timestamp if there isn't already an echo ready to go. Since
   * the receiver may delay sacks and ack 2 data packets with one sack, we want
   * to echo the timestamp of the first data chunk that triggers the sack.
   */
  if(spTimestampChunk->sHdr.ucFlags & SCTP_TIMESTAMP_FLAG_TS)
    {
      if(eNeedTimestampEcho == FALSE)
	{
	  eNeedTimestampEcho = TRUE;
	  fOutTimestampEcho = spTimestampChunk->fTimestamp;
	  DBG_PL(ProcessTimestampChunk, "timestamp=%f ...echoing!"), 
	    spTimestampChunk->fTimestamp DBG_PR;
	}
      else
	{
	  DBG_PL(ProcessTimestampChunk, 
		 "timestamp=%f ...ignoring (already echoing one)"), 
	    spTimestampChunk->fTimestamp DBG_PR;
	}
    }

  if(spTimestampChunk->sHdr.ucFlags & SCTP_TIMESTAMP_FLAG_ECHO)
    {
      fInTimestampEcho = spTimestampChunk->fEcho;
      DBG_PL(ProcessTimestampChunk, "echo=%f"), 
	spTimestampChunk->fEcho DBG_PR;
    }

  DBG_X(ProcessTimestampChunk);
}
