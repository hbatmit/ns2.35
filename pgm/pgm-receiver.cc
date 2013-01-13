
/*
 * pgm-receiver.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: pgm-receiver.cc,v 1.9 2010/03/08 05:54:52 tom_henderson Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Pragmatic General Multicast (PGM), Reliable Multicast
 *
 * pgm-receiver.cc
 *
 * This implements the Receiving PGM agent, "Agent/PGM/Receiver".
 *
 * Ryan S. Barnett, 2001
 * rbarnett@catarina.usc.edu
 */

#include "config.h"
#ifdef HAVE_STL

#include <stdlib.h>
#include <stdio.h>

#include <map>

#include "config.h"
#include "tclcl.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "random.h"
#include "basetrace.h"

#include "rcvbuf.h"

#include "pgm.h"

// ************************************************************
// Define the PGM Receive Timer Class
// ************************************************************
class PgmReceiver;

// Types of timers.
enum {
  NAK_TIMER = 0
};

class PgmReceiverTimer : public TimerHandler {
public:
  PgmReceiverTimer(PgmReceiver *a, int type) : TimerHandler(), data_(NULL) {
    a_ = a;
    type_ = type;
  }

  void * & data() { return data_; }

protected:
  virtual void expire(Event *e);
  PgmReceiver *a_;
  int type_;
  void *data_;
};

// ************************************************************
// Define the NakItem Class
// ************************************************************

// The different states of a NakItem entry.
enum {
  BACK_OFF_STATE = 0,
  WAIT_NCF_STATE = 1,
  WAIT_DATA_STATE = 2
};

class NakItem {
public:
  NakItem(PgmReceiver *a, int seqno) : nak_state_(BACK_OFF_STATE),
       nak_sent_(false), seqno_(seqno), ncf_retry_count_(0),
       data_retry_count_(0), nak_timer_(a, NAK_TIMER) { }

  int & nak_state() { return nak_state_; }
  bool & nak_sent() { return nak_sent_; }
  int & seqno() { return seqno_; }
  int & ncf_retry_count() { return ncf_retry_count_; }
  int & data_retry_count() { return data_retry_count_; }
  PgmReceiverTimer & nak_timer() { return nak_timer_; }

protected:

  // The current state of the NAK entry.
  int nak_state_;

  // Whether or not this NAK has been transmitted once or more.
  bool nak_sent_;

  // Sequence number of the missing NAK item.
  int seqno_;

  // Number of times we have sent out a NAK but timed out waiting for an NCF.
  int ncf_retry_count_;

  // Number of times we got an NCF but timed out waiting for RDATA/ODATA.
  int data_retry_count_;

  PgmReceiverTimer nak_timer_;
};

// ************************************************************
// Define the PGM Receiver Class
// ************************************************************
static int pgm_rcv_uid_ = 0;

struct Stats {
  // Number of naks that did NOT get sent because we received an NCF
  // before our timer went up.
  int naks_transmitted_;

  // Number of duplicate naks that were sent to the upstream node.
  int naks_duplicated_;
};

class PgmReceiver: public Agent {
public:
  PgmReceiver();

  virtual void recv(Packet *, Handler *);
  virtual void timeout(int type, void *data);
  virtual int command(int argc, const char*const* argv);

protected:

  void handle_spm(Packet *pkt);
  void handle_odata(Packet *pkt);
  void handle_rdata(Packet *pkt);
  void handle_nak(Packet *pkt);
  void handle_ncf(Packet *pkt);

  void generate_Nak(int seqno);
  void cancel_Nak(int seqno, NakItem *nitem = NULL);
  void timeout_nak(NakItem *data);
  void send_nak(int seqno);

  void print_stats();
  void display_packet(Packet *pkt); // For debugging.

  void trace_event(char *evType, double evTime);

  EventTrace * et_;  //Trace Object for Custom Event Trace

  char uname_[16]; // Unique PGM receiver name, for debugging.

  // Various statistical information.
  Stats stats_;

  // Maximum number of times we can send out a NAK and time-out waiting for
  // an NCF reply. Once we hit this many times, we discard the NAK state
  // entirely and loose data.
  int max_nak_ncf_retries_;

  // Maximum number of times we can time-out waiting for RDATA after an
  // NCF confirmation for a NAK request.  Once we hit this many times, we
  // discard the NAK state entirely and loose data.
  int max_nak_data_retries_;

  // A random amount of this time period will be selected to wait for an
  // NCF after detecting a gap in the data stream, before sending out a NAK.
  double nak_bo_ivl_;

  // The amount of time to wait for a NCF packet after sending out a NAK
  // packet to the upstream node.
  double nak_rpt_ivl_;

  // The amount of time to wait for RDATA after receiving an NCF confirmation
  // for a given NAK.
  double nak_rdata_ivl_;

  // Whether or not the tsi/upstream_node/upstream_face are valid, i.e. have
  // we received at least one SPM packet for the session.
  bool have_tsi_state_;

  int spm_seqno_; // Last largest received SPM sequence number.

  ns_addr_t tsi_; // Transport Session ID
  ns_addr_t upstream_node_; // Address of upstream PGM router.
  int upstream_iface_; // Interface of upstream PGM router.

  // Source and group of ODATA/RDATA packets. Used when sending NAK messages.
  ns_addr_t source_;
  ns_addr_t group_;

  // Keep track of received packets and collect various statistics.
  RcvBuffer rcvbuf_;

  // Collection of sequence numbers that we are waiting for RDATA/ODATA.
  map<int, NakItem> naks_;

};

static class PgmReceiverClass : public TclClass {
public:
  PgmReceiverClass() : TclClass("Agent/PGM/Receiver") {}
  TclObject * create(int , const char * const * ) {
    return (new PgmReceiver());
  }
} class_pgm_receiver;

void PgmReceiverTimer::expire(Event *) {
  a_->timeout(type_, data_);
}

// Constructor.
PgmReceiver::PgmReceiver() : Agent(PT_PGM), have_tsi_state_(false),
			     spm_seqno_(-1)
{
  stats_.naks_transmitted_ = 0;
  stats_.naks_duplicated_ = 0;

  sprintf (uname_, "pgmRecv-%d", pgm_rcv_uid_++);
  bind("max_nak_ncf_retries_", &max_nak_ncf_retries_);
  bind("max_nak_data_retries_", &max_nak_data_retries_);
  bind_time("nak_bo_ivl_", &nak_bo_ivl_);
  bind_time("nak_rpt_ivl_", &nak_rpt_ivl_);
  bind_time("nak_rdata_ivl_", &nak_rdata_ivl_);

  et_ = (EventTrace *) NULL;

}

// Code to execute when a packet is received.
void PgmReceiver::recv(Packet *pkt, Handler *)
{
  hdr_pgm *hp = HDR_PGM(pkt);

  hdr_cmn *hc = HDR_CMN(pkt);

  if (hc->ptype_ != PT_PGM) {
    printf("%s ERROR (PgmReceiver::recv): received non PGM pkt type %d, discarding.\n", uname_, hc->ptype_);
    Packet::free(pkt);
    return;
  }

#ifdef PGM_DEBUG
  display_packet(pkt);
#endif

  switch(hp->type_) {
  case PGM_SPM:
    handle_spm(pkt);
    break;
  case PGM_ODATA:
    handle_odata(pkt);
    break;
  case PGM_RDATA:
    handle_rdata(pkt);
    break;
  case PGM_NAK:
    // We only receive a NAK if it is multicast from another receiver
    // who is not directly connected to a PGM router.
    handle_nak(pkt);
    break;
  case PGM_NCF:
    handle_ncf(pkt);
    break;
  default:
    printf("ERROR (PgmReceiver::recv): Received invalid PGM type %d.\n",
	   hp->type_);
    break;
  }

  Packet::free(pkt);
}

// Code to execute when a timeout occurs.
void PgmReceiver::timeout(int type, void *data)
{

  switch(type) {
  case NAK_TIMER:
    timeout_nak((NakItem *) data);
    break;
  default:
    printf("ERROR (PgmReceiver::timeout): Unknown timeout type %d.\n", type);
    break;
  }

}

// Called when a TCL command is issued to the PGM Receiver object.
int PgmReceiver::command(int argc, const char*const* argv)
{
  //  Tcl& tcl = Tcl::instance();

  if (argc == 2) {
    if (strcmp(argv[1], "print-stats") == 0) {
      print_stats();
      return (TCL_OK);
    }
  }
  else if (argc == 3) {
    if (strcmp(argv[1], "eventtrace") == 0) {
      et_ = (EventTrace *)TclObject::lookup(argv[2]);
      return (TCL_OK);
    }
  }

  return (Agent::command(argc, argv));
}

void PgmReceiver::trace_event(char *evType, double evTime) {

  if (et_ == NULL) return;
  char *wrk = et_->buffer();
  char *nwrk = et_->nbuffer();

  if (wrk != NULL) {
    sprintf(wrk, "E "TIME_FORMAT" %d %d PGM %s "TIME_FORMAT, 
            et_->round(Scheduler::instance().clock()),   
            addr(),                    
            0,                   
            evType,                  
			evTime);	
  if (nwrk != 0)
    sprintf(nwrk,
			"E -t "TIME_FORMAT" -o PGM -e %s -s %d.%d -d %d.%d",
			et_->round(Scheduler::instance().clock()),   // time
			evType,                    // event type
			addr(),                       // owner (src) node id
			port(),                       // owner (src) port id
			0,                      // dst node id
			0                       // dst port id
			);
	et_->dump();
  }

}

void PgmReceiver::handle_spm(Packet *pkt)
{
  hdr_cmn *hc = HDR_CMN(pkt);
  hdr_ip *hip = HDR_IP(pkt);
  hdr_pgm *hp = HDR_PGM(pkt);
  hdr_pgm_spm *hps = HDR_PGM_SPM(pkt);

  if (have_tsi_state_ == false) {
    // First SPM message.
    have_tsi_state_ = true;

    // Set the TSI.
    tsi_ = hp->tsi_;

    // Set the source and group addresses for this TSI.
    source_ = hip->src();
    group_ = hip->dst();
  }
  else {

    // Check that the TSI is correct.
    if (!(hp->tsi_.isEqual (tsi_))) {
      printf("%s Received SPM with incorrect TSI, discarding.\n", uname_);
      return;
    }

    // Check that the sequence number is newer than a previous SPM message.
    if (hp->seqno_ <= spm_seqno_) {
      printf("%s received an old SPM seqno, discarding.\n", uname_);
      return;
    }
  }

  // Set the initial sequence number.
  spm_seqno_ = hp->seqno_;

  // Set the upstream node.
  upstream_node_ = hps->spm_path_;

  // Set the upstream interface.
  upstream_iface_ = hc->iface();
}

void PgmReceiver::handle_odata(Packet *pkt)
{
  hdr_pgm *hp = HDR_PGM(pkt);

  // Check that the TSI is correct.
  if ( (have_tsi_state_ == true) && !(hp->tsi_.isEqual (tsi_)) ) {
    printf("PGM Receiver received ODATA with incorrect TSI, discarding.\n");
    return;
  }

  double clock = Scheduler::instance().clock();

  if (rcvbuf_.nextpkt_ < hp->seqno_) {
    int lo = rcvbuf_.nextpkt_;
    int hi = hp->seqno_ - 1;

    for (int i = lo; i <= hi; i++) {
      printf("%s detected loss of seq %d\n", uname_, i);

      if (have_tsi_state_ == false) {
	printf("%s has no TSI/SPM state when lost packet was detected. This results in unrecoverable data loss.\n", uname_);
      }
      else {
	generate_Nak(i);
      }
    }
  }

  rcvbuf_.add_pkt(hp->seqno_, clock);

  // Recept of ODATA that came in late could cancel a previously
  // generated NAK.
  cancel_Nak(hp->seqno_, NULL);
}

void PgmReceiver::handle_rdata(Packet *pkt)
{

  hdr_pgm *hp = HDR_PGM(pkt);

  // Check that the TSI is correct.
  if ( (have_tsi_state_ == true) && !(hp->tsi_.isEqual (tsi_)) ) {
    printf("%s received RDATA with incorrect TSI, discarding.\n", uname_);
    return;
  }

  if ( (rcvbuf_.nextpkt_ > hp->seqno_) && !rcvbuf_.exists_pkt(hp->seqno_) ) {
    // The receive state may or may not exist, depending on how late the
    // RDATA packet came back to this node.
    cancel_Nak(hp->seqno_, NULL);
  }

  double clock = Scheduler::instance().clock();
  rcvbuf_.add_pkt (hp->seqno_, clock);

}

// The receiver will receive a NAK only if it is sent as a multicast from
// another receiver in the event that the other receiver is not directly
// connected to a PGM router.
void PgmReceiver::handle_nak(Packet *pkt)
{
  hdr_pgm *hp = HDR_PGM(pkt);

  // Check that the TSI is correct.
  if ( (have_tsi_state_ == true) && !(hp->tsi_.isEqual (tsi_)) ) {
    printf("%s received NAK with incorrect TSI, discarding.\n", uname_);
    return;
  }

  // Locate the nak state for the given multicast NAK.
  map<int, NakItem>::iterator result = naks_.find(hp->seqno_);

  if (result == naks_.end()) {
    // No state was found. Discard the NCF.
    printf("%s received multicast NAK but no NAK state found, discarding.\n", uname_);
    return;
  }

  NakItem *nitem = &((*result).second);

  switch( nitem->nak_state() ) {
  case BACK_OFF_STATE:
    // Move to WAIT_NCF_STATE.
    nitem->nak_state() = WAIT_NCF_STATE;

    // Reset the timer.
    nitem->nak_timer().resched(nak_rpt_ivl_);

    break;
  case WAIT_NCF_STATE:
    // Stay in the same state.

    // Reset the timer.
    nitem->nak_timer().resched(nak_rpt_ivl_);

    break;
  case WAIT_DATA_STATE:
    // Stay in the same state.

    // Reset the timer.
    nitem->nak_timer().resched(nak_rdata_ivl_);

    break;
  default:
    printf("ERROR (PgmReceiver::handle_nak): Unknown nak state %d.\n", nitem->nak_state());
    break;
  }

}

void PgmReceiver::handle_ncf(Packet *pkt)
{
  hdr_pgm *hp = HDR_PGM(pkt);

  // Check that the TSI is correct.
  if ( (have_tsi_state_ == true) && !(hp->tsi_.isEqual (tsi_)) ) {
    printf("%s received NCF with incorrect TSI, discarding.\n", uname_);
    return;
  }

  // Check that the NCF came from our uplink interface.
  // Causes a problem because iface doesn't get relabeled.  But this isn't
  // needed if every receiver also has a PGM/Agent running on the node.
  /*
  if (hc->iface() != upstream_iface_) {
    printf("%s received NCF from non-upstream interface, discarding. Upstream_iface = %d\n", uname_, upstream_iface_);
    return;
  }
  */

  // Locate the nak state for the given NCF.
  map<int, NakItem>::iterator result = naks_.find(hp->seqno_);

  if (result == naks_.end()) {
    // No state was found. Discard the NCF.
    printf("%s received NCF but no NAK state found, discarding.\n", uname_);
    return;
  }  

  NakItem *nitem = &((*result).second);

  switch( nitem->nak_state() ) {
  case BACK_OFF_STATE:
    // Move to WAIT_DATA_STATE.
    nitem->nak_state() = WAIT_DATA_STATE;

    // Reset the timer.
    nitem->nak_timer().resched(nak_rdata_ivl_);

    break;
  case WAIT_NCF_STATE:
    // Move to WAIT_DATA_STATE.
    nitem->nak_state() = WAIT_DATA_STATE;

    // Reset the timer.
    nitem->nak_timer().resched(nak_rdata_ivl_);
    break;
  case WAIT_DATA_STATE:
    // Stay in the same state.

    // Reset the timer.
    nitem->nak_timer().resched(nak_rdata_ivl_);
    
    break;
  default:
    printf("ERROR (PgmReceiver::handle_ncf): Unknown nak state %d.\n", nitem->nak_state());
    return;
  }

}

void PgmReceiver::generate_Nak(int seqno)
{
#ifdef PGM_DEBUG
  double now = Scheduler::instance().clock();
  printf("at %f %s generating NAK state for seqno %d.\n", now, uname_, seqno);
#endif

  // Insert the given sequence number into the nak map.
  pair<map<int, NakItem>::iterator, bool> result;

  result = naks_.insert(pair<int, NakItem>(seqno, NakItem(this, seqno)));

  NakItem *nitem = &(result.first->second);

  if (result.second == true) {
    // New NAK entry was added. Select a backoff time period over nak_bo_ivl_.
    double backoff = Random::uniform(nak_bo_ivl_);

    // Set the data field of the nak timer.
    nitem->nak_timer().data() = nitem;

    // Set the NAK timer to expire in BACK_OFF_STATE with the selected time.
    nitem->nak_timer().resched(backoff);

    printf("backoff: %f\n", backoff);
    trace_event("DETECT", backoff); 	//Detected Loss, will send NACK after backoff
  }
  else {
    printf("%s generate_Nak was called with NAK state already established, ignoring.\n", uname_);
  }

}

void PgmReceiver::cancel_Nak(int seqno, NakItem *nitem)
{

  if (nitem == NULL) {
    // Look up the sequence number in the nak map.
    map<int, NakItem>::iterator result = naks_.find(seqno);

    if (result == naks_.end()) {
      // The NAK state was not found. This is fine since the handle_odata()
      // function calls cancel_Nak on all packets in case 
      return;
    }

    nitem = &((*result).second);
  }

  // Cancel the NAK timer.
  nitem->nak_timer().force_cancel();

  // Erase the item from the Nak map.
  if (!naks_.erase(seqno)) {
    printf("ERROR (PgmReceiver::cancel_Nak): Failed erasing seqno from nak map.\n");
  }

}

void PgmReceiver::timeout_nak(NakItem *nitem)
{

  double backoff;

  switch(nitem->nak_state()) {
  case BACK_OFF_STATE:

    if (nitem->nak_sent() == false) {
      nitem->nak_sent() = true;
    }
    else {
      stats_.naks_duplicated_++;
    }

    // Move into WAIT_NCF_STATE.
    nitem->nak_state() = WAIT_NCF_STATE;

    // Set new timer to go off.
    nitem->nak_timer().resched(nak_rpt_ivl_);

    send_nak(nitem->seqno());

    break;
  case WAIT_NCF_STATE:

    // If we have exceeded the number of times we can retry this NAK,
    // then cancel.
    if (nitem->ncf_retry_count() > max_nak_ncf_retries_) {
      // Cancel this NAK generation. Remove all state associated with the
      // NAK, we have unrecoverable data loss.
      printf("%s reached max_nak_ncf_retries, stopping NAK generation.\n", uname_);
      cancel_Nak(nitem->seqno(), nitem);
      return;
    }

    nitem->ncf_retry_count() += 1;

    // Move into BACK_OFF_STATE
    nitem->nak_state() = BACK_OFF_STATE;

    // Set timer to go off.
    backoff = Random::uniform(nak_bo_ivl_);

    // Set the NAK timer to expire in BACK_OFF_STATE with the selected time.
    nitem->nak_timer().resched(backoff);

    break;
  case WAIT_DATA_STATE:

    // Exceeded the number of times we wait for RDATA for this confirmed NAK?
    if (nitem->data_retry_count() > max_nak_data_retries_) {
      printf("%s reached max_nak_data_retries, stopping NAK generation.\n", uname_);
      cancel_Nak(nitem->seqno(), nitem);
      return;
    }

    nitem->data_retry_count() += 1;

    // Move into BACK_OFF_STATE
    nitem->nak_state() = BACK_OFF_STATE;

    // Set timer to go off.
    backoff = Random::uniform(nak_bo_ivl_);

    // Set the NAK timer to expire in BACK_OFF_STATE with the selected time.
    nitem->nak_timer().resched(backoff);

    break;
  default:
    printf("ERROR (PgmReceiver::timeout_nak): Unknown NAK state %d.\n", nitem->nak_state());
    break;
  }

}

void PgmReceiver::send_nak(int seqno)
{
  printf("%s send_nak is called.\n", uname_);

  Packet *nak_pkt = allocpkt();
  // Set the simulated size of the NAK packet.
  hdr_cmn *nak_hc = HDR_CMN(nak_pkt);
  nak_hc->size_ = sizeof(hdr_pgm) + sizeof(hdr_pgm_nak);
  nak_hc->ptype_ = PT_PGM;

  // Set the destination address to be our upstream node.
  hdr_ip *nak_hip = HDR_IP(nak_pkt);
  nak_hip->dst() = upstream_node_;

  // Set the color for NAK packets in nam.
  nak_hip->fid_ = 8;

  // Fill in the PGM header for the NAK packet.
  hdr_pgm *nak_hp = HDR_PGM(nak_pkt);
  nak_hp->type_ = PGM_NAK;
  nak_hp->tsi_ = tsi_;
  nak_hp->seqno_ = seqno;

  // Fill in the PGM NAK header for the NAK packet.
  hdr_pgm_nak *nak_hpn = HDR_PGM_NAK(nak_pkt);
  nak_hpn->source_ = source_;
  nak_hpn->group_ = group_;

  // Increment the statistical counter that keeps track of the number
  // of naks transmitted.
  stats_.naks_transmitted_++;

  // Send out the packet.
  send(nak_pkt, 0);

  // TBA: Send out the NAK packet to multicast with TTL 1 if the uplink
  //      PGM router is not directly connected to this node.
}

void PgmReceiver::print_stats()
{
  printf("%s:\n", uname_);
  printf("\tLast packet:\t\t%d\n", rcvbuf_.nextpkt_-1);
  printf("\tMax packet:\t\t%d\n", rcvbuf_.maxpkt_);
  if (rcvbuf_.pkts_recovered_) {
    printf("\tPackets recovered:\t%d\n", rcvbuf_.pkts_recovered_);
    printf("\tLatency (min, max, avg):\t%f, %f, %f\n",
	   rcvbuf_.min_delay_, rcvbuf_.max_delay_,
	   rcvbuf_.delay_sum_ / rcvbuf_.pkts_recovered_);
  }
  if (rcvbuf_.duplicates_) {
    printf("\tDuplicate RDATA:\t%d\n", rcvbuf_.duplicates_);
  }
  printf("\tTotal NAKs sent:\t%d\n", stats_.naks_transmitted_);
  printf("\tRetransmitted NAKs:\t%d\n", stats_.naks_duplicated_);
}

#ifdef PGM_DEBUG
void PgmReceiver::display_packet(Packet *pkt)
{

  double now = Scheduler::instance().clock();

  hdr_ip *hip = HDR_IP(pkt);
  hdr_cmn *hc = HDR_CMN(pkt);

  printf("at %f %s received packet type ", now, uname_);

  hdr_pgm *hp = HDR_PGM(pkt);
  
  hdr_pgm_spm *hps;
  hdr_pgm_nak *hpn;

  switch(hp->type_) {
  case PGM_SPM:
    hps = HDR_PGM_SPM(pkt);

    printf("SPM (TSI %d:%d) from %d:%d to %d:%d iface %d, size %d, seqno %d, spm_path %d:%d\n", hp->tsi_.addr_, hp->tsi_.port_, hip->saddr(), hip->sport(), hip->daddr(), hip->dport(), hc->iface(), hc->size(), hp->seqno_, hps->spm_path_.addr_, hps->spm_path_.port_);

    break;
  case PGM_ODATA:
    printf("ODATA (TSI %d:%d) from %d:%d to %d:%d iface %d, size %d, seqno %d\n", hp->tsi_.addr_, hp->tsi_.port_, hip->saddr(), hip->sport(), hip->daddr(), hip->dport(), hc->iface(), hc->size(), hp->seqno_);

    break;
  case PGM_RDATA:
    printf("RDATA (TSI %d:%d) from %d:%d to %d:%d iface %d, size %d, seqno %d\n", hp->tsi_.addr_, hp->tsi_.port_, hip->saddr(), hip->sport(), hip->daddr(), hip->dport(), hc->iface(), hc->size(), hp->seqno_);

    break;
  case PGM_NAK:
    hpn = HDR_PGM_NAK(pkt);

    printf("NAK (TSI %d:%d) from %d:%d to %d:%d iface %d, size %d, seqno %d, source %d:%d, group %d:%d\n", hp->tsi_.addr_, hp->tsi_.port_, hip->saddr(), hip->sport(), hip->daddr(), hip->dport(), hc->iface(), hc->size(), hp->seqno_, hpn->source_.addr_, hpn->source_.port_, hpn->group_.addr_, hpn->group_.port_);

    break;
  case PGM_NCF:
    printf("NCF (TSI %d:%d) from %d:%d to %d:%d iface %d, size %d, seqno %d\n", hp->tsi_.addr_, hp->tsi_.port_, hip->saddr(), hip->sport(), hip->daddr(), hip->dport(), hc->iface(), hc->size(), hp->seqno_);

    break;
  default:
    printf("UNKNOWN (TSI %d:%d) from %d:%d to %d:%d iface %d, size %d, seqno %d\n", hp->tsi_.addr_, hp->tsi_.port_, hip->saddr(), hip->sport(), hip->daddr(), hip->dport(), hc->iface(), hc->size(), hp->seqno_);

    break;
  }

}
#endif // PGM_DEBUG

#endif //HAVE_STL
