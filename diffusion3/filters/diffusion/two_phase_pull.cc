//
// two_phase_pull.cc  : Two-Phase Pull/One-Phase Push Filter
// author             : Fabio Silva and Chalermek Intanagonwiwat
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: two_phase_pull.cc,v 1.6 2005/09/13 04:53:47 tomh Exp $
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

#include "two_phase_pull.hh"

#ifdef NS_DIFFUSION
static class GradientFilterClass : public TclClass {
public:
  GradientFilterClass() : TclClass("Application/DiffApp/GradientFilter") {}
  TclObject* create(int argc, const char*const* argv) {
    if (argc == 5)
      return(new GradientFilter(argv[4]));
    else
      fprintf(stderr, "Insufficient number of args for creating GradientFilter");
    return (NULL);
  }
} class_gradient_filter;

int GradientFilter::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcasecmp(argv[1], "debug") == 0) {
      global_debug_level = atoi(argv[2]);
      if (global_debug_level < 1 || global_debug_level > 10) {
	global_debug_level = DEBUG_DEFAULT;
	printf("Error: Debug level outside range(1-10) or missing !\n");
      }
    }
  }
  return DiffApp::command(argc, argv);
}

#endif // NS_DIFFUSION

void GradientFilterReceive::recv(Message *msg, handle h)
{
  app_->recv(msg, h);
}

int TppMessageSendTimer::expire()
{
  // Call timeout function
  agent_->messageTimeout(msg_);

  // Do not reschedule this timer
  delete this;
  return -1;
}

int TppInterestForwardTimer::expire()
{
  // Call timeout function
  agent_->interestTimeout(msg_);

  // Do not reschedule this timer
  delete this;
  return -1;
}

int TppSubscriptionExpirationTimer::expire()
{
  int retval;

  retval = agent_->subscriptionTimeout(attrs_);

  // Delete timer if we are not rescheduling it
  if (retval == -1)
    delete this;

  return retval;
}

int TppGradientExpirationCheckTimer::expire()
{
  // Call the callback function
  agent_->gradientTimeout();

  // Reschedule this timer
  return 0;
}

int TppReinforcementCheckTimer::expire()
{
  // Call the callback function
  agent_->reinforcementTimeout();

  // Reschedule this timer
  return 0;
}

void GradientFilter::interestTimeout(Message *msg)
{
  DiffPrint(DEBUG_MORE_DETAILS, "Interest Timeout !\n");

  msg->last_hop_ = LOCALHOST_ADDR;
  msg->next_hop_ = BROADCAST_ADDR;
 
  ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);
}

void GradientFilter::messageTimeout(Message *msg)
{
  DiffPrint(DEBUG_MORE_DETAILS, "Message Timeout !\n");

  ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);
}

void GradientFilter::gradientTimeout()
{
  RoutingTable::iterator routing_itr;
  GradientList::iterator grad_itr;
  AgentList::iterator agent_itr;
  TppRoutingEntry *routing_entry;
  GradientEntry *gradient_entry;
  AgentEntry *agent_entry;
  struct timeval tmv;

  DiffPrint(DEBUG_MORE_DETAILS, "Gradient Timeout !\n");

  GetTime(&tmv);

  routing_itr = routing_list_.begin();

  while (routing_itr != routing_list_.end()){
    routing_entry = *routing_itr;

    // Step 1: Delete expired gradients
    grad_itr = routing_entry->gradients_.begin();
    while (grad_itr != routing_entry->gradients_.end()){
      gradient_entry = *grad_itr;
      if (tmv.tv_sec > (gradient_entry->tv_.tv_sec + GRADIENT_TIMEOUT)){

	DiffPrint(DEBUG_NO_DETAILS, "Deleting Gradient to node %d !\n",
		  gradient_entry->node_addr_);

	grad_itr = routing_entry->gradients_.erase(grad_itr);
	delete gradient_entry;
      }
      else{
	grad_itr++;
      }
    }

    // Step 2: Remove non-active agents
    agent_itr = routing_entry->agents_.begin();
    while (agent_itr != routing_entry->agents_.end()){
      agent_entry = *agent_itr;
      if (tmv.tv_sec > (agent_entry->tv_.tv_sec + GRADIENT_TIMEOUT)){

	DiffPrint(DEBUG_NO_DETAILS,
		  "Deleting Gradient to agent %d !\n", agent_entry->port_);

	agent_itr = routing_entry->agents_.erase(agent_itr);
	delete agent_entry;
      }
      else{
	agent_itr++;
      }
    }

    // Remove the Routing Entry if no gradients and no agents
    if ((routing_entry->gradients_.size() == 0) &&
	(routing_entry->agents_.size() == 0)){
      // Deleting Routing Entry
      DiffPrint(DEBUG_DETAILS,
		"Nothing left for this data type, cleaning up !\n");
      routing_itr = routing_list_.erase(routing_itr);
      delete routing_entry;
    }
    else{
      routing_itr++;
    }
  }
}

void GradientFilter::reinforcementTimeout()
{
  DataNeighborList::iterator data_neighbor_itr;
  DataNeighborEntry *data_neighbor_entry;
  RoutingTable::iterator routing_itr;
  TppRoutingEntry *routing_entry;
  Message *my_message;

  DiffPrint(DEBUG_MORE_DETAILS, "Reinforcement Timeout !\n");

  routing_itr = routing_list_.begin();

  while (routing_itr != routing_list_.end()){
    routing_entry = *routing_itr;

    // Step 1: Delete expired gradients
    data_neighbor_itr = routing_entry->data_neighbors_.begin();

    while (data_neighbor_itr != routing_entry->data_neighbors_.end()){
      data_neighbor_entry = *data_neighbor_itr;

      if (data_neighbor_entry->data_flag_ == OLD_MESSAGE){
	my_message = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
				 0, 0, routing_entry->attrs_->size(), pkt_count_,
				 random_id_, data_neighbor_entry->neighbor_id_,
				 LOCALHOST_ADDR);
	my_message->msg_attr_vec_ = CopyAttrs(routing_entry->attrs_);

	DiffPrint(DEBUG_NO_DETAILS,
		  "Sending Negative Reinforcement to node %d !\n",
		  data_neighbor_entry->neighbor_id_);

	((DiffusionRouting *)dr_)->sendMessage(my_message, filter_handle_);

	pkt_count_++;
	delete my_message;

	// Done. Delete entry
	data_neighbor_itr = routing_entry->data_neighbors_.erase(data_neighbor_itr);
	delete data_neighbor_entry;
      }
      else{
	data_neighbor_itr++;
      }
    }

    // Step 2: Delete data neighbors with no activity, zero flags
    data_neighbor_itr = routing_entry->data_neighbors_.begin();
    while (data_neighbor_itr != routing_entry->data_neighbors_.end()){
      data_neighbor_entry = *data_neighbor_itr;
      if (data_neighbor_entry->data_flag_ == NEW_MESSAGE){
	data_neighbor_entry->data_flag_ = 0;
	data_neighbor_itr++;
      }
      else{
	// Delete entry
	data_neighbor_itr = routing_entry->data_neighbors_.erase(data_neighbor_itr);
	delete data_neighbor_entry;
      }
    }

    // Advance to the next routing entry
    routing_itr++;
  }
}

int GradientFilter::subscriptionTimeout(NRAttrVec *attrs)
{
  AttributeList::iterator attribute_itr;
  AttributeEntry *attribute_entry;
  TppRoutingEntry *routing_entry;
  struct timeval tmv;

  DiffPrint(DEBUG_MORE_DETAILS, "Subscription Timeout !\n");

  GetTime(&tmv);

  // Find the correct Routing Entry
  routing_entry = findRoutingEntry(attrs);

  if (routing_entry){
    // Step 1: Check Timeouts

    attribute_itr = routing_entry->attr_list_.begin();

    while (attribute_itr != routing_entry->attr_list_.end()){
      attribute_entry = *attribute_itr;
      if (tmv.tv_sec > (attribute_entry->tv_.tv_sec + SUBSCRIPTION_TIMEOUT)){
	sendDisinterest(attribute_entry->attrs_, routing_entry);
	attribute_itr = routing_entry->attr_list_.erase(attribute_itr);
	delete attribute_entry;
      }
      else{
	attribute_itr++;
      }
    }
  }
  else{
    DiffPrint(DEBUG_DETAILS, "Warning: SubscriptionTimeout could't find RE - maybe deleted by GradientTimeout ?\n");

    // Cancel Timer
    return -1;
  }

  // Keep Timer
  return 0;
}

void GradientFilter::deleteRoutingEntry(TppRoutingEntry *routing_entry)
{
  RoutingTable::iterator routing_itr;
  TppRoutingEntry *current_entry;

  for (routing_itr = routing_list_.begin(); routing_itr != routing_list_.end(); ++routing_itr){
    current_entry = *routing_itr;
    if (current_entry == routing_entry){
      routing_itr = routing_list_.erase(routing_itr);
      delete routing_entry;
      return;
    }
  }
  DiffPrint(DEBUG_ALWAYS, "Error: deleteRoutingEntry could not find entry to delete !\n");
}

TppRoutingEntry * GradientFilter::matchRoutingEntry(NRAttrVec *attrs, RoutingTable::iterator start, RoutingTable::iterator *place)
{
  RoutingTable::iterator routing_itr;
  TppRoutingEntry *routing_entry;

  for (routing_itr = start; routing_itr != routing_list_.end(); ++routing_itr){
    routing_entry = *routing_itr;
    if (MatchAttrs(routing_entry->attrs_, attrs)){
      *place = routing_itr;
      return routing_entry;
    }
  }
  return NULL;
}

TppRoutingEntry * GradientFilter::findRoutingEntry(NRAttrVec *attrs)
{
  RoutingTable::iterator routing_itr;
  TppRoutingEntry *routing_entry;

  for (routing_itr = routing_list_.begin(); routing_itr != routing_list_.end(); ++routing_itr){
    routing_entry = *routing_itr;
    if (PerfectMatch(routing_entry->attrs_, attrs))
      return routing_entry;
  }
  return NULL;
}

AttributeEntry * GradientFilter::findMatchingSubscription(TppRoutingEntry *routing_entry,
							  NRAttrVec *attrs)
{
  AttributeList::iterator attribute_itr;
  AttributeEntry *attribute_entry;

  for (attribute_itr = routing_entry->attr_list_.begin(); attribute_itr != routing_entry->attr_list_.end(); ++attribute_itr){
    attribute_entry = *attribute_itr;
    if (PerfectMatch(attribute_entry->attrs_, attrs))
      return attribute_entry;
  }
  return NULL;
}

void GradientFilter::updateGradient(TppRoutingEntry *routing_entry,
				    int32_t last_hop, bool reinforced)
{
  GradientList::iterator gradient_itr;
  GradientEntry *gradient_entry;

  for (gradient_itr = routing_entry->gradients_.begin();
       gradient_itr != routing_entry->gradients_.end(); ++gradient_itr){
    gradient_entry = *gradient_itr;
    if (gradient_entry->node_addr_ == last_hop){
      GetTime(&(gradient_entry->tv_));
      if (reinforced)
	gradient_entry->reinforced_ = true;
      return;
    }
  }

  // We need to add a new gradient
  gradient_entry = new GradientEntry(last_hop);
  if (reinforced)
    gradient_entry->reinforced_ = true;

  routing_entry->gradients_.push_back(gradient_entry);
}

void GradientFilter::updateAgent(TppRoutingEntry *routing_entry,
				 u_int16_t source_port)
{
  AgentList::iterator agent_itr;
  AgentEntry *agent_entry;

  for (agent_itr = routing_entry->agents_.begin(); agent_itr != routing_entry->agents_.end(); ++agent_itr){
    agent_entry = *agent_itr;
    if (agent_entry->port_ == source_port){
      // We already have this guy
      GetTime(&(agent_entry->tv_));
      return;
    }
  }

  // This is a new agent, so we create a new entry and add it to the
  // list of known agents
  agent_entry = new AgentEntry(source_port);
  routing_entry->agents_.push_back(agent_entry);
}

void GradientFilter::forwardPushExploratoryData(Message *msg,
						DataForwardingHistory *forwarding_history)
{
  RoutingTable::iterator routing_itr;
  TppRoutingEntry *routing_entry;
  AgentList::iterator agent_itr;
  AgentEntry *agent_entry;
  Message *data_msg, *sink_message;
  TimerCallback *data_timer;
  unsigned int key[2];
  HashEntry *hash_entry;

  // Sink processing
  routing_itr = routing_list_.begin();
  routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				    &routing_itr);

  sink_message = CopyMessage(msg);

  while (routing_entry){

    // Forward message to all local sinks
    for (agent_itr = routing_entry->agents_.begin();
	 agent_itr != routing_entry->agents_.end(); ++agent_itr){
      agent_entry = *agent_itr;

      if (!forwarding_history->alreadyForwardedToLibrary(agent_entry->port_)){
	// Send DATA message to local sinks
	sink_message->next_hop_ = LOCALHOST_ADDR;
	sink_message->next_port_ = agent_entry->port_;

	((DiffusionRouting *)dr_)->sendMessage(sink_message, filter_handle_);

	// Add agent to the forwarding history
	forwarding_history->forwardingToLibrary(agent_entry->port_);
      }
    }

    if ((!forwarding_history->alreadyReinforced()) &&
	(routing_entry->agents_.size() > 0) &&
	(msg->last_hop_ != LOCALHOST_ADDR)){
      // Send a positive reinforcement if we have sinks
      sendPositiveReinforcement(routing_entry->attrs_, msg->rdm_id_,
				msg->pkt_num_, msg->last_hop_);
      // Record reinforcement in the forwarding history so we do it
      // only once per received data message
      forwarding_history->sendingReinforcement();
    }

    // Look for other matching data types
    routing_itr++;
    routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				      &routing_itr);
  }

  // Delete sink_message after sink processing
  delete sink_message;

  // Intermediate node processing

  // Add message information to the hash table
  if (msg->last_hop_ != LOCALHOST_ADDR){
    key[0] = msg->pkt_num_;
    key[1] = msg->rdm_id_;

    hash_entry = new HashEntry(msg->last_hop_);

    putHash(hash_entry, key[0], key[1]);
  }

  // Rebroadcast the exploratory push data message
  if (!forwarding_history->alreadyForwardedToNetwork(BROADCAST_ADDR)){
    data_msg = CopyMessage(msg);
    data_msg->next_hop_ = BROADCAST_ADDR;

    data_timer = new TppMessageSendTimer(this, data_msg);

    // Add data timer to the queue
    ((DiffusionRouting *)dr_)->addTimer(PUSH_DATA_FORWARD_DELAY +
					(int) ((PUSH_DATA_FORWARD_JITTER * (GetRand() * 1.0 / RAND_MAX) - (PUSH_DATA_FORWARD_JITTER / 2))),
					data_timer);

    // Add broadcast information to forwarding history
    forwarding_history->forwardingToNetwork(BROADCAST_ADDR);
  }
}

void GradientFilter::forwardExploratoryData(Message *msg,
					    TppRoutingEntry *routing_entry,
					    DataForwardingHistory *forwarding_history)
{
#ifdef USE_BROADCAST_TO_MULTIPLE_RECIPIENTS
  Message *data_msg;
  TimerCallback *data_timer;
#else
  GradientList::iterator gradient_itr;
  GradientEntry *gradient_entry;
#endif // USE_BROADCAST_TO_MULTIPLE_RECIPIENTS
  AgentList::iterator agent_itr;
  AgentEntry *agent_entry;
  Message *sink_message;
  unsigned int key[2];
  HashEntry *hash_entry;

  sink_message = CopyMessage(msg);

  // Step 1: Sink Processing
  for (agent_itr = routing_entry->agents_.begin();
       agent_itr != routing_entry->agents_.end(); ++agent_itr){
    agent_entry = *agent_itr;

    if (!forwarding_history->alreadyForwardedToLibrary(agent_entry->port_)){
      // Forward the data message to local sinks
      sink_message->next_hop_ = LOCALHOST_ADDR;
      sink_message->next_port_ = agent_entry->port_;

      // Add agent to the forwarding list
      forwarding_history->forwardingToLibrary(agent_entry->port_);

      ((DiffusionRouting *)dr_)->sendMessage(sink_message, filter_handle_);
    }
  }

  delete sink_message;

  // Step 1A: Reinforcement Processing
  if ((!forwarding_history->alreadyReinforced()) &&
      (routing_entry->agents_.size() > 0) &&
      (msg->last_hop_ != LOCALHOST_ADDR)){
    // Send reinforcement to 'last_hop'
    sendPositiveReinforcement(routing_entry->attrs_, msg->rdm_id_,
			      msg->pkt_num_, msg->last_hop_);
    // Record reinforcement in the forwarding history so we do it only
    // once per received data message
    forwarding_history->sendingReinforcement();
  }

  // Step 2: Intermediate Processing

  // Set reinforcement flags
  if (msg->last_hop_ != LOCALHOST_ADDR){
    setReinforcementFlags(routing_entry, msg->last_hop_, NEW_MESSAGE);
  }

  // Add message information to the hash table
  if (msg->last_hop_ != LOCALHOST_ADDR){
    key[0] = msg->pkt_num_;
    key[1] = msg->rdm_id_;

    hash_entry = new HashEntry(msg->last_hop_);

    putHash(hash_entry, key[0], key[1]);
  }

  // Forward the EXPLORATORY message
#ifdef USE_BROADCAST_TO_MULTIPLE_RECIPIENTS
  if (!forwarding_history->alreadyForwardedToNetwork(BROADCAST_ADDR)){
    if (routing_entry->gradients_.size() > 0){
      // Broadcast DATA message
      data_msg = CopyMessage(msg);
      data_msg->next_hop_ = BROADCAST_ADDR;

      // Add to the forwarding history
      forwarding_history->forwardingToNetwork(BROADCAST_ADDR);

      data_timer = new TppMessageSendTimer(this, data_msg);

      // Add timer for forwarding the data packet
      ((DiffusionRouting *)dr_)->addTimer(DATA_FORWARD_DELAY +
					  (int) ((DATA_FORWARD_JITTER * (GetRand() * 1.0 / RAND_MAX) - (DATA_FORWARD_JITTER / 2))),
					  data_timer);
    }
  }
#else
  // Forward DATA to all output gradients
  for (gradient_itr = routing_entry->gradients_.begin();
       gradient_itr != routing_entry->gradients_.end(); ++gradient_itr){

    gradient_entry = *gradient_itr;

    // Check forwarding history
    if (!forwarding_history->alreadyForwardedToNetwork(gradient_entry->node_addr_)){
      msg->next_hop_ = gradient_entry->node_addr_;
      ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);

      // Add to the forwarding history
      forwarding_history->forwardingToNetwork(gradient_entry->node_addr_);
    }
  }
#endif // USE_BROADCAST_TO_MULTIPLE_RECIPIENTS
}

void GradientFilter::forwardData(Message *msg, TppRoutingEntry *routing_entry,
				 DataForwardingHistory *forwarding_history)
{
  GradientList::iterator gradient_itr;
  AgentList::iterator agent_itr;
  GradientEntry *gradient_entry;
  AgentEntry *agent_entry;
  Message *sink_message, *negative_reinforcement_msg;
  bool has_sink = false;

  sink_message = CopyMessage(msg);

  // Step 1: Sink Processing
  for (agent_itr = routing_entry->agents_.begin(); agent_itr != routing_entry->agents_.end(); ++agent_itr){
    agent_entry = *agent_itr;

    has_sink = true;

    if (!forwarding_history->alreadyForwardedToLibrary(agent_entry->port_)){
      // Forward DATA to local sinks
      sink_message->next_hop_ = LOCALHOST_ADDR;
      sink_message->next_port_ = agent_entry->port_;

      // Add agent to the forwarding list
      forwarding_history->forwardingToLibrary(agent_entry->port_);

      ((DiffusionRouting *)dr_)->sendMessage(sink_message, filter_handle_);
    }
  }

  delete sink_message;

  // Step 2: Intermediate Processing

  // Set reinforcement flags
  if (msg->last_hop_ != LOCALHOST_ADDR){
    setReinforcementFlags(routing_entry, msg->last_hop_, NEW_MESSAGE);
  }

  // Forward DATA only to reinforced gradients
  gradient_itr = routing_entry->gradients_.begin();
  gradient_entry = findReinforcedGradients(&routing_entry->gradients_,
					   gradient_itr, &gradient_itr);

  if (gradient_entry){
    while (gradient_entry){

      // Found reinforced gradient, forward data message to this
      // neighbor only if the messages comes from a different neighbor
      if (gradient_entry->node_addr_ != msg->last_hop_){
	msg->next_hop_ = gradient_entry->node_addr_;

	// Check if we have forwarded the message to this neighbor already
	if (!forwarding_history->alreadyForwardedToNetwork(msg->next_hop_)){
	  DiffPrint(DEBUG_NO_DETAILS,
		    "Node%d: Forwarding data using Reinforced Gradient to node %d !\n",
		    ((DiffusionRouting *)dr_)->getNodeId(), gradient_entry->node_addr_);

	  ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);

	  // Add the node to the forwarding history
	  forwarding_history->forwardingToNetwork(msg->next_hop_);
	}
      }

      // Move to the next one
      gradient_itr++;
      gradient_entry = findReinforcedGradients(&routing_entry->gradients_,
					       gradient_itr, &gradient_itr);
    }
  }
  else{
    // We could not find a reinforced path, so we send a negative
    // reinforcement to last_hop
    if ((!has_sink) && (msg->last_hop_ != LOCALHOST_ADDR)){
      negative_reinforcement_msg = new Message(DIFFUSION_VERSION,
					       NEGATIVE_REINFORCEMENT,
					       0, 0,
					       routing_entry->attrs_->size(),
					       pkt_count_,
					       random_id_,
					       msg->last_hop_,
					       LOCALHOST_ADDR);
      negative_reinforcement_msg->msg_attr_vec_ = CopyAttrs(routing_entry->attrs_);

      DiffPrint(DEBUG_NO_DETAILS,
		"Sending Negative Reinforcement to node %d !\n",
		msg->last_hop_);

      ((DiffusionRouting *)dr_)->sendMessage(negative_reinforcement_msg,
					     filter_handle_);

      pkt_count_++;
      delete negative_reinforcement_msg;
    }
  }
}

void GradientFilter::sendPositiveReinforcement(NRAttrVec *reinf_attrs,
					       int32_t data_rdm_id,
					       int32_t data_pkt_num,
					       int32_t destination)
{
  ReinforcementBlob *reinforcement_blob;
  NRAttribute *reinforcement_attr;
  TimerCallback *reinforcement_timer;
  Message *pos_reinf_message;
  NRAttrVec *attrs;

  reinforcement_blob = new ReinforcementBlob(data_rdm_id, data_pkt_num);

  reinforcement_attr = ReinforcementAttr.make(NRAttribute::IS,
					      (void *) reinforcement_blob,
					      sizeof(ReinforcementBlob));

  attrs = CopyAttrs(reinf_attrs);
  attrs->push_back(reinforcement_attr);

  pos_reinf_message = new Message(DIFFUSION_VERSION, POSITIVE_REINFORCEMENT,
				  0, 0, attrs->size(), pkt_count_,
				  random_id_, destination, LOCALHOST_ADDR);
  pos_reinf_message->msg_attr_vec_ = CopyAttrs(attrs);

  DiffPrint(DEBUG_NO_DETAILS, "Sending Positive Reinforcement to node %d !\n",
	    destination);

  // Create timer for sending this message
  reinforcement_timer = new TppMessageSendTimer(this, pos_reinf_message);

  // Add timer to the event queue
  ((DiffusionRouting *)dr_)->addTimer(POS_REINFORCEMENT_SEND_DELAY +
				      (int) ((POS_REINFORCEMENT_JITTER * (GetRand() * 1.0 / RAND_MAX) - (POS_REINFORCEMENT_JITTER / 2))),
				      reinforcement_timer);
  pkt_count_++;
  ClearAttrs(attrs);
  delete attrs;
  delete reinforcement_blob;
}

GradientEntry * GradientFilter::findReinforcedGradients(GradientList *gradients,
							GradientList::iterator start,
							GradientList::iterator *place)
{
  GradientList::iterator gradient_itr;
  GradientEntry *gradient_entry;

  for (gradient_itr = start; gradient_itr != gradients->end(); ++gradient_itr){
    gradient_entry = *gradient_itr;
    if (gradient_entry->reinforced_){
      *place = gradient_itr;
      return gradient_entry;
    }
  }

  return NULL;
}

GradientEntry * GradientFilter::findReinforcedGradient(int32_t node_addr,
						       TppRoutingEntry *routing_entry)
{
  GradientList::iterator gradient_itr;
  GradientEntry *gradient_entry;

  gradient_itr = routing_entry->gradients_.begin();
  gradient_entry = findReinforcedGradients(&routing_entry->gradients_,
					   gradient_itr, &gradient_itr);

  if (gradient_entry){
    while(gradient_entry){
      if (gradient_entry->node_addr_ == node_addr)
	return gradient_entry;

      // This is not the gradient we are looking for, keep looking
      gradient_itr++;
      gradient_entry = findReinforcedGradients(&routing_entry->gradients_,
					       gradient_itr, &gradient_itr);
    }
  }

  return NULL;
}

void GradientFilter::deleteGradient(TppRoutingEntry *routing_entry,
				    GradientEntry *gradient_entry)
{
  GradientList::iterator gradient_itr;
  GradientEntry *current_entry;

  for (gradient_itr = routing_entry->gradients_.begin();
       gradient_itr != routing_entry->gradients_.end(); ++gradient_itr){
    current_entry = *gradient_itr;
    if (current_entry == gradient_entry){
      gradient_itr = routing_entry->gradients_.erase(gradient_itr);
      delete gradient_entry;
      return;
    }
  }
  DiffPrint(DEBUG_ALWAYS,
	    "Error: deleteGradient could not find gradient to delete !\n");
}

void GradientFilter::setReinforcementFlags(TppRoutingEntry *routing_entry,
					   int32_t last_hop, int new_message)
{
  DataNeighborList::iterator data_neighbor_itr;
  DataNeighborEntry *data_neighbor_entry;

  for (data_neighbor_itr = routing_entry->data_neighbors_.begin();
       data_neighbor_itr != routing_entry->data_neighbors_.end();
       ++data_neighbor_itr){
    data_neighbor_entry = *data_neighbor_itr;
    if (data_neighbor_entry->neighbor_id_ == last_hop){
      if (data_neighbor_entry->data_flag_ > 0)
	return;
      data_neighbor_entry->data_flag_ = new_message;
      return;
    }
  }

  // We need to add a new data neighbor
  data_neighbor_entry = new DataNeighborEntry(last_hop, new_message);

  routing_entry->data_neighbors_.push_back(data_neighbor_entry);
}

void GradientFilter::sendInterest(NRAttrVec *attrs, TppRoutingEntry *routing_entry)
{
  AgentList::iterator agent_itr;
  AgentEntry *agent_entry;

  Message *msg = new Message(DIFFUSION_VERSION, INTEREST, 0, 0,
			     attrs->size(), 0, 0, LOCALHOST_ADDR,
			     LOCALHOST_ADDR);

  msg->msg_attr_vec_ = CopyAttrs(attrs);

  for (agent_itr = routing_entry->agents_.begin(); agent_itr != routing_entry->agents_.end(); ++agent_itr){
    agent_entry = *agent_itr;

    msg->next_port_ = agent_entry->port_;

    ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);
  }

  delete msg;
}

void GradientFilter::sendDisinterest(NRAttrVec *attrs,
				     TppRoutingEntry *routing_entry)
{
  NRAttrVec *new_attrs;
  NRSimpleAttribute<int> *nrclass = NULL;

  new_attrs = CopyAttrs(attrs);

  nrclass = NRClassAttr.find(new_attrs);
  if (!nrclass){
    DiffPrint(DEBUG_ALWAYS,
	      "Error: sendDisinterest couldn't find the class attribute !\n");
    ClearAttrs(new_attrs);
    delete new_attrs;
    return;
  }

  // Change the class_key value
  nrclass->setVal(NRAttribute::DISINTEREST_CLASS);

  sendInterest(new_attrs, routing_entry);
   
  ClearAttrs(new_attrs);
  delete new_attrs;
}

void GradientFilter::recv(Message *msg, handle h)
{
  if (h != filter_handle_){
    DiffPrint(DEBUG_ALWAYS,
	      "Error: received msg for handle %d, subscribed to handle %d !\n",
	      h, filter_handle_);
    return;
  }

  if (msg->new_message_ == 1)
    processNewMessage(msg);
  else
    processOldMessage(msg);
}

void GradientFilter::processOldMessage(Message *msg)
{
  TppRoutingEntry *routing_entry;
  RoutingTable::iterator routing_itr;

  switch (msg->msg_type_){

  case INTEREST:

    DiffPrint(DEBUG_NO_DETAILS, "Node%d: Received Old Interest !\n", ((DiffusionRouting *)dr_)->getNodeId());

    if (msg->last_hop_ == LOCALHOST_ADDR){
      // Old interest should not come from local agent
      DiffPrint(DEBUG_ALWAYS, "Warning: Old Interest from local agent !\n");
      break;
    }

    // Get the routing entry for these attrs      
    routing_entry = findRoutingEntry(msg->msg_attr_vec_);
    if (routing_entry)
      updateGradient(routing_entry, msg->last_hop_, false);

    break;

  case DATA: 

    DiffPrint(DEBUG_NO_DETAILS, "Node%d: Received an old Data message !\n", ((DiffusionRouting *)dr_)->getNodeId());

    // Find the correct routing entry
    routing_itr = routing_list_.begin();
    routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				      &routing_itr);

    while (routing_entry){
      DiffPrint(DEBUG_NO_DETAILS,
		"Set flags to %d to OLD_MESSAGE !\n", msg->last_hop_);

      // Set reinforcement flags
      if (msg->last_hop_ != LOCALHOST_ADDR){
	setReinforcementFlags(routing_entry, msg->last_hop_, OLD_MESSAGE);
      }

      // Continue going through other data types
      routing_itr++;
      routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
					&routing_itr);
    }

    break;

  case PUSH_EXPLORATORY_DATA:

    // Just drop it
    DiffPrint(DEBUG_NO_DETAILS,
	      "Received an old Push Exploratory Data. Loop detected !\n");
    
    break;

  case EXPLORATORY_DATA:
    
    // Just drop it
    DiffPrint(DEBUG_NO_DETAILS,
	      "Received an old Exploratory Data. Loop detected !\n");

    break;

  case POSITIVE_REINFORCEMENT:

    DiffPrint(DEBUG_IMPORTANT, "Received an old Positive Reinforcement !\n");

    break;

  case NEGATIVE_REINFORCEMENT:

    DiffPrint(DEBUG_IMPORTANT, "Received an old Negative Reinforcement !\n");

    DiffPrint(DEBUG_IMPORTANT, "pkt_num = %d, rdm_id = %d !\n",
	      msg->pkt_num_, msg->rdm_id_);

    break;

  default: 

    break;
  }
}

void GradientFilter::processNewMessage(Message *msg)
{
  NRSimpleAttribute<void *> *reinforcement_attr = NULL;
  DataForwardingHistory *forwarding_history;
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;
  ReinforcementBlob *reinforcement_blob;
  RoutingTable::iterator routing_itr;
  TppRoutingEntry *routing_entry;
  GradientList::iterator gradient_itr;
  GradientEntry *gradient_entry;
  NRAttrVec::iterator place;
  HashEntry *hash_entry;
  AttributeEntry *attribute_entry;
  Message *my_msg;
  TimerCallback *interest_timer, *subscription_timer;
  unsigned int key[2];
  bool new_data_type = false;

  switch (msg->msg_type_){

  case INTEREST:

    DiffPrint(DEBUG_NO_DETAILS, "Received Interest !\n");

    nrclass = NRClassAttr.find(msg->msg_attr_vec_);
    nrscope = NRScopeAttr.find(msg->msg_attr_vec_);

    if (!nrclass || !nrscope){
      DiffPrint(DEBUG_ALWAYS,
		"Warning: Can't find CLASS/SCOPE attributes in the message !\n");
      return;
    }

    // Step 1: Look for the same data type
    routing_entry = findRoutingEntry(msg->msg_attr_vec_);

    if (!routing_entry){
      // Create a new routing entry for this data type
      routing_entry = new TppRoutingEntry;
      routing_entry->attrs_ = CopyAttrs(msg->msg_attr_vec_);
      routing_list_.push_back(routing_entry);
      new_data_type = true;
    }

    if (msg->last_hop_ == LOCALHOST_ADDR){
      // From local agent
      updateAgent(routing_entry, msg->source_port_);
    }
    else{
      // From outside, we just add the new gradient
      updateGradient(routing_entry, msg->last_hop_, false);
    }

    if ((nrclass->getVal() == NRAttribute::INTEREST_CLASS) &&
	(nrclass->getOp() == NRAttribute::IS)){

      // Global interest messages should always be forwarded
      if (nrscope->getVal() == NRAttribute::GLOBAL_SCOPE){

	interest_timer = new TppInterestForwardTimer(this, CopyMessage(msg));

	((DiffusionRouting *)dr_)->addTimer(INTEREST_FORWARD_DELAY +
					    (int) ((INTEREST_FORWARD_JITTER * (GetRand() * 1.0 / RAND_MAX) - (INTEREST_FORWARD_JITTER / 2))),
					    interest_timer);
      }
    }
    else{
      if ((nrclass->getOp() != NRAttribute::IS) &&
	  (nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE) &&
	  (new_data_type)){

	subscription_timer = new TppSubscriptionExpirationTimer(this,
							     CopyAttrs(msg->msg_attr_vec_));
	
	((DiffusionRouting *)dr_)->addTimer(SUBSCRIPTION_DELAY +
					    (int) (SUBSCRIPTION_DELAY * (GetRand() * 1.0 / RAND_MAX)),
					    subscription_timer);
      }

      // Subscriptions don't have to match other subscriptions
      break;
    }

    // Step 2: Match other routing tables
    routing_itr = routing_list_.begin();
    routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				      &routing_itr);

    while (routing_entry){
      // Got a match
      attribute_entry = findMatchingSubscription(routing_entry,
						 msg->msg_attr_vec_);

      // Do we already have this subscription
      if (attribute_entry){
	GetTime(&(attribute_entry->tv_));
      }
      else{
	// Create a new attribute entry, add it to the attribute list
	// and send an interest message to the local agent
	attribute_entry = new AttributeEntry(CopyAttrs(msg->msg_attr_vec_));
	routing_entry->attr_list_.push_back(attribute_entry);
	sendInterest(attribute_entry->attrs_, routing_entry);
      }
      // Move to the next TppRoutingEntry
      routing_itr++;
      routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
					&routing_itr);
    }

      break;

  case DATA:

    DiffPrint(DEBUG_NO_DETAILS, "Node%d: Received Data !\n",((DiffusionRouting *)dr_)->getNodeId());

    // Create data message forwarding cache
    forwarding_history = new DataForwardingHistory;

    // Find the correct routing entry
    routing_itr = routing_list_.begin();
    routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				      &routing_itr);

    while (routing_entry){
      forwardData(msg, routing_entry, forwarding_history);
      routing_itr++;
      routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
					&routing_itr);
    }

    delete forwarding_history;

    break;

  case EXPLORATORY_DATA:

    DiffPrint(DEBUG_NO_DETAILS, "Received Exploratory Data !\n");

    // Create data message forwarding cache
    forwarding_history = new DataForwardingHistory;

    // Find the correct routing entry
    routing_itr = routing_list_.begin();
    routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				      &routing_itr);

    while (routing_entry){
      forwardExploratoryData(msg, routing_entry, forwarding_history);
      routing_itr++;
      routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
					&routing_itr);
    }

    // Delete data forwarding cache
    delete forwarding_history;

    break;

  case PUSH_EXPLORATORY_DATA:

    DiffPrint(DEBUG_NO_DETAILS, "Received Push Exploratory Data !\n");

    // Create data message forwarding cache
    forwarding_history = new DataForwardingHistory;

    // Forward data message
    forwardPushExploratoryData(msg, forwarding_history);

    // Delete data forwarding cache
    delete forwarding_history;

    break;

  case POSITIVE_REINFORCEMENT:

    DiffPrint(DEBUG_NO_DETAILS, "Received a Positive Reinforcement !\n");

    // Step 0: Look for reinforcement attribute
    place = msg->msg_attr_vec_->begin();
    reinforcement_attr = ReinforcementAttr.find_from(msg->msg_attr_vec_,
						     place, &place);
    if (!reinforcement_attr){
      DiffPrint(DEBUG_ALWAYS,
		"Error: Received an invalid Positive Reinforcement message !\n");
      return;
    }

    // Step 1: Extract reinforcement blob from message and look for an
    // entry in our hash table
    reinforcement_blob = (ReinforcementBlob *) reinforcement_attr->getVal();

    key[0] = reinforcement_blob->pkt_num_;
    key[1] = reinforcement_blob->rdm_id_;

    hash_entry = getHash(key[0], key[1]);

    // Step 2: Remove the reinforcement attribute from the message
    msg->msg_attr_vec_->erase(place);

    // Step 3: Find a routing entry that matches this message
    routing_entry = findRoutingEntry(msg->msg_attr_vec_);

    if (!routing_entry){
      // So, if we do not know about this data type, this must be a
      // reinforcement message to a PUSHED_EXPLORATORY_DATA message

      // Check for class/scope (all interest message should have it)
      nrclass = NRClassAttr.find(msg->msg_attr_vec_);
      nrscope = NRScopeAttr.find(msg->msg_attr_vec_);

      if (!nrclass || !nrscope){
	DiffPrint(DEBUG_ALWAYS,
		  "Warning: Can't find CLASS/SCOPE attributes in the message !\n");
	return;
      }

      // Create new Routing Entry
      routing_entry = new TppRoutingEntry;
      routing_entry->attrs_ = CopyAttrs(msg->msg_attr_vec_);
      routing_list_.push_back(routing_entry);
    }

    // Add reinforced gradient to last_hop
    updateGradient(routing_entry, msg->last_hop_, true);

    // Add the reinforcement attribute back to the message
    msg->msg_attr_vec_->push_back(reinforcement_attr);

    // If we have no record of this message it is either because we
    // originated the message (in which case, no further action is
    // required) or because we dropped it a long time ago because of
    // our hashing configuration parameters (in this case, we can't do
    // anything)
    if (hash_entry){
      msg->next_hop_ = hash_entry->last_hop_;

      DiffPrint(DEBUG_NO_DETAILS,
		"Forwarding Positive Reinforcement to node %d !\n",
		hash_entry->last_hop_);

      ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);
    }

    break;

  case NEGATIVE_REINFORCEMENT:

    DiffPrint(DEBUG_NO_DETAILS, "Received a Negative Reinforcement !\n");

    routing_entry = findRoutingEntry(msg->msg_attr_vec_);

    if (routing_entry){
      gradient_entry = findReinforcedGradient(msg->last_hop_, routing_entry);

      if (gradient_entry){
	// Remove reinforced gradient to last_hop
	deleteGradient(routing_entry, gradient_entry);

	gradient_entry = findReinforcedGradients(&routing_entry->gradients_,
						 routing_entry->gradients_.begin(),
						 &gradient_itr);

	// If there are no other reinforced outgoing gradients
	// we need to send our own negative reinforcement
	if (!gradient_entry){
	  my_msg = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
			       0, 0, routing_entry->attrs_->size(), pkt_count_,
			       random_id_, BROADCAST_ADDR, LOCALHOST_ADDR);
	  my_msg->msg_attr_vec_ = CopyAttrs(routing_entry->attrs_);

	  DiffPrint(DEBUG_NO_DETAILS,
		    "Forwarding Negative Reinforcement to ALL !\n");

	  ((DiffusionRouting *)dr_)->sendMessage(my_msg, filter_handle_);

	  pkt_count_++;
	  delete my_msg;
	}
      }
    }

    break;

  default:

    break;
  }
}

HashEntry * GradientFilter::getHash(unsigned int pkt_num,
				    unsigned int rdm_id)
{
   unsigned int key[2];
   
   key[0] = pkt_num;
   key[1] = rdm_id;
   
   Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable_, (char *)key);
   
   if (entryPtr == NULL)
      return NULL;
   
   return ((HashEntry *) Tcl_GetHashValue(entryPtr));
}

void GradientFilter::putHash(HashEntry *new_hash_entry,
			     unsigned int pkt_num,
			     unsigned int rdm_id)
{
   Tcl_HashEntry *tcl_hash_entry;
   HashEntry *hash_entry;
   HashList::iterator hash_itr;
   unsigned int key[2];
   int new_hash_key;
 
   if (hash_list_.size() == HASH_TABLE_DATA_MAX_SIZE){
      // Hash table reached maximum size
      
      for (int i = 0; ((i < HASH_TABLE_DATA_REMOVE_AT_ONCE)
		       && (hash_list_.size() > 0)); i++){
	 hash_itr = hash_list_.begin();
	 tcl_hash_entry = *hash_itr;
	 hash_entry = (HashEntry *) Tcl_GetHashValue(tcl_hash_entry);
	 delete hash_entry;
	 hash_list_.erase(hash_itr);
	 Tcl_DeleteHashEntry(tcl_hash_entry);
      }
   }
  
   key[0] = pkt_num;
   key[1] = rdm_id;
   
   tcl_hash_entry = Tcl_CreateHashEntry(&htable_, (char *) key, &new_hash_key);

   if (new_hash_key == 0){
      DiffPrint(DEBUG_IMPORTANT, "Key already exists in hash !\n");
      return;
   }

   Tcl_SetHashValue(tcl_hash_entry, new_hash_entry);
   hash_list_.push_back(tcl_hash_entry);
}

handle GradientFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // For the gradient filter, we use a single attribute with an "IS"
  // operator. This causes this filter to match every single packet
  // getting to diffusion
  attrs.push_back(NRClassAttr.make(NRAttribute::IS,
				   NRAttribute::INTEREST_CLASS));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs,
					   GRADIENT_FILTER_PRIORITY, filter_callback_);

  ClearAttrs(&attrs);
  return h;
}

#ifndef NS_DIFFUSION
void GradientFilter::run()
{
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
}
#endif // !NS_DIFFUSION

#ifdef NS_DIFFUSION
GradientFilter::GradientFilter(const char *diffrtg)
{
  DiffAppAgent *agent;
#else
GradientFilter::GradientFilter(int argc, char **argv)
{
#endif // NS_DIFFUSION
  struct timeval tv;
  TimerCallback *reinforcement_timer, *gradient_timer;

  GetTime(&tv);
  SetSeed(&tv);
  pkt_count_ = GetRand();
  random_id_ = GetRand();

  // Create Diffusion Routing class
#ifdef NS_DIFFUSION
  agent = (DiffAppAgent *)TclObject::lookup(diffrtg);
  dr_ = agent->dr();
#else
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION

  // Create callback classes and set up pointers
  filter_callback_ = new GradientFilterReceive(this);

  // Initialize Hashing structures
  Tcl_InitHashTable(&htable_, 2);

  // Set up the filter
  filter_handle_ = setupFilter();

  // Print filter information
  DiffPrint(DEBUG_IMPORTANT,
	    "Gradient filter subscribed to *, received handle %d\n",
	    filter_handle_);

  // Add timers for keeping state up-to-date
  gradient_timer = new TppGradientExpirationCheckTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(GRADIENT_DELAY, gradient_timer);

  reinforcement_timer = new TppReinforcementCheckTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(REINFORCEMENT_DELAY, reinforcement_timer);

  GetTime(&tv);

  DiffPrint(DEBUG_ALWAYS, "Gradient filter initialized at time %ld:%ld!\n",
	    tv.tv_sec, tv.tv_usec);
}

#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  GradientFilter *app;

  // Initialize and run the Gradient Filter
  app = new GradientFilter(argc, argv);
  app->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE
