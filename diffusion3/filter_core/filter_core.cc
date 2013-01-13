// 
// filter_core.cc  : Main Diffusion program
// authors         : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: filter_core.cc,v 1.6 2011/10/02 22:32:34 tom_henderson Exp $
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

#include "filter_core.hh"

#ifndef NS_DIFFUSION
DiffusionCoreAgent *agent;
#endif // !NS_DIFFUSION

class HashEntry {
public:
  bool dummy;

  HashEntry() { 
    dummy  = false;
  }
};

class NeighborEntry {
public:
  int32_t id;
  struct timeval tmv;

  NeighborEntry(int _id) : id(_id)
  {
    GetTime(&tmv);
  }
};

int NeighborsTimeoutTimer::expire()
{
  agent_->neighborsTimeout();

  return 0;
}

int FilterTimeoutTimer::expire()
{
  agent_->filterTimeout();

  return 0;
}

int DiffusionStopTimer::expire()
{
  agent_->timeToStop();
#ifndef NS_DIFFUSION
  exit(0);
#endif // !NS_DIFFUSION

  // Never gets here !
  return 0;
}

void DiffusionCoreAgent::timeToStop()
{
#ifdef STATS
  char out_filename[100];
  FILE *outfile = NULL;

  if (stats_){
    sprintf(out_filename, "/tmp/diffusion-%d.out", my_id_);
    outfile = fopen(out_filename, "w");

    if (outfile == NULL){
      DiffPrint(DEBUG_ALWAYS,
		"Diffusion Error: Cannot create %s\n", out_filename);
      return;
    }

    stats_->printStats(stdout);
    if (outfile){
      stats_->printStats(outfile);
      fclose(outfile);
    }
  }
#endif // STATS
}

#ifndef NS_DIFFUSION

void signal_handler(int p)
{
  agent->timeToStop();
  exit(0);
}

void DiffusionCoreAgent::usage(char *s)
{

  DiffPrint(DEBUG_ALWAYS, "Usage: %s [-d debug] [-f filename] [-t stoptime] [-v] [-h] [-p port]", s);
#ifdef IO_LOG
  DiffPrint(DEBUG_ALWAYS, " [-l]");
#endif // IO_LOG
#ifdef STATS
  DiffPrint(DEBUG_ALWAYS, " [-s] [-i warm_up_time]");
#endif // STATS
  DiffPrint(DEBUG_ALWAYS, "\n\n");
  DiffPrint(DEBUG_ALWAYS, "\t-d - Sets debug level (0-10)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-t - Stops after stoptime seconds\n");
  DiffPrint(DEBUG_ALWAYS, "\t-f - Uses filename as the config file\n");
  DiffPrint(DEBUG_ALWAYS, "\t-v - Prints diffusion version\n");
  DiffPrint(DEBUG_ALWAYS, "\t-h - Prints this information\n");
  DiffPrint(DEBUG_ALWAYS, "\t-p - Sets diffusion port to port\n");
#ifdef IO_LOG
  DiffPrint(DEBUG_ALWAYS, "\t-l - Turns on i/o logging\n");
#endif // IO_LOG
#ifdef STATS
  DiffPrint(DEBUG_ALWAYS, "\t-s - Disables statistics\n");
  DiffPrint(DEBUG_ALWAYS, "\t-i - Ignores traffic from the first warm_up_time seconds for stats\n");
#endif // STATS

  DiffPrint(DEBUG_ALWAYS, "\n");

  exit(0);
}

void DiffusionCoreAgent::run()
{
  DeviceList::iterator device_itr;
  DiffPacket in_pkt;
  fd_set fds;
  bool flag;
  int status, max_sock, fd;
  struct timeval tv;

  // Main Select Loop
  while (1){

    // Wait for incoming packets
    FD_ZERO(&fds);
    max_sock = 0;

    // Figure out how much time to wait
    timers_manager_->nextTimerTime(&tv);
    if (tv.tv_sec == 0 && tv.tv_usec == 0){
      // Timer has expired !
      timers_manager_->executeAllExpiredTimers();
      continue;
    }

    for (device_itr = in_devices_.begin();
	 device_itr != in_devices_.end(); ++device_itr){
      (*device_itr)->addInFDS(&fds, &max_sock);
    }

    status = select(max_sock+1, &fds, NULL, NULL, &tv);

    if (status == 0){
      // We process all expired timers
      timers_manager_->executeAllExpiredTimers();
    }

    // Check for new packets
    if (status > 0){
      do{
	flag = false;
	for (device_itr = in_devices_.begin();
	     device_itr != in_devices_.end(); ++device_itr){
	  fd = (*device_itr)->checkInFDS(&fds);
	  if (fd != -1){
	    // Message waiting
	    in_pkt = (*device_itr)->recvPacket(fd);

	    if (in_pkt)
	      recvPacket(in_pkt);

	    // Clear this fd
	    FD_CLR(fd, &fds);
	    status--;
	    flag = true;
	  }
	}
      } while ((status > 0) && (flag == true));
    }

    // This should not happen
    if (status < 0){
      DiffPrint(DEBUG_IMPORTANT, "Select returned %d\n", status);
    }
  }
}
#endif // !NS_DIFFUSION

void DiffusionCoreAgent::neighborsTimeout()
{
  struct timeval tmv;
  NeighborEntry *neighbor_entry;
  NeighborList::iterator neighbor_itr;

  DiffPrint(DEBUG_MORE_DETAILS, "Neighbors Timeout !\n");

  GetTime(&tmv);

  neighbor_itr = neighbor_list_.begin();

  while(neighbor_itr != neighbor_list_.end()){
    neighbor_entry = *neighbor_itr;
    if (tmv.tv_sec > neighbor_entry->tmv.tv_sec + NEIGHBORS_TIMEOUT){
      // This neighbor expired
      neighbor_itr = neighbor_list_.erase(neighbor_itr);
      delete neighbor_entry;
    }
    else{
      neighbor_itr++;
    }
  }
}

void DiffusionCoreAgent::filterTimeout()
{
  struct timeval tmv;
  FilterEntry *filter_entry;
  FilterList::iterator filter_itr;

  DiffPrint(DEBUG_MORE_DETAILS, "Filter Timeout !\n");

  GetTime(&tmv);

  filter_itr = filter_list_.begin();

  while(filter_itr != filter_list_.end()){
    filter_entry = *filter_itr;
    if (tmv.tv_sec > filter_entry->tmv_.tv_sec + FILTER_TIMEOUT){

      // This filter expired
      DiffPrint(DEBUG_NO_DETAILS, "Filter %d, %d, %d timed out !\n",
		filter_entry->agent_, filter_entry->handle_,
		filter_entry->priority_);
      filter_itr = filter_list_.erase(filter_itr);
      delete filter_entry;
    }
    else{
      filter_itr++;
    }
  }
}

void DiffusionCoreAgent::sendMessage(Message *msg)
{
  Tcl_HashEntry *tcl_hash_entry;
  unsigned int key[2];
  Message *send_message;
  
  send_message = new Message(DIFFUSION_VERSION, msg->msg_type_, diffusion_port_,
			     0, 0, msg->pkt_num_, msg->rdm_id_,
			     msg->next_hop_, 0);

  send_message->msg_attr_vec_ = CopyAttrs(msg->msg_attr_vec_);
  send_message->num_attr_ = send_message->msg_attr_vec_->size();
  send_message->data_len_ = CalculateSize(send_message->msg_attr_vec_);

  // Adjust message size for logging and check hash
  key[0] = msg->pkt_num_;
  key[1] = msg->rdm_id_;
  tcl_hash_entry = Tcl_FindHashEntry(&htable_, (char *) key);
  if (tcl_hash_entry)
    msg->new_message_ = 0;
  else
    msg->new_message_ = 1;

  send_message->new_message_ = msg->new_message_;

  // Check if message goes to an agent or the network
  if (msg->next_port_){
    // Message goes to an agent
    send_message->last_hop_ = LOCALHOST_ADDR;

    // If it's a local message, it has to go to a local agent
    if (send_message->next_hop_ != LOCALHOST_ADDR){
      DiffPrint(DEBUG_ALWAYS, "Error: Message destination is a local agent but next_hop != LOCALHOST_ADDR !\n");
      delete send_message;
      return;
    }

    // Send the message to the agent specified
    sendMessageToLibrary(send_message, msg->next_port_);
  }
  else{
    // Message goes to the network
    send_message->last_hop_ = my_id_;

#ifdef STATS
    if (stats_)
      stats_->logOutgoingMessage(send_message);
#endif // STATS

    // Add message to the hash table      
    if (tcl_hash_entry == NULL)
      putHash(key[0], key[1]);
    else
      DiffPrint(DEBUG_DETAILS, "Node%d: Message being sent is an old message !\n", my_id_);

    // Send Message
    sendMessageToNetwork(send_message);
  }

  delete send_message;
}

void DiffusionCoreAgent::forwardMessage(Message *msg, FilterEntry *filter_entry)
{
  RedirectMessage *original_hdr;
  NRAttribute *original_header_attr;
  Message *send_message;

  // Create an attribute with the original header
  original_hdr = new RedirectMessage(msg->new_message_, msg->msg_type_,
				     msg->source_port_, msg->data_len_,
				     msg->num_attr_, msg->rdm_id_,
				     msg->pkt_num_, msg->next_hop_,
				     msg->last_hop_, filter_entry->handle_,
				     msg->next_port_);

  original_header_attr = OriginalHdrAttr.make(NRAttribute::IS,
					      (void *)original_hdr,
					      sizeof(RedirectMessage));

  send_message = new Message(DIFFUSION_VERSION, REDIRECT, diffusion_port_, 0,
			     0, pkt_count_, random_id_, LOCALHOST_ADDR, my_id_);

  // Increment pkt_counter
  pkt_count_++;

  // Duplicate the message's attributes
  send_message->msg_attr_vec_ = CopyAttrs(msg->msg_attr_vec_);
  
  // Add the extra attribute
  send_message->msg_attr_vec_->push_back(original_header_attr);
  send_message->num_attr_ = send_message->msg_attr_vec_->size();
  send_message->data_len_ = CalculateSize(send_message->msg_attr_vec_);

  sendMessageToLibrary(send_message, filter_entry->agent_);

  delete send_message;
  delete original_hdr;
}

#ifndef NS_DIFFUSION
void DiffusionCoreAgent::sendMessageToLibrary(Message *msg, u_int16_t agent_id)
{
  DiffPacket out_pkt = NULL;
  struct hdr_diff *dfh;
  int len;
  char *pos;

  out_pkt = AllocateBuffer(msg->msg_attr_vec_);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(msg->msg_attr_vec_, pos);

  LAST_HOP(dfh) = htonl(msg->last_hop_);
  NEXT_HOP(dfh) = htonl(msg->next_hop_);
  DIFF_VER(dfh) = msg->version_;
  MSG_TYPE(dfh) = msg->msg_type_;
  DATA_LEN(dfh) = htons(len);
  PKT_NUM(dfh) = htonl(msg->pkt_num_);
  RDM_ID(dfh) = htonl(msg->rdm_id_);
  NUM_ATTR(dfh) = htons(msg->num_attr_);
  SRC_PORT(dfh) = htons(msg->source_port_);

  sendPacketToLibrary(out_pkt, sizeof(struct hdr_diff) + len, agent_id);

  delete [] out_pkt;
}
#else
void DiffusionCoreAgent::sendMessageToLibrary(Message *msg, u_int16_t agent_id)
{
  Message *send_message;
  DeviceList::iterator device_itr;
  int len;

  send_message = CopyMessage(msg);
  len = CalculateSize(send_message->msg_attr_vec_);
  len = len + sizeof(struct hdr_diff);

  for (device_itr = local_out_devices_.begin();
       device_itr != local_out_devices_.end(); ++device_itr){
    (*device_itr)->sendPacket((DiffPacket) send_message, len, agent_id);
  }
}
#endif // !NS_DIFFUSION

#ifndef NS_DIFFUSION
void DiffusionCoreAgent::sendMessageToNetwork(Message *msg)
{
  DiffPacket out_pkt = NULL;
  struct hdr_diff *dfh;
  int len;
  char *pos;

  out_pkt = AllocateBuffer(msg->msg_attr_vec_);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(msg->msg_attr_vec_, pos);

  LAST_HOP(dfh) = htonl(msg->last_hop_);
  NEXT_HOP(dfh) = htonl(msg->next_hop_);
  DIFF_VER(dfh) = msg->version_;
  MSG_TYPE(dfh) = msg->msg_type_;
  DATA_LEN(dfh) = htons(len);
  PKT_NUM(dfh) = htonl(msg->pkt_num_);
  RDM_ID(dfh) = htonl(msg->rdm_id_);
  NUM_ATTR(dfh) = htons(msg->num_attr_);
  SRC_PORT(dfh) = htons(msg->source_port_);

  sendPacketToNetwork(out_pkt, sizeof(struct hdr_diff) + len, msg->next_hop_);

  delete [] out_pkt;
}
#else
void DiffusionCoreAgent::sendMessageToNetwork(Message *msg)
{
  Message *send_message;
  int len;
  int32_t dst;
  DeviceList::iterator device_itr;

  send_message = CopyMessage(msg);
  len = CalculateSize(send_message->msg_attr_vec_);
  len = len + sizeof(struct hdr_diff);
  dst = send_message->next_hop_;

  for (device_itr = out_devices_.begin();
       device_itr != out_devices_.end(); ++device_itr){
    (*device_itr)->sendPacket((DiffPacket) send_message, len, dst);
  }
}
#endif // !NS_DIFFUSION

void DiffusionCoreAgent::sendPacketToLibrary(DiffPacket pkt, int len,
					     u_int16_t dst)
{
  DeviceList::iterator device_itr;

  for (device_itr = local_out_devices_.begin();
       device_itr != local_out_devices_.end(); ++device_itr){
    (*device_itr)->sendPacket(pkt, len, dst);
  }
}

void DiffusionCoreAgent::sendPacketToNetwork(DiffPacket pkt, int len, int dst)
{
  DeviceList::iterator device_itr;

  for (device_itr = out_devices_.begin();
       device_itr != out_devices_.end(); ++device_itr){
    (*device_itr)->sendPacket(pkt, len, dst);
  }
}

void DiffusionCoreAgent::updateNeighbors(int id)
{
  NeighborList::iterator neighbor_itr;
  NeighborEntry *neighbor_entry;

  if (id == LOCALHOST_ADDR || id == my_id_)
    return;

  for (neighbor_itr = neighbor_list_.begin();
       neighbor_itr != neighbor_list_.end(); ++neighbor_itr){
    if ((*neighbor_itr)->id == id)
      break;
  }

  if (neighbor_itr == neighbor_list_.end()){
    // This is a new neighbor
    neighbor_entry = new NeighborEntry(id);
    neighbor_list_.push_front(neighbor_entry);
  }
  else{
    // Just update the neighbor timeout
    GetTime(&((*neighbor_itr)->tmv));
  }
}

FilterEntry * DiffusionCoreAgent::findFilter(int16_t handle, u_int16_t agent)
{
  FilterList::iterator filter_itr;
  FilterEntry *filter_entry;

  for (filter_itr = filter_list_.begin();
       filter_itr != filter_list_.end(); ++filter_itr){
    filter_entry = *filter_itr;
    if (handle != filter_entry->handle_ || agent != filter_entry->agent_)
      continue;

    // Found
    return filter_entry;
  }
  return NULL;
}

FilterEntry * DiffusionCoreAgent::deleteFilter(int16_t handle, u_int16_t agent)
{
  FilterList::iterator filter_itr = filter_list_.begin();
  FilterEntry *filter_entry = NULL;

  while (filter_itr != filter_list_.end()){
    filter_entry = *filter_itr;
    if (handle == filter_entry->handle_ && agent == filter_entry->agent_){
      filter_list_.erase(filter_itr);
      break;
    }
    filter_entry = NULL;
    filter_itr++;
  }
  return filter_entry;
}

bool DiffusionCoreAgent::addFilter(NRAttrVec *attrs, u_int16_t agent,
				   int16_t handle, u_int16_t priority)
{
  FilterList::iterator filter_itr;
  FilterEntry *filter_entry;

  filter_itr = filter_list_.begin();
  while (filter_itr != filter_list_.end()){
    filter_entry = *filter_itr;
    if (filter_entry->priority_ == priority)
      return false;
    filter_itr++;
  }

  filter_entry = new FilterEntry(handle, priority, agent);

  // Copy the Attribute Vector
  filter_entry->filter_attrs_ = CopyAttrs(attrs);

  // Add this filter to the filter list
  filter_list_.push_back(filter_entry);

  return true;
}

FilterList::iterator DiffusionCoreAgent::findMatchingFilter(NRAttrVec *attrs,
							    FilterList::iterator filter_itr)
{
  FilterEntry *filter_entry;

  for (;filter_itr != filter_list_.end(); ++filter_itr){
    filter_entry = *filter_itr;

    if (OneWayMatch(filter_entry->filter_attrs_, attrs)){
      // That's a match !
      break;
    }
  }
  return filter_itr;
}

bool DiffusionCoreAgent::restoreOriginalHeader(Message *msg)
{
  NRAttrVec::iterator attr_itr = msg->msg_attr_vec_->begin();
  NRSimpleAttribute<void *> *original_header_attr = NULL;
  RedirectMessage *original_hdr;

  // Find original Header
  original_header_attr = OriginalHdrAttr.find_from(msg->msg_attr_vec_,
						   attr_itr, &attr_itr);
  if (!original_header_attr){
    DiffPrint(DEBUG_ALWAYS, "Error: DiffusionCoreAgent::ProcessControlMessage couldn't find the OriginalHdrAttr !\n");
    return false;
  }

  // Restore original Header
  original_hdr = (RedirectMessage *) original_header_attr->getVal();

  msg->msg_type_ = original_hdr->msg_type_;
  msg->source_port_ = original_hdr->source_port_;
  msg->pkt_num_ = original_hdr->pkt_num_;
  msg->rdm_id_ = original_hdr->rdm_id_;
  msg->next_hop_ = original_hdr->next_hop_;
  msg->last_hop_ = original_hdr->last_hop_;
  msg->new_message_ = original_hdr->new_message_;
  msg->num_attr_ = original_hdr->num_attr_;
  msg->data_len_ = original_hdr->data_len_;
  msg->next_port_ = original_hdr->next_port_;

  // Delete attribute from original set
  msg->msg_attr_vec_->erase(attr_itr);
  delete original_header_attr;

  return true;
}

FilterList * DiffusionCoreAgent::getFilterList(NRAttrVec *attrs)
{
  FilterList *matching_filter_list = new FilterList;
  FilterList::iterator known_filters_itr, filter_list_itr;
  FilterEntry *matching_filter_entry, *filter_entry;

  // We need to come up with a list of filters to call
  // F1 will be called before F2 if F1->priority > F2->priority

  known_filters_itr = findMatchingFilter(attrs, filter_list_.begin());

  while (known_filters_itr != filter_list_.end()){
    // We have a match !
    matching_filter_entry = *known_filters_itr;

    for (filter_list_itr = matching_filter_list->begin();
	 filter_list_itr != matching_filter_list->end(); ++filter_list_itr){
      filter_entry = *filter_list_itr;

      // Figure out where to insert 
      if (matching_filter_entry->priority_ > filter_entry->priority_)
	break;
    }

    // Insert matching filter in the list
    matching_filter_list->insert(filter_list_itr, matching_filter_entry);

    // Continue the search
    known_filters_itr++;
    known_filters_itr = findMatchingFilter(attrs, known_filters_itr);
  }
  return matching_filter_list;
}

u_int16_t DiffusionCoreAgent::getNextFilterPriority(int16_t handle,
						    u_int16_t priority,
						    u_int16_t agent)
{
  FilterList::iterator filter_itr;
  FilterEntry *filter_entry;

  if ((priority < FILTER_MIN_PRIORITY) ||
      (priority > FILTER_KEEP_PRIORITY))
    return FILTER_INVALID_PRIORITY;

  if (priority < FILTER_KEEP_PRIORITY)
    return (priority - 1);

  filter_itr = filter_list_.begin();

  while (filter_itr != filter_list_.end()){
    filter_entry = *filter_itr;

    if ((filter_entry->handle_ == handle) && (filter_entry->agent_ == agent)){
      // Found this filter
      return (filter_entry->priority_ - 1);
    }

    filter_itr++;
  }

  return FILTER_INVALID_PRIORITY;
}

void DiffusionCoreAgent::processMessage(Message *msg)
{
  FilterList *filter_list;
  FilterList::iterator filter_list_itr;
  FilterEntry *filter_entry;

  filter_list = getFilterList(msg->msg_attr_vec_);

  // Ok, we have a list of Filters to call. Send this message
  // to the first filter on this list
  if (filter_list->size() > 0){
    filter_list_itr = filter_list->begin();
    filter_entry = *filter_list_itr;

    forwardMessage(msg, filter_entry);
    filter_list->clear();
  }
  delete filter_list;
}

void DiffusionCoreAgent::processControlMessage(Message *msg)
{
  NRSimpleAttribute<void *> *ctrl_msg_attr = NULL;
  NRAttrVec::iterator attr_itr;
  ControlMessage *control_blob = NULL;
  FilterList *filter_list;
  FilterList::iterator filter_list_itr;
  FilterEntry *filter_entry;
  int command, param1, param2;
  u_int16_t priority, source_port, new_priority;
  int16_t handle;
  bool filter_is_last = false;

  // Control messages should not come from other nodes
  if (msg->last_hop_ != LOCALHOST_ADDR){
    DiffPrint(DEBUG_ALWAYS,
	      "Error: Received control message from another node !\n");
    return;
  }

  // Find the control attribute
  attr_itr = msg->msg_attr_vec_->begin();
  ctrl_msg_attr = ControlMsgAttr.find_from(msg->msg_attr_vec_,
					   attr_itr, &attr_itr);

  if (!ctrl_msg_attr){
    // Control message is invalid
    DiffPrint(DEBUG_ALWAYS, "Error: Control message received is invalid !\n");
    return;
  }

  // Extract the control message info
  control_blob = (ControlMessage *) ctrl_msg_attr->getVal();
  command = control_blob->command_;
  param1 = control_blob->param1_;
  param2 = control_blob->param2_;

  // Filter API definitions
  //
  // command = ADD_UPDATE_FILTER
  // param1  = priority
  // param2  = handle
  // attrs   = other attrs specify the filter
  // 
  // Remarks: If this filter is already present for this module,
  //          we don't create a new one. A filter is identified
  //          by the handle and the originating agent. The filter
  //          gets refreshed if it already exists. If attrs and
  //          handle are the same, we update the priority.
  //
  //
  // command = REMOVE_FILTER
  // param1  = handle
  //
  // Remarks: Remove the filter identified by (agent, handle)
  //          If it's not found, a warning message is generated.
  //
  //
  // Remarks: Send message from a local App to another App or
  //          a neighbor. If agent_id is zero, the packet goes
  //          out to the network. Otherwise, it goes to the
  //          agent_id located on this node.
  //
  //
  // command = SEND_MESSAGE
  // param1  = handle
  // param2  = priority
  //
  // Remarks: Send this message to the next filter or to a local
  //          application. We have to assemble the list again
  //          and figure out the current agent's position on the
  //          list. Then, we send to the next guy. If there is
  //          no other filter in the list, we try to send it to
  //          the network, if next_hop contains a node address.

  logControlMessage(msg, command, param1, param2);

  // First we remove the control attribute from the message
  msg->msg_attr_vec_->erase(attr_itr);
  delete ctrl_msg_attr;

  switch(command){
  case ADD_UPDATE_FILTER:

    priority = param1;
    handle = param2;

    filter_entry = findFilter(handle, msg->source_port_);

    if (filter_entry){
      // Filter already present, must be an update message
      if (PerfectMatch(filter_entry->filter_attrs_, msg->msg_attr_vec_)){
	// Attrs also match, let's update the filter's timeout
	GetTime(&(filter_entry->tmv_));

	// Check if the priority has changed...
	if (priority == filter_entry->priority_){
	  // Nothing to do !
	  DiffPrint(DEBUG_SOME_DETAILS, "Filter %d, %d, %d refreshed.\n",
		    filter_entry->agent_, filter_entry->handle_,
		    filter_entry->priority_);
	}
	else{
	  // Update the priority
	  DiffPrint(DEBUG_NO_DETAILS,
		    "Updated priority of filter %d, %d, %d to %d\n",
		    msg->source_port_, handle, filter_entry->priority_, priority);
	  filter_entry->priority_ = priority;
	}

	break;
      }
      else{
	// Filter attributes have changed ! This is not allowed !
	DiffPrint(DEBUG_ALWAYS,
		  "Filter attributes cannot change during an update !\n");
	break;
      }
    }
    else{
      // This is a new filter
      if (!addFilter(msg->msg_attr_vec_, msg->source_port_, handle, priority)){
	DiffPrint(DEBUG_ALWAYS, "Failed to add filter %d, %d, %d\n",
		  msg->source_port_, handle, priority);
      }
      else{
	DiffPrint(DEBUG_NO_DETAILS, "Adding filter %d, %d, %d\n",
		  msg->source_port_, handle, priority);
      }
    }

    break;

  case REMOVE_FILTER:

    handle = param1;
    filter_entry = deleteFilter(handle, msg->source_port_);
    if (filter_entry){
      // Filter deleted
      DiffPrint(DEBUG_NO_DETAILS, "Filter %d, %d, %d deleted.\n",
		filter_entry->agent_, filter_entry->handle_,
		filter_entry->priority_);

      delete filter_entry;
    }
    else{
      DiffPrint(DEBUG_ALWAYS, "Couldn't find filter to delete !\n");
    }

    break;

  case SEND_MESSAGE:

    handle = param1;
    priority = param2;
    source_port = msg->source_port_;

    if (!restoreOriginalHeader(msg))
      break;

    new_priority = getNextFilterPriority(handle, priority, source_port);

    if (new_priority == FILTER_INVALID_PRIORITY)
      break;

    // Now process the incoming message
    filter_list = getFilterList(msg->msg_attr_vec_);

    // Find the filter after the 'current' filter on the list
    if (filter_list->size() > 0){
      for (filter_list_itr = filter_list->begin();
	   filter_list_itr != filter_list->end(); ++filter_list_itr){
	filter_entry = *filter_list_itr;
	if (filter_entry->priority_ <= new_priority){
	  forwardMessage(msg, filter_entry);
	  break;
	}
      }

      if (filter_list_itr == filter_list->end())
	filter_is_last = true;

    }
    else{
      filter_is_last = true;
    }

    if (filter_is_last){
      // Forward message to the network or the destination application
      sendMessage(msg);
    }

    filter_list->clear();

    delete filter_list;

    break;

  case ADD_TO_BLACKLIST:

    DiffPrint(DEBUG_IMPORTANT, "Diffusion: Adding node %d to blacklist !\n",
	      param1);
    black_list_.push_front(param1);

    break;

  case CLEAR_BLACKLIST:

    DiffPrint(DEBUG_IMPORTANT, "Diffusion: Clearing blacklist !\n");

    black_list_.clear();

    break;

  default:

    DiffPrint(DEBUG_ALWAYS, "Error: Unknown control message received !\n");

    break;
  }
}

void DiffusionCoreAgent::logControlMessage(Message * , int , int , int )
{
  // Logs the incoming message
}

#ifdef NS_DIFFUSION
DiffusionCoreAgent::DiffusionCoreAgent(DiffRoutingAgent *diffrtg, int nodeid)
{
#else
DiffusionCoreAgent::DiffusionCoreAgent(int argc, char **argv)
{
  int opt;
  int debug_level;
#endif // NS_DIFFUSION
#ifdef IO_LOG
  DeviceList *in_devices;
#endif
  DeviceList *out_devices, *local_out_devices;
  DiffusionIO *device;
  TimerCallback *callback;
#ifdef USE_EMSIM
  char *sim_id = getenv("SIM_ID");
  char *sim_group = getenv("SIM_GROUP");
  int32_t group_id;
#endif // USE_EMSIM
  long stop_time;
  struct timeval tv;
#ifdef IO_LOG
  IOLog *pseudo_io_device;
  bool use_io_log = false;
#endif // IO_LOG
#ifdef STATS
  bool use_io_stats = true;
  int stats_warm_up_time = 0;
#endif // STATS
  //bool node_id_configured = false;

  opterr = 0;
  config_file_ = NULL;
  stop_time = 0;

  diffusion_port_ = DEFAULT_DIFFUSION_PORT;

#ifndef NS_DIFFUSION
  char *node_id_env;
  node_id_env = getenv("node_addr");
  // Parse command line options
  while (1){
    opt = getopt(argc, argv, COMMAND_LINE_ARGS);

    switch(opt){

    case 'p':

      diffusion_port_ = (u_int16_t) atoi(optarg);
      if ((diffusion_port_ < 1024) || (diffusion_port_ >= 65535)){
	DiffPrint(DEBUG_ALWAYS,
		  "Diffusion Error: Port must be between 1024 and 65535 !\n");
	exit(-1);
      }

      break;

    case 't':

      stop_time = atol(optarg);
      if (stop_time <= 0){
	DiffPrint(DEBUG_ALWAYS, "Diffusion Error: stop time must be > 0\n");
	exit(-1);
      }
      else{
	DiffPrint(DEBUG_ALWAYS, "%s will stop after %ld seconds\n",
		  PROGRAM, stop_time);
      }

      break;

#ifdef IO_LOG
    case 'l':

      use_io_log = true;

      break;

#endif // IO_LOG

#ifdef STATS
    case 's':

      use_io_stats = false;

      break;

    case 'i':

      stats_warm_up_time = atoi(optarg);
      if (stats_warm_up_time < 0){
	DiffPrint(DEBUG_ALWAYS, "Diffusion Error: warm_up_time must be > 0\n");
	exit(-1);
      }

      break;
#endif // STATS

    case 'h':

      usage(argv[0]);

      break;

    case 'v':

      DiffPrint(DEBUG_ALWAYS, "\n%s %s\n", PROGRAM, RELEASE);
      exit(0);

      break;

    case 'd':

      debug_level = atoi(optarg);

      if (debug_level < 1 || debug_level > 10){
	DiffPrint(DEBUG_ALWAYS,
		  "Error: Debug level outside range or missing !\n");
	usage(argv[0]);
      }

      global_debug_level = debug_level;

      break;

    case 'f':

      if (!strncasecmp(optarg, "-", 1)){
	DiffPrint(DEBUG_ALWAYS, "Error: Parameter is missing !\n");
	usage(argv[0]);
      }

      config_file_ = strdup(optarg);

      break;

    case '?':

      DiffPrint(DEBUG_ALWAYS,
		"Error: %c isn't a valid option or its parameter is missing !\n",
		optopt);
      usage(argv[0]);

      break;

    case ':':

      DiffPrint(DEBUG_ALWAYS, "Parameter missing !\n");
      usage(argv[0]);

      break;
    }

    if (opt == -1)
      break;
  }

  if (!config_file_)
    config_file_ = strdup(DEFAULT_CONFIG_FILE);

  // Get diffusion ID
  if (!node_id_configured){
    // Try to get id from environment variable
    if (node_id_env != NULL){
      my_id_ = atoi(node_id_env);
      node_id_configured = true;
    }
  }

#ifdef USE_EMSIM
  if (!node_id_configured){
    // Try to read groups and node id from emsim environment variables
    if (sim_id && sim_group){
      my_id_ = atoi(sim_id);
      group_id = atoi(sim_group);
      diffusion_port_ = diffusion_port_ + my_id_ + (100 * group_id);
      node_id_configured = true;
    }
  }
#endif // USE_EMSIM

  // Use random node id if user has not specified it
  if (!node_id_configured){
    DiffPrint(DEBUG_ALWAYS, "Diffusion : node_addr not set. Using random id.\n");

    // Generate random ID
    do{
      GetTime(&tv);
      SetSeed(&tv);
      my_id_ = GetRand();
    }
    while(my_id_ == LOCALHOST_ADDR || my_id_ == BROADCAST_ADDR);
  }

#else
  my_id_ = nodeid;
#endif // !NS_DIFFUSION

  // Initialize variables
  lon_ = 0.0;
  lat_ = 0.0;

#ifdef STATS
  if (use_io_stats)
    stats_ = new DiffusionStats(my_id_, stats_warm_up_time);
  else
    stats_ = NULL;
#endif // STATS

  GetTime(&tv);
  SetSeed(&tv);
  pkt_count_ = GetRand();
  random_id_ = GetRand();

  Tcl_InitHashTable(&htable_, 2);

  // Initialize EventQueue
  timers_manager_ = new TimerManager;

  // Create regular timers
  callback = new NeighborsTimeoutTimer(this);
  timers_manager_->addTimer(NEIGHBORS_DELAY, callback);

  callback = new FilterTimeoutTimer(this);
  timers_manager_->addTimer(FILTER_DELAY, callback);

  if (stop_time > 0){
    callback = new DiffusionStopTimer(this);
    timers_manager_->addTimer((stop_time * 1000), callback);
  }

  GetTime(&tv);

  // Print Initialization message
  DiffPrint(DEBUG_ALWAYS, "Diffusion : starting at time %ld:%ld\n",
	    tv.tv_sec, tv.tv_usec);
  DiffPrint(DEBUG_ALWAYS, "Diffusion : Node id = %d\n", my_id_);

  // Initialize diffusion io devices
#ifdef IO_LOG
  if (use_io_log){
    pseudo_io_device = new IOLog(my_id_);
    in_devices_.push_back(pseudo_io_device);
    out_devices_.push_back(pseudo_io_device);

    in_devices = &(pseudo_io_device->in_devices_);
    out_devices = &(pseudo_io_device->out_devices_);
    local_out_devices = &(local_out_devices_);
  }
  else{
    in_devices = &(in_devices_);
    out_devices = &(out_devices_);
    local_out_devices = &(local_out_devices_);
  }
  in_devices = &(in_devices_);
#else
  out_devices = &(out_devices_);
  local_out_devices = &(local_out_devices_);
#endif // IO_LOG

#ifdef NS_DIFFUSION
  device = new LocalApp(diffrtg);
  local_out_devices->push_back(device);

  device = new LinkLayerAbs(diffrtg);
  out_devices->push_back(device);
#endif // NS_DIFFUSION

#ifdef UDP
  device = new UDPLocal(&diffusion_port_);
  in_devices->push_back(device);
  local_out_devices->push_back(device);

#ifdef WIRED
  device = new UDPWired(config_file_);
  out_devices->push_back(device);
#endif // WIRED
#endif // UDP

#ifdef USE_RPC
  device = new RPCIO();
  in_devices->push_back(device);
  out_devices->push_back(device);
#endif // USE_RPC

#ifdef USE_MOTE_NIC
  device = new MOTEIO();
  in_devices->push_back(device);
  out_devices->push_back(device);
#endif // USE_MOTE_NIC

#ifdef USE_SMAC
  device = new SMAC();
  in_devices->push_back(device);
  out_devices->push_back(device);
#endif // USE_SMAC

#ifdef USE_EMSTAR
#ifdef USE_EMSIM
  device = new Emstar(my_id_, group_id, true);
#else
  device = new Emstar();
#endif // USE_EMSIM
  in_devices->push_back(device);
  out_devices->push_back(device);
#endif // USE_EMSTAR

#ifdef USE_WINSNG2
  device = new WINSNG2();
  in_devices->push_back(device);
  out_devices->push_back(device);
#endif // USE_WINSNG2
}

HashEntry * DiffusionCoreAgent::getHash(unsigned int pkt_num,
					 unsigned int rdm_id)
{
  unsigned int key[2];

  key[0] = pkt_num;
  key[1] = rdm_id;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable_, (char *)key);

  if (entryPtr == NULL)
    return NULL;

  return (HashEntry *)Tcl_GetHashValue(entryPtr);
}

void DiffusionCoreAgent::putHash(unsigned int pkt_num,
				 unsigned int rdm_id)
{
  Tcl_HashEntry *tcl_hash_entry;
  HashEntry *hash_entry;
  HashList::iterator hash_itr;
  unsigned int key[2];
  int new_hash_key;

  if (hash_list_.size() == HASH_TABLE_MAX_SIZE){
    // Hash table reached maximum size

    for (int i = 0; ((i < HASH_TABLE_REMOVE_AT_ONCE)
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

  tcl_hash_entry = Tcl_CreateHashEntry(&htable_, (char *)key, &new_hash_key);

  if (new_hash_key == 0){
    DiffPrint(DEBUG_IMPORTANT, "Key already exists in hash !\n");
    return;
  }

  hash_entry = new HashEntry;

  Tcl_SetHashValue(tcl_hash_entry, hash_entry);
  hash_list_.push_back(tcl_hash_entry);
}

#ifndef NS_DIFFUSION
void DiffusionCoreAgent::recvPacket(DiffPacket pkt)
{
  struct hdr_diff *dfh = HDR_DIFF(pkt);
  Message *rcv_message = NULL;
  int8_t version, msg_type;
  u_int16_t data_len, num_attr, source_port;
  int32_t rdm_id, pkt_num, next_hop, last_hop;   

  // Read header
  version = DIFF_VER(dfh);
  msg_type = MSG_TYPE(dfh);
  source_port = ntohs(SRC_PORT(dfh));
  pkt_num = ntohl(PKT_NUM(dfh));
  rdm_id = ntohl(RDM_ID(dfh));
  num_attr = ntohs(NUM_ATTR(dfh));
  next_hop = ntohl(NEXT_HOP(dfh));
  last_hop = ntohl(LAST_HOP(dfh));
  data_len = ntohs(DATA_LEN(dfh));

  // Packet is good, create a message
  rcv_message = new Message(version, msg_type, source_port, data_len,
			    num_attr, pkt_num, rdm_id, next_hop, last_hop);

  // Read all attributes into the Message structure
  rcv_message->msg_attr_vec_ = UnpackAttrs(pkt, num_attr);

  // Process the incoming message
  recvMessage(rcv_message);

  // Don't forget to message when we're done
  delete rcv_message;

  delete [] pkt;
}
#endif // !NS_DIFFUSION

void DiffusionCoreAgent::recvMessage(Message *msg)
{
  BlackList::iterator black_list_itr;
  Tcl_HashEntry *tcl_hash_entry;
  unsigned int key[2];

  // Check version
  if (msg->version_ != DIFFUSION_VERSION)
    return;

  // Check for ID conflict
  if (msg->last_hop_ == my_id_){
    DiffPrint(DEBUG_ALWAYS, "Error: A diffusion ID conflict has been detected !\n");
    exit(-1);
  }

  // Address filtering
  if ((msg->next_hop_ != BROADCAST_ADDR) &&
      (msg->next_hop_ != LOCALHOST_ADDR) &&
      (msg->next_hop_ != my_id_))
    return;

  // Blacklist filtering
  black_list_itr = black_list_.begin();
  while (black_list_itr != black_list_.end()){
    if (*black_list_itr == msg->last_hop_){
      DiffPrint(DEBUG_DETAILS, "Ignoring message from blacklisted node %d !\n",
		msg->last_hop_);
      return;
    }
    black_list_itr++;
  }

  // Control Messages are unique and don't go to the hash
  if (msg->msg_type_ != CONTROL){
    // Hash table keeps info about packets
  
    key[0] = msg->pkt_num_;
    key[1] = msg->rdm_id_;
    tcl_hash_entry = Tcl_FindHashEntry(&htable_, (char *) key);

    if (tcl_hash_entry != NULL){
      DiffPrint(DEBUG_DETAILS, "Node%d: Received old message !\n", my_id_);
      msg->new_message_ = 0;
    }
    else{
      // Add message to the hash table
      putHash(key[0], key[1]);
      msg->new_message_ = 1;
    }
  }

#ifdef STATS
  if (stats_)
    stats_->logIncomingMessage(msg);
#endif // STATS

  // Check if it's a control of a regular message
  if (msg->msg_type_ == CONTROL)
    processControlMessage(msg);
  else
    processMessage(msg);
}

#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  agent = new DiffusionCoreAgent(argc, argv);

  signal(SIGINT, signal_handler);

  agent->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE
