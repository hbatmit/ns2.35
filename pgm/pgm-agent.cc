
/*
 * pgm-agent.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: pgm-agent.cc,v 1.11 2010/03/08 05:54:52 tom_henderson Exp $
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
 * pgm-agent.cc
 *
 * This implements the network element PGM agent, "Agent/PGM".
 *
 * Ryan S. Barnett, 2001
 * rbarnett@catarina.usc.edu
 */

#include "config.h"
#ifdef HAVE_STL

#include <stdlib.h>
#include <stdio.h>

#include <map>
#include <list>
#include <algorithm>

#include "config.h"
#include "tclcl.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "random.h"
#include "basetrace.h"

#include "pgm.h"

// ************************************************************
// Register the PGM packet headers.
// ************************************************************

// Declare the static header offsets.
int hdr_pgm::offset_;
int hdr_pgm_spm::offset_;
int hdr_pgm_nak::offset_;

// Register the hdr_pgm with the packet header manager.
class PGMHeaderClass : public PacketHeaderClass {
public:
  PGMHeaderClass() : PacketHeaderClass("PacketHeader/PGM", sizeof(hdr_pgm)) {
    bind_offset(&hdr_pgm::offset_);
  }

} class_pgmhdr;

// Register the hdr_pgm_spm with the packet header manager.
class PGM_SPMHeaderClass : public PacketHeaderClass {
public:
  PGM_SPMHeaderClass() : PacketHeaderClass("PacketHeader/PGM_SPM",
					   sizeof(hdr_pgm_spm)) {
    bind_offset(&hdr_pgm_spm::offset_);
  }

} class_pgm_spmhdr;

// Register the hdr_pgm_nak with the packet header manager.
class PGM_NAKHeaderClass : public PacketHeaderClass {
public:
  PGM_NAKHeaderClass() : PacketHeaderClass("PacketHeader/PGM_NAK",
					   sizeof(hdr_pgm_nak)) {
    bind_offset(&hdr_pgm_nak::offset_);
  }

} class_pgm_nakhdr;


// ************************************************************
// Define the PGM Agent Timer Class
// ************************************************************
class PgmAgent;

// Different timer types.
enum {
  TIMER_NAK_RETRANS = 0,
  TIMER_NAK_RPT = 1,
  TIMER_NAK_RDATA = 2,
  TIMER_NAK_ELIM = 3
};

class PgmAgentTimer : public TimerHandler {
public:
  PgmAgentTimer(PgmAgent *a, int type) : TimerHandler(), data_(NULL) {
    a_ = a;
    type_ = type;
  }

  void * &data() { return data_; }

protected:
  virtual void expire(Event *e);
  PgmAgent *a_;
  int type_;
  void *data_;
};

// ************************************************************
// Define the Repair State control block.
// ************************************************************

// Different repair states.
enum {
  NAK_PENDING = 0,
  NAK_CONFIRMED = 1
};

class StateInfo;

class RepairState {
public:
  RepairState(PgmAgent *a, StateInfo *sinfo, int seqno, ns_addr_t &source,
	      ns_addr_t &group) : seqno_(seqno), source_(source),
				  group_(group),
        nak_state_(NAK_PENDING), nak_elimination_(true),
	nak_retrans_timer_(a, TIMER_NAK_RETRANS),
	nak_rpt_timer_(a, TIMER_NAK_RPT),
	nak_rdata_timer_(a, TIMER_NAK_RDATA),
	nak_elim_timer_(a, TIMER_NAK_ELIM),
        sinfo_(sinfo)
  { }

  int & seqno() { return seqno_; }
  ns_addr_t & source() { return source_; }
  ns_addr_t & group() { return group_; }

  int & nak_state() { return nak_state_; }
  bool & nak_elimination() { return nak_elimination_; }
  PgmAgentTimer & nak_retrans_timer() { return nak_retrans_timer_; }
  PgmAgentTimer & nak_rpt_timer() { return nak_rpt_timer_; }
  PgmAgentTimer & nak_rdata_timer() { return nak_rdata_timer_; }
  PgmAgentTimer & nak_elim_timer() { return nak_elim_timer_; }
  list<int> & iface_list() { return iface_list_; }
  list<NsObject *> & agent_list() { return agent_list_; }
  StateInfo * & sinfo() { return sinfo_; }

protected:

  // Which sequence number is being requested for repair.
  int seqno_;

  ns_addr_t source_; // Original source of ODATA for the repair.
  ns_addr_t group_; // The multicast group.

  int nak_state_; // Present repair block state.

  // Indicates whether or not we are to discard incoming NAK packets
  // once a previous NAK is outstanding (got NCF, waiting for RDATA). 
  // (See 7.4 of PGM specification) By default we do. When nak_elim_timer_
  // expires, then we do not.
  bool nak_elimination_;

  // This timer controls sending retransmissions of NAK packets.
  PgmAgentTimer nak_retrans_timer_;

  // Timer that measures how long we can repeat NAK packets while waiting
  // for NCF confirmation. Once expires, the repair state is removed.
  PgmAgentTimer nak_rpt_timer_;

  // Timer that is triggered waiting for RDATA for a given NAK seqno,
  // provided that NAK has been confirmed by an NCF.  Only gets set
  // once NCF is received.
  PgmAgentTimer nak_rdata_timer_;

  // Timer that is triggered when we disable nak_elimination_, allowing
  // a duplicate NAK to be processed. This occurs after a previous
  // NAK has been confirmed with an NCF, but before the RDATA has been
  // received.  This timer gets set when we receive an NCF for a pending
  // NAK.
  PgmAgentTimer nak_elim_timer_;

  // List of interfaces upon which the RDATA will be sent to.
  list<int> iface_list_;

  // List of agents upon which the RDATA will be sent to.
  list<NsObject *> agent_list_;

  // Back-pointer to the state information block that is holding this
  // repair data.  We use this so we can get the upstream_path and the TSI.
  StateInfo *sinfo_;

};

// ************************************************************
// Define the TSI State control block.
// ************************************************************

class StateInfo {
public:

  StateInfo(ns_addr_t tsi) : tsi_(tsi), spm_seqno_(-1) { }

  // Only used if the container holding StateInfo's will be in sorted order.
  int operator<(const StateInfo &right) const {
    return ((tsi_.addr_ < right.tsi_.addr_) || ( (tsi_.addr_==right.tsi_.addr_) && (tsi_.port_ < right.tsi_.port_)));
  }

  ns_addr_t & tsi() { return tsi_; }
  int & spm_seqno() { return spm_seqno_; }
  ns_addr_t & upstream_node() { return upstream_node_; }
  int & upstream_iface() { return upstream_iface_; }
  map<int, RepairState> & repair() { return repair_; }

protected:

  ns_addr_t tsi_; // Transport Session ID
  int spm_seqno_; // Most recent SPM sequence number.
  ns_addr_t upstream_node_; // Upstream node address.
  int upstream_iface_; // Upstream interface number.

  // Map between a NAK sequence number and the corresponding repair state
  // for that sequence number.
  map<int, RepairState> repair_;

};


// ************************************************************
// Define the PGM Agent Class
// ************************************************************

// Structure to hold statistical information for PGM Agent.
struct Stats {
  int num_unsolicited_ncf_;
  int num_unsolicited_rdata_;
  int num_suppressed_naks_;
  int num_naks_transmitted_;
};

// Used to count number of unique pgm agents.
static int pgm_agent_uid_ = 0;

class PgmAgent : public Agent {
public:
  PgmAgent();
  virtual void recv(Packet *, Handler *);
  virtual void timeout(int type, void *data);
  virtual int command(int argc, const char*const* argv);

protected:

  void handle_spm(Packet *pkt);
  void handle_odata(Packet *pkt);
  void handle_rdata(Packet *pkt);
  void handle_nak(Packet *pkt);
  void handle_ncf(Packet *pkt);

  void send_nak(ns_addr_t &upstream_node, ns_addr_t &tsi, int seqno, ns_addr_t &source, ns_addr_t &group);

  void timeout_nak_retrans(RepairState *rstate);
  void timeout_nak_rpt(RepairState *rstate);
  void timeout_nak_rdata(RepairState *rstate);
  void timeout_nak_elim(RepairState *rstate);

  void remove_repair_state(RepairState *rstate);

  void print_stats();

  void trace_event(char *evType, double evTime);

#ifdef PGM_DEBUG
  void display_packet(Packet *pkt);
#endif

  NsObject* iface2link(int iface);
  NsObject* pkt2agent (Packet *pkt);

  StateInfo * find_TSI(ns_addr_t &tsi);
  StateInfo * insert_TSI(ns_addr_t &tsi);

  EventTrace * et_; //Trace Object for custom event trace

  int pgm_enabled_; // Is this agent enabled? Default is YES.

  char uname_[16]; // Agent's unique name.

  Stats stats_; // Statistical information.

  // TSI-indexed state control block list.
  list<StateInfo> state_list_;

  // Number of seconds to wait between retransmitting a NAK that is waiting
  // for a NCF packet.
  double nak_retrans_ival_;

  // The length of time for which a network element will continue to repeat
  // NAKs while waiting for a corresponding NCF.  Once this time expires and
  // no NCF is received, then we remove the entire repair state.
  double nak_rpt_ival_;

  // The length of time for which a network element will wait for the
  // corresponding RDATA before removing the entire repair state.
  double nak_rdata_ival_;

  // Once a NAK has been confirmed, network elements must discard all
  // further NAKs for up to this length of time.  Should be a fraction
  // of nak_rdata_ival_.
  double nak_elim_ival_;

};

static class PgmClass : public TclClass {
public:
  PgmClass() : TclClass("Agent/PGM") {}
  TclObject * create(int , const char * const * ) {
    return (new PgmAgent());
  }
} class_pgm_agent;

void PgmAgentTimer::expire(Event *) {
  a_->timeout(type_, data_);
}

// Constructor.
PgmAgent::PgmAgent() : Agent(PT_PGM), pgm_enabled_(1)
{
  // Set the unique identifier.
  sprintf (uname_, "pgmAgent-%d", pgm_agent_uid_++);

  // Initialize statistics.
  stats_.num_unsolicited_ncf_ = 0;
  stats_.num_unsolicited_rdata_ = 0;
  stats_.num_suppressed_naks_ = 0;
  stats_.num_naks_transmitted_ = 0;

  bind("pgm_enabled_", &pgm_enabled_);
  bind_time("nak_retrans_ival_", &nak_retrans_ival_);
  bind_time("nak_rpt_ival_", &nak_rpt_ival_);
  bind_time("nak_rdata_ival_", &nak_rdata_ival_);
  bind_time("nak_elim_ival_", &nak_elim_ival_);

  et_ = (EventTrace *) NULL;
}

// Code to execute when a packet is received.
void PgmAgent::recv(Packet* pkt, Handler*)
{
  hdr_pgm *hp = HDR_PGM(pkt);

  if (!pgm_enabled_) {
    target_->recv(pkt);
    return;
  }

  hdr_cmn *hc = HDR_CMN(pkt);

  if (hc->ptype_ != PT_PGM) {
    printf("%s ERROR (PgmAgent::recv): received non PGM pkt type %d, discarding.\n", uname_, hc->ptype_);
    Packet::free(pkt);
    return;
  }

#ifdef PGM_DEBUG
  display_packet(pkt);
#endif

  // Note, each handle function will free the packet itself or modify the
  // headers and pass it on to the next NS object.

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
    handle_nak(pkt);
    break;
  case PGM_NCF:
    handle_ncf(pkt);
    break;
  default:
    printf("ERROR (PgmAgent::recv): Received PGM packet with unknown type %d.\n", hp->type_);

    Packet::free(pkt);

    break;
  }

}

// Code to execute when a timeout occurs.
void PgmAgent::timeout(int type, void *data)
{

  switch(type) {
  case TIMER_NAK_RETRANS:
    timeout_nak_retrans((RepairState *) data);
    break;
  case TIMER_NAK_RPT:
    timeout_nak_rpt((RepairState *) data);
    break;
  case TIMER_NAK_RDATA:
    timeout_nak_rdata((RepairState *) data);
    break;
  case TIMER_NAK_ELIM:
    timeout_nak_elim((RepairState *) data);
    break;
  default:
    printf("ERROR (PgmAgent::timeout()): Invalid timeout type %d.\n", type);
    break;
  }

}

// Code to execute when a TCL command is issued to the PGM Agent object.
int PgmAgent::command (int argc, const char*const* argv)
{
  //  Tcl& tcl = Tcl::instance();

  if (argc == 2) {
    if (strcmp(argv[1], "print-stats") == 0) {
      print_stats();
      return (TCL_OK);
    }
  }
  else if (argc == 3) { //Set the Event Trace handle if Event Tracing is on
    if (strcmp(argv[1], "eventtrace") == 0) {
      et_ = (EventTrace *)TclObject::lookup(argv[2]);
      return (TCL_OK);
    }
  }

  return (Agent::command(argc, argv));
}      

void PgmAgent::trace_event(char *evType, double evTime) {

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

void PgmAgent::handle_spm(Packet *pkt)
{

  hdr_pgm *hp = HDR_PGM(pkt);
  hdr_pgm_spm *hs = HDR_PGM_SPM(pkt);
  hdr_cmn *hc = HDR_CMN(pkt);

  // Use the TSI from the SPM packet and locate the proper state block.
  StateInfo *state = find_TSI(hp->tsi_);

  if (state == NULL) {
    // There is no state block for this TSI. Create new state.
    state = insert_TSI(hp->tsi_);

    // Set the sequence number.
    state->spm_seqno() = hp->seqno_;
    // Set the upstream path.
    state->upstream_node() = hs->spm_path_;
    state->upstream_iface() = hc->iface();
  }
  else {
    // State already exists for this TSI. Check if the sequence number is
    // newer than the last recorded sequence number.

    if ( state->spm_seqno() < hp->seqno_ ) {
      // Update the SPM sequence number.
      state->spm_seqno() = hp->seqno_;
      // Set the upstream path.
      state->upstream_node() = hs->spm_path_;
      state->upstream_iface() = hc->iface();
    }
    else {
      printf("%s received an old SPM seqno, discarding.\n", uname_);
      Packet::free(pkt);
      return;
    }
  }

  // Modify the SPM packet and set the upstream path to be equal to
  // the address of this agent.
  hs->spm_path_ = here_;

  // Send the modified packet off to the rest of the multicast group.
  send(pkt, 0);
}

void PgmAgent::handle_odata(Packet *pkt)
{
  // Pass the ODATA along to the rest of the multicast group.  ODATA
  // does not cancel NAK forwarding.

  //hdr_cmn *hc = HDR_CMN(pkt);

  send(pkt, 0);
}

void PgmAgent::handle_rdata(Packet *pkt)
{

  // Look for the TSI for this RDATA packet.
  hdr_pgm *hp = HDR_PGM(pkt);
  //  hdr_ip *hip = HDR_IP(pkt);

  StateInfo *state = find_TSI(hp->tsi_);
  if (state == NULL) {
    printf("%s received RDATA for which no SPM state is established, discarding.\n", uname_);
    stats_.num_unsolicited_rdata_++;
    Packet::free(pkt);
    return;
  }

  // Look for the repair state for this RDATA packet.
  map<int, RepairState>::iterator result = state->repair().find(hp->seqno_);

  if (result == state->repair().end()) {
    // No repair state present for this RDATA packet.
    printf("%s received RDATA for which no repair state is present, discarding.\n", uname_);
    stats_.num_unsolicited_rdata_++;
    Packet::free(pkt);
    return;
  }

  RepairState *rstate = &((*result).second);

  // Get the interface list for the repair state. For each interface, send
  // out the RDATA packet.  Similarly for each agent that is also receiving
  // RDATA attached to this node.

  if (rstate->iface_list().empty() && rstate->agent_list().empty()) {
    printf("%s received RDATA but repair state has no interfaces for it, discarding.\n", uname_);

    stats_.num_unsolicited_rdata_++;

    Packet::free(pkt);
  }

  NsObject *tgt;
  Packet *new_pkt;
  int flag = 0;

  trace_event("SEND RDATA", 0); //Repair is being forwarded

  //  hdr_cmn *hc = HDR_CMN(pkt);

  if (!rstate->iface_list().empty()) {
    list<int>::iterator iter = rstate->iface_list().begin();

    while (iter != rstate->iface_list().end()) {
      if (!flag) {
	tgt = iface2link(*iter);
	if (tgt == NULL) {
	  printf("ERROR (PgmAgent::handle_rdata): iface2link returned NULL.\n");
	  abort();
	}
	tgt->recv(pkt);
	flag = 1;
      }
      else {
	// Make a copy of each packet before sending it, so we don't free()
	// the same packet at the different receivers causing a deallocation
	// problem.
	new_pkt = pkt->copy();
	tgt = iface2link(*iter);
	if (tgt == NULL) {
	  printf("ERROR (PgmAgent::handle_rdata): iface2link returned NULL.\n");
	  abort();
	}
	tgt->recv(new_pkt);
      }

      iter++;
    }
  }

  if (!rstate->agent_list().empty()) {
    list<NsObject *>::iterator iter = rstate->agent_list().begin();

    while (iter != rstate->agent_list().end()) {
      if (!flag) {
	(*iter)->recv(pkt);
	flag = 1;
      }
      else {
	// Make a copy of each packet before sending it, so we don't free()
	// the same packet at the different receivers causing a deallocation
	// problem.
	new_pkt = pkt->copy();
	(*iter)->recv(new_pkt);
      }

      iter++;
    }
  }

  // Remove the repair state for this RDATA sequence number, since we sent
  // out the repairs.
  remove_repair_state(&((*result).second));

}

void PgmAgent::handle_nak(Packet *pkt)
{

  hdr_pgm *hp = HDR_PGM(pkt);
  hdr_pgm_nak *hpn = HDR_PGM_NAK(pkt);
  hdr_cmn *hc = HDR_CMN(pkt);
  //  hdr_ip *hip = HDR_IP(pkt);

  // Check to see if there is a state control block for the given TSI.
  StateInfo *state = find_TSI(hp->tsi_);

  if (state == NULL) {
    printf("PGM Agent received NAK for which no SPM state is established, discarding.\n");
    Packet::free(pkt);
    return;
  }

  // Create an NCF packet in response to the NAK packet.
  Packet *ncf_pkt = allocpkt();
  hdr_cmn *ncf_hc = HDR_CMN(ncf_pkt);
  ncf_hc->size_ = sizeof(hdr_pgm); // Size of NCF packet.
  ncf_hc->ptype_ = PT_PGM;
  hdr_pgm *ncf_hp = HDR_PGM(ncf_pkt);
  ncf_hp->type_ = PGM_NCF;
  ncf_hp->tsi_ = hp->tsi_;
  ncf_hp->seqno_ = hp->seqno_;

  // Change the source of the NCF packet to be the original ODATA source.
  hdr_ip *ncf_ip = HDR_IP(ncf_pkt);
  ncf_ip->src() = hpn->source_;
  // Set the destination to be the multicast group.
  ncf_ip->dst() = hpn->group_;

  // Set the color of NCF packets in nam to be green.
  ncf_ip->fid_ = 6;

  // Send out the NCF to the interface (or agent) for which the NAK was
  // received.
  NsObject *tgt;

  if (hc->iface() < 0) {
    tgt = pkt2agent(pkt);
    if (tgt == NULL) {
      printf("ERROR: (PgmAgent::handle_nak) pkt2agent returned NULL.\n");
      abort();
    }
    tgt->recv(ncf_pkt);
  }
  else {
    tgt = iface2link(hc->iface());
    if (tgt == NULL) {
      printf("ERROR: (PgmAgent::handle_nak) iface2link returned NULL.\n");
      abort();
    }
    tgt->recv(ncf_pkt);
  }

  // Create repair state for the NAK query. Associate the sequence number
  // of the NAK packet with the interface where the packet was received.
  pair<map<int, RepairState>::iterator, bool> result;

  result = state->repair().insert(pair<int, RepairState>(hp->seqno_, RepairState(this, state, hp->seqno_, hpn->source_, hpn->group_)));

  RepairState *rstate = &(result.first->second);

  if (result.second == true) {
    // There was no previous repair state for the given NAK seqno.
    // This must be a new NAK.

    // Set the data fields of the timer.
    rstate->nak_retrans_timer().data() = rstate;
    rstate->nak_rpt_timer().data() = rstate;
    rstate->nak_rdata_timer().data() = rstate;
    rstate->nak_elim_timer().data() = rstate;

    // Add the interface (or agent) to the interface list.
    if (hc->iface() < 0) {
      rstate->agent_list().push_back(pkt2agent(pkt));
    }
    else {
      rstate->iface_list().push_back(hc->iface());
    }

    // Start the nak retransmission cycle time.
    rstate->nak_retrans_timer().resched(nak_retrans_ival_);

    // Set the nak repeat interval.
    rstate->nak_rpt_timer().resched(nak_rpt_ival_);

    trace_event("SEND NACK", nak_rpt_ival_); //Nack being Sent, Nack will refire after ival

    // Don't set the RDATA timer until the NCF is received.
    // Don't set the elimintation timer until the NCF is received.

    // We're now in the NAK_PENDING state.
  }
  else {
    // There was previous repair state for the given NAK seqno.

    if (hc->iface() < 0) {
      // Scan the agent list to see if the agent is in the list already
      // for this repair state.
      list<NsObject *> *agent_list = &(rstate->agent_list());
      list<NsObject *>::iterator res = find(agent_list->begin(), agent_list->end(), pkt2agent(pkt));

      if (res == agent_list->end()) {
	agent_list->push_back(pkt2agent(pkt));
      }

    }
    else {

      // Scan the interface list to see if the interface is in the list
      // already for this repair state.
      list<int> *iface_list = &(rstate->iface_list());
      list<int>::iterator res = find(iface_list->begin(), iface_list->end(), hc->iface());

      if (res == iface_list->end()) {
	// Interface not found in iface list for this NAK, add it.
	iface_list->push_back(hc->iface());
      }
    }

    // If the NAK elimination timer has expired, then we are allowed to
    // send another NAK to our upstream.
    if (rstate->nak_elimination() == false) {
      rstate->nak_state() = NAK_PENDING;
      // Start the nak retransmission cycle time.
      rstate->nak_retrans_timer().resched(nak_retrans_ival_);
      rstate->nak_rpt_timer().resched(nak_rpt_ival_);

      // Disable the rdata and elim timer if they were previously running.
      rstate->nak_rdata_timer().force_cancel();
      rstate->nak_elim_timer().force_cancel();

      rstate->nak_elimination() = true;

#ifdef PGM_DEBUG
      printf("%s: Got NAK for seqno %d with previous NAK state, accepted.\n",
	     uname_, hp->seqno_);
#endif
    }
    else {
      // NAK elimination requires us to not act on this duplicate NAK packet.
#ifdef PGM_DEBUG
      printf("%s: Got NAK for seqno %d but have previous NAK state, discarding.\n", uname_, hp->seqno_);
#endif
      stats_.num_suppressed_naks_++;
      Packet::free(pkt);
      return;
    }

  }

  stats_.num_naks_transmitted_++;

  // Send the NAK packet to our upstream
  send_nak(state->upstream_node(), hp->tsi_, hp->seqno_, hpn->source_, hpn->group_);

  Packet::free(pkt);
}

void PgmAgent::handle_ncf(Packet *pkt)
{

  hdr_pgm *hp = HDR_PGM(pkt);
  hdr_cmn *hc = HDR_CMN(pkt);
  hdr_ip *hip = HDR_IP(pkt);

  // Locate the state control block for this TSI.
  StateInfo *state = find_TSI(hp->tsi_);

  if (state == NULL) {
    printf("%s received NCF for which no SPM state is established, discarding.\n", uname_);
    stats_.num_unsolicited_ncf_++;
    Packet::free(pkt);
    return;
  }

  if (hc->iface() != state->upstream_iface()) {
    printf("%s received NCF from non-upstream interface, discarding.\n", uname_);
    stats_.num_unsolicited_ncf_++;
    Packet::free(pkt);
    return;
  }

  trace_event("SEND NCF", 0);

  // Look for the repair state for this NCF packet.
  map<int, RepairState>::iterator result = state->repair().find(hp->seqno_);
  RepairState *rstate;

  if (result == state->repair().end()) {
    // No repair state present for this NCF packet.

    // Since the interface for this NCF packet comes from the same interface
    // used to reach our upstream node, we can create empty repair state.
    // This is NAK Anticipation (see 7.5 in PGM specification).

    pair<map<int, RepairState>::iterator, bool> res;
    res = state->repair().insert(pair<int, RepairState>(hp->seqno_, RepairState(this, state, hp->seqno_, hip->src(), hip->dst())));

    rstate = &(res.first->second);

    // Set the data field of the timers.
    rstate->nak_retrans_timer().data() = rstate;
    rstate->nak_rpt_timer().data() = rstate;
    rstate->nak_rdata_timer().data() = rstate;
    rstate->nak_elim_timer().data() = rstate;

    stats_.num_unsolicited_ncf_++;
  }
  else {
    rstate = &((*result).second);

    // Disable either of the retransmission or repeat interval timers since
    // the NAK is confirmed.
    rstate->nak_retrans_timer().force_cancel();
    rstate->nak_rpt_timer().force_cancel();
  }

  rstate->nak_state() = NAK_CONFIRMED;
  // Set/reset the rdata and elim timer to expire at the appropriate time.
  rstate->nak_rdata_timer().resched(nak_rdata_ival_);
  rstate->nak_elim_timer().resched(nak_elim_ival_);

  Packet::free(pkt);
}

// Create and send a nak packet to the upstream path.
void PgmAgent::send_nak(ns_addr_t &upstream_node, ns_addr_t &tsi, int seqno, ns_addr_t &source, ns_addr_t &group)
{

#ifdef PGM_DEBUG
  double now = Scheduler::instance().clock();
  printf("at %f %s sending NAK for seqno %d upstream.\n", now, uname_, seqno);
#endif

  Packet *nak_pkt = allocpkt();
  // Set the simulated size of the NAK packet.
  hdr_cmn *nak_hc = HDR_CMN(nak_pkt);
  nak_hc->size_ = sizeof(hdr_pgm) + sizeof(hdr_pgm_nak);
  nak_hc->ptype_ = PT_PGM;

  // Set the destination address to be our upstream node.
  hdr_ip *nak_hip = HDR_IP(nak_pkt);
  nak_hip->dst() = upstream_node;

  // Set the color of NAK packet to be black in nam.
  nak_hip->fid_ = 8;

  // Fill in the PGM header for the NAK packet.
  hdr_pgm *nak_hp = HDR_PGM(nak_pkt);
  nak_hp->type_ = PGM_NAK;
  nak_hp->tsi_ = tsi;
  nak_hp->seqno_ = seqno;

  // Fill in the PGM NAK header for the NAK packet.
  hdr_pgm_nak *nak_hpn = HDR_PGM_NAK(nak_pkt);
  nak_hpn->source_ = source;
  nak_hpn->group_ = group;

  // Send out the packet.
  send(nak_pkt, 0);
}

// Code that is executed when the nak retransmission timer expires.
void PgmAgent::timeout_nak_retrans(RepairState *rstate)
{

  stats_.num_naks_transmitted_++;

  // Send out a new NAK packet.
  send_nak(rstate->sinfo()->upstream_node(), rstate->sinfo()->tsi(), rstate->seqno(), rstate->source(), rstate->group());

  // Reset the retransmission timer.
  rstate->nak_retrans_timer().resched(nak_retrans_ival_);

}

// Code that is executed when a repair state NAK RPT timer expires.
void PgmAgent::timeout_nak_rpt(RepairState *rstate)
{

  printf("%s: timeout_nak_rpt expired, removing repair state.\n", uname_);

  // We never got a confirmation for our NAK packet. We must now
  // remove the repair state entirely.
  remove_repair_state(rstate);

}

// Code that is executed when a repair state NAK RDATA timer expires.
void PgmAgent::timeout_nak_rdata(RepairState *rstate)
{

  printf("%s: timeout_nak_rdata expired, removing repair state.\n", uname_);

  // We never got the RDATA for our NAK. We must now remove the repair
  // state entirely.
  remove_repair_state(rstate);

}

// Code that is executed when a repair state NAK elimination timer expires.
void PgmAgent::timeout_nak_elim(RepairState *rstate)
{

  // Allow one duplicate NAK to come in to be processed and forwarded.
  rstate->nak_elimination() = false;

}

void PgmAgent::remove_repair_state(RepairState *rstate)
{

  // Cancel all timers.
  rstate->nak_retrans_timer().force_cancel();
  rstate->nak_rpt_timer().force_cancel();
  rstate->nak_rdata_timer().force_cancel();
  rstate->nak_elim_timer().force_cancel();

  // Erase the repair state from the TSI repair map.
  if (!rstate->sinfo()->repair().erase(rstate->seqno())) {
    printf("ERROR (PgmAgent::remove_repair_state): Did not erase seqno from map.\n");
  }

}

NsObject* PgmAgent::iface2link (int iface)
{
//      Tcl::instance().evalf("%s get-outlink %d", name(), iface);
//      char* ni = Tcl::instance().result();
        Tcl&    tcl = Tcl::instance();
        char    wrk[64];

	if (iface == -1) {
	  return NULL;
	}

        sprintf (wrk, "[%s set node_] ifaceGetOutLink %d", name (), iface);
        tcl.evalc (wrk);
        const char* result = tcl.result ();
#ifdef PGM_DEBUG
printf ("[iface2link] agent %s\n", result);
#endif
        NsObject* obj = (NsObject*)TclObject::lookup(result);
        return (obj);
}

NsObject* PgmAgent::pkt2agent (Packet *pkt)
{
        Tcl&            tcl = Tcl::instance();
        char            wrk[64];
        const char     *result;
        int             port;
        NsObject*       agent;
        hdr_ip*         ih = HDR_IP(pkt);
        //nsaddr_t        src = ih->saddr();

        port = ih->sport();

        sprintf (wrk, "[%s set node_] agent %d", name (), port);
        tcl.evalc (wrk);
        result = tcl.result ();

#ifdef PGM_DEBUG
printf ("[pkt2agent] port %d, agent %s\n", port, result);
#endif

        agent = (NsObject*)TclObject::lookup (result);
        return (agent);
}

// Find the state control block given a TSI. Returns NULL if not found.
StateInfo * PgmAgent::find_TSI(ns_addr_t &tsi)
{

  // Use the TSI from the SPM packet and locate the proper state block.
  list<StateInfo>::iterator iter = state_list_.begin();
  while(iter != state_list_.end()) {
    if ( (*iter).tsi().isEqual (tsi) ) {
      return &(*iter);
    }
    iter++;
  }

  return NULL;
}

// Insert a new state control block for the given TSI, and return a pointer
// to the control block.
StateInfo * PgmAgent::insert_TSI(ns_addr_t &tsi)
{
  state_list_.push_back(StateInfo(tsi));

  return &(state_list_.back());
}

void PgmAgent::print_stats()
{
  printf("%s:\n", uname_);
  printf("\tNAKs Transmitted: \t%d\n", stats_.num_naks_transmitted_);
  printf("\tNAKs Suppressed: \t%d\n", stats_.num_suppressed_naks_);
  printf("\tUnsolicited NCFs: \t%d\n", stats_.num_unsolicited_ncf_);
  printf("\tUnsolicited RDATA: \t%d\n", stats_.num_unsolicited_rdata_);
}

#ifdef PGM_DEBUG
void PgmAgent::display_packet(Packet *pkt)
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
