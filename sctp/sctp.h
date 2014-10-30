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
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp.h,v 1.11 2009/11/16 05:51:27 tom_henderson Exp $ (UD/PEL)
 */

#ifndef ns_sctp_h
#define ns_sctp_h

#include "agent.h"
#include "node.h"
#include "packet.h"

/* The SCTP Common header is 12 bytes.
 */
#define SCTP_HDR_SIZE         12

#define MAX_RWND_SIZE         0xffffffff
#define MAX_DATA_CHUNK_SIZE   0xffffffff
#define MIN_DATA_CHUNK_SIZE   16
#define MAX_NUM_STREAMS       0x0000ffff

#define DELAYED_SACK_TRIGGER  2      // sack for every 2 data packets

#define RTO_ALPHA             0.125  // RTO.alpha is 1/8
#define RTO_BETA              0.25   // RTO.Beta is 1/4

#define MAX_BURST             4
enum MaxBurstUsage_E
{
  MAX_BURST_USAGE_OFF,      // 0
  MAX_BURST_USAGE_ON        // 1
};

/* Let us use OUR typedef'd enum for FALSE and TRUE. It's much better this way.
 * ...why? because NOW all boolean variables can be explicitly declared as
 * such. There is no longer any ambiguity between a regular int variable and a
 * boolean variable.  
 */
#undef FALSE
#undef TRUE
enum Boolean_E
{
  FALSE,
  TRUE
};

/* Each time the sender retransmits marked chunks, how many can be sent? Well,
 * immediately after a timeout or when the 4 missing report is received, only
 * one packet of retransmits may be sent. Later, the number of packets is gated
 * by cwnd
 */
enum SctpRtxLimit_E
{
  RTX_LIMIT_ONE_PACKET,
  RTX_LIMIT_CWND,
  RTX_LIMIT_ZERO
};

enum MarkedForRtx_E
{
  NO_RTX,
  FAST_RTX,
  TIMEOUT_RTX
};

enum RtxToAlt_E
{
  RTX_TO_ALT_OFF,
  RTX_TO_ALT_ON,
  RTX_TO_ALT_TIMEOUTS_ONLY
};

/* What behavior is used during dormant state (ie, all destinations have
 * failed) when timeouts persist?
 */
enum DormantAction_E
{
  DORMANT_HOP,        // keep hopping to another destination
  DORMANT_PRIMARY,    // goto primary and stay there
  DORMANT_LASTDEST    // stay at the last destination used before dormant state
};

/* Who controls the data sending, app layer or the transport layer 
 * (as in the case of infinite data)
 */
enum DataSource_E
{
  DATA_SOURCE_APPLICATION,
  DATA_SOURCE_INFINITE
};

/* SCTP chunk types 
 */
enum SctpChunkType_E
{
  SCTP_CHUNK_DATA,
  SCTP_CHUNK_INIT,
  SCTP_CHUNK_INIT_ACK,
  SCTP_CHUNK_SACK,
  SCTP_CHUNK_HB,
  SCTP_CHUNK_HB_ACK,
  SCTP_CHUNK_ABORT,
  SCTP_CHUNK_SHUTDOWN,
  SCTP_CHUNK_SHUTDOWN_ACK,
  SCTP_CHUNK_ERROR,
  SCTP_CHUNK_COOKIE_ECHO,
  SCTP_CHUNK_COOKIE_ACK,
  SCTP_CHUNK_ECNE,                  // reserved in rfc2960
  SCTP_CHUNK_CWR,                   // reserved in rfc2960
  SCTP_CHUNK_SHUTDOWN_COMPLETE,

  /* RFC2960 leaves room for later defined chunks */

  /* for U-SCTP/PR-SCTP
   */
  SCTP_CHUNK_FORWARD_TSN,    // should be 192, but this is a simulation! :-)

  /* for timestamp option (sctp-timestamp.cc)
   */
  SCTP_CHUNK_TIMESTAMP,

  /* PN: 6/17/2007. NR-Sack chunk type */
  SCTP_CHUNK_NRSACK
};

struct AppData_S 
{
  /* Parameters needed for establishing an association 
   */
  u_short usNumStreams;     // Number of streams to associate
  u_short usNumUnreliable;  // first usNumUnreliable streams will be unreliable

  /* Parameters needed for app data messages 
   */
  u_short    usStreamId;     // Which stream does this message go on?
  u_short    usReliability;  // What level of relability does this message have
  Boolean_E  eUnordered;     // Is this an unordered message?
  u_int      uiNumBytes;     // Number of databytes in message
};

/* ns specific header fields used for tracing SCTP traffic
 * (This was done so that the 'trace' module wouldn't have to look into the
 * payload of SCTP packets)
 */
struct SctpTrace_S
{
  SctpChunkType_E  eType;
  u_int            uiTsn;   // (cum ack for sacks, -1 for other control chunks)
  u_short          usStreamId;     // -1 for control chunks
  u_short          usStreamSeqNum; // -1 for control chunks
};

struct hdr_sctp
{
  /* ns required header fields/methods 
   */
  static int offset_;	// offset for this header
  inline static int& offset() { return offset_; }
  inline static hdr_sctp* access(Packet* p) 
  {
    return (hdr_sctp*) p->access(offset_);
  }

  /* ns specific header fields used for tracing SCTP traffic
   * (This was done so that the 'trace' module wouldn't have to look into the
   * payload of SCTP packets)
   */
  u_int         uiNumChunks;
  SctpTrace_S  *spSctpTrace;

  u_int&        NumChunks() { return uiNumChunks; }
  SctpTrace_S*& SctpTrace() { return spSctpTrace; }
};

struct SctpChunkHdr_S
{
  u_char  ucType;
  u_char  ucFlags;
  u_short usLength;
};

/* INIT paramater types
 */
#define SCTP_INIT_PARAM_UNREL  0xC000
struct SctpUnrelStreamsParam_S
{
  u_short  usType;
  u_short  usLength;

  /* unreliable stream start-end pairs are appended dynamically
   */
};

struct SctpUnrelStreamPair_S
{
  u_short  usStart;
  u_short  usEnd;
};

struct SctpInitChunk_S  // this is used for init ack, too 
{
  SctpChunkHdr_S  sHdr;
  u_int           uiInitTag;		 // tag of mine (not used)
  u_int           uiArwnd; 	         // referred to as a_rwnd in rfc2960
  u_short         usNumOutboundStreams;	 // referred to as OS in rfc2960
  u_short         usMaxInboundStreams;   // referred to as MIS in rfc2960
  u_int           uiInitialTsn;          

  SctpUnrelStreamsParam_S  sUnrelStream;	
};
typedef SctpInitChunk_S SctpInitAckChunk_S;

struct SctpCookieEchoChunk_S
{
  SctpChunkHdr_S  sHdr;
	
  /* cookie would go here, but we aren't implementing this at the moment */
};
typedef SctpCookieEchoChunk_S SctpCookieAckChunk_S;

struct SctpDataChunkHdr_S
{
  SctpChunkHdr_S  sHdr;
  u_int           uiTsn;
  u_short         usStreamId;
  u_short         usStreamSeqNum;
  u_int           uiPayloadType;     // not used

  /* user data must be appended dynamically when filling packets */
};

/* Data Chunk Specific Flags
 */
#define SCTP_DATA_FLAG_END        0x01  // indicates last fragment
#define SCTP_DATA_FLAG_BEGINNING  0x02  // indicates first fragment
#define SCTP_DATA_FLAG_UNORDERED  0x04  // indicates unordered DATA chunk

/* SACK has the following structure and following it will be some number of
 * gap acks and duplicate tsns.
 */
struct SctpSackChunk_S
{
  SctpChunkHdr_S  sHdr;
  u_int           uiCumAck;
  u_int           uiArwnd;
  u_short         usNumGapAckBlocks;
  u_short         usNumDupTsns;

  /* Gap Ack Blocks and Duplicate TSNs are appended dynamically
   */
};

/* PN: 5/2007. NR-Sack Chunk structure */
struct SctpNonRenegSackChunk_S
{
  SctpChunkHdr_S  sHdr;
  u_int           uiCumAck;
  u_int           uiArwnd;
  u_short         usNumGapAckBlocks;

  /* PN: 5/2007. NR-Sack information */
  u_short	  usNumNonRenegSackBlocks;

  u_short         usNumDupTsns;


  /* Gap Ack Blocks, NR Gap Ack Blocks and Duplicate TSNs are appended
   * dynamically 
   */

};

struct SctpGapAckBlock_S
{
  u_short  usStartOffset;
  u_short  usEndOffset;
};

struct SctpDupTsn_S
{
  u_int  uiTsn;
};

/* PN: 5/2007. NR-Sacks */
/* PN TODO: Use SctpTsn_S and NODE_TYPE_TSN instead of SctpDupTsn_S 
 * and NODE_TYPE_DUP_TSN everywhere ?
 */
struct SctpTsn_S
{
  u_int  uiTsn;
};

#define SCTP_CHUNK_FORWARD_TSN_LENGTH  8
struct SctpForwardTsnChunk_S
{
  SctpChunkHdr_S  sHdr;
  u_int           uiNewCum;
};

struct SctpDest_S;
#define SCTP_CHUNK_HEARTBEAT_LENGTH  24
struct SctpHeartbeatChunk_S
{
  SctpChunkHdr_S  sHdr;
  u_short         usInfoType;      // filled in, but not really used
  u_short         usInfoLength;    // filled in, but not really used
  double          dTimestamp;
  SctpDest_S     *spDest;
};
typedef SctpHeartbeatChunk_S SctpHeartbeatAckChunk_S;

/* SCTP state defines for internal state machine */
enum SctpState_E
{
  SCTP_STATE_UNINITIALIZED,
  SCTP_STATE_CLOSED,
  SCTP_STATE_ESTABLISHED,
  SCTP_STATE_COOKIE_WAIT,
  SCTP_STATE_COOKIE_ECHOED,
  SCTP_STATE_SHUTDOWN_SENT,        // not currently used
  SCTP_STATE_SHUTDOWN_RECEIVED,    // not currently used
  SCTP_STATE_SHUTDOWN_ACK_SENT,    // not currently used
  SCTP_STATE_SHUTDOWN_PENDING      // not currently used
};

class SctpAgent;

class T1InitTimer : public TimerHandler 
{
public:
  T1InitTimer(SctpAgent *a) : TimerHandler(), opAgent(a) { }
	
protected:
  virtual void expire(Event *);
  SctpAgent *opAgent;
};

class T1CookieTimer : public TimerHandler 
{
public:
  T1CookieTimer(SctpAgent *a) : TimerHandler(), opAgent(a) { }
	
protected:
  virtual void expire(Event *);
  SctpAgent *opAgent;
};

class T3RtxTimer : public TimerHandler 
{
public:
  T3RtxTimer(SctpAgent *a, SctpDest_S *d) 
    : TimerHandler(), opAgent(a) {spDest = d;}
	
protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
  SctpDest_S *spDest;  // destination this timer corresponds to
};

class CwndDegradeTimer : public TimerHandler 
{
public:
  CwndDegradeTimer(SctpAgent *a, SctpDest_S *d) 
    : TimerHandler(), opAgent(a) {spDest = d;}
	
protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
  SctpDest_S *spDest;  // destination this timer corresponds to
};

class HeartbeatGenTimer : public TimerHandler 
{
public:
  HeartbeatGenTimer(SctpAgent *a) 
    : TimerHandler(), opAgent(a) {}

  double      dStartTime; // timestamp of when timer started
	
protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
};

class HeartbeatTimeoutTimer : public TimerHandler 
{
public:
  HeartbeatTimeoutTimer(SctpAgent *a, SctpDest_S *d) 
    : TimerHandler(), opAgent(a) {spDest = d;}
	
  SctpDest_S *spDest;  // destination this timer corresponds to

protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
};

/* This timer simulates the route lifetime in the routing tables of
 * reactive routing protocols for MANETs, etc. When this timer expires,
 * the route is flushed and any future data sent to this dest will cause a 
 * route calculation.
 *
 * Note: This timer is not normally used. It's only for our simulated reactive
 * routing overheads for MANETs, etc. 
 */
class RouteCacheFlushTimer : public TimerHandler 
{
public:
  RouteCacheFlushTimer(SctpAgent *a, SctpDest_S *d) 
    : TimerHandler(), opAgent(a) {spDest = d;}
	
protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
  SctpDest_S *spDest;  // destination this timer corresponds to
};

/* This timer simulates the time it takes to calculate a route in
 * reactive routing protocols for MANETs, etc. 
 *
 * Note: This timer is not normally used. It's only for our simulated reactive
 * routing overheads for MANETs, etc. 
 */
class RouteCalcDelayTimer : public TimerHandler 
{
public:
  RouteCalcDelayTimer(SctpAgent *a, SctpDest_S *d) 
    : TimerHandler(), opAgent(a) {spDest = d;}
	
protected:
  virtual void expire(Event *);
  SctpAgent  *opAgent;
  SctpDest_S *spDest;  // destination this timer corresponds to
};

struct SctpInterface_S
{
  int        iNsAddr;
  int        iNsPort;
  NsObject  *opTarget;
  NsObject  *opLink;
};

enum SctpDestStatus_E
{
  SCTP_DEST_STATUS_INACTIVE,
  SCTP_DEST_STATUS_POSSIBLY_FAILED,
  SCTP_DEST_STATUS_ACTIVE,
  SCTP_DEST_STATUS_UNCONFIRMED
};

enum NodeType_E
{
  NODE_TYPE_STREAM_BUFFER,
  NODE_TYPE_RECV_TSN_BLOCK,
  NODE_TYPE_DUP_TSN,
  NODE_TYPE_SEND_BUFFER,
  NODE_TYPE_APP_LAYER_BUFFER,
  NODE_TYPE_INTERFACE_LIST,
  NODE_TYPE_DESTINATION_LIST,
  NODE_TYPE_PACKET_BUFFER,

  /* PN: 5/2007. NR-Sacks */
  NODE_TYPE_TSN

};

struct Node_S
{
  NodeType_E  eType;
  void       *vpData;   // u can put any data type into the node
  Node_S     *spNext;
  Node_S     *spPrev;
};

struct List_S
{
  u_int    uiLength;
  Node_S  *spHead;
  Node_S  *spTail;
};

struct SctpSendBufferNode_S;
struct SctpDest_S
{
  int        iNsAddr;  // ns "IP address"
  int        iNsPort;  // ns "port"

  /* Packet is simply used for determing src addr. The header stores dest addr,
   * which makes it easy to determine src target using
   * Connector->find(Packet *). Then with the target, we can determine src 
   * addr.
   */
  Packet    *opRoutingAssistPacket;

  int           iCwnd;                // current congestion window
  int           iSsthresh;            // current ssthresh value
  Boolean_E     eFirstRttMeasurement; // is this our first RTT measurement?
  double        dRto;                 // current retransmission timeout value
  double        dSrtt;                // current smoothed round trip time
  double        dRttVar;              // current RTT variance
  int           iPmtu;                // current known path MTU (not used)
  Boolean_E     eRtxTimerIsRunning;   // is there a timer running already?
  T3RtxTimer   *opT3RtxTimer;         // retransmission timer
  Boolean_E     eRtoPending;          // DATA chunk being used to measure RTT?

  int  iPartialBytesAcked; // helps to modify cwnd in congestion avoidance mode
  int  iOutstandingBytes;  // outstanding bytes still in flight (unacked)

  int                     iTimeoutCount;           // total number of timeouts
  int                     iErrorCount;             // destination error counter
  SctpDestStatus_E        eStatus;                 // active/inactive
  CwndDegradeTimer       *opCwndDegradeTimer;      // timer to degrade cwnd
  double                  dIdleSince;              // timestamp since idle
  HeartbeatTimeoutTimer  *opHeartbeatTimeoutTimer; // heartbeat timeout timer

  float fLossrate;          // Set from tcl, in SetLossrate(). Allows an
			    // Oracle to tell sender a given destination's
			    // lossrate. Used in CMT's RTX_LOSSRATE rtx
			    // policy.

  /* these are temporary variables needed per destination and they should
   * be cleared for each usage.
   */
  Boolean_E              eCcApplied;          // cong control already applied?
  SctpSendBufferNode_S  *spFirstOutstanding;  // first outstanding on this dest
  int                    iNumNewlyAckedBytes; // newly ack'd bytes in a sack

  /* These variables are generally not used. They are only for our
   * simulated reactive routing overheads for MANETs, etc. 
   */
  int                    iRcdCount;     // total count of route calc delays
  Boolean_E              eRouteCached;  // route cache hit or miss?
  RouteCacheFlushTimer  *opRouteCacheFlushTimer;
  RouteCalcDelayTimer   *opRouteCalcDelayTimer;  
  List_S                 sBufferedPackets;

  /* CMT variables follow. These are used only when CMT is used. 
   */
  Boolean_E  eSawNewAck;                // was this destination acked 
                                        // in the current SACK?
  u_int uiHighestTsnInSackForDest;      // highest TSN newly acked sent to this 
                                        // destination in SACK
  u_int      uiExpectedPseudoCum;       // expected pseudo cumack for this dest
  Boolean_E  eNewPseudoCum;             // is there a new pseudo cum for dest?
  Boolean_E  eFindExpectedPseudoCum;    // find a new expected pseudo cumack?
  u_int      uiExpectedRtxPseudoCum;    // expected rtx pseudo cum for dest
  Boolean_E  eFindExpectedRtxPseudoCum; // find a new expected rtx pseudo cum?
  u_int uiLowestTsnInSackForDest;       // lowest TSN newly acked sent to this 
                                        // destination in SACK

  u_int iNumPacketsSent;    // for the one packet limit during rtx. With 
                            // independent bottlenecks, apply limit per path.
  u_int uiBurstLength;      // tracks sending burst per SACK per dest
  Boolean_E eMarkedChunksPending;  // added global var per dest
  u_int uiRecover;                 // To enable newreno recovery per dest
  SctpRtxLimit_E eRtxLimit; // Which destination should use 
                            // RTX_ONE_PACKET_LIMIT
                            // when calling rtxmarkedchunks()

  u_int uiNumTimeouts;      // track number of timeouts for this dest

  Boolean_E eHBTimerIsRunning;   // CMT-PF: Track & stop HB timer when 
                                 // destination state changes from PF->Active 
  double dPFSince;               // CMT-PF: time when destination is marked PF

  u_int uiHighestTsnSent;	// PN: 12/21/2007. NR-SACKs
				// Track highest TSN sent on this destination

  /* End of CMT variables
   */
  
};

struct SctpRecvTsnBlock_S
{
  u_int  uiStartTsn;
  u_int  uiEndTsn;
};

struct SctpSendBufferNode_S
{
  SctpDataChunkHdr_S  *spChunk;
  Boolean_E            eAdvancedAcked;     // acked via rtx expiration (u-sctp)
  Boolean_E            eGapAcked;          // acked via a Gap Ack Block
  Boolean_E            eAddedToPartialBytesAcked; // already accounted for?
  int                  iNumMissingReports; // # times reported missing
  int                  iUnrelRtxLimit;     // limit on # of unreliable rtx's
  MarkedForRtx_E       eMarkedForRtx;      // has it been marked for rtx?
  Boolean_E            eIneligibleForFastRtx; // ineligible for fast rtx??
  int                  iNumTxs;            // # of times transmitted (orig+rtx)
  double               dTxTimestamp;  
  SctpDest_S          *spDest;             // destination last sent to

  /* variables used for extensions
   */
  u_int                uiFastRtxRecover;   // sctp-multipleFastRtxs.cc uses this

  Boolean_E eMeasuringRtt;  // Maintain which TSN is being used for RTT
			    // measurement, so that all TSNs can have
			    // their timestamp. This timestamp can be used
			    // for different things, and is used by CMT
			    // for the RTT heuristic to determine whether
			    // a TSN should be rtxd or not.  If now <=
			    // timestamp + srtt, then no rtx.
};

struct SctpStreamBufferNode_S
{
  SctpDataChunkHdr_S  *spChunk;
};

enum SctpStreamMode_E
{
  SCTP_STREAM_RELIABLE,
  SCTP_STREAM_UNRELIABLE
};

struct SctpInStream_S
{
  SctpStreamMode_E  eMode;
  u_short           usNextStreamSeqNum;
  List_S            sBufferedChunkList;
};

struct SctpOutStream_S
{
  SctpStreamMode_E  eMode;
  u_short           usNextStreamSeqNum;	
};

class SackGenTimer : public TimerHandler 
{
public:
  SackGenTimer(SctpAgent *a) : TimerHandler(), opAgent(a) { }
	
protected:
  virtual void expire(Event *);
  SctpAgent *opAgent;
};

class SctpAgent : public Agent 
{
public:
  SctpAgent();
  ~SctpAgent();
	
  virtual void  recv(Packet *pkt, Handler*);
  virtual void  sendmsg(int nbytes, const char *flags = 0);
  virtual int   command(int argc, const char*const* argv);
	
  void          T1InitTimerExpiration();
  void          T1CookieTimerExpiration();
  virtual void  Timeout(SctpChunkType_E, SctpDest_S *);
  virtual void  CwndDegradeTimerExpiration(SctpDest_S *);
  virtual void  HeartbeatGenTimerExpiration(double);
  void          SackGenTimerExpiration();
  void          RouteCacheFlushTimerExpiration(SctpDest_S *);
  void          RouteCalcDelayTimerExpiration(SctpDest_S *);

protected:
  virtual void  delay_bind_init_all();
  virtual int   delay_bind_dispatch(const char *varName, const char *localName,
				    TclObject *tracer);

  /* initialization stuff
   */
  void           SetDebugOutFile();
  virtual void   Reset();
  virtual void   OptionReset();
  virtual u_int  ControlChunkReservation();

  /* tracing functions
   */
  virtual void  TraceVar(const char*);
  virtual void  TraceAll();
  void          trace(TracedVar*);

  /* generic list functions
   */
  void       InsertNode(List_S *, Node_S *, Node_S *, Node_S *);
  void       DeleteNode(List_S *, Node_S *);
  void       ClearList(List_S *);

  /* PN: 5/2007. NR-Sacks */
  void 	 InsertNodeInSortedList(List_S *, Node_S *);

  /* multihome functions
   */
  void       AddInterface(int, int, NsObject *, NsObject *);
  void       AddDestination(int, int);
  int        SetPrimary(int);
  int        ForceSource(int);
  int        SetLossrate(int, float); // Oracle tells sender lossrate to
				      // dest. Called by handling of tcl
				      // set-destination-lossrate command

  /* chunk generation functions
   */
  virtual int  GenChunk(SctpChunkType_E, u_char *);
  u_int        GetNextDataChunkSize();
  int          GenOneDataChunk(u_char *);
  virtual int  GenMultipleDataChunks(u_char *, int);
  virtual int  BundleControlChunks(u_char *);

  /* sending functions
   */
  void          StartT3RtxTimer(SctpDest_S *);
  void          StopT3RtxTimer(SctpDest_S *);
  virtual void  AddToSendBuffer(SctpDataChunkHdr_S *,int, u_int, SctpDest_S *);
  void          RttUpdate(double, SctpDest_S *);
  virtual void  SendBufferDequeueUpTo(u_int);
  virtual void  AdjustCwnd(SctpDest_S *);
  void          AdvancePeerAckPoint();
  virtual u_int GetHighestOutstandingTsn();
  virtual void  FastRtx();
  void          TimeoutRtx(SctpDest_S *);
  void          MarkChunkForRtx(SctpSendBufferNode_S *, MarkedForRtx_E);
  Boolean_E     AnyMarkedChunks();
  virtual void  RtxMarkedChunks(SctpRtxLimit_E);
  void          SendHeartbeat(SctpDest_S *);
  SctpDest_S   *GetNextDest(SctpDest_S *);
  double        CalcHeartbeatTime(double);
  void          SetSource(SctpDest_S *);
  void          SetDestination(SctpDest_S *);
  void          SendPacket(u_char *, int, SctpDest_S *);
  SctpDest_S   *GetReplyDestination(hdr_ip *);
  u_int         TotalOutstanding();
  virtual void  SendMuch();

  /* receiving functions
   */
  Boolean_E  UpdateHighestTsn(u_int);
  Boolean_E  IsDuplicateChunk(u_int);
  void       InsertDuplicateTsn(u_int);
  void       UpdateCumAck();
  void       UpdateRecvTsnBlocks(u_int);
  void       PassToUpperLayer(SctpDataChunkHdr_S *);
  void       InsertInStreamBuffer(List_S *, SctpDataChunkHdr_S *);
  void       PassToStream(SctpDataChunkHdr_S *);
  void       UpdateAllStreams();
  
  /* PN: 5/2007. NR-Sacks */
  void	 BuildNonRenegTsnBlockList();
  virtual void UpdateHighestTsnSent();

  /* processing functions
   */
  void               ProcessInitChunk(u_char *);
  void               ProcessInitAckChunk(u_char *);
  void               ProcessCookieEchoChunk(SctpCookieEchoChunk_S *);
  void               ProcessCookieAckChunk(SctpCookieAckChunk_S *);
  void               ProcessDataChunk(SctpDataChunkHdr_S *);
  virtual Boolean_E  ProcessGapAckBlocks(u_char *, Boolean_E);
  virtual void       ProcessSackChunk(u_char *);
  
  /* PN: 5/2007. NR-Sacks */
  virtual void       ProcessNonRenegSackChunk(u_char *);
  void       	     ProcessNonRenegSackBlocks(u_char *);
  void               ProcessForwardTsnChunk(SctpForwardTsnChunk_S *);  
  void               ProcessHeartbeatAckChunk(SctpHeartbeatChunk_S *);  
  virtual void       ProcessOptionChunk(u_char *);
  virtual int        ProcessChunk(u_char *, u_char **);
  void               NextChunk(u_char **, int *);
  int    	     CalculateBytesInFlight();

  /* misc functions
   */
  void       Close();

  /* debugging functions
   */
  void DumpSendBuffer();

  /* sctp association state variable
   */
  SctpState_E     eState;

  /* App Layer buffer
   */
  List_S          sAppLayerBuffer;

  /* multihome variables
   */
  Classifier         *opCoreTarget;
  List_S              sInterfaceList;
  List_S              sDestList;
  SctpDest_S         *spPrimaryDest;       // primary destination
  SctpDest_S         *spNewTxDest;         // destination for new transmissions
  SctpDest_S         *spReplyDest; // reply with sacks or control chunk replies
  Boolean_E           eForceSource;
  int                 iAssocErrorCount;  // total error counter for the assoc

  /* heartbeat variables
   */
  HeartbeatGenTimer      *opHeartbeatGenTimer;      // to trigger a heartbeat

  /* sending variables
   */
  T1InitTimer       *opT1InitTimer;    // T1-init timer
  T1CookieTimer     *opT1CookieTimer;  // T1-cookie timer
  int                iInitTryCount;    // # of unsuccessful INIT attempts
  u_int              uiNextTsn;
  u_short            usNextStreamId; // used to round-robin the streams
  SctpOutStream_S   *spOutStreams;
  u_int              uiPeerRwnd;
  u_int              uiCumAckPoint;  
  u_int              uiAdvancedPeerAckPoint;
  u_int              uiHighestTsnNewlyAcked; // global for HTNA
  u_int              uiHighestTsnSent;       // NE : value of last TSN sent
  u_int              uiRecover;
  List_S             sSendBuffer;
  Boolean_E          eForwardTsnNeeded;  // is a FORWARD TSN chunk needed?
  Boolean_E          eSendNewDataChunks; // should we send new data chunks too?
  Boolean_E          eMarkedChunksPending; // chunks waiting to be rtx'd?
  Boolean_E          eApplyMaxBurst; 
  DataSource_E       eDataSource;
  u_int              uiBurstLength;  // tracks sending burst per SACK
  
  /* receiving variables
   */
  u_int            uiMyRwnd;
  u_int            uiCumAck;       
  u_int            uiHighestRecvTsn; // higest tsn recv'd of entire assoc
  List_S           sRecvTsnBlockList;
  List_S           sDupTsnList;
  int              iNumInStreams;
  
  /* PN: 5/2007. NR-Sacks */
  List_S	   sNonRenegTsnBlockList;
  List_S	   sNonRenegTsnList;
  SctpInStream_S  *spInStreams;
  Boolean_E        eStartOfPacket;             // for delayed sack triggering
  int              iDataPktCountSinceLastSack; // for delayed sack triggering
  Boolean_E        eSackChunkNeeded; // do we need to transmit a sack chunk?
  SackGenTimer    *opSackGenTimer;    // sack generation timer
  
  /* tcl bindable variables
   */
  u_int            uiDebugMask;     // 32 bits for fine level debugging
  int              iDebugFileIndex; // 1 debug output file per agent 
  u_int            uiPathMaxRetrans;
  u_int            uiChangePrimaryThresh;
  u_int            uiAssociationMaxRetrans;
  u_int            uiMaxInitRetransmits;
  u_int            uiHeartbeatInterval;
  u_int            uiMtu;
  u_int            uiInitialRwnd;
  int              iInitialSsthresh;
  u_int            uiIpHeaderSize;
  u_int            uiDataChunkSize;
  u_int            uiNumOutStreams;
  Boolean_E        eUseDelayedSacks; // are we using delayed sacks?
  double           dSackDelay;
  MaxBurstUsage_E  eUseMaxBurst;
  int              iInitialCwnd;
  double           dInitialRto;
  double           dMinRto;
  double           dMaxRto;
  int              iFastRtxTrigger;
  u_int            uiNumUnrelStreams;
  u_int            uiReliability; // k-rtx on all chunks & all unrel streams
  Boolean_E        eUnordered;    // sets for all chunks on all streams :-(
  RtxToAlt_E       eRtxToAlt;     // rtxs to alternate destination?
  DormantAction_E  eDormantAction;// behavior during dormant state
  double           dRouteCacheLifetime; 
  double           dRouteCalcDelay; 
  
  /* PN: 5/07. Send window simulation for Non-renegable Sacks.
   * 0 = Do not use send window (infinite send buffer)
   * else, simulate a finite send buffer.
   */
  /* ?? Swnd > 65535 bytes? */
  u_int            uiInitialSwnd;
  u_int            uiAvailSwnd;

  /* PN: 5/07. Use Non-renegable Sacks? 
   */
  Boolean_E	   eUseNonRenegSacks;

  Boolean_E        eTraceAll;     // trace all variables on one line?
  TracedInt        tiCwnd;        // trace cwnd for all destinations
  TracedInt        tiRwnd;        // trace rwnd
  TracedDouble     tdRto;         // trace rto for all destinations
  TracedInt        tiErrorCount;  // trace error count for all destinations
  TracedInt        tiFrCount;     // trace each time a fast rtx gets triggered
  TracedInt        tiTimeoutCount;// trace each time a timeout occurs
  TracedInt        tiRcdCount;    // trace each time a route calc delay occurs

  /* globally used non-tcl bindable variables, but rely on the tcl bindable
   */
  u_int           uiMaxPayloadSize; // we don't want this to be tcl bindable
  u_int           uiMaxDataSize; // max payload size - reserved control bytes
  FILE           *fhpDebugFile; 	   // file pointer for debugging output

  /* tracing variables that will be copied into hdr_sctp
   */
  u_int           uiNumChunks;
  SctpTrace_S    *spSctpTrace;  
};

#endif
