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

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp.cc,v 1.18 2011/10/02 22:32:34 tom_henderson Exp $ (UD/PEL)";
#endif

#include "ip.h"
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

int hdr_sctp::offset_;

static class SCTPHeaderClass : public PacketHeaderClass 
{
public:
  SCTPHeaderClass() : PacketHeaderClass("PacketHeader/SCTP",
					sizeof(hdr_sctp)) 
  {
    bind_offset(&hdr_sctp::offset_);
  }
} class_sctphdr;

static class SctpClass : public TclClass 
{ 
public:
  SctpClass() : TclClass("Agent/SCTP") {}
  TclObject* create(int, const char*const*) 
  {
    return (new SctpAgent());
  }
} classSctp;

SctpAgent::SctpAgent() : Agent(PT_SCTP)
{
  fhpDebugFile = NULL;  // init
  opCoreTarget = NULL;  // initialize to NULL in case multihoming isn't used.
  memset(&sInterfaceList, 0, sizeof(List_S) );
  memset(&sDestList, 0, sizeof(List_S) );
  memset(&sAppLayerBuffer, 0, sizeof(List_S) );
  memset(&sSendBuffer, 0, sizeof(List_S) );
  memset(&sRecvTsnBlockList, 0, sizeof(List_S) );
  memset(&sDupTsnList, 0, sizeof(List_S) );
  spOutStreams = NULL;

 /* PN: 5/2007. NR-Sacks */
  memset(&sNonRenegTsnBlockList, 0, sizeof(List_S) );
  memset(&sNonRenegTsnList, 0, sizeof(List_S) );

  opSackGenTimer = new SackGenTimer(this);
  spSctpTrace = NULL;
  opHeartbeatGenTimer = new HeartbeatGenTimer(this);
  opT1InitTimer = new T1InitTimer(this);
  opT1CookieTimer = new T1CookieTimer(this);
  eState = SCTP_STATE_UNINITIALIZED;
}

SctpAgent::~SctpAgent()
{
  Node_S *spCurrNode = NULL;
  Node_S *spPrevNode = NULL;
  SctpDest_S *spDest = NULL;

  delete opSackGenTimer;
  opSackGenTimer = NULL;
  delete opHeartbeatGenTimer;
  opHeartbeatGenTimer = NULL;
  delete opT1InitTimer;
  opT1InitTimer = NULL;
  delete opT1CookieTimer;
  opT1CookieTimer = NULL;

  /* NE: 10/27/2008 Bug fix from Eduardo Ribeiro and Igor Gavriloff. */ 
  if(spOutStreams != NULL)
    delete[] spOutStreams;

  for(spCurrNode = sInterfaceList.spHead;
      spCurrNode != NULL;
      spPrevNode = spCurrNode,spCurrNode=spCurrNode->spNext, delete spPrevNode)
    {
      delete (SctpInterface_S *) spCurrNode->vpData;
      spCurrNode->vpData = NULL;
    }

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spPrevNode = spCurrNode,spCurrNode=spCurrNode->spNext, delete spPrevNode)
    {
      spDest = (SctpDest_S *) spCurrNode->vpData;
      if(spDest->opT3RtxTimer != NULL)
	{
	  delete spDest->opT3RtxTimer;
	  spDest->opT3RtxTimer = NULL;
	}
      if(spDest->opCwndDegradeTimer != NULL)
	{
	  delete spDest->opCwndDegradeTimer;
	  spDest->opCwndDegradeTimer = NULL;
	}
      if(spDest->opHeartbeatTimeoutTimer != NULL)
	{
	  delete spDest->opHeartbeatTimeoutTimer;
	  spDest->opHeartbeatTimeoutTimer = NULL;
	}
      if(spDest->opRouteCacheFlushTimer != NULL)
	{
	  delete spDest->opRouteCacheFlushTimer;
	  spDest->opRouteCacheFlushTimer = NULL;
	}
      if(spDest->opRouteCalcDelayTimer != NULL)
	{
	  delete spDest->opRouteCalcDelayTimer;
	  spDest->opRouteCalcDelayTimer = NULL;
	}
      Packet::free(spDest->opRoutingAssistPacket);
      spDest->opRoutingAssistPacket = NULL;
      delete (SctpDest_S *) spCurrNode->vpData;  //spDest
      spCurrNode->vpData = NULL;
    }

  /* NE: 10/27/2008 Bug fix from Eduardo Ribeiro and Igor Gavriloff.*/
  if(spSctpTrace != NULL)
    {
      delete[] spSctpTrace;
      spSctpTrace = NULL;
    }
}

void SctpAgent::delay_bind_init_all()
{
  delay_bind_init_one("debugMask_");
  delay_bind_init_one("debugFileIndex_");
  delay_bind_init_one("associationMaxRetrans_");
  delay_bind_init_one("pathMaxRetrans_");
  delay_bind_init_one("changePrimaryThresh_");
  delay_bind_init_one("maxInitRetransmits_");
  delay_bind_init_one("heartbeatInterval_");
  delay_bind_init_one("mtu_");
  delay_bind_init_one("initialRwnd_");
  delay_bind_init_one("initialSsthresh_");
  delay_bind_init_one("ipHeaderSize_");
  delay_bind_init_one("dataChunkSize_");
  delay_bind_init_one("numOutStreams_");
  delay_bind_init_one("useDelayedSacks_");
  delay_bind_init_one("sackDelay_");
  delay_bind_init_one("useMaxBurst_");
  delay_bind_init_one("initialCwnd_");
  delay_bind_init_one("initialRto_");
  delay_bind_init_one("minRto_");
  delay_bind_init_one("maxRto_");
  delay_bind_init_one("fastRtxTrigger_");
  delay_bind_init_one("numUnrelStreams_");
  delay_bind_init_one("reliability_");
  delay_bind_init_one("unordered_");
  delay_bind_init_one("rtxToAlt_");
  delay_bind_init_one("dormantAction_");
  delay_bind_init_one("routeCacheLifetime_");
  delay_bind_init_one("routeCalcDelay_");

  /* PN: 5/07. Simulate send window */
  delay_bind_init_one("initialSwnd_");
  /* PN: 5/07. NR-Sacks */
  delay_bind_init_one("useNonRenegSacks_");

  delay_bind_init_one("trace_all_");
  delay_bind_init_one("cwnd_");
  delay_bind_init_one("rwnd_");
  delay_bind_init_one("rto_");
  delay_bind_init_one("errorCount_");
  delay_bind_init_one("frCount_");
  delay_bind_init_one("timeoutCount_");
  delay_bind_init_one("rcdCount_");

  Agent::delay_bind_init_all();
}

int SctpAgent::delay_bind_dispatch(const char *cpVarName, 
				   const char *cpLocalName, 
				   TclObject *opTracer)
{
  if(delay_bind(cpVarName, cpLocalName, "debugMask_", &uiDebugMask, opTracer)) 
    return TCL_OK;
  
  if(delay_bind(cpVarName, cpLocalName, 
		"debugFileIndex_", &iDebugFileIndex, opTracer)) 
    return TCL_OK;
  
  if(delay_bind(cpVarName, cpLocalName, 
		"associationMaxRetrans_", &uiAssociationMaxRetrans, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"pathMaxRetrans_", &uiPathMaxRetrans, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"changePrimaryThresh_", &uiChangePrimaryThresh, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"maxInitRetransmits_", &uiMaxInitRetransmits, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"heartbeatInterval_", &uiHeartbeatInterval, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, "mtu_", &uiMtu, opTracer)) 
    return TCL_OK;
  
  if(delay_bind(cpVarName, cpLocalName, 
		"initialRwnd_", &uiInitialRwnd, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"initialSsthresh_", &iInitialSsthresh, opTracer)) 
    return TCL_OK;
  
  if(delay_bind(cpVarName, cpLocalName, 
		"ipHeaderSize_", &uiIpHeaderSize, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"dataChunkSize_", &uiDataChunkSize, opTracer)) 
    return TCL_OK;
  
  if(delay_bind(cpVarName, cpLocalName, 
		"numOutStreams_", &uiNumOutStreams, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"useDelayedSacks_", (int *) &eUseDelayedSacks, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"sackDelay_",  &dSackDelay, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"useMaxBurst_", (int *) &eUseMaxBurst, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"initialCwnd_", &iInitialCwnd, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"initialRto_", &dInitialRto, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"minRto_", &dMinRto, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"maxRto_", &dMaxRto, opTracer)) 
    return TCL_OK;
    
  if(delay_bind(cpVarName, cpLocalName, 
		"fastRtxTrigger_", &iFastRtxTrigger, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"numUnrelStreams_", &uiNumUnrelStreams, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"reliability_", &uiReliability, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"unordered_", (int *) &eUnordered, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"rtxToAlt_", (int *) &eRtxToAlt, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"dormantAction_", (int *) &eDormantAction, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"routeCacheLifetime_", &dRouteCacheLifetime, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"routeCalcDelay_", &dRouteCalcDelay, opTracer)) 
    return TCL_OK;

  /* PN: 5/07. Simulate send window */
  if(delay_bind(cpVarName, cpLocalName, 
		"initialSwnd_", &uiInitialSwnd, opTracer)) 
    return TCL_OK;

  /* PN: 5/07. NR-Sacks */
  if(delay_bind(cpVarName, cpLocalName, 
		"useNonRenegSacks_", (int *) &eUseNonRenegSacks, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, "cwnd_", &tiCwnd, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, "rwnd_", &tiRwnd, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, "rto_", &tdRto, opTracer)) 
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, "errorCount_", &tiErrorCount,opTracer))
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, "frCount_", &tiFrCount, opTracer))
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"timeoutCount_", &tiTimeoutCount, opTracer))
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, "rcdCount_", &tiRcdCount, opTracer))
    return TCL_OK;

  if(delay_bind(cpVarName, cpLocalName, 
		"trace_all_", (int *) &eTraceAll, opTracer)) 
    return TCL_OK;

  return Agent::delay_bind_dispatch(cpVarName, cpLocalName, opTracer);
}

void SctpAgent::TraceAll()
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
	      "cwnd: %d pba: %d out: %d ssthresh: %d rwnd: %d peerRwnd: %d "
	      "rto: %-6.3f srtt: %-6.3f rttvar: %-6.3f "
	      "assocErrors: %d pathErrors: %d dstatus: %s isPrimary: %s "
	      "frCount: %d timeoutCount: %d rcdCount: %d\n",
	      dCurrTime,
	      addr(), port(), spCurrDest->iNsAddr, spCurrDest->iNsPort,
	      spCurrDest->iCwnd, spCurrDest->iPartialBytesAcked, 
	      spCurrDest->iOutstandingBytes, spCurrDest->iSsthresh, 
	      uiMyRwnd, uiPeerRwnd,
	      spCurrDest->dRto, spCurrDest->dSrtt, 
	      spCurrDest->dRttVar,
	      iAssocErrorCount,
	      spCurrDest->iErrorCount,
	      spCurrDest->eStatus ? "ACTIVE" : "INACTIVE",
	      (spCurrDest == spPrimaryDest) ? "TRUE" : "FALSE",
	      int(tiFrCount),
	      spCurrDest->iTimeoutCount,
	      spCurrDest->iRcdCount);
      if(channel_)
	(void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
    }

  sprintf(cpOutString, "\n");
  if(channel_)
    (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));
}

void SctpAgent::TraceVar(const char* cpVar)
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

  else if(!strcmp(cpVar, "rwnd_"))
    {
      sprintf(cpOutString, "time: %-8.5f rwnd: %d peerRwnd: %d\n", 
	      dCurrTime, uiMyRwnd, uiPeerRwnd);
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


void SctpAgent::trace(TracedVar* v) 
{
  if(eTraceAll == TRUE)
    TraceAll();
  else 
    TraceVar(v->name());
}

/* Reset() is called to refresh the current association settings. It is used
 * for the first association. It can also be used to abruptly terminate the
 * existing association (so that a new one can begin), but this usage has NOT
 * been tested extensively.
 */
void SctpAgent::Reset()
{
  /* Just in case the user uses the standard ns-2 way of turning on
   * debugging, we turn on all debugging. However, if the uiDebugMask is
   * used, then the user knows about SCTP's fine-level debugging control
   * and the debug_ flag is ignored.
   */
  if(debug_ == TRUE && uiDebugMask == 0)
    uiDebugMask = 0xffffffff;

  /* No debugging output will appear until this is called. Why do we do it
   * here?  This is the earliest point at which we have the necessary
   * debugging variables bound from TCL.  
   */
  DBG_FOPEN(); // internal check is done to ensure we only open once!
  DBG_I(Reset);

  if(eState != SCTP_STATE_UNINITIALIZED && eState != SCTP_STATE_CLOSED)
    Close();  // abruptly close the connection

  DBG_PL(Reset, "uiDebugMask=%u"), uiDebugMask DBG_PR;
  DBG_PL(Reset, "iDebugFileIndex=%d"), iDebugFileIndex DBG_PR;
  DBG_PL(Reset, "uiAssociationMaxRetrans=%ld"), uiAssociationMaxRetrans DBG_PR;
  DBG_PL(Reset, "uiPathMaxRetrans=%ld"), uiPathMaxRetrans DBG_PR;
  DBG_PL(Reset, "uiChangePrimaryThresh=%d"), uiChangePrimaryThresh DBG_PR;
  DBG_PL(Reset, "uiHeartbeatInterval=%ld"), uiHeartbeatInterval DBG_PR;
  DBG_PL(Reset, "uiMtu=%ld"), uiMtu DBG_PR;
  DBG_PL(Reset, "uiInitialRwnd=%ld"), uiInitialRwnd DBG_PR;
  DBG_PL(Reset, "iInitialSsthresh=%ld"), iInitialSsthresh DBG_PR;
  DBG_PL(Reset, "uiIpHeaderSize=%ld"), uiIpHeaderSize DBG_PR;
  DBG_PL(Reset, "uiDataChunkSize=%ld"), uiDataChunkSize DBG_PR;
  DBG_PL(Reset, "uiNumOutStreams=%ld"), uiNumOutStreams DBG_PR;
  DBG_PL(Reset, "eUseDelayedSacks=%s"), 
    eUseDelayedSacks ? "TRUE" : "FALSE" DBG_PR;
  DBG_PL(Reset, "dSackDelay=%f"), dSackDelay DBG_PR;
  DBG_PL(Reset, "eUseMaxBurst=%s"), eUseMaxBurst ? "TRUE" : "FALSE" DBG_PR;
  DBG_PL(Reset, "iInitialCwnd=%ld"), iInitialCwnd DBG_PR;
  DBG_PL(Reset, "dInitialRto=%f"), dInitialRto DBG_PR;
  DBG_PL(Reset, "dMinRto=%f"), dMinRto DBG_PR;
  DBG_PL(Reset, "dMaxRto=%f"), dMaxRto DBG_PR;
  DBG_PL(Reset, "iFastRtxTrigger=%ld"), iFastRtxTrigger DBG_PR;
  DBG_PL(Reset, "uiNumUnrelStreams=%ld"), uiNumUnrelStreams DBG_PR;
  DBG_PL(Reset, "uiReliability=%ld"), uiReliability DBG_PR;
  DBG_PL(Reset, "eUnordered=%s"), eUnordered ? "TRUE" : "FALSE" DBG_PR;

  /* PN: 5/2007. NR-Sacks */
  DBG_PL(Reset, "eUseNonRenegSacks=%s"), 
	eUseNonRenegSacks ? "TRUE" : "FALSE" DBG_PR;
  /* PN: 5/2007. Simulate send window */
  DBG_PL(Reset, "uiInitialSwnd=%ld"), uiInitialSwnd DBG_PR;
  uiAvailSwnd = uiInitialSwnd;

  switch(eRtxToAlt)
    {
    case RTX_TO_ALT_OFF:
      DBG_PL(Reset, "eRtxToAlt=RTX_TO_ALT_OFF") DBG_PR;
      break;

    case RTX_TO_ALT_ON:
      DBG_PL(Reset, "eRtxToAlt=RTX_TO_ALT_ON") DBG_PR;
      break;

    case RTX_TO_ALT_TIMEOUTS_ONLY:
      DBG_PL(Reset, "eRtxToAlt=RTX_TO_ALT_TIMEOUTS_ONLY") DBG_PR;
      break;
    }

  switch(eDormantAction)
    {
    case DORMANT_HOP:
      DBG_PL(Reset, "eDormantAction=DORMANT_HOP") DBG_PR;
      break;

    case DORMANT_PRIMARY:
      DBG_PL(Reset, "eDormantAction=DORMANT_PRIMARY") DBG_PR;
      break;

    case DORMANT_LASTDEST:
      DBG_PL(Reset, "eDormantAction=DORMANT_LASTDEST") DBG_PR;
      break;
    }

  DBG_PL(Reset, "eTraceAll=%s"), eTraceAll ? "TRUE" : "FALSE" DBG_PR;

  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;
  int i;

  if(uiInitialRwnd > MAX_RWND_SIZE)
    {
      fprintf(stderr, "SCTP ERROR: initial rwnd (%d) > max (%d)\n",
	      uiInitialRwnd, MAX_RWND_SIZE);
      DBG_PL(Reset, "ERROR: initial rwnd (%d) > max (%d)"), 
	uiInitialRwnd, MAX_RWND_SIZE DBG_PR;
      DBG_PL(Reset, "exiting...") DBG_PR;
      exit(-1);
    }

  if(uiNumOutStreams > MAX_NUM_STREAMS)
    {
      fprintf(stderr, "%s number of streams (%d) > max (%d)\n",
	      "SCTP ERROR:",
	      uiNumOutStreams, MAX_NUM_STREAMS);
      DBG_PL(Reset, "ERROR: number of streams (%d) > max (%d)"), 
	uiNumOutStreams, MAX_NUM_STREAMS DBG_PR;
      DBG_PL(Reset, "exiting...") DBG_PR;
      exit(-1);
    }
  else if(uiNumUnrelStreams > uiNumOutStreams)
    {
      fprintf(stderr, "%s number of unreliable streams (%d) > total (%d)\n",
	      "SCTP ERROR:",
	      uiNumUnrelStreams, uiNumOutStreams);
      DBG_PL(Reset, "ERROR: number of unreliable streams (%d) > total (%d)"), 
	uiNumUnrelStreams, uiNumOutStreams DBG_PR;
      DBG_PL(Reset, "exiting...") DBG_PR;
      exit(-1);
    }

  uiMaxPayloadSize = uiMtu - SCTP_HDR_SIZE - uiIpHeaderSize;
  uiMaxDataSize = uiMaxPayloadSize - ControlChunkReservation();

  if(uiDataChunkSize > MAX_DATA_CHUNK_SIZE)
    {
      fprintf(stderr, "%s data chunk size (%d) > max (%d)\n",
	      "SCTP ERROR:",
	      uiDataChunkSize, MAX_DATA_CHUNK_SIZE);
      DBG_PL(Reset, "ERROR: data chunk size (%d) > max (%d)"), 
	uiDataChunkSize, MAX_DATA_CHUNK_SIZE DBG_PR;
      DBG_PL(Reset, "exiting...") DBG_PR;
      exit(-1);
    }
  else if(uiDataChunkSize > uiMaxDataSize)
    {
      fprintf(stderr, "SCTP ERROR: DATA chunk size (%d) too big!\n",
	      uiDataChunkSize);
      fprintf(stderr, "            SCTP/IP header = %d\n",
	      SCTP_HDR_SIZE + uiIpHeaderSize);
      fprintf(stderr, "            Control chunk reservation = %d\n",
	      ControlChunkReservation());
      fprintf(stderr, "            MTU = %d\n", uiMtu);
      fprintf(stderr, "\n");

      DBG_PL(Reset, 
	     "ERROR: data chunk size (%d) + SCTP/IP header(%d) + Reserved (%d) > MTU (%d)"), 
	uiDataChunkSize, SCTP_HDR_SIZE + uiIpHeaderSize, 
	ControlChunkReservation(), uiMtu DBG_PR;
      DBG_PL(Reset, "exiting...") DBG_PR;
      exit(-1);
    }
  else if(uiDataChunkSize < MIN_DATA_CHUNK_SIZE)
    {
      fprintf(stderr, "%s data chunk size (%d) < min (%d)\n",
	      "SCTP ERROR:",
	      uiDataChunkSize, MIN_DATA_CHUNK_SIZE);
      DBG_PL(Reset, "ERROR: data chunk size (%d) < min (%d)"), 
	uiDataChunkSize, MIN_DATA_CHUNK_SIZE DBG_PR;
      DBG_PL(Reset, "exiting...") DBG_PR;
      exit(-1);
    }

  /* size_ is an Agent variable which is normally the packet size, but
   * SCTP uses size_ to dictate to non-sctp aware applications the max
   * data size of an application write. If size_ isn't set by SCTP, some
   * non-sctp aware apps (such as Telnet) will call sendmsg() with 0
   * bytes.
   */
  size_ = uiMtu - SCTP_HDR_SIZE - uiIpHeaderSize - sizeof(SctpDataChunkHdr_S);

  eState = SCTP_STATE_CLOSED;
  eForceSource = FALSE;
  iAssocErrorCount = 0;

  if(uiHeartbeatInterval != 0)
    {
      opHeartbeatGenTimer->force_cancel();
    }

  opT1InitTimer->force_cancel();
  opT1CookieTimer->force_cancel();
  iInitTryCount = 0;
  uiNextTsn = 0;
  usNextStreamId = 0;

  /* if it's already allocated, let's delete and reallocate just in case user
   * has changed TCL bindable variables
   */
  /* NE: 10/27/2008 Bug fix from Eduardo Ribeiro and Igor Gavriloff. */
  if(spOutStreams != NULL)
    delete[] spOutStreams;
  spOutStreams = new SctpOutStream_S [uiNumOutStreams];
  memset(spOutStreams, 0, (uiNumOutStreams * sizeof(SctpOutStream_S)) );

  for(i = 0; i < (int) uiNumUnrelStreams; i++)
    {
      DBG_PL(Reset, "setting outStream %d to UNRELIABLE"), i DBG_PR;
      spOutStreams[i].eMode = SCTP_STREAM_UNRELIABLE;
    }
  for(; i < (int) uiNumOutStreams; i++)
    {
      DBG_PL(Reset, "setting outStream %d to RELIABLE"), i DBG_PR;
      spOutStreams[i].eMode = SCTP_STREAM_RELIABLE;
    }

  uiPeerRwnd = 0;
  uiCumAckPoint = 0;
  uiAdvancedPeerAckPoint = 0;
  uiHighestTsnNewlyAcked = 0;
  uiRecover = 0;
  memset(&sAppLayerBuffer, 0, sizeof(List_S) );
  memset(&sSendBuffer, 0, sizeof(List_S) );

  /* NE: initialize the value for last TSN sent */
  uiHighestTsnSent = 0;

  if(uiAssociationMaxRetrans > (sDestList.uiLength * uiPathMaxRetrans))
    {
      DBG_PL(Reset, 
	     "WARNING: Association.Max.Retrans > "
	     "summation of all destinations' Path.Max.Retrans "
	     "(rfc2960 section 8.1)") DBG_PR;
    }

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrDest = (SctpDest_S *) spCurrNode->vpData;
      
      /* Lukasz Budzisz : 03/09/2006
	 Section 7.2.1 of RFC4960 */
      spCurrDest->iCwnd = 
	MIN(4 * (int) uiMaxDataSize, MAX(4380, 2 * (int) uiMaxDataSize));
      spCurrDest->iSsthresh = iInitialSsthresh;
      spCurrDest->eFirstRttMeasurement = TRUE;
      spCurrDest->dRto = dInitialRto;

      if(spCurrDest->opT3RtxTimer == NULL)
	spCurrDest->opT3RtxTimer = new T3RtxTimer(this, spCurrDest);
      else 
	spCurrDest->opT3RtxTimer->force_cancel();

      spCurrDest->iOutstandingBytes = 0;
      spCurrDest->iPartialBytesAcked = 0;

      spCurrDest->iErrorCount = 0;
      spCurrDest->iTimeoutCount = 0;
      spCurrDest->eStatus = SCTP_DEST_STATUS_ACTIVE;

      if(spCurrDest->opCwndDegradeTimer == NULL)
	{
	  spCurrDest->opCwndDegradeTimer =
	    new CwndDegradeTimer(this, spCurrDest);
	}
      else
	{
	  spCurrDest->opCwndDegradeTimer->force_cancel();
	}
      
      spCurrDest->dIdleSince = 0;
            
      if(spCurrDest->opHeartbeatTimeoutTimer == NULL)
	{
	  spCurrDest->opHeartbeatTimeoutTimer =
	    new HeartbeatTimeoutTimer(this, spCurrDest);
	}
      else
	{
	  spCurrDest->opHeartbeatTimeoutTimer->force_cancel();
	}
      
      spCurrDest->eCcApplied = FALSE;
      spCurrDest->spFirstOutstanding = NULL;
      
      /* per destination vars for CMT
       */
      spCurrDest->uiBurstLength = 0;
      spCurrDest->eMarkedChunksPending = FALSE;
      
      spCurrDest->iRcdCount = 0;      
      spCurrDest->eRouteCached = FALSE;
      
      if(spCurrDest->opRouteCacheFlushTimer == NULL)
	{
	  spCurrDest->opRouteCacheFlushTimer =
	    new RouteCacheFlushTimer(this, spCurrDest);
	}
      else
	{
	  spCurrDest->opRouteCacheFlushTimer->force_cancel();
	}

      if(spCurrDest->opRouteCalcDelayTimer == NULL)
	{
	  spCurrDest->opRouteCalcDelayTimer =
	    new RouteCalcDelayTimer(this, spCurrDest);
	}
      else
	{
	  spCurrDest->opRouteCalcDelayTimer->force_cancel();
	}

      memset(&spCurrDest->sBufferedPackets, 0, sizeof(List_S) );
    }

  eForwardTsnNeeded = FALSE;
  eSendNewDataChunks = FALSE;
  eMarkedChunksPending = FALSE;
  eApplyMaxBurst = FALSE; // Why is MaxBurst init'd to FALSE?? (JRI)
  eDataSource = DATA_SOURCE_APPLICATION;
  uiBurstLength = 0;

  uiMyRwnd = uiInitialRwnd;
  uiCumAck = 0; 
  uiHighestRecvTsn = 0;
  memset(&sRecvTsnBlockList, 0, sizeof(List_S) );
  memset(&sDupTsnList, 0, sizeof(List_S) );
  eStartOfPacket = FALSE;
  iDataPktCountSinceLastSack = 0;
  eSackChunkNeeded = FALSE;

  /* PN: 5/2007. NR-Sacks */
  ClearList(&sNonRenegTsnBlockList);
  ClearList(&sNonRenegTsnList);
  memset(&sNonRenegTsnBlockList, 0, sizeof(List_S) );
  memset(&sNonRenegTsnList, 0, sizeof(List_S) );

  opSackGenTimer->force_cancel();

  /* if it's already allocated, let's delete and reallocate just in case user
   * has changed TCL bindable variables
   */
  /* NE: 10/27/2008 Bug fix from Eduardo Ribeiro and Igor Gavriloff.*/
  if(spSctpTrace != NULL)
    delete[] spSctpTrace;
  
  /* We don't know how many chunks will be in a packet, so we assume the 
   * theoretical maximum... a packet full of only chunk headers.
   */
  spSctpTrace = new SctpTrace_S[uiMaxPayloadSize / sizeof(SctpChunkHdr_S)];

  uiNumChunks = 0;

  /* Trigger changes for trace to pick up (we want to know the initial values)
   */
  tiCwnd++;
  tiRwnd++;
  tdRto++;
  tiErrorCount++;
  tiFrCount = 0;
  tiTimeoutCount++;
  tiRcdCount++;

  OptionReset();

  DBG_PL(Reset, "spSctpTrace=%p"), spSctpTrace DBG_PR;
  DBG_X(Reset);
}

/* This function serves as a hook for optional extensions to reset additional
 * state variables.
 */
void SctpAgent::OptionReset()
{
  return;
}

/* This function returns a size based on how much space is needed for
 * bundled chunks. Real implementations, such as the KAME stack, reserve
 * space for ECN, etc. We are using it for experimental extensions such as
 * the Timestamp chunk (sctp-timestamp.cc). Such a function avoids having
 * to maintain nearly identical copies of the Reset() function for each
 * extension. For the base sctp, we don't reserve any space... so return 0.
 */
u_int SctpAgent::ControlChunkReservation()
{
  DBG_I(ControlChunkReservation);
  DBG_PL(ControlChunkReservation, "returning 0") DBG_PR;
  DBG_X(ControlChunkReservation);
  return 0;
}

int SctpAgent::command(int argc, const char*const* argv)
{
  DBG_I(command);// internal check is done to avoid printing if file is unopen!

  double dCurrTime = Scheduler::instance().clock();
  DBG_PL(command, "<time:%f> argc=%d argv[1]=%s"),
    dCurrTime, argc, argv[1] DBG_PR;

  Tcl& oTcl = Tcl::instance();
  Node *opNode = NULL;
  int iNsAddr;
  int iNsPort;
  NsObject *opTarget = NULL;
  NsObject *opLink = NULL;
  int iRetVal;

  if(argc == 2)   
    {
      if (strcmp(argv[1], "reset") == 0) 
	{
	  Reset();
	  DBG_X(command);
	  return (TCL_OK);
	}
      else if (strcmp(argv[1], "close") == 0) 
	{
	  Close();
	  DBG_X(command);
	  return (TCL_OK);
	}
    }
  else if(argc == 3) 
    {
      if (strcmp(argv[1], "advance") == 0) 
	{
	  DBG_X(command);
	  return (TCL_OK);
	}
      else if (strcmp(argv[1], "set-multihome-core") == 0) 
	{ 
	  opCoreTarget = (Classifier *) TclObject::lookup(argv[2]);
	  if(opCoreTarget == NULL) 
	    {
	      oTcl.resultf("no such object %s", argv[4]);
	      return (TCL_ERROR);
	    }
	  DBG_X(command);
	  return (TCL_OK);
	}
      else if (strcmp(argv[1], "set-primary-destination") == 0)
	{
	  opNode = (Node *) TclObject::lookup(argv[2]);
	  if(opNode == NULL) 
	    {
	      oTcl.resultf("no such object %s", argv[2]);
	      return (TCL_ERROR);
	    }
	  iRetVal = SetPrimary( opNode->address() );

	  if(iRetVal == TCL_ERROR)
	    {
	      fprintf(stderr, "[SctpAgent::command] ERROR:"
		      "%s is not a valid destination\n", argv[2]);
	      DBG_X(command);
	      return (TCL_ERROR);
	    }
	  DBG_X(command);
	  return (TCL_OK);
	}
      else if (strcmp(argv[1], "force-source") == 0)
	{
	  opNode = (Node *) TclObject::lookup(argv[2]);
	  if(opNode == NULL) 
	    {
	      oTcl.resultf("no such object %s", argv[2]);
	      return (TCL_ERROR);
	    }
	  iRetVal = ForceSource( opNode->address() );

	  if(iRetVal == TCL_ERROR)
	    {
	      fprintf(stderr, "[SctpAgent::command] ERROR:"
		      "%s is not a valid source\n", argv[2]);
	      DBG_X(command);
	      return (TCL_ERROR);
	    }
	  DBG_X(command);
	  return (TCL_OK);
	}
      else if (strcmp(argv[1], "print") == 0) 
	{
	  if(eTraceAll == TRUE)
	    TraceAll();
	  else 
	    TraceVar(argv[2]);
	  DBG_X(command);
	  return (TCL_OK);
	}
    }
  else if(argc == 4)
    {
      if (strcmp(argv[1], "add-multihome-destination") == 0) 
	{ 
	  iNsAddr = atoi(argv[2]);
	  iNsPort = atoi(argv[3]);
	  AddDestination(iNsAddr, iNsPort);
	  DBG_X(command);
	  return (TCL_OK);
	}
      else if (strcmp(argv[1], "set-destination-lossrate") == 0)
	{
	  opNode = (Node *) TclObject::lookup(argv[2]);
	  if(opNode == NULL) 
	    {
	      oTcl.resultf("no such object %s", argv[2]);
	      return (TCL_ERROR);
	    }
	  iRetVal = SetLossrate( opNode->address(), atof(argv[3]) );

	  if(iRetVal == TCL_ERROR)
	    {
	      fprintf(stderr, "[SctpAgent::command] ERROR:"
		      "%s is not a valid destination\n", argv[2]);
	      DBG_X(command);
	      return (TCL_ERROR);
	    }
	  DBG_X(command);
	  return (TCL_OK);
	}
    }
  else if(argc == 6)
    {
      if (strcmp(argv[1], "add-multihome-interface") == 0) 
	{ 
	  iNsAddr = atoi(argv[2]);
	  iNsPort = atoi(argv[3]);
	  opTarget = (NsObject *) TclObject::lookup(argv[4]);
	  if(opTarget == NULL) 
	    {
	      oTcl.resultf("no such object %s", argv[4]);
	      return (TCL_ERROR);
	    }
	  opLink = (NsObject *) TclObject::lookup(argv[5]);
	  if(opLink == NULL) 
	    {
	      oTcl.resultf("no such object %s", argv[5]);
	      return (TCL_ERROR);
	    }
	  AddInterface(iNsAddr, iNsPort, opTarget, opLink);
	  DBG_X(command);
	  return (TCL_OK);
	}
    }

  DBG_X(command);
  return (Agent::command(argc, argv));
}

/* Given params: sList, spPrevNode, spNextNode, spNewNode... insert spNewNode
 * into sList between spPrevNode and spNextNode.
 */
void SctpAgent::InsertNode(List_S *spList, Node_S *spPrevNode, 
			  Node_S *spNewNode,  Node_S *spNextNode)
{
  if(spPrevNode == NULL)
    spList->spHead = spNewNode;
  else
    spPrevNode->spNext = spNewNode;

  spNewNode->spPrev = spPrevNode;
  spNewNode->spNext = spNextNode;

  if(spNextNode == NULL)
    spList->spTail = spNewNode;
  else
    spNextNode->spPrev = spNewNode;

  spList->uiLength++;
}

void SctpAgent::DeleteNode(List_S *spList, Node_S *spNode)
{
  if(spNode->spPrev == NULL)
    spList->spHead = spNode->spNext;
  else
    spNode->spPrev->spNext = spNode->spNext;
  
  if(spNode->spNext == NULL)
    spList->spTail = spNode->spPrev;
  else
    spNode->spNext->spPrev = spNode->spPrev;

  /* now let's free the internal structures
   */
  switch(spNode->eType)
    {
    case NODE_TYPE_STREAM_BUFFER:
      delete[] (u_char *) ((SctpStreamBufferNode_S *) spNode->vpData)->spChunk;
      ((SctpStreamBufferNode_S *) spNode->vpData)->spChunk = NULL;
      delete (SctpStreamBufferNode_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;

    case NODE_TYPE_RECV_TSN_BLOCK:
      delete (SctpRecvTsnBlock_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;

    case NODE_TYPE_DUP_TSN:
      delete (SctpDupTsn_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;

    case NODE_TYPE_SEND_BUFFER:
      delete[] (u_char *) ((SctpSendBufferNode_S *) spNode->vpData)->spChunk;
      ((SctpSendBufferNode_S *) spNode->vpData)->spChunk = NULL;
      delete (SctpSendBufferNode_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;

    case NODE_TYPE_APP_LAYER_BUFFER:
      delete (AppData_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;

    case NODE_TYPE_INTERFACE_LIST:
      delete (SctpInterface_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;

    case NODE_TYPE_DESTINATION_LIST:
      delete (SctpDest_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;

    case NODE_TYPE_PACKET_BUFFER:
      /* no need to free data, because SctpAgent hands over responsibility to
       * the send() function
       */
      spNode->vpData = NULL;
      break;

    /* PN: NR-Sacks */
    case NODE_TYPE_TSN:
      delete (SctpTsn_S *) spNode->vpData;
      spNode->vpData = NULL;
      break;
    
    }

  delete spNode;
  spList->uiLength--;
}

/* PN: 5/2007. NR-Sacks */
/* Given params: spList, spNewNode... insert spNewNode in ascending order
 * into spList. spNewNode will not be inserted if it contains duplicate data.
 */
void SctpAgent::InsertNodeInSortedList(List_S *spList, Node_S *spNewNode)
{

  Node_S *spCurrNode = spList->spHead;
  Node_S *spPrevNode = NULL; // NE : Should be NULL?

  while (spCurrNode != NULL)
  {
    switch(spNewNode->eType)
    {
      case NODE_TYPE_TSN:

	/* Do not add Tsn if it already exists in the list */
    	if ( ((SctpTsn_S *) spCurrNode->vpData)->uiTsn == 
	     ((SctpTsn_S *)spNewNode->vpData)->uiTsn )
	  return;

    	if ( ((SctpTsn_S *)spCurrNode->vpData)->uiTsn >
	     ((SctpTsn_S *)spNewNode->vpData)->uiTsn )
        {
    	  /* Insert new node between spPrevNode and spCurrNode */
          InsertNode(spList, spPrevNode, spNewNode, spCurrNode);
	    return;
	}

	/* break from switch case */
	break;  

      default:
	/* new node inserted at tail */
	break; 

      } /* End switch */

      spPrevNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;

  } /* End of while */

  /* Insert at tail */
  InsertNode(spList, spList->spTail, spNewNode, NULL);

}

void SctpAgent::ClearList(List_S *spList)
{
  Node_S *spCurrNode = spList->spHead;
  Node_S *spDeleteNode = NULL;

  while(spCurrNode != NULL)
    {
      spDeleteNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;
      DeleteNode(spList, spDeleteNode);
    }
}

void SctpAgent::AddInterface(int iNsAddr, int iNsPort, 
			     NsObject *opTarget, NsObject *opLink)
{
  DBG_I(AddInterface);

  Node_S *spNewNode = new Node_S;
  spNewNode->eType = NODE_TYPE_INTERFACE_LIST;
  spNewNode->vpData = new SctpInterface_S;

  SctpInterface_S *spNewInterface = (SctpInterface_S *) spNewNode->vpData;
  spNewInterface->iNsAddr = iNsAddr;
  spNewInterface->iNsPort = iNsPort;
  spNewInterface->opTarget = opTarget;
  spNewInterface->opLink = opLink;
  
  InsertNode(&sInterfaceList, sInterfaceList.spTail, spNewNode, NULL);

  DBG_X(AddInterface);
}

void SctpAgent::AddDestination(int iNsAddr, int iNsPort)
{
  DBG_I(AddDestination);

  Node_S *spNewNode = new Node_S;
  spNewNode->eType = NODE_TYPE_DESTINATION_LIST;
  spNewNode->vpData = new SctpDest_S;

  SctpDest_S *spNewDest = (SctpDest_S *) spNewNode->vpData;
  memset(spNewDest, 0, sizeof(SctpDest_S));
  spNewDest->iNsAddr = iNsAddr;
  spNewDest->iNsPort = iNsPort;

  /* set the primary to the last destination added just in case the user does
   * not set a primary
   */
  spPrimaryDest = spNewDest;
  spNewTxDest = spPrimaryDest;

  /* allocate packet with the dest addr. to be used later for setting src dest
   */
  daddr() = spNewDest->iNsAddr;
  dport() = spNewDest->iNsPort;
  spNewDest->opRoutingAssistPacket = allocpkt();

  InsertNode(&sDestList, sDestList.spTail, spNewNode, NULL);

  DBG_X(AddDestination);
}

int SctpAgent::SetPrimary(int iNsAddr)
{
  DBG_I(SetPrimary);

  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrDest = (SctpDest_S *) spCurrNode->vpData;
      
      if(spCurrDest->iNsAddr == iNsAddr)
	{
	  spPrimaryDest = spCurrDest;
	  spNewTxDest = spPrimaryDest;
	  DBG_PL(SetPrimary, "returning TCL_OK") DBG_PR;
	  DBG_X(SetPrimary);
	  return (TCL_OK);
	}
    }

  DBG_PL(SetPrimary, "returning TCL_ERROR") DBG_PR;
  DBG_X(SetPrimary);
  return (TCL_ERROR);
}

/* Helps maintain information passed down from the TCL script by an oracle
 * about path lossrates.  Used in RTX-LOSSRATE rtx policy for
 * CMT. Lossrate for a path is set in the dest node's datastructure.
 */
int SctpAgent::SetLossrate(int iNsAddr, float fLossrate)
{

  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrDest = (SctpDest_S *) spCurrNode->vpData;
      if(spCurrDest->iNsAddr == iNsAddr)
	{
	  spCurrDest->fLossrate = fLossrate;
	  return (TCL_OK);
	}
    }
  return (TCL_ERROR);
}

int SctpAgent::ForceSource(int iNsAddr)
{
  DBG_I(ForceSource);

  Node_S *spCurrNode = sInterfaceList.spHead;
  SctpInterface_S *spCurrInterface = NULL;

  while(spCurrNode != NULL)
    {
      spCurrInterface = (SctpInterface_S *) spCurrNode->vpData;

      if(spCurrInterface->iNsAddr == iNsAddr)
	{
	  addr() = spCurrInterface->iNsAddr;
	  port() = spCurrInterface->iNsPort;
	  target_ = spCurrInterface->opTarget;
	  eForceSource = TRUE;
	  DBG_PL(ForceSource, "returning TCL_OK") DBG_PR;
	  DBG_X(ForceSource);
	  return (TCL_OK);
	}
      else
	spCurrNode = spCurrNode->spNext;
    }

  DBG_PL(ForceSource, "returning TCL_ERROR") DBG_PR;
  DBG_X(ForceSource);
  return (TCL_ERROR);
}

/* returns the size of the chunk
 */
int SctpAgent::GenChunk(SctpChunkType_E eType, u_char *ucpChunk)
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

      DBG_PL(GenChunk, "SACK CumAck=%d arwnd=%d"), uiCumAck, uiMyRwnd DBG_PR;

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
	    = ((SctpRecvTsnBlock_S *)spCurrFrag->vpData)->uiStartTsn -uiCumAck;

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

    /* PN: 6/17/2007. NR-Sacks */
    case SCTP_CHUNK_NRSACK:
      iSize = sizeof(SctpNonRenegSackChunk_S);
      ((SctpNonRenegSackChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->uiCumAck = uiCumAck;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->uiArwnd = uiMyRwnd;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumGapAckBlocks = 0;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumNonRenegSackBlocks
	  = sNonRenegTsnBlockList.uiLength;
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumDupTsns = 
	sDupTsnList.uiLength;

      DBG_PL(GenChunk, "NR-SACK: CumAck=%d arwnd=%d dCurrTime=%f"), 
	uiCumAck, uiMyRwnd, dCurrTime  DBG_PR;

      /* Append all the Gap Ack Blocks
       */
      /* NE: let's generate disjoint Gap Ack Blocks and Non-Renegable Gap 
	 Ack Blocks, and add disjoint Gap Ack Blocks to the NR-SACK chunk */
      spCurrGapFrag = sRecvTsnBlockList.spHead; 
      spCurrNonRenegGapFrag = sNonRenegTsnBlockList.spHead;
      
      while (spCurrGapFrag != NULL && 
	     (iSize + sizeof(SctpGapAckBlock_S) < uiMaxDataSize)) 
	{  	  
	  /* NE: current Gap Ack Block */
	  gapAckBlockStartTsn = 
	    ((SctpRecvTsnBlock_S *)spCurrGapFrag->vpData)->uiStartTsn;
	  gapAckBlockEndTsn = 
	    ((SctpRecvTsnBlock_S *)spCurrGapFrag->vpData)->uiEndTsn;
	
	  DBG_PL(GenChunk, "Current GapAckBlock StartTSN=%d EndTSN=%d"), 
	    gapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
 
	  /* NE: check if still there are some Non-Renegable Gap Ack Blocks */
	  if (spCurrNonRenegGapFrag != NULL) {
	
	    /* NE: current Non-Renegable Gap Ack Block */
            nonRenegGapAckBlockStartTsn = 
	      ((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiStartTsn;
	    nonRenegGapAckBlockEndTsn = 
	      ((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiEndTsn;

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

	      SctpGapAckBlock_S *spGapAckBlock = 
		(SctpGapAckBlock_S *) (ucpChunk+iSize);
	      
	      spGapAckBlock->usStartOffset = gapAckBlockStartTsn - uiCumAck;
	      spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
	      iSize += sizeof(SctpGapAckBlock_S);
	      
	    } /* NE: current Gap Ack Block contains some Non-Renegable TSNs */
	    else if ((gapAckBlockStartTsn <= nonRenegGapAckBlockStartTsn) && 
		     (nonRenegGapAckBlockStartTsn <= gapAckBlockEndTsn)) {
	      
	      /* NE: current Gap Ack Blocks all TSNs are Non-Renegable, 
		 so let's skip(do not add to NR-SACK chunk) the current Gap Ack Block */
     	      if ((gapAckBlockStartTsn == nonRenegGapAckBlockStartTsn) && 
		  (gapAckBlockEndTsn == nonRenegGapAckBlockEndTsn)) {
		DBG_PL(GenChunk, "GapAckBlock Skipping StartTSN=%d EndTSN=%d"), 
		  gapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
		
		spCurrGapFrag = spCurrGapFrag->spNext;
		spCurrNonRenegGapFrag = spCurrNonRenegGapFrag->spNext;
		continue;
	      }
	      else {   /* NE: let's split the current Gap Ack Block into some 
			  number of Gap Ack Blocks and Non-Renegable Gap Ack 
			  Blocks which are disjoint */

		dividedGapAckBlockStartTsn = gapAckBlockStartTsn;
		
		/* NE: current Gap Ack Block has some Non-Renegable TSNs */
		while (dividedGapAckBlockStartTsn <= gapAckBlockEndTsn && 
		       (iSize + sizeof(SctpGapAckBlock_S) < uiMaxDataSize)) {
		  
		  /* NE: First let's check if the current Non-Renegable Gap Ack
		     Block is part of the current Gap Ack Block. If not add the 
		     remaining part of the Gap Ack Block to NR-SACK chunk */
		  if (spCurrNonRenegGapFrag == NULL || 
		      nonRenegGapAckBlockStartTsn > gapAckBlockEndTsn) {
		    DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
		      dividedGapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;
		    
		    usNumGapAckBlocks++;
		    
		    /* NE: add the Gap Ack Block to the NR-SACK chunk */
		    DBG_PL(GenChunk, 
			   "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
		      iSize, usNumGapAckBlocks DBG_PR;
		    DBG_PL(GenChunk, 
			   "RenegGapBlock StartOffset=%d EndOffset=%d"), 
		      (dividedGapAckBlockStartTsn - uiCumAck), 
		      (gapAckBlockEndTsn - uiCumAck) DBG_PR;
		    
		    SctpGapAckBlock_S *spGapAckBlock = 
		      (SctpGapAckBlock_S *) (ucpChunk+iSize);
	      
		    spGapAckBlock->usStartOffset = 
		      dividedGapAckBlockStartTsn - uiCumAck;
		    spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
		    iSize += sizeof(SctpGapAckBlock_S);

		    break;
		  }

		  /* NE: Non-Renegable Gap Ack and Gap Ack Block blocks left 
		     edges are same so let's skip the Non-Renegable TSNs from 
		     the Gap Ack Block and continue with the remainder TSNs, 
		     also advance to the next Non-Renegable Gap Ack Block */
		  if (dividedGapAckBlockStartTsn == nonRenegGapAckBlockStartTsn) {
		    
		    DBG_PL(GenChunk, "GapAckBlock Skipping StartTSN=%d EndTSN=%d"), 
		      nonRenegGapAckBlockStartTsn, nonRenegGapAckBlockEndTsn DBG_PR;

		    dividedGapAckBlockStartTsn = nonRenegGapAckBlockEndTsn + 1;
		    
		    spCurrNonRenegGapFrag = spCurrNonRenegGapFrag->spNext;

		    /* NE: If there is no more Non-Renegable Gap Ack Blocks left,
		       add the remaining TSNs to the as Gap Ack Block and exit 
		       loop */
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
			  (dividedGapAckBlockStartTsn - uiCumAck), 
			  (gapAckBlockEndTsn - uiCumAck) DBG_PR;

			SctpGapAckBlock_S *spGapAckBlock = 
			  (SctpGapAckBlock_S *) (ucpChunk+iSize);
			
			spGapAckBlock->usStartOffset = 
			  dividedGapAckBlockStartTsn - uiCumAck;
			spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
			iSize += sizeof(SctpGapAckBlock_S);
		      }
		      break;
		    }
		    else {
		      /* NE: current Non-Renegable Gap Ack Block */
		      nonRenegGapAckBlockStartTsn = 
			((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiStartTsn;
		      nonRenegGapAckBlockEndTsn = 
			((SctpRecvTsnBlock_S *)spCurrNonRenegGapFrag->vpData)->uiEndTsn;

		      DBG_PL(GenChunk, 
			     "Current NonRenegGapAckBlock StartTSN=%d EndTSN=%d"), 
			nonRenegGapAckBlockStartTsn, 
			nonRenegGapAckBlockEndTsn DBG_PR;
		      
		      continue;
		    }

		  } /* NE: let's add the disjoint Gap Ack Block to NR-SACK 
		       chunk */
		  else if (spCurrNonRenegGapFrag != NULL && 
			   dividedGapAckBlockStartTsn < nonRenegGapAckBlockStartTsn) {
		    
		    DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
		      dividedGapAckBlockStartTsn, 
		      (nonRenegGapAckBlockStartTsn - 1) DBG_PR;
		    
		    usNumGapAckBlocks++;

		    /* NE: add the Gap Ack Block to the NR-SACK chunk */
		    DBG_PL(GenChunk, 
			   "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
		      iSize, usNumGapAckBlocks DBG_PR;
		    DBG_PL(GenChunk, 
			   "RenegGapBlock StartOffset=%d EndOffset=%d"), 
		      (dividedGapAckBlockStartTsn - uiCumAck), 
		      (nonRenegGapAckBlockStartTsn - 1 - uiCumAck) DBG_PR;
		    
		    SctpGapAckBlock_S *spGapAckBlock = 
		      (SctpGapAckBlock_S *)(ucpChunk+iSize);
		    
		    spGapAckBlock->usStartOffset = 
		      dividedGapAckBlockStartTsn - uiCumAck;
		    spGapAckBlock->usEndOffset = 
		      (nonRenegGapAckBlockStartTsn-1) - uiCumAck;
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
	  else { /* NE: No more Non-Renegable Gap Ack Blocks exists. So let's 
		    add all remaining TSNs as Gap Ack Blocks. Let's add the 
		    Gap Ack Block to the NR-SACK chunk and advance to the next
		    Gap Ack Block */
	    DBG_PL(GenChunk, "GapAckBlock StartTSN=%d EndTSN=%d"), 
	      gapAckBlockStartTsn, gapAckBlockEndTsn DBG_PR;

	    usNumGapAckBlocks++;
	    
	    /* NE: add the Gap Ack Block to the NR-SACK chunk */
	    DBG_PL(GenChunk, 
		   "RenegSackBlock StartOffset=%d usNumSackBlocks=%d"), 
	      iSize, usNumGapAckBlocks DBG_PR;
	    DBG_PL(GenChunk, 
		   "RenegGapBlock StartOffset=%d EndOffset=%d"), 
	      (gapAckBlockStartTsn - uiCumAck), 
	      (gapAckBlockEndTsn - uiCumAck) DBG_PR;
	    
	    SctpGapAckBlock_S *spGapAckBlock = 
	      (SctpGapAckBlock_S *)(ucpChunk+iSize);
	    
	    spGapAckBlock->usStartOffset = gapAckBlockStartTsn - uiCumAck;
	    spGapAckBlock->usEndOffset = gapAckBlockEndTsn - uiCumAck;
	    iSize += sizeof(SctpGapAckBlock_S);
	  }

	  /* NE: move to the next Gap Ack Block */
	  spCurrGapFrag = spCurrGapFrag->spNext;
	}
      
      DBG_PL(GenChunk, "usNumSackBlocks=%u"), usNumGapAckBlocks DBG_PR;
      
      /*NE: let's set the correct number of gap-ack blocks */
      ((SctpNonRenegSackChunk_S *) ucpChunk)->usNumGapAckBlocks = 
	usNumGapAckBlocks;
      
      /* PN: 5/2007. NR-Sacks */
      /* Append all the NR-Sack Blocks
       */
      
      DBG_PL(GenChunk, 
	     "NonRenegSackBlock StartOffset=%d usNumNonRenegSackBlocks=%d"), 
	iSize, sNonRenegTsnBlockList.uiLength DBG_PR;
      
      for(Node_S *spCurrFrag = sNonRenegTsnBlockList.spHead;
	  (spCurrFrag != NULL) &&
	    (iSize + sizeof(SctpGapAckBlock_S) < uiMaxDataSize); 
	  spCurrFrag = spCurrFrag->spNext, iSize += sizeof(SctpGapAckBlock_S))
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
	    DeleteNode(&sDupTsnList, spPrevDup), iSize += sizeof(SctpDupTsn_S))
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
      spSctpTrace[uiNumChunks].uiTsn = 
	((SctpNonRenegSackChunk_S *) ucpChunk)->uiCumAck;
      spSctpTrace[uiNumChunks].usStreamId = (u_short) -1;
      spSctpTrace[uiNumChunks].usStreamSeqNum = (u_short) -1;
      break;

    case SCTP_CHUNK_FORWARD_TSN:
      iSize = SCTP_CHUNK_FORWARD_TSN_LENGTH;
      ((SctpForwardTsnChunk_S *) ucpChunk)->sHdr.ucType = eType;
      ((SctpForwardTsnChunk_S *) ucpChunk)->sHdr.ucFlags = 0;// flags not used?
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

u_int SctpAgent::GetNextDataChunkSize()
{
  DBG_I(GetNextDataChunkSize);

  AppData_S *spAppMessage = NULL;
  u_int uiChunkSize = 0;

  if(eDataSource == DATA_SOURCE_APPLICATION)
    {
      if(sAppLayerBuffer.uiLength == 0)
	{
	  uiChunkSize = 0;
	}
      else
	{
	  spAppMessage = (AppData_S *) (sAppLayerBuffer.spHead->vpData);
	  uiChunkSize = spAppMessage->uiNumBytes + sizeof(SctpDataChunkHdr_S);
	}
    }
  else
    {
      uiChunkSize = uiDataChunkSize;
    }

  /* DATA chunks must be padded to a 4 byte boundary (section 3.3.1)
   */
  if( (uiChunkSize % 4) != 0)
    uiChunkSize += 4 - (uiChunkSize % 4);

  DBG_PL(GetNextDataChunkSize, "returning %d"), uiChunkSize DBG_PR;
  DBG_X(GetNextDataChunkSize);
  return uiChunkSize;
}

/* This function generates ONE data chunk.Since we could call GenChunk directly
 * with only one extra parameter, it may seem pointless to have this function.
 * However, it isn't! All variable adjustments that go with generating a new
 * data chunk should be isolated in one place to avoid bugs.
 */
int SctpAgent::GenOneDataChunk(u_char *ucpOutData)
{
  DBG_I(GenOneDataChunk);

  AppData_S *spAppData = NULL;
  int iChunkSize = GenChunk(SCTP_CHUNK_DATA, ucpOutData);

  if(eDataSource == DATA_SOURCE_APPLICATION)
    {
      spAppData = (AppData_S *) sAppLayerBuffer.spHead->vpData;
      AddToSendBuffer( (SctpDataChunkHdr_S *) ucpOutData, iChunkSize, 
		       spAppData->usReliability, spNewTxDest);
      DeleteNode(&sAppLayerBuffer, sAppLayerBuffer.spHead);
    }
  else
    {
      AddToSendBuffer( (SctpDataChunkHdr_S *) ucpOutData, iChunkSize, 
		       uiReliability, spNewTxDest);
    }
  
  DBG_PL(GenOneDataChunk, "dest=%p chunk length=%d"), 
    spNewTxDest, ((SctpDataChunkHdr_S *) ucpOutData)->sHdr.usLength DBG_PR;

  spNewTxDest->iOutstandingBytes 
    += ((SctpDataChunkHdr_S *) ucpOutData)->sHdr.usLength;

  DBG_PL(GenOneDataChunk,"After adding chunk, out=%d"), 
    spNewTxDest->iOutstandingBytes DBG_PR;

  /* rfc2960 section 6.2.1.B
   */
  if(iChunkSize <= (int) uiPeerRwnd)
    uiPeerRwnd -= iChunkSize; 
  else
    uiPeerRwnd = 0;

  if(spNewTxDest->eRtxTimerIsRunning == FALSE)
    StartT3RtxTimer(spNewTxDest); //section 6.3.2.R1

  /* PN: 5/2007. Simulating sendwindow */
  if (uiInitialSwnd)
  {
    if(iChunkSize <= (int) uiAvailSwnd)
      uiAvailSwnd -= iChunkSize;
    else
      uiAvailSwnd = 0;

    DBG_PL(GenOneDataChunk, "AvailSendwindow=%ld"), uiAvailSwnd DBG_PR;
  }

  DBG_X(GenOneDataChunk);
  return iChunkSize;
}

/* This function fills a packet with as many chunks that can fit into the PMTU 
 * and the peer's rwnd.
 *
 * Since, this function is called the first time a chunk(s) is sent, all chunks
 * generated here are added to the sSendBuffer.
 *
 * returns the resulting size of the packet
 */
int SctpAgent::GenMultipleDataChunks(u_char *ucpOutData, int iTotalOutDataSize)
{
  DBG_I(GenMultipleDataChunks);
  
  int iChunkSize = 0;
  int iStartingTotalSize = iTotalOutDataSize;

  while((iTotalOutDataSize + GetNextDataChunkSize()  <= uiMaxDataSize) &&
	(GetNextDataChunkSize()                      <= uiPeerRwnd) &&
	(eDataSource == DATA_SOURCE_INFINITE || sAppLayerBuffer.uiLength != 0))
    {
      iChunkSize = GenOneDataChunk(ucpOutData);
      ucpOutData += iChunkSize;
      iTotalOutDataSize += iChunkSize;
    }
  
  DBG_X(GenMultipleDataChunks);
  return iTotalOutDataSize - iStartingTotalSize;
}

/* This function serves as a hook for extensions (such as Timestamp) which
 * need to bundle extra control chunks. However, it can later be used to
 * actually bundle SACKs, FOWARD_TSN, DATA w/ COOKIE and
 * COOKIE_ECHO. Since we don't currently bundle any control chunks in the
 * base SCTP, this function just returns 0 (ie, no bundled chunks).
 *
 * returns the aggregate size of the control chunk(s)
 */
int SctpAgent::BundleControlChunks(u_char *)
{
  DBG_I(BundleControlChunks);
  DBG_PL(BundleControlChunks, "None... returning 0") DBG_PR;
  DBG_X(BundleControlChunks);
  return 0;
}

void SctpAgent::StartT3RtxTimer(SctpDest_S *spDest)
{
  DBG_I(StartT3RtxTimer);
  double dCurrTime = Scheduler::instance().clock();
  DBG_PL(StartT3RtxTimer, "spDest=%p dCurrTime=%f expires at %f "), 
    spDest, dCurrTime, dCurrTime+spDest->dRto DBG_PR;
  spDest->opT3RtxTimer->resched(spDest->dRto);
  spDest->eRtxTimerIsRunning = TRUE;
  DBG_X(StartT3RtxTimer);
}

void SctpAgent::StopT3RtxTimer(SctpDest_S *spDest)
{
  DBG_I(StopT3RtxTimer);
  double dCurrTime = Scheduler::instance().clock();
  DBG_PL(StopT3RtxTimer, "spDest=%p dCurrTime=%f"), spDest, dCurrTime DBG_PR;
  spDest->opT3RtxTimer->force_cancel();
  spDest->eRtxTimerIsRunning = FALSE;
  DBG_X(StopT3RtxTimer);
}

void SctpAgent::AddToSendBuffer(SctpDataChunkHdr_S *spChunk, 
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

void SctpAgent::RttUpdate(double dTxTime, SctpDest_S *spDest)
{
  DBG_I(RttUpdate);

  double dNewRtt;
  double dCurrTime = Scheduler::instance().clock();

  dNewRtt = dCurrTime - dTxTime;
      
  if(spDest->eFirstRttMeasurement == TRUE) // section 6.3.1.C2
    {
      /* Bug Fix. eFirstRttMeasurement should be set to FALSE here.
       * 06/11/2001 - PDA and JRI
       */
      spDest->eFirstRttMeasurement = FALSE; 
      spDest->dSrtt = dNewRtt;
      spDest->dRttVar = dNewRtt/2;
      spDest->dRto = spDest->dSrtt + 4 * spDest->dRttVar;
    }
  else //section 6.3.1.C3
    {
      spDest->dRttVar 
	= ( (1 - RTO_BETA) * spDest->dRttVar 
	    + RTO_BETA * abs(spDest->dSrtt - dNewRtt) );

      spDest->dSrtt 
	= (1 - RTO_ALPHA) * spDest->dSrtt + RTO_ALPHA * dNewRtt;
	    
      spDest->dRto = spDest->dSrtt + 4 * spDest->dRttVar;
    }

  if(spDest->dRto < dMinRto)  // section 6.3.1.C6
    spDest->dRto = dMinRto;
  else if(spDest->dRto > dMaxRto)  // section 6.3.1.C7
    spDest->dRto = dMaxRto;
  tdRto++; // trigger changes for trace to pick up

  DBG_PL(RttUpdate, "spDest->dRto=%f"), spDest->dRto DBG_PR;
  DBG_X(RttUpdate);
}

/* Go thru the send buffer deleting all chunks which have a tsn <= the 
 * tsn parameter passed in. We assume the chunks in the rtx list are ordered by
 * their tsn value. In addtion, for each chunk deleted:
 *   1. we add the chunk length to # newly acked bytes and partial bytes acked
 *   2. we update round trip time if appropriate
 *   3. stop the timer if the chunk's destination timer is running
 */
void SctpAgent::SendBufferDequeueUpTo(u_int uiTsn)
{
  DBG_I(SendBufferDequeueUpTo);

  Node_S *spDeleteNode = NULL;
  Node_S *spCurrNode = sSendBuffer.spHead;
  SctpSendBufferNode_S *spCurrNodeData = NULL;

  /* PN: 5/2007. Simulate send window */
  u_short usChunkSize = 0;
  char cpOutString[500];
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
	    
      /* We update the RTT estimate if the following hold true:
       *   1. RTO pending flag is set (6.3.1.C4 measured once per round trip)
       *   2. Timestamp is set for this chunk(ie, we were measuring this chunk)
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
	   * (See below, where we talk about error count resetting)
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

  if (channel_)
        (void)Tcl_Write(channel_, cpOutString, strlen(cpOutString));

  DBG_X(SendBufferDequeueUpTo);
}

/* This function uses the iOutstandingBytes variable and assumes that it has 
 * NOT been updated yet to reflect the newly received SACK. Hence, it is the 
 * previously outstanding DATA bytes.
 */
void SctpAgent::AdjustCwnd(SctpDest_S *spDest)
{
  DBG_I(AdjustCwnd);

  if(spDest->iCwnd <= spDest->iSsthresh) //in slow-start mode?
    {
      DBG_PL(AdjustCwnd, "slow start mode") DBG_PR;
      DBG_PL(AdjustCwnd, "iSsthresh=%d"), spDest->iSsthresh DBG_PR;

      /* section 7.2.1 (w/ implementor's guide)
       */
      if(spDest->iOutstandingBytes >= spDest->iCwnd)
	{
	  spDest->iCwnd += MIN(spDest->iNumNewlyAckedBytes,(int)uiMaxDataSize);
	  tiCwnd++; // trigger changes for trace to pick up
	}
    }
  else // congestion avoidance mode
    {
      DBG_PL(AdjustCwnd,"congestion avoidance mode") DBG_PR;
      DBG_PL(AdjustCwnd,"iPartialBytesAcked=%d iCwnd=%d iOutStandingBytes=%d"),
	spDest->iPartialBytesAcked, 
	spDest->iCwnd, 
	spDest->iOutstandingBytes
	DBG_PR;

      /* section 7.2.2
       */
      if(spDest->iPartialBytesAcked >= spDest->iCwnd &&
	 spDest->iOutstandingBytes >= spDest->iCwnd )
	{
	  DBG_PL(AdjustCwnd, "adjusting cwnd") DBG_PR;
	  if(spDest->iCwnd <= spDest->iPartialBytesAcked)
	      spDest->iPartialBytesAcked -= spDest->iCwnd;
	  else
	    spDest->iPartialBytesAcked = 0;
	  spDest->iCwnd += uiMaxDataSize;
	  tiCwnd++; // trigger changes for trace to pick up
	}
    }	

  DBG_PL(AdjustCwnd, "pba=%d cwnd=%d out=%d PeerRwnd=%d"),
    spDest->iPartialBytesAcked, 
    spDest->iCwnd, 
    spDest->iOutstandingBytes,
    uiPeerRwnd
    DBG_PR;
  DBG_X(AdjustCwnd);
}

void SctpAgent::AdvancePeerAckPoint()
{
  DBG_I(AdvancePeerAckPoint);

  Node_S *spCurrNode = NULL;
  SctpSendBufferNode_S *spCurrNodeData = NULL;

  if(uiAdvancedPeerAckPoint < uiCumAckPoint)
    uiAdvancedPeerAckPoint = uiCumAckPoint;

  for(spCurrNode = sSendBuffer.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;
      
      /* If we haven't marked the chunk as ack'd either via an u-stream's
       * rtx exhaustion or a gap ack, then jump out of the loop. ...we've
       * advanced as far as we can!
       */
      if(spCurrNodeData->eAdvancedAcked == FALSE &&
	 spCurrNodeData->eGapAcked == FALSE)
	break;

      uiAdvancedPeerAckPoint = spCurrNodeData->spChunk->uiTsn;
    }

  DBG_PL(AdvancePeerAckPoint, "uiAdvancedPeerAckPoint=%d"), 
    uiAdvancedPeerAckPoint DBG_PR;
  DBG_X(AdvancePeerAckPoint);
}

u_int SctpAgent::GetHighestOutstandingTsn()
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
      
      if(spCurrBuffData->eMarkedForRtx == NO_RTX)  // is it oustanding?
	{
	  uiHighestOutstandingTsn = spCurrBuffData->spChunk->uiTsn;// found it!
	  break;
	}
    }
  
  /* NE: update the value of uiHighestOutstandignTSN if it is less than the 
     uiHighestTsnSent */
  if (uiHighestOutstandingTsn < uiHighestTsnSent ) 
    uiHighestOutstandingTsn = uiHighestTsnSent;
  else
    uiHighestTsnSent = uiHighestOutstandingTsn;
  
  DBG_PL(GetHighestOutstandingTsn, "uiHighestOutstandingTsn=%d"), 
    uiHighestOutstandingTsn DBG_PR;
  DBG_PL(GetHighestOutstandingTsn, "uiHighestTsnSent=%d"), 
    uiHighestTsnSent DBG_PR;
  DBG_X(GetHighestOutstandingTsn);

  return uiHighestOutstandingTsn;
}

/* PN: 12/21/2007. NR-SACKs.
 * New method to update last tsn sent.
 * Used to update HighestOutstandingTsn.
 */
void SctpAgent::UpdateHighestTsnSent()
{
  DBG_I(UpdateHighestTsnSent);

  /* start from the tail of the send buffer and search towards the head for the 
   * last tsn sent on the destination... 
   */
  if (sSendBuffer.spTail != NULL && 
	((SctpSendBufferNode_S *)sSendBuffer.spTail->vpData)->spChunk->uiTsn > 
	uiHighestTsnSent ) {
      uiHighestTsnSent = 
	((SctpSendBufferNode_S *)sSendBuffer.spTail->vpData)->spChunk->uiTsn;
    }

  DBG_PL(UpdateHighestTsnSent, "uiHighestTsnSent=%d"),
    uiHighestTsnSent DBG_PR;
 
  DBG_X(UpdateHighestTsnSent);

}

/* This function is called as soon as we are done processing a SACK which 
 * notifies the need of a fast rtx. Following RFC2960, we pack as many chunks
 * as possible into one packet (PTMU restriction). The remaining marked packets
 * are sent as soon as cwnd allows.
 */
void SctpAgent::FastRtx()
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
      if( (spCurrBuffData->eMarkedForRtx != NO_RTX ||
	  spCurrBuffData->eAdvancedAcked == TRUE) &&
	 spCurrBuffData->spDest->eCcApplied == FALSE &&
	 spCurrBuffData->spChunk->uiTsn > uiRecover)
	 
	{ 
	  /* Lukasz Budzisz : 03/09/2006
	     Section 7.2.3 of rfc4960 
	   */
	  spCurrBuffData->spDest->iSsthresh 
	    = MAX(spCurrBuffData->spDest->iCwnd/2, 4 * (int) uiMaxDataSize);
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

	  /* Set the recover variable to avoid multiple cwnd cuts for losses
	   * in the same window (ie, round-trip).
	   */
	  uiRecover = GetHighestOutstandingTsn();
	}
    }

  /* possible that no chunks are pending retransmission since they could be 
   * advanced ack'd 
   */
  if(eMarkedChunksPending == TRUE)  
    RtxMarkedChunks(RTX_LIMIT_ONE_PACKET);

  DBG_X(FastRtx);
}

void SctpAgent::TimeoutRtx(SctpDest_S *spDest)
{
  DBG_I(TimeoutRtx);

  Node_S *spCurrNode = NULL;
  SctpSendBufferNode_S *spCurrNodeData = NULL;
  
  DBG_PL(TimeoutRtx, "spDest=%p"), spDest DBG_PR;

  for(spCurrNode = sSendBuffer.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;

      /* Mark chunks that were sent to the destination which had a timeout and
       * have NOT been gap ack'd or advanced.
       */
      if(spCurrNodeData->spDest == spDest &&
	 spCurrNodeData->eGapAcked == FALSE &&
	 spCurrNodeData->eAdvancedAcked == FALSE)
	{
	  MarkChunkForRtx(spCurrNodeData, TIMEOUT_RTX);
	  spCurrNodeData->iNumMissingReports = 0;
	}
    }

  if(eMarkedChunksPending == TRUE)  
    RtxMarkedChunks(RTX_LIMIT_ONE_PACKET);

  DBG_X(TimeoutRtx);
}

void SctpAgent::MarkChunkForRtx(SctpSendBufferNode_S *spNodeData,
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

  /* let's see if this chunk is on an unreliable stream. if so and the chunk 
   * has run out of retransmissions, mark it as advanced acked and unmark it 
   * for rtx
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
    eMarkedChunksPending = TRUE;

  DBG_PL(MarkChunkForRtx, "uiPeerRwnd=%lu"), uiPeerRwnd DBG_PR;
  DBG_X(MarkChunkForRtx);
}

Boolean_E SctpAgent::AnyMarkedChunks()
{
  DBG_I(AnyMarkedChunks);

  Node_S *spCurrBuffNode = NULL;
  SctpSendBufferNode_S *spCurrBuffNodeData = NULL;

  for(spCurrBuffNode = sSendBuffer.spHead;
      spCurrBuffNode != NULL; 
      spCurrBuffNode = spCurrBuffNode->spNext)
    {
      spCurrBuffNodeData = (SctpSendBufferNode_S *) spCurrBuffNode->vpData;
      if(spCurrBuffNodeData->eMarkedForRtx != NO_RTX)
	{
	  DBG_PL(AnyMarkedChunks, "TRUE") DBG_PR;
	  DBG_X(AnyMarkedChunks);
	  return TRUE;
	}
    }

  DBG_PL(AnyMarkedChunks, "FALSE") DBG_PR;
  DBG_X(AnyMarkedChunks);
  return FALSE;
}

/* This function goes through the entire send buffer filling a packet with 
 * chunks marked for retransmission. Once a packet is full (according to MTU)
 * it is transmittted. If the eLimit is one packet, than that is all that is
 * done. If the eLimit is cwnd, then packets full of marked tsns are sent until
 * cwnd is full.
 */
void SctpAgent::RtxMarkedChunks(SctpRtxLimit_E eLimit)
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

  /* We need to set the destination address for the retransmission(s).We assume
   * that on a given call to this function, all should all be sent to the same
   * address (should be a reasonable assumption). So, to determine the address,
   * we find the first marked chunk and determine the destination it was last 
   * sent to. 
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
		  iBundledControlChunkSize=BundleControlChunks(ucpCurrOutData);
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
   * marked chunks. If we didn't hit the end, let's see if there are more
   * marked chunks.
   */
  eMarkedChunksPending = AnyMarkedChunks();

  DBG_X(RtxMarkedChunks);
  delete [] ucpOutData;
}

/* Updates uiHighestRecvTsn
 */
Boolean_E SctpAgent::UpdateHighestTsn(u_int uiTsn)
{
  DBG_I(UpdateHighestTsn);

  if(uiTsn > uiHighestRecvTsn)
    {
      uiHighestRecvTsn = uiTsn;
      DBG_PL(UpdateHighestTsn, "returning TRUE") DBG_PR;
      DBG_X(UpdateHighestTsn);
      return TRUE;
    } 
  else
    {
      DBG_PL(UpdateHighestTsn, "returning FALSE") DBG_PR;
      DBG_X(UpdateHighestTsn);
      return FALSE; /* no update of highest */ 
    }
}

/* Determines whether a chunk is duplicate or not. 
 */
Boolean_E SctpAgent::IsDuplicateChunk(u_int uiTsn)
{
  DBG_I(IsDuplicateChunk);

  Node_S *spCurrNode = NULL;

  /* Assume highest have already been updated 
   */
  if(uiTsn <= uiCumAck)
    {
      DBG_PL(IsDuplicateChunk, "returning TRUE") DBG_PR;
      DBG_X(IsDuplicateChunk);
      return TRUE;
    }
  if( !((uiCumAck < uiTsn) && (uiTsn <= uiHighestRecvTsn)) )
    {
      DBG_PL(IsDuplicateChunk, "returning FALSE") DBG_PR;
      DBG_X(IsDuplicateChunk);
      return FALSE;
    }

  /* Let's see if uiTsn is in the sorted list of recv'd tsns
   */
  if(sRecvTsnBlockList.uiLength == 0)
    {
      DBG_PL(IsDuplicateChunk, "returning FALSE") DBG_PR;
      DBG_X(IsDuplicateChunk);
      return FALSE;           /* no frags in list! */
    }

  /* If we get this far, we need to check whether uiTsn is already in the list
   * of received tsns. Simply do a linear search thru the sRecvTsnBlockList.
   */
  for(spCurrNode = sRecvTsnBlockList.spHead; 
      spCurrNode != NULL; 
      spCurrNode = spCurrNode->spNext)
    {
      if(((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiStartTsn <= uiTsn &&
	 uiTsn <= ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiEndTsn )
	{
	  DBG_PL(IsDuplicateChunk, "returning TRUE") DBG_PR;
	  DBG_X(IsDuplicateChunk);
	  return TRUE;     
	}

      /* Assuming an ordered list of tsn blocks, don't continue looking if this
       * block ends with a larger tsn than the chunk we currently have.
       */
      if( ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiEndTsn > uiTsn )
	{
	  DBG_PL(IsDuplicateChunk, "returning FALSE") DBG_PR;
	  DBG_X(IsDuplicateChunk);
	  return FALSE;
	}
    }

  DBG_PL(IsDuplicateChunk, "returning FALSE") DBG_PR;
  DBG_X(IsDuplicateChunk);
  return FALSE;
}


/* Inserts uiTsn into the list of duplicates tsns
 */
void SctpAgent::InsertDuplicateTsn(u_int uiTsn)
{
  DBG_I(InsertDuplicateTsn);
  Node_S *spCurrNode = NULL;
  Node_S *spPrevNode = NULL;
  Node_S *spNewNode = NULL;
  u_int uiCurrTsn;

  /* linear search
   */
  for(spPrevNode = NULL, spCurrNode = sDupTsnList.spHead; 
      spCurrNode != NULL;
      spPrevNode = spCurrNode, spCurrNode = spCurrNode->spNext)
    {
      uiCurrTsn = ((SctpDupTsn_S *) spCurrNode->vpData)->uiTsn;
      if(uiTsn <= uiCurrTsn)
	break;
    }

  /* If we reached the end of the list 
   * OR we found the location in the list where it should go (assuming it 
   * isn't already there)... insert it.
   */
  if( (spCurrNode == NULL) || (uiTsn != uiCurrTsn) )
    {
      spNewNode = new Node_S;
      spNewNode->eType = NODE_TYPE_DUP_TSN;
      spNewNode->vpData = new SctpDupTsn_S;
      ((SctpDupTsn_S *) spNewNode->vpData)->uiTsn = uiTsn;
      InsertNode(&sDupTsnList, spPrevNode, spNewNode, spCurrNode);
    }

  DBG_X(InsertDuplicateTsn);
}

/* This function updates uiCumAck (a receive variable) to reflect newly arrived
 * data.
 */
void SctpAgent::UpdateCumAck()
{
  DBG_I(UpdateCumAck);
  Node_S *spCurrNode = NULL;

  if(sRecvTsnBlockList.uiLength == 0)
    {
      DBG_X(UpdateCumAck);
      return;
    }

  for(spCurrNode = sRecvTsnBlockList.spHead; 
      spCurrNode != NULL; 
      spCurrNode = spCurrNode->spNext)
    {
      if( uiCumAck+1 == ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiStartTsn)
	{
	  uiCumAck = ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiEndTsn;
	}
      else
        {
          DBG_X(UpdateCumAck);
          return;
        }
    }

  DBG_X(UpdateCumAck);
}

/**
 * Helper function to do the correct update the received tsn blocks.
 * This function will also call UpdateCumAck() when necessary.
 */
void SctpAgent::UpdateRecvTsnBlocks(u_int uiTsn)
{
  DBG_I(UpdateRecvTsnBlocks);

  u_int uiLow;
  u_int uiHigh;
  u_int uiGapSize;

  Node_S *spCurrNode = NULL;
  Node_S *spPrevNode = NULL;
  Node_S *spNewNode = NULL;

  uiLow = uiCumAck + 1;

  for(spCurrNode = sRecvTsnBlockList.spHead; 
      spCurrNode != NULL; 
      spPrevNode = spCurrNode, spCurrNode = spCurrNode->spNext)
    {
      uiHigh = ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiStartTsn - 1;
      
      /* Does uiTsn fall in the gap?
       */
      if( uiLow <= uiTsn && uiTsn <= uiHigh )
	{
	  uiGapSize = uiHigh - uiLow + 1;

	  if(uiGapSize > 1) // is the gap too big for one uiTsn to fill?
	    {
	      /* Does uiTsn border the lower edge of the current tsn block?
	       */
	      if(uiTsn == uiHigh) 
		{
		  ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiStartTsn 
		    = uiTsn;

		  UpdateCumAck();
		  DBG_X(UpdateRecvTsnBlocks);
		  return;
		} 

	      /* Does uiTsn border the left edge of the previous tsn block?
	       */
	      else if(uiTsn == uiLow)
		{
		  if(uiTsn == uiCumAck+1) // can we increment our uiCumAck?
		    {
		      uiCumAck++;
		      UpdateCumAck();
		      DBG_X(UpdateRecvTsnBlocks);
		      return;
		    }
		  else // otherwise, move previous tsn block's right edge
		    {
		      ((SctpRecvTsnBlock_S *)spPrevNode->vpData)->uiEndTsn 
			= uiTsn;

		      UpdateCumAck();
		      DBG_X(UpdateRecvTsnBlocks);
		      return;
		    }
		}

	      /* This uiTsn creates a new tsn block in between uiLow & uiHigh
	       */
	      else 
		{   
		  spNewNode = new Node_S;
		  spNewNode->eType = NODE_TYPE_RECV_TSN_BLOCK;
		  spNewNode->vpData = new SctpRecvTsnBlock_S;
		  ((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiStartTsn 
		    = uiTsn;
		  ((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiEndTsn = uiTsn;

		  InsertNode(&sRecvTsnBlockList, 
			     spPrevNode, spNewNode, spCurrNode);

		  DBG_X(UpdateRecvTsnBlocks);
		  return; // no UpdateCumAck() necessary
		}
	    } 

	  else // uiGapSize == 1
	    {
	      if(uiLow == uiCumAck+1)  // can we adjust our uiCumAck?
		{
		  /* delete tsn block
		   */
		  uiCumAck 
		    = ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiEndTsn;

		  DeleteNode(&sRecvTsnBlockList, spCurrNode);
		  spCurrNode = NULL;
		  UpdateCumAck();
		  DBG_X(UpdateRecvTsnBlocks);
		  return;
		} 
	      else  // otherwise, move previous tsn block's right edge
		{
		  ((SctpRecvTsnBlock_S *)spPrevNode->vpData)->uiEndTsn
		    = ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiEndTsn;

		  DeleteNode(&sRecvTsnBlockList, spCurrNode);
		  spCurrNode = NULL;
		  UpdateCumAck();
		  DBG_X(UpdateRecvTsnBlocks);
		  return;
		}
	    }
	} 

      /* uiTsn is not in the gap between these two tsn blocks, so let's move 
       * our "low pointer" to one past the end of the current tsn block and 
       * continue
       */
      else 
	{         
	  uiLow =  ((SctpRecvTsnBlock_S *)spCurrNode->vpData)->uiEndTsn + 1;
	}
    }

  /* If we get here, then the list is either NULL or the end of the list has
   * been reached 
   */
  if(uiTsn == uiLow) 
    {
      if(uiTsn == uiCumAck+1) // Can we increment the uiCumAck?
	{
	  uiCumAck = uiTsn;
	  UpdateCumAck();
	  DBG_X(UpdateRecvTsnBlocks);
	  return;
        }
      
      /* Update previous tsn block by increasing it's uiEndTsn
       */
      if(spPrevNode != NULL)
	{
	  ((SctpRecvTsnBlock_S *) spPrevNode->vpData)->uiEndTsn++;	  
	}
      DBG_X(UpdateRecvTsnBlocks);
      return; // no UpdateCumAck() necessary
    } 

  /* uiTsn creates a new tsn block to go at the end of the sRecvTsnBlockList
   */
  else 
    {
      spNewNode = new Node_S;
      spNewNode->eType = NODE_TYPE_RECV_TSN_BLOCK;
      spNewNode->vpData = new SctpRecvTsnBlock_S;
      ((SctpRecvTsnBlock_S *) spNewNode->vpData)->uiStartTsn = uiTsn;
      ((SctpRecvTsnBlock_S *) spNewNode->vpData)->uiEndTsn = uiTsn;
      InsertNode(&sRecvTsnBlockList, spPrevNode, spNewNode, spCurrNode);
      DBG_X(UpdateRecvTsnBlocks);
      return; // no UpdateCumAck() necessary
    }
}

/* This function is merely a hook for future implementation and currently does
 * NOT actually pass the data to the upper layer.
 */
void SctpAgent::PassToUpperLayer(SctpDataChunkHdr_S *spDataChunkHdr)
{
  DBG_I(PassToUpperLayer);
  DBG_PL(PassToUpperLayer, "tsn=%d"), spDataChunkHdr->uiTsn DBG_PR;

  /* We really don't want to credit the window until the upper layer actually
   * wants the data, but now we'll assume that the application readily
   * consumes incoming chunks immediately.  
   */
  uiMyRwnd += spDataChunkHdr->sHdr.usLength; 
  tiRwnd++; // trigger changes to be traced

  DBG_PL(PassToUpperLayer, "uiMyRwnd=%d"), uiMyRwnd DBG_PR;
  DBG_X(PassToUpperLayer);
}

void SctpAgent::InsertInStreamBuffer(List_S *spBufferedChunkList,
				     SctpDataChunkHdr_S *spChunk)
{
  DBG_I(InsertInStreamBuffer);

  Node_S *spPrevNode;
  Node_S *spCurrNode;
  Node_S *spNewNode;
  SctpStreamBufferNode_S *spCurrNodeData;
  SctpStreamBufferNode_S *spNewNodeData;
  u_short usCurrStreamSeqNum;

  spPrevNode = NULL;
  spCurrNode = spBufferedChunkList->spHead;

  /* linear search through stream sequence #'s 
   */ 
  for(spPrevNode = NULL, spCurrNode = spBufferedChunkList->spHead;
      spCurrNode != NULL;
      spPrevNode = spCurrNode, spCurrNode = spCurrNode->spNext)
    {
      spCurrNodeData = (SctpStreamBufferNode_S *) spCurrNode->vpData;
      usCurrStreamSeqNum = spCurrNodeData->spChunk->usStreamSeqNum;

      /* break out of loop when we find the place to insert our new chunk
       */
      if(spChunk->usStreamSeqNum <= usCurrStreamSeqNum)
	break;
    }
  
  /* If we reached the end of the list OR we found the location in the list
   * where it should go (assuming it isn't already there)... insert it.  
   */
  if( (spCurrNode == NULL) || (usCurrStreamSeqNum != spChunk->usStreamSeqNum) )
    {
      spNewNode = new Node_S;
      spNewNode->eType = NODE_TYPE_STREAM_BUFFER;
      spNewNode->vpData = new SctpStreamBufferNode_S;
      spNewNodeData = (SctpStreamBufferNode_S *) spNewNode->vpData;
	
      /* This can NOT simply be a 'new SctpDataChunkHdr_S', because we
       * need to allocate the space for the ENTIRE data chunk and not just
       * the data chunk header.  
       */
      spNewNodeData->spChunk 
	= (SctpDataChunkHdr_S *) new u_char[spChunk->sHdr.usLength];
      memcpy(spNewNodeData->spChunk, spChunk, spChunk->sHdr.usLength);

      DBG_PL(InsertInStreamBuffer, "vpData=%p spChunk=%p"), 
	spNewNodeData, spNewNodeData->spChunk DBG_PR;

      InsertNode(spBufferedChunkList, spPrevNode, spNewNode, spCurrNode);
    }

  DBG_X(InsertInStreamBuffer);
}

/* We have not implemented fragmentation, so this function safely assumes all
 * chunks are complete. 
 */
void SctpAgent::PassToStream(SctpDataChunkHdr_S *spChunk)
{
  DBG_I(PassToStream);
  DBG_PL(PassToStream, "uiMyRwnd=%d"), uiMyRwnd DBG_PR;

  /* PN: 5/2007. NR-Sacks */
  Node_S *spNewNode = NULL;

  SctpInStream_S *spStream = &(spInStreams[spChunk->usStreamId]);

  if(spChunk->sHdr.ucFlags & SCTP_DATA_FLAG_UNORDERED)
    {
      PassToUpperLayer(spChunk);

      /* PN: 5/2007. NR-Sacks */
      if ((eUseNonRenegSacks == TRUE) && (spChunk->uiTsn > uiCumAck))
	{
	  spNewNode = new Node_S;
	  spNewNode->eType = NODE_TYPE_TSN;
	  spNewNode->vpData = new SctpTsn_S;
	  ((SctpTsn_S *)spNewNode->vpData)->uiTsn = spChunk->uiTsn;
	  InsertNodeInSortedList(&sNonRenegTsnList, spNewNode);
	  DBG_PL(PassToStream, "Chunk with TSN=%d added to NR-Sack List"), 
	    spChunk->uiTsn DBG_PR;
	}
    } 
  else 
    {
      /* We got a numbered chunk (ordered delivery)...
       *
       * We insert the chunk into the corresponding buffer whether or not the 
       * chunk's stream seq # is in order or not.
       */
      DBG_PL(PassToStream, "streamId=%d streamSeqNum=%d"),
	spChunk->usStreamId, spChunk->usStreamSeqNum DBG_PR;

      InsertInStreamBuffer( &(spStream->sBufferedChunkList), spChunk);
    }

  DBG_PL(PassToStream, "uiMyRwnd=%d"), uiMyRwnd DBG_PR;
  DBG_X(PassToStream);
}  

void SctpAgent::UpdateAllStreams()
{
  DBG_I(UpdateAllStreams);
  DBG_PL(UpdateAllStreams, "uiMyRwnd=%d"), uiMyRwnd DBG_PR;

  Node_S *spCurrNode = NULL;
  Node_S *spDeleteNode = NULL;
  SctpDataChunkHdr_S *spBufferedChunk = NULL;
  SctpInStream_S *spStream = NULL;
  int i;

  /* PN: 5/2007. NR-Sacks */
  Node_S *spNewNode = NULL;

  for(i = 0; i < iNumInStreams; i++)
    {
      DBG_PL(UpdateAllStreams, "examining stream %d"), i DBG_PR;
      spStream = &(spInStreams[i]);

      /* Start from the lowest stream seq # buffered and sequentially pass
       * up all the chunks which are "deliverable".  
       */
      spCurrNode = spStream->sBufferedChunkList.spHead;
      while(spCurrNode != NULL)
	{
	  spBufferedChunk 
	    = ((SctpStreamBufferNode_S *)spCurrNode->vpData)->spChunk;

	  /* For unreliable streams... Any waiting tsn which is less than
	   * or equal to the cum ack, must be delivered now. We first
	   * deliver chunks without even considering the SSNs, because in
	   * case of unreliable streams, there may be gaps in the SSNs
	   * which we want to ignore.
	   */
	  if((spStream->eMode == SCTP_STREAM_UNRELIABLE) &&
	     (spBufferedChunk->uiTsn <= uiCumAck) )
	    {
	      spStream->usNextStreamSeqNum = spBufferedChunk->usStreamSeqNum+1;
	      PassToUpperLayer(spBufferedChunk);
	      spDeleteNode = spCurrNode;
	      spCurrNode = spCurrNode->spNext;
	      DeleteNode( &(spStream->sBufferedChunkList), spDeleteNode );
	      spDeleteNode = NULL;
	    }
	  
	  /* Let's see if we can deliver anything else based on the SSNs.
	   */
	  else if( spBufferedChunk->usStreamSeqNum == 
		   spStream->usNextStreamSeqNum )
	    {
	      spStream->usNextStreamSeqNum++;
	      PassToUpperLayer(spBufferedChunk);

		/* PN: 5/2007. NR-Sacks */
	      if ((eUseNonRenegSacks == TRUE) && 
	        (spBufferedChunk->uiTsn > uiCumAck))
	      {
		  spNewNode = new Node_S;
		  spNewNode->eType = NODE_TYPE_TSN;
		  spNewNode->vpData = new SctpTsn_S;
		  ((SctpTsn_S *)spNewNode->vpData)->uiTsn = 
		    spBufferedChunk->uiTsn;

		  InsertNodeInSortedList(&sNonRenegTsnList, spNewNode);

	      }

	      spDeleteNode = spCurrNode;
	      spCurrNode = spCurrNode->spNext;
	      DeleteNode( &(spStream->sBufferedChunkList), spDeleteNode );
	      spDeleteNode = NULL;
	    }
	  
	  /* ok, we have delivered all that we can!
	   */
	  else
	    break;
	}
    }

    /* PN: 5/2007. NR-Sacks */
    if (eUseNonRenegSacks == TRUE)  
    {
	// Rebuild NR-Sack Block List 
	BuildNonRenegTsnBlockList();

    }

  DBG_PL(UpdateAllStreams, "uiMyRwnd=%d"), uiMyRwnd DBG_PR;
  DBG_X(UpdateAllStreams);
} 

/* PN: 5/2007. NR-Sacks */
/* Helper function to build Non-Renegable Tsn Block list from a sorted (asc)
 * list of non-renegable Tsns. This function assumes that global uiCumAck var
 * reflects the correct receiver's cumAck. Tsns < cumAck are deleted from the 
 * sorted list and are not be added to NR-Sack blocks(these Tsns are taken care
 * of by uiCumAck in the Sack chunk.
 */
void SctpAgent::BuildNonRenegTsnBlockList()
{

  u_int uiStartTsn = 0, uiEndTsn = 0, uiCurrTsn;
  Node_S *spCurrNode = NULL;
  Node_S *spPrevNode = NULL;
  Node_S *spNewNode = NULL;

  // Rebuild TsnBlocks from the sorted Tsn list
  // This is just simpler than adjusting the Sack Block list
  // I plan to look at these optimizations later...
  ClearList(&sNonRenegTsnBlockList);

  spCurrNode = sNonRenegTsnList.spHead;
  while (spCurrNode != NULL)
  {
    uiCurrTsn = ((SctpTsn_S *)spCurrNode->vpData)->uiTsn;
    if (uiCurrTsn <= uiCumAck)
    {
      // Tsns < cumack should/need not be NR-Sacked. Delete these Tsns.
      spPrevNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;
      DeleteNode(&sNonRenegTsnList, spPrevNode);

      // reset for next block
      spPrevNode = NULL;
      uiStartTsn = 0;
      continue;
    }

    if (!uiStartTsn)
    {
      uiStartTsn = uiEndTsn = uiCurrTsn;
      spPrevNode = spCurrNode;

      /* NE: if single and last tsn block */
      if (spCurrNode->spNext == NULL) {
	
	// Create and add this block
	spNewNode = NULL;
	spNewNode = new Node_S;
	spNewNode->eType = NODE_TYPE_RECV_TSN_BLOCK;
	spNewNode->vpData = new SctpRecvTsnBlock_S;
	((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiStartTsn = uiCurrTsn;
	((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiEndTsn = uiCurrTsn;
	
	InsertNode(&sNonRenegTsnBlockList, sNonRenegTsnBlockList.spTail,
		   spNewNode, NULL);
	break;
      }
    }
    else
    {
      if (((SctpTsn_S *)spPrevNode->vpData)->uiTsn == (uiCurrTsn - 1)) 
      {
	// consecutive Tsns; They belong to the same block
	uiEndTsn = uiCurrTsn;
      }
      
      if (((SctpTsn_S *)spPrevNode->vpData)->uiTsn != (uiCurrTsn - 1)
	  /* Non consecutive tsns */
	  || (spCurrNode->spNext == NULL))
	  /* last tsn in sNonRenegTsnList */
	{ 
	  // Create and add this block
	  spNewNode = new Node_S;
	  spNewNode->eType = NODE_TYPE_RECV_TSN_BLOCK;
	  spNewNode->vpData = new SctpRecvTsnBlock_S;
	  ((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiStartTsn = uiStartTsn;
	  ((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiEndTsn = uiEndTsn;
	
	  InsertNode(&sNonRenegTsnBlockList, sNonRenegTsnBlockList.spTail,
		     spNewNode, NULL);

	  /* NE: last TSN which creates a gap block with single TSN in it */
	  if ((spCurrNode->spNext == NULL) && (uiEndTsn != uiCurrTsn)) {
	    
 	    // Create and add this block
 	    spNewNode = NULL;
	    spNewNode = new Node_S;
 	    spNewNode->eType = NODE_TYPE_RECV_TSN_BLOCK;
 	    spNewNode->vpData = new SctpRecvTsnBlock_S;
 	    ((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiStartTsn = uiCurrTsn;
 	    ((SctpRecvTsnBlock_S *)spNewNode->vpData)->uiEndTsn = uiCurrTsn;
	    
 	    InsertNode(&sNonRenegTsnBlockList, sNonRenegTsnBlockList.spTail,
		       spNewNode, NULL);
 	  }

	  // initialize for next block
	  uiStartTsn = uiEndTsn = uiCurrTsn;

	}

      spPrevNode = spCurrNode;
    } 
    spCurrNode = spCurrNode->spNext;
  } // end of while

}

void SctpAgent::ProcessInitChunk(u_char *ucpInitChunk)
{
  DBG_I(ProcessInitChunk);

  SctpInitChunk_S *spInitChunk = (SctpInitChunk_S *) ucpInitChunk;
  SctpUnrelStreamPair_S *spUnrelStreamPair 
    = (SctpUnrelStreamPair_S *) (ucpInitChunk + sizeof(SctpInitChunk_S));
  int i = 0;

  uiPeerRwnd = spInitChunk->uiArwnd;
  iNumInStreams = spInitChunk->usNumOutboundStreams;
  spInStreams = new SctpInStream_S[iNumInStreams];
  memset(spInStreams, 0, (iNumInStreams * sizeof(SctpInStream_S)) );

  if(spInitChunk->sHdr.usLength > sizeof(SctpInitChunk_S) )
  {
    for(i = spUnrelStreamPair->usStart; i <= spUnrelStreamPair->usEnd; i++)
      {
	DBG_PL(ProcessInitChunk, "setting inStream %d to UNRELIABLE"),i DBG_PR;
	spInStreams[i].eMode = SCTP_STREAM_UNRELIABLE;
      }
  }
  
  for(; i < iNumInStreams; i++)
    {
      DBG_PL(ProcessInitChunk, "setting inStream %d to RELIABLE"), i DBG_PR;
      spInStreams[i].eMode = SCTP_STREAM_RELIABLE;
    }
  
  DBG_X(ProcessInitChunk);
}

void SctpAgent::ProcessInitAckChunk(u_char *ucpInitAckChunk)
{
  DBG_I(ProcessInitAckChunk);

  SctpInitAckChunk_S *spInitAckChunk = (SctpInitAckChunk_S *) ucpInitAckChunk;
  SctpUnrelStreamPair_S *spUnrelStreamPair 
    = (SctpUnrelStreamPair_S *)(ucpInitAckChunk + sizeof(SctpInitAckChunk_S) );
  int i = 0;

  opT1InitTimer->force_cancel();
  uiPeerRwnd = spInitAckChunk->uiArwnd;
  tiRwnd++; // trigger changes to be traced

  iNumInStreams = spInitAckChunk->usNumOutboundStreams;
  spInStreams = new SctpInStream_S[iNumInStreams];
  memset(spInStreams, 0, (iNumInStreams * sizeof(SctpInStream_S)) );

  if(spInitAckChunk->sHdr.usLength > sizeof(SctpInitAckChunk_S) )
  {
    for(i = spUnrelStreamPair->usStart; i <= spUnrelStreamPair->usEnd; i++)
      {
	DBG_PL(ProcessInitAckChunk, "setting inStream %d to UNRELIABLE"), 
	  i DBG_PR;
	spInStreams[i].eMode = SCTP_STREAM_UNRELIABLE;
      }
  }
  
  for(; i < iNumInStreams; i++)
    {
      DBG_PL(ProcessInitAckChunk, "setting inStream %d to RELIABLE"), i DBG_PR;
      spInStreams[i].eMode = SCTP_STREAM_RELIABLE;
    }
  
  DBG_X(ProcessInitAckChunk);
}

//void SctpAgent::ProcessCookieEchoChunk(SctpCookieEchoChunk_S *spCookieEchoChunk)
void SctpAgent::ProcessCookieEchoChunk(SctpCookieEchoChunk_S *)
{
  /* dummy empty function left as a hook for cookie echo processing */
}

//void SctpAgent::ProcessCookieAckChunk(SctpCookieAckChunk_S *spCookieAckChunk)
void SctpAgent::ProcessCookieAckChunk(SctpCookieAckChunk_S *)
{
  opT1CookieTimer->force_cancel();
}

/* This function treats only one incoming data chunk at a time.
 */
void SctpAgent::ProcessDataChunk(SctpDataChunkHdr_S *spChunk)
{
  DBG_I(ProcessDataChunk);

  /* Is there still room in my receiver window?? We can only process the DATA
   * chunk if there is. Otherwise, we drop it!
   */
  if(spChunk->sHdr.usLength <= uiMyRwnd) 
    {
      if((UpdateHighestTsn(spChunk->uiTsn) != TRUE) &&
	 (IsDuplicateChunk(spChunk->uiTsn) == TRUE) )
	{
	  InsertDuplicateTsn(spChunk->uiTsn);
	  eSackChunkNeeded = TRUE; // section 6.7 - send sack immediately!
	  DBG_PL(ProcessDataChunk, "duplicate tsn=%d"), spChunk->uiTsn DBG_PR;
	}
      else 
	{
	  /* Received a new chunk...  Reduce receiver window until application
	   * consumes the incoming chunk 
	   */
	  uiMyRwnd -= spChunk->sHdr.usLength;
	  tiRwnd++; // trigger rwnd changes to be traced

	  UpdateRecvTsnBlocks(spChunk->uiTsn);
	  PassToStream(spChunk);
	  UpdateAllStreams();
	  DBG_PL(ProcessDataChunk, "uiMyRwnd=%d"), uiMyRwnd DBG_PR;
	}
    }
  else
    {
      /* Do not generate a SACK if we are dropping the chunk!!
       */
      eSackChunkNeeded = FALSE; 
      DBG_PL(ProcessDataChunk, "rwnd full... dropping tsn=%d"), 
	spChunk->uiTsn DBG_PR;
    }
  
  DBG_X(ProcessDataChunk);
}

/* returns a boolean of whether a fast retransmit is necessary
 */
Boolean_E SctpAgent::ProcessGapAckBlocks(u_char *ucpSackChunk,
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
 
  /* PN: 04/2008 */ 
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
		   * we haven't already incremented it for this TSN
		   */
		  if(( spCurrNodeData->spDest->iCwnd 
		       > spCurrNodeData->spDest->iSsthresh) &&
		     eNewCumAck == TRUE &&
		     spCurrNodeData->eAddedToPartialBytesAcked == FALSE)
		    {
		      DBG_PL(ProcessGapAckBlocks, 
			     "setting eAddedToPartiallyBytesAcked=TRUE")DBG_PR;
		      
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
		   * these sacks to clear the error counter, or else
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
		      tiErrorCount++;                      // ... and trace it!
		      spCurrNodeData->spDest->eStatus=SCTP_DEST_STATUS_ACTIVE;
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
       * have run out of gap ack blocks. In the case that we have run out of 
       * gap ack blocks before we finished running through the send buffer, we
       * need to mark the remaining chunks in the send buffer as 
       * eGapAcked=FALSE. This final marking needs to be done, because we only
       * trust gap ack info from the last SACK. Otherwise, renegging (which we
       * don't do) or out of order SACKs would give the sender an incorrect 
       * view of the peer's rwnd.
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

      DBG_PL(ProcessGapAckBlocks,"now incrementing missing reports...") DBG_PR;
      DBG_PL(ProcessGapAckBlocks,"uiHighestTsnNewlyAcked=%d"), 
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
	      
	      DBG_PL(ProcessGapAckBlocks, 
		     "eNewCumAck = %d uiHighestTsnNewlyAcked = %d uiHighestTsnSacked = %d uiRecover = %d"),
		eNewCumAck, uiHighestTsnNewlyAcked, 
		uiHighestTsnSacked, uiRecover DBG_PR;
	      
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

/* PN: 5/2007. NR-Sacks */
/* Function to process NonReneg Gap Ack Blocks -- deque the TSNs from
 * send buffer and free up send buffer.
 * Assumes GapAckBlocks in the sack chunk have already been processed
 */
void SctpAgent::ProcessNonRenegSackBlocks(u_char *ucpNonRenegSackChunk)
{

  DBG_I(ProcessNonRenegSackBlocks);

  SctpNonRenegSackChunk_S *spNonRenegSackChunk = 
    (SctpNonRenegSackChunk_S *) ucpNonRenegSackChunk;
  u_short usChunkSize = 0, 
    usNumNonRenegSackBlocks = spNonRenegSackChunk->usNumNonRenegSackBlocks;
  SctpGapAckBlock_S *spCurrNonRenegSack = NULL;
  u_char *ucpNonRenegStartOffset = NULL;
  Node_S *spCurrNode = sSendBuffer.spHead;
  Node_S *spDeleteNode = NULL;
  u_int uiStartTsn = 0, uiEndTsn = 0;
  int i = 0; 

  if ((eUseNonRenegSacks == FALSE) || (usNumNonRenegSackBlocks == 0))
  {
      /* Nothing to do */
      return;
  }

  if(sSendBuffer.spHead == NULL) // do we have ANYTHING in the rtx buffer?
  { 
    /* This COULD mean that this sack arrived late, and a previous one
     * already cum ack'd everything. ...so, what do we do? nothing??
     */
    return;
  }
    
  else // we do have chunks in the rtx buffer
  {
    /* PN: 12/21/2007
     * Update Highest Tsn last sent before deleting Tsns from send buffer.
     * This information is needed to correctly update uiHighestOutstandingTsn
     */
    UpdateHighestTsnSent();

    ucpNonRenegStartOffset =  (ucpNonRenegSackChunk
      + sizeof(SctpNonRenegSackChunk_S)
      + (spNonRenegSackChunk->usNumGapAckBlocks * sizeof(SctpGapAckBlock_S)));

      DBG_PL(ProcessNonRenegSackBlocks,
        "CumAck=%d NumNonRenegSackBlocks=%d NonRenegStartOffset=%d"), 
	spNonRenegSackChunk->uiCumAck, usNumNonRenegSackBlocks, 
	(int)(ucpNonRenegStartOffset - ucpNonRenegSackChunk) DBG_PR;

      for (i = 0; i < usNumNonRenegSackBlocks; i++)
      {
 	  spCurrNonRenegSack = (SctpGapAckBlock_S *) (ucpNonRenegStartOffset +
				i * sizeof(SctpGapAckBlock_S));

	  uiStartTsn = spNonRenegSackChunk->uiCumAck 
	    + spCurrNonRenegSack->usStartOffset;
	  uiEndTsn = spNonRenegSackChunk->uiCumAck 
	    + spCurrNonRenegSack->usEndOffset;
	
	  DBG_PL(ProcessNonRenegSackBlocks, "StartTSN=%ld EndTSN=%ld"), 
	    uiStartTsn, uiEndTsn DBG_PR;

	  while (spCurrNode != NULL && ((SctpSendBufferNode_S *)
	         spCurrNode->vpData)->spChunk->uiTsn < uiStartTsn) 
	    {
	      /* not yet reached uiStartTsn in send buffer */
	    
	      DBG_PL(ProcessNonRenegSackBlocks, "Skipping TSN=%ld"), 
		((SctpSendBufferNode_S *)spCurrNode->vpData)->spChunk->uiTsn 
		DBG_PR;
	    
	      spCurrNode = spCurrNode->spNext; 
	      continue; 
	    }

	  while (spCurrNode != NULL && ((SctpSendBufferNode_S *)
		 spCurrNode->vpData)->spChunk->uiTsn <= uiEndTsn) 
	    {
	      spDeleteNode = spCurrNode;
	      spCurrNode = spCurrNode->spNext;
	      usChunkSize = ((SctpSendBufferNode_S *)
			     spDeleteNode->vpData)->spChunk->sHdr.usLength;
	    
	      DBG_PL(ProcessNonRenegSackBlocks, "Deleting TSN=%ld ChunkSize=%d"), 
		((SctpSendBufferNode_S *)spDeleteNode->vpData)->spChunk->uiTsn,
		usChunkSize 
		DBG_PR;

	      DeleteNode(&sSendBuffer, spDeleteNode);
	      spDeleteNode = NULL;

	      /* PN: 5/2007. Simulate send window */
	      if (uiInitialSwnd) 
		{
		  
		  uiAvailSwnd += usChunkSize;
		  
		  DBG_PL(ProcessNonRenegSackBlocks, "AvailSendwindow=%ld"), 
		    uiAvailSwnd DBG_PR;
		} 
	    }
	  
	  if (spCurrNode == NULL)
	    {
	      /* Reached end of send buffer queue; break */
	      break;
	    }
	  
      } /* End for looping through NonRenegSackBlocks */
    
  } /* End else send buffer not empty */

  DBG_X(ProcessNonRenegSackBlocks);

}

void SctpAgent::ProcessSackChunk(u_char *ucpSackChunk)
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

  /* make sure we clear all the iNumNewlyAckedBytes before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->iNumNewlyAckedBytes = 0;
      spCurrDestNodeData->spFirstOutstanding = NULL;
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
	 spCurrDestNodeData->iNumNewlyAckedBytes > 0 &&
	 spSackChunk->uiCumAck >= uiRecover)
	{
	  AdjustCwnd(spCurrDestNodeData);
	}

      /* The number of outstanding bytes is reduced by how many bytes this 
       * sack acknowledges.
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

/* PN: 6/17/07
 * Chunk to process Non-Reneg Sack chunks.
 * Most of the code is similar to ProcessSackChunk. 
 * Is there a way to minimize code repetition here?
 */
void SctpAgent::ProcessNonRenegSackChunk(u_char *ucpNonRenegSackChunk)
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

  /* make sure we clear all the iNumNewlyAckedBytes before using them!
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;
      spCurrDestNodeData->iNumNewlyAckedBytes = 0;
      spCurrDestNodeData->spFirstOutstanding = NULL;
    }

  if(spNonRenegSackChunk->uiCumAck < uiCumAckPoint) 
    {
      /* this cumAck's a previously cumAck'd tsn (ie, it's out of order!)
       * ...so ignore!
       */
      DBG_PL(ProcessNonRenegSackChunk, "ignoring out of order NR-SACK!") DBG_PR;
      DBG_X(ProcessNonRenegSackChunk);
      return;
    }
  else if(spNonRenegSackChunk->uiCumAck > uiCumAckPoint)
    {
      eNewCumAck = TRUE; // incomding SACK's cum ack advances the cum ack point
      SendBufferDequeueUpTo(spNonRenegSackChunk->uiCumAck);
      uiCumAckPoint = spNonRenegSackChunk->uiCumAck; // Advance the cumAck pointer
    }

  /* NE: Are there any gap ack or non-renegable gap ack blocks?? */
  if(usNumGapAckBlocks != 0 || usNumNonRenegGapAckBlocks != 0) 
    {
      eFastRtxNeeded = ProcessGapAckBlocks(ucpNonRenegSackChunk, eNewCumAck);

      /* PN: 5/2007. NR-Sacks */
      /* Process the NR-SACKed TSNs in the send buffer. 
       */
      if (usNumNonRenegGapAckBlocks != 0)

	  /* NonRenegSackBlock processing assumes GapAck blocks in the
	   * sack chunk have already been processed
	   */
	  ProcessNonRenegSackBlocks(ucpNonRenegSackChunk); 
    }

  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      spCurrDestNodeData = (SctpDest_S *) spCurrDestNode->vpData;

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
	 spCurrDestNodeData->iNumNewlyAckedBytes > 0 &&
	 spNonRenegSackChunk->uiCumAck >= uiRecover)
	{
	  AdjustCwnd(spCurrDestNodeData);
	}

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

  DBG_F(ProcessNonRenegSackChunk, DumpSendBuffer());

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
  if(uiTotalOutstanding <= spNonRenegSackChunk->uiArwnd)
    uiPeerRwnd = (spNonRenegSackChunk->uiArwnd  - uiTotalOutstanding);
  else
    uiPeerRwnd = 0;
  
  DBG_PL(ProcessNonRenegSackChunk, "uiPeerRwnd=%d, uiArwnd=%d"), uiPeerRwnd, 
    spNonRenegSackChunk->uiArwnd DBG_PR;
  DBG_X(ProcessNonRenegSackChunk);
}

void SctpAgent::ProcessForwardTsnChunk(SctpForwardTsnChunk_S *spForwardTsnChunk)
{
  DBG_I(ProcessForwardTsnChunk);
  
  int i;
  u_int uiNewCum = spForwardTsnChunk->uiNewCum;

  /* Although we haven't actually received a DATA chunk, we have received
   * a FORWARD CUM TSN chunk, which essentially tells the receiver to
   * pretend that it has received all the DATA chunks up to the forwarded cum.
   * Some of the tsns we are forwarding past have been received, while others
   * have not.
   */
  for(i = uiCumAck+1; i <= (int) uiNewCum; i++)
    {
      if( IsDuplicateChunk(i) == FALSE )  // make sure it hasn't been received
	UpdateRecvTsnBlocks(i);
    }

  UpdateAllStreams();

  DBG_PL(ProcessForwardTsnChunk, "uiCumAck=%d"), uiCumAck DBG_PR;
  DBG_X(ProcessForwardTsnChunk);
}

void SctpAgent::ProcessHeartbeatAckChunk(SctpHeartbeatAckChunk_S 
					 *spHeartbeatAckChunk)
{
  DBG_I(ProcessHeartbeatAckChunk);

  iAssocErrorCount = 0;

  /* trigger trace ONLY if it was previously NOT 0
   */
  
  /* NE - 4/11/2007
   * Change for Confirming Destination Addresses
   * condition (spHeartbeatAckChunk->spDest->iErrorCount != 0) is changed 
   * with (spHeartbeatAckChunk->spDest->iErrorCount >= 0) to enable
   * destination address confirmation when the association moves to 
   * ESTABLISHED state. iErrorCount may be 0 for any given destinations
   * during the confirmation.
   */
  if(spHeartbeatAckChunk->spDest->iErrorCount >= 0)
    {
      DBG_PL(ProcessHeartbeatAckChunk, "marking dest %p to active"),
                        spHeartbeatAckChunk->spDest DBG_PR;
      spHeartbeatAckChunk->spDest->eStatus = SCTP_DEST_STATUS_ACTIVE;
      spHeartbeatAckChunk->spDest->iErrorCount = 0; // clear the error count
      tiErrorCount++;                               // ...and trace it too!

      if(spHeartbeatAckChunk->spDest == spPrimaryDest &&
	 spNewTxDest != spPrimaryDest) 
	{
	  DBG_PL(ProcessHeartbeatAckChunk,
		 "primary recovered... migrating back from %p to %p"),
	    spNewTxDest, spPrimaryDest DBG_PR;
	  spNewTxDest = spPrimaryDest; // return to primary
	}
    }

  RttUpdate(spHeartbeatAckChunk->dTimestamp, spHeartbeatAckChunk->spDest);

  DBG_PL(ProcessHeartbeatAckChunk, "set rto of dest=%p to %f"),
    spHeartbeatAckChunk->spDest, spHeartbeatAckChunk->spDest->dRto DBG_PR;
  
  spHeartbeatAckChunk->spDest->opHeartbeatTimeoutTimer->force_cancel();

  /* Track HB-Timer for CMT-PF 
   */
  spHeartbeatAckChunk->spDest->eHBTimerIsRunning = FALSE;

  DBG_X(ProcessHeartbeatAckChunk);
}

/* This function is left as a hook for extensions to process chunk types not
 * supported in the base scpt. In the base sctp, we simply report the error...
 * the chunk is unexpected and unknown.
 */
void SctpAgent::ProcessOptionChunk(u_char *ucpInChunk)
{
  DBG_I(ProcessOptionChunk);
  double dCurrTime = Scheduler::instance().clock();

  DBG_PL(ProcessOptionChunk, "unexpected chunk type (unknown: %d) at %f"),
    ((SctpChunkHdr_S *)ucpInChunk)->ucType, dCurrTime DBG_PR;
  printf("[ProcessOptionChunk] unexpected chunk type (unknown: %d) at %f\n",
	 ((SctpChunkHdr_S *)ucpInChunk)->ucType, dCurrTime);

  DBG_X(ProcessOptionChunk);
}

int SctpAgent::ProcessChunk(u_char *ucpInChunk, u_char **ucppOutData)
{
  DBG_I(ProcessChunk);
  int iThisOutDataSize = 0;
  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;
  double dCurrTime = Scheduler::instance().clock();
  double dTime;
  SctpHeartbeatAckChunk_S *spHeartbeatChunk = NULL;
  SctpHeartbeatAckChunk_S *spHeartbeatAckChunk = NULL;


  /* NE: 4/11/2007 - Confirming Destinations */
  Boolean_E eThisDestWasUnconfirmed = FALSE;
  Boolean_E eFoundUnconfirmedDest = FALSE;
  /* End of Confirming Destinations */
  
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
	   * still be packets in transit.if and when those packets arrive, they
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
      
      /* NE: 4/11/2007 - Confirming Destinations 
       * Confirming Destination Addresses for sender of INIT.
       * In our implementation destinations are confirmed just for 
       * sender of the INIT. Confirming of destinations is not done for
       * receiver of the INIT.
       *
       * RFC 4460 - section 5.4 - Path Verification
       * Rule 3 - all addresses not covered by rule 1 and 2 are 
       * considered UNCONFIRMED.
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

      /* NE: 4/11/2007 - Confirming Destinations */
      /* Do not send any data until all dests have been confirmed
       * RFC allows to send data on confirmed dests though.
       * This implementation is more conservative than the RFC
       */ 
      eSendNewDataChunks = FALSE;
	  
      break;

    case SCTP_STATE_ESTABLISHED:
      switch( ((SctpChunkHdr_S *)ucpInChunk)->ucType)
	{
	case SCTP_CHUNK_DATA:
	  DBG_PL(ProcessChunk, "got DATA (TSN=%d)!!"), 
	    ((SctpDataChunkHdr_S *)ucpInChunk)->uiTsn DBG_PR;
	  
	  /* NE - 10/2/2007 :After the reception of the first DATA chunk in an 
	   * association the endpoint MUST immediately respond with a SACK to 
	   * acknowledge the DATA chunk. Section 5.1 RFC4960
	   */
	  // are we doing delayed sacks?
	  if(eUseDelayedSacks == FALSE || 
	     ((SctpDataChunkHdr_S *)ucpInChunk)->uiTsn == 1) 
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
		      iDataPktCountSinceLastSack = 0; // reset
		      opSackGenTimer->force_cancel();
		      eSackChunkNeeded = TRUE;
		    }
		}
	    }

	  ProcessDataChunk( (SctpDataChunkHdr_S *) ucpInChunk );

	  /* section 6.7 - There is at least one "gap in the received DATA
	   * chunk sequence", so let's ensure we send a SACK immediately!
	   */
	  if(sRecvTsnBlockList.uiLength > 0)
	    {
	      iDataPktCountSinceLastSack = 0; // reset
	      opSackGenTimer->force_cancel();
	      eSackChunkNeeded = TRUE;
	    }

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

	/* PN: 6/17/2007. NR-Sacks */
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
	  
	  spHeartbeatAckChunk = (SctpHeartbeatAckChunk_S *) ucpInChunk;
	  
	  /* NE: 4/11/2007 - Confirming Destinations */
	  if( spHeartbeatAckChunk->spDest->eStatus ==
	      SCTP_DEST_STATUS_UNCONFIRMED )
	    eThisDestWasUnconfirmed = TRUE;
	  
	  ProcessHeartbeatAckChunk( (SctpHeartbeatAckChunk_S *) ucpInChunk);
	  
	  /* NE: 4/11/2007 - Confirming Destinations */
	  /* Check if all dests have been confirmed. 
           * If yes, allow new data transmission
           */
	  if(eThisDestWasUnconfirmed == TRUE) 
            {
	      for(spCurrNode = sDestList.spHead;
                  spCurrNode != NULL;
                  spCurrNode = spCurrNode->spNext)
                {
                  spCurrDest = (SctpDest_S *) spCurrNode->vpData;
                  if(spCurrDest->eStatus == SCTP_DEST_STATUS_UNCONFIRMED)
		    {
		      eFoundUnconfirmedDest = TRUE;
		      DBG_PL(ProcessChunk, "dest=%p UNCONFIRMED"), 
			spCurrDest DBG_PR;
		      break;
		    }
		}
	  
		if(eFoundUnconfirmedDest == TRUE)
		  break; /* From this case */
		else 
		  {
		    /* All dests have been confirmed */
		    eSendNewDataChunks = TRUE;	
		    
		    /* Moved code from COOKIE ACK recvd */
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
		    
		    DBG_PL(ProcessChunk, "All destinations confirmed!") 
		      DBG_PR;
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

/* Moves the pointer to the next chunk.
 * If there is no next chunk, then sent pointer to NULL. 
 */
void SctpAgent::NextChunk(u_char **ucpChunk, int *ipRemainingDataLen)
{
  DBG_I(NextChunk);
  unsigned long ulSize = ((SctpChunkHdr_S *) *ucpChunk)->usLength;

  /* the chunk length field does not include the padded bytes, so we need to
   * account for these extra bytes.
   */
  if( (ulSize % 4) != 0 ) 
    ulSize += 4 - (ulSize % 4);

  *ipRemainingDataLen -= ulSize;

  if(*ipRemainingDataLen > 0)
    *ucpChunk += ulSize;
  else
    *ucpChunk = NULL;

  DBG_X(NextChunk);
}

/* Return the next active destination based on the destination passed in by 
 * using round robin algorithm for selection. It is possible that this function
 * will return the same destination passed in. This will happen if (1) there is
 * only one destination and/or (2) all other destinations are inactive.
 */
SctpDest_S *SctpAgent::GetNextDest(SctpDest_S *spDest)
{
  DBG_I(GetNextDest);
  DBG_PL(GetNextDest, "previously dest = %p"), spDest DBG_PR;

  Node_S *spCurrDestNode = NULL;
  Node_S *spNextDestNode = NULL;
  SctpDest_S *spNextDestNodeData = NULL;

  /* find spDest before we can pick next
   */
  for(spCurrDestNode = sDestList.spHead;
      spCurrDestNode != NULL;
      spCurrDestNode = spCurrDestNode->spNext)
    {
      if(spCurrDestNode->vpData == spDest)
	break; // found it, so jump out!
    }

  assert(spCurrDestNode != NULL); // if NULL, means spDest wasn't found :-/

  /* let's get the next ACTIVE destination. stop if loops wraps around to 
   * current destination.
   */
  spNextDestNode = spCurrDestNode;
  do
    {
      spNextDestNode = spNextDestNode->spNext;
      if(spNextDestNode == NULL)
	spNextDestNode = sDestList.spHead; // wrap around to head of list
      spNextDestNodeData = (SctpDest_S *) spNextDestNode->vpData;      
    } while(spNextDestNodeData->eStatus == SCTP_DEST_STATUS_INACTIVE &&
	    spNextDestNode != spCurrDestNode);

  /* Are we in the dormant state?
   */
  if(spNextDestNode == spCurrDestNode &&
     spNextDestNodeData->eStatus == SCTP_DEST_STATUS_INACTIVE)
    {
      switch(eDormantAction)
	{
	case DORMANT_HOP:
	  /*
	   * Simply go to the next destination. We don't care if it is inactive
	   * or if it is the same destination we are already on.
	   */
	  spNextDestNode = spNextDestNode->spNext;
	  if(spNextDestNode == NULL)
	    spNextDestNode = sDestList.spHead; // wrap around to head of list
	  spNextDestNodeData = (SctpDest_S *) spNextDestNode->vpData;      
	  break;

	case DORMANT_PRIMARY:
	  spNextDestNodeData = spPrimaryDest;
	  break;

	case DORMANT_LASTDEST:  
	  /*
	   * Nothing to do. Since we just want to stay at the last destination
	   * used before dormant state, then the loop above does that for us
	   * already.
	   */
	  break;
	}
    }

  DBG_PL(GetNextDest, "next dest = %p"), spNextDestNodeData DBG_PR;
  DBG_X(GetNextDest);
  return spNextDestNodeData;
}

/* section 8.3 (with implementer's guide changes) - On an idle destination
 * address that is allowed to heartbeat, a HEARTBEAT chunk is RECOMMENDED
 * to be sent once per RTO of that destination address plus the protocol
 * parameter 'HB.interval' , with jittering of +/- 50% of the RTO value
 */
double SctpAgent::CalcHeartbeatTime(double dRto)
{
  DBG_I(CalcHeartbeatTime);

  double dRetVal = 0;
  double dCurrTime = Scheduler::instance().clock();

  dRetVal = dRto + uiHeartbeatInterval;
  dRetVal += Random::uniform(dRto);
  dRetVal -= dRto/2;

  DBG_PL(CalcHeartbeatTime, "next HEARTBEAT interval in %f secs"), dRetVal
    DBG_PR;
  DBG_PL(CalcHeartbeatTime, "next HEARTBEAT interval at %f"), 
    dRetVal+dCurrTime DBG_PR;

  DBG_X(CalcHeartbeatTime);
  return dRetVal;
}

/* Sets the source addr, port, and target based on the destination information.
 * The SctpDest_S holds an already formed packet with the destination's
 * dest addr & port in the header. With this packet, we can find the correct
 * src addr, port, and target.  */
void SctpAgent::SetSource(SctpDest_S *spDest)
{
  DBG_I(SetSource);

  /* dynamically pick the route only if 2 conditions hold true:
   *    1. we are not forcing the source address and interface. 
   *    2. there exists a "multihome core" (ie, the agent is multihomed)
   *
   * otherwise, use the values are already preset
   */
  if(eForceSource == FALSE && opCoreTarget != NULL)
    {
      Node_S *spCurrNode = sInterfaceList.spHead;
      SctpInterface_S *spCurrInterface = NULL;

      /* find the connector (link) to use from the "routing table" of the core
       */
      Connector *connector 
	= (Connector *) opCoreTarget->find(spDest->opRoutingAssistPacket);

      while(spCurrNode != NULL)
	{
	  spCurrInterface = (SctpInterface_S *) spCurrNode->vpData;
	  
	  if(spCurrInterface->opLink == connector)
	    {
	      addr() = spCurrInterface->iNsAddr;
	      port() = spCurrInterface->iNsPort;
	      target_ = spCurrInterface->opTarget;
	      break;
	    }
	  else
	    spCurrNode = spCurrNode->spNext;
	}
    }

  DBG_PL(SetSource, "(%d:%d)"), addr(), port() DBG_PR;
  DBG_X(SetSource);
}

void SctpAgent::SetDestination(SctpDest_S *spDest)
{
  DBG_I(SetDestination);

  daddr() = spDest->iNsAddr;
  dport() = spDest->iNsPort;

  DBG_PL(SetDestination, "(%d:%d)"), daddr(), dport() DBG_PR;
  DBG_X(SetDestination);
}

void SctpAgent::SendPacket(u_char *ucpData, int iDataSize, SctpDest_S *spDest)
{
  DBG_I(SendPacket);
  DBG_PL(SendPacket, "spDest=%p (%d:%d)"), 
    spDest, spDest->iNsAddr, spDest->iNsPort DBG_PR;
  DBG_PL(SendPacket, "iDataSize=%d  uiNumChunks=%d"), 
    iDataSize, uiNumChunks DBG_PR;

  Node_S *spNewNode = NULL;
  Packet *opPacket = NULL;
  PacketData *opPacketData = NULL;

  SetSource(spDest); // set src addr, port, target based on "routing table"
  SetDestination(spDest); // set dest addr & port 

  opPacket = allocpkt();
  opPacketData = new PacketData(iDataSize);
  memcpy(opPacketData->data(), ucpData, iDataSize);
  opPacket->setdata(opPacketData);
  hdr_cmn::access(opPacket)->size() = iDataSize + SCTP_HDR_SIZE+uiIpHeaderSize;

  hdr_sctp::access(opPacket)->NumChunks() = uiNumChunks;
  hdr_sctp::access(opPacket)->SctpTrace() = new SctpTrace_S[uiNumChunks];
  memcpy(hdr_sctp::access(opPacket)->SctpTrace(), spSctpTrace, 
	 (uiNumChunks * sizeof(SctpTrace_S)) );

  uiNumChunks = 0; // reset the counter

  if(dRouteCalcDelay == 0) // simulating reactive routing overheads?
    {
      send(opPacket, 0); // no... so send immediately
    }
  else
    {
      if(spDest->eRouteCached == TRUE)  
	{
	  /* Since the route is "cached" already, we can reschedule the route
	   * flushing and send the packet immediately.
	   */
	  spDest->opRouteCacheFlushTimer->resched(dRouteCacheLifetime);
	  send(opPacket, 0);
	}
      else
	{
	  /* The route isn't cached, so queue the packet and "calculate" the 
	   * route.
	   */
	  spNewNode = new Node_S;
	  spNewNode->eType = NODE_TYPE_PACKET_BUFFER;
	  spNewNode->vpData = opPacket;
	  InsertNode(&spDest->sBufferedPackets,spDest->sBufferedPackets.spTail,
		     spNewNode, NULL);

	  if(spDest->opRouteCalcDelayTimer->status() != TIMER_PENDING)
	    spDest->opRouteCalcDelayTimer->sched(dRouteCalcDelay);
	}
    }

  DBG_X(SendPacket);
}

SctpDest_S *SctpAgent::GetReplyDestination(hdr_ip *spIpHdr)
{
  Node_S *spCurrNode = NULL;
  SctpDest_S *spDest = NULL;
  int iAddr = spIpHdr->saddr();
  int iPort = spIpHdr->sport();
  
  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spDest = (SctpDest_S *) spCurrNode->vpData;

      if(spDest->iNsAddr == iAddr && spDest->iNsPort == iPort)
	return spDest;
    }
  
  return NULL;  // we should never get here!
}

void SctpAgent::recv(Packet *opInPkt, Handler*)
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

      /* PN: 6/17/2007
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

  /* Do we want to send out new DATA chunks in addition to whatever we may have
   * already transmitted? If so, we can only send new DATA if no marked chunks
   * are pending retransmission.
   * 
   * Note: We aren't bundling with what was sent above, but we could. Just 
   * avoiding that for now... why? simplicity :-)
   */
  if(eSendNewDataChunks == TRUE && eMarkedChunksPending == FALSE) 
    {
      SendMuch();       // Send new data till our cwnd is full!
      eSendNewDataChunks = FALSE; // reset AFTER sent (o/w breaks dependencies)
    }

  delete hdr_sctp::access(opInPkt)->SctpTrace();
  hdr_sctp::access(opInPkt)->SctpTrace() = NULL;
  Packet::free(opInPkt);
  opInPkt = NULL;
  DBG_X(recv);
  delete [] ucpOutData;
}

u_int SctpAgent::TotalOutstanding()
{
  DBG_I(TotalOutstanding);

  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;
  u_int uiTotalOutstanding = 0;

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrDest = (SctpDest_S *) spCurrNode->vpData;
      uiTotalOutstanding += spCurrDest->iOutstandingBytes;
      DBG_PL(TotalOutstanding, "spCurrDest->iOutstandingBytes=%lu"),
	spCurrDest->iOutstandingBytes DBG_PR;
    }

  DBG_PL(TotalOutstanding, "%lu"), uiTotalOutstanding DBG_PR;
  DBG_X(TotalOutstanding);
  return uiTotalOutstanding;
}

void SctpAgent::SendMuch()
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

  /* PN: 5/2007. Send window simulation */
  u_int uiChunkSize = 0;

  /* Keep sending out packets until our cwnd is full!  The proposed
   * correction to RFC2960 section 6.1.B says "The sender may exceed
   * cwnd by up to (PMTU-1) bytes on a new transmission if the cwnd is
   * not currently exceeded". Hence, as long as cwnd isn't
   * full... send another packet.  
   *
   * Also, if our data source is the application layer (instead of the infinite
   * source used for ftp), check if there is any data buffered from the app 
   * layer. If so, then send as much as we can.
   */
  while((spNewTxDest->iOutstandingBytes < spNewTxDest->iCwnd) &&
	(eDataSource == DATA_SOURCE_INFINITE || sAppLayerBuffer.uiLength != 0))
    {
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

      uiChunkSize = GetNextDataChunkSize(); 
      if(uiChunkSize <= uiPeerRwnd)
	{
	  /* This addresses the proposed change to RFC2960 section 7.2.4,
	   * regarding using of Max.Burst. We have an option which allows
	   * to control if Max.Burst is applied.
	   */
	  if(eUseMaxBurst == MAX_BURST_USAGE_ON)
	    if( (eApplyMaxBurst == TRUE) && (uiBurstLength++ >= MAX_BURST) )
	      {
		/* we've reached Max.Burst limit, so jump out of loop
		 */
		eApplyMaxBurst = FALSE;// reset before jumping out of loop
		break;
	      }

	  /* PN: 5/2007. Simulating Sendwindow */ 
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
	  iOutDataSize += GenMultipleDataChunks(ucpOutData+iOutDataSize, 0);
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
    }

  /* this reset MUST happen at the end of the function, because the burst 
   * measurement is carried over from RtxMarkedChunks() if it was called.
   */
  uiBurstLength = 0;

  DBG_X(SendMuch);
  delete [] ucpOutData;
}

void SctpAgent::sendmsg(int iNumBytes, const char *cpFlags)
{
  /* Let's make sure that a Reset() is called, because it isn't always
   * called explicitly with the "reset" command. For example, wireless
   * nodes don't automatically "reset" their agents, but wired nodes do. 
   */
  if(eState == SCTP_STATE_UNINITIALIZED)
    Reset();

  DBG_I(sendmsg);

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  int iOutDataSize = 0;
  AppData_S *spAppData = (AppData_S *) cpFlags;
  Node_S *spNewNode = NULL;
  int iMsgSize = 0;
  u_int uiMaxFragSize = uiMaxDataSize - sizeof(SctpDataChunkHdr_S);

  if(iNumBytes == -1) 
    eDataSource = DATA_SOURCE_INFINITE;    // Send infinite data
  else 
    eDataSource = DATA_SOURCE_APPLICATION; // Send data passed from app
      
  if(eDataSource == DATA_SOURCE_APPLICATION) 
    {
      if(spAppData != NULL)
	{
	  /* This is an SCTP-aware app!! Anything the app passes down
	   * overrides what we bound from TCL.
	   */
	  DBG_PL (sendmsg, "sctp-aware app: iNumBytes=%d"), iNumBytes DBG_PR;
	  spNewNode = new Node_S;
	  uiNumOutStreams = spAppData->usNumStreams;
	  uiNumUnrelStreams = spAppData->usNumUnreliable;
	  spNewNode->eType = NODE_TYPE_APP_LAYER_BUFFER;
	  spNewNode->vpData = spAppData;
	  InsertNode(&sAppLayerBuffer, sAppLayerBuffer.spTail, spNewNode,NULL);
	}
      else
	{
	  /* This is NOT an SCTP-aware app!! We rely on TCL-bound variables.
	   */
	  DBG_PL (sendmsg,"non-sctp-aware app: iNumBytes=%d"),iNumBytes DBG_PR;
	  uiNumOutStreams = 1; // non-sctp-aware apps only use 1 stream
	  uiNumUnrelStreams = (uiNumUnrelStreams > 0) ? 1 : 0;

	  /* To support legacy applications and uses such as "ftp send
	   * 12000", we "fragment" the message. _HOWEVER_, this is not
	   * REAL SCTP fragmentation!! We do not maintain the same SSN or
	   * use the B/E bits. Think of this block of code as a shim which
	   * breaks up the message into useable pieces for SCTP. 
	   */
	  for(iMsgSize = iNumBytes; 
	      iMsgSize > 0; 
	      iMsgSize -= MIN(iMsgSize, (int) uiMaxFragSize) )
	    {
	      spNewNode = new Node_S;
	      spNewNode->eType = NODE_TYPE_APP_LAYER_BUFFER;
	      spAppData = new AppData_S;
	      spAppData->usNumStreams = uiNumOutStreams;
	      spAppData->usNumUnreliable = uiNumUnrelStreams;
	      spAppData->usStreamId = 0;  
	      spAppData->usReliability = uiReliability;
	      spAppData->eUnordered = eUnordered;
	      spAppData->uiNumBytes = MIN(iMsgSize, (int) uiMaxFragSize);
	      spNewNode->vpData = spAppData;
	      InsertNode(&sAppLayerBuffer, sAppLayerBuffer.spTail, 
			 spNewNode, NULL);
	    }
	}      

      if(uiNumOutStreams > MAX_NUM_STREAMS)
	{
	  fprintf(stderr, "%s number of streams (%d) > max (%d)\n",
		  "SCTP ERROR:",
		  uiNumOutStreams, MAX_NUM_STREAMS);
	  DBG_PL(sendmsg, "ERROR: number of streams (%d) > max (%d)"), 
	    uiNumOutStreams, MAX_NUM_STREAMS DBG_PR;
	  DBG_PL(sendmsg, "exiting...") DBG_PR;
	  exit(-1);
	}
      else if(uiNumUnrelStreams > uiNumOutStreams)
	{
	  fprintf(stderr,"%s number of unreliable streams (%d) > total (%d)\n",
		  "SCTP ERROR:",
		  uiNumUnrelStreams, uiNumOutStreams);
	  DBG_PL(sendmsg, 
		 "ERROR: number of unreliable streams (%d) > total (%d)"), 
	    uiNumUnrelStreams, uiNumOutStreams DBG_PR;
	  DBG_PL(sendmsg, "exiting...") DBG_PR;
	  exit(-1);
	}

      if(spAppData->uiNumBytes + sizeof(SctpDataChunkHdr_S) 
	 > MAX_DATA_CHUNK_SIZE)
	{
	  fprintf(stderr, "SCTP ERROR: message size (%d) too big\n",
		  spAppData->uiNumBytes);
	  fprintf(stderr, "%s data chunk size (%lu) > max (%d)\n",
		  "SCTP ERROR:",
		  spAppData->uiNumBytes + 
		    (unsigned long) sizeof(SctpDataChunkHdr_S), 
		  MAX_DATA_CHUNK_SIZE);
	  DBG_PL(sendmsg, "ERROR: message size (%d) too big"),
		 spAppData->uiNumBytes DBG_PR;
	  DBG_PL(sendmsg, "ERROR: data chunk size (%lu) > max (%d)"), 
	    spAppData->uiNumBytes + (unsigned long) sizeof(SctpDataChunkHdr_S), 
	    MAX_DATA_CHUNK_SIZE 
	    DBG_PR;
	  DBG_PL(sendmsg, "exiting...") DBG_PR;
	  exit(-1);
	}
      else if(spAppData->uiNumBytes + sizeof(SctpDataChunkHdr_S)
	      > uiMaxDataSize)
	{
	  fprintf(stderr, "SCTP ERROR: message size (%d) too big\n",
		  spAppData->uiNumBytes);
	  fprintf(stderr, 
		  "%s data chunk size (%lu) + SCTP/IP header(%d) > MTU (%d)\n",
		  "SCTP ERROR:",
		  spAppData->uiNumBytes + 
		    (unsigned long) sizeof(SctpDataChunkHdr_S),
		  SCTP_HDR_SIZE + uiIpHeaderSize, uiMtu);
	  fprintf(stderr, "           %s\n",
		  "...chunk fragmentation is not yet supported!");
	  DBG_PL(sendmsg, "ERROR: message size (%d) too big"),
		 spAppData->uiNumBytes DBG_PR;
	  DBG_PL(sendmsg, "exiting...") DBG_PR;
	  exit(-1);
	}
    }

  switch(eState)
    {
    case SCTP_STATE_CLOSED:
      DBG_PL(sendmsg, "sending INIT") DBG_PR;

      /* This must be done especially since some of the legacy apps use their
       * own packet type (don't ask me why). We need our packet type to be
       * sctp so that our tracing output comes out correctly for scripts, etc
       */
      set_pkttype(PT_SCTP); 
      iOutDataSize = GenChunk(SCTP_CHUNK_INIT, ucpOutData);
      opT1InitTimer->resched(spPrimaryDest->dRto);
      eState = SCTP_STATE_COOKIE_WAIT;
      SendPacket(ucpOutData, iOutDataSize, spPrimaryDest);
      break;
      
    case SCTP_STATE_ESTABLISHED:
      if(eDataSource == DATA_SOURCE_APPLICATION) 
	{ 
	  /* NE: 10/12/2007 Check if all the destinations are confirmed (new
	     data can be sent) or there are any marked chunks. */
	  if(eSendNewDataChunks == TRUE && eMarkedChunksPending == FALSE) 
	    {
	      SendMuch();
	      eSendNewDataChunks = FALSE;
	    }
	}
      else if(eDataSource == DATA_SOURCE_INFINITE)
	{
	  fprintf(stderr, "[sendmsg] ERROR: unexpected state... %s\n",
		  "sendmsg called more than once for infinite data");
	  DBG_PL(sendmsg, 
		 "ERROR: unexpected state... %s"), 
	    "sendmsg called more than once for infinite data" DBG_PR;
	  DBG_PL(sendmsg, "exiting...") DBG_PR;
	  exit(-1);
	}
      break;
      
    default:  
      /* If we are here, we assume the application is trying to send data
       * before the 4-way handshake has completed. ...so buffering the
       * data is ok, but DON'T send it yet!!  
       */
      break;
    }

  DBG_X(sendmsg);
  delete [] ucpOutData;
}

void SctpAgent::T1InitTimerExpiration()
{
  DBG_I(T1InitTimerExpiration);

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  int iOutDataSize = 0;

  iInitTryCount++;
  if(iInitTryCount > (int) uiMaxInitRetransmits)
    {
      Close();
    }
  else 
    {
      spPrimaryDest->dRto *= 2;
      if(spPrimaryDest->dRto > dMaxRto)
	spPrimaryDest->dRto = dMaxRto;
      tdRto++;              // trigger changes for trace to pick up

      iOutDataSize = GenChunk(SCTP_CHUNK_INIT, ucpOutData);
      SendPacket(ucpOutData, iOutDataSize, spPrimaryDest);
      opT1InitTimer->resched(spPrimaryDest->dRto);
    }

  DBG_X(T1InitTimerExpiration);
  delete [] ucpOutData;
}

void T1InitTimer::expire(Event*)
{
  opAgent->T1InitTimerExpiration();
}

void SctpAgent::T1CookieTimerExpiration()
{
  DBG_I(T1CookieTimerExpiration);

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  int iOutDataSize = 0;
 
  iInitTryCount++;
  if(iInitTryCount > (int) uiMaxInitRetransmits)
    Close();
  else 
    {
      spPrimaryDest->dRto *= 2;
      if(spPrimaryDest->dRto > dMaxRto)
	spPrimaryDest->dRto = dMaxRto;
      tdRto++;              // trigger changes for trace to pick up

      iOutDataSize = GenChunk(SCTP_CHUNK_COOKIE_ECHO, ucpOutData);
      SendPacket(ucpOutData, iOutDataSize, spPrimaryDest);
      opT1CookieTimer->resched(spPrimaryDest->dRto);
  }

  DBG_X(T1CookieTimerExpiration);
  delete [] ucpOutData;
}

void T1CookieTimer::expire(Event*)
{
  opAgent->T1CookieTimerExpiration();
}

void SctpAgent::Close()
{
  DBG_I(Close);

  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrDest = NULL;

  eState = SCTP_STATE_CLOSED; 
  
  /* stop all timers
   */
  opT1InitTimer->force_cancel();
  opT1CookieTimer->force_cancel();
  opSackGenTimer->force_cancel();
  if(uiHeartbeatInterval != 0)
    {
      opHeartbeatGenTimer->force_cancel();
    }

  for(spCurrNode = sDestList.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrDest = (SctpDest_S *) spCurrNode->vpData;
      spCurrDest->opT3RtxTimer->force_cancel();
      spCurrDest->opCwndDegradeTimer->force_cancel();

      spCurrDest->opHeartbeatTimeoutTimer->force_cancel();
    }

  ClearList(&sSendBuffer);
  ClearList(&sAppLayerBuffer);
  ClearList(&sRecvTsnBlockList);
  ClearList(&sDupTsnList);

  DBG_X(Close);
}

/* Handles timeouts for both DATA chunks and HEARTBEAT chunks. The actions are
 * slightly different for each. Covers rfc sections 6.3.3 and 8.3.
 */
void SctpAgent::Timeout(SctpChunkType_E eChunkType, SctpDest_S *spDest)
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
      
      /* Lukasz Budzisz : 03/09/2006 
       * Section 7.2.3 of rfc4960
       * NE: 4/29/2007 - This conditioal is used for some reason but for 
       * now we dont know why. 
       */
      if(spDest->iCwnd > 1 * (int) uiMaxDataSize)
	{
	  spDest->iSsthresh 
	    = MAX(spDest->iCwnd/2, 4 * (int) uiMaxDataSize);
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
      if(spDest->eStatus == SCTP_DEST_STATUS_INACTIVE && 
	 uiHeartbeatInterval!=0)
	SendHeartbeat(spDest);  // just marked inactive, so send HB immediately
    }
  else if(eChunkType == SCTP_CHUNK_HB)
    {
      if( (uiHeartbeatInterval != 0) ||
	  (spDest->eStatus == SCTP_DEST_STATUS_UNCONFIRMED))
	
	SendHeartbeat(spDest);
    }

  DBG_X(Timeout);  
}

void T3RtxTimer::expire(Event*)
{
  opAgent->Timeout(SCTP_CHUNK_DATA, spDest);
}

void HeartbeatTimeoutTimer::expire(Event*)
{
  /* Track HB-Timer for CMT-PF 
   */
  spDest->eHBTimerIsRunning = FALSE;
  opAgent->Timeout(SCTP_CHUNK_HB, spDest);
}

void SctpAgent::CwndDegradeTimerExpiration(SctpDest_S *spDest)
{
  DBG_I(CwndDegradeTimerExpiration);
  DBG_PL(CwndDegradeTimerExpiration, "spDest=%p"), spDest DBG_PR;
  if(spDest->iCwnd > iInitialCwnd * (int) uiMaxDataSize)
    {
      /* Lukasz Budzisz : 03/09/2006
       * Section : 7.2.1 and 7.2.2
       */
      spDest->iCwnd = MAX(spDest->iCwnd/2, 4 * (int) uiMaxDataSize);
      tiCwnd++; // trigger changes for trace to pick up
    }
  spDest->opCwndDegradeTimer->resched(spDest->dRto);
  DBG_X(CwndDegradeTimerExpiration);  
}

void CwndDegradeTimer::expire(Event*)
{
  opAgent->CwndDegradeTimerExpiration(spDest);
}

void SctpAgent::SendHeartbeat(SctpDest_S *spDest)
{
  DBG_I(SendHeartbeat);
  DBG_PL(SendHeartbeat, "spDest=%p"), spDest DBG_PR;

  SctpHeartbeatChunk_S sHeartbeatChunk;
  double dCurrTime = Scheduler::instance().clock();

  GenChunk(SCTP_CHUNK_HB, (u_char *) &sHeartbeatChunk); // doesn't fill dest
  sHeartbeatChunk.spDest = spDest;          // ...so we fill it here :-)
  SendPacket((u_char *) &sHeartbeatChunk, SCTP_CHUNK_HEARTBEAT_LENGTH, spDest);

  spDest->dIdleSince = dCurrTime;
  
  spDest->opHeartbeatTimeoutTimer->resched(spDest->dRto);
  
  /* Track HB-Timer for CMT-PF 
   */
  spDest->eHBTimerIsRunning = TRUE;
  DBG_PL(SendHeartbeat, "HEARTBEAT times out at %f"), 
    spDest->dRto+dCurrTime DBG_PR;

  DBG_X(SendHeartbeat);
}

void SctpAgent::HeartbeatGenTimerExpiration(double dTimerStartTime)
{
  DBG_I(HeartbeatGenTimerExpiration);

  Node_S *spCurrNode = NULL;
  SctpDest_S *spCurrNodeData = NULL;
  Node_S *spLongestIdleNode = NULL;
  SctpDest_S *spLongestIdleNodeData = NULL;
  double dCurrTime = Scheduler::instance().clock();
  double dTime;

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
  if(spLongestIdleNodeData->dIdleSince <= dTimerStartTime)
    SendHeartbeat(spLongestIdleNodeData);
  else
    DBG_PL(HeartbeatGenTimerExpiration, 
	       "longest idle dest not idle long enough!") DBG_PR;
  
  /* start the timer again...
   */
  dTime = CalcHeartbeatTime(spLongestIdleNodeData->dRto);
  opHeartbeatGenTimer->resched(dTime);
  opHeartbeatGenTimer->dStartTime = dCurrTime;
    
  DBG_X(HeartbeatGenTimerExpiration);
}

void HeartbeatGenTimer::expire(Event*)
{
  opAgent->HeartbeatGenTimerExpiration(dStartTime);
}

void SctpAgent::SackGenTimerExpiration() // section 6.2
{
  DBG_I(SackGenTimerExpiration);

  u_char *ucpOutData = new u_char[uiMaxPayloadSize];
  int iOutDataSize = 0;
  memset(ucpOutData, 0, uiMaxPayloadSize);

  iDataPktCountSinceLastSack = 0; // reset 

  iOutDataSize = BundleControlChunks(ucpOutData);

  /* PN: 6/17/2007
   * Send NR-Sacks instead of Sacks?
   */
  if (eUseNonRenegSacks == TRUE) 
     iOutDataSize += GenChunk(SCTP_CHUNK_NRSACK, ucpOutData+iOutDataSize);
  else
     iOutDataSize += GenChunk(SCTP_CHUNK_SACK, ucpOutData+iOutDataSize);

  SendPacket(ucpOutData, iOutDataSize, spReplyDest); 

  if (eUseNonRenegSacks == TRUE) {
    DBG_PL(SackGenTimerExpiration, "NR-SACK sent (%d bytes)"), 
      iOutDataSize DBG_PR;
  }
  else {
    DBG_PL(SackGenTimerExpiration, "SACK sent (%d bytes)"), 
      iOutDataSize DBG_PR;
  }

  DBG_X(SackGenTimerExpiration);
  delete [] ucpOutData;
}

void SackGenTimer::expire(Event*)
{
  opAgent->SackGenTimerExpiration();
}

void SctpAgent::RouteCacheFlushTimerExpiration(SctpDest_S *spDest)
{
  DBG_I(RouteCacheFlushTimerExpiration);
  DBG_PL(RouteCacheFlushTimerExpiration, "spDest=%p"), spDest DBG_PR;
  double dCurrTime = Scheduler::instance().clock();
  DBG_PL(RouteCacheFlushTimerExpiration, "dCurrTime=%f"), dCurrTime DBG_PR;

  spDest->eRouteCached = FALSE;

  DBG_X(RouteCacheFlushTimerExpiration);
}

void RouteCacheFlushTimer::expire(Event*)
{
  opAgent->RouteCacheFlushTimerExpiration(spDest);
}

void SctpAgent::RouteCalcDelayTimerExpiration(SctpDest_S *spDest)
{
  DBG_I(RouteCalcDelayTimerExpiration);
  DBG_PL(RouteCalcDelayTimerExpiration, "spDest=%p"), spDest DBG_PR;
  double dCurrTime = Scheduler::instance().clock();
  DBG_PL(RouteCalcDelayTimerExpiration, "dCurrTime=%f"), dCurrTime DBG_PR;

  Node_S *spCurrNode = spDest->sBufferedPackets.spHead;
  Node_S *spDeleteNode = NULL;

  spDest->iRcdCount++;
  tiRcdCount++;
  spDest->eRouteCached = TRUE;
  SetSource(spDest); // set src addr, port, target based on "routing table"
  SetDestination(spDest); // set dest addr & port 

  /* Dequeue and send all queued packets
   */
  while(spCurrNode != NULL)
    {
      send( (Packet *) spCurrNode->vpData, 0);
      spDeleteNode = spCurrNode;
      spCurrNode = spCurrNode->spNext;
      DeleteNode(&spDest->sBufferedPackets, spDeleteNode);
    }

  DBG_X(RouteCalcDelayTimerExpiration);
}

void RouteCalcDelayTimer::expire(Event*)
{
  opAgent->RouteCalcDelayTimerExpiration(spDest);
}

/****************************************************************************
 * debugging functions
 ****************************************************************************/

void SctpAgent::DumpSendBuffer()
{
  DBG_IF(DumpSendBuffer)
    {
      DBG_I(DumpSendBuffer);

      Node_S *spCurrNode = NULL;
      SctpSendBufferNode_S *spCurrNodeData = NULL;
      SctpDataChunkHdr_S  *spChunk = NULL;

      for(spCurrNode = sSendBuffer.spHead;
	  spCurrNode != NULL;
	  spCurrNode = spCurrNode->spNext)
	{
	  spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;
	  spChunk = spCurrNodeData->spChunk;
	  DBG_PL(DumpSendBuffer, 
		 "tsn=%d spDest=%p eMarkedForRtx=%s %s %s"),
	    spChunk->uiTsn, 
	    spCurrNodeData->spDest,
	    (!spCurrNodeData->eMarkedForRtx ? "NO_RTX" 
	     : (spCurrNodeData->eMarkedForRtx==FAST_RTX ? "FAST_RTX"
		: "TIMEOUT_RTX")),
	    spCurrNodeData->eMarkedForRtx ? "MarkedForRtx" : "",
	    spCurrNodeData->eGapAcked ? "GapAcked" : "",
	    spCurrNodeData->eAdvancedAcked ? "AdvancedAcked" : ""
	    DBG_PR;
	  DBG_PL(DumpSendBuffer, 
		 "       spCurrNodeData->spDest->iOutstandingBytes=%lu"),
	    spCurrNodeData->spDest->iOutstandingBytes DBG_PR;
	}

      DBG_X(DumpSendBuffer);
    }
}

/* PN: 12/21/2007. 
 * This method goes through the send buffer and counts
 * the number of bytes that have not been gap acked.
 * While using NR-SACKs, all tsns with eGapAcked=FALSE, 
 * are all tsns that are actually in flight (in the network).
 * Since ns-2 does not reneg, in case of unordered transfer, all TSNs 
 * with eGapAcked=TRUE are 
 * delivered to app. With multistreamed and ordered transfer, this is not true.
 */
/* Need help from Nasif to change the variable names appropriately
 * TODO
 */
int SctpAgent::CalculateBytesInFlight() 
{
  
  Node_S *spCurrNode = NULL;
  SctpSendBufferNode_S *spCurrNodeData = NULL;
  int usChunkSize = 0;
  int totalOutstanding = 0;

  for(spCurrNode = sSendBuffer.spHead;
      spCurrNode != NULL;
      spCurrNode = spCurrNode->spNext)
    {
      spCurrNodeData = (SctpSendBufferNode_S *) spCurrNode->vpData;
      
      if(spCurrNodeData->eGapAcked == FALSE)
	{
	  usChunkSize = spCurrNodeData->spChunk->sHdr.usLength;
	  totalOutstanding += usChunkSize;
	}
    }
  
  return totalOutstanding;
}
