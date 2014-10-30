
/*
 * pgm-sender.cc
 * Copyright (C) 2001 by the University of Southern California
 * $Id: pgm-sender.cc,v 1.13 2010/03/08 05:54:52 tom_henderson Exp $
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
 * pgm-sender.cc
 *
 * This implements the Sending PGM agent, "Agent/PGM/Sender".
 *
 * Ryan S. Barnett, 2001
 * rbarnett@catarina.usc.edu
 */

#include "config.h"
#ifdef HAVE_STL

#include <stdlib.h>
#include <stdio.h>

/* Standard Template Library */
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
// Define the PGM Sender Timer Class
// ************************************************************
class PgmSender;

/* Define the different timer types. */
enum {
  TIMER_SPM = 0,
  TIMER_RDATA = 1
};

class PgmSenderTimer : public TimerHandler {
public:
  PgmSenderTimer(PgmSender *a, int type) : TimerHandler(), data_(NULL) {
    a_ = a;
    type_ = type;
  }

  int & type() { return type_; }
  void * &data() { return data_; }

protected:
  virtual void expire(Event *e);
  PgmSender *a_;
  int type_;
  void *data_;
};

// Bundles an RDATA packet with a timer (for sending the RDATA packet),
// and a list of interfaces for which the packet should be sent to.
class RdataItem {
public:
  RdataItem(int seqno, PgmSender *a, Packet *rdata_pkt = NULL) :
    seqno_(seqno), rdata_pkt_(rdata_pkt), rdata_timer_(a, TIMER_RDATA) { }

  // Allow direct access to the private data.
  int & seqno() { return seqno_; }
  Packet * & rdata_pkt() { return rdata_pkt_; }
  PgmSenderTimer & rdata_timer() { return rdata_timer_; }
  list<int> & iface_list() { return iface_list_; }
  list<NsObject *> & agent_list() { return agent_list_; }

protected:
  // The sequence number of this RDATA item.
  int seqno_;
  // The RDATA packet itself.
  Packet *rdata_pkt_;
  // The timer responsible for sending out this RDATA packet.
  PgmSenderTimer rdata_timer_;
  // The list of interfaces for which this packet must be sent to.
  list<int> iface_list_;
  // The list of agents for which this packet must be sent to.
  list<NsObject *> agent_list_;
};

// A class used to keep track of duplicate reply requests.
class ReplyItem {
public:
  ReplyItem(int seqno) : seqno_(seqno), retransmissions_(0) { }

  int & seqno() { return seqno_; }
  int & retransmissions() { return retransmissions_; }

protected:
  int seqno_;
  int retransmissions_;
};

// Largest size we will allow the reply list to grow.
const int MAX_REPLY_LIST_SIZE = 100;

// Miscellaneous statistical information gathered during simulation.
struct Stats {
  int num_naks_received_;
  int num_rdata_sent_;
  int max_num_repeated_rdata_;
};

// ************************************************************
// Define the PGM Sender Class
// ************************************************************
static int pgm_snd_uid_ = 0;

class PgmSender: public Agent {
public:
  PgmSender();

  virtual void recv(Packet *, Handler *);
  virtual void timeout(int type, void *data);
  virtual int command(int argc, const char*const* argv);
  virtual void sendmsg(int nbytes, const char *flags = 0);

protected:

  virtual void start(); // Starts the SPM heartbeats.
  virtual void stop(); // Stops the SPM heartbeats.

  virtual void handle_nak(Packet *pkt); // Process a NAK packet.

  virtual void send_spm(); // Sends an SPM packet to the multicast group.

  virtual void send_rdata(RdataItem *pkt); // Sends the given RDATA packet.

  NsObject* iface2link(int iface);
  NsObject* pkt2agent(Packet *pkt);

  void print_stats();

  void display_packet(Packet *pkt); // For debugging.

  void trace_event(char *evType, nsaddr_t daddr, double evTime); 

  EventTrace * et_; 	//Trace Object for custom Event Traces

  Stats stats_; // Keep track of various statistics.

  char uname_[16]; // Unique PGM sender name.

  // Map the sequence number of a NAK (requested RDATA) with an item
  // that represents the RDATA packet including which interfaces the
  // RDATA should be sent to, along with a timer that is used to trigger
  // sending of the RDATA packet.
  map<int, RdataItem> pending_rdata_;

  // A list to keep track of the number of retransmissions for a given
  // RDATA reply.  The max size this will grow is MAX_REPLY_LIST_SIZE.
  list<ReplyItem> reply_;

  // The 'typicial' size of a data packet including header. This should
  // get set automatically from the application calling sendmsg().  We
  // make a simplifying assumption that all packet sizes in a session are
  // of equal size.
  int pktSize_;

  PgmSenderTimer spm_heartbeat_; // Timer for sending out SPM packets.
  int spm_running_; // Whether the heartbeats are running or not.
  double spm_interval_; // Time between SPM packets (in seconds).
  // Time to delay sending out an RDATA in response to a NAK packet, this
  // is to allow slow NAKs to get processed at one time, so we don't send
  // out duplicate RDATA.
  double rdata_delay_;

  int odata_seqno_; // Current ODATA sequence number.
  int spm_seqno_; // Current SPM sequence number.

  //  nsaddr_t group_; // The multicast group we send to.

};

void PgmSenderTimer::expire(Event *) {
  a_->timeout(type_, data_);
}

static class PgmSenderClass : public TclClass {
public:
  PgmSenderClass() : TclClass("Agent/PGM/Sender") {}
  TclObject * create(int , const char * const * ) {
    return (new PgmSender());
  }
} class_pgm_sender;

// Constructor.
PgmSender::PgmSender() : Agent(PT_PGM), pktSize_(0),
			 spm_heartbeat_(this, TIMER_SPM),
			 spm_running_(0), odata_seqno_(-1), spm_seqno_(-1)
{
  stats_.num_naks_received_ = 0;
  stats_.num_rdata_sent_ = 0;
  stats_.max_num_repeated_rdata_ = 0;

  sprintf(uname_, "pgmSender-%d", pgm_snd_uid_++);

  bind_time("spm_interval_", &spm_interval_);
  bind_time("rdata_delay_", &rdata_delay_);

  et_ = (EventTrace *) NULL;

}

// Code that is called when a packet is received.
void PgmSender::recv(Packet *pkt, Handler *)
{
  hdr_cmn* hc = HDR_CMN(pkt);

  if (hc->ptype_ == PT_PGM) {

#ifdef PGM_DEBUG
    display_packet(pkt);
#endif

    // Identify the type of PGM packet, if it is a NAK process it, otherwise
    // throw an error.
    hdr_pgm *hp = HDR_PGM(pkt);

    if (hp->type_ == PGM_NAK) {
      handle_nak(pkt);
    }
    else {
      printf("ERROR (PgmSender::handle_pgm_pkt): received unexpected PGM packet type %d, discarding.\n", hp->type_);
    }
  }
  else {
    printf ("%s ERROR (PgmSender::recv): received non PGM pkt type %d, discarding.\n", uname_, hc->ptype_);
  }

  // Free all packets that this agent receives.
  Packet::free(pkt);
}

// Code that is called when a timer expires.
void PgmSender::timeout(int type, void *data)
{
  switch(type) {
  case TIMER_SPM:
    if (spm_running_) {
      send_spm();
      spm_heartbeat_.resched(spm_interval_);
    }
    break;
  case TIMER_RDATA:
    send_rdata((RdataItem *)data);
    break;
  default:
    printf("ERROR (PgmSender::timeout): invalid timeout type.\n");
    break;
  }

}

// Code that is called when a TCL command is issued to the PGM Sender object.
int PgmSender::command(int argc, const char*const* argv)
{
  if (argc == 2) {
    if (strcmp(argv[1], "start-SPM") == 0) {
      start();
      return (TCL_OK);
    }
    if (strcmp(argv[1], "stop-SPM") == 0) {
      stop();
      return (TCL_OK);
    }
    if (strcmp(argv[1], "print-stats") == 0) {
      print_stats();
      return (TCL_OK);
    }
  }
  else if (argc == 3) { 	//If Event Trace is on, set the Event trace handle
    if (strcmp(argv[1], "eventtrace") == 0) {
      et_ = (EventTrace *)TclObject::lookup(argv[2]);
      return (TCL_OK);
    }
  }

  return (Agent::command(argc, argv));
}

void PgmSender::trace_event(char *evType, nsaddr_t daddr, double evTime) {

  if (et_ == NULL) return;
  char *wrk = et_->buffer();
  char *nwrk = et_->nbuffer();

  if (wrk != NULL) {
    sprintf(wrk, "E "TIME_FORMAT" %d %d PGM %s "TIME_FORMAT, 
            et_->round(Scheduler::instance().clock()),   
            addr(),                    
            daddr,                   
            evType,                  
			evTime);	
  if (nwrk != 0)
    sprintf(nwrk,
			"E -t "TIME_FORMAT" -o PGM -e %s -s %d.%d -d %d.%d",
			et_->round(Scheduler::instance().clock()),   // time
			evType,                    // event type
			addr(),                       // owner (src) node id
			port(),                       // owner (src) port id
			daddr,                      // dst node id
			0                       // dst port id
			);
	et_->dump();
  }

}

// The application calls this function to send out new ODATA (original DATA).
void PgmSender::sendmsg(int nbytes, const char * /*flags  = 0 */)
{
  odata_seqno_++;

#ifdef PGM_DEBUG
  double now = Scheduler::instance().clock();

  printf("at %f %s sending ODATA seqno %d\n", now, uname_, odata_seqno_);
#endif

  // Create a packet with the given ODATA.
  Packet *pkt = allocpkt();
  // Set the simulated size of the packet to the indicated nbytes.
  hdr_cmn *hc = HDR_CMN(pkt);
  pktSize_ = nbytes + sizeof(hdr_pgm);
  hc->size_ = pktSize_;

  hc->ptype_ = PT_PGM;

  // Fill in the PGM header.
  hdr_pgm *hp = HDR_PGM(pkt);
  hp->type_ = PGM_ODATA;
  hp->tsi_ = here_; // Set transport session ID to addr/port of this agent.
  hp->seqno_ = odata_seqno_;

  hdr_ip *hip = HDR_IP(pkt);
  // Set the color for ODATA packets.
  hip->fid_ = 1;

  // Send out the packet.
  send(pkt, 0);
}

void PgmSender::start()
{
  spm_running_ = 1;
  send_spm();
  spm_heartbeat_.resched(spm_interval_);
}

void PgmSender::stop()
{
  spm_heartbeat_.cancel();
  spm_running_ = 0;
  Tcl::instance().evalf("%s done", this->name());
}

// Process a NAK packet.
void PgmSender::handle_nak(Packet *pkt)
{

  hdr_cmn *hc = HDR_CMN(pkt);
  hdr_pgm *hp = HDR_PGM(pkt);
  //hdr_pgm_nak *pnak = HDR_PGM_NAK(pkt);

  if (!(hp->tsi_.isEqual (here_))) {
    printf("%s received NAK with wrong TSI, discarding.\n", uname_);
    return;
  }

  stats_.num_naks_received_++;

  // Create the NCF packet.
  Packet *ncf_pkt = allocpkt();
  // Set the simulated size of the NCF packet.
  hdr_cmn *ncf_hc = HDR_CMN(ncf_pkt);
  ncf_hc->size_ = sizeof(hdr_pgm);
  ncf_hc->ptype_ = PT_PGM;
  // Fill in the PGM header for the NCF packet.
  hdr_pgm *ncf_hp = HDR_PGM(ncf_pkt);
  ncf_hp->type_ = PGM_NCF;
  ncf_hp->tsi_ = here_;
  ncf_hp->seqno_ = hp->seqno_;

  hdr_ip *ncf_hip = HDR_IP(ncf_pkt);
  // Set the color for NCF packets in nam.
  ncf_hip->fid_ = 6;

  // Immediately send the NCF packet to the interface where the NAK
  // packet was received. If the packet came from another agent attached
  // to this node, then send the packet to that agent.
  NsObject *tgt;

  if (hc->iface() < 0) {
    tgt = pkt2agent(pkt);
  }
  else {
    tgt = iface2link(hc->iface());
  }

  if (tgt == NULL) {
    printf("ERROR (PgmSender::handle_nak): iface2link returned NULL.\n");
    abort();
  }
  tgt->recv(ncf_pkt);

  // Queue up an RDATA packet to be transferred to the requestor on the
  // appropriate interface.

  // Attempt to locate this NAK sequence number on the pending RDATA map.
  pair<map<int, RdataItem>::iterator, bool> result;

  result = pending_rdata_.insert(pair<int, RdataItem>(hp->seqno_, RdataItem(hp->seqno_, this)));

  RdataItem *ritem = &(result.first->second);

  if (result.second == true) {
    // The entry was added to the map.

    // Set the data field of the timer.
    ritem->rdata_timer().data() = ritem;

    // There is NO pending RDATA for the indicated sequence number.

    // Create the RDATA packet.
    Packet *rdata_pkt = allocpkt();
    // Set the simulated size of the RDATA packet to the typicial data size.
    hdr_cmn *rdata_hc = HDR_CMN(rdata_pkt);
    rdata_hc->size_ = pktSize_;
    rdata_hc->ptype_ = PT_PGM;
    // Fill in the PGM header for RDATA packet.
    hdr_pgm *rdata_hp = HDR_PGM(rdata_pkt);
    rdata_hp->type_ = PGM_RDATA;
    rdata_hp->tsi_ = here_; // Set transport session ID to addr/port of this agent.
    rdata_hp->seqno_ = hp->seqno_;

    hdr_ip *rdata_hip = HDR_IP(rdata_pkt);
    // Set the color for RDATA packets in nam.
    rdata_hip->fid_ = 3;

    // Place the new packet into the RdataItem in the map.
    ritem->rdata_pkt() = rdata_pkt;

    // Set the timer to go off at rdata_delay_ seconds from now.
    ritem->rdata_timer().resched(rdata_delay_);

	//Output Event Trace, Repair will be sent after rdata_delay_
    trace_event("REPAIR BACKOFF", rdata_hip->daddr(), rdata_delay_);

    if (hc->iface() < 0) {
      // The NAK was sent from a local agent attached to this node. Keep
      // track of which agent this is.
      ritem->agent_list().push_back(pkt2agent(pkt));
    }
    else {
      // Set the interface number for this RdataItem.
      ritem->iface_list().push_back(hc->iface());
    }
  }
  else {
    // Seqno entry already exists in the map.

    // The RDATA is already pending.

    if (hc->iface() < 0) {
      // Scan the agent list to see if the agent is already registered
      // for this RDATA.

      list<NsObject *> *agent_list = &(ritem->agent_list());

      list<NsObject *>::iterator res = find(agent_list->begin(), agent_list->end(), pkt2agent(pkt));

      if (res == agent_list->end()) {
	// Agent not found in agent list for this RDATA, add it.
	agent_list->push_back(pkt2agent(pkt));
      }
      else {
	printf("%s: NAK received and already had NAK state for that same agent.\n", uname_);
      }

    }
    else {
      // Scan the interface list to see if the interface is already registered
      // for this RDATA.
      list<int> *iface_list = &(ritem->iface_list());
      list<int>::iterator res = find(iface_list->begin(), iface_list->end(), hc->iface());

      if (res == iface_list->end()) {
	// Interface not found in iface list for this RDATA, add it.
	iface_list->push_back(hc->iface());
      }
      else {
	// Interface already present in the iface list for this RDATA,
	// therefore this NAK is a duplicate.
	printf("%s: NAK received and already had NAK state for that same interface.\n", uname_);
      }
    }

  }

}

// Send out a new SPM packet to the multicast group.
void PgmSender::send_spm()
{
  spm_seqno_++;

  // Create a packet with the given ODATA.
  Packet *pkt = allocpkt();
  // Set the simulated size of the packet to the indicated nbytes.
  hdr_cmn *hc = HDR_CMN(pkt);
  hc->size_ = sizeof(hdr_pgm) + sizeof(hdr_pgm_spm);

  hc->ptype_ = PT_PGM;

  hdr_ip *hip = HDR_IP(pkt);
  // Set the color for SPM packets in nam.
  hip->fid_ = 7;
  //  hip->daddr() = group_;

  // Fill in the PGM header.
  hdr_pgm *hp = HDR_PGM(pkt);
  hp->type_ = PGM_SPM;
  hp->tsi_ = here_; // Set transport session ID to addr/port of this agent.
  hp->seqno_ = spm_seqno_;

  // Fill in SPM header.
  hdr_pgm_spm *hs = HDR_PGM_SPM(pkt);
  hs->spm_path_ = here_; // Set current path to the source agent.

#ifdef PGM_DEBUG
  double now = Scheduler::instance().clock();
  printf("at %f %s sending SPM, from %d:%d (here = %d:%d) to %d:%d, TSI %d:%d, type %d\n", now, uname_, hip->saddr(), hip->sport(), addr(), port(), hip->daddr(), hip->dport(), hp->tsi_.addr_, hp->tsi_.port_, hp->type_);
#endif

  // Send out the packet.
  send(pkt, 0);
}

// Send out the given RDATA packet. The packet should be already created
// and headers filled in.  This is triggered when the timer expires.
void PgmSender::send_rdata(RdataItem *item)
{

  // Locate the sequence number of this rdata in the list of sent
  // retransmissions.
  int count = 0;
  list<ReplyItem>::iterator iter = reply_.begin();
  while (iter != reply_.end()) {
    if ((*iter).seqno() == item->seqno()) {
      (*iter).retransmissions() += 1;

      if ((*iter).retransmissions() > stats_.max_num_repeated_rdata_) {
	stats_.max_num_repeated_rdata_ = (*iter).retransmissions();
      }

      break;
    }

    count++;
    iter++;
  }

  if (iter == reply_.end()) {
    // This is the first time we're sending out this RDATA. Append it to
    // the back of the reply list.
    if (count >= MAX_REPLY_LIST_SIZE) {
      // Pop off the front-most element if we've reached the max size of
      // the reply list.
      reply_.pop_front();
    }

    reply_.push_back(ReplyItem(item->seqno()));
  }

  stats_.num_rdata_sent_++;

  // Send the packet to each of the interfaces.
  if (!item->iface_list().empty()) {
    list<int>::iterator iter = item->iface_list().begin();
    int flag = 0;   // Used to determine when we need to make additional copies of the packet.
    while (iter != item->iface_list().end()) {
      NsObject *tgt;
      tgt = iface2link(*iter);
      Packet *pkt = item->rdata_pkt();
      if (flag) {
	// Make a copy of each packet before sending it, so we don't free()
	// the same packet at the different receivers causing a deallocation
	// problem.
	pkt = pkt->copy();
      } else {
	trace_event("SEND RDATA", HDR_IP(pkt)->daddr(), 0);
	flag = 1;
      }
      tgt->recv(pkt);

      iter++;
    }

  }

  if (!item->agent_list().empty()) {
    list<NsObject *>::iterator iter = item->agent_list().begin();
    int flag = 0;
    while (iter != item->agent_list().end()) {
      Packet *pkt = item->rdata_pkt();
      if (flag) {
	pkt = pkt->copy ();
      } else {
	flag = 1;
      }
      (*iter)->recv(pkt);

      iter++;
    }
  }

  hdr_pgm *hp = HDR_PGM(item->rdata_pkt());

  // Remove this sequence number from the pending RDATA list, since
  // we have now sent that RDATA.
  if (!pending_rdata_.erase(hp->seqno_)) {
    printf("ERROR (PgmSender::send_rdata): Did not erase RdataItem from map.\n");
  }

}

NsObject* PgmSender::iface2link (int iface)
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

NsObject* PgmSender::pkt2agent (Packet *pkt)
{
        Tcl&            tcl = Tcl::instance();
        char            wrk[64];
        const char            *result;
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

void PgmSender::print_stats()
{
  printf("%s\n", uname_);
  printf("\tLast ODATA seqno: %d\n", odata_seqno_);
  printf("\tLast SPM seqno: %d\n", spm_seqno_);
  printf("\tNumber of NAKs received: %d\n", stats_.num_naks_received_);
  printf("\tNumber of RDATA transmitted: %d\n", stats_.num_rdata_sent_);
  printf("\tMax retransmission count for a single RDATA: %d\n", stats_.max_num_repeated_rdata_);
}

#ifdef PGM_DEBUG
void PgmSender::display_packet(Packet *pkt)
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

