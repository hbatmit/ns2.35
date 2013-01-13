//
// one_phase_pull.hh    : One-Phase Pull Include File
// author               : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: one_phase_pull.hh,v 1.4 2005/09/13 04:53:47 tomh Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.

#ifndef _ONE_PHASE_PULL_HH_
#define _ONE_PHASE_PULL_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <algorithm>
#include "diffapp.hh"

#ifdef NS_DIFFUSION
#include <tcl.h>
#include "diffagent.h"
#else
#include "main/hashutils.hh"
#endif // NS_DIFFUSION

#define ONE_PHASE_PULL_FILTER_PRIORITY 80

class OPPGradientEntry {
public:
  OPPGradientEntry(int32_t node_id) : node_id_(node_id)
  {
    GetTime(&tv_);
  };

  int32_t node_id_;
  struct timeval tv_;
};

typedef list<OPPGradientEntry *> GradientList;

class SinkEntry {
public:
  SinkEntry(u_int16_t port) : port_(port)
  {
    GetTime(&tv_);
  };

  u_int16_t port_;
  struct timeval tv_;
};

typedef list<SinkEntry *> SinkList;

class OPPDataNeighborEntry {
public:
  OPPDataNeighborEntry(int32_t node_id) : node_id_(node_id)
  {
    messages_ = 1;
  };

  int32_t node_id_;
  int messages_;
  bool new_messages_;
};

typedef list<OPPDataNeighborEntry *> DataNeighborList;

class SubscriptionEntry {
public:
  SubscriptionEntry(NRAttrVec *attrs) : attrs_(attrs)
  {
    GetTime(&tv_);
  };

  ~SubscriptionEntry()
  {
    ClearAttrs(attrs_);
    delete attrs_;
  };

  struct timeval tv_;
  NRAttrVec *attrs_;
};

typedef list<SubscriptionEntry *> SubscriptionList;

typedef list<int> FlowIdList;

class RoundIdEntry {
public:
  RoundIdEntry(int32_t round_id) : round_id_(round_id)
  {
    GetTime(&tv_);
  };

  ~RoundIdEntry()
  {
    GradientList::iterator gradient_itr;
    SinkList::iterator sink_itr;

    // Clear the gradient list
    for (gradient_itr = gradients_.begin();
	 gradient_itr != gradients_.end(); gradient_itr++){
      delete (*gradient_itr);
    }
    gradients_.clear();

    // Clear the local sink list
    for (sink_itr = sinks_.begin(); sink_itr != sinks_.end(); sink_itr++){
      delete (*sink_itr);
    }
    sinks_.clear();
  };

  OPPGradientEntry * findGradient(int32_t node_id);
  void deleteGradient(int32_t node_id);
  void addGradient(int32_t node_id);
  void updateSink(u_int16_t sink_id);
  void deleteExpiredSinks();
  void deleteExpiredGradients();
  
  int32_t round_id_;
  struct timeval tv_;
  GradientList gradients_;
  SinkList sinks_;
};

typedef list<RoundIdEntry *> RoundIdList;

class RoutingEntry {
public:
  RoutingEntry() {
    GetTime(&tv_);
  };

  ~RoutingEntry() {
    DataNeighborList::iterator data_neighbor_itr;
    RoundIdList::iterator round_id_itr;
    SubscriptionList::iterator subscription_itr;

    // Clear Attributes
    ClearAttrs(attrs_);
    delete attrs_;

    // Clear the attribute list
    for (subscription_itr = subscription_list_.begin();
	 subscription_itr != subscription_list_.end();
	 subscription_itr++){
      delete (*subscription_itr);
    }
    subscription_list_.clear();

    // Clear the round_ids list
    for (round_id_itr = round_ids_.begin(); round_id_itr != round_ids_.end(); round_id_itr++){
      delete (*round_id_itr);
    }
    round_ids_.clear();

    // Clear the data neighbor's list
    for (data_neighbor_itr = data_neighbors_.begin(); data_neighbor_itr != data_neighbors_.end(); data_neighbor_itr++){
      delete (*data_neighbor_itr);
    }
    data_neighbors_.clear();
  };

  RoundIdEntry * findRoundIdEntry(int32_t round_id);
  RoundIdEntry * addRoundIdEntry(int32_t round_id);
  void updateNeighborDataInfo(int32_t node_id, bool new_message);
  void addGradient(int32_t last_hop, int32_t round_id, bool new_gradient);
  void updateSink(u_int16_t sink_id, int32_t round_id);
  void deleteExpiredRoundIds();
  void getSinksFromList(FlowIdList *msg_list, FlowIdList *sink_list);
  void getFlowsFromList(FlowIdList *msg_list, FlowIdList *flow_list);
  int32_t getNeighborFromFlow(int32_t flow_id);

  struct timeval tv_;
  NRAttrVec *attrs_;
  RoundIdList round_ids_;
  SubscriptionList subscription_list_;
  DataNeighborList data_neighbors_;
};

typedef list<RoutingEntry *> RoutingTable;
class OnePhasePullFilter;

class OnePhasePullFilterReceive : public FilterCallback {
public:
  OnePhasePullFilterReceive(OnePhasePullFilter *filter) : filter_(filter) {};
  void recv(Message *msg, handle h);

  OnePhasePullFilter *filter_;
};

class DataForwardingHistory {
public:
  DataForwardingHistory()
  {
    data_reinforced_ = false;
  };

  ~DataForwardingHistory()
  {
    node_list_.clear();
    sink_list_.clear();
  };

  bool alreadyForwardedToNetwork(int32_t node_id)
  {
    list<int32_t>::iterator list_itr;

    list_itr = find(node_list_.begin(), node_list_.end(), node_id);
    if (list_itr == node_list_.end())
      return false;
    return true;
  };

  bool alreadyForwardedToLibrary(u_int16_t sink_id)
  {
    list<u_int16_t>::iterator list_itr;

    list_itr = find(sink_list_.begin(), sink_list_.end(), sink_id);
    if (list_itr == sink_list_.end())
      return false;
    return true;
  };

  bool alreadyReinforced()
  {
    return data_reinforced_;
  };

  void sendingReinforcement()
  {
    data_reinforced_ = true;
  };

  void forwardingToNetwork(int32_t node_id)
  {
    node_list_.push_back(node_id);
  };

  void forwardingToLibrary(u_int16_t sink_id)
  {
    sink_list_.push_back(sink_id);
  };

private:
  list<int32_t> node_list_;
  list<u_int16_t> sink_list_;
  bool data_reinforced_;
};

class OnePhasePullFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  OnePhasePullFilter(const char *dr);
  int command(int argc, const char*const* argv);
  void run() {}
#else
  OnePhasePullFilter(int argc, char **argv);
  void run();
#endif // NS_DIFFUSION

  virtual ~OnePhasePullFilter()
  {
    // Nothing to do
  };

  void recv(Message *msg, handle h);

  // Timers
  void messageTimeout(Message *msg);
  void interestTimeout(Message *msg);
  void gradientTimeout();
  void reinforcementTimeout();
  int subscriptionTimeout(NRAttrVec *attrs);

protected:

  // General Variables
  handle filter_handle_;
  int pkt_count_;
  int random_id_;

  // Receive Callback for the filter
  OnePhasePullFilterReceive *filter_callback_;

  // List of all known datatypes
  RoutingTable routing_list_;

  // Setup the filter
  handle setupFilter();

  // Matching functions
  RoutingEntry * findRoutingEntry(NRAttrVec *attrs);
  void deleteRoutingEntry(RoutingEntry *routing_entry);
  RoutingEntry * matchRoutingEntry(NRAttrVec *attrs, RoutingTable::iterator start, RoutingTable::iterator *place);
  SubscriptionEntry * findMatchingSubscription(RoutingEntry *routing_entry, NRAttrVec *attrs);

  // Message forwarding functions
  void sendInterest(NRAttrVec *attrs, RoutingEntry *routing_entry);
  void sendDisinterest(NRAttrVec *attrs, RoutingEntry *routing_entry);
  void forwardData(Message *msg, RoutingEntry *routing_entry,
		   DataForwardingHistory *forwarding_history);

  // Message Processing functions
  void processOldMessage(Message *msg);
  void processNewMessage(Message *msg);

  // Flow Ids Processing functions
  void addLocalFlowsToMessage(Message *msg);
  void readFlowsFromList(int number_of_flows, FlowIdList *flow_list,
			 void *source_blob);
  int * writeFlowsToList(FlowIdList *flow_list);
  bool removeFlowFromList(FlowIdList *flow_list, int32_t flow);
};

class OppGradientExpirationCheckTimer : public TimerCallback {
public:
  OppGradientExpirationCheckTimer(OnePhasePullFilter *agent) : agent_(agent) {};
  ~OppGradientExpirationCheckTimer() {};
  int expire();

  OnePhasePullFilter *agent_;
};

class OppReinforcementCheckTimer : public TimerCallback {
public:
  OppReinforcementCheckTimer(OnePhasePullFilter *agent) : agent_(agent) {};
  ~OppReinforcementCheckTimer() {};
  int expire();

  OnePhasePullFilter *agent_;
};

class OppMessageSendTimer : public TimerCallback {
public:
  OppMessageSendTimer(OnePhasePullFilter *agent, Message *msg) :
    agent_(agent), msg_(msg) {};
  ~OppMessageSendTimer()
  {
    delete msg_;
  };
  int expire();

  OnePhasePullFilter *agent_;
  Message *msg_;
};

class OppInterestForwardTimer : public TimerCallback {
public:
  OppInterestForwardTimer(OnePhasePullFilter *agent, Message *msg) :
    agent_(agent), msg_(msg) {};
  ~OppInterestForwardTimer()
  {
    delete msg_;
  };
  int expire();

  OnePhasePullFilter *agent_;
  Message *msg_;
};

class OppSubscriptionExpirationTimer : public TimerCallback {
public:
  OppSubscriptionExpirationTimer(OnePhasePullFilter *agent, NRAttrVec *attrs) :
    agent_(agent), attrs_(attrs) {};
  ~OppSubscriptionExpirationTimer()
  {
    ClearAttrs(attrs_);
    delete attrs_;
  };
  int expire();

  OnePhasePullFilter *agent_;
  NRAttrVec *attrs_;
};

#endif // !_ONE_PHASE_PULL_HH_
