//
// one_phase_pull.cc    : One-Phase Pull Filter
// author               : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: one_phase_pull.cc,v 1.6 2005/09/13 04:53:47 tomh Exp $
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

#include "one_phase_pull.hh"

#ifdef NS_DIFFUSION
static class OnePhasePullFilterClass : public TclClass {
public:
  OnePhasePullFilterClass() : TclClass("Application/DiffApp/OnePhasePullFilter") {}
  TclObject* create(int argc, const char*const* argv) {
    if (argc == 5)
      return(new OnePhasePullFilter(argv[4]));
    else
      fprintf(stderr,
	      "Insufficient number of args for creating OnePhasePullFilter");
    return (NULL);
  }
} class_one_phase_pull_filter;

int OnePhasePullFilter::command(int argc, const char*const* argv) {
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

void OnePhasePullFilterReceive::recv(Message *msg, handle h)
{
  filter_->recv(msg, h);
}

int OppMessageSendTimer::expire()
{
  // Call timeout function
  agent_->messageTimeout(msg_);

  // Do not reschedule this timer
  delete this;
  return -1;
}

int OppInterestForwardTimer::expire()
{
  // Call timeout function
  agent_->interestTimeout(msg_);

  // Do not reschedule this timer
  delete this;
  return -1;
}

int OppSubscriptionExpirationTimer::expire()
{
  int retval;

  retval = agent_->subscriptionTimeout(attrs_);

  // Delete timer if we are not rescheduling it
  if (retval == -1)
    delete this;

  return retval;
}

int OppGradientExpirationCheckTimer::expire()
{
  // Call the callback function
  agent_->gradientTimeout();

  // Reschedule this timer
  return 0;
}

int OppReinforcementCheckTimer::expire()
{
  // Call the callback function
  agent_->reinforcementTimeout();

  // Reschedule this timer
  return 0;
}

RoundIdEntry * RoutingEntry::findRoundIdEntry(int32_t round_id)
{
  RoundIdList::iterator round_id_itr;
  RoundIdEntry *round_id_entry;

  // Iterate through round ids for this routing entry
  for (round_id_itr = round_ids_.begin();
       round_id_itr != round_ids_.end(); round_id_itr++){
    round_id_entry = *round_id_itr;

    // Check if round ids match
    if (round_id_entry->round_id_ == round_id)
      return round_id_entry;
  }

  // Couldn't find a matching round id entry
  return NULL;
}

RoundIdEntry * RoutingEntry::addRoundIdEntry(int32_t round_id)
{
  RoundIdEntry *round_id_entry;

  // Create a new round id entry
  round_id_entry = new RoundIdEntry(round_id);

  // Add it to the round id list
  round_ids_.push_back(round_id_entry);

  return round_id_entry;
}

void RoutingEntry::updateNeighborDataInfo(int32_t node_id,
					  bool new_message)
{
  DataNeighborList::iterator data_neighbor_itr;
  OPPDataNeighborEntry *data_neighbor_entry;

  for (data_neighbor_itr = data_neighbors_.begin();
       data_neighbor_itr != data_neighbors_.end(); ++data_neighbor_itr){
    data_neighbor_entry = *data_neighbor_itr;

    // Find neighbor
    if (data_neighbor_entry->node_id_ == node_id){

      // Increment message count
      data_neighbor_entry->messages_++;

      // If this is a new message, just set flag and return
      if (new_message){
	data_neighbor_entry->new_messages_ = new_message;
	return;
      }
    }
  }

  // We need to add a new data neighbor
  data_neighbor_entry = new OPPDataNeighborEntry(node_id);
  data_neighbor_entry->new_messages_ = new_message;
  data_neighbors_.push_back(data_neighbor_entry);
}

void RoutingEntry::addGradient(int32_t last_hop,
			       int32_t round_id, bool new_gradient)
{
  RoundIdEntry *round_id_entry;
  OPPGradientEntry *gradient_entry;

  // Look for an existing routing id entry
  round_id_entry = findRoundIdEntry(round_id);

  // Create new entry if not found
  if (!round_id_entry)
    round_id_entry = addRoundIdEntry(round_id);

  if (new_gradient){
    // Marks the beginning of a new round
    round_id_entry->gradients_.clear();
  }
  else{
    // Look for a gradient to our last_hop neighbor
    gradient_entry = round_id_entry->findGradient(last_hop);

    if (gradient_entry){
      // Gradient already in the list, we just update time
      GetTime(&gradient_entry->tv_);

      return;
    }
  }

  // Gradient not yet in the list, add this neighbor to the list
  round_id_entry->addGradient(last_hop);
}

void RoutingEntry::updateSink(u_int16_t sink_id, int32_t round_id)
{
  RoundIdEntry *round_id_entry;

  // Lock for an existing round id entry
  round_id_entry = findRoundIdEntry(round_id);

  // Create new entry if not found
  if (!round_id_entry)
    round_id_entry = addRoundIdEntry(round_id);

  // Add/Update this sink
  round_id_entry->updateSink(sink_id);
}

void RoutingEntry::deleteExpiredRoundIds()
{
  RoundIdList::iterator round_id_itr;
  RoundIdEntry *round_id_entry;
  struct timeval tmv;

  GetTime(&tmv);

  // Go through all round ids
  for (round_id_itr = round_ids_.begin();
       round_id_itr != round_ids_.end(); round_id_itr++){
    round_id_entry = *round_id_itr;

    round_id_entry->deleteExpiredSinks();
    round_id_entry->deleteExpiredGradients();

    // Delete round id if nothing left
    if (round_id_entry->gradients_.size() == 0 &&
	round_id_entry->sinks_.size() == 0){

      // Round Id has expired, delete it from the list
      DiffPrint(DEBUG_NO_DETAILS, "Delete expired Round Id: %d\n",
		round_id_entry->round_id_);

      round_id_itr = round_ids_.erase(round_id_itr);
      delete round_id_entry;
    }
  }
}

void RoutingEntry::getSinksFromList(FlowIdList *msg_list,
				    FlowIdList *sink_list)
{
  RoundIdList::iterator round_id_itr;
  RoundIdEntry *round_id_entry;
  FlowIdList::iterator flow_id_itr;

  for (round_id_itr = round_ids_.begin();
       round_id_itr != round_ids_.end(); round_id_itr++){
    round_id_entry = *round_id_itr;

    flow_id_itr = find(msg_list->begin(),
		       msg_list->end(), round_id_entry->round_id_);
    if (flow_id_itr != msg_list->end()){
      // Flow id in the list
      if (round_id_entry->sinks_.size() > 0){
	sink_list->push_back(round_id_entry->round_id_);
      }
    }
  }
}

void RoutingEntry::getFlowsFromList(FlowIdList *msg_list,
				    FlowIdList *flow_list)
{
  RoundIdList::iterator round_id_itr;
  RoundIdEntry *round_id_entry;
  FlowIdList::iterator flow_id_itr;

  for (round_id_itr = round_ids_.begin();
       round_id_itr != round_ids_.end(); round_id_itr++){
    round_id_entry = *round_id_itr;

    flow_id_itr = find(msg_list->begin(),
		       msg_list->end(), round_id_entry->round_id_);
    if (flow_id_itr != msg_list->end()){
      // Flow id in the list
      if (round_id_entry->sinks_.size() == 0){
	// This is a flow we have no local sink for
	flow_list->push_back(round_id_entry->round_id_);
      }
    }
  }
}

int32_t RoutingEntry::getNeighborFromFlow(int32_t flow_id)
{
  RoundIdList::iterator round_id_itr;
  RoundIdEntry *round_id_entry;
  OPPGradientEntry *gradient_entry;

  for (round_id_itr = round_ids_.begin();
       round_id_itr != round_ids_.end(); round_id_itr++){
    round_id_entry = *round_id_itr;

    if (round_id_entry->round_id_ == flow_id){
      // Flow matches, get 'reinforced neighbor'

      if (round_id_entry->gradients_.size() > 0){
	// Get the first gradient

	gradient_entry = *round_id_entry->gradients_.begin();
	return gradient_entry->node_id_;
      }

      DiffPrint(DEBUG_ALWAYS, "Cannot find 'reinforced neighbor !\n");
      break;
    }
  }

  // Couldn't find neighbor for this flow
  return BROADCAST_ADDR;
}
void RoundIdEntry::deleteExpiredSinks()
{
  SinkList::iterator sink_itr;
  SinkEntry *sink_entry;
  struct timeval tmv;

  GetTime(&tmv);

  // Go through all sinks
  for (sink_itr = sinks_.begin(); sink_itr != sinks_.end(); sink_itr++){
    sink_entry = *sink_itr;

    // Check if expired
    if (tmv.tv_sec > (sink_entry->tv_.tv_sec + GRADIENT_TIMEOUT)){

      // Expired, delete it
      DiffPrint(DEBUG_NO_DETAILS,
		"Deleting Gradient to sink %d !\n", sink_entry->port_);
      sink_itr = sinks_.erase(sink_itr);
      delete sink_entry;
    }
  }
}

void RoundIdEntry::deleteExpiredGradients()
{
  GradientList::iterator gradient_itr;
  OPPGradientEntry *gradient_entry;
  struct timeval tmv;

  GetTime(&tmv);

  // Go through all gradients
  for (gradient_itr = gradients_.begin();
       gradient_itr != gradients_.end(); gradient_itr++){
    gradient_entry = *gradient_itr;

    // Check if expired
    if (tmv.tv_sec > (gradient_entry->tv_.tv_sec + GRADIENT_TIMEOUT)){

      // Expired, delete it
      DiffPrint(DEBUG_NO_DETAILS,
		"Deleting gradient to node %d !\n",
		gradient_entry->node_id_);
      gradient_itr = gradients_.erase(gradient_itr);
      delete gradient_entry;
    }
  }
}

void RoundIdEntry::updateSink(u_int16_t sink_id)
{
  SinkList::iterator sink_itr;
  SinkEntry *sink_entry;

  // Go through all sinks
  for (sink_itr = sinks_.begin(); sink_itr != sinks_.end(); ++sink_itr){
    sink_entry = *sink_itr;

    if (sink_entry->port_ == sink_id){
      // We already have this guy
      GetTime(&(sink_entry->tv_));
      return;
    }
  }

  // This is a new sink, so we create a new entry on the list
  sink_entry = new SinkEntry(sink_id);
  sinks_.push_back(sink_entry);
}

OPPGradientEntry * RoundIdEntry::findGradient(int32_t node_id)
{
  GradientList::iterator gradient_itr;
  OPPGradientEntry *gradient_entry;

  // Go through all gradients
  for (gradient_itr = gradients_.begin();
       gradient_itr != gradients_.end(); gradient_itr++){
    gradient_entry = *gradient_itr;

    // Is this the one we are looking for ?
    if (gradient_entry->node_id_ == node_id)
      return gradient_entry;
  }

  // Did not find a match
  return NULL;
}

void RoundIdEntry::addGradient(int32_t node_id)
{
  OPPGradientEntry *gradient_entry;

  // Create new gradient
  gradient_entry = new OPPGradientEntry(node_id);
  gradients_.push_back(gradient_entry);
}

void RoundIdEntry::deleteGradient(int32_t node_id)
{
  GradientList::iterator gradient_itr;
  OPPGradientEntry *gradient_entry;

  // Go through all gradients
  for (gradient_itr = gradients_.begin();
       gradient_itr != gradients_.end(); gradient_itr++){
    gradient_entry = *gradient_itr;

    // Is this the one we are looking for ?
    if (gradient_entry->node_id_ == node_id){
  
      DiffPrint(DEBUG_NO_DETAILS, "Deleting gradient to node %d !\n",
		node_id);

      // Found. Delete it from the list and return
      gradient_itr = gradients_.erase(gradient_itr);
      delete gradient_entry;
      return;
    }
  }
}

void OnePhasePullFilter::interestTimeout(Message *msg)
{
  DiffPrint(DEBUG_MORE_DETAILS, "Node%d: Interest Timeout !\n", ((DiffusionRouting *)dr_)->getNodeId());

  msg->last_hop_ = LOCALHOST_ADDR;
  msg->next_hop_ = BROADCAST_ADDR;
 
  ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);
}

void OnePhasePullFilter::messageTimeout(Message *msg)
{
  DiffPrint(DEBUG_MORE_DETAILS, "Node%d: Message Timeout !\n", ((DiffusionRouting *)dr_)->getNodeId());

  ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);
}

void OnePhasePullFilter::gradientTimeout()
{
  RoutingTable::iterator routing_itr;
  RoutingEntry *routing_entry;

  DiffPrint(DEBUG_MORE_DETAILS, "Node%d: Gradient Timeout !\n",((DiffusionRouting *)dr_)->getNodeId());

  routing_itr = routing_list_.begin();

  // Iterate through the routing table
  for (routing_itr = routing_list_.begin();
       routing_itr != routing_list_.end(); routing_itr++){
    routing_entry = *routing_itr;

    // Step 1: Delete expired round ids
    routing_entry->deleteExpiredRoundIds();

    // Step 2: Remove the routing entry if no round ids left
    if (routing_entry->round_ids_.size() == 0){

      // Deleting Routing Entry
      DiffPrint(DEBUG_DETAILS,
		"Nothing left for this data type, cleaning up !\n");
      routing_itr = routing_list_.erase(routing_itr);
      delete routing_entry;
    }
  }
}

void OnePhasePullFilter::reinforcementTimeout()
{
  DataNeighborList::iterator data_neighbor_itr;
  OPPDataNeighborEntry *data_neighbor_entry;
  RoutingTable::iterator routing_itr;
  RoutingEntry *routing_entry;
  Message *my_message;

  DiffPrint(DEBUG_MORE_DETAILS, "Reinforcement Timeout !\n");

  routing_itr = routing_list_.begin();

  while (routing_itr != routing_list_.end()){
    routing_entry = *routing_itr;

    // Step 1: Delete expired gradients
    data_neighbor_itr = routing_entry->data_neighbors_.begin();

    while (data_neighbor_itr != routing_entry->data_neighbors_.end()){
      data_neighbor_entry = *data_neighbor_itr;

      if ((!data_neighbor_entry->new_messages_) &&
	  (data_neighbor_entry->messages_ > 0)){
	my_message = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
				 0, 0, routing_entry->attrs_->size(), pkt_count_,
				 random_id_, data_neighbor_entry->node_id_,
				 LOCALHOST_ADDR);
	my_message->msg_attr_vec_ = CopyAttrs(routing_entry->attrs_);

	DiffPrint(DEBUG_NO_DETAILS,
		  "Node%d: Sending Negative Reinforcement to node %d !\n",
		  ((DiffusionRouting *)dr_)->getNodeId(), data_neighbor_entry->node_id_);

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
      if (data_neighbor_entry->messages_ > 0){
	data_neighbor_entry->messages_ = 0;
	data_neighbor_entry->new_messages_ = false;
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

int OnePhasePullFilter::subscriptionTimeout(NRAttrVec *attrs)
{
  SubscriptionList::iterator subscription_itr;
  SubscriptionEntry *subscription_entry;
  RoutingEntry *routing_entry;
  struct timeval tmv;

  DiffPrint(DEBUG_MORE_DETAILS, "Subscription Timeout !\n");

  GetTime(&tmv);

  // Find the correct Routing entry
  routing_entry = findRoutingEntry(attrs);

  if (routing_entry){
    // Routing entry found

    subscription_itr = routing_entry->subscription_list_.begin();

    // Go through all attributes
    while (subscription_itr != routing_entry->subscription_list_.end()){
      subscription_entry = *subscription_itr;

      // Check timeouts
      if (tmv.tv_sec > (subscription_entry->tv_.tv_sec + SUBSCRIPTION_TIMEOUT)){

	// Time expired, send disinterest message
	sendDisinterest(subscription_entry->attrs_, routing_entry);
	subscription_itr = routing_entry->subscription_list_.erase(subscription_itr);
	delete subscription_entry;
      }
      else{
	subscription_itr++;
      }
    }
  }
  else{
    DiffPrint(DEBUG_DETAILS, "Warning: Could't find subscription entry - maybe deleted by GradientTimeout ?\n");

    // Cancel Timer
    return -1;
  }

  // Keep Timer
  return 0;
}

void OnePhasePullFilter::deleteRoutingEntry(RoutingEntry *routing_entry)
{
  RoutingTable::iterator routing_itr;
  RoutingEntry *current_entry;

  // Go through the routing table
  for (routing_itr = routing_list_.begin(); routing_itr != routing_list_.end(); ++routing_itr){
    current_entry = *routing_itr;

    // Is this the entry we are looking for ?
    if (current_entry == routing_entry){
      routing_itr = routing_list_.erase(routing_itr);
      delete routing_entry;
      return;
    }
  }
  DiffPrint(DEBUG_ALWAYS, "Error: Could not find entry to delete !\n");
}

RoutingEntry * OnePhasePullFilter::matchRoutingEntry(NRAttrVec *attrs, RoutingTable::iterator start, RoutingTable::iterator *place)
{
  RoutingTable::iterator routing_itr;
  RoutingEntry *routing_entry;

  for (routing_itr = start; routing_itr != routing_list_.end(); ++routing_itr){
    routing_entry = *routing_itr;
    if (MatchAttrs(routing_entry->attrs_, attrs)){
      *place = routing_itr;
      return routing_entry;
    }
  }
  return NULL;
}

RoutingEntry * OnePhasePullFilter::findRoutingEntry(NRAttrVec *attrs)
{
  RoutingTable::iterator routing_itr;
  RoutingEntry *routing_entry;

  for (routing_itr = routing_list_.begin(); routing_itr != routing_list_.end(); ++routing_itr){
    routing_entry = *routing_itr;
    if (PerfectMatch(routing_entry->attrs_, attrs))
      return routing_entry;
  }
  return NULL;
}

SubscriptionEntry * OnePhasePullFilter::findMatchingSubscription(RoutingEntry *routing_entry,
								 NRAttrVec *attrs)
{
  SubscriptionList::iterator subscription_itr;
  SubscriptionEntry *subscription_entry;

  for (subscription_itr = routing_entry->subscription_list_.begin(); subscription_itr != routing_entry->subscription_list_.end(); ++subscription_itr){
    subscription_entry = *subscription_itr;
    if (PerfectMatch(subscription_entry->attrs_, attrs))
      return subscription_entry;
  }
  return NULL;
}

void OnePhasePullFilter::forwardData(Message *msg,
				     RoutingEntry *routing_entry,
				     DataForwardingHistory *forwarding_history)
{
  NRSimpleAttribute<void *> *nr_data_attr = NULL;
  NRAttrVec::iterator attribute_iterator;
  FlowIdList msg_flow_list, sinks_flow_list, local_flow_list;
  FlowIdList out_flow_list;
  int32_t out_neighbor;
  int *packed_flows;
  FlowIdList::iterator flow_id_itr;
  RoundIdList::iterator round_id_itr;
  SinkList::iterator sink_itr;
  RoundIdEntry *round_id_entry;
  SinkEntry *sink_entry;
  Message *sink_message, *out_message;

  // Step 0: Read flows from message

  // Find NRFlowAttr and remove from the message
  attribute_iterator = msg->msg_attr_vec_->begin();
  nr_data_attr = NRFlowAttr.find_from(msg->msg_attr_vec_,
				      attribute_iterator,
				      &attribute_iterator);

  if (!nr_data_attr){
    DiffPrint(DEBUG_ALWAYS, "Cannot find NRFlowAttr !\n");
    return;
  }

  msg->msg_attr_vec_->erase(attribute_iterator);

  // Read flow ids from list
  readFlowsFromList(nr_data_attr->getLen() / sizeof(int),
		    &msg_flow_list, nr_data_attr->getVal());

  // Fill lists of sinks and flows
  routing_entry->getSinksFromList(&msg_flow_list, &sinks_flow_list);
  routing_entry->getFlowsFromList(&msg_flow_list, &local_flow_list);

  // Step 1: Sink Processing
  if (sinks_flow_list.size() > 0){

    // Copy original message so we can change it
    sink_message = CopyMessage(msg);

    // Go through all rounds
    for (round_id_itr = routing_entry->round_ids_.begin();
	 round_id_itr != routing_entry->round_ids_.end();
	 round_id_itr++){

      round_id_entry = *round_id_itr;

      flow_id_itr = find(sinks_flow_list.begin(), sinks_flow_list.end(),
			 round_id_entry->round_id_);

      if (flow_id_itr != sinks_flow_list.end()){
	// Flows match ! Send message to sink

	for (sink_itr = round_id_entry->sinks_.begin();
	     sink_itr != round_id_entry->sinks_.end(); ++sink_itr){
	  sink_entry = *sink_itr;

	  if (!forwarding_history->alreadyForwardedToLibrary(sink_entry->port_)){
	    // Forward DATA to local sinks
	    sink_message->next_hop_ = LOCALHOST_ADDR;
	    sink_message->next_port_ = sink_entry->port_;

	    // Add sink to the forwarding list
	    forwarding_history->forwardingToLibrary(sink_entry->port_);

	    ((DiffusionRouting *)dr_)->sendMessage(sink_message, filter_handle_);
	  }
	}

	// Remove sink from the flow_list
	if (!removeFlowFromList(&msg_flow_list, round_id_entry->round_id_)){
	  // We should not get here
	  DiffPrint(DEBUG_ALWAYS, "Cannot remove flow from msg_flow_list !\n");
	}
      }
    }

    // Delete sink message
    delete sink_message;
  }

  // Step 2: Intermediate Processing
  DiffPrint(DEBUG_NO_DETAILS, "Node%d: Forwarding Data\n", ((DiffusionRouting *)dr_)->getNodeId());

  // Set reinforcement flags
  if (msg->last_hop_ != LOCALHOST_ADDR)
    routing_entry->updateNeighborDataInfo(msg->last_hop_, true);

  // Work on local list until we finish processing all flows
  while (local_flow_list.size() > 0){

    // Initialize out_flow_list
    out_flow_list.clear();

    // Move first flow from the local flow list to out_flow_list
    out_flow_list.push_back(*(local_flow_list.begin()));
    local_flow_list.erase(local_flow_list.begin());

    // Remove flow from the flow_list
    if (!removeFlowFromList(&msg_flow_list, *(out_flow_list.begin()))){
      // We should not get here
      DiffPrint(DEBUG_ALWAYS, "Cannot remove flow from msg_flow_list !\n");
    }

    // Select output_neighbor
    out_neighbor = routing_entry->getNeighborFromFlow(*(out_flow_list.begin()));

    // Must have a valid neighbor
    if (out_neighbor == BROADCAST_ADDR)
      continue;
    
    // Go through all other local flows
    for (flow_id_itr = local_flow_list.begin();
	 flow_id_itr != local_flow_list.end(); flow_id_itr++){

      // Check if output neighbor for this flow matches current
      if (routing_entry->getNeighborFromFlow(*flow_id_itr) == out_neighbor){

	// Yes it does !

	// Remove flow from the flow_list
	if (!removeFlowFromList(&msg_flow_list, *flow_id_itr)){
	  // We should not get here
	  DiffPrint(DEBUG_ALWAYS, "Cannot remove flow from msg_flow_list !\n");
	}

	// Aggregate both in a single message
	out_flow_list.push_back(*flow_id_itr);
	flow_id_itr = local_flow_list.erase(flow_id_itr);

      }
    }

    // out_flow_list should have a list of flow for out_neighbor
    out_message = CopyMessage(msg);
    out_message->next_hop_ = out_neighbor;

    packed_flows = writeFlowsToList(&out_flow_list);

    out_message->msg_attr_vec_->push_back(NRFlowAttr.make(NRAttribute::IS,
							  (void *) packed_flows,
							  sizeof(int) * out_flow_list.size()));

    // NRFlowAttr.make will copy this, so we must delete it
    delete [] packed_flows;

    // Send it out
    DiffPrint(DEBUG_NO_DETAILS, "Forwarding data to node %d !\n",
	      out_neighbor);

    ((DiffusionRouting *)dr_)->sendMessage(out_message, filter_handle_);

    // Delete message
    delete out_message;
  }

  // Done processing for this data type, we replace the NRFlowAttr
  // with the (possibly) shorter msg_flow_list list
  packed_flows = writeFlowsToList(&msg_flow_list);

  nr_data_attr->setVal((void *) packed_flows, sizeof(int) * msg_flow_list.size());
  msg->msg_attr_vec_->push_back(nr_data_attr);

  // setVal makes a copy of this, so we must delete it
  delete [] packed_flows;
}

void OnePhasePullFilter::sendInterest(NRAttrVec *attrs,
				      RoutingEntry *routing_entry)
{
  RoundIdList::iterator round_id_itr;
  RoundIdEntry *round_id_entry;
  SinkList::iterator sink_itr;
  SinkEntry *sink_entry;

  Message *msg = new Message(DIFFUSION_VERSION, INTEREST, 0, 0,
			     attrs->size(), 0, 0, LOCALHOST_ADDR,
			     LOCALHOST_ADDR);

  msg->msg_attr_vec_ = CopyAttrs(attrs);

  // Go through all round ids
  for (round_id_itr = routing_entry->round_ids_.begin();
       round_id_itr != routing_entry->round_ids_.end(); round_id_itr++){
    round_id_entry = *round_id_itr;

    // Send interest message to all local sinks
    for (sink_itr = round_id_entry->sinks_.begin();
	 sink_itr != round_id_entry->sinks_.end(); ++sink_itr){

      sink_entry = *sink_itr;
      msg->next_port_ = sink_entry->port_;

      ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);
    }
  }

  delete msg;
}

void OnePhasePullFilter::sendDisinterest(NRAttrVec *attrs,
					 RoutingEntry *routing_entry)
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

void OnePhasePullFilter::readFlowsFromList(int number_of_flows,
					   FlowIdList *flow_list,
					   void *source_blob)
{
  int *current_flow;

  // Point to the beginning of the list
  current_flow = (int *) source_blob;

  for (int i = 0; i < number_of_flows; i++){
    flow_list->push_back(*current_flow);

    // Advance to next flow
    current_flow++;
  }
}

int * OnePhasePullFilter::writeFlowsToList(FlowIdList *flow_list)
{
  FlowIdList::iterator flow_itr;
  int number_of_flows;
  int *flows, *current;;

  number_of_flows = flow_list->size();
  flows = new int[number_of_flows];
  current = flows;

  for (flow_itr = flow_list->begin();
       flow_itr != flow_list->end(); flow_itr++){
    *current = *flow_itr;
    current++;
  }

  return flows;
}

bool OnePhasePullFilter::removeFlowFromList(FlowIdList *flow_list,
					    int32_t flow)
{
  FlowIdList::iterator flow_itr;

  flow_itr = find(flow_list->begin(), flow_list->end(), flow);

  if (flow_itr != flow_list->end()){
    flow_itr = flow_list->erase(flow_itr);
    return true;
  }

  return false;
}

void OnePhasePullFilter::addLocalFlowsToMessage(Message *msg)
{
  RoutingTable::iterator routing_itr;
  RoundIdList::iterator round_id_itr;
  RoutingEntry *routing_entry;
  RoundIdEntry *round_id_entry;
  FlowIdList local_flows;
  int *packed_flows;

  // First we loop through our routing entries
  for (routing_itr = routing_list_.begin();
       routing_itr != routing_list_.end(); routing_itr++){
    routing_entry = *routing_itr;

    // Now go through each round
    for (round_id_itr = routing_entry->round_ids_.begin();
	 round_id_itr != routing_entry->round_ids_.end();
	 round_id_itr++){
      round_id_entry = *round_id_itr;

      local_flows.push_back(round_id_entry->round_id_);
    }
  }
  
  packed_flows = writeFlowsToList(&local_flows);

  msg->msg_attr_vec_->push_back(NRFlowAttr.make(NRAttribute::IS,
						(void *) packed_flows,
						sizeof(int) * local_flows.size()));

  // NRFlowAttr.make will copy this, so we must delete it here
  delete [] packed_flows;
  local_flows.clear();
}

void OnePhasePullFilter::recv(Message *msg, handle h)
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

void OnePhasePullFilter::processOldMessage(Message *msg)
{
  NRSimpleAttribute<int> *nrsubscription = NULL;
  NRAttrVec::iterator attribute_iterator;
  RoutingTable::iterator routing_itr;
  RoutingEntry *routing_entry;
  int32_t round_id;

  switch (msg->msg_type_){

  case INTEREST:

    DiffPrint(DEBUG_NO_DETAILS, "Node%d: Received Old Interest !\n",((DiffusionRouting *)dr_)->getNodeId());

    if (msg->last_hop_ == LOCALHOST_ADDR){
      // Old interest should not come from local sink
      DiffPrint(DEBUG_ALWAYS, "Warning: Old Interest from local sink !\n");
      break;
    }

    // Step 0: Take out the subscription attribute
    attribute_iterator = msg->msg_attr_vec_->begin();
    nrsubscription = NRSubscriptionAttr.find_from(msg->msg_attr_vec_,
						  attribute_iterator,
						  &attribute_iterator);

    // Return if we cannot find a subscription attribute
    if (!nrsubscription){
      DiffPrint(DEBUG_ALWAYS,
		"Warning: Can't find SUBSCRIPTION attribute in the message !\n");
      return;
    }

    // Delete attribute from the message
    msg->msg_attr_vec_->erase(attribute_iterator);

    // Get the routing entry for these attrs      
    routing_entry = findRoutingEntry(msg->msg_attr_vec_);

    if (routing_entry){

      // Use subscription id for identifying this flow
      round_id = nrsubscription->getVal();

      // Add gradient to the current round entry
      routing_entry->addGradient(msg->last_hop_, round_id, false);
    }

    // Add the subscription attribute back to the message
    msg->msg_attr_vec_->push_back(nrsubscription);

    break;

  case EXPLORATORY_DATA:
  case PUSH_EXPLORATORY_DATA:

    DiffPrint(DEBUG_ALWAYS, "Received and OLD EXPLORATORY message !\n");

    break;

  case DATA: 

    DiffPrint(DEBUG_NO_DETAILS, "Received an old Data message !\n");

    // Find the correct routing entry
    routing_itr = routing_list_.begin();
    routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				      &routing_itr);

    while (routing_entry){
      DiffPrint(DEBUG_NO_DETAILS,
		"Set flags to %d to OLD_MESSAGE !\n", msg->last_hop_);

      // Set reinforcement flags
      if (msg->last_hop_ != LOCALHOST_ADDR)
	routing_entry->updateNeighborDataInfo(msg->last_hop_, false);

      // Continue going through other data types
      routing_itr++;
      routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
					&routing_itr);
    }

    break;

   case NEGATIVE_REINFORCEMENT:

    DiffPrint(DEBUG_IMPORTANT, "Received an old Negative Reinforcement !\n");

    break;

  default:

    DiffPrint(DEBUG_ALWAYS,
	      "Received an unknown message type: %d\n", msg->msg_type_);

    break;
  }
}

void OnePhasePullFilter::processNewMessage(Message *msg)
{
  DataForwardingHistory *forwarding_history;
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;
  NRSimpleAttribute<int> *nrsubscription = NULL;
  RoundIdList::iterator round_id_itr;
  RoutingTable::iterator routing_itr;
  NRAttrVec::iterator attribute_iterator;
  RoundIdEntry *round_id_entry;
  RoutingEntry *routing_entry;
  SubscriptionEntry *subscription_entry;
  Message *my_msg;
  TimerCallback *interest_timer, *subscription_timer;
  bool new_data_type = false;
  int32_t round_id;

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

    // Step 0: Take out the subscription attribute
    attribute_iterator = msg->msg_attr_vec_->begin();
    nrsubscription = NRSubscriptionAttr.find_from(msg->msg_attr_vec_,
						  attribute_iterator,
						  &attribute_iterator);

    // Return if we cannot find a subscription attribute
    if (!nrsubscription){
      DiffPrint(DEBUG_ALWAYS,
		"Warning: Can't find SUBSCRIPTION attribute in the message !\n");
      return;
    }

    // Delete attribute from the message
    msg->msg_attr_vec_->erase(attribute_iterator);

    // Step 1: Look for the same data type
    routing_entry = findRoutingEntry(msg->msg_attr_vec_);

    if (!routing_entry){
      // Create a new routing entry for this data type
      routing_entry = new RoutingEntry;
      routing_entry->attrs_ = CopyAttrs(msg->msg_attr_vec_);
      routing_list_.push_back(routing_entry);
      new_data_type = true;
    }

    // Add the subscription attribute back to the message
    msg->msg_attr_vec_->push_back(nrsubscription);

    // Use subscription id for identifying this flow
    round_id = nrsubscription->getVal();

    if (msg->last_hop_ == LOCALHOST_ADDR){
      // From local sink
      routing_entry->updateSink(msg->source_port_, round_id);
    }
    else{
      // Interest received from the network. Add gradient to our
      // last_hop neighbor

      // Add gradient to the current round entry
      routing_entry->addGradient(msg->last_hop_, round_id, true);
    }

    if ((nrclass->getVal() == NRAttribute::INTEREST_CLASS) &&
	(nrclass->getOp() == NRAttribute::IS)){

      // Global interest messages should always be forwarded
      if (nrscope->getVal() == NRAttribute::GLOBAL_SCOPE){

	interest_timer = new OppInterestForwardTimer(this, CopyMessage(msg));

	((DiffusionRouting *)dr_)->addTimer(INTEREST_FORWARD_DELAY +
					    (int) ((INTEREST_FORWARD_JITTER * (GetRand() * 1.0 / RAND_MAX) - (INTEREST_FORWARD_JITTER / 2))),
					    interest_timer);
      }
    }
    else{
      if ((nrclass->getOp() != NRAttribute::IS) &&
	  (nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE) &&
	  (new_data_type)){

	subscription_timer = new OppSubscriptionExpirationTimer(this,
							     CopyAttrs(msg->msg_attr_vec_));
	
	((DiffusionRouting *)dr_)->addTimer(SUBSCRIPTION_DELAY +
					    (int) (SUBSCRIPTION_DELAY * (GetRand() * 1.0 / RAND_MAX)),
					    subscription_timer);
      }

      // Subscriptions don't have to match other subscriptions
      break;
    }

    // Step 2: Match interest against other subscriptions
    routing_itr = routing_list_.begin();
    routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
				      &routing_itr);

    while (routing_entry){
      // Got a match
      subscription_entry = findMatchingSubscription(routing_entry,
						    msg->msg_attr_vec_);

      // Do we already have this subscription
      if (subscription_entry){
	GetTime(&(subscription_entry->tv_));
      }
      else{
	// Create a new attribute entry, add it to the attribute list
	// and send an interest message to the local sink
	subscription_entry = new SubscriptionEntry(CopyAttrs(msg->msg_attr_vec_));
	routing_entry->subscription_list_.push_back(subscription_entry);
	sendInterest(subscription_entry->attrs_, routing_entry);
      }
      // Move to the next RoutingEntry
      routing_itr++;
      routing_entry = matchRoutingEntry(msg->msg_attr_vec_, routing_itr,
					&routing_itr);
    }

      break;

  case EXPLORATORY_DATA:
  case PUSH_EXPLORATORY_DATA:

    DiffPrint(DEBUG_ALWAYS, "Node%d: Received EXPLORATORY Message !\n",((DiffusionRouting *)dr_)->getNodeId());

    break;

  case DATA:

    DiffPrint(DEBUG_NO_DETAILS, "Node%d: Received Data !\n",((DiffusionRouting *)dr_)->getNodeId());

    // Create data message forwarding cache
    forwarding_history = new DataForwardingHistory;

    // If message comes from local source, we include our local flows
    if (msg->last_hop_ == LOCALHOST_ADDR){
      // From local source
      addLocalFlowsToMessage(msg);
    }

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

  case NEGATIVE_REINFORCEMENT:

    DiffPrint(DEBUG_NO_DETAILS, "Received a Negative Reinforcement !\n");

    // Find matching routing entry
    routing_entry = findRoutingEntry(msg->msg_attr_vec_);

    if (routing_entry){

      // Go through all round ids
      for (round_id_itr = routing_entry->round_ids_.begin();
	   round_id_itr != routing_entry->round_ids_.end();
	   round_id_itr++){

	round_id_entry = *round_id_itr;

	// Delete gradient to last hop
	round_id_entry->deleteGradient(msg->last_hop_);

	// Delete round id entry if nothing left
	if (round_id_entry->gradients_.size() == 0){

	  round_id_itr = routing_entry->round_ids_.erase(round_id_itr);
	  delete round_id_entry;
	}

      }

      // If there are no other gradients we need to send our own
      // negative reinforcement
      if (routing_entry->round_ids_.size() == 0){
	my_msg = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
			     0, 0, routing_entry->attrs_->size(), pkt_count_,
			     random_id_, BROADCAST_ADDR, LOCALHOST_ADDR);
	my_msg->msg_attr_vec_ = CopyAttrs(routing_entry->attrs_);

	DiffPrint(DEBUG_NO_DETAILS,
		  "Broadcasting Negative Reinforcement !\n");

	((DiffusionRouting *)dr_)->sendMessage(my_msg, filter_handle_);

	pkt_count_++;
	delete my_msg;
      }
    }

    break;

  default:

    break;
  }
}

handle OnePhasePullFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // For the One-Phase Pull filter, we set up a filter to receive
  // messages using this protocol
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::EQ,
				       NRAttribute::ONE_PHASE_PULL_ALGORITHM));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs,
					   ONE_PHASE_PULL_FILTER_PRIORITY,
					   filter_callback_);

  ClearAttrs(&attrs);
  return h;
}

#ifndef NS_DIFFUSION
void OnePhasePullFilter::run()
{
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
}
#endif // !NS_DIFFUSION

#ifdef NS_DIFFUSION
OnePhasePullFilter::OnePhasePullFilter(const char *diffrtg)
{
  DiffAppAgent *agent;
#else
OnePhasePullFilter::OnePhasePullFilter(int argc, char **argv)
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
  filter_callback_ = new OnePhasePullFilterReceive(this);

  // Set up the filter
  filter_handle_ = setupFilter();

  // Print filter information
  DiffPrint(DEBUG_IMPORTANT, "One-Phase Pull filter received handle %d\n",
	    filter_handle_);

  // Add timers for keeping state up-to-date
  gradient_timer = new OppGradientExpirationCheckTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(GRADIENT_DELAY, gradient_timer);

  reinforcement_timer = new OppReinforcementCheckTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(REINFORCEMENT_DELAY, reinforcement_timer);

  GetTime(&tv);

  DiffPrint(DEBUG_ALWAYS,
	    "One-Phase Pull filter initialized at time %ld:%ld!\n",
	    tv.tv_sec, tv.tv_usec);
}

#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  OnePhasePullFilter *app;

  // Initialize and run the Gradient Filter
  app = new OnePhasePullFilter(argc, argv);
  app->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE
