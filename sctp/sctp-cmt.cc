/*
 * Copyright (c) 2006-2009 by the Protocol Engineering Lab, U of Delaware
 * All rights reserved.
 *
 * Protocol Engineering Lab web page : http://pel.cis.udel.edu/
 *
 * Paul D. Amer        <amer@@cis,udel,edu>
 * Armando L. Caro Jr. <acaro@@cis,udel,edu>
 * Janardhan Iyengar   <iyengar@@cis,udel,edu>
 * Preethi Natarajan   <nataraja@@cis,udel,edu>
 * Nasif Ekiz          <nekiz@@cis,udel,edu>
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

/* Concurrent Multipath Transfer extension: To allow SCTP to correctly use
 * the multiple paths available at the transport.
 */

#include "ip.h"
#include "sctp-cmt.h"
#include "sctp.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#include "sctpDebug.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))


static class SctpCMTClass : public TclClass 
{ 
public:
  SctpCMTClass() : TclClass("Agent/SCTP/CMT") {}
  TclObject* create(int, const char*const*) 
  {
    return (new SctpCMTAgent());
  }
} classSctpCMT;

SctpCMTAgent::SctpCMTAgent() : SctpAgent() {}

void SctpCMTAgent::delay_bind_init_all()
{
  delay_bind_init_one("useCmtReordering_");
  delay_bind_init_one("useCmtCwnd_");
  delay_bind_init_one("useCmtDelAck_");
  delay_bind_init_one("eCmtRtxPolicy_");
  delay_bind_init_one("useCmtPF_");
  delay_bind_init_one("cmtPFCwnd_");
  delay_bind_init_one("countPFToActiveNewData_");
  delay_bind_init_one("countPFToActiveRtxms_");
  //  delay_bind_init_one("useSharedCC_");
  SctpAgent::delay_bind_init_all();
}

int SctpCMTAgent::delay_bind_dispatch(const char *cpVarName, 
					  const char *cpLocalName, 
					  TclObject *opTracer)
{
  if(delay_bind(cpVarName, cpLocalName, "useCmtReordering_", 
		(int*)&eUseCmtReordering, opTracer))
    return TCL_OK;
  if(delay_bind(cpVarName, cpLocalName, "useCmtCwnd_", 
		(int*)&eUseCmtCwnd, opTracer))
    return TCL_OK;
  if(delay_bind(cpVarName, cpLocalName, "useCmtDelAck_", 
		(int*)&eUseCmtDelAck, opTracer))
    return TCL_OK;
  if(delay_bind(cpVarName, cpLocalName, "eCmtRtxPolicy_", 
		(int*)&eCmtRtxPolicy, opTracer))
    return TCL_OK;
  if(delay_bind(cpVarName, cpLocalName, "useCmtPF_", 
		(int*)&eUseCmtPF, opTracer))
    return TCL_OK;
  if(delay_bind(cpVarName, cpLocalName, "cmtPFCwnd_", 
		(u_int*)&uiCmtPFCwnd, opTracer))
    return TCL_OK;
  if(delay_bind(cpVarName, cpLocalName, "countPFToActiveNewData_", 
	&tiCountPFToActiveNewData, opTracer))
    return TCL_OK;
  if(delay_bind(cpVarName, cpLocalName, "countPFToActiveRtxms_", 
	&tiCountPFToActiveRtxms, opTracer))
    return TCL_OK;
  //   if(delay_bind(cpVarName, cpLocalName, "useSharedCC_", 
  // 		(int*)&eUseSharedCC, opTracer))
  //     return TCL_OK;
  else
    return SctpAgent::delay_bind_dispatch(cpVarName, 
					  cpLocalName, opTracer);
}

void SctpCMTAgent::TraceAll()
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
	      "assocErrors: %d pathErrors: %d dstatus: %s "
	      "frCount: %d timeoutCount: %d rcdCount: %d "
	      "countPFToActiveNewData: %d countPFToActiveRtxms: %d\n",
	      dCurrTime,
	      addr(), port(), spCurrDest->iNsAddr, spCurrDest->iNsPort,
	      spCurrDest->iCwnd, spCurrDest->iPartialBytesAcked, 
	      spCurrDest->iOutstandingBytes, spCurrDest->iSsthresh, 
	      uiPeerRwnd,
	      spCurrDest->dRto, spCurrDest->dSrtt, 
	      spCurrDest->dRttVar,
	      iAssocErrorCount,
	      spCurrDest->iErrorCount,
	      PrintDestStatus(spCurrDest),
	      int(tiFrCount),
	      spCurrDest->iTimeoutCount,
	      spCurrDest->iRcdCount,
	      int(tiCountPFToActiveNewData),
              int(tiCountPFToActiveRtxms));
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }

  sprintf(cpOutString, "\n");
  if(channel_)
    (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
}

void SctpCMTAgent::TraceVar(const char* cpVar)
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

  //   else if(!strcmp(cpVar, "rwnd_"))
  //     {
  //       sprintf(cpOutString, "time: %-8.5f rwnd: %d peerRwnd: %d\n", 
  // 	      dCurrTime, uiMyRwnd, uiPeerRwnd);
  //       if(channel_)
  //       	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
  //     }
  
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
		"rto: %-6.3f srtt: %-6.3f rttvar: %-6.3f timeoutCount: %d\n",
		dCurrTime, 
		addr(), port(), 
		spCurrDest->iNsAddr, spCurrDest->iNsPort,
		spCurrDest->dRto, spCurrDest->dSrtt, 
		spCurrDest->dRttVar, spCurrDest->iTimeoutCount);
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
		"assocErrors: %d pathErrors: %d dstatus: %d\n",
		dCurrTime, 
		addr(), port(), 
		spCurrDest->iNsAddr, spCurrDest->iNsPort,
		iAssocErrorCount,
		spCurrDest->iErrorCount,
		spCurrDest->eStatus);
	if(channel_)
	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
      }

  else if(!strcmp(cpVar, "frCount_"))
    {
      /* JRI-TODO: Check code in SCTP for frCount
       */
      sprintf(cpOutString, 
	      "time: %-8.5f  "
	      "frCount: %d\n",
	      dCurrTime, 
	      int(tiFrCount));
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }

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
  else if(!strcmp(cpVar, "countPFToActiveNewData_"))
    {
      sprintf(cpOutString,
	      "time: %-8.5f   "
	      "countPFToActiveNewData: %d\n",
	      dCurrTime, 
	      int(tiCountPFToActiveNewData));
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }
  else if(!strcmp(cpVar, "countPFToActiveRtxms_"))
    {
      sprintf(cpOutString,
	      "time: %-8.5f   "
	      "countPFToActiveRtxms: %d\n",
	      dCurrTime, 
	      int(tiCountPFToActiveRtxms));
	if(channel_)
	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
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

void SctpCMTAgent::trace(TracedVar *v)
{
  if(eTraceAll == TRUE)
	TraceAll();
  else
	TraceVar(v->name());
}

void SctpCMTAgent::OptionReset()
{
  DBG_I(OptionReset);
  SctpAgent::OptionReset();
  DBG_X(OptionReset);
}

void SctpCMTAgent::Reset() 
{
  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;

  /* Call parent's reset() first
   */
  SctpAgent::Reset();

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrDest = (SctpDest_S *) spCurrNode->vpData;

      spCurrDest->uiHighestTsnInSackForDest = 0;
      spCurrDest->uiLowestTsnInSackForDest = 0;
      spCurrDest->eSawNewAck = FALSE;
      spCurrDest->uiRecover = 0;
      spCurrDest->eFindExpectedPseudoCum = TRUE;
      spCurrDest->eFindExpectedRtxPseudoCum = TRUE;
      spCurrDest->eRtxLimit = RTX_LIMIT_ZERO;
    }

  tiCountPFToActiveNewData = 0;
  tiCountPFToActiveRtxms = 0;

}

/* returns the size of the chunk
 */
int SctpCMTAgent::GenChunk(SctpChunkType_E eType, u_char *ucpChunk)
{
  DBG_I(GenChunk);

  double dCurrTime = Scheduler::instance().clock();
  AppData_S *spAppMessage = NULL;
  int iSize = 0;

  /* NE: variables used for generation of disjoint Gap Ack and Non-Renegable 
   Gap Ack Blocks */
  u_short gapAckBlockStartTsn, gapAckBlockEndTsn; 
  u_short dividedGapAckBlockStartTsn;
  u_short nonRenegGapAckBlockStartTsn, nonRenegGapAckBlockEndTsn;
  Node_S *spCurrGapFrag, *spCurrNonRenegGapFrag;
  u_short usNumGapAckBlocks = 0;

  DBG_PL(GenChunk, "spSctpTrace=%p"), spSctpTrace DBG_PR;

  switch(eType)
    {
    case SCTP_CHUNK_INIT:
      iSize = sizeof(SctpInitChunk_S);
      ((SctpInitChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpInitChunk_S *) ucpChunk)->uiArwnd = uiInitialRwnd; 
      ((SctpInitChunk_S *) ucpChunk)->usNumOutboundStreams = uiNumOutStreams;
      ((SctpInitChunk_S *) ucpChunk)->uiInitialTsn = 0; 

      ((SctpInitChunk_S *) ucpChunk)->sUnrelStream.usType 
	= SCTP_INIT_PARAM_UNREL;

      ((SctpInitChunk_S *) ucpChunk)->sUnrelStream.usLength
	= sizeof(SctpUnrelStreamsParam_S);
      
      if(uiNumUnrelStreams > 0)
	{
	  ((SctpInitChunk_S *) ucpChunk)->sUnrelStream.usLength 
	    += sizeof(SctpUnrelStreamPair_S);

	  SctpUnrelStreamPair_S *spUnrelStreamPair 
            = (SctpUnrelStreamPair_S *) (ucpChunk+iSize);
          spUnrelStreamPair->usStart = 0;
          spUnrelStreamPair->usEnd = uiNumUnrelStreams - 1;

	  iSize += sizeof(SctpUnrelStreamPair_S);
 	}

      ((SctpInitChunk_S *) ucpChunk)->sHdr.usLength = iSize;

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = (u_int) -1;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;

    case SCTP_CHUNK_INIT_ACK:
      iSize = sizeof(SctpInitAckChunk_S);
      ((SctpInitAckChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpInitAckChunk_S *) ucpChunk)->uiArwnd = uiInitialRwnd; 
      ((SctpInitChunk_S *) ucpChunk)->usNumOutboundStreams = uiNumOutStreams;
      ((SctpInitAckChunk_S *) ucpChunk)->uiInitialTsn = 0; 

      ((SctpInitAckChunk_S *) ucpChunk)->sUnrelStream.usType 
	= SCTP_INIT_PARAM_UNREL;

      ((SctpInitAckChunk_S *) ucpChunk)->sUnrelStream.usLength
	= sizeof(SctpUnrelStreamsParam_S);

      if(uiNumUnrelStreams > 0)
	{
	  ((SctpInitAckChunk_S *) ucpChunk)->sUnrelStream.usLength 
	    += sizeof(SctpUnrelStreamPair_S);

	  SctpUnrelStreamPair_S *spUnrelStreamPair 
            = (SctpUnrelStreamPair_S *) (ucpChunk+iSize);
          spUnrelStreamPair->usStart = 0;
          spUnrelStreamPair->usEnd = uiNumUnrelStreams - 1;

	  iSize += sizeof(SctpUnrelStreamPair_S);
	}

      ((SctpInitAckChunk_S *) ucpChunk)->sHdr.usLength = iSize;

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = (u_int) -1;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;

    case SCTP_CHUNK_COOKIE_ECHO:
      iSize = sizeof(SctpCookieEchoChunk_S);
      ((SctpCookieEchoChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpCookieEchoChunk_S *) ucpChunk)->sHdr.usLength = iSize;

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = (u_int) -1;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;
      
    case SCTP_CHUNK_COOKIE_ACK:
      iSize = sizeof(SctpCookieAckChunk_S);
      ((SctpCookieAckChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpCookieAckChunk_S *) ucpChunk)->sHdr.usLength = iSize;

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = (u_int) -1;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;

    case SCTP_CHUNK_DATA:
      /* Depending on eDataSource, we either use chunk parameters 
       * from the app layer buffer, or use the defaults for infinite 
       * data generation.
       */
      if(eDataSource == DATA_SOURCE_APPLICATION)
	{
	  /* If DATA_SOURCE_APPLICATION, we have to get chunk parameters
	   * from the app layer buffer 
	   */
	  DBG_PL (GenChunk, "eDataSource=DATA_SOURCE_APPLICATION") DBG_PR; 
	  spAppMessage = (AppData_S *) (sAppLayerBuffer.spHead->vpData);

	  DBG_PL (GenChunk, "App Message ---------------->") DBG_PR;
	  DBG_PL (GenChunk, "usNumStreams: %d"), 
	    spAppMessage->usNumStreams DBG_PR;
	  DBG_PL (GenChunk, "usNumUnreliable: %d"), 
	    spAppMessage->usNumUnreliable DBG_PR;
	  DBG_PL (GenChunk, "uiNumBytes: %d"),
	    spAppMessage->uiNumBytes DBG_PR;
	  DBG_PL (GenChunk, "usStreamId: %d"), 
	    spAppMessage->usStreamId DBG_PR;
	  DBG_PL (GenChunk, "eUnordered: %s"), 
	    spAppMessage->eUnordered ? "TRUE" : "FALSE" DBG_PR;
	  DBG_PL (GenChunk, "usReliability: %d"), 
	    spAppMessage->usReliability DBG_PR;
	  DBG_PL (GenChunk, "App Message <----------------") DBG_PR;

	  iSize = spAppMessage->uiNumBytes + sizeof(SctpDataChunkHdr_S);
	  ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucType = SCTP_CHUNK_DATA;
	  ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.usLength = iSize;

	  /* DATA chunks must be padded to a 4 byte boundary (section 3.3.1)
	   * ...but the adjustment should happen after usLength field is set,
	   * because the field should represent the size before padding.
	   */
	  if( (iSize % 4) != 0)
	    iSize += 4 - (iSize % 4);

	  ((SctpDataChunkHdr_S *) ucpChunk)->uiTsn = ++uiNextTsn;
	  
	  ((SctpDataChunkHdr_S *) ucpChunk)->usStreamId 
	    = spAppMessage->usStreamId; 

	  if(spAppMessage->eUnordered == TRUE)
	    {
	      ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucFlags 
		|= SCTP_DATA_FLAG_UNORDERED;

	      /* no stream seq num on unordered chunks!
	       */
	    }
	  else
	    {
	      ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucFlags = 0;
	      ((SctpDataChunkHdr_S *) ucpChunk)->usStreamSeqNum 
		= spOutStreams[spAppMessage->usStreamId].usNextStreamSeqNum++;
	    }
	}
      else
	{
	  DBG_PL (GenChunk, "eDataSource=DATA_SOURCE_INFINITE") DBG_PR; 

	  iSize = uiDataChunkSize;
	  ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucType = SCTP_CHUNK_DATA;

	  if(eUnordered == TRUE)
	    {
	      ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucFlags 
		|= SCTP_DATA_FLAG_UNORDERED;
	    }
	  else
	    {
	      ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucFlags = 0;
	    }

	  ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.usLength = iSize;

	  /* DATA chunks must be padded to a 4 byte boundary (section 3.3.1)
	   * ...but the adjustment should happen after usLength field is set,
	   * because the field should represent the size before padding.
	   */
	  if( (uiDataChunkSize % 4) != 0)
	    iSize += 4 - (uiDataChunkSize % 4);

	  ((SctpDataChunkHdr_S *) ucpChunk)->uiTsn = ++uiNextTsn;
	  
	  ((SctpDataChunkHdr_S *) ucpChunk)->usStreamId 
	    = (usNextStreamId % uiNumOutStreams); 

	  if(eUnordered == TRUE)
	    {
	      ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucFlags 
		|= SCTP_DATA_FLAG_UNORDERED;

	      /* no stream seq num on unordered chunks!
	       */
	    }
	  else
	    {
	      ((SctpDataChunkHdr_S *) ucpChunk)->sHdr.ucFlags = 0;
	      ((SctpDataChunkHdr_S *) ucpChunk)->usStreamSeqNum 
		= spOutStreams[usNextStreamId%uiNumOutStreams].usNextStreamSeqNum++;
	    }
	  
	  usNextStreamId++; // round-robin stream feeding
	}

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn 
	= ((SctpDataChunkHdr_S *) ucpChunk)->uiTsn;
      spSctpTrace[uiNumChunks].usStreamId 
	= ((SctpDataChunkHdr_S *) ucpChunk)->usStreamId;
      spSctpTrace[uiNumChunks].usStreamSeqNum 
	= ((SctpDataChunkHdr_S *) ucpChunk)->usStreamSeqNum;

      DBG_PL(GenChunk, "DataTsn=%d at dCurrTime=%f"), 
	uiNextTsn, dCurrTime DBG_PR;
      break;

    case SCTP_CHUNK_SACK:
      iSize = sizeof(SctpSackChunk_S);
      ((SctpSackChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpSackChunk_S *) ucpChunk)->uiCumAck = uiCumAck;
      ((SctpSackChunk_S *) ucpChunk)->uiArwnd = uiMyRwnd;
      ((SctpSackChunk_S *) ucpChunk)->usNumGapAckBlocks 
	= sRecvTsnBlockList.uiLength;
      ((SctpSackChunk_S *) ucpChunk)->usNumDupTsns = sDupTsnList.uiLength;

      /****** Begin CMT change ******/

      /* The SACK chunk flags are unused in RFC2960. We propose to use 1
       * of the 8 bits in the SACK flags for the CMT delayed ack algo.
       * These bits will be used to indicate the number of DATA packets
       * were received between the previous and the current SACK. This 
       * information will be used by the DATA sender to increment missing
       * reports better in CMT.
       */
      ((SctpSackChunk_S*)ucpChunk)->sHdr.ucFlags = iDataPktCountSinceLastSack;

      DBG_PL(GenChunk, "SACK CumAck=%d arwnd=%d flags=%d"), 
	uiCumAck, uiMyRwnd, iDataPktCountSinceLastSack DBG_PR;
      
      /* Pkt count is put into SACK flags for CMT delayed ack algo. 
       * Reset only after putting into SACK flags.
       */
      iDataPktCountSinceLastSack = 0; // reset

      /****** End CMT change ******/

      /* Append all the Gap Ack Blocks
       */
      for(Node_S *spCurrFrag = sRecvTsnBlockList.spHead;
	  (spCurrFrag != NULL) &&
	    (iSize + sizeof(SctpGapAckBlock_S) < uiMaxDataSize); 
	  spCurrFrag = spCurrFrag->spNext, iSize += sizeof(SctpGapAckBlock_S) )
	{
	  SctpGapAckBlock_S *spGapAckBlock 
	    = (SctpGapAckBlock_S *) (ucpChunk+iSize);

	  spGapAckBlock->usStartOffset 
	    = ((SctpRecvTsnBlock_S *)spCurrFrag->vpData)->uiStartTsn-uiCumAck;

	  spGapAckBlock->usEndOffset
	    = ((SctpRecvTsnBlock_S *)spCurrFrag->vpData)->uiEndTsn - uiCumAck;

	  DBG_PL(GenChunk, "GapAckBlock StartOffset=%d EndOffset=%d"), 
	    spGapAckBlock->usStartOffset, spGapAckBlock->usEndOffset DBG_PR;
	}

      /* Append all the Duplicate TSNs
       */
      for(Node_S *spPrevDup = NULL, *spCurrDup = sDupTsnList.spHead;
	  (spCurrDup != NULL) &&
	    (iSize + sizeof(SctpDupTsn_S) < uiMaxDataSize); 
	  spPrevDup = spCurrDup, spCurrDup = spCurrDup->spNext, 
	    DeleteNode(&sDupTsnList, spPrevDup), iSize += sizeof(SctpDupTsn_S))
	{
	  SctpDupTsn_S *spDupTsn = (SctpDupTsn_S *) (ucpChunk+iSize);
	  
	  spDupTsn->uiTsn = ((SctpDupTsn_S *) spCurrDup->vpData)->uiTsn;

	  DBG_PL(GenChunk, "DupTsn=%d"), spDupTsn->uiTsn DBG_PR;
	}

      /* After all the dynamic appending, we can NOW fill in the chunk size!
       */
      ((SctpSackChunk_S *) ucpChunk)->sHdr.usLength = iSize;

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = ((SctpSackChunk_S *)ucpChunk)->uiCumAck;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;


    /* PN: 12/20/2007, NR-SACKs
     */
    case SCTP_CHUNK_NRSACK:
	iSize = sizeof(SctpNonRenegSackChunk_S);
      ((SctpNonRenegSackChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->uiCumAck = uiCumAck;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->uiArwnd = uiMyRwnd;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumGapAckBlocks = 0; 
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumNonRenegSackBlocks
	= sNonRenegTsnBlockList.uiLength;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumDupTsns = sDupTsnList.uiLength;

      /****** Begin CMT change ******/

	/* PN: 12/2007. NR-SACKs.
	 * This CMT change for SCTP_CHUNK_SACK case is adapted for NR-SACKs. 
	 */

      /* The SACK chunk flags are unused in RFC2960. We propose to use 1
       * of the 8 bits in the SACK flags for the CMT delayed ack algo.
       * These bits will be used to indicate the number of DATA packets
       * were received between the previous and the current SACK. This 
       * information will be used by the DATA sender to increment missing
       * reports better in CMT.
       */
      ((SctpSackChunk_S*)ucpChunk)->sHdr.ucFlags = iDataPktCountSinceLastSack;
  
      DBG_PL(GenChunk, "NR-SACK: CumAck=%d arwnd=%d dCurrTime=%f"), 
	uiCumAck, uiMyRwnd, dCurrTime  DBG_PR;
      
      /* Pkt count is put into SACK flags for CMT delayed ack algo. 
       * Reset only after putting into SACK flags.
       */
      iDataPktCountSinceLastSack = 0; // reset

      /****** End CMT change ******/

      /* Append all the Gap Ack Blocks
       */
      /* NE: let's generate disjoint Gap Ack Blocks and Non-Renegable Gap Ack Blocks */
      spCurrGapFrag = sRecvTsnBlockList.spHead; 
      spCurrNonRenegGapFrag = sNonRenegTsnBlockList.spHead;
      
      while (spCurrGapFrag != NULL && (iSize + sizeof(SctpGapAckBlock_S) < uiMaxDataSize)) 
	{  	  
	  /* NE: current Gap Ack Block */
	  gapAckBlockStartTsn = ((SctpRecvTsnBlock_S *)spCurrGapFrag->vpData)->uiStartTsn;
	  gapAckBlockEndTsn = ((SctpRecvTsnBlock_S *)spCurrGapFrag->vpData)->uiEndTsn;
	  
	  DBG_PL(GenChunk, "Current GapAckBlock StartTSN=%d EndTSN=%d"), 
	    gapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
	  
	  /* NE: check if still there are some Non-Renegable Gap Ack Blocks */
	  if (spCurrNonRenegGapFrag != NULL) {
	    
	    /* NE: current Non-Renegable Gap Ack Block */
            nonRenegGapAckBlockStartTsn = ((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiStartTsn;
	    nonRenegGapAckBlockEndTsn = ((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiEndTsn;
	    
	    DBG_PL(GenChunk, "Current NonRenegGapAckBlock StartTSN=%d EndTSN=%d"), 
	      nonRenegGapAckBlockStartTsn, nonRenegGapAckBlockEndTsn DBG_PR;
	    
	    /* NE: current Gap Ack Block does not contain any Non-Renegable TSNs */    
            if (gapAckBlockEndTsn < nonRenegGapAckBlockStartTsn) {
        
	      /* add the Gap Ack Block to the NR-SACK chunk and advance to the next
		 Gap Ack Block */
              DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
		gapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
	      
	      usNumGapAckBlocks++; 
	      
	      DBG_PL(GenChunk, 
		     "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
		iSize, usNumGapAckBlocks DBG_PR;
	      DBG_PL(GenChunk, 
		     "RenegGapBlock StartOffset=%d EndOffset=%d"), 
		(gapAckBlockStartTsn - uiCumAck), (gapAckBlockEndTsn - uiCumAck) DBG_PR;

	      SctpGapAckBlock_S *spGapAckBlock = (SctpGapAckBlock_S *) (ucpChunk+iSize);
	      
	      spGapAckBlock->usStartOffset = gapAckBlockStartTsn - uiCumAck;
	      spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
	      iSize += sizeof(SctpGapAckBlock_S);
	      
	    } /* NE: current Gap Ack Block contains some Non-Renegable TSNs */
	    else if ((gapAckBlockStartTsn <= nonRenegGapAckBlockStartTsn) && 
		     (nonRenegGapAckBlockStartTsn <= gapAckBlockEndTsn)) {
	      
	      /* NE: current Gap Ack Blocks all TSNs are Non-Renegable, 
		 so let's skip (do not add to NR-SACK chunk) the current Gap Ack Block */
     	      if ((gapAckBlockStartTsn == nonRenegGapAckBlockStartTsn) && 
		  (gapAckBlockEndTsn == nonRenegGapAckBlockEndTsn)) {
		DBG_PL(GenChunk, "GapAckBlock Skipping StartTSN=%d EndTSN=%d"), 
		  gapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
		
		spCurrGapFrag = spCurrGapFrag->spNext;
		spCurrNonRenegGapFrag = spCurrNonRenegGapFrag->spNext;
		continue;
	      }
	      else {   /* NE: let's split the current Gap Ack Block into some number of
			  Gap Ack Blocks and Non-Renegable Gap Ack Blocks which are disjoint */
		
		dividedGapAckBlockStartTsn = gapAckBlockStartTsn;
		
		/* NE: current Gap Ack Block has some Non-Renegable TSNs in it */
		while (dividedGapAckBlockStartTsn <= gapAckBlockEndTsn && 
		       (iSize + sizeof(SctpGapAckBlock_S) < uiMaxDataSize)) {
		  
		  /* NE: First let's check if the current Non-Renegable Gap Ack Block is part of the 
		     current Gap Ack Block. If not add the remaining part of the Gap Ack Block to 
		     NR-SACK chunk */
		  if (spCurrNonRenegGapFrag == NULL || nonRenegGapAckBlockStartTsn > gapAckBlockEndTsn) {
		    DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
		      dividedGapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
		    
		    usNumGapAckBlocks++;
		    
		    /* NE: add the Gap Ack Block to the NR-SACK chunk */
		    DBG_PL(GenChunk, 
			   "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
		      iSize, usNumGapAckBlocks DBG_PR;
		    DBG_PL(GenChunk, 
			   "RenegGapBlock StartOffset=%d EndOffset=%d"), 
		      (dividedGapAckBlockStartTsn - uiCumAck), (gapAckBlockEndTsn - uiCumAck) DBG_PR;
		    
		    SctpGapAckBlock_S *spGapAckBlock = (SctpGapAckBlock_S *) (ucpChunk+iSize);
	      
		    spGapAckBlock->usStartOffset = dividedGapAckBlockStartTsn - uiCumAck;
		    spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
		    iSize += sizeof(SctpGapAckBlock_S);
		    
		    break;
		  }
		  
		  /* NE: Non-Renegable Gap Ack and Gap Ack Block blocks left edges are same so let's
		     skip the Non-Renegable TSNs from the Gap Ack Block and continue with
		     the remainder TSNs, also advance to the next Non-Renegable Gap Ack Block */
		  if (dividedGapAckBlockStartTsn == nonRenegGapAckBlockStartTsn) {
		    
		    DBG_PL(GenChunk, "GapAckBlock Skipping StartTSN=%d EndTSN=%d"), 
		      nonRenegGapAckBlockStartTsn, nonRenegGapAckBlockEndTsn DBG_PR;
		    
		    dividedGapAckBlockStartTsn = nonRenegGapAckBlockEndTsn + 1;
		    
		    spCurrNonRenegGapFrag = spCurrNonRenegGapFrag->spNext;
		    
		    /* NE: If there is no more Non-Renegable Gap Ack Blocks left, add the remaining 
		       TSNs to the as Gap Ack Block and exit loop */
		    if (spCurrNonRenegGapFrag == NULL) {
		      if (dividedGapAckBlockStartTsn <= gapAckBlockEndTsn) {
			DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
			  dividedGapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
			
			usNumGapAckBlocks++;
			
			/* NE: add the Gap Ack Block to the NR-SACK chunk */
			DBG_PL(GenChunk, 
			       "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
			  iSize, usNumGapAckBlocks DBG_PR;
			DBG_PL(GenChunk, 
			       "RenegGapBlock StartOffset=%d EndOffset=%d"), 
			  (dividedGapAckBlockStartTsn - uiCumAck), (gapAckBlockEndTsn - uiCumAck) DBG_PR;
			
			SctpGapAckBlock_S *spGapAckBlock = (SctpGapAckBlock_S *) (ucpChunk+iSize);
			
			spGapAckBlock->usStartOffset = dividedGapAckBlockStartTsn - uiCumAck;
			spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
			iSize += sizeof(SctpGapAckBlock_S);
		      }
		      break;
		    }
		    else {
		      /* NE: current Non-Renegable Gap Ack Block */
		      nonRenegGapAckBlockStartTsn = ((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiStartTsn;
		      nonRenegGapAckBlockEndTsn = ((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiEndTsn;
		      
		      DBG_PL(GenChunk, "Current NonRenegGapAckBlock StartTSN=%d EndTSN=%d"), 
			nonRenegGapAckBlockStartTsn, nonRenegGapAckBlockEndTsn DBG_PR;
		      
		      continue;
		    }
		    
		  } /* NE: now let's add the disjoint Gap Ack Block to NR-SACK chunk */
		  else if (spCurrNonRenegGapFrag != NULL && 
			   dividedGapAckBlockStartTsn < nonRenegGapAckBlockStartTsn) {
		    
		    DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
		      dividedGapAckBlockStartTsn, (nonRenegGapAckBlockStartTsn - 1) DBG_PR;
		    
		    usNumGapAckBlocks++;

		    /* NE: add the Gap Ack Block to the NR-SACK chunk */
		    DBG_PL(GenChunk, 
			   "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
		      iSize, usNumGapAckBlocks DBG_PR;
		    DBG_PL(GenChunk, 
			   "RenegGapBlock StartOffset=%d EndOffset=%d"), 
		      (dividedGapAckBlockStartTsn - uiCumAck), (nonRenegGapAckBlockStartTsn - 1 - uiCumAck) DBG_PR;
		    
		    SctpGapAckBlock_S *spGapAckBlock = (SctpGapAckBlock_S *) (ucpChunk+iSize);
		    
		    spGapAckBlock->usStartOffset = dividedGapAckBlockStartTsn - uiCumAck;
		    spGapAckBlock->usEndOffset = (nonRenegGapAckBlockStartTsn-1) - uiCumAck;
		    iSize += sizeof(SctpGapAckBlock_S);
		    
		    dividedGapAckBlockStartTsn = nonRenegGapAckBlockStartTsn;
		  }
		}		
	      }
	    }
	    else {
	      DBG_PL(GenChunk, 
		     "WARNING! This statement should not be reached!") DBG_PR;
	    }
	  }
	  else { /* NE: No more Non-Renegable Gap Ack Blocks exists. So let's add all 
		    remaining TSNs as Gap Ack Blocks. Let's add the Gap Ack Block to 
		    the NR-SACK chunk and advance to the next Gap Ack Block */
	    DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
	      gapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
	    
	    usNumGapAckBlocks++;
	    
	    /* NE: add the Gap Ack Block to the NR-SACK chunk */
	    DBG_PL(GenChunk, 
		   "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
	      iSize, usNumGapAckBlocks DBG_PR;
	    DBG_PL(GenChunk, 
		   "RenegGapBlock StartOffset=%d EndOffset=%d"), 
	      (gapAckBlockStartTsn - uiCumAck), (gapAckBlockEndTsn - uiCumAck) DBG_PR;
	    
	    SctpGapAckBlock_S *spGapAckBlock = (SctpGapAckBlock_S *) (ucpChunk+iSize);
	    
	    spGapAckBlock->usStartOffset = gapAckBlockStartTsn - uiCumAck;
	    spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
	    iSize += sizeof(SctpGapAckBlock_S);
	  }

	  /* NE: move to the next Gap Ack Block */
	  spCurrGapFrag = spCurrGapFrag->spNext;
	}
      
      DBG_PL(GenChunk, "usNumSackBlocks=%u"), usNumGapAckBlocks DBG_PR;
      
      /*NE: let's set the correct number of gap-ack blocks */
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumGapAckBlocks = usNumGapAckBlocks;
      
      /* PN: 5/2007. NR-Sacks */
      /* Append all the NR-Sack Blocks
       */
      
      DBG_PL(GenChunk, 
	     "NonRenegSackBlock StartOffset=%d usNumNonRenegSackBlocks=%d"), 
	iSize, sNonRenegTsnBlockList.uiLength DBG_PR;
      
      for(Node_S *spCurrFrag = sNonRenegTsnBlockList.spHead;
	  (spCurrFrag != NULL) &&
	    (iSize + sizeof(SctpGapAckBlock_S) < uiMaxDataSize); 
	  spCurrFrag = spCurrFrag->spNext, iSize += sizeof(SctpGapAckBlock_S) )
	{
	  SctpGapAckBlock_S *spGapAckBlock 
	    = (SctpGapAckBlock_S *) (ucpChunk+iSize);
	  
	  spGapAckBlock->usStartOffset 
	    = ((SctpRecvTsnBlock_S *)spCurrFrag->vpData)->uiStartTsn - uiCumAck;
	  
	  spGapAckBlock->usEndOffset
	    = ((SctpRecvTsnBlock_S *)spCurrFrag->vpData)->uiEndTsn - uiCumAck;
	  
	  DBG_PL(GenChunk, "NonRenegGapBlock StartOffset=%d EndOffset=%d"), 
	    spGapAckBlock->usStartOffset, spGapAckBlock->usEndOffset DBG_PR;
	}
      
      /* Append all the Duplicate TSNs
       */
      for(Node_S *spPrevDup = NULL, *spCurrDup = sDupTsnList.spHead;
	  (spCurrDup != NULL) &&
	    (iSize + sizeof(SctpDupTsn_S) < uiMaxDataSize); 
	  spPrevDup = spCurrDup, spCurrDup = spCurrDup->spNext, 
	    DeleteNode(&sDupTsnList, spPrevDup), iSize += sizeof(SctpDupTsn_S) )
	{
	  SctpDupTsn_S *spDupTsn = (SctpDupTsn_S *) (ucpChunk+iSize);

	  spDupTsn->uiTsn = ((SctpDupTsn_S *) spCurrDup->vpData)->uiTsn;
	  
	  DBG_PL(GenChunk, "DupTsn=%d"), spDupTsn->uiTsn DBG_PR;
	}

      /* After all the dynamic appending, we can NOW fill in the chunk size!
       */
      ((SctpNonRenegSackChunk_S *) ucpChunk)->sHdr.usLength = iSize;
      DBG_PL(GenChunk, "NR-SackChunkSize=%d"), iSize DBG_PR;

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = ((SctpSackChunk_S *)ucpChunk)->uiCumAck;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;


    case SCTP_CHUNK_FORWARD_TSN:
      iSize = SCTP_CHUNK_FORWARD_TSN_LENGTH;
      ((SctpForwardTsnChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpForwardTsnChunk_S *) ucpChunk)->sHdr.ucFlags = 0; //flags not used?
      ((SctpForwardTsnChunk_S *) ucpChunk)->sHdr.usLength = iSize;
      ((SctpForwardTsnChunk_S *) ucpChunk)->uiNewCum = uiAdvancedPeerAckPoint;

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = uiAdvancedPeerAckPoint;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;

    case SCTP_CHUNK_HB:     
    case SCTP_CHUNK_HB_ACK:
      iSize = SCTP_CHUNK_HEARTBEAT_LENGTH;
      ((SctpHeartbeatChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpHeartbeatChunk_S *) ucpChunk)->sHdr.ucFlags = 0;
      ((SctpHeartbeatChunk_S *) ucpChunk)->sHdr.usLength = iSize;
      ((SctpHeartbeatChunk_S *) ucpChunk)->usInfoType = 1;
      ((SctpHeartbeatChunk_S *) ucpChunk)->usInfoLength 
	= iSize - sizeof(SctpChunkHdr_S);
      ((SctpHeartbeatChunk_S *) ucpChunk)->dTimestamp = dCurrTime;
      ((SctpHeartbeatChunk_S *) ucpChunk)->spDest = NULL; // caller sets dest

      /* fill in tracing fields too
       */
      spSctpTrace[uiNumChunks].eType = eType;
      spSctpTrace[uiNumChunks].uiTsn = (u_int) -1;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;

    case SCTP_CHUNK_SHUTDOWN:
      break;

    case SCTP_CHUNK_SHUTDOWN_ACK:
      break;

    case SCTP_CHUNK_SHUTDOWN_COMPLETE:
      break;

    default:
      fprintf(stderr, "[GenChunk] ERROR: bad chunk type!\n");
      DBG_PL(GenChunk, "ERROR: bad chunk type!") DBG_PR;
      DBG_PL(GenChunk, "exiting...") DBG_PR;
      exit(-1);
    }

  uiNumChunks++;

  DBG_X(GenChunk);
  return iSize;
}


void SctpCMTAgent::AddToSendBuffer(SctpDataChunkHdr_S *spChunk, 
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

  /*** Begin CMT change ***/
  /* Maintain timestamp for each TSN to use for rtx decision after RTO
   * (RTT heuristic). Orig SCTP uses existence of a non-zero timestamp for
   * indication of whether this TSN is being used for RTT estimate or
   * not. Since that cannot be used anymore with this CMT change, we
   * introduce a new per-TSN variable eMeasuringRtt which is TRUE is this
   * TSN is being used, and FALSE otherwise.
   */

  spNewNodeData->dTxTimestamp = Scheduler::instance().clock();

  /* Is there already a DATA chunk in flight measuring an RTT? 
   * (6.3.1.C4 RTT measured once per round trip)
   */
  if(spDest->eRtoPending == FALSE)  // NO?
    {
      spNewNodeData->eMeasuringRtt = TRUE;
      spDest->eRtoPending = TRUE;   // ...well now there is :-)
    }
  else
    spNewNodeData->eMeasuringRtt = FALSE; // don't use this TSN for RTT est

  /*** End of CMT change ***/

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

void SctpCMTAgent::SendBufferDequeueUpTo(u_int uiTsn)
{
  DBG_I(SendBufferDequeueUpTo);

  Node_S *spDeleteNode = NULL;
  Node_S *spCurrNode = sSendBuffer.spHead;
  SctpSendBufferNode_S *spCurrNodeData = NULL;

  /* PN: 12/20/2007. Simulate send window */
  u_short usChunkSize = 0;
  char cpOutString[500];

  iAssocErrorCount = 0;

  DBG_PL(SendBufferDequeueUpTo, "uiTsn=%ld"), uiTsn DBG_PR;

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

	  /****** Begin CMT Change ******/
	  /* DAC algo: 
	   * Update lowest TSN seen in current SACK (per destination) 
	   * if appropriate. The lowest TSN is initialized to zero,
	   * and since we process cum and gap TSNs in TSN order, 
	   * the first value this variable gets set to will continue to be
	   * the lowest TSN acked. uiLowestTsnInSackForDest is used for
	   * missing report increments with delayed SACKs in CMT.
	   */
	  if (spCurrNodeData->spDest->uiLowestTsnInSackForDest == 0)
	    {
	      spCurrNodeData->spDest->uiLowestTsnInSackForDest =
		spCurrNodeData->spChunk->uiTsn;
	    }

	  /* CUC algo: Since this tsn has NOT been gapacked 
	   * previously, the pseudo-cum for this dest must be <= tsn.
	   * :. Set eFindExpectedPseudoCum to TRUE for destination 
	   * being cum acked. In gap ack processing, we'll start 
	   * searching for new expected-pseudo-cum.
	   */
	  spCurrNodeData->spDest->eFindExpectedPseudoCum = TRUE;
	  spCurrNodeData->spDest->eFindExpectedRtxPseudoCum = TRUE;
	    
	  /* CUC algo: Set eNewPseudoCum to TRUE to trigger cwnd adjustment.
	   * The cwnd should be adjusted only if new-pseudo-cum exists;
	   * otherwise this TSN would have already contributed to the cwnd.
	   * If this TSN had been gap acked previously, then it cannot be 
	   * the current expected-pseudo-cum.. and hence, the TSN cannot 
	   * contribute as a new-pseudo-cum. Here, we know that the TSN
	   * has not been gap acked before.
	   */
	  spCurrNodeData->spDest->eNewPseudoCum = TRUE;
	  
	  /****** End CMT Change ******/

	  /* only add to partial bytes acked if we are in congestion
	   * avoidance mode and if there was cwnd amount of data
	   * outstanding on the destination (implementor's guide) 
	   */
	  if(spCurrNodeData->spDest->iCwnd >spCurrNodeData->spDest->iSsthresh 
	     &&
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

      /****** Begin CMT Change ******/
      /* We update the RTT estimate if the following hold true:
       *   1. RTO pending flag is set (6.3.1.C4 measured once per round trip)
       *   2. We were measuring RTT with this chunk
       *   3. This chunk has not been retransmitted
       *   4. This chunk has not been gap acked already 
       *   5. This chunk has not been advanced acked (pr-sctp: exhausted rtxs)
       */
      if(spCurrNodeData->spDest->eRtoPending == TRUE &&
	 spCurrNodeData->eMeasuringRtt == TRUE &&
	 spCurrNodeData->iNumTxs == 1 &&
	 spCurrNodeData->eGapAcked == FALSE &&
	 spCurrNodeData->eAdvancedAcked == FALSE) 
	{
	  /* If the chunk is marked for timeout rtx, then the sender is an 
	   * ambigious state. Were the sacks lost or was there a failure? 
	   * (See below, where we talk about error count resett1ing)
	   * Since we don't clear the error counter below, we also don't
	   * update the RTT. This could be a problem for late arriving SACKs.
	   */
	  if(spCurrNodeData->eMarkedForRtx != TIMEOUT_RTX)
	    RttUpdate(spCurrNodeData->dTxTimestamp, spCurrNodeData->spDest);
	  spCurrNodeData->spDest->eRtoPending = FALSE;
	}

      /* if there is a timer running on the chunk's destination, then stop it
       */
      /* CMT: Added check for expectedPseudoCum.Before this change, even if the
       * timer for a dest was not running on any of the TSNs being dequeued
       * (because the pseudoCum had moved way further already), the timer
       * was being restarted due to the reset here. This caused timeouts
       * to get delayed, and timeout recovery got longer.
       */
      if(((spCurrNodeData->spDest->eRtxTimerIsRunning == TRUE) &&
	 (spCurrNodeData->spDest->uiExpectedPseudoCum <=
	  spCurrNodeData->spChunk->uiTsn)) ||
	 (spCurrNodeData->spDest->uiExpectedRtxPseudoCum <=
	  spCurrNodeData->spChunk->uiTsn))
       {
	StopT3RtxTimer(spCurrNodeData->spDest);
       }

      /****** End CMT Change ******/
 
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
	  
	  /* If running CMT-PF, clear error counter and move destination
	   * from PF to Active only if this chunk has been transmitted
	   * only once. Else, ambiguity in deciding whether sack was for
	   * original transmission or retransmission.
	   */
	  if (((eUseCmtPF == TRUE) && (spCurrNodeData->iNumTxs == 1)) ||
	      (eUseCmtPF == FALSE))
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
	}
      
      /* PN: 12/20/2007. Simulate send window */
      usChunkSize = spCurrNodeData->spChunk->sHdr.usLength;
      
      spDeleteNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;
      
      DeleteNode(&sSendBuffer, spDeleteNode);
      spDeleteNode = NULL;

      /* PN: 12/20/2007. Simulate send window */
      if (uiInitialSwnd) 
      {
	  uiAvailSwnd += usChunkSize;

	  DBG_PL(SendBufferDequeueUpTo, "AvailSendwindow=%ld"), uiAvailSwnd DBG_PR;
      }

    }

  if (channel_)
        (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));

  DBG_X(SendBufferDequeueUpTo);
}

Boolean_E SctpCMTAgent::AnyMarkedChunks()
{
  /****** Begin CMT Change ******/
  DBG_I(AnyMarkedChunks);

  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffNodeData = NULL;
  Boolean_E eMarkedChunksExist = FALSE;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;

  /* CMT change: added eMarkedChunksPending per destination.
   * reset eMarkedChunksPending for all dests
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->eMarkedChunksPending = FALSE;
    }

  for(spCurrBuffNode = sSendBuffer.spHead;
      spCurrBuffNode != NULL; 
      spCurrBuffNode = spCurrBuffNode->spNext)
    {
      spCurrBuffNodeData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;
      if(spCurrBuffNodeData->eMarkedForRtx != NO_RTX)
	{
	  eMarkedChunksExist = TRUE;
	  spCurrBuffNodeData->spDest->eMarkedChunksPending = TRUE;
	  DBG_PL(AnyMarkedChunks, "Dest %p: TRUE"), 
	    spCurrBuffNodeData->spDest DBG_PR;
	}
    }

  DBG_PL(AnyMarkedChunks, "Returning %s"), 
    eMarkedChunksExist ? "TRUE" : "FALSE" DBG_PR;
  DBG_X(AnyMarkedChunks);
  return eMarkedChunksExist;
  /****** End CMT Change ******/
}

/* This function goes through the entire send buffer filling a packet with 
 * chunks marked for retransmission. Once a packet is full (according to MTU)
 * it is transmittted. If the eLimit is one packet, than that is all that is
 * done. If the eLimit is cwnd, then packets full of marked tsns are sent until
 * cwnd is full.
 */
void SctpCMTAgent::RtxMarkedChunks(SctpRtxLimit_E eLimit)
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

  /****** Begin CMT Change ******/
  /* CMT change: Var is now per dest, part of SctpDest_S
   *
   * int iNumPacketsSent = 0; 
   */
  SctpDest_S *spTraceDest = NULL;
  char cpOutString[500];
  u_int uiRtxTsn = 0;
  double dCurrTime = Scheduler::instance().clock();
  /****** End CMT Change ******/

  memset(ucpOutData, 0, uiMaxPayloadSize);

  /****** Begin CMT Change ******/
  /* CMT change: This var is now per dest. Initialized inside loop.
   * uiBurstLength = 0;
   */
  /****** End CMT Change ******/

  /* make sure we clear all the spFirstOutstanding pointers before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->spFirstOutstanding = NULL;  // reset

      /****** Begin CMT Change ******/
      spCurrDestNodeData->iNumPacketsSent = 0; // reset
      spCurrDestNodeData->uiBurstLength = 0; // reset
      /****** End CMT Change ******/
    }

  /****** Begin CMT Change ******/
  /* We need to set the destination address for the retransmission(s).We assume
   * that on a given call to this function, all should all be sent to the same
   * address (should be a reasonable assumption). So, to determine the address,
   * we find the first marked chunk and determine the destination it was last 
   * sent to. 
   *
   * CMT change: This assumption is not reasonable for CMT. We therefore 
   * separate the rtxdest selection into a separate function.
   *
   * Also, we temporarily count all marked chunks as not outstanding.Why? Well,
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
	  spCurrBuffNodeData->spDest->iOutstandingBytes
	    -= spCurrChunk->sHdr.usLength;
	  /* CMT change: Removed code for RtxDest selection. CMT does its
	   * selection per TSN, and is inside the loop.
	   */
	}
    }

  spCurrBuffNode = sSendBuffer.spHead;

  /* CMT change: While loop should go through entire rtx list. 
   * checks are within loop to decide when an rtx should occur or not.
   * check has to be done per tsn to check if a dest is available for
   * that tsn and whether the eLimit applies to a given dest.
   */
  while(spCurrBuffNode != NULL)
    {
      
      /* CMT change: Set spRtxDest to the rtx dest 
       * of the first chunk in a rtx packet. Init to NULL now, and
       * set to rtx dest (according to policy) when chunk to be rtx'd 
       * is found.
       */
       spRtxDest = NULL;

      /* section 7.2.4.3
       *
       * continue filling up the packet with chunks which are marked for
       * rtx. exit loop when we have either run out of chunks or the
       * packet is full.
       *
       * note: we assume at least one chunk fits in the packet.  
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

	      /* CMT rtx policy used here. Set spRtxDest (according
	       * to policy) to the rtx dest of the first chunk in a rtx packet.
	       * spRtxDest is set to NULL at the beginning of a packet; once
	       * a chunk to be rtx'd is found, rtxDest for the packet is set to
	       * the rtx dest for that chunk, which is decided by the rtx 
	       * policy.
	       */
	      if(spRtxDest == NULL) 
		{
		  /* SelectRtxDest() decides rtx dest based on policy and
		   * returns dest. If NULL is returned, this TSN cannot
		   * be rtx'd at this time---this will cause the for loop
		   * to terminate, and the while loop to be reentered.
		   */
		  spRtxDest = SelectRtxDest(spCurrBuffNodeData, eLimit);
		}
	      
	      /* Check finally if for loop should be quit.
	       * If spRtxDest is NULL, no dest was able to send rtx.
	       */
	      if(spRtxDest == NULL)
		{
		  /* Jump out of for loop;move spCurrBuffNode to next in queue.
		   * At this point, no dest was able to send this rtx. 
		   * There may be other TSNs in the queue waiting for rtx, 
		   * so we move on to process them. "while" loop is reentered,
		   * and all tsns in the queue are processed.
		   */
		  spCurrBuffNode = spCurrBuffNode->spNext;
		  DBG_PL(RtxMarkedChunks, 
			 "Unable to rtx TSN %d at this time\n"), 
		    spCurrChunk->uiTsn DBG_PR;
		  break; 
		}
	      else
		{
		  /* CMT CUC algo: If rtxdest is not the same as orig dest, set
 		   * both eFindExpectedPseudoCumack and 
		   * eFindExpectedRtxPseudoCumack to TRUE.
		   */
		  if(spCurrBuffNodeData->spDest != spRtxDest)
		    {
		      spCurrBuffNodeData->spDest->eFindExpectedPseudoCum 
			= TRUE;
		      spCurrBuffNodeData->spDest->eFindExpectedRtxPseudoCum
			= TRUE;
		    }
		}

	      DBG_PL(RtxMarkedChunks, 
		     "eLimit=%s spRtxDest->eRtxLimit = %d pktsSent=%d out=%d cwnd=%d spCurrChunk->uiTsn=%d"),
		(eLimit == RTX_LIMIT_ONE_PACKET) ? "ONE_PACKET" : "CWND",
		spRtxDest->eRtxLimit,
		spRtxDest->iNumPacketsSent, spRtxDest->iOutstandingBytes, 
		spRtxDest->iCwnd, 
		spCurrChunk->uiTsn DBG_PR;

	      /* CMT change: Code moved from timeoutrtx
	       * No fast rtxs allowed on TSNs that have timed out.
	       * This is particularly important for Changeover/CMT.
	       */
	      spCurrBuffNodeData->eIneligibleForFastRtx = TRUE;
	      
	      /****** End CMT Change ******/

	      /* bundle the control chunk before any data chunks and only
	       * once per packet
	       */
	      if(eControlChunkBundled == FALSE)
		{
		  eControlChunkBundled = TRUE;
		  iBundledControlChunkSize 
		    = BundleControlChunks(ucpCurrOutData);
		  ucpCurrOutData += iBundledControlChunkSize;
		  iOutDataSize += iBundledControlChunkSize;
		}

	      /* can we fit this chunk into the packet without exceeding MTU?? 
	       */
  	      if((iOutDataSize + spCurrChunk->sHdr.usLength) 
		 > (int) uiMaxPayloadSize)
		break;  // doesn't fit in packet... jump out of the for loop

	      /* If this chunk was being used to measure the RTT,stop using it.
	       */
	      if(spCurrBuffNodeData->spDest->eRtoPending == TRUE &&
		 spCurrBuffNodeData->eMeasuringRtt == TRUE)
		{
		  spCurrBuffNodeData->eMeasuringRtt = FALSE;
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

	      /* section 6.1 - Whenever a transmission or retransmission is 
	       * made to any address, if the T3-rtx timer of that address 
	       * is not currently running, the sender MUST start that timer.
	       * If the timer for that address is already running, the sender 
	       * MUST restart the timer if the earliest (i.e., lowest TSN)
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
	      uiRtxTsn=spCurrChunk->uiTsn;

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
	} // end of for loop

      /* Transmit the packet now...
       */
      if(iOutDataSize > 0)
	{
	  SendPacket(ucpOutData, iOutDataSize, spRtxDest);
	  DBG_PL(RtxMarkedChunks, "OutDataSize = %d"), iOutDataSize DBG_PR;

	  /****** Begin CMT Change ******/
	  /* PN: Track rtx tsns in the trace file for debug purposes. 
	   */
	  spTraceDest = spRtxDest;
	  SetSource(spTraceDest); // gives us the correct source addr & port
	  sprintf(cpOutString,
		  "time: %-8.5f  "
		  "saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
		  "RTX TSN: %d ",
		  dCurrTime,
		  addr(), port(), spTraceDest->iNsAddr, spTraceDest->iNsPort,
		  uiRtxTsn);
	  if(channel_)
	    (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
	  sprintf(cpOutString, "\n\n");
	  if(channel_)
	    (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
	  /****** End CMT Change ******/

	  if(spRtxDest->eRtxTimerIsRunning == FALSE)
	    StartT3RtxTimer(spRtxDest);

	  /****** Begin CMT Change ******/
	  (spRtxDest->iNumPacketsSent)++;	  
	  /****** End CMT Change ******/
	  iOutDataSize = 0; // reset
	  ucpCurrOutData = ucpOutData; // reset
	  memset(ucpOutData, 0, uiMaxPayloadSize); // reset

	  spRtxDest->opCwndDegradeTimer->resched(spRtxDest->dRto);

	  /****** Begin CMT Change ******/
	  /* This addresses the proposed change to RFC2960 section 7.2.4,
	   * regarding using of Max.Burst. We have an option which allows
	   * to control if Max.Burst is applied.
	   */
	  /* CMT change: Use per dest burstlength to track bursts per dest.  
	   * *** BUG: The while loop should terminate after ALL
	   * selectable destinations have either exhausted their cwnds, or
	   * their burst limits.  As the code stands, whenever the burst
	   * limit is reached on ANY dest, the while loop is quit.  Is
	   * this a problem? Well... maybe not. RTX-SSTHRESH/CWND/LR
	   * should have (normally) one destination to which rtxs are
	   * being sent. In this case, we would not more destinations to
	   * try on anyways. In RTX-SAME/ASAP, the next incoming SACK will
	   * trigger a call to this function anyways.  
	   * 
	   * JRI-TODO: This bug should be fixed! Idea: Move check inside 
	   * SelectRtxDest(). Dest not selectable if MaxBurst exceeded. 
	   * That should do it.
	   */
	  if(eUseMaxBurst == MAX_BURST_USAGE_ON)
	    if( (eApplyMaxBurst == TRUE) && 
		((spRtxDest->uiBurstLength)++ >= MAX_BURST) )
	      {
		/* we've reached Max.Burst limit, so jump out of loop
		 */
		break;
	      }
	  /****** End CMT Change ******/
	}
    } // end of while loop


  /****** Begin CMT Change ******/
  /* 
   * With the RTT heuristic in CMT, TSNs outstanding for less than 1 SRTT
   * are not marked for rtx. Therefore, at this point in the function, if
   * no rtxs were sent out, but there are these unmarked TSNs less than 1
   * SRTT old outstanding, then the timer should be running for those TSNs.
   * 
   * Interesting case here. Rtxs are marked, but should not be
   * outstanding, and may not have been rtx'd at all. But with the RTT
   * heuristic there may be TSNs sent less than 1 SRTT ago that are
   * outstanding (i.e., not marked for rtx) and need the timer. So, what
   * should spFirstOutstanding point to? We think that the first REAL
   * outstanding chunk should be spFirstOutstanding. That is, set
   * spFirstOutstanding to the first chunk that (i) is not marked for rtx,
   * and (ii) is not yet gapacked. If a TSN does get rtx'd later to this
   * same dest, then the spFirstOutstanding will move to point to the
   * lower TSN.  
   * 
   * JRI-TODO, PN-TODO: Go through sendbuf and set spFirstOutstanding to
   * first TSN which is (i) is not marked for rtx, and (ii) is not yet
   * gapacked.
   */

    for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      if((spCurrDestNodeData->eRtxTimerIsRunning == FALSE) && 
	 (spCurrDestNodeData->iOutstandingBytes > 0)) 
	{
	  DBG_PL(RtxMarkedChunks, "Dest %p: starting T3 timer"), 
	    spCurrDestNodeData DBG_PR;
	  StartT3RtxTimer(spCurrBuffNodeData->spDest);
	}
    }
    /****** End CMT Change ******/

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
   * marked chunks.If we didn't hit the end, let's see if there are more marked
   * chunks.
   */
  eMarkedChunksPending = AnyMarkedChunks();

  DBG_X(RtxMarkedChunks);
  delete [] ucpOutData;
}

/****** Begin CMT Change ******/
/* New CMT function:
 * returns the retransmission destination for the given
 * TSN and policy.  returns NULL if no dest is found that can accomodate
 * this TSN given the policy and eLimit.
 *
 * CMT-PF: CMT-PF code implemented only for 
 * RTX-SSTHRESH and RTX-CWND policies
 */
SctpDest_S* SctpCMTAgent::SelectRtxDest(SctpSendBufferNode_S 
					*spCurrBuffNodeData,
					SctpRtxLimit_E eLimit)
{
  Boolean_E eFoundRtxDest = FALSE;
  SctpDest_S *spRtxDest = NULL;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;
  unsigned int uiHighestSsthresh = 0, uiHighestCwnd = 0;
  float fLowestLossrate = 0;
  
  DBG_I(SelectRtxDest);

  eFoundRtxDest = FALSE;

  switch (eCmtRtxPolicy)
    {
    case RTX_ASAP:
      /* NE: 04/16/2007
       * CMT rtx policy - rtxasap: Try to rtx to any destination for
       * which the sender has cwnd space available at the time of 
       * retransmission. If multiple destinations have available cwnd
       * space, one is chosen randomly.
       * Randomization of selecting a destination is not uniform in 
       * this implementation.
       * IMPORTANT NOTICE: This rtx. policy is used for experimental purposes
       * ONLY and is retained for verifying previous experiments. Please use
       * one of the suggested rtx. policies for CMT and CMT-PF experiments: 
       * RTX_SSTHRESH or RTX_CWND.
       */
      
      spCurrDestNodeData = spCurrBuffNodeData->spDest;

      DBG_PL(SelectRtxDest, "Current Dest: %p, Status: %d"), 
	spCurrDestNodeData, spCurrDestNodeData->eStatus DBG_PR;

      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;

	  /* Modified rtx mechanisms from SCTP. Allow ONE_PACKET_LIMIT
	   * for dest which has suffered a loss, so that at least one dest
	   * is allowed to retransmit one lost packet immediately. The rest
	   * of the dests are bound by their resp. cwnds. Subsequent calls 
	   * triggering sending rtxs will be bound by the resp. cwnds.
	   */
	  if((eLimit == RTX_LIMIT_ONE_PACKET && 
	      spCurrDestNodeData->eRtxLimit == RTX_LIMIT_ONE_PACKET &&
	      spCurrDestNodeData->iNumPacketsSent < 1 ) ||
	     (spCurrDestNodeData->iOutstandingBytes < 
	      spCurrDestNodeData->iCwnd)) 
	    {
	      if (spCurrDestNodeData->eStatus == SCTP_DEST_STATUS_ACTIVE) 
		{		  
		  if((eFoundRtxDest == TRUE) && (Random::random()&01))
		    {
		      /* Randomize dest if more than one found.
		       * If random() returns 1, then retain prev dest.,
		       * else set rtxDest to this dest.
		       * Nothing to be done here.
		       */
		    }
		  else
		    {
		      /* rtx will be sent to this destination.
		       * Set eFoundRtxDest to TRUE, to continue with 
		       * the procedure. 
		       */
		      eFoundRtxDest = TRUE;
		      spRtxDest = spCurrDestNodeData;
		      
		    }
		}
	    }
	}

      /* No Active destinations found, set eFoundRtxDest to FALSE
       * to return NULL 
       */
      if (spRtxDest == NULL) 
	eFoundRtxDest = FALSE;
      else
        {
	  
	  DBG_PL(SelectRtxDest, "Dest: %p"), spRtxDest  DBG_PR;
	  
      	  /* Modified rtx mechanisms from SCTP. Allow ONE_PACKET_LIMIT
       	   * for dest which has suffered a loss, so that at least one dest
       	   * is allowed to retransmit one lost packet immediately. The rest
       	   * of the dests are bound by their resp. cwnds. Subsequent calls 
       	   * triggering sending rtxs will be bound by the resp. cwnds.
       	   */
      	  if((eLimit == RTX_LIMIT_ONE_PACKET && 
	      spRtxDest->eRtxLimit == RTX_LIMIT_ONE_PACKET &&
	      spRtxDest->iNumPacketsSent < 1 ) ||
	     (spRtxDest->iOutstandingBytes < spRtxDest->iCwnd)) 
	    {
	      /* rtx will be sent to rtxDest. Set eFoundRtxDest
	       * to TRUE, to continue with the rtx procedure.
	       */
	      eFoundRtxDest = TRUE;
	      DBG_PL(SelectRtxDest, "SelectedRtxDest: %p"), spRtxDest DBG_PR;
	    }
        }
      
      break; 
      
    case RTX_TO_SAME:
      /* NE: 4/29/2007
       * CMT rtx policy - rtxtosame: Rtx goes to same dest only(until the 
       * destination is deemed INACTIVE due to failure).
       * IMPORTANT NOTICE: This rtx. policy is used for experimental purposes
       * ONLY and is retained for verifying previous experiments. Please use
       * one of the suggested rtx. policies for CMT and CMT-PF experiments: 
       * RTX_SSTHRESH or RTX_CWND. 
       */

      DBG_PL(SelectRtxDest, "Rtx policy: RTX-SAME") DBG_PR;

      spRtxDest = spCurrBuffNodeData->spDest;

      /* If RtxDest is failed, get next in list. 
       * Infinite loop if all dests failed! 
       */
      while (spRtxDest->eStatus == SCTP_DEST_STATUS_INACTIVE) 
	{
	  spRtxDest = GetNextDest(spRtxDest);
	}
      
      DBG_PL(SelectRtxDest, 
	     "spRtxDest %p: iCwnd=%d iOutstandingBytes=%d iNumPacketsSent=%d"),
	spRtxDest, spRtxDest->iCwnd, spRtxDest->iOutstandingBytes, 
	spRtxDest->iNumPacketsSent DBG_PR;
      
      /* No Active destinations found, set eFoundRtxDest to FALSE
       * to return NULL 
       */
      if (spRtxDest == NULL) 
	eFoundRtxDest = FALSE;
      else
        {
	  if((eLimit == RTX_LIMIT_ONE_PACKET && 
	      spRtxDest->eRtxLimit == RTX_LIMIT_ONE_PACKET &&
	      spRtxDest->iNumPacketsSent < 1 ) ||
	     (spRtxDest->iOutstandingBytes < spRtxDest->iCwnd)) 
	    {
	      /* rtx will be sent to same dest as tx. Set eFoundRtxDest
	       * to TRUE, to continue with the rtx procedure.
	       */
	      eFoundRtxDest = TRUE;
	      DBG_PL(SelectRtxDest, "SelectedRtxDest: %p"), spRtxDest DBG_PR;
	    }
	  else 
	    {
	      DBG_PL(SelectRtxDest, "Unable to find rtx destination") DBG_PR;
	    }
	}
      break; // end of rtxtosame dest selection

    case RTX_SSTHRESH:
      /* NE: 4/29/2007
       * CMT rtx. policy - RTX_SSTHRESH: rtx. is sent to the destination for 
       * which the sender has the largest ssthresh. A tie is broken by random
       * selection. 
       */

      uiHighestSsthresh = 0;

      /* CMT-PF: If all destinations are marked PF, select one from them
       * for retransmissions
       */
      if (eUseCmtPF == TRUE)
	{
	  spRtxDest = SelectFromPFDests();
	  if (spRtxDest != NULL)
	    {
	      /* spRtxDest was PF and changed to Active by SelectFromPFDests()
	       */
	      tiCountPFToActiveRtxms++ ; //trace will pick it up
	      
	      eFoundRtxDest = TRUE;
	      
	      break; // break for RTX_SSTHRESH case.
	    }
	}

      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;

	  DBG_PL(SelectRtxDest, "Dest: %p, Status: %d"), 
	    spCurrDestNodeData, spCurrDestNodeData->eStatus DBG_PR;

	  /* Do not retransmit to an INACTIVE or PF destination. 
	   */
	  if((spCurrDestNodeData->eStatus == SCTP_DEST_STATUS_INACTIVE) ||
	     (spCurrDestNodeData->eStatus == SCTP_DEST_STATUS_POSSIBLY_FAILED))
	    continue;

	  DBG_PL(SelectRtxDest, "Dest: %p, Ssthresh: %d, Out: %d"), 
			spCurrDestNodeData, spCurrDestNodeData->iSsthresh,
			spCurrDestNodeData->iOutstandingBytes DBG_PR;
	  if ((int)uiHighestSsthresh < spCurrDestNodeData->iSsthresh)
	    {
	      uiHighestSsthresh = spCurrDestNodeData->iSsthresh;
	      spRtxDest = spCurrDestNodeData;
	    }
	  else if ((int)uiHighestSsthresh == spCurrDestNodeData->iSsthresh)
	    {
	      // break ties with random selection
	      if(Random::random()&01)
		spRtxDest = spCurrDestNodeData;
	    }
	}

      /* if no active dest found, then return null
       */
      if (spRtxDest == NULL) 
	eFoundRtxDest = FALSE;
      else
	{
	  DBG_PL(SelectRtxDest, "Dest: %p"), spRtxDest  DBG_PR;
	  /* Modified rtx mechanisms from SCTP. Allow ONE_PACKET_LIMIT
	   * for dest which has suffered a loss, so that at least one dest
	   * is allowed to retransmit one lost packet immediately. The rest
	   * of the dests are bound by their resp. cwnds. Subsequent calls 
	   * triggering sending rtxs will be bound by the resp. cwnds.
	   */
	  if((eLimit == RTX_LIMIT_ONE_PACKET && 
	      spRtxDest->eRtxLimit == RTX_LIMIT_ONE_PACKET &&
	      spRtxDest->iNumPacketsSent < 1 ) ||
	     (spRtxDest->iOutstandingBytes < spRtxDest->iCwnd)) 
	    {
	      /* rtx will be sent to rtxDest. Set eFoundRtxDest
	       * to TRUE, to continue with the rtx procedure.
	       */
	      eFoundRtxDest = TRUE;
	      DBG_PL(SelectRtxDest, "SelectedRtxDest: %p"), spRtxDest DBG_PR;
	    }
	}
      break;


    case RTX_LOSSRATE:
      /* NE: 4/24/2007
       * CMT rtx. policy - RTX_LOSSRATE: rtx. is sent to the destination with
       * the lowest loss rate path. If multiple destinations have the same
       * loss rate, one is selected randomly.
       * IMPORTANT NOTICE: This rtx. policy is used for experimental purposes
       * ONLY and is retained for verifying previous experiments. Please use
       * one of the suggested rtx. policies for CMT and CMT-PF experiments: 
       * RTX_SSTHRESH or RTX_CWND.
       */
	 
      fLowestLossrate = 10;

      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;

	  if (spCurrDestNodeData->eStatus == SCTP_DEST_STATUS_INACTIVE)
	    continue;

	  if (fLowestLossrate > spCurrDestNodeData->fLossrate)
	    {
	      fLowestLossrate = spCurrDestNodeData->fLossrate;
	      spRtxDest = spCurrDestNodeData;
	      
	    }
	  else if (fLowestLossrate == spCurrDestNodeData->fLossrate)
	    {
	      // break ties with random selection
	      if(Random::random()&01)
		spRtxDest = spCurrDestNodeData;
	    }
	}

      /* if no active dest found, then return null
       */
      if (spRtxDest == NULL) 
	eFoundRtxDest = FALSE;
      else
	{
	  /* Modified rtx mechanisms from SCTP. Allow ONE_PACKET_LIMIT
	   * for dest which has suffered a loss, so that at least one dest
	   * is allowed to retransmit one lost packet immediately. The rest
	   * of the dests are bound by their resp. cwnds. Subsequent calls 
	   * triggering sending rtxs will be bound by the resp. cwnds.
	   */
	  if((eLimit == RTX_LIMIT_ONE_PACKET && 
	      spRtxDest->eRtxLimit == RTX_LIMIT_ONE_PACKET &&
	      spRtxDest->iNumPacketsSent < 1 ) ||
	     (spRtxDest->iOutstandingBytes < spRtxDest->iCwnd)) 
	    {
	      /* rtx will be sent to rtxDest. Set eFoundRtxDest
	       * to TRUE, to continue with the rtx procedure.
	       */
	      eFoundRtxDest = TRUE;
	      DBG_PL(SelectRtxDest, "SelectedRtxDest: %p"), spRtxDest DBG_PR;
	    }
	}

      break;

    case RTX_CWND:
      /* NE: 4/29/2007
       * CMT rtx. policy - RTX_CWND: rtx. is sent to the destination for which
       * the sender has the largest cwnd. A tie is broken by random selection.
       */

      uiHighestCwnd = 0;

      /* CMT-PF: If all destinations are marked PF, select one from them
       * for retransmissions
       */
      if (eUseCmtPF == TRUE)	
	{
	  spRtxDest = SelectFromPFDests();
	  if (spRtxDest != NULL)
	    {
	      /* spRtxDest was PF and changed to Active by SelectFromPFDests()
	       */
	      tiCountPFToActiveRtxms++ ; //trace will pick it up
	      
	      eFoundRtxDest = TRUE;
	      
	      break; // break for RTX_CWND case.
	    }
	  
	} /* if (eUseCmtPF == TRUE) */
      
      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
	  
	  DBG_PL(SelectRtxDest, "Dest: %p, Status: %d"), 
	    spCurrDestNodeData, spCurrDestNodeData->eStatus DBG_PR;
	  
	  /* Do not retransmit to an INACTIVE or PF destination. 
	   */
	  if((spCurrDestNodeData->eStatus == SCTP_DEST_STATUS_INACTIVE) ||
	     (spCurrDestNodeData->eStatus == SCTP_DEST_STATUS_POSSIBLY_FAILED))
	    continue;

	  DBG_PL(SelectRtxDest, "Dest: %p, Cwnd: %d, Out: %d"), 
	    spCurrDestNodeData, spCurrDestNodeData->iCwnd,
	    spCurrDestNodeData->iOutstandingBytes DBG_PR;
	  
	  if ((int)uiHighestCwnd < spCurrDestNodeData->iCwnd)
	    {
	      uiHighestCwnd = spCurrDestNodeData->iCwnd;
	      spRtxDest = spCurrDestNodeData;
	    }
	  else if ((int)uiHighestCwnd == spCurrDestNodeData->iCwnd)
	    {
	      // break ties with random selection
	      if(Random::random()&01)
		spRtxDest = spCurrDestNodeData;
	    }
	}
      
      /* No Active destinations found, set eFoundRtxDest to FALSE
       * to return NULL 
       */
      if (spRtxDest == NULL) 
	eFoundRtxDest = FALSE;
      else
        {
	  
	  DBG_PL(SelectRtxDest, "Dest: %p"), spRtxDest  DBG_PR;
	  
      	  /* Modified rtx mechanisms from SCTP. Allow ONE_PACKET_LIMIT
       	   * for dest which has suffered a loss, so that at least one dest
       	   * is allowed to retransmit one lost packet immediately. The rest
       	   * of the dests are bound by their resp. cwnds. Subsequent calls 
       	   * triggering sending rtxs will be bound by the resp. cwnds.
       	   */
      	  if((eLimit == RTX_LIMIT_ONE_PACKET && 
	      spRtxDest->eRtxLimit == RTX_LIMIT_ONE_PACKET &&
	      spRtxDest->iNumPacketsSent < 1 ) ||
	     (spRtxDest->iOutstandingBytes < spRtxDest->iCwnd)) 
	    {
	      /* rtx will be sent to rtxDest. Set eFoundRtxDest
	       * to TRUE, to continue with the rtx procedure.
	       */
	      eFoundRtxDest = TRUE;
	      DBG_PL(SelectRtxDest, "SelectedRtxDest: %p"), spRtxDest DBG_PR;
	    }
        }
      break;
      
    }
  
  if(eFoundRtxDest == FALSE)
    spRtxDest = NULL;

  DBG_PL(SelectRtxDest, "Selected Dest: %p"), spRtxDest DBG_PR;
 
  DBG_X(SelectRtxDest);
  return spRtxDest;
}
/****** End CMT Change ******/


/* returns a boolean of whether a fast retransmit is necessary
 */
Boolean_E SctpCMTAgent::ProcessGapAckBlocks(u_char *ucpSackChunk,
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

  /* PN: 12/20/2007, NR-SACKs */
  SctpGapAckBlock_S *spCurrGapAck = NULL;
  /* NE: 06/2009 */
  SctpGapAckBlock_S *spCurrNonRenegGapAck = NULL;
  
  Node_S *spPrevNode = sSendBuffer.spHead;
  Boolean_E eDoNotChangeCurrNode = FALSE;
  
  u_short usNumGapAckBlocks = spSackChunk->usNumGapAckBlocks;

  /* NE: number of Non-Renegable Gap Ack Blocks reported */
  u_short usNumNonRenegGapAckBlocks = 0;

  /* NE: let's decide which block (Gap Ack or Non-Renegable Gap Ack) 
     is used for the gap ack processing */
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
	  
	  /****** Begin CMT Change ******/
	  /* Initialize uiHighestTsnInSackForDest = current cumack, since it 
	   * has to be at least equal to the cumack for the destinations
	   * which matter in the fast rtx process. uiHighestTsnInSackForDest 
	   * is used for HTNA also.
	   */
	  spCurrDestNodeData->uiHighestTsnInSackForDest =spSackChunk->uiCumAck;
	  /****** End CMT Change ******/
	}

      /* NE: process all reported Gap Ack/Non-Renegable Ack Blocks */
      for(spCurrNode = sSendBuffer.spHead;
	  (spCurrNode != NULL) &&
	    ((usNumGapAcksProcessed != usNumGapAckBlocks) ||
	     (usNumNonRenegGapAcksProcessed != usNumNonRenegGapAckBlocks)); 
	  spCurrNode = spCurrNode->spNext)                
	{
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;
		 
	  /****** Begin CMT Change ******/
	  /* CMT CUC algo: If expected-pseudo(-rtx)-cum is to be found,
	   * check if this tsn has been previously acked. If not, set
	   * expected-pseudo(-rtx)-cum to this tsn. uiExpectedPseudoCum
	   * tracks the PCum for new txs on a dest, and
	   * uiExpectedRtxPseudoCum tracks the pcum for rtxs on the dest.
	   * NOTE: This cwnd algo assumes that the sendbuf is sorted
	   * according to TSN.
	   */
	  
	  if ((spCurrNodeData->spDest->eFindExpectedPseudoCum == TRUE) &&
	      (spCurrNodeData->eGapAcked == FALSE) &&
	      (spCurrNodeData->iNumTxs == 1)) 
	    {
	      spCurrNodeData->spDest->uiExpectedPseudoCum = 
		spCurrNodeData->spChunk->uiTsn;
	      spCurrNodeData->spDest->eFindExpectedPseudoCum = FALSE;
	    }

	  if ((spCurrNodeData->spDest->eFindExpectedRtxPseudoCum == TRUE) &&
	      (spCurrNodeData->eGapAcked == FALSE) &&
	      (spCurrNodeData->iNumTxs > 1)) 
	    {
	      spCurrNodeData->spDest->uiExpectedRtxPseudoCum = 
		spCurrNodeData->spChunk->uiTsn;
	      spCurrNodeData->spDest->eFindExpectedRtxPseudoCum = FALSE;
	    }
	  /****** End CMT Change ******/

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
	  
	  DBG_PL(ProcessGapAckBlocks, "GapAckBlock StartOffset=%d EndOffset=%d"),
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

		  /****** Begin CMT Change ******/
		  /* CMT-PF: if the gapacked chunk was new data sent to a
		   * PF dest, then, mark destination to ACTIVE; else, 
		   * ambiguity in deciding whether gap-ack was for the
		   * original transmission or retransmission.
		   */
		   if ((spCurrNodeData->spDest->eStatus == 
			SCTP_DEST_STATUS_POSSIBLY_FAILED) &&
			(spCurrNodeData->iNumTxs == 1))
		     {
		       spCurrNodeData->spDest->eStatus=SCTP_DEST_STATUS_ACTIVE;
		       spCurrNodeData->spDest->iErrorCount = 0; //clr err ctr
		       tiErrorCount++;     // ... and trace it too!

		       DBG_PL(ProcessGapAckBlocks, 
			      "Dest:%p, New data gapacked. Mark from PF to Active"),
			 spCurrNodeData->spDest DBG_PR;
		     }

		  /* CMT CUC algo: If newly acked TSN passes
		   * pseudo-cumack, we have a new pseudo-cumack!
		   * Increment cwnd accordingly.
		   */
		  if (spCurrNodeData->spChunk->uiTsn == 
		      spCurrNodeData->spDest->uiExpectedPseudoCum) {

		    spCurrNodeData->spDest->eNewPseudoCum = TRUE;
		    spCurrNodeData->spDest->eFindExpectedPseudoCum = TRUE;
		  }

		  if (spCurrNodeData->spChunk->uiTsn == 
		      spCurrNodeData->spDest->uiExpectedRtxPseudoCum) {

		    spCurrNodeData->spDest->eNewPseudoCum = TRUE;
		    spCurrNodeData->spDest->eFindExpectedRtxPseudoCum = TRUE;
		  }

		  /* SFR algo: Mark eSawNewAck for newly acked destination and
		   * update highest TSN seen in current SACK (per destination) 
		   * if appropriate. uiHighestTsnInSackForDest is used for 
		   * HTNA also. We do not need uiHighestTsnNewlyAcked with 
		   * the SFR algo, hence commented out.
		   */
		  spCurrNodeData->spDest->eSawNewAck = TRUE;

		  if (spCurrNodeData->spDest->uiHighestTsnInSackForDest < 
		      spCurrNodeData->spChunk->uiTsn)
		    {
		      spCurrNodeData->spDest->uiHighestTsnInSackForDest =
			spCurrNodeData->spChunk->uiTsn;
		    }
		  
		  /* CMT DAC algo: Update lowest TSN seen in current SACK
		   * (per destination) if appropriate. The lowest TSN is
		   * initialized to zero, and since we process cum and gap
		   * TSNs in TSN order, the first value this variable is
		   * set to will continue to be the lowest TSN
		   * acked. uiLowestTsnInSackForDest is used for missing
		   * report increments with delayed SACKs in CMT.
		   */
		  if (spCurrNodeData->spDest->uiLowestTsnInSackForDest == 0)
		    {
		      spCurrNodeData->spDest->uiLowestTsnInSackForDest =
			spCurrNodeData->spChunk->uiTsn;
		    }
		  /****** End CMT Change ******/
		  
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
		  
		  /****** Begin CMT Change ******/
		  /* only increment partial bytes acked if we are in
		   * congestion avoidance mode, we have a new cum ack, and
		   * we haven't already incremented it for this TSN
		   */
		  /* CMT change: If new pseudo-cumack, then adjust cwnd. 
		   * This check looks a little ugly, so look closely.
		   */
		  /* JRI-TODO: Remove Check for CmtCwnd
		   */
		  if( (spCurrNodeData->spDest->iCwnd > 
                     spCurrNodeData->spDest->iSsthresh) &&
		     (((eUseCmtCwnd) && 
                     (spCurrNodeData->spDest->eNewPseudoCum)) || 
                     ((!eUseCmtCwnd) && (eNewCumAck))) &&
		     (spCurrNodeData->eAddedToPartialBytesAcked == FALSE))
		  /****** End CMT Change ******/
		    {
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting eAddedToPartiallyBytesAcked=TRUE")DBG_PR;
		      
		      spCurrNodeData->eAddedToPartialBytesAcked = TRUE; // set

		      spCurrNodeData->spDest->iPartialBytesAcked 
			+= spCurrNodeData->spChunk->sHdr.usLength;
		    }

		  /****** Begin CMT Change ******/
		  /* We update the RTT estimate if the following hold true:
		   *   1. RTO pending flag is set (6.3.1.C4)
		   *   2. RTT is being measured for this chunk 
		   *   3. This chunk has not been retransmitted
		   *   4. This chunk has not been gap acked already 
		   *   5. This chunk has not been advanced acked (pr-sctp)
		   */
		  if(spCurrNodeData->spDest->eRtoPending == TRUE &&
		     spCurrNodeData->eMeasuringRtt == TRUE &&
		     spCurrNodeData->iNumTxs == 1 &&
		     spCurrNodeData->eAdvancedAcked == FALSE) 
		    /****** End CMT Change ******/
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
		   * these sacks to clear the error counter, or else
		   * failover would take longer.
		   */
		  if((spCurrNodeData->spDest->iErrorCount != 0) &&
		     (spCurrNodeData->eMarkedForRtx != TIMEOUT_RTX))
		    {
		      DBG_PL(ProcessGapAckBlocks,
			     "clearing error counter for %p with tsn=%lu"), 
			spCurrNodeData->spDest, 
			spCurrNodeData->spChunk->uiTsn DBG_PR;

		      spCurrNodeData->spDest->iErrorCount = 0; // clear errors
		      tiErrorCount++;                      // ... and trace it!
		      spCurrNodeData->spDest->eStatus =SCTP_DEST_STATUS_ACTIVE;
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
	      /* This point in the rtx buffer is already past the tsns which 
	       * are being acked by this gap ack block.  
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
	      /* if(usNumGapAcksProcessed != spSackChunk->usNumGapAckBlocks) */
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

	    /* 04/18/2008. PN, track prev node for correct processing with
	     * nr-sacks. Account for missing tsns in send buffer */

	    if (eDoNotChangeCurrNode == TRUE) {
		spCurrNode = spPrevNode; //spPrevNode remains unchanged -- the previous
	    } 
	    else {
	    	spPrevNode = spCurrNode;
	    }
	    eDoNotChangeCurrNode = FALSE;
	}

      /* By this time, either we have run through the entire send buffer or we
       * have run out of gap ack blocks.In the case that we have run out of gap
       * ack blocks before we finished running through the send buffer, we need
       * to mark the remaining chunks in the send buffer as eGapAcked=FALSE.
       * This final marking needs to be done,because we only trust gap ack info
       * from the last SACK. Otherwise, renegging (which we don't do) or out of
       * order SACKs would give the sender an incorrect view of the peer's 
       * rwnd.
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
	  if(spCurrNodeData->eGapAcked == TRUE && !eUseNonRenegSacks)
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

      DBG_PL(ProcessGapAckBlocks,"now incrementing missing reports...") DBG_PR;
      DBG_PL(ProcessGapAckBlocks, "uiHighestTsnNewlyAcked=%d"), 
	uiHighestTsnNewlyAcked DBG_PR;

      /****** Begin CMT Change ******/
      /* CMT DAC algo: The SACK chunk flags are unused in RFC2960. We
       * propose to use 1 of the 8 bits in the SACK flags for the CMT
       * delayed ack algo.  This bit will be used to indicate the number
       * of DATA packets were received between the previous and the
       * current SACK. This information will be used by the DATA sender to
       * increment missing reports better in CMT.
       * 
       ** If the SACK is a mixed SACK (multiple dests being acked) then
       *  the sender conservatively increases missing reports by 1. 
       ** If not a mixed SACK, and TSNs are acked across a lost TSN t
       *  (t- and t+ TSNs being acked by this SACK), sender conservatively 
       *  can use only 1 missing report.
       ** If not a mixed SACK, and TSNs are not acked across lost TSN t, 
       *  then the sender can use number of packets reported by the SACK 
       *  as the number of missing reports to be used.
       *
       * For 2 latter cases, sender needs to know lowest TSN and highest TSN 
       * acked in the current SACK for the destination for which missing 
       * reports are to be incremented. If the TSN to be marked is between
       * the highest and the lowest, then the SACK should be treated as 
       * acking across the missing TSN. This code appears further below.
       */

      uiNumDestsSacked = 0;
      for(spCurrDestNode = sDestList.spHead;
	  spCurrDestNode != NULL;
	  spCurrDestNode = spCurrDestNode->spNext)
	{
	  spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
	  if (spCurrDestNodeData->eSawNewAck)
	    uiNumDestsSacked++;
	}

      DBG_PL(ProcessGapAckBlocks, "number of dests SACKed = %d"),
	     uiNumDestsSacked DBG_PR;
      
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
	      /* Applying SFR. The modified check satisfies SFR and
	       * handles part of HTNA per destination. Does not take care
	       * of the HTNA algo during Fast Recovery.
	       */
	      /* JRI-TODO: Look into HTNA bit during Fast Recovery,
	       * and how it can be implemented.
	       */
	      if (((eUseCmtReordering == FALSE) &&
		   (spCurrNodeData->spChunk->uiTsn < uiHighestTsnNewlyAcked)) 
		  ||
		  // ((eUseCmtReordering == FALSE) && (eNewCumAck == TRUE) && 
		  // (uiHighestTsnNewlyAcked <= uiRecover) &&
		  // (spCurrNodeData->spChunk->uiTsn < uiHighestTsnSacked)) ||
		  ((eUseCmtReordering == TRUE) && 
		   (spCurrNodeData->spDest->eSawNewAck) &&
		   (spCurrNodeData->spChunk->uiTsn < 
		    spCurrNodeData->spDest->uiHighestTsnInSackForDest)))
		{
		  /* CMT DAC algo: For 2 latter cases mentioned above,
		   * sender needs to know lowest TSN and highest TSN acked
		   * in the current SACK for the destination for which
		   * missing reports are to be incremented. If the TSN to
		   * be marked is between the highest and the lowest, then
		   * the SACK should be treated as acking across the
		   * missing TSN.
		   */
		  if (eUseCmtDelAck == TRUE) 
		    {
		      if ((uiNumDestsSacked > 1) || 
			  (spCurrNodeData->spDest->uiLowestTsnInSackForDest < 
			   spCurrNodeData->spChunk->uiTsn)) 
			{
			  spCurrNodeData->iNumMissingReports += 1;
			} else if (uiNumDestsSacked == 1) {
			spCurrNodeData->iNumMissingReports += 
			  uiNumPacketsSacked;
		      } else {/* do nothing if uiNumDestsSacked < 1 ! */}
		    } 
		  else {
		    spCurrNodeData->iNumMissingReports++;
		  }
		  /****** End CMT Change ******/

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


void SctpCMTAgent::ProcessSackChunk(u_char *ucpSackChunk)
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

  /****** Begin CMT Change ******/
  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffNodeData = NULL;
  SctpDataChunkHdr_S  *spCurrChunk;

  /* CMT DAC algo: The SACK chunk flags are unused in RFC2960. We propose
   * to use 2 of the 8 bits in the SACK flags for the CMT delayed ack
   * algo.  These bits will be used to indicate the number of DATA packets
   * were received between the previous and the current SACK. This
   * information will be used by the DATA sender to increment missing
   * reports better in CMT.
   */
  uiNumPacketsSacked = spSackChunk->sHdr.ucFlags;
  DBG_PL(ProcessSackChunk, "Sack Flags=%d"), spSackChunk->sHdr.ucFlags DBG_PR;
  /****** End CMT Change ******/
  
  /* make sure we clear all the iNumNewlyAckedBytes before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->iNumNewlyAckedBytes = 0;
      spCurrDestNodeData->spFirstOutstanding = NULL;

      /****** Begin CMT Change ******/
      spCurrDestNodeData->eNewPseudoCum = FALSE;
      spCurrDestNodeData->eSawNewAck = FALSE;
      /* CMT DAC algo: Initialize uiLowestTsn to zero, this variable
       * should get set during SACK processing to some TSN.
       */
      spCurrDestNodeData->uiLowestTsnInSackForDest = 0;
      spCurrDestNodeData->uiBurstLength = 0;
      /****** End CMT Change ******/
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

  /****** Begin CMT Change ******/
  /* CMT change In case of reneg, or reordered SACKs where previously
   * present gapack info is now absent, arwnd info can be stale and
   * dangerous if used without consideration to the fact that the receiver
   * has apparently reneged on data. Note that ProcessGapAckBlocks was
   * previously not called when gapacks were absent in the SACK. What if
   * previously present gapacks were reneged? Since we do not reneg in
   * this implementation, this can happen only with reordered SACKs.  In
   * any case, ProcessGapAckBlocks() has code that will handle such
   * apparent reneging, i.e., calculate the correct new
   * outstanding_bytes. Therefore, this function should be called even if
   * there are no gapack blocks in the SACK chunk to handle renegs, or
   * reordered SACKs with no gapack info.
   *
   * if(spSackChunk->usNumGapAckBlocks != 0) // are there any gaps??
   * {
   *   eFastRtxNeeded = ProcessGapAckBlocks(ucpSackChunk, eNewCumAck);
   * } 
   */
  /* JRI-TODO: Move change to SCTP code. This problem can be caused by 
   * reordered SACKs too.The SCTP code handles calculation of outstanding bytes
   * when SACK is reordered, so this check should be removed there too.
   */
  eFastRtxNeeded = ProcessGapAckBlocks(ucpSackChunk, eNewCumAck);

  /* TEMP-FIX: Calculate the correct outstanding bytes in each dest, since
   * even data that is marked for rtx is still considered oustanding. The
   * value of outstanding_bytes is restored later below. Need this to stop
   * T3 timers correctly.  This problem needs to be fixed here, and in SCTP.
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
            -= spCurrChunk->sHdr.usLength;
        }
    }
  /****** End CMT Change ******/
  
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;

      /****** Begin CMT Change ******/
      /* Adjust cwnd if sack advanced the pseudo-cumack point AND this
       * destination has newly acked bytes. Also, we MUST adjust our
       * congestion window BEFORE we update the number of outstanding
       * bytes to reflect the newly acked bytes in received SACK.  
       */
      DBG_PL(ProcessSackChunk, "eNewPseudoCum=%d, newlyacked=%d"), 
	spCurrDestNodeData->eNewPseudoCum, 
	spCurrDestNodeData->iNumNewlyAckedBytes DBG_PR;

      if (eUseCmtCwnd == TRUE) 
	{
	  if((spCurrDestNodeData->eNewPseudoCum == TRUE) && 
	     (spCurrDestNodeData->iNumNewlyAckedBytes > 0))
	    AdjustCwnd(spCurrDestNodeData);
	} 
      else if (eUseCmtCwnd == FALSE) 
	{
	  if(eNewCumAck == TRUE && spCurrDestNodeData->iNumNewlyAckedBytes > 0)
	    AdjustCwnd(spCurrDestNodeData);
	}
      /****** End CMT Change ******/

      /* The number of outstanding bytes is reduced by how many bytes this sack
       * acknowledges.
       */
      if(spCurrDestNodeData->iNumNewlyAckedBytes <=
	 spCurrDestNodeData->iOutstandingBytes)
	{
	  spCurrDestNodeData->iOutstandingBytes 
	    -= spCurrDestNodeData->iNumNewlyAckedBytes;
	}
      else
	spCurrDestNodeData->iOutstandingBytes = 0;

      DBG_PL(ProcessSackChunk,"Dest #%d (%d:%d) (%p): outstanding=%d, cwnd=%d"),
	++i, spCurrDestNodeData->iNsAddr, spCurrDestNodeData->iNsPort,
	spCurrDestNodeData, spCurrDestNodeData->iOutstandingBytes, 
	spCurrDestNodeData->iCwnd DBG_PR;

      if(spCurrDestNodeData->iOutstandingBytes == 0)
	{
	  /* All outstanding data has been acked
	   */
	  spCurrDestNodeData->iPartialBytesAcked = 0;  // section 7.2.2

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
      if(spCurrDestNodeData->iOutstandingBytes > 0 &&
	 spCurrDestNodeData->eRtxTimerIsRunning == FALSE)
	{
	  StartT3RtxTimer(spCurrDestNodeData);
	}
    }

  /****** Begin CMT Change ******/
  /* TEMP-FIX: Restore value of outstanding_bytes. See TEMP-FIX comment
   * above for details.
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
  /****** End CMT Change ******/

  DBG_F(ProcessSackChunk, DumpSendBuffer());

  AdvancePeerAckPoint();

  /****** Begin CMT Change ******/
  uiArwnd = spSackChunk->uiArwnd;
  /****** End CMT Change ******/

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

  tiRwnd++; // trigger changes to be traced

  DBG_PL(ProcessSackChunk, "uiPeerRwnd=%d, uiArwnd=%d"), uiPeerRwnd, 
    spSackChunk->uiArwnd DBG_PR;
  DBG_X(ProcessSackChunk);
}

/* PN: 12/21/07
 * Method to process Non-Reneg Sack chunks.
 * Most of the code is similar to ProcessSackChunk. 
 * Is there a way to minimize code repetition here?
 */
void SctpCMTAgent::ProcessNonRenegSackChunk(u_char *ucpNonRenegSackChunk)
{
  DBG_I(ProcessNonRenegSackChunk);
  
  SctpNonRenegSackChunk_S *spNonRenegSackChunk 
    = (SctpNonRenegSackChunk_S *) ucpNonRenegSackChunk;
  
  u_short usNumGapAckBlocks = spNonRenegSackChunk->usNumGapAckBlocks;
  u_short usNumNonRenegGapAckBlocks = 
    spNonRenegSackChunk->usNumNonRenegSackBlocks;

  DBG_PL(ProcessNonRenegSackChunk, 
	 "cum=%d arwnd=%d #gapacks=%d #nrgaps=%d #duptsns=%d"),
    spNonRenegSackChunk->uiCumAck, spNonRenegSackChunk->uiArwnd, 
    usNumGapAckBlocks, 
    usNumNonRenegGapAckBlocks,
    spNonRenegSackChunk->usNumDupTsns
    DBG_PR;

  Boolean_E eFastRtxNeeded = FALSE;
  Boolean_E eNewCumAck = FALSE;
  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestNodeData = NULL;
  u_int uiTotalOutstanding = 0;
  int i = 0;

  /****** Begin CMT Change ******/
  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffNodeData = NULL;
  SctpDataChunkHdr_S  *spCurrChunk;

  /* CMT DAC algo: The SACK chunk flags are unused in RFC2960. We propose
   * to use 2 of the 8 bits in the SACK flags for the CMT delayed ack
   * algo.  These bits will be used to indicate the number of DATA packets
   * were received between the previous and the current SACK. This
   * information will be used by the DATA sender to increment missing
   * reports better in CMT.
   */
  uiNumPacketsSacked = spNonRenegSackChunk->sHdr.ucFlags;
	
  DBG_PL(ProcessNonRenegSackChunk, "NR-Sack Flags=%x, uiNumPacketsSacked=%d"), 
	spNonRenegSackChunk->sHdr.ucFlags, uiNumPacketsSacked DBG_PR;
  /****** End CMT Change ******/
  
  /* make sure we clear all the iNumNewlyAckedBytes before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->iNumNewlyAckedBytes = 0;
      spCurrDestNodeData->spFirstOutstanding = NULL;

      /****** Begin CMT Change ******/
      spCurrDestNodeData->eNewPseudoCum = FALSE;
      spCurrDestNodeData->eSawNewAck = FALSE;
      /* CMT DAC algo: Initialize uiLowestTsn to zero, this variable
       * should get set during SACK processing to some TSN.
       */
      spCurrDestNodeData->uiLowestTsnInSackForDest = 0;
      spCurrDestNodeData->uiBurstLength = 0;
      /****** End CMT Change ******/
    }

  if(spNonRenegSackChunk->uiCumAck < uiCumAckPoint) 
    {
      /* this cumAck's a previously cumAck'd tsn (ie, it's out of order!)
       * ...so ignore!
       */
      DBG_PL(ProcessNonRenegSackChunk, "ignoring out of order NR-Sack!") DBG_PR;
      DBG_X(ProcessNonRenegSackChunk);
      return;
    }
  else if(spNonRenegSackChunk->uiCumAck > uiCumAckPoint)
    {
      eNewCumAck = TRUE; // incomding SACK's cum ack advances the cum ack point
      SendBufferDequeueUpTo(spNonRenegSackChunk->uiCumAck);
      uiCumAckPoint = spNonRenegSackChunk->uiCumAck; // Advance the cumAck pointer
    }

  /****** Begin CMT Change ******/
  /* CMT change In case of reneg, or reordered SACKs where previously
   * present gapack info is now absent, arwnd info can be stale and
   * dangerous if used without consideration to the fact that the receiver
   * has apparently reneged on data. Note that ProcessGapAckBlocks was
   * previously not called when gapacks were absent in the SACK. What if
   * previously present gapacks were reneged? Since we do not reneg in
   * this implementation, this can happen only with reordered SACKs.  In
   * any case, ProcessGapAckBlocks() has code that will handle such
   * apparent reneging, i.e., calculate the correct new
   * outstanding_bytes. Therefore, this function should be called even if
   * there are no gapack blocks in the SACK chunk to handle renegs, or
   * reordered SACKs with no gapack info.
   *
   * if(spSackChunk->usNumGapAckBlocks != 0) // are there any gaps??
   * {
   *   eFastRtxNeeded = ProcessGapAckBlocks(ucpSackChunk, eNewCumAck);
   * } 
   */
  /* JRI-TODO: Move change to SCTP code. This problem can be caused by 
   * reordered SACKs too.The SCTP code handles calculation of outstanding bytes
   * when SACK is reordered, so this check should be removed there too.
   */
  eFastRtxNeeded = ProcessGapAckBlocks(ucpNonRenegSackChunk, eNewCumAck);

  /* PN: NR-SACKs. NonRenegSackBlock processing assumes GapAck blocks in the
   * sack chunk have already been processed
   */
  /* PN: 5/2007. NR-Sacks */
  /* Process the NR-SACKed TSNs in the send buffer. 
   */
  if (usNumNonRenegGapAckBlocks != 0)
    
    /* NonRenegSackBlock processing assumes GapAck blocks in the
     * sack chunk have already been processed
     */
    ProcessNonRenegSackBlocks(ucpNonRenegSackChunk);
  

  /* TEMP-FIX: Calculate the correct outstanding bytes in each dest, since
   * even data that is marked for rtx is still considered oustanding. The
   * value of outstanding_bytes is restored later below. Need this to stop
   * T3 timers correctly.  This problem needs to be fixed here, and in SCTP.
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
            -= spCurrChunk->sHdr.usLength;
        }
    }
  /****** End CMT Change ******/
  
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;

      /****** Begin CMT Change ******/
      /* Adjust cwnd if sack advanced the pseudo-cumack point AND this
       * destination has newly acked bytes. Also, we MUST adjust our
       * congestion window BEFORE we update the number of outstanding
       * bytes to reflect the newly acked bytes in received SACK.  
       */
      DBG_PL(ProcessNonRenegSackChunk, "eNewPseudoCum=%d, newlyacked=%d"), 
	spCurrDestNodeData->eNewPseudoCum, 
	spCurrDestNodeData->iNumNewlyAckedBytes DBG_PR;

      if (eUseCmtCwnd == TRUE) 
	{
	  if((spCurrDestNodeData->eNewPseudoCum == TRUE) && 
	     (spCurrDestNodeData->iNumNewlyAckedBytes > 0))
	    AdjustCwnd(spCurrDestNodeData);
	} 
      else if (eUseCmtCwnd == FALSE) 
	{
	  if(eNewCumAck == TRUE && spCurrDestNodeData->iNumNewlyAckedBytes > 0)
	    AdjustCwnd(spCurrDestNodeData);
	}
      /****** End CMT Change ******/

      /* The number of outstanding bytes is reduced by how many bytes this sack
       * acknowledges.
       */
      if(spCurrDestNodeData->iNumNewlyAckedBytes <=
	 spCurrDestNodeData->iOutstandingBytes)
	{
	  spCurrDestNodeData->iOutstandingBytes 
	    -= spCurrDestNodeData->iNumNewlyAckedBytes;
	}
      else
	spCurrDestNodeData->iOutstandingBytes = 0;

      DBG_PL(ProcessNonRenegSackChunk,"Dest #%d (%d:%d) (%p): outstanding=%d, cwnd=%d"),
	++i, spCurrDestNodeData->iNsAddr, spCurrDestNodeData->iNsPort,
	spCurrDestNodeData, spCurrDestNodeData->iOutstandingBytes, 
	spCurrDestNodeData->iCwnd DBG_PR;

      if(spCurrDestNodeData->iOutstandingBytes == 0)
	{
	  /* All outstanding data has been acked
	   */
	  spCurrDestNodeData->iPartialBytesAcked = 0;  // section 7.2.2

	  /* section 6.3.2.R2
	   */
	  if(spCurrDestNodeData->eRtxTimerIsRunning == TRUE)
	    {
	      DBG_PL(ProcessNonRenegSackChunk, "Dest #%d (%p): stopping timer"), 
		i, spCurrDestNodeData DBG_PR;
	      StopT3RtxTimer(spCurrDestNodeData);
	    }
	}

      /* section 6.3.2.R3 - Restart timers for destinations that have
       * acknowledged their first outstanding (ie, no timer running) and
       * still have outstanding data in flight.  
       */
      if(spCurrDestNodeData->iOutstandingBytes > 0 &&
	 spCurrDestNodeData->eRtxTimerIsRunning == FALSE)
	{
	  StartT3RtxTimer(spCurrDestNodeData);
	}
    }

  /****** Begin CMT Change ******/
  /* TEMP-FIX: Restore value of outstanding_bytes. See TEMP-FIX comment
   * above for details.
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
  /****** End CMT Change ******/

  DBG_F(ProcessNonRenegSackChunk, DumpSendBuffer());

  AdvancePeerAckPoint();

  /****** Begin CMT Change ******/
  uiArwnd = spNonRenegSackChunk->uiArwnd;
  /****** End CMT Change ******/

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
  if(uiTotalOutstanding <= spNonRenegSackChunk->uiArwnd)
    uiPeerRwnd = (spNonRenegSackChunk->uiArwnd  - uiTotalOutstanding);
  else
    uiPeerRwnd = 0;

  tiRwnd++; // trigger changes to be traced

  DBG_PL(ProcessNonRenegSackChunk, "uiPeerRwnd=%d, uiArwnd=%d"), uiPeerRwnd, 
    spNonRenegSackChunk->uiArwnd DBG_PR;
  DBG_X(ProcessNonRenegSackChunk);
}


int SctpCMTAgent::ProcessChunk(u_char *ucpInChunk, u_char **ucppOutData)
{
  DBG_I(ProcessChunk);
  int iThisOutDataSize = 0;
  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;
  double dCurrTime = Scheduler::instance().clock();
  double dTime;
  SctpHeartbeatAckChunk_S *spHeartbeatChunk = NULL;
  SctpHeartbeatAckChunk_S *spHeartbeatAckChunk = NULL;

  /****** Begin CMT Change ******/
  Boolean_E eCmtPFCwndChange = FALSE;
  u_int uiTotalOutstanding = 0;
  Boolean_E eThisDestWasInactive = FALSE; /* For triggering data tx in CMT */
  /****** End CMT Change ******/
  Boolean_E eThisDestWasUnconfirmed = FALSE;
  Boolean_E eFoundUnconfirmedDest = FALSE;

  switch(eState)
    {
    case SCTP_STATE_CLOSED:
      switch( ((SctpChunkHdr_S *)ucpInChunk)->ucType)
	{
	case SCTP_CHUNK_INIT:
	  DBG_PL(ProcessChunk, "got INIT!! ...sending INIT_ACK") DBG_PR;
	  ProcessInitChunk(ucpInChunk);
	  iThisOutDataSize = GenChunk(SCTP_CHUNK_INIT_ACK, *ucppOutData);
	  *ucppOutData += iThisOutDataSize;
	  /* stay in the closed state */
	  break;

	case SCTP_CHUNK_COOKIE_ECHO:
	  DBG_PL(ProcessChunk, 
		 "got COOKIE_ECHO!! (established!) ...sending COOKIE_ACK")
	    DBG_PR;
	  ProcessCookieEchoChunk( (SctpCookieEchoChunk_S *) ucpInChunk );
	  iThisOutDataSize = GenChunk(SCTP_CHUNK_COOKIE_ACK, *ucppOutData);
	  *ucppOutData += iThisOutDataSize;
	  eState = SCTP_STATE_ESTABLISHED;

	  if(uiHeartbeatInterval != 0)
	    {
	      dTime = CalcHeartbeatTime(spPrimaryDest->dRto);
	      opHeartbeatGenTimer->force_cancel();
	      opHeartbeatGenTimer->resched(dTime);
	      opHeartbeatGenTimer->dStartTime = dCurrTime;

	      for(spCurrNode = sDestList.spHead;
		  spCurrNode != NULL;
		  spCurrNode = spCurrNode->spNext)
		{
		  spCurrDest = (SctpDest_S *) spCurrNode->vpData;
		  spCurrDest->dIdleSince = dCurrTime;
		}
	    }

	  break;
	  
  	default:
	  /* ALC 1/25/2002
	   *
	   * no error statement here, because there are times when this could
	   * occur due to abrupt disconnections via the "reset" command. how?
	   * well, "reset" resets all the association state. however, there may
	   * still be packets in transit. if and when those packets arrive,they
	   * will be unexpected packets since the association is closed. since
	   * this is a simulation, it shouldn't be a problem. however, if an 
	   * application needs a more graceful shutdown, we would need to 
	   * implement sctp's proper shutdown procedure. until the need arises,
	   * we won't do it. instead, what do we do? ignore the "unexpected"
	   * packet.
	   */
	  DBG_PL(ProcessChunk, "association closed... ignoring chunk %s"), 
	    "(not COOKIE_ECHO or INIT)" DBG_PR;
	  break;
	}
      break;
	  
    case SCTP_STATE_COOKIE_WAIT:
      DBG_PL(ProcessChunk, "got INIT_ACK!! ...sending COOKIE_ECHO") DBG_PR;
      ProcessInitAckChunk(ucpInChunk);
      iThisOutDataSize = GenChunk(SCTP_CHUNK_COOKIE_ECHO, *ucppOutData);
      *ucppOutData += iThisOutDataSize;
      opT1CookieTimer->resched(spPrimaryDest->dRto);
      eState = SCTP_STATE_COOKIE_ECHOED;
      break;

    case SCTP_STATE_COOKIE_ECHOED:
      DBG_PL(ProcessChunk, "got COOKIE_ACK!! (established!) ...sending DATA")
	DBG_PR;
      ProcessCookieAckChunk( (SctpCookieAckChunk_S *) ucpInChunk );
      eSendNewDataChunks = TRUE;
      eState = SCTP_STATE_ESTABLISHED;

      /* PN: Confirming Destinations 
       * RFC allows to send data to a confirmed destination.
       * However, this implementation is more conservative than
       * the RFC, since, data is sent to any destionation only if
       * all destinations have been confirmed. 
       */ 
      for(spCurrNode = sDestList.spHead;
	  spCurrNode != NULL;
	  spCurrNode = spCurrNode->spNext)
	{
	  spCurrDest = (SctpDest_S *) spCurrNode->vpData;
	  spCurrDest->eStatus = SCTP_DEST_STATUS_UNCONFIRMED;
	  
	   /* Send Heartbeat to confirm this dest and get RTT */
	  SendHeartbeat(spCurrDest);
	}
      
      eSendNewDataChunks = FALSE;
      
      break;

    case SCTP_STATE_ESTABLISHED:
      switch( ((SctpChunkHdr_S *)ucpInChunk)->ucType)
	{
	case SCTP_CHUNK_DATA:
	  DBG_PL(ProcessChunk, "got DATA (TSN=%d)!!"), 
	    ((SctpDataChunkHdr_S *)ucpInChunk)->uiTsn DBG_PR;

	  if(eUseDelayedSacks == FALSE) // are we doing delayed sacks?
	    {
	      /* NO, so generate sack immediately!
	       */
	      eSackChunkNeeded = TRUE;
	    }
	  else  // we are doing delayed sacks, so...
	    {
	      /* rfc2960 section 6.2 - determine if a SACK will be generated
	       */
	      if(eStartOfPacket == TRUE)  
		{
		  eStartOfPacket = FALSE;  // reset
		  iDataPktCountSinceLastSack++;

		  if(iDataPktCountSinceLastSack == 1)   
		    {
		      opSackGenTimer->resched(dSackDelay);
		    }
		  else if(iDataPktCountSinceLastSack == DELAYED_SACK_TRIGGER)
		    {
		      /****** Begin CMT Change ******/
		      /* CMT change: Do not reset packet count yet !  Pkt
		       * count is put into SACK flags for CMT delayed ack
		       * algo. Hence reset only after putting into SACK
		       * flags.
		       *
		       * iDataPktCountSinceLastSack = 0; // reset
		       */
		      /****** End CMT Change ******/
		      opSackGenTimer->force_cancel();
		      eSackChunkNeeded = TRUE;
		    }
		}
	    }

	  ProcessDataChunk( (SctpDataChunkHdr_S *) ucpInChunk );

	  /****** Begin CMT Change ******/
	  /* section 6.7 - There is at least one "gap in the received DATA
	   * chunk sequence", so let's ensure we send a SACK immediately!
	   */
	  /* If CMT DAC algo, always delay SACKs; sender has the
	   * responsibility of handling missing report counts according to
	   * number of packets acked.
	   */
	  if((eUseCmtDelAck == FALSE) && (sRecvTsnBlockList.uiLength > 0))
	    {
	      iDataPktCountSinceLastSack = 0; // reset
	      opSackGenTimer->force_cancel();
	      eSackChunkNeeded = TRUE;
	    }
	  /****** End CMT Change ******/

	  /* no state change 
	   */	  
	  break;

	case SCTP_CHUNK_SACK:
	  DBG_PL(ProcessChunk, "got SACK (CumAck=%d)!!"),
	    ((SctpSackChunk_S *)ucpInChunk)->uiCumAck DBG_PR;

	  ProcessSackChunk(ucpInChunk);

	  /* Do we need to transmit a FORWARD TSN chunk??
	   */
	  if(uiAdvancedPeerAckPoint > uiCumAckPoint)
	    eForwardTsnNeeded = TRUE;

	  eSendNewDataChunks = TRUE;
	  break; // no state change

	/* PN: 12/20/2007. NR-Sacks */
	case SCTP_CHUNK_NRSACK:
	  DBG_PL(ProcessChunk, "got NR-SACK (CumAck=%d)!!"),
	    ((SctpNonRenegSackChunk_S *)ucpInChunk)->uiCumAck DBG_PR;

	  ProcessNonRenegSackChunk(ucpInChunk);

	  /* Do we need to transmit a FORWARD TSN chunk??
	   */
	  if(uiAdvancedPeerAckPoint > uiCumAckPoint)
	    eForwardTsnNeeded = TRUE;

	  eSendNewDataChunks = TRUE;
	  break; // no state change

	case SCTP_CHUNK_FORWARD_TSN:
	  DBG_PL(ProcessChunk, "got FORWARD TSN (tsn=%d)!!"),
	    ((SctpForwardTsnChunk_S *) ucpInChunk)->uiNewCum DBG_PR;

	  ProcessForwardTsnChunk( (SctpForwardTsnChunk_S *) ucpInChunk );
	  break; // no state change

	case SCTP_CHUNK_HB:
	  DBG_PL(ProcessChunk, "got HEARTBEAT!!") DBG_PR;

	  /* GenChunk() doesn't copy HB info
	   */
	  iThisOutDataSize = GenChunk(SCTP_CHUNK_HB_ACK, *ucppOutData);

	  /* ...so we copy it here!
	   */
	  spHeartbeatChunk = (SctpHeartbeatAckChunk_S *) ucpInChunk;
	  spHeartbeatAckChunk = (SctpHeartbeatAckChunk_S *) *ucppOutData;
	  spHeartbeatAckChunk->dTimestamp = spHeartbeatChunk->dTimestamp;
	  spHeartbeatAckChunk->spDest = spHeartbeatChunk->spDest;
	  *ucppOutData += iThisOutDataSize;
	  break; // no state change

	case SCTP_CHUNK_HB_ACK:
	  DBG_PL(ProcessChunk, "got HEARTBEAT ACK!!") DBG_PR;

	  /****** Begin CMT Change ******/

	  spHeartbeatAckChunk = (SctpHeartbeatAckChunk_S *) ucpInChunk;

	  /* CMT-PF: Is HB-ACK on a PF path?
	   */
	  if (spHeartbeatAckChunk->spDest->eStatus ==
	      SCTP_DEST_STATUS_POSSIBLY_FAILED)
	    {
	      eCmtPFCwndChange = TRUE;
	      DBG_PL(ProcessChunk, "HB-ACK: dest:%p, status:%s"), 
		spHeartbeatAckChunk->spDest, 
		PrintDestStatus(spHeartbeatAckChunk->spDest) DBG_PR;
	    }
	  
	  if (spHeartbeatAckChunk->spDest->eStatus ==
	      SCTP_DEST_STATUS_INACTIVE)
	    eThisDestWasInactive = TRUE;

 	  /* CMT-PF: Are there any non-PF destinations? Need this info
	   * for HB timer processing further below.
	   */
          for(spCurrNode = sDestList.spHead;
                  spCurrNode != NULL;
                  spCurrNode = spCurrNode->spNext)
                {
                  spCurrDest = (SctpDest_S *) spCurrNode->vpData;
		  if (spCurrDest->eStatus != SCTP_DEST_STATUS_POSSIBLY_FAILED)
		    {
		      DBG_PL(ProcessChunk, "Found non PF dest=%p"), 
			spCurrDest DBG_PR;
		      /* break; */
		    }
               	}


          /* PN: Is this destination unconfirmed?
	   */
	  if (spHeartbeatAckChunk->spDest->eStatus ==
	      SCTP_DEST_STATUS_UNCONFIRMED)
		eThisDestWasUnconfirmed = TRUE;
	  
	  ProcessHeartbeatAckChunk( (SctpHeartbeatAckChunk_S *) ucpInChunk);

	  /* PN: If this destination was unconfirmed and all other
	   * destinations have been confirmed, then allow data
	   * transmission on the association
           */
	  if (eThisDestWasUnconfirmed == TRUE) {
          	for(spCurrNode = sDestList.spHead;
                  spCurrNode != NULL;
                  spCurrNode = spCurrNode->spNext)
             	{
                  spCurrDest = (SctpDest_S *) spCurrNode->vpData;
		  if (spCurrDest->eStatus == SCTP_DEST_STATUS_UNCONFIRMED)
		    {
		      eFoundUnconfirmedDest = TRUE;
		      DBG_PL(ProcessChunk, "dest=%p UNCONFIRMED"), 
			spCurrDest DBG_PR;
		      break;
		    }
		}
	  	
		if (eFoundUnconfirmedDest == TRUE)
			break; /* From case HB_ACK chunk */
		else {
			
		   /* All destinations have been confirmed.
		    * Start new data transfer
		    */
		   eSendNewDataChunks = TRUE;	

		   /* Process HeartbeatTimers for the association. 
		    * This code was originally in the COOKIE ACK case above  
		    */ 
      		   if(uiHeartbeatInterval != 0)
		     {
		       dTime = CalcHeartbeatTime(spPrimaryDest->dRto);
		       opHeartbeatGenTimer->force_cancel();
		       opHeartbeatGenTimer->resched(dTime);
		       opHeartbeatGenTimer->dStartTime = dCurrTime;

		       for(spCurrNode = sDestList.spHead;
			   spCurrNode != NULL;
			   spCurrNode = spCurrNode->spNext)
			 {
			   spCurrDest = (SctpDest_S *) spCurrNode->vpData;
			   spCurrDest->dIdleSince = dCurrTime;
			 }
		     } 

		  break; /* From case HB_ACK */
		  
		} /* End else eFoundUnconfirmedDest == TRUE */
		
	  } /* End of if (eThisDestWasUnconfirmed == TRUE) */
	  
	  /* CMT-PF: Set cwnd to uiCmtPFCwnd MTUs if HB-ack was received from a
	   * destination marked PF
	   */
	  if ((eCmtPFCwndChange == TRUE) || (eThisDestWasInactive == TRUE))
	    {
	      if (eCmtPFCwndChange == TRUE)
		{
		  
		  spHeartbeatAckChunk->spDest->iCwnd 
		    = uiCmtPFCwnd*uiMaxDataSize;
		  DBG_PL(ProcessChunk, "dest=%p, CMTPFCwnd=%ld"), 
		    spHeartbeatAckChunk->spDest, 
		    spHeartbeatAckChunk->spDest->iCwnd DBG_PR;
		  //tiCwnd++; // for trace
	      
		  spHeartbeatAckChunk->spDest->dPFSince = 0; //unset
		}

	      /* CMT: Since all paths were INACTIVE and one of them has
	       * become ACTIVE now. First send retransmissions to the
	       * ACTIVE destination. 
	       *
	       * CMT-PF: Since a PF destination has become ACTIVE, first 
	       * send retransmissions to the ACTIVE destination. 
	       */

	      TimeoutRtx(spHeartbeatAckChunk->spDest);
	      
	      /* CMT, CMT-PF: If no marked chunks, send new data 
	       */
	      if (AnyMarkedChunks() == FALSE) 
		{
		  /* All marked chunks are in flight or have been sacked.
		   * If they are in flight, then peerRwnd does not reflect
		   * these outstanding bytes. To recap: MarkChunksForRtx
		   * marks chunks and adds their size to
		   * peerRwnd. RtxMarkedChunks does not decrement peerRwnd
		   * when these chunks are rtxmed. So right now, we have
		   * incorrect view of peerRwnd.  If peerRwnd is > than
		   * what it should be, sending new data in the space will
		   * cause problems. (How?: rtx tsns were lost and before
		   * this was detected, new data sent fills up
		   * peerRwnd. In that case, recv silently discards later
		   * rtxms and sender keeps timing out on that rtxms.
		   * Deadlock !)
		   *
		   * So update peerRwnd based on the last advertised
		   * receiver window. 
		   */
		  uiTotalOutstanding = TotalOutstanding();
		  if(uiTotalOutstanding <= uiArwnd)
		    uiPeerRwnd = (uiArwnd  - uiTotalOutstanding);
		  else
		    uiPeerRwnd = 0;
		  
		  DBG_PL(ProcessChunk,"uiPeerRwnd=%d, uiArwnd=%d"), uiPeerRwnd,
		    uiArwnd DBG_PR;
		  
		  /* Set flag for SendMuch to send new data. 
		   */
		  eSendNewDataChunks = TRUE;
		} 
	     }
	  	  
	  break; // no state change

	case SCTP_CHUNK_INIT:
	  DBG_PL(ProcessChunk, "unexpected chunk type (INIT) at %f"),
	    dCurrTime DBG_PR;
	  printf("[ProcessChunk] unexpected chunk type (INIT) at %f\n",
	    dCurrTime);
	  break;

	case SCTP_CHUNK_INIT_ACK:
	  DBG_PL(ProcessChunk, "unexpected chunk type (INIT_ACK) at %f"),
	    dCurrTime DBG_PR;
	  printf("[ProcessChunk] unexpected chunk type (INIT_ACK) at %f\n",
	    dCurrTime);
	  break;

	  /* even though the association is established,COOKIE_ECHO needs to be
	   * handled because the peer may have not received the COOKIE_ACK.
	   *
	   * Note: we don't follow the rfc's complex process for handling this
	   * case, because we don't deal with tie-tags, etc in simulation. :-)
	   */
	case SCTP_CHUNK_COOKIE_ECHO:
	  DBG_PL(ProcessChunk,
		 "got COOKIE_ECHO!! (established!) ...sending COOKIE_ACK")
	    DBG_PR;
	  ProcessCookieEchoChunk( (SctpCookieEchoChunk_S *) ucpInChunk);
	  iThisOutDataSize = GenChunk(SCTP_CHUNK_COOKIE_ACK, *ucppOutData);
	  *ucppOutData += iThisOutDataSize;
	  break;

	case SCTP_CHUNK_COOKIE_ACK:
	  DBG_PL(ProcessChunk, "unexpected chunk type (COOKIE_ACK) at %f"),
	    dCurrTime DBG_PR;
	  printf("[ProcessChunk] unexpected chunk type (COOKIE_ACK) at %f\n",
		 dCurrTime);
	  break;

	default:
	  ProcessOptionChunk(ucpInChunk);
	  break;
	}
      break;

    case SCTP_STATE_UNINITIALIZED:
    case SCTP_STATE_SHUTDOWN_SENT:
    case SCTP_STATE_SHUTDOWN_RECEIVED:
    case SCTP_STATE_SHUTDOWN_ACK_SENT:
    case SCTP_STATE_SHUTDOWN_PENDING:
      break;
    }

  DBG_X(ProcessChunk);
  return iThisOutDataSize;
}

/* This function is called as soon as we are done processing a SACK which 
 * notifies the need of a fast rtx. Following RFC2960, we pack as many chunks
 * as possible into one packet (PTMU restriction). The remaining marked packets
 * are sent as soon as cwnd allows.
 */
void SctpCMTAgent::FastRtx()
{
  DBG_I(FastRtx);

  Node_S *spCurrDestNode = NULL;
  SctpDest_S *spCurrDestData = NULL;
  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffData = NULL;

  /* be sure we clear all the eCcApplied flags before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestData->eCcApplied = FALSE;

      /****** Begin CMT Change *******/
      /* JRI-TODO: Why do I need RTX_LIMIT_ZERO? Why not use LIMIT_CWND as
       * the default?
       */
      spCurrDestData->eRtxLimit = RTX_LIMIT_ZERO;
      /****** End CMT Change *******/
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

      /* If the chunk has been either marked for rtx or advanced ack, we want
       * to apply congestion control (assuming we didn't already).
       *
       * Why do we do it for advanced ack chunks? Well they were advanced ack'd
       * because they were lost. The ONLY reason we are not fast rtxing them is
       * because the chunk has run out of retransmissions (u-sctp). So we need
       * to still account for the fact they were lost... so apply congestion
       * control!
       */

      /****** Begin CMT Change *******/
      /* CMT: use per-destination uiRecover
       */
      if((spCurrBuffData->eMarkedForRtx != NO_RTX ||
	  spCurrBuffData->eAdvancedAcked == TRUE) &&
	 spCurrBuffData->spDest->eCcApplied == FALSE &&
	 spCurrBuffData->spChunk->uiTsn > spCurrBuffData->spDest->uiRecover)
	/****** End CMT Change *******/
	{ 
	  /* section 7.2.3 of rfc2960 (w/ implementor's guide) 
	   */
	  spCurrBuffData->spDest->iSsthresh 
	    = MAX(spCurrBuffData->spDest->iCwnd/2, 
		  iInitialCwnd * (int) uiMaxDataSize);
	  spCurrBuffData->spDest->iCwnd = spCurrBuffData->spDest->iSsthresh;
	  spCurrBuffData->spDest->iPartialBytesAcked = 0; //reset
	  tiCwnd++; // trigger changes for trace to pick up
	  spCurrBuffData->spDest->eCcApplied = TRUE;

	  /* Cancel any pending RTT measurement on this
	   * destination. Stephan Baucke (2004-04-27) suggested this
	   * action as a fix for the following simple scenario:
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
	  spCurrBuffData->spDest->eRtoPending = FALSE; 

	  /****** Begin CMT Change *******/
	  /* Set the recover variable to avoid multiple cwnd cuts for losses
	   * in the same window (ie, round-trip).
	   */
	  spCurrBuffData->spDest->uiRecover = 
	    GetHighestOutstandingTsn(spCurrBuffData->spDest);
	  /****** End CMT Change *******/
	}

      /****** Begin CMT Change *******/
      /* To specify which destinationto apply LIMIT_ONE_PACKET to. 
       * Earlier, every destination got one packet rtx out when 
       * rtxmarkedchunks was called with LIMIT_ONE_PACKET.
       */
      if(spCurrBuffData->eMarkedForRtx == FAST_RTX)
      {
	spCurrBuffData->spDest->eRtxLimit = RTX_LIMIT_ONE_PACKET;
      }
      /****** End CMT Change *******/
    }

  /****** Begin CMT Change *******/
  eMarkedChunksPending = AnyMarkedChunks();
  /****** End CMT Change *******/

  /* possible that no chunks are pending retransmission since they could be 
   * advanced ack'd 
   */
  if(eMarkedChunksPending == TRUE)
    RtxMarkedChunks(RTX_LIMIT_ONE_PACKET);

  DBG_X(FastRtx);
}


void SctpCMTAgent::MarkChunkForRtx(SctpSendBufferNode_S *spNodeData, 
				   MarkedForRtx_E eMarkedForRtx)
{
  DBG_I(MarkChunkForRtx);

  SctpDataChunkHdr_S  *spChunk = spNodeData->spChunk;
  SctpOutStream_S *spStream = &(spOutStreams[spChunk->usStreamId]);

  DBG_PL(MarkChunkForRtx, "tsn=%lu eMarkedForRtx=%s"), 
    spChunk->uiTsn,
    !eMarkedForRtx ? "NO_RTX" 
    : (eMarkedForRtx==FAST_RTX ? "FAST_RTX": "TIMEOUT_RTX") DBG_PR;

  spNodeData->eMarkedForRtx = eMarkedForRtx;
  uiPeerRwnd += spChunk->sHdr.usLength; // 6.2.1.C1 

  /* let's see if this chunk is on an unreliable stream.if so and the chunk has
   * run out of retransmissions,mark it as advanced acked and unmark it for rtx
   */
  if(spStream->eMode == SCTP_STREAM_UNRELIABLE)
    {
      /* have we run out of retransmissions??
       */
      if(spNodeData->iNumTxs > spNodeData->iUnrelRtxLimit)
	{
	  DBG_PL(MarkChunkForRtx, "giving up on tsn %lu..."),
	    spChunk->uiTsn DBG_PR;

	  spNodeData->eAdvancedAcked = TRUE;
	  spNodeData->eMarkedForRtx = NO_RTX;
	  spNodeData->spDest->iOutstandingBytes -= spChunk->sHdr.usLength;
	}
    }
  
  if(spNodeData->eMarkedForRtx != NO_RTX)
    {
      eMarkedChunksPending = TRUE;

      /*** Begin CMT change ***/
      /* Also set eMarkedChunksPending for corresp. destination
       */
      spNodeData->spDest->eMarkedChunksPending = TRUE;
      /*** End of CMT change ***/
    }

  DBG_PL(MarkChunkForRtx, "uiPeerRwnd=%lu"), uiPeerRwnd DBG_PR;
  DBG_X(MarkChunkForRtx);
}

void SctpCMTAgent::recv(Packet *opInPkt, Handler*)
{
  /* Let's make sure that a Reset() is called, because it isn't always
   * called explicitly with the "reset" command. For example, wireless
   * nodes don't automatically "reset" their agents, but wired nodes do. 
   */
  if(eState == SCTP_STATE_UNINITIALIZED)
    Reset(); 

  DBG_I(recv);

  hdr_ip *spIpHdr = hdr_ip::access(opInPkt);
  PacketData *opInPacketData = (PacketData *) opInPkt->userdata();
  u_char *ucpInData = opInPacketData->data();
  u_char *ucpCurrInChunk = ucpInData;
  int iRemainingDataLen = opInPacketData->size();

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  u_char *ucpCurrOutData = ucpOutData;

  /* local variable which maintains how much data has been filled in the 
   * current outgoing packet
   */
  int iOutDataSize = 0; 

  memset(ucpOutData, 0, uiMaxPayloadSize);
  memset(spSctpTrace, 0,
	 (uiMaxPayloadSize / sizeof(SctpChunkHdr_S)) * sizeof(SctpTrace_S) );

  spReplyDest = GetReplyDestination(spIpHdr);

  eStartOfPacket = TRUE;

  do
    {
      DBG_PL(recv, "iRemainingDataLen=%d"), iRemainingDataLen DBG_PR;

      /* processing chunks may need to generate response chunks, so the
       * current outgoing packet *may* be filled in and our out packet's data
       * size is incremented to reflect the new data
       */
      iOutDataSize += ProcessChunk(ucpCurrInChunk, &ucpCurrOutData);
      NextChunk(&ucpCurrInChunk, &iRemainingDataLen);
    }
  while(ucpCurrInChunk != NULL);

  /* Let's see if we have any response chunks(currently only handshake related)
   * to transmit. 
   *
   * Note: We don't bundle these responses (yet!)
   */
  if(iOutDataSize > 0) 
    {
      SendPacket(ucpOutData, iOutDataSize, spReplyDest);
      DBG_PL(recv, "responded with control chunk(s)") DBG_PR;
    }

  /* Let's check to see if we need to generate and send a SACK chunk.
   *
   * Note: With uni-directional traffic, SACK and DATA chunks will not be 
   * bundled together in one packet.
   * Perhaps we will implement this in the future?  
   */
  if(eSackChunkNeeded == TRUE)
    {
      memset(ucpOutData, 0, uiMaxPayloadSize);
      iOutDataSize = BundleControlChunks(ucpOutData);

      /* PN: 12/20/2007
       * Send NR-Sacks instead of Sacks?
       */
      if (eUseNonRenegSacks == TRUE)
	{
	  iOutDataSize += GenChunk(SCTP_CHUNK_NRSACK, ucpOutData+iOutDataSize);
	}
      else
	{
	  iOutDataSize += GenChunk(SCTP_CHUNK_SACK, ucpOutData+iOutDataSize);
	}
      
      SendPacket(ucpOutData, iOutDataSize, spReplyDest);

      if (eUseNonRenegSacks == TRUE) 
	{
	  DBG_PL(recv, "NR-SACK sent (%d bytes)"), iOutDataSize DBG_PR;
	}
      else
	{
	  DBG_PL(recv, "SACK sent (%d bytes)"), iOutDataSize DBG_PR;
	}

      eSackChunkNeeded = FALSE;  // reset AFTER sent (o/w breaks dependencies)
    }

  /* Do we need to transmit a FORWARD TSN chunk??
   */
  if(eForwardTsnNeeded == TRUE)
    {
      memset(ucpOutData, 0, uiMaxPayloadSize);
      iOutDataSize = BundleControlChunks(ucpOutData);
      iOutDataSize += GenChunk(SCTP_CHUNK_FORWARD_TSN,ucpOutData+iOutDataSize);
      SendPacket(ucpOutData, iOutDataSize, spNewTxDest);
      DBG_PL(recv, "FORWARD TSN chunk sent") DBG_PR;
      eForwardTsnNeeded = FALSE; // reset AFTER sent (o/w breaks dependencies)
    }


  /***** Begin CMT Change *****/
  /* CMT change: We do not check to see if any rtxs are pending to be
   * sent.  Since rtxs can be sent to a different dest than the original,
   * the ordering rule of SCTP that rtxs need to be sent before txs is
   * blatantly ignored by CMT :)
   */
  /* Note: We aren't bundling with what was sent above, but we could. Just 
   * avoiding that for now... why? simplicity :-)
   */
  DBG_PL(recv, "eSendNewDataChunnks: %d"), eSendNewDataChunks DBG_PR;
  if(eSendNewDataChunks == TRUE)
    {
      SendMuch();       // Send new data till our cwnd is full!
      eSendNewDataChunks = FALSE; // reset AFTER sent (o/w breaks dependencies)
    }
  /***** End CMT Change *****/

  delete hdr_sctp::access(opInPkt)->SctpTrace();
  hdr_sctp::access(opInPkt)->SctpTrace() = NULL;
  Packet::free(opInPkt);
  opInPkt = NULL;
  DBG_X(recv);
  delete [] ucpOutData;
}

void SctpCMTAgent::SendMuch()
{
  DBG_I(SendMuch);
  DBG_PL(SendMuch, "eDataSource=%s"), 
    ( (eDataSource == DATA_SOURCE_APPLICATION)
      ? "DATA_SOURCE_APPLICATION" 
      : "DATA_SOURCE_INFINITE" )
    DBG_PR;

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  int iOutDataSize = 0;
  double dCurrTime = Scheduler::instance().clock();

  /* PN: 12/2007. Send window simulation */
  u_int uiChunkSize = 0;

  /****** Begin CMT Change ******/
  Node_S *spCurrNode = NULL;
  static Node_S *spNodeLastSentTo = NULL;
  Node_S *spLoopStartNode = NULL;
  SctpDest_S *spSavedTxDest = NULL;
  SctpDest_S *spTempDest = NULL;

  /* spNewDest is a var used throughout the program to maintain vars for
   * dest to which new transmissions are sent, such as cwnd. Since we
   * choose which dest new data is sent to during this loop, we save the
   * previous value of this var, and then set it back at the end of this
   * function.
   */
  spSavedTxDest = spNewTxDest;

  /* CMT-PF: If all destinations are marked PF, select one from them for
   * data transmission 
   */
  if (eUseCmtPF == TRUE)
    {
      spTempDest = SelectFromPFDests();
      if (spTempDest != NULL)
     	{
	  /* spTempDest was PF and changed to Active by SelectFromPFDests()
	   */

	   tiCountPFToActiveNewData++; // trace will pick up

	}
    }
  
  /* RR Scheduling: Start sending to the dest next to the one last sent to.
   */
  if (spNodeLastSentTo != NULL) 
    {
      spCurrNode = spNodeLastSentTo->spNext;
      if (spCurrNode == NULL) {
	spCurrNode = sDestList.spHead;
      }
    } 
  else 
    spCurrNode = sDestList.spHead;
  
  spLoopStartNode = spCurrNode;
  DBG_PL(SendMuch,"RR debug: Entering loop with dest=%p"), 
    (SctpDest_S *) spLoopStartNode->vpData DBG_PR;

  /* Sending to destinations as cwnd permits. Check to see if we can send
   * on any destination address. If there is space available in any cwnd,
   * we send. Hence this bigger loop.
   */
  do {
      iOutDataSize = 0;
      spNewTxDest = (SctpDest_S *) spCurrNode->vpData;

      DBG_PL(SendMuch,"RR debug: In send loop, trying for dest=%p"), 
        spNewTxDest DBG_PR;

      /* CMT-PF: Do not send to failed or PF destinations! 
       */
      if (spNewTxDest->eStatus != SCTP_DEST_STATUS_ACTIVE) 
	{
	  /* RR Scheduling: get next dest
	   */
	  spCurrNode = spCurrNode->spNext;
	  if (spCurrNode == NULL) 
	    spCurrNode = sDestList.spHead;
	  continue;
	}
      
      /* At this point, if there are chunks marked for rtx, no new data
       * will be transmitted anyways to this dest, since there will be no
       * cwnd space left. Note that sendmuch() is called after
       * rtxmarkedchunks() is called, therefore if an rtx could be sent to
       * this dest, it has been sent already. There is no point in
       * checking whether any rtxs are pending on this dest. 
       * (The reason this check was done at this point in SCTP was because
       * SCTP did not send any new data on any path when rtxs had to be
       * sent. Therefore, even if rtxs were outstanding on the alternate,
       * no new data could be sent to the primary.  In CMT, we ignore this
       * policy.)
       */
      /****** End CMT Change ******/
      
      /* Keep sending out packets until our cwnd is full!  The proposed
       * correction to RFC2960 section 6.1.B says "The sender may exceed
       * cwnd by up to (PMTU-1) bytes on a new transmission if the cwnd is
       * not currently exceeded". Hence, as long as cwnd isn't
       * full... send another packet.  
       *
       * Also, if our data source is the application layer (instead of the 
       * infinite source used for ftp), check if there is any data buffered 
       * from the app layer. If so, then send as much as we can.
       */
      while((spNewTxDest->iOutstandingBytes < spNewTxDest->iCwnd) &&
            (eDataSource == DATA_SOURCE_INFINITE || 
	     sAppLayerBuffer.uiLength != 0))
        {
	  DBG_PL(SendMuch, "Dest=%p, status=%s"), spNewTxDest, 
	    (spNewTxDest->eStatus == SCTP_DEST_STATUS_ACTIVE) ? "ACTIVE":"PF" 
	    DBG_PR;
	  DBG_PL(SendMuch, "uiAdvancedPeerAckPoint=%d uiCumAckPoint=%d"), 
	    uiAdvancedPeerAckPoint, uiCumAckPoint DBG_PR;
	  DBG_PL(SendMuch, "uiPeerRwnd=%d"), uiPeerRwnd DBG_PR;
	  DBG_PL(SendMuch, "spNewTxDest->iCwnd=%d"), spNewTxDest->iCwnd DBG_PR;
	  DBG_PL(SendMuch, "spNewTxDest->iPartialBytesAcked=%d"), 
	    spNewTxDest->iPartialBytesAcked DBG_PR;
	  DBG_PL(SendMuch, "spNewTxDest->iOutstandingBytes=%d"), 
	    spNewTxDest->iOutstandingBytes DBG_PR;
	  DBG_PL(SendMuch, "TotalOutstanding=%lu"), 
	    TotalOutstanding() DBG_PR;
	  DBG_PL(SendMuch, "eApplyMaxBurst=%s uiBurstLength=%d"),
	    eApplyMaxBurst ? "TRUE" : "FALSE", uiBurstLength DBG_PR;

	  /* PN: 12/2007. Simulating Send window */
	  uiChunkSize = GetNextDataChunkSize();
          if(uiChunkSize <= uiPeerRwnd)
            {
              /* This addresses the proposed change to RFC2960 section 7.2.4,
               * regarding using of Max.Burst. We have an option which allows
               * to control if Max.Burst is applied.
               */
	      /****** Begin CMT Change ******/
	      /* CMT change: per destination max.burst
	       */
              if(eUseMaxBurst == MAX_BURST_USAGE_ON)
                if((eApplyMaxBurst == TRUE) && 
		   ((spNewTxDest->uiBurstLength)++ >= MAX_BURST))
                  {
                    /* we've reached Max.Burst limit, so jump out of loop
                     */
		    DBG_PL(SendMuch,"Burstlength=%d - max.burst reached"),
                      spNewTxDest->uiBurstLength DBG_PR;
                    break;
                  }
              DBG_PL(SendMuch,"Burstlength=%d"), 
		spNewTxDest->uiBurstLength DBG_PR;
	      /****** End CMT Change ******/

  	      /* PN: 12/2007. Simulating Sendwindow */ 
	      if (uiInitialSwnd)
	      {
	        if (uiChunkSize > uiAvailSwnd) 
	        {
	      	  /* No send window space available to get application data */
	      	  DBG_PL(SendMuch, "Data ChunkSize=%ld > AvailSwnd=%d"), 
			uiChunkSize, uiAvailSwnd DBG_PR;
	      	  break; // from loop
	        }
	      }
	      
              memset(ucpOutData, 0, uiMaxPayloadSize); // reset
              iOutDataSize = BundleControlChunks(ucpOutData);
              iOutDataSize += GenMultipleDataChunks(ucpOutData+iOutDataSize,0);
              SendPacket(ucpOutData, iOutDataSize, spNewTxDest);
              DBG_PL(SendMuch, "DATA chunk(s) sent") DBG_PR;
            }
          else if(TotalOutstanding() == 0)  // section 6.1.A
            {
              /* probe for a change in peer's rwnd
               */
              memset(ucpOutData, 0, uiMaxPayloadSize);
              iOutDataSize = BundleControlChunks(ucpOutData);
              iOutDataSize += GenOneDataChunk(ucpOutData+iOutDataSize);
              SendPacket(ucpOutData, iOutDataSize, spNewTxDest);
              DBG_PL(SendMuch, "DATA chunk probe sent") DBG_PR;
            }
          else
            {
              break;
            }      
        }

      if(iOutDataSize > 0)  // did we send anything??
        {
          spNewTxDest->opCwndDegradeTimer->resched(spNewTxDest->dRto);
	  if(uiHeartbeatInterval != 0)
            {
              spNewTxDest->dIdleSince = dCurrTime;
            }

	  /****** Begin CMT Change ******/
          /* RR Scheduling: Maintain last dest data was sent to
           */
          spNodeLastSentTo = spCurrNode;
          DBG_PL(SendMuch,"RR debug: Sent to dest=%p, %d"), 
            spNewTxDest, iOutDataSize DBG_PR;
        }
      /* RR Scheduling: get next dest
       */
      spCurrNode = spCurrNode->spNext;
      if (spCurrNode == NULL)
        spCurrNode = sDestList.spHead;

  } while (spCurrNode != spLoopStartNode); // End of CMT destination loop

  /* Restore spNewTxDest */
  /* JRI-TODO: Check why spNewTxDest needs to be saved and restored.
   * This code can be cleaner if the value did not have to be maintained.
   */
  spNewTxDest = spSavedTxDest;

  /* this reset MUST happen at the end of the function, because the burst 
   * measurement is carried over from RtxMarkedChunks() if it was called.
   */
  /* CMT change: per destination burstlength
   */
  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      ((SctpDest_S*)spCurrNode->vpData)->uiBurstLength = 0; // reset
    }
  /****** End CMT Change ******/
    
  DBG_X(SendMuch);
  delete [] ucpOutData;
}

/* Handles timeouts for both DATA chunks and HEARTBEAT chunks. The actions are
 * slightly different for each. Covers rfc sections 6.3.3 and 8.3.
 */
void SctpCMTAgent::Timeout(SctpChunkType_E eChunkType, SctpDest_S *spDest)
{

  /*** Begin CMT change ***/
  SctpDest_S *spTraceDest;
  char cpOutString[500];
  /*** End CMT change ***/

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
      
      /* section 7.2.3 of rfc2960 (w/ implementor's guide v.02)
       */
      
      /* Why is there a conditional change to ssthresh below??
       * No information reg. the same in either  RFC or implementors guide.
       * During failure or high loss rates that result in back-to-back 
       * timeouts, ssthresh is not reduced after the first
       * timeout. Hence, CMT's RTX_SSTHRESH is at a disadvantage. 
       * NASIF FIX THIS: Confirm if this should be here. if not, remove
       * comment below. This might have to be fixed in sctp.cc's Timeout() as 
       * well.
       */
      /* Begin Change:
       *if(spDest->iCwnd > 1 * (int) uiMaxDataSize)
       * {
       *
       */
      spDest->iSsthresh 
	= MAX(spDest->iCwnd/2, iInitialCwnd * (int)uiMaxDataSize);
      spDest->iCwnd = 1*uiMaxDataSize;
      
      spDest->iPartialBytesAcked = 0; // reset
      tiCwnd++; // trigger changes for trace to pick up
      
       /* } End Change */

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

      /*** Begin CMT change ***/
      /* track data timeouts
       */
      spTraceDest = spDest;
      SetSource(spTraceDest); // gives us the correct source addr & port
      sprintf(cpOutString,
	      "time: %-8.5f  "
	      "saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d "
	      "DataTimeout, peerRwnd: %d rto: %-6.3f errCnt: %d \n",
	      dCurrTime,
	      addr(), port(), spTraceDest->iNsAddr, spTraceDest->iNsPort,
	      uiPeerRwnd, spTraceDest->dRto, spTraceDest->iErrorCount);
      if(channel_)
    	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));

      sprintf(cpOutString, "\n");
      if(channel_)
    	  (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
      /*** End CMT change ***/
    }

  DBG_PL(Timeout, "was spDest->dRto=%f"), spDest->dRto DBG_PR;
  spDest->dRto *= 2;    // back off the timer
  if(spDest->dRto > dMaxRto)
    spDest->dRto = dMaxRto;
  tdRto++;              // trigger changes for trace to pick up
  DBG_PL(Timeout, "now spDest->dRto=%f"), spDest->dRto DBG_PR;

  spDest->iTimeoutCount++;
  spDest->iErrorCount++; // @@@ window probe timeouts sould not be counted
  DBG_PL(Timeout, "now spDest->iErrorCount=%d"), spDest->iErrorCount DBG_PR;

  /*** Begin CMT change ***/
  if((spDest->eStatus == SCTP_DEST_STATUS_ACTIVE) || 
		(spDest->eStatus == SCTP_DEST_STATUS_POSSIBLY_FAILED))
    {  
      iAssocErrorCount++;
      DBG_PL(Timeout, "now iAssocErrorCount=%d"), iAssocErrorCount DBG_PR;

      if ((spDest->iErrorCount == 1) && (eUseCmtPF == TRUE))
	{
	  spDest->eStatus = SCTP_DEST_STATUS_POSSIBLY_FAILED;
	  spDest->dPFSince = dCurrTime;
	  DBG_PL(Timeout, "Dest=%p Marked PF at time: %-8.5f"), spDest,
	    spDest->dPFSince DBG_PR;
	  
	  if(uiHeartbeatInterval != 0)
	    {
	      spDest->opHeartbeatTimeoutTimer->force_cancel();
	      DBG_PL(Timeout, "Dest=%p Cancelled Heartbeat Timer"),
		spDest DBG_PR;
	    } 
	}
      /*** End CMT change ***/
      
      // Path.Max.Retrans exceeded?
      if(spDest->iErrorCount > (int) uiPathMaxRetrans) 
	{
	  spDest->eStatus = SCTP_DEST_STATUS_INACTIVE;
	  if(spDest == spNewTxDest)
	    {
	      spNewTxDest = GetNextDest(spNewTxDest);
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

  /*** Begin CMT change ***/
  if(eChunkType == SCTP_CHUNK_DATA)
    {
      TimeoutRtx(spDest);

      DBG_PL(Timeout, "Dest: %p, out: %d"), spDest, spDest->iOutstandingBytes
		DBG_PR; 
      if(spDest->eStatus == SCTP_DEST_STATUS_POSSIBLY_FAILED) 
      {
  	SendHeartbeat(spDest);
      } 
	
      if(spDest->eStatus == SCTP_DEST_STATUS_INACTIVE && 
	 uiHeartbeatInterval!=0)
	SendHeartbeat(spDest); // just marked inactive, so send HB immediately!
    }
  else if(eChunkType == SCTP_CHUNK_HB)
    {
      if((uiHeartbeatInterval != 0) ||
	 (spDest->eStatus == SCTP_DEST_STATUS_POSSIBLY_FAILED) ||
	 (spDest->eStatus == SCTP_DEST_STATUS_UNCONFIRMED))
	SendHeartbeat(spDest);
    }
  /*** End CMT change ***/

  DBG_X(Timeout);  
}

/* CMT change: changed function to get highest outstanding TSN per dest
 */
u_int SctpCMTAgent::GetHighestOutstandingTsn(SctpDest_S *spOutstandingOnDest)
{
  DBG_I(GetHighestOutstandingTsn);

  u_int uiHighestOutstandingTsn = 0;
  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffData = NULL;

  /* start from the tailof the send buffer and search towards the head for the 
   * first tsn oustanding... that's the highest outstanding tsn.
   */
  for(spCurrBuffNode = sSendBuffer.spTail;
      spCurrBuffNode != NULL;
      spCurrBuffNode = spCurrBuffNode->spPrev)
    {
      spCurrBuffData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;

      /****** Begin CMT Change ******/
      /* CMT change: changed function to get highest outstanding TSN per dest
       */
      if( (spCurrBuffData->spDest == spOutstandingOnDest)&&
	  (spCurrBuffData->eMarkedForRtx == NO_RTX))  // is it oustanding?
        {
          uiHighestOutstandingTsn = spCurrBuffData->spChunk->uiTsn; //found it!
          break;
        }
      /****** End CMT Change ******/
    }

  /* NE, NR-SACKs: Update the value of uiHighestOutstandignTSN 
   * if it is less than the destinations's uiHighestTsnSent
   */
  if (uiHighestOutstandingTsn < spOutstandingOnDest->uiHighestTsnSent ) 
    uiHighestOutstandingTsn = uiHighestTsnSent;
  else
    spOutstandingOnDest->uiHighestTsnSent = uiHighestOutstandingTsn;
  
  DBG_PL(GetHighestOutstandingTsn, "uiHighestOutstandingTsn=%d"), 
    uiHighestOutstandingTsn DBG_PR;
  DBG_PL(GetHighestOutstandingTsn, "uiHighestTsnSent=%d"), 
    spOutstandingOnDest->uiHighestTsnSent DBG_PR;
  
  DBG_X(GetHighestOutstandingTsn);
 
  return uiHighestOutstandingTsn;
}

/* PN: 12/21/2007. NR-SACKs.
 * New method to update last tsn sent to a particular destination.
 * Used to update HighestOutstandingTsn. 
 * CMT Change: Update for all destinations.
 */
void SctpCMTAgent::UpdateHighestTsnSent()
{
  DBG_I(UpdateHighestTsnSent);

  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffData = NULL;
  SctpDest_S *spDest = NULL;
  Node_S *spDestNode = NULL;


  /* start from the tail of the send buffer and search towards the head for the 
   * last tsn sent on the destination... 
   */

  for(spDestNode = sDestList.spHead;
      spDestNode != NULL;
      spDestNode = spDestNode->spNext)
    {
      spDest = (SctpDest_S *) spDestNode->vpData;

      for(spCurrBuffNode = sSendBuffer.spTail;
        spCurrBuffNode != NULL;
        spCurrBuffNode = spCurrBuffNode->spPrev)
      {
        spCurrBuffData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;

        if(spCurrBuffData->spDest == spDest)
        {

	    if (spCurrBuffData->spChunk->uiTsn > spDest->uiHighestTsnSent ) 
            spDest->uiHighestTsnSent = spCurrBuffData->spChunk->uiTsn;

	    break; // from for

	  }
      }

    DBG_PL(UpdateHighestTsnSent, "spDest=%p uiHighestTsnSent=%d"),
      spDest, spDest->uiHighestTsnSent DBG_PR;
      
    }
  
  DBG_X(UpdateHighestTsnSent);

}

void SctpCMTAgent::HeartbeatGenTimerExpiration(double dTimerStartTime)
{
  DBG_I(HeartbeatGenTimerExpiration);

  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrNodeData = NULL;
  Node_S *spLongestIdleNode = NULL;
  SctpDest_S *spLongestIdleNodeData = NULL;
  double dCurrTime = Scheduler::instance().clock();
  double dTime;

  DBG_PL(HeartbeatGenTimerExpiration, "Time: %-8.5f"), dCurrTime DBG_PR;
  
  DBG_PL(HeartbeatGenTimerExpiration, "finding the longest idle dest...") 
    DBG_PR;

  /* find the destination which has been idle the longest 
   */
  for(spCurrNode = spLongestIdleNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrNodeData = (SctpDest_S *) spCurrNode->vpData;
      spLongestIdleNodeData = (SctpDest_S *) spLongestIdleNode->vpData;
      
      DBG_PL(HeartbeatGenTimerExpiration, "spDest=%p idle since %f"), 
	spCurrNodeData, spCurrNodeData->dIdleSince DBG_PR;
	  
      if(spCurrNodeData->dIdleSince < spLongestIdleNodeData->dIdleSince)
	spLongestIdleNode = spCurrNode;
    }
  
  /* has it been idle long enough?
   */
  spLongestIdleNodeData = (SctpDest_S *) spLongestIdleNode->vpData;
  DBG_PL(HeartbeatGenTimerExpiration, "longest idle dest since %f"),
    spLongestIdleNodeData->dIdleSince DBG_PR;
  DBG_PL(HeartbeatGenTimerExpiration, "timer start time %f"),
    dTimerStartTime DBG_PR;
  
  /****** Begin CMT Change ******/
  /* For PF destinations, Timeout() (T3 timer expiration) function
   * sends out HBs. Snd HB here only if destination is not PF.
   */
  if(spLongestIdleNodeData->dIdleSince <= dTimerStartTime && 
     spLongestIdleNodeData->eStatus != SCTP_DEST_STATUS_POSSIBLY_FAILED )
    SendHeartbeat(spLongestIdleNodeData);
  else
    DBG_PL(HeartbeatGenTimerExpiration, 
	   "longest idle dest not idle long enough!") DBG_PR;
  /****** End CMT Change ******/
  
  /* start the timer again...
   */
  dTime = CalcHeartbeatTime(spLongestIdleNodeData->dRto);
  opHeartbeatGenTimer->resched(dTime);
  opHeartbeatGenTimer->dStartTime = dCurrTime;

  DBG_X(HeartbeatGenTimerExpiration);
}


void SctpCMTAgent::SackGenTimerExpiration() // section 6.2
{
  DBG_I(SackGenTimerExpiration);

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  int iOutDataSize = 0;
  memset(ucpOutData, 0, uiMaxPayloadSize);

  iDataPktCountSinceLastSack = 0; // reset 

  iOutDataSize = BundleControlChunks(ucpOutData);

  /* PN: 12/2007
   * Send NR-Sacks instead of Sacks?
   */
  if (eUseNonRenegSacks == TRUE) 
    {
     iOutDataSize += GenChunk(SCTP_CHUNK_NRSACK, ucpOutData+iOutDataSize);
    }
  else
    {
      iOutDataSize += GenChunk(SCTP_CHUNK_SACK, ucpOutData+iOutDataSize);
    }

  SendPacket(ucpOutData, iOutDataSize, spReplyDest); 

  if (eUseNonRenegSacks == TRUE)
    {
      DBG_PL(SackGenTimerExpiration, "NR-SACK sent (%d bytes)"), iOutDataSize DBG_PR;
    }
  else
    { 
      DBG_PL(SackGenTimerExpiration, "SACK sent (%d bytes)"), iOutDataSize DBG_PR;
    }
      
  /**** Begin CMT change ****/
  /* this variable should be reset only after GenChunk() is called.
   * GenChunk puts in the number of pkts into the SACK flags, which is
   * used by the sender to increment the missing report count (CMT DAC
   * algo). If this var is reset here, when a SACK is sent after SACK
   * timer expires, missing report count is incremented by 0 ! Moving line
   * to after SendPacket().
   */
   iDataPktCountSinceLastSack = 0; // reset 
  /**** End of CMT change ****/

  DBG_X(SackGenTimerExpiration);
  delete [] ucpOutData;
}


/****** Begin CMT Change ******/

/* New CMT function:
 * Function returns dest status as string for printing
 */
char* SctpCMTAgent::PrintDestStatus(SctpDest_S* spDest)
{
  switch(spDest->eStatus)
    {
    case SCTP_DEST_STATUS_ACTIVE:
      return "ACTIVE";
    case SCTP_DEST_STATUS_INACTIVE:
      return "INACTIVE";
    case SCTP_DEST_STATUS_POSSIBLY_FAILED:
      return "PF";
    case SCTP_DEST_STATUS_UNCONFIRMED:
      return "UNCONFIRMED";
    default:
      return "UNKNOWN";
    }
}

/* New CMT-PF method:
 *
 * If all destinations are marked PF, send data to one of
 * the destinations so that CMT-PF does not perform worse than
 * CMT.  
 *
 * Algorithm to select the PF destination:
 *   Select the dest with the least error count.
 *   If tie in errorcount values, then all destinations
 *   have "Possibly failed" to the same degree. So, select dest
 *   that was most recently active (marked PF most recently). 
 *
 * Implementation: 
 * If atleast 1 non-PF dest exists, return NULL
 * else, 
 *	- select a dest based on the above algo.
 * 	- set cwnd on the selected PF dest to 1 MTU
 * 	- stop the HB timer if it is running. - hb goes to blackhole !
 *	  hb loss cannot be detected. But, if hb-ack comes back,
 *	  errorcount for the destination is cleared. Situation is
 *	  similar to an ACTIVE dest recving hb. 
 * 	- set eRtxTimerIsRunning to false -- when data is sent, 
 * 	  T3RTX timer be started by RtxMarkedChunks or GenOneDataChunk.
 *	- change dest status to ACTIVE. This dest status is just for
 *	  easier implementation and does not change anything in the
 *	  CMT-PF details. This implementation uses the following rule:
 *		If a dest is PF, only HBs are sent
 *		If a dest is ACTIVE, data is sent
 *	- Schedule HeartbeatGenTimer 
 *
 * Note: This implementation can be considered _aggressive_,
 * since, on the selected PF dest, a HB could've been sent and
 * before HB-ACK arrives, 1MTU of data will be sent.  
 * MAYBE, this aggressiveness can be
 * avoided with a better implementation - do not send HB on
 * a destination with the least error count? 
 */


SctpDest_S*  SctpCMTAgent::SelectFromPFDests()
{
  
  int iLeastErrorCount = (int) uiPathMaxRetrans; 
  double dMostRecentPFSince = 0; 
  Node_S *spCurrDestNode = NULL;
  SctpDest_S* spCurrDestNodeData = NULL;
  SctpDest_S* spSelectedDest = NULL;
  
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    { 
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      
      if (spCurrDestNodeData->eStatus == SCTP_DEST_STATUS_POSSIBLY_FAILED)
	{
	  if (spCurrDestNodeData->iErrorCount < iLeastErrorCount) 
	    {
	      spSelectedDest = spCurrDestNodeData;
	      iLeastErrorCount = spCurrDestNodeData->iErrorCount;
	      dMostRecentPFSince = spCurrDestNodeData->dPFSince;
	    }
	  else if (spCurrDestNodeData->iErrorCount == iLeastErrorCount)
	    {
	      if (spCurrDestNodeData->dPFSince > dMostRecentPFSince)
		{
		  spSelectedDest = spCurrDestNodeData;
		  iLeastErrorCount = spCurrDestNodeData->iErrorCount;
		  dMostRecentPFSince = spCurrDestNodeData->dPFSince;
		}
	      else if (spCurrDestNodeData->dPFSince == dMostRecentPFSince)
		{
		  if (Random::random()&01)
		    spSelectedDest = spCurrDestNodeData;
		}
	    } 
	  
	} 
      else 
	{
	  /* Found a non-PF dest, return NULL 
	   */
	  return NULL;
	  
	}
      
    } 

  /* A PF destination was selected to be marked Active
   */
  if (spSelectedDest->eRtxTimerIsRunning == TRUE)
    StopT3RtxTimer(spSelectedDest);
  
  spSelectedDest->opHeartbeatTimeoutTimer->force_cancel();
  spSelectedDest->eHBTimerIsRunning = FALSE;
  
  spSelectedDest->iCwnd = 1*uiMaxDataSize;
  
  spSelectedDest->eStatus = SCTP_DEST_STATUS_ACTIVE;
  spSelectedDest->dPFSince = 0;
  
  //  DBG_PL(SelectFromPFDests, "Dest: %p changed from PF to Active"),
  // spSelectedDest  DBG_PR;
  
  return spSelectedDest;

}

/* New CMT function. Currently not used. */
//void SctpCMTAgent::SetSharedCCParams(SctpDest_S *spCurrInfo)
void SctpCMTAgent::SetSharedCCParams(SctpDest_S *)
{
// {
//   Node_S *spCurrNode = NULL;
//   SctpDest_S *spCurrDest = NULL;

//   for(spCurrNode = sDestList.spHead;
//       spCurrNode != NULL;
//       spCurrNode = spCurrNode->spNext)
//     {
//       spCurrDest = (SctpDest_S *) spCurrNode->vpData;
//       spCurrDest->iCwnd = spCurrInfo->iSharedCwnd;
//       spCurrDest->iSsthresh = spCurrInfo->iSharedSsthresh;
//     }
// }
}
 
/****** End CMT Change ******/
