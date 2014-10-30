/*
 * Copyright (c) 2006-2009 by the Protocol Engineering Lab, U of Delaware
 * All rights reserved.
 *
 * Protocol Engineering Lab web page : http://pel.cis.udel.edu/
 *
 * Paul D. Amer        <amer@@cis,udel,edu>
 * Armando L. Caro Jr. <acaro@@cis,udel,edu>
 *
 * Armando's debugging output system. 
 * Based on a previous version written by Phill Conrad and Ed Golden.
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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctpDebug.h,v 1.6 2009/11/16 05:51:27 tom_henderson Exp $ (UD/PEL)
 */

#ifndef ns_sctpDebug_h
#define ns_sctpDebug_h

void sctpDebugEmptyPrintf (char const *format, ...);

#if(DEBUG)

/* debug variable format:  __dbg_FunctionName
 */
#define __dbg_command                          0x00000001
#define __dbg_Reset                            0x00000001
#define __dbg_OptionReset                      0x00000001
#define __dbg_ControlChunkReservation          0x00000001

#define __dbg_recv                             0x00000002

#define __dbg_sendmsg                          0x00000004

#define __dbg_GenChunk                         0x00000008

#define __dbg_GetNextDataChunkSize             0x00000010
#define __dbg_GenOneDataChunk                  0x00000010
#define __dbg_GenMultipleDataChunks            0x00000010
#define __dbg_BundleControlChunks              0x00000010

#define __dbg_GetHighestOutstandingTsn         0x00000020
#define __dbg_FastRtx                          0x00000020
#define __dbg_StartT3RtxTimer                  0x00000020
#define __dbg_StopT3RtxTimer                   0x00000020
#define __dbg_Timeout                          0x00000020
#define __dbg_TimeoutRtx                       0x00000020
#define __dbg_T1InitTimerExpiration            0x00000020
#define __dbg_T1CookieTimerExpiration          0x00000020
#define __dbg_SackGenTimerExpiration           0x00000020
#define __dbg_MarkChunkForRtx                  0x00000020
#define __dbg_AnyMarkedChunks                  0x00000020
#define __dbg_RtxMarkedChunks                  0x00000020

#define __dbg_AddToSendBuffer                  0x00000040

#define __dbg_RttUpdate                        0x00000080

#define __dbg_SendBufferDequeueUpTo            0x00000100

#define __dbg_AdjustCwnd                       0x00000200

#define __dbg_UpdateHighestTsn                 0x00000400
#define __dbg_IsDuplicateChunk                 0x00000400
#define __dbg_InsertDuplicateTsn               0x00000400

#define __dbg_UpdateCumAck                     0x00000800
#define __dbg_UpdateRecvTsnBlocks              0x00000800

#define __dbg_AdvancePeerAckPoint              0x00001000
#define __dbg_ProcessForwardTsnChunk           0x00001000

#define __dbg_ProcessInitChunk                 0x00002000
#define __dbg_ProcessInitAckChunk              0x00002000

#define __dbg_ProcessDataChunk                 0x00004000
#define __dbg_ProcessGapAckBlocks              0x00004000
#define __dbg_ProcessSackChunk                 0x00004000

/* PN: 5/2007. NR-Sacks */
#define __dbg_ProcessNonRenegSackBlocks        0x00004000
#define __dbg_ProcessNonRenegSackChunk         0x00004000
#define __dbg_UpdateHighestTsnSent	       0x00004000

#define __dbg_ProcessChunk                     0x00008000
#define __dbg_ProcessOptionChunk               0x00008000

#define __dbg_NextChunk                        0x00010000

#define __dbg_SetSource                        0x00020000
#define __dbg_SetDestination                   0x00020000
#define __dbg_SendPacket                       0x00020000
#define __dbg_TotalOutstanding                 0x00020000
#define __dbg_SendMuch                         0x00020000

#define __dbg_PassToUpperLayer                 0x00040000

#define __dbg_InsertInStreamBuffer             0x00080000
#define __dbg_PassToStream                     0x00080000
#define __dbg_UpdateAllStreams                 0x00080000

#define __dbg_AddInterface                     0x00100000
#define __dbg_AddDestination                   0x00100000
#define __dbg_SetPrimary                       0x00100000
#define __dbg_ForceSource                      0x00100000

#define __dbg_GetNextDest                      0x00200000

#define __dbg_CwndDegradeTimerExpiration       0x00400000
#define __dbg_CalcHeartbeatTime                0x00400000
#define __dbg_SendHeartbeat                    0x00400000
#define __dbg_HeartbeatGenTimerExpiration      0x00400000
#define __dbg_ProcessHeartbeatAckChunk         0x00400000

#define __dbg_Close                            0x00800000

/* functions which process optional chunks
 */
#define __dbg_ProcessTimestampChunk            0x01000000

/* optional functions for simulated reactive routing protocols (for MANETs, etc)
 */
#define __dbg_RouteCacheFlushTimerExpiration   0x02000000
#define __dbg_RouteCalcDelayTimerExpiration    0x02000000

/* CMT debugging
 */
#define __dbg_SelectRtxDest                    0x04000000

/* verbose debugging functions
 */
#define __dbg_DumpSendBuffer                   0x80000000

/* CMT-PF debugging
 */
#define __dbg_SelectFromPFDests                0x10000000
 

/**************************** DEBUGGING MACROS **************************/

/* For Function Entry 
 *
 * example usage:   DBG_I(SomeFunctionName);
 */
#define DBG_I(funcName) \
        if(fhpDebugFile != NULL) /* make sure output file is open! */ \
          if (uiDebugMask & __dbg_##funcName) \
	    {fprintf(fhpDebugFile, "[%s] <I> \n", #funcName);}

/* For Printing Details
 *
 * example usage:
 *          DBG_PL(SomeFunctionName, "testing") DBG_PR;
 *          DBG_PL(SomeFunctionName, "current someValue=%d"), someValue DBG_PR;
 */
#define DBG_PL(funcName, format) \
        if(fhpDebugFile != NULL) /* make sure output file is open! */ \
          if (uiDebugMask & __dbg_##funcName) \
	    fprintf(fhpDebugFile, "[%s] " format "\n", #funcName
#define DBG_PR )

/* For Calling Debugging Functions
 * 
 * example usage:
 *          DBG_F(SomeFunctionName, SomeDebugFunction(param1, parm2, ...) );
 *
 * Note: both SomeFunctionName and SomeDebugFunction debug bits must be set
 */
#define DBG_F(funcName, debugFuncCall) \
        if(fhpDebugFile != NULL) /* make sure output file is open! */ \
          if (uiDebugMask & __dbg_##funcName) \
	    debugFuncCall

/* For executing blocks of code only if debug bit is set
 *
 * example usage:
 *          DBG_IF(SomeFunctionName)
 *            {
 *              // block of code goes here
 *            }
 */
#define DBG_IF(funcName) \
        if(fhpDebugFile != NULL) /* make sure output file is open! */ \
          if (uiDebugMask & __dbg_##funcName)

/* For Function Exit
 *
 * example usage:    DBG_X(SomeFunctionName);
 */
#define DBG_X(funcName) \
        if(fhpDebugFile != NULL) /* make sure output file is open! */ \
          if (uiDebugMask & __dbg_##funcName) \
	    {fprintf(fhpDebugFile, "[%s] <X> \n", #funcName);}

#define DBG_FOPEN() \
        if(fhpDebugFile == NULL) /* only open file once! */ \
          { \
	     if(iDebugFileIndex >= 0) \
	       { \
		 char cpFileName[256]; \
		 sprintf(cpFileName, "debug.SctpAgent.%d", iDebugFileIndex); \
		 fhpDebugFile = fopen(cpFileName, "w"); \
	       } \
	     else \
	       fhpDebugFile = stdout; \
	  }

#else  /* DEBUG is not defined, then preprocess these macros out! */

#define DBG_I(bitmask)            { /* empty */ }

/* This is a HACK!!! ...but who cares, it's only debugging and gcc should strip
 * it out anyway.
 */
#define DBG_PL(funcName, format)  if(0) sctpDebugEmptyPrintf (""
#define DBG_PR                    )

#define DBG_F(funcName, debugFuncCall)  { /* empty */ }

#define DBG_IF(funcName)          if(0)

#define DBG_X(funcName)           { /* empty */ }

#define DBG_FOPEN()               { /* empty */ }

#endif
#endif
