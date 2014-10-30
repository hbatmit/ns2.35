
/*
 * landmark.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: landmark.cc,v 1.8 2005/08/25 18:58:12 johnh Exp $
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

// $Header: /cvsroot/nsnam/ns-2/sensor-nets/landmark.cc,v 1.8 2005/08/25 18:58:12 johnh Exp $

// Author: Satish Kumar, kkumar@isi.edu

#include <stdarg.h>
#include <float.h>

#include "landmark.h"
#include <random.h>
#include <cmu-trace.h>
#include <address.h>

static int lm_index = 0;


void
LMNode::copy_tag_list(compr_taglist *taglist)
{
  compr_taglist *tags = NULL;
  compr_taglist *tag_ptr1, *tag_ptr2;

  // Delete the old tag list if it exists
  if(tag_list_) {
    tag_ptr1 = tag_list_;
    while(tag_ptr1) {
      tag_ptr2 = tag_ptr1;
      tag_ptr1 = tag_ptr1->next_;
      delete tag_ptr2;
    }
    tag_list_ = NULL;
  }

  // Copy the specified taglist
  tag_ptr1 = taglist;
  while(tag_ptr1) {
    if(!tag_list_) {
      tag_list_ = new compr_taglist;
      tag_ptr2 = tag_list_;
    }
    else {
      tag_ptr2->next_ = new compr_taglist;
      tag_ptr2 = tag_ptr2->next_;
    }
    tag_ptr2->obj_name_ = tag_ptr1->obj_name_;
    tag_ptr1 = tag_ptr1->next_;
  }
}





// Returns a random number between 0 and max
inline double 
LandmarkAgent::jitter(double max, int be_random_)
{
  return (be_random_ ? Random::uniform(max) : 0);
}



// Returns a random number between 0 and max
inline double 
LandmarkAgent::random_timer(double max, int be_random_)
{
  return (be_random_ ? rn_->uniform(max) : 0);
}



void
LandmarkAgent::stop()
{
  ParentChildrenList *pcl = parent_children_list_, *tmp_pcl;

  trace("Node %d: LM Agent going down at time %f",myaddr_,NOW);

  // Cancel any running timers and reset relevant variables
  if(promo_timer_running_) {
    promo_timer_running_ = 0;
    promo_timer_->cancel();
  }
  num_resched_ = 0;
  wait_state_ = 0;
  total_wait_time_ = 0;

  // Reset highest level of node
  highest_level_ = 0;

  // Delete ParentChildrenList objects for all levels
  while(pcl) {
    tmp_pcl = pcl;
    pcl = pcl->next_;
    delete tmp_pcl;
  }
  parent_children_list_ = NULL;

  // Indicate that node is dead so that packets will not be processed
  node_dead_ = 1;

  global_lm_id_ = NO_GLOBAL_LM;
  global_lm_level_ = -1;

  // Event id > 0 for scheduled event
  if(tag_advt_event_->uid_ > 0) {
    Scheduler &s = Scheduler::instance();
    s.cancel(tag_advt_event_);
  }
}





void LandmarkAgent::
trace (char *fmt,...)
{
  va_list ap; // Define a variable ap that will refer to each argument in turn

  if (!tracetarget_)
    return;

  // Initializes ap to first argument
  va_start (ap, fmt); 
  // Prints the elements in turn
  vsprintf (tracetarget_->buffer (), fmt, ap); 
  tracetarget_->dump ();
  // Does the necessary clean-up before returning
  va_end (ap); 
}



static class LandmarkClass:public TclClass
{
  public:
  LandmarkClass ():TclClass ("Agent/landmark")
  {
  }
  TclObject *create (int, const char *const *)
  {
    return (new LandmarkAgent ());
  }
} class_landmark;




LandmarkAgent::LandmarkAgent (): Agent(PT_MESSAGE), promo_start_time_(0), promo_timeout_(50), promo_timeout_decr_(1), promo_timer_running_(0), seqno_(0), highest_level_(0), parent_children_list_(NULL), ll_queue(0), be_random_(1), wait_state_(0), total_wait_time_(0), debug_(1) ,qry_debug_(0)
 {

  promo_timer_ = new PromotionTimer(this);

  //  bind_time ("promo_timeout_", "&promo_timeout_");
  num_resched_ = 0;
  tag_dbase_ = NULL;
  node_ = NULL;

  cache_ = 0; // default is to disable caching
  tag_cache_ = new TagCache[MAX_CACHE_ITEMS];
  num_cached_items_ = 0;

  recent_demotion_msgs_ = new RecentMsgRecord[MAX_DEMOTION_RECORDS];
  num_demotion_msgs_ = 0;

  // default value for the update period
  update_period_ = 400;
  update_timeout_ = update_period_ + 4 * LM_STARTUP_JITTER;

  adverts_type_ = FLOOD;  // default is to flood adverts
  global_lm_ = 0; // No global LMs by default
  global_lm_id_ = NO_GLOBAL_LM;
  global_lm_level_ = -1;

  // myaddr_ not defined at this point ... So use lm_index for init
  rn_ = new RNG(RNG::RAW_SEED_SOURCE,lm_index++);;
  // Throw away a bunch of initial values
  for(int i = 0; i < 128; ++i) {
    rn_->uniform(200);
  }

  node_dead_ = 0;

  bind ("be_random_", &be_random_);
  //  bind ("myaddr_", &myaddr_);
  bind ("debug_", &debug_);

  num_nbrs_ = 0;
  nbrs_ = NULL;

  tag_mobility_ = new TagMobilityHandler(this);
  tag_mobility_event_ = new Event;

  // myaddr_ not defined at this point ... So use lm_index for init
  tag_rng_ = new RNG(RNG::RAW_SEED_SOURCE,lm_index++);;
  // Throw away a bunch of initial values
  for(int i = 0; i < 128; ++i) {
    tag_rng_->uniform(200);
  }

  mobility_period_ = 60;
  mobile_tags_ = NULL;

  tag_advt_handler_ = new TagAdvtHandler(this);
  tag_advt_event_ = new Event;
}




int
LandmarkAgent::CheckDemotionMsg(nsaddr_t id, int level, int origin_time)
{
  int i = 0;

  // If object already exists in cache, update info if necessary
  for(i = 0; i < num_demotion_msgs_; ++i) {
    if(recent_demotion_msgs_[i].id_ == id && recent_demotion_msgs_[i].level_ == level) {
      if(recent_demotion_msgs_[i].origin_time_ >= origin_time) {
        return(OLD_MESSAGE);
      }
      else {
        recent_demotion_msgs_[i].origin_time_ = origin_time;
        return(NEW_ENTRY);
      }
    }
  }

  if(num_demotion_msgs_ < MAX_DEMOTION_RECORDS) {
    i = num_demotion_msgs_;
    ++num_demotion_msgs_;
    recent_demotion_msgs_[i].id_ = id;
    recent_demotion_msgs_[i].level_ = level;
    recent_demotion_msgs_[i].origin_time_ = origin_time;
  }
  else {
    // Use LRU cache replacement
    int replace_index = 0;
    int least_time = recent_demotion_msgs_[replace_index].origin_time_;
    for(i = 0; i < MAX_DEMOTION_RECORDS; ++i) {
      if(recent_demotion_msgs_[i].origin_time_ < least_time)
        replace_index = i;
    }
    recent_demotion_msgs_[replace_index].id_ = id;
    recent_demotion_msgs_[replace_index].level_ = level;
    recent_demotion_msgs_[replace_index].origin_time_ = origin_time;
  }
  return(NEW_ENTRY);
}




void
ParentChildrenList::UpdateChildLMAddr(nsaddr_t id, int num_lm_addrs, int64_t *lm_addrs)
{
  LMNode *potl_ch = NULL;

  potl_ch = pchildren_;
  while(potl_ch) {
    if(potl_ch->id_ == id)
      break;
    potl_ch = potl_ch->next_;
  }

  assert(potl_ch);
  (potl_ch->lmaddr_)->delete_lm_addrs();
  for(int i = 0; i < num_lm_addrs; ++i)
    (potl_ch->lmaddr_)->add_lm_addr(lm_addrs[i]);
}




int
ParentChildrenList::UpdatePotlParent(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int num_children, int energy, int origin_time, int delete_flag)
{
  LMNode *potl_parent, *list_ptr;
  double now = Scheduler::instance().clock();
  // Extract seqnum and origin time
  int seqnum = origin_time & 0xFFFF;
  origin_time = origin_time >> 16;
  
  assert(num_pparent_ >= 0);

  // cannot delete from an empty list!
  if(delete_flag && !pparent_)
    return(ENTRY_NOT_FOUND);

  //  if((a_->debug_) && (a_->myaddr_ == 24)) {
  //    a_->trace("Node %d: Updating Potl Parent level %d, id %d, delete_flag %d, time %f",a_->myaddr_,level_,id,delete_flag,now);
  //  }

  if(pparent_ == NULL) {
    pparent_ = new LMNode(id, next_hop, num_hops, level, num_children, energy, origin_time, now);
    pparent_->last_upd_seqnum_ = seqnum;
    parent_ = pparent_;
    ++num_pparent_;
    return(NEW_ENTRY);
  }
  
  list_ptr = pparent_;
  potl_parent = list_ptr;
  while(list_ptr != NULL) {
    if(list_ptr->id_ == id) {
      // Check if this is a old message floating around in the network
      if(list_ptr->last_upd_origin_time_ > origin_time || (list_ptr->last_upd_origin_time_ == origin_time && list_ptr->last_upd_seqnum_ >= seqnum)) {
	// Check if we got the old update on a shorter path
	if(list_ptr->num_hops_ > num_hops) {
	  list_ptr->next_hop_ = next_hop;
	  list_ptr->num_hops_ = num_hops;
	  return(OLD_ENTRY);
	}
	return(OLD_MESSAGE); 
      }
      if(!delete_flag) {
	// Make this node as parent if it's closer than current parent
	if(parent_->num_hops_ > num_hops + 10 || num_hops == 0) {
	  parent_ = list_ptr;
	}
	list_ptr->next_hop_ = next_hop;
	list_ptr->num_hops_ = num_hops;
	list_ptr->level_ = level;
	list_ptr->num_children_ = num_children;
	list_ptr->energy_ = energy;
	list_ptr->last_upd_origin_time_ = origin_time;
	list_ptr->last_upd_seqnum_ = seqnum;
	list_ptr->last_update_rcvd_ = Scheduler::instance().clock();
      }
      else { // delete the entry
	if(num_pparent_)
	  --(num_pparent_);

	if(pparent_ == list_ptr)
	  pparent_ = list_ptr->next_;
	else
	  potl_parent->next_ = list_ptr->next_;

	if(parent_->id_ == list_ptr->id_)
	  assert(parent_ == list_ptr);

	// No parent if potl parent list is empty
	if(pparent_ == NULL) {
	  parent_ = NULL;
	}
	else if(parent_ == list_ptr) {
	// Select new parent if current parent is deleted and
	// potl parent is not empty; closest potl parent is new parent
	  LMNode *tmp = pparent_;
	  int best_num_hops = pparent_->num_hops_;
	  LMNode *best_parent = pparent_;
	  while(tmp != NULL) {
	    if(tmp->num_hops_ < best_num_hops) {
	      best_num_hops = tmp->num_hops_;
	      best_parent = tmp;
	    }
	    tmp = tmp->next_;
	  }
	  parent_ = best_parent;
	}
	delete list_ptr;
      }
      return(OLD_ENTRY);
    }
    potl_parent = list_ptr;
    list_ptr = list_ptr->next_;
  }

  if(delete_flag)
    return(ENTRY_NOT_FOUND);

  potl_parent->next_ = new LMNode(id, next_hop, num_hops, level, num_children, energy, origin_time, now);
  (potl_parent->next_)->last_upd_seqnum_ = seqnum;
  ++num_pparent_;
  // Make this node as parent if it's closer than current parent
  if(parent_->num_hops_ > num_hops) {
    parent_ = potl_parent->next_;
  }
  return(NEW_ENTRY);
}




int
ParentChildrenList::UpdatePotlChild(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int num_children, int energy, int origin_time, int child_flag, int delete_flag, compr_taglist *taglist)
{
  LMNode *potl_child, *list_ptr;
  double now = Scheduler::instance().clock();
  int new_child = 0;
  int tags_changed = 0;
  int seqnum = origin_time & 0xFFFF;
  origin_time = origin_time >> 16;

  //  if(a_->debug_) printf("Node %d: Number of potl children %d",a_->myaddr_,num_potl_children_);

  // cannot delete from an empty list!
  if(delete_flag && !pchildren_) {
    return(ENTRY_NOT_FOUND);
  }

  assert(num_potl_children_ >= 0);
  assert(num_children_ >= 0);

  if(pchildren_ == NULL) {
    pchildren_ = new LMNode(id, next_hop, num_hops, level, num_children, energy, origin_time, now);
    pchildren_->child_flag_ = child_flag;
    pchildren_->last_upd_seqnum_ = seqnum;
    if(child_flag == IS_CHILD) ++(num_children_);
    if(child_flag != NOT_POTL_CHILD) ++(num_potl_children_);
    ++(num_heard_);
    pchildren_->copy_tag_list(taglist);
    if(child_flag == IS_CHILD)
      return(NEW_CHILD);
    else
      return(NEW_ENTRY);
  }
  
  list_ptr = pchildren_;
  potl_child = list_ptr;
  while(list_ptr != NULL) {
    if(list_ptr->id_ == id) {
      // Check if this is a old message floating around in the network
      if(list_ptr->last_upd_origin_time_ > origin_time || (list_ptr->last_upd_origin_time_ == origin_time && list_ptr->last_upd_seqnum_ >= seqnum)) {

	// Check if we got this update on a shorter path; If the update rcvd
	// on a shorter path, we need to forward this message to ensure that
	// this message reaches all nodes in the advertising node's vicinity
	if(list_ptr->num_hops_ > num_hops) {
	  list_ptr->next_hop_ = next_hop;
	  list_ptr->num_hops_ = num_hops;
	  return(OLD_ENTRY);
	}
	
	return(OLD_MESSAGE);
      }
      if(!delete_flag) {

	// Old entry but the status has changed to child or vice-versa
	if((list_ptr->child_flag_ == NOT_CHILD || list_ptr->child_flag_ == NOT_POTL_CHILD) && (child_flag == IS_CHILD)) {
	  list_ptr->child_flag_ = child_flag;
	  ++(num_children_);
	  new_child = 1;
	}
	else if((list_ptr->child_flag_ == IS_CHILD) && (child_flag == NOT_CHILD || child_flag == NOT_POTL_CHILD)) {
	  list_ptr->child_flag_ = child_flag;
	  --(num_children_);
	}
	list_ptr->next_hop_ = next_hop;
	list_ptr->num_hops_ = num_hops;
	list_ptr->level_ = level;
	list_ptr->num_children_ = num_children;
	list_ptr->energy_ = energy;
	list_ptr->last_upd_origin_time_ = origin_time;
	list_ptr->last_upd_seqnum_ = seqnum;
	list_ptr->last_update_rcvd_ = Scheduler::instance().clock();
	if(!a_->compare_tag_lists(list_ptr->tag_list_,-1,taglist,-1)) {
	  tags_changed = 1;
	  // Delete the old tag list and copy the specified taglist
	  list_ptr->copy_tag_list(taglist);
	}
      }
      else {
	if(pchildren_ == list_ptr)
	  pchildren_ = list_ptr->next_;
	else
	  potl_child->next_ = list_ptr->next_;

	if(list_ptr->child_flag_ == IS_CHILD) --num_children_;
	if(list_ptr->child_flag_ != NOT_POTL_CHILD) --num_potl_children_;
	--num_heard_;
	delete list_ptr;
      }
      if(new_child)
	return(NEW_CHILD);
      else if(tags_changed && child_flag == IS_CHILD)
	return(OLD_CHILD_TAGS_CHANGED);
      else
	return(OLD_ENTRY);
    }
    potl_child = list_ptr;
    list_ptr = list_ptr->next_;
  }

  // delete flag must be FALSE if we are here
  // assert(!delete_flag);
  if(delete_flag) {
    return(ENTRY_NOT_FOUND);
  }
  
  potl_child->next_ = new LMNode(id, next_hop, num_hops, level, num_children, energy, origin_time, now);
  (potl_child->next_)->copy_tag_list(taglist);
  (potl_child->next_)->child_flag_ = child_flag;
  (potl_child->next_)->last_upd_seqnum_ = seqnum;
  if(child_flag == IS_CHILD) ++(num_children_);
  if(child_flag != NOT_POTL_CHILD) ++(num_potl_children_);
  ++(num_heard_);
  if(child_flag == IS_CHILD)
    return(NEW_CHILD);
  else
    return(NEW_ENTRY);
}





void
LandmarkAgent::ProcessHierUpdate(Packet *p)
{
  hdr_ip *iph = HDR_IP(p);
  hdr_cmn *hdrc = HDR_CMN(p);
  Scheduler &s = Scheduler::instance();
  double now = s.clock();
  int origin_time = 0;
  unsigned char *walk;
  nsaddr_t origin_id, parent, next_hop;
  int i, level, vicinity_radius, num_hops, potl_parent_flag = FALSE;
  int action, energy = 0;
  nsaddr_t *potl_children;
  int num_children = 0;
  int num_potl_children = 0;
  int num_lm_addrs = 0;
  int num_advert_lm_addrs = 0;
  int64_t *advert_lm_addrs = NULL;
  int64_t *lm_addrs = NULL;
  // Packet will have seqnum, level, vicinity radii, parent info
  // and possibly potential children (if the node is at level > 0)
  int num_tags = 0;
  compr_taglist *adv_tags = NULL, *tag_ptr;
  compr_taglist *tag_ptr1, *tag_ptr2;
  Packet *newp;

  //  if(now > 412.5) {
  //    purify_printf("ProcessHierUpdate1, %f, %d\n",now,myaddr_);
  //    purify_new_leaks();
  //  }


  //  if(debug_)
  //    trace("Node %d: Received packet from %d with ttl %d", myaddr_,iph->saddr(),iph->ttl_);

  walk = p->accessdata();

  // Originator of the LM advertisement and the next hop to reach originator
  origin_id = iph->saddr();    

  // Free and return if we are seeing our own packet again
  if(origin_id == myaddr_) {
    Packet::free(p);
    return;
  }
  
  // type of advertisement
  action = *walk++;

  num_advert_lm_addrs = *walk++;
  if(num_advert_lm_addrs)
    advert_lm_addrs = new int64_t[num_advert_lm_addrs];
  for(int j = 0; j < num_advert_lm_addrs; ++j) {
    advert_lm_addrs[j] = *walk++;
    advert_lm_addrs[j] = (advert_lm_addrs[j] << 8) | *walk++;
    advert_lm_addrs[j] = (advert_lm_addrs[j] << 8) | *walk++;
    advert_lm_addrs[j] = (advert_lm_addrs[j] << 8) | *walk++;
    advert_lm_addrs[j] = (advert_lm_addrs[j] << 8) | *walk++;
    advert_lm_addrs[j] = (advert_lm_addrs[j] << 8) | *walk++;
    advert_lm_addrs[j] = (advert_lm_addrs[j] << 8) | *walk++;
    advert_lm_addrs[j] = (advert_lm_addrs[j] << 8) | *walk++;
  }
  
  // level of the originator
  level = *walk++;

    //    if(num_advert_lm_addrs)
    //      trace("Node 10: Rcved msg from 0, level %d, num_lm_addrs %d, advert_lm_addrs %x, time %f",level,num_advert_lm_addrs,advert_lm_addrs[0],now);

  //  trace("Node %d: Processing Hierarchy Update Packet", myaddr_);

  //  if((myaddr_ == 153) && (origin_id == 29)) {
  //    trace("Node 153: Receiving level %d update from node 29 at time %f,action = %d",level,s.clock(),action);
  //  }

  // energy level of advertising node
  energy = *walk++;
  energy = (energy << 8) | *walk++;
  energy = (energy << 8) | *walk++;
  energy = (energy << 8) | *walk++;

  // next hop info
  next_hop = *walk++;
  next_hop = (next_hop << 8) | *walk;

  // Change next hop to ourselves for the outgoing packet
  --walk;
  (*walk++) = (myaddr_ >> 8) & 0xFF;
  (*walk++) = (myaddr_) & 0xFF;
  
  // vicinity radius
  vicinity_radius = *walk++;
  vicinity_radius = (vicinity_radius << 8) | *walk++;
  // number of hops away from advertising LM
  num_hops = vicinity_radius - (iph->ttl_ - 1);

  //  if(myaddr_ == 155)
  //    trace("Node 155: Receiving level %d update from node %d at time %f,action = %d, num_hops = %d",level,origin_id,s.clock(),action,num_hops);

  // origin time of advertisement
  origin_time = *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;
  
  //  if(debug_ && myaddr_ == 33)
  //    trace("Node %d: Processing level %d pkt from %d at t=%f, origin %d, num hops %d", myaddr_,level,iph->saddr_,now,origin_time,num_hops);


  // Parent of the originator
  parent = *walk++;
  parent = parent << 8 | *walk++;

  // Number of hops has to be less than vicinity radius to ensure that atleast
  // 2 level K LMs see each other if they exist
  if(level > 0 && (action == PERIODIC_ADVERTS || action == GLOBAL_ADVERT || action == UNICAST_ADVERT_CHILD || action == UNICAST_ADVERT_PARENT)) {
    // Number of children
    num_children = *walk++;
    num_children = num_children << 8 | *walk++;

    // Number of potential children
    num_potl_children = *walk++;
    num_potl_children = num_potl_children << 8 | *walk++;

  // If level of advertising LM > 1, check if we are in potl children list.
  // If so, add as potl parent to level - 1
    if(num_potl_children) {
      potl_children = new nsaddr_t[num_potl_children];
      for(i = 0; i < num_potl_children; ++i) {
	potl_children[i] = *walk++;
	potl_children[i] = potl_children[i] << 8 | *walk++;
	int tmp_num_addrs = *walk++;
	if(potl_children[i] == myaddr_) {
	  potl_parent_flag = TRUE;
	  num_lm_addrs = tmp_num_addrs;
	  if(num_lm_addrs) {
	    lm_addrs = new int64_t[num_lm_addrs];
	    for(int j = 0; j < num_lm_addrs; ++j) {
	      lm_addrs[j] = *walk++;
	      lm_addrs[j] = (lm_addrs[j] << 8) | *walk++;
	      lm_addrs[j] = (lm_addrs[j] << 8) | *walk++;
	      lm_addrs[j] = (lm_addrs[j] << 8) | *walk++;
	      lm_addrs[j] = (lm_addrs[j] << 8) | *walk++;
	      lm_addrs[j] = (lm_addrs[j] << 8) | *walk++;
	      lm_addrs[j] = (lm_addrs[j] << 8) | *walk++;
	      lm_addrs[j] = (lm_addrs[j] << 8) | *walk++;
	    }
	  }
	}
	else 
	  walk = walk + tmp_num_addrs*8;
      }
    }
  }
  
  num_tags = 0;
  if(action == PERIODIC_ADVERTS || action == GLOBAL_ADVERT || action == UNICAST_ADVERT_CHILD || action == UNICAST_ADVERT_PARENT) {
    num_tags = *walk++;
    num_tags = (num_tags << 8) | *walk++;
  }


  adv_tags = NULL;
  // Store tag info only if the level of advertising LM is less than 
  // our highest level; otherwise we dont need this information
  if(num_tags && level < highest_level_) {
    adv_tags = new compr_taglist;
    tag_ptr = adv_tags;
    i = 0;
    while(i < num_tags) {
      if(i) {
	tag_ptr->next_ = new compr_taglist;
	tag_ptr = tag_ptr->next_;
      }
      tag_ptr->obj_name_ = *walk++;
      tag_ptr->obj_name_ = (tag_ptr->obj_name_ << 8) | *walk++;
      tag_ptr->obj_name_ = (tag_ptr->obj_name_ << 8) | *walk++;
      tag_ptr->obj_name_ = (tag_ptr->obj_name_ << 8) | *walk++;
      ++i;
      //	trace("tag name: %d.%d.%d",(tag_ptr->obj_name_ >> 24) & 0xFF,(tag_ptr->obj_name_ >> 16) & 0xFF,(tag_ptr->obj_name_) & 0xFFFF);
    }
  }

  //  if(level == 253)
  //    trace("Level is 253 at time %f\n",now);

  ParentChildrenList **pcl1 = NULL;
  ParentChildrenList **pcl2 = NULL;
  int found1 = FALSE;
  int found2 = FALSE;
  ParentChildrenList **pcl = &parent_children_list_;

  // Insert parent-child objects for levels: level-1 (if level > 0) & level + 1
  while((*pcl) != NULL) {
    if((*pcl)->level_ == (level-1)) {
      found1 = TRUE;
      pcl1 = pcl;
    }
    if((*pcl)->level_ == (level+1)) {
      found2 = TRUE;
      pcl2 = pcl;
    }
    pcl = &((*pcl)->next_);
  }


  // check if level > 0
  if(!found1 && level) {
    *pcl = new ParentChildrenList(level-1, this);
    pcl1 = pcl;
    pcl = &((*pcl)->next_);
  }

  if(!found2) {
    *pcl = new ParentChildrenList(level+1, this);
    pcl2 = pcl;
    pcl = &((*pcl)->next_);
  }

  // If level is same as our level, we can decrease the promotion timer 
  // if it's running provided we havent already heard advertisements from
  // this node

  int delete_flag = FALSE; // Add the child/potl parent entry
  if(action == DEMOTION) delete_flag = TRUE;
  int child_flag = NOT_CHILD; // Indicates whether this node is our child
  if(parent == myaddr_) 
    child_flag = IS_CHILD;
  else if(action == GLOBAL_ADVERT && num_hops > vicinity_radius) 
  // The global LM may not be a potential child for us at any level if
  // it is farther away than the vicinity radius
    child_flag = NOT_POTL_CHILD;

  int ret_value = (*pcl2)->UpdatePotlChild(origin_id, next_hop, num_hops, level, num_children, energy, origin_time, child_flag, delete_flag,adv_tags);


  // Free packet and return if we have seen this packet before
  if(ret_value == OLD_MESSAGE && action != UNICAST_ADVERT_CHILD && action != UNICAST_ADVERT_PARENT) {
    if(num_potl_children) delete[] potl_children;
    if(num_lm_addrs) delete[] lm_addrs;
    if(num_advert_lm_addrs) delete[] advert_lm_addrs;
    // Free the tag list
    tag_ptr1 = adv_tags;
    while(tag_ptr1) {
      tag_ptr2 = tag_ptr1;
      tag_ptr1 = tag_ptr1->next_;
      delete tag_ptr2;
    }
    Packet::free(p);
    return;
  }
  
  // Send hierarchy advts if tag list has changed due to new child 
  // or change in the taglist of an old child
  if(ret_value == NEW_CHILD || ret_value == OLD_CHILD_TAGS_CHANGED)
    SendChangedTagListUpdate(0,level);
  
  
  //  if(level == highest_level_)
  //     num_resched_ = (*pcl2)->num_potl_children_ - 1;

  // If promotion timer is running, decrement by constant amount
  if((ret_value == NEW_ENTRY) && (level == highest_level_) && (action == PERIODIC_ADVERTS || action == UNICAST_ADVERT_CHILD || action == UNICAST_ADVERT_PARENT) && (num_hops < radius(level))) {
    // Promotion timer is running but is not in wait state
    if(promo_timer_running_ && !wait_state_) {
      double resched_time = promo_timeout_ - (now - promo_start_time_) - promo_timeout_decr_;
      if(resched_time < 0) resched_time = 0;
      //      trace("Node %d: Rescheduling timer in ProcessHierUpdate, now %f, st %f, decr %f, resch %f\n", myaddr_, now, promo_start_time_, promo_timeout_decr_, resched_time);
      promo_timer_->resched(resched_time);
    }
  }

  // If our parent has demoted itself, we might have to start 
  // the election process again
  if(action == DEMOTION) {
    (*pcl1)->UpdatePotlParent(origin_id, next_hop, num_hops, level, num_children, energy, origin_time, TRUE);
    if(((*pcl1)->parent_ == NULL) && (!promo_timer_running_ || (promo_timer_running_ && wait_state_)) && (highest_level_ == level-1)) {
      //      if (debug_) printf("Node %d: sched timer in ProcessHierUpdate\n",myaddr_);
      ParentChildrenList *tmp_pcl = parent_children_list_;
      while(tmp_pcl) {
	if(tmp_pcl->level_ == level) break;
	tmp_pcl = tmp_pcl->next_;
      }
      assert(tmp_pcl);
      
      num_resched_ = tmp_pcl->num_heard_ - 1;
      if(num_resched_) {
	// Cancel any timer running in wait state
	if(promo_timer_running_)
	  promo_timer_->cancel();
	promo_timer_running_ = 1;
	wait_state_ = 0;
	total_wait_time_ = 0;
	promo_timeout_ = random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * radius(level) + MAX_TIMEOUT/((num_resched_+1) * pow(2,highest_level_+1))), be_random_);
	trace("Node %d: Promotion timeout after wait period in ProcessHierUpdate: %f", myaddr_,promo_timeout_);
	num_resched_ = 0;
	promo_start_time_ = s.clock();
	promo_timer_->resched(promo_timeout_);
      }
      else if(!promo_timer_running_) {
	double wait_time = PERIODIC_WAIT_TIME;
	promo_timer_running_ = 1;
	wait_state_ = 1;
	total_wait_time_ += wait_time;
	trace("Node %d: Entering wait state in ProcessHierUpdate because of no parent: %f", myaddr_,now);
	promo_timer_->resched(wait_time);
      }
    }
  }
  // If the advertising LM is a potl parent, add to level-1 
  // ParentChildrenList object
  else if(potl_parent_flag) {
    LMNode *old_parent = (*pcl1)->parent_;
    (*pcl1)->UpdatePotlParent(origin_id, next_hop, num_hops, level, num_children, energy, origin_time, FALSE);

    // If we receive this message from a parent at some level, update
    // the assigned addresses
    if((((*pcl1)->parent_)->id_ == origin_id) && (level-1 == highest_level_)) {
      //      if(num_lm_addrs) 
      //	trace("Node %d: Rcvd higher level lm addr from %d at time %f",myaddr_,origin_id,now);
      //      else
      //	trace("Node %d: Rcvd higher level lm addr msg with no addrs from %d at time %f",myaddr_,origin_id,now);
      ((*pcl1)->mylmaddrs_)->delete_lm_addrs();
      assign_lmaddress(lm_addrs, num_lm_addrs, (*pcl1)->level_);
    }

    // Check if parent has changed
    int new_advert = 0;
    // The first condition may arise if the old parent obj is deleted ... (?)
    if((*pcl1)->parent_ == old_parent && old_parent) {
      if(((*pcl1)->parent_)->id_ != old_parent->id_)
	new_advert = 1;
    }
    else if((*pcl1)->parent_ != old_parent && (*pcl1)->parent_ && old_parent) {
      if(((*pcl1)->parent_)->id_ != old_parent->id_)
	new_advert = 1;
    }
    else if((*pcl1)->parent_ != old_parent)
      new_advert = 1;
    
    // Trigger advertisement if parent has changed
    if(new_advert && (level-1 <= highest_level_)) {
      newp = makeUpdate((*pcl1), HIER_ADVS, PERIODIC_ADVERTS);
      s.schedule(target_,newp,0);
      (*pcl1)->last_update_sent_ = now;
    }

  // If a parent has been selected for highest_level_, cancel promotion timer
  // (for promotion to highest_level_+1) if it's running
    if((level == (highest_level_ + 1)) && ((*pcl1)->parent_ != NULL)) {
      if(promo_timer_running_ && !wait_state_) {
	trace("Node %d: Promotion timer cancelled at time %f in ProcessHierUpdate\n",myaddr_,s.clock());
	promo_timer_->cancel();
	total_wait_time_ = 0;
	wait_state_ = 1;
	double wait_time = PERIODIC_WAIT_TIME;
	total_wait_time_ += wait_time;
	promo_timer_->sched(wait_time);
      }
    }
    else if(level > 0 && level == highest_level_) {
      // Check if the potl parent for highest_level_-1 that we see covers our
      // potential children. If so, we can demote ourselves and cancel our 
      // current promotion timer
      pcl = &parent_children_list_;
      while((*pcl) != NULL) {
	if((*pcl)->level_ == level) {
	  break;
	}
	pcl = &((*pcl)->next_);
      }
      assert(*pcl);

      LMNode *lm = (*pcl)->pchildren_;
      int is_subset = TRUE;
      if((*pcl)->num_potl_children_ > num_potl_children) {
	is_subset = FALSE;
	lm = NULL;
      }

      int is_element = FALSE;
      while(lm) {
	is_element = FALSE;
	for(i = 0; i < num_potl_children; ++i) 
	  if(lm->id_ == potl_children[i] && lm->child_flag_ != NOT_POTL_CHILD) {
	    is_element = TRUE;
	    break;
	  }
	if(is_element == FALSE && lm->child_flag_ != NOT_POTL_CHILD) {
	  is_subset = FALSE;
	  break;
	}
	lm = lm->next_;
      }
      // Demotion process
      if(is_subset == TRUE) {
    	--(highest_level_);
    	delete_flag = TRUE;
	if((*pcl1)->parent_)
	  trace("Node %d: Num potl ch %d, Node %d: Num potl ch %d, time %d",myaddr_, (*pcl)->num_potl_children_,origin_id,num_potl_children,(int)now);
	trace("Node %d: Parent before demotion: %d, msg from %d at time %f",myaddr_, ((*pcl1)->parent_)->id_,origin_id,now);
	
	int upd_time = (int)now;
	upd_time = (upd_time << 16) | ((*pcl1)->seqnum_ & 0xFFFF);
	++((*pcl1)->seqnum_);
	(*pcl1)->UpdatePotlParent(myaddr_, 0, 0, 0, 0, 0, upd_time, delete_flag);

	if((*pcl1)->parent_)
	  trace("Node %d: Parent after demotion: %d",myaddr_, ((*pcl1)->parent_)->id_);

	upd_time = (int) now;
	upd_time = (upd_time << 16) | ((*pcl2)->seqnum_ & 0xFFFF);
	++((*pcl2)->seqnum_);
    	(*pcl2)->UpdatePotlChild(myaddr_, 0, 0, 0, 0, 0, upd_time, IS_CHILD, delete_flag,NULL);

	// Send out demotion messages
	newp = makeUpdate(*pcl, HIER_ADVS, DEMOTION);
	s.schedule(target_, newp, 0);

    	if(promo_timer_running_ && !wait_state_) {
    	  trace("Node %d: Promotion timer cancelled due to demotion at time %d\n",myaddr_,(int)now);
    	  promo_timer_->cancel();
	  total_wait_time_ = 0;
	  wait_state_ = 1;
	  double wait_time = PERIODIC_WAIT_TIME;
	  total_wait_time_ += wait_time;
	  promo_timer_->sched(wait_time);
    	}
      }
    }      
  }


  // If new entry, flood advertisements for level > adv LM's level
  if(ret_value == NEW_ENTRY) {
    ParentChildrenList *tmp_pcl = parent_children_list_;
    while(tmp_pcl) {
      // New nodes should have an initial wait time of atleast 0.1 seconds
      if(tmp_pcl->level_ <= highest_level_ && tmp_pcl->level_ >= level && (now - tmp_pcl->last_update_sent_) > 0.1) {
	newp = makeUpdate(tmp_pcl, HIER_ADVS, PERIODIC_ADVERTS);
	s.schedule(target_,newp,0);
	tmp_pcl->last_update_sent_ = now;
      }
      tmp_pcl = tmp_pcl->next_;
    }
  }

  if(num_potl_children) delete[] potl_children;
  if(num_lm_addrs) delete[] lm_addrs;
  if(num_advert_lm_addrs) delete[] advert_lm_addrs;
  // Delete tag list
  tag_ptr1 = adv_tags;
  while(tag_ptr1) {
    tag_ptr2 = tag_ptr1;
    tag_ptr1 = tag_ptr1->next_;
    delete tag_ptr2;
  }
  
  // Do not forward demotion message if we have seen this message before
  if(action == DEMOTION) {
    //    if(myaddr_ == 30)
    //      printf("Am here\n");
    if(CheckDemotionMsg(origin_id, level, (int)origin_time) == OLD_MESSAGE) {
      Packet::free(p);
      return;
    }
  }

  // Do not forward packet if ttl is lower unless the packet is from the global
  // LM in which case the packet needs to be flooded throughout the network
  if(--iph->ttl_ == 0 && action != GLOBAL_ADVERT) {
    //    drop(p, DROP_RTR_TTL);
    Packet::free(p);
    return;
  }

  // Do not forward if the advertisement is for this node
  if((iph->daddr() >> 8) == myaddr_) {
    //    trace("Node %d: Received unicast advert from %d at level %d for us at time %f",myaddr_,iph->saddr(),level,now);
    Packet::free(p);
    return;
  }

  if(action == UNICAST_ADVERT_CHILD) {
    hdrc->next_hop() = get_next_hop(iph->daddr(),level);
    //    if(myaddr_ == 153)
    //      trace("Node %d: Received child unicast advert from %d to %d at level %d at time %f, next hop %d",myaddr_,iph->saddr(),iph->daddr(),level,now,hdrc->next_hop());
  }
  else if(action == UNICAST_ADVERT_PARENT) {
    //    trace("Node %d: Received parent unicast advert from %d to %d at level %d at time %f",myaddr_,iph->saddr(),iph->daddr(),level,now);
    hdrc->next_hop() = get_next_hop(iph->daddr(),level+2);
  }
  
  assert(hdrc->next_hop() != NO_NEXT_HOP);

  //  if(now > 412.5) {
  //    purify_printf("ProcessHierUpdate2\n");
  //    purify_new_leaks();
  //  }

  // Need to send the packet down the stack
  hdrc->direction() = hdr_cmn::DOWN;
  
  //  if(debug_) printf("Node %d: Forwarding Hierarchy Update Packet", myaddr_);
  s.schedule(target_, p, 0);
}
  



void
LandmarkAgent::assign_lmaddress(int64_t *lmaddr, int num_lm_addrs, int root_level)
{
  ParentChildrenList *tmp_pcl, *cur_pcl = NULL, *child_pcl = NULL;
  ParentChildrenList *parent_pcl = NULL;
  LMNode *pchild;
  int i;  
  int level = root_level;

  //  assert(root_level != 0);
  
  while(level >= 0) {
    tmp_pcl = parent_children_list_;
    while(tmp_pcl) {
      if(tmp_pcl->level_ == level)
	cur_pcl = tmp_pcl;
      if(tmp_pcl->level_ == (level-1))
	child_pcl = tmp_pcl;
      if(tmp_pcl->level_ == (level+1))
	parent_pcl = tmp_pcl;
      tmp_pcl = tmp_pcl->next_;
    }
    
    assert(cur_pcl);
    if(level) assert(child_pcl);
    assert(parent_pcl);

    // Update LM address at the appropriate level
    if(level == root_level) {
      (cur_pcl->mylmaddrs_)->delete_lm_addrs();
      if(num_lm_addrs) {
	for(i = 0; i < num_lm_addrs; ++i) {
	  (cur_pcl->mylmaddrs_)->add_lm_addr(lmaddr[i]);
	}
	parent_pcl->UpdateChildLMAddr(myaddr_,num_lm_addrs,lmaddr);
      }
    }
    
    int num_addrs = 0;
    int64_t *assigned_addrs = NULL;
    (cur_pcl->mylmaddrs_)->get_lm_addrs(&num_addrs,&assigned_addrs);

    if(num_addrs == 0) {
      pchild = cur_pcl->pchildren_;
      while(pchild) {
	if(pchild->child_flag_ == IS_CHILD) {
	  (pchild->lmaddr_)->delete_lm_addrs();
	  if(pchild->id_ == myaddr_ && level)
	      (child_pcl->mylmaddrs_)->delete_lm_addrs();
	}
	pchild = pchild->next_;
      }
    }
    else if(cur_pcl->num_children_) {
      // ID at a particular level starts from 1
      for(i = 0; i < num_addrs; ++i) {
	int64_t j = 1;
	int64_t addr = assigned_addrs[i] + (j << ((cur_pcl->level_-1)*8));
	// Assign addresses to child nodes
	
	//	while( j <= MAX_CHILDREN) {
	pchild = cur_pcl->pchildren_;
	assert(cur_pcl->num_children_ <= MAX_CHILDREN);
	
	while(pchild) {
	  if(pchild->child_flag_ == IS_CHILD) {
	    (pchild->lmaddr_)->delete_lm_addrs();
	    (pchild->lmaddr_)->add_lm_addr(addr);
	    if(pchild->id_ == myaddr_ && level) {
	      (child_pcl->mylmaddrs_)->delete_lm_addrs();
	      (child_pcl->mylmaddrs_)->add_lm_addr(addr);
	    }
	    ++j;
	    addr = assigned_addrs[i] + (j << ((cur_pcl->level_-1)*8));
	    //	    if(j > MAX_CHILDREN) break;
	    assert(j <= MAX_CHILDREN);
	  } /* if */
	  pchild = pchild->next_;
	  //	  }/* while */
	}
      }
    }
    if(num_addrs) delete[] assigned_addrs;
    --level;
  }
}




void
LandmarkAgent::periodic_callback(Event *e, int level)
{
  //  if(debug_) printf("Periodic Callback for level %d", level);
  Scheduler &s = Scheduler::instance();
  double now = Scheduler::instance().clock(), next_update_delay;
  int energy = (int)(node_->energy());
  int unicast_flag = FALSE, suppress_flag = FALSE;
  Packet *newp;
  hdr_ip *iph, *new_iph;
  hdr_cmn *cmh, *new_cmh;
  int action = PERIODIC_ADVERTS, parent_changed = 0, child_changed = 0;
  int upd_time = (int) now;

  //  if(now > 412.5) {
  //    purify_printf("periodic_callback1: %f,%d\n",now,myaddr_);
  //    purify_new_leaks();
  //  }

  //  if(myaddr_ == 12 && now > 402)
  //    purify_new_leaks();

  // Should always have atleast the level 0 object
  assert(parent_children_list_ != NULL); 
  ParentChildrenList *pcl = parent_children_list_;
  ParentChildrenList *cur_pcl = NULL;
  ParentChildrenList *new_pcl = NULL;
  ParentChildrenList *pcl1 = NULL;
  ParentChildrenList *pcl2 = NULL;

  // return if we have been demoted from this level
  if(highest_level_ < level) 
    return; 
  
  while(pcl != NULL) {
    new_pcl = pcl;
    if(pcl->level_ == level){
      cur_pcl = pcl;
    }
    if(pcl->level_ == (level - 1)){
      pcl1 = pcl;
    }
    if(pcl->level_ == (level + 1)){
      pcl2 = pcl;
    }
    pcl = pcl->next_;
  }
  
  assert(cur_pcl);
  if(level)
    assert(pcl1);

  // Create level+1 object if it doesnt exist
  if(!pcl2) {
    new_pcl->next_ = new ParentChildrenList(level+1, this);
    pcl2 = new_pcl->next_;
  }

  assert(pcl2);

  // Delete stale potential parent entries
  LMNode *lmnode = cur_pcl->pparent_;
  LMNode *tmp_lmnode;
  int delete_flag = TRUE;
  LMNode *old_parent = cur_pcl->parent_;
  while(lmnode) {
    // Record next entry in linked list incase the current element is deleted
    tmp_lmnode = lmnode->next_;
    if(((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_)) {
      //      if(debug_) trace("Node %d: Deleting stale entry for %d at time %d",myaddr_,lmnode->id_,(int)now);
      upd_time = (int) now;
      upd_time = (upd_time << 16) | ((lmnode->last_upd_seqnum_ + 1) & 0xFFFF);
      cur_pcl->UpdatePotlParent(lmnode->id_, 0, 0, 0, 0, 0, upd_time, delete_flag);
    }
    lmnode = tmp_lmnode;
  }
  
  // The first condition may arise if the old parent obj is deleted ... (?)
  if(cur_pcl->parent_ == old_parent && old_parent) {
    if((cur_pcl->parent_)->id_ != old_parent->id_)
      parent_changed = 1;
  }
  else if(cur_pcl->parent_ != old_parent && cur_pcl->parent_ && old_parent) {
    if((cur_pcl->parent_)->id_ != old_parent->id_)
      parent_changed = 1;
  }
  else if(cur_pcl->parent_ != old_parent)
    parent_changed = 1;
  

  // Delete stale potential children entries
  lmnode = cur_pcl->pchildren_;
  delete_flag = TRUE;
  int demotion = FALSE;
  while(lmnode) {
    // Record next entry in linked list incase the current element is deleted
    tmp_lmnode = lmnode->next_;
    if((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_) {
      if(lmnode->child_flag_ == IS_CHILD)
	child_changed = 1;
      assert(level && lmnode->id_ != myaddr_);
      upd_time = (int) now;
      upd_time = (upd_time << 16) | ((lmnode->last_upd_seqnum_ + 1) & 0xFFFF);
      cur_pcl->UpdatePotlChild(lmnode->id_, 0, 0, 0, 0, 0, upd_time, lmnode->child_flag_, delete_flag,NULL);
    }
    lmnode = tmp_lmnode;
  }
  
  // Send updates if tag list has changed i.e., when a child has changed
  if(child_changed)
    SendChangedTagListUpdate(0,level-1);
  
  // Demote ourself if any child's energy > 30 % of our energy
  if(demotion) {
    trace("Node %d: Demotion due to lesser energy than child",myaddr_);
    highest_level_ = level - 1;
    Packet *p = makeUpdate(cur_pcl, HIER_ADVS, DEMOTION);
    s.schedule(target_, p, 0);
  }


  // Check if a parent exists after updating potl parents. If not, start
  // promotion timer.
  // A LM at level 3 is also at levels 0, 1 and 2. For each of these levels,
  // the LM designates itself as parent. At any given instant, only the 
  // level 3 (i.e., highest_level_) LM may not have a parent and may need to 
  // promote itself. But if the promotion timer is running, then the election
  // process for the next level has already begun.

  if(parent_changed && (cur_pcl->parent_ == NULL) && !demotion) {

    // Cancel any promotion timer that is running for promotion from a higher 
    // level and send out demotion messages
    if(promo_timer_running_ && level <= highest_level_) {
      wait_state_ = 0;
      total_wait_time_ = 0;
      promo_timer_running_ = 0;
      promo_timer_->cancel();
    }

    num_resched_ = pcl2->num_heard_ - 1;
    if(num_resched_) {
      promo_timer_running_ = 1;
      wait_state_ = 0;
      total_wait_time_ = 0;
      promo_timeout_ = random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * radius(level+1) + MAX_TIMEOUT/((num_resched_+1) * pow(2,highest_level_+1))), be_random_);
      trace("Node %d: Promotion timeout after wait period in periodic_callback: %f", myaddr_,promo_timeout_);
      num_resched_ = 0;
      promo_start_time_ = s.clock();
      promo_timer_->resched(promo_timeout_);
    }
    else {
      double wait_time = PERIODIC_WAIT_TIME;
      promo_timer_running_ = 1;
      wait_state_ = 1;
      total_wait_time_ += wait_time;
      //	trace("Node %d: Entering wait period in periodic_callback at time %f", myaddr_,now);
      promo_timer_->resched(wait_time);
    }
  }

  // Update ourself as potential child and parent for appropriate levels
  // in our hierarchy tables
  if(!demotion) {
    if(level) {
      upd_time = (int) now;
      upd_time = (upd_time << 16) | (pcl1->seqnum_ & 0xFFFF);
      ++(pcl1->seqnum_);
      pcl1->UpdatePotlParent(myaddr_, myaddr_, 0, level, cur_pcl->num_children_, energy, upd_time, FALSE);
    }
    upd_time = (int) now;
    upd_time = (upd_time << 16) | (pcl2->seqnum_ & 0xFFFF);
    ++(pcl2->seqnum_);
    pcl2->UpdatePotlChild(myaddr_, myaddr_, 0, level, cur_pcl->num_children_, energy, upd_time, IS_CHILD, FALSE,cur_pcl->tag_list_);
  }
  
  // Check if this is the root node. If so, set the unicast flag or suppress
  // flag when no changes occur for 3 times the update period
  // If this is a lower level node that has a parent, either suppress 
  // (for hard-state case) or unicast maintenance messages
  if(!(cur_pcl->parent_) && (total_wait_time_ >= (2*PERIODIC_WAIT_TIME)) && wait_state_) {
    if(adverts_type_ == UNICAST) {
      unicast_flag = TRUE;
    }
    else if(adverts_type_ == SUPPRESS) {
      suppress_flag = TRUE;
    }
    
    // Start assigning landmark addresses to child nodes; 
    // Shift 1, number of levels * 8 times to left for address of root node
    int64_t *lmaddr = new int64_t[1];
    lmaddr[0] = 1;
    lmaddr[0] = (lmaddr[0] << (cur_pcl->level_ * 8));
    int num_lm_addrs = 1;
    assign_lmaddress(lmaddr, num_lm_addrs, cur_pcl->level_);
    // The advertisements from the root LM need to be broadcast in the hash
    // scheme
    delete[] lmaddr;
    if(global_lm_)
      action = GLOBAL_ADVERT;
  }
  else if(cur_pcl->parent_) {
    if(adverts_type_ == UNICAST) {
      unicast_flag = TRUE;
    }
    else if(adverts_type_ == SUPPRESS) {
      suppress_flag = TRUE;
    }
  } 
  
  //  if(!demotion && (now - cur_pcl->last_update_sent_ >= cur_pcl->update_period_) && !suppress_flag) 
  if(!demotion && !suppress_flag) {
    //    trace("Node %d: Sending update at time %f",myaddr_,now);
    Packet *p = makeUpdate(cur_pcl, HIER_ADVS, action);
    unsigned char *walk;
    if(unicast_flag) {
      if(level) {
	// Unicast updates to parent and children for level > 0
	lmnode = cur_pcl->pchildren_;
	while(lmnode) {
	  if(lmnode->id_ != myaddr_) {
	    newp = p->copy();
	    new_iph = HDR_IP(newp);
	    new_cmh = HDR_CMN(newp);
	    walk = newp->accessdata();
	    //    trace("Node %d: Generating unicast advert to child %d at time %f with next hop %d",myaddr_,lmnode->id_,now,lmnode->next_hop_);
	    
	    new_iph->daddr() = lmnode->id_ << 8;
	    new_iph->dport() = ROUTER_PORT;
	    new_cmh->next_hop() = lmnode->next_hop_;
	    new_cmh->addr_type() = NS_AF_INET;
	    if(cur_pcl->level_)
	      new_cmh->size() = new_cmh->size() - 4 * (cur_pcl->num_potl_children_ - 1);
	    (*walk) = (UNICAST_ADVERT_CHILD) & 0xFF;
	    walk++;
	    int num_addrs = (*walk);
	    walk += (10 + 8*num_addrs);
	    
	    // Update seqnum field for each packet; Otherwise subsequent 
            // (to first) messages would be dropped by intermediate nodes
	    (*walk++) = (cur_pcl->seqnum_ >> 24) & 0xFF;
	    (*walk++) = (cur_pcl->seqnum_ >> 16) & 0xFF;
	    (*walk++) = (cur_pcl->seqnum_ >> 8) & 0xFF;
	    (*walk++) = (cur_pcl->seqnum_) & 0xFF;
	    ++(cur_pcl->seqnum_);
	    
	    s.schedule(target_,newp,0);
	  }
	  lmnode = lmnode->next_;
	}
      }
      if(cur_pcl->parent_) {
	if((cur_pcl->parent_)->id_ != myaddr_) {
	  iph = HDR_IP(p);
	  cmh = HDR_CMN(p);
	  walk = p->accessdata();
	  
	  //	  trace("Node %d: Generating unicast advert to parent %d at time %f with next hop %d",myaddr_,cur_pcl->parent_->id_,now,(cur_pcl->parent_)->next_hop_);
	  iph->daddr() = (cur_pcl->parent_)->id_;
	  iph->dport() = ROUTER_PORT;
	  cmh->next_hop() = (cur_pcl->parent_)->next_hop_;
	  cmh->addr_type() = NS_AF_INET;
	  cmh->size() = cmh->size() - 4 * (cur_pcl->num_potl_children_);
	  
	  (*walk) = (UNICAST_ADVERT_PARENT) & 0xFF;
	  walk++;
	  int num_addrs = (*walk);
	  walk += (10 + 8*num_addrs);

	  // Update seqnum field for each packet; Otherwise subsequent 
	  // (to first) messages would be dropped by intermediate nodes
	  (*walk++) = (cur_pcl->seqnum_ >> 24) & 0xFF;
	  (*walk++) = (cur_pcl->seqnum_ >> 16) & 0xFF;
	  (*walk++) = (cur_pcl->seqnum_ >> 8) & 0xFF;
	  (*walk++) = (cur_pcl->seqnum_) & 0xFF;
	  ++(cur_pcl->seqnum_);
	  
	  s.schedule(target_,p,0);
	}
	else 
	  Packet::free(p);
      }
      else
	Packet::free(p);
    }
    else {
      //      trace("Node %d: Generating update msg at time %f",myaddr_,now);
      s.schedule(target_, p, 0);
    }
    cur_pcl->last_update_sent_ = now;
  }

  // Schedule next update 
  if(cur_pcl->last_update_sent_ == now || suppress_flag)
    next_update_delay = cur_pcl->update_period_ + jitter(LM_STARTUP_JITTER, be_random_);
  else
    next_update_delay = cur_pcl->update_period_ - (now - cur_pcl->last_update_sent_) + jitter(LM_STARTUP_JITTER, be_random_);
  

  if(!demotion)
    s.schedule(cur_pcl->periodic_handler_, cur_pcl->periodic_update_event_, next_update_delay);

  //  if(now > 412.5) {
  //    purify_printf("periodic_callback2: %f,%d\n",now,myaddr_);
  //    purify_new_leaks();
  //  }

  //  if(myaddr_ == 12 && now > 402)
  //    purify_new_leaks();

  // Update entries for levels greater than our highest level in our
  // highest_level_ periodic_callback event
  if(level == highest_level_) {
    cur_pcl = parent_children_list_;
    while(cur_pcl) {
      if(cur_pcl->level_ > highest_level_) {
	lmnode = cur_pcl->pparent_;
	delete_flag = TRUE;
	while(lmnode) {
	  
	  // Update potential parent list
	  // Record next entry in list incase current element is deleted
	  tmp_lmnode = lmnode->next_;
	  if(((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_)) {
	    //      if(debug_) trace("Node %d: Deleting stale entry for %d at time %d",myaddr_,lmnode->id_,(int)now);
	    upd_time = (int) now;
	    upd_time = (upd_time << 16) | ((lmnode->last_upd_seqnum_+1) & 0xFFFF);
	    cur_pcl->UpdatePotlParent(lmnode->id_, 0, 0, 0, 0, 0, upd_time, delete_flag);
	  }
	  lmnode = tmp_lmnode;
	}
	
	// Update children list
	lmnode = cur_pcl->pchildren_;
	while(lmnode) {
	  // Record next entry in list incase current element is deleted
	  tmp_lmnode = lmnode->next_;
	  if((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_) {
	    upd_time = (int) now;
	    upd_time = (upd_time << 16) | ((lmnode->last_upd_seqnum_+1) & 0xFFFF);
	    // Check if global LM entry is being deleted; Global LM at level i
	    // will have entry in level i+1 pcl object
	    if(cur_pcl->level_ == global_lm_level_+1 && lmnode->id_ == global_lm_id_) {
	      global_lm_level_ = -1;
	      global_lm_id_ = NO_GLOBAL_LM;
	    }

	    cur_pcl->UpdatePotlChild(lmnode->id_, 0, 0, 0, 0, 0, upd_time, lmnode->child_flag_, delete_flag,NULL);
	  }
	  lmnode = tmp_lmnode;
	}
      }
      cur_pcl = cur_pcl->next_;
    }
  }
}




Packet *
LandmarkAgent::makeUpdate(ParentChildrenList *pcl, int pkt_type, int action)
{
  Packet *p = allocpkt();
  hdr_ip *iph = HDR_IP(p);
  hdr_cmn *hdrc = HDR_CMN(p);
  unsigned char *walk;
  compr_taglist *adv_tags = NULL;
  double now = Scheduler::instance().clock();
  int64_t *lmaddrs;
  int num_lm_addrs = 0;

  hdrc->next_hop_ = IP_BROADCAST; // need to broadcast packet
  hdrc->addr_type_ = NS_AF_INET;
  iph->daddr() = IP_BROADCAST;  // packet needs to be broadcast
  iph->dport() = ROUTER_PORT;
  iph->ttl_ = radius(pcl->level_);

  iph->saddr() = myaddr_;
  iph->sport() = ROUTER_PORT;

  if(action == GLOBAL_ADVERT)
    trace("Node %d: Generating global LM message at time %f",myaddr_,now);

  assert(pcl->num_tags_ >= 0);

  if(pkt_type == HIER_ADVS) {
    
    if(pcl->level_ == 0) {
      // A level 0 node cannot be demoted!
      assert(action != DEMOTION);

      // No children for level 0 LM
      // totally 12 bytes in packet for now; need to add our energy level later
      // each tag name is 4 bytes; 2 bytes for num_tags info
      // Landmark address; 1 byte to indicate #addrs and 8 bytes for each addr
      lmaddrs = NULL;
      num_lm_addrs = 0;
      (pcl->mylmaddrs_)->get_lm_addrs(&num_lm_addrs, &lmaddrs);

      p->allocdata(12 + (4 * pcl->num_tags_) + 2 + 4 + 1 + (8*num_lm_addrs)); 
      walk = p->accessdata();

      // Packet type; 1 byte
      (*walk++) = (action) & 0xFF;

      // Landmark address; 1 byte to indicate #addrs and 8 bytes for each addr
      (*walk++) = (num_lm_addrs) & 0xFF;
      //      if(num_lm_addrs)
      //	trace("Num_lm_addrs = %d",num_lm_addrs);
      for(int i = 0; i < num_lm_addrs; ++i) {
	// Landmark address of node
	(*walk++) = (lmaddrs[i] >> 56) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 48) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 40) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 32) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 24) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 16) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 8) & 0xFF;
	(*walk++) = (lmaddrs[i]) & 0xFF;
      }
      if(num_lm_addrs) 
	delete[] lmaddrs;

      // level of LM advertisement; 1 byte
      (*walk++) = (pcl->level_) & 0xFF;

      // Our energy level; 4 bytes (just integer portion)
      int energy = (int)(node_->energy());
      (*walk++) = (energy >> 24) & 0xFF;
      (*walk++) = (energy >> 16) & 0xFF;
      (*walk++) = (energy >> 8) & 0xFF;
      (*walk++) = (energy) & 0xFF;

      // make ourselves as next hop; 2 bytes
      (*walk++) = (myaddr_ >> 8) & 0xFF;
      (*walk++) = (myaddr_) & 0xFF;

      // Vicinity size in number of hops; Carrying this information allows
      // different LMs at same level to have different vicinity radii; 2 bytes
      (*walk++) = (radius(pcl->level_) >> 8) & 0xFF;
      (*walk++) = (radius(pcl->level_)) & 0xFF;

      // Time at which packet was originated;
      // 3 bytes for integer portion of time and 1 byte for fraction
      // Note that we probably need both an origin_time and sequence number
      // field to distinguish between msgs generated at same time.
      // (origin_time required to discard old state when net dynamics present)
      int origin_time = (int)now;
      (*walk++) = (origin_time >> 8) & 0xFF;
      (*walk++) = (origin_time) & 0xFF;
      (*walk++) = (pcl->seqnum_ >> 8) & 0xFF;
      (*walk++) = (pcl->seqnum_) & 0xFF;
      ++(pcl->seqnum_);

      // Parent ID; 2 bytes
      if(pcl->parent_ == NULL) {
	(*walk++) = (NO_PARENT >> 8) & 0xFF;
	(*walk++) = (NO_PARENT) & 0xFF;
      }
      else {
	(*walk++) = ((pcl->parent_)->id_ >> 8) & 0xFF;
	(*walk++) = ((pcl->parent_)->id_) & 0xFF;
      }

      (*walk++) = (pcl->num_tags_ >> 8) & 0xFF;
      (*walk++) = (pcl->num_tags_) & 0xFF;
      if(pcl->num_tags_) {
	adv_tags = pcl->tag_list_;
	while(adv_tags) {
	  (*walk++) = (adv_tags->obj_name_ >> 24) & 0xFF;
	  (*walk++) = (adv_tags->obj_name_ >> 16) & 0xFF;
      	  (*walk++) = (adv_tags->obj_name_ >> 8) & 0xFF;
	  (*walk++) = (adv_tags->obj_name_) & 0xFF;
	  adv_tags = adv_tags->next_;
	}
      }

      // In real life each of the above fields would be 
      // 4 byte integers; 20 bytes for IP addr + 2 bytes for num_children
      // 4 byte for number of tags; 4 byte for each tag name; 4 byte for energy
      hdrc->size_ = 20 + 20 + 4 + (4 * pcl->num_tags_) + 4 + 1 + (8*num_lm_addrs); 
    }
    else {
      // Need to list all the potential children LMs
      // Pkt size: 12 bytes (as before); 2 each for number of children 
      // and potl_children; 
      // 2 byte for each child's id + 8 bytes for landmark address
      // 4 bytes for each tag name; 2 bytes for num_tags info
      int pkt_size = 0;
      lmaddrs = NULL;
      num_lm_addrs = 0;
      if(action == PERIODIC_ADVERTS || action == GLOBAL_ADVERT) {
	LMNode *pch = pcl->pchildren_;

	// Compute number of landmark addrs of children for pkt size calc
	int size = 0;
	while(pch) {
	  int64_t *addrs;
	  int num_addrs = 0;
	  (pch->lmaddr_)->get_lm_addrs(&num_addrs,&addrs);
	  if(num_addrs) delete[] addrs;
	  size += (1 + num_addrs*8);
	  pch = pch->next_;
	}

	// Compute number of landmark addrs of this node for pkt size calc
	// Landmark address; 1 byte to indicate #addrs and 8 bytes for each 
	// addr
	(pcl->mylmaddrs_)->get_lm_addrs(&num_lm_addrs, &lmaddrs);

	pkt_size = 12 + 4 + (2 * pcl->num_potl_children_) + size + (4 * pcl->num_tags_) + 2 + 4 + 1 + (8*num_lm_addrs);
      }
      else
	pkt_size = 17; // Demotion message

      p->allocdata(pkt_size);
      walk = p->accessdata();

      // Packet type; 1 byte
      (*walk++) = (action) & 0xFF;

      //      if(myaddr_ == 0)
      //	trace("Node 0: Generating update message for level %d at time %f, num_lm_addrs %d",pcl->level_,now,num_lm_addrs);

      // Landmark address; 1 byte to indicate #addrs and 8 bytes for each addr
      (*walk++) = (num_lm_addrs) & 0xFF;
      for(int i = 0; i < num_lm_addrs; ++i) {
	// Landmark address of node
	(*walk++) = (lmaddrs[i] >> 56) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 48) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 40) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 32) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 24) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 16) & 0xFF;
	(*walk++) = (lmaddrs[i] >> 8) & 0xFF;
	(*walk++) = (lmaddrs[i]) & 0xFF;
      }
      if(num_lm_addrs) 
	delete[] lmaddrs;

      
      // level of LM advertisement; 1 byte
      (*walk++) = (pcl->level_) & 0xFF;

      // Our energy level; 4 bytes (just integer portion)
      int energy = (int)(node_->energy());
      (*walk++) = (energy >> 24) & 0xFF;
      (*walk++) = (energy >> 16) & 0xFF;
      (*walk++) = (energy >> 8) & 0xFF;
      (*walk++) = (energy) & 0xFF;

      // make ourselves as next hop; 2 bytes
      (*walk++) = (myaddr_ >> 8) & 0xFF;
      (*walk++) = (myaddr_) & 0xFF;

      // Vicinity size in number of hops; 2 bytes
      (*walk++) = (radius(pcl->level_) >> 8) & 0xFF;
      (*walk++) = (radius(pcl->level_)) & 0xFF;

      // Time at which packet was originated;
      // 3 bytes for integer portion of time and 1 byte for fraction
      int origin_time = (int)now;
      (*walk++) = (origin_time >> 8) & 0xFF;
      (*walk++) = (origin_time) & 0xFF;
      (*walk++) = (pcl->seqnum_ >> 8) & 0xFF;
      (*walk++) = (pcl->seqnum_) & 0xFF;
      ++(pcl->seqnum_);

      if(origin_time > now) {
	printf("Node %d: id %d, level %d, vicinity_radius %d",myaddr_,myaddr_,pcl->level_,radius(pcl->level_));
	assert(origin_time < now);
      }

      // Parent's id; 2 bytes
      if(pcl->parent_ == NULL) {
	(*walk++) = (NO_PARENT >> 8) & 0xFF;
	(*walk++) = (NO_PARENT) & 0xFF;
      }
      else {
	(*walk++) = ((pcl->parent_)->id_ >> 8) & 0xFF;
	(*walk++) = ((pcl->parent_)->id_) & 0xFF;
      }
      
      if(action == PERIODIC_ADVERTS || action == GLOBAL_ADVERT) {
	// Number of children; 2 bytes
	(*walk++) = (pcl->num_children_ >> 8) & 0xFF;
	(*walk++) = (pcl->num_children_) & 0xFF;
	
	// Number of potential children; 2 bytes
	(*walk++) = (pcl->num_potl_children_ >> 8) & 0xFF;
	(*walk++) = (pcl->num_potl_children_) & 0xFF;
	
	LMNode *potl_ch = pcl->pchildren_;
	while(potl_ch) {
	  if(potl_ch->child_flag_ != NOT_POTL_CHILD) {
	    (*walk++) = (potl_ch->id_ >> 8) & 0xFF;
	    (*walk++) = (potl_ch->id_) & 0xFF;
	    int64_t *addrs = NULL;
	    int num_addrs = 0;
	    ((potl_ch)->lmaddr_)->get_lm_addrs(&num_addrs, &addrs);

	    //	    if(myaddr_ == 0 && now > 1000)
	    //	      trace("Node 0: Child %d, num_addrs: %d at time %f",potl_ch->id_,num_addrs,now);

	    // Number of landmark addrs
	    (*walk++) = (num_addrs) & 0xFF;
	    for(int i = 0; i < num_addrs; ++i) {
	      // Landmark address of node
	      (*walk++) = (addrs[i] >> 56) & 0xFF;
	      (*walk++) = (addrs[i] >> 48) & 0xFF;
	      (*walk++) = (addrs[i] >> 40) & 0xFF;
	      (*walk++) = (addrs[i] >> 32) & 0xFF;
	      (*walk++) = (addrs[i] >> 24) & 0xFF;
	      (*walk++) = (addrs[i] >> 16) & 0xFF;
	      (*walk++) = (addrs[i] >> 8) & 0xFF;
	      (*walk++) = (addrs[i]) & 0xFF;
	    }
	    if(num_addrs) 
	      delete[] addrs;
	  }
	  potl_ch = potl_ch->next_;
	}

	(*walk++) = (pcl->num_tags_ >> 8) & 0xFF;
	(*walk++) = (pcl->num_tags_) & 0xFF;
	if(pcl->num_tags_) {
	  adv_tags = pcl->tag_list_;
	  while(adv_tags) {
	    (*walk++) = (adv_tags->obj_name_ >> 24) & 0xFF;
	    (*walk++) = (adv_tags->obj_name_ >> 16) & 0xFF;
	    (*walk++) = (adv_tags->obj_name_ >> 8) & 0xFF;
	    (*walk++) = (adv_tags->obj_name_) & 0xFF;
	    adv_tags = adv_tags->next_;
	  }
	}
	
	// 8 addl bytes for num_children and num_potl_children info; Assuming 
	// worst case of 8 levels in computing packet size
	// SHOULD DISABLE SENDING TAG INFO IN THE HASH SCHEME
	// Landmark address; 1 byte to indicate #addrs and 8 bytes 
	// for each addr
	hdrc->size_ = 20 + 8 + ((4+8) * pcl->num_potl_children_) + 20 + 4 + (4 * pcl->num_tags_) + 4 + 1 + (8 * num_lm_addrs); 
	// In real life each of the above fields would be
	// 4 byte integers; 20 bytes for IP addr
	//	if(myaddr_ == 11) 
	//	  trace("Node 11: Packet size: %d",hdrc->size_);
      }
      else if(action == DEMOTION) {
	hdrc->size_ = 20 + 20;
      }      
    }
  }

  // Optimization for reducing energy consumption; Just advertise
  // sequence number in steady state
  //  if(pcl->parent_ == NULL && action != DEMOTION) 
  //    hdrc->size_ = 20 + 4;

  // Cancel periodic_callback event if node is being demoted
  if(action == DEMOTION && pcl->periodic_update_event_->uid_)
      Scheduler::instance().cancel(pcl->periodic_update_event_);

  hdrc->direction() = hdr_cmn::DOWN;
  return p;
}




int
LandmarkAgent::radius(int level)
{
  // level i's radius >= (2 *level i-1's radius) + 1
  return((int(pow(2,level+1) + pow(2,level) - 1)));

  //  return((level + 1)*2 + 1);
  //  return(int(pow(2,level+1)) + 1);
}




ParentChildrenList::ParentChildrenList(int level, LandmarkAgent *a) : parent_(NULL), num_heard_(0), num_children_(0), num_potl_children_(0), num_pparent_(0), pchildren_(NULL), pparent_(NULL) , seqnum_(0) ,last_update_sent_(-(a->update_period_)), update_period_(a->update_period_), update_timeout_(a->update_timeout_), next_(NULL)
{
  level_ = level;
  periodic_update_event_ = new Event;
  periodic_handler_ = new LMPeriodicAdvtHandler(this);
  a_ = a;
  tag_list_ = NULL;
  num_tags_ = 0;
  adverts_type_ = FLOOD; // default is to flood adverts
  mylmaddrs_ = new LMAddrs;
}




void
PromotionTimer::expire(Event *e) {
  ParentChildrenList *pcl = a_->parent_children_list_;
  ParentChildrenList *new_pcl, *higher_level_pcl = NULL, *lower_level_pcl;
  ParentChildrenList *pcl1 = NULL; // Pointer to new highest_level_-1 object
  ParentChildrenList *pcl2 = NULL; // Pointer to new highest_level_+1 object
  ParentChildrenList *cur_pcl = NULL;
  int found = FALSE, has_parent = FALSE;
  int64_t *my_lm_addrs = NULL;
  int num_my_lm_addrs = 0;
  int num_potl_ch = 0;
  int addr_changed = 0;
  Scheduler &s = Scheduler::instance();
  double now = s.clock();
  Packet *p, *newp;

  //  if(now > 412.5) {
  //    purify_printf("expire1: %f,%d\n",now,a_->myaddr_);
  //    purify_new_leaks();
  //  }

  while(pcl != NULL) {
    if(pcl->level_ == (a_->highest_level_ + 1)) {
      // Exclude ourself from the count of the lower level nodes heard
      higher_level_pcl = pcl;
      a_->num_resched_ = pcl->num_heard_ - 1;
      num_potl_ch = pcl->num_potl_children_;
    }
    else if(pcl->level_ == a_->highest_level_) {
      cur_pcl = pcl;
      if(pcl->parent_) {
	has_parent = TRUE;
      }
    }
    else if(pcl->level_ == (a_->highest_level_-1)) {
      lower_level_pcl = pcl;
    }
    pcl = pcl->next_;
  }

  assert(higher_level_pcl);
  if(a_->highest_level_)
    assert(lower_level_pcl);
  assert(cur_pcl);

  if(a_->wait_state_) {

    if(a_->num_resched_ && !has_parent) {
      a_->wait_state_ = 0;
      a_->total_wait_time_ = 0;
      
      // Promotion timeout is num_resched_ times the estimated time for 
      // a message to reach other nodes in its vicinity
      // PROM0_TIMEOUT_MULTIPLE is an estimate of time for adv to reach 
      // all nodes in vicinity
      a_->promo_timeout_ = a_->random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * a_->radius(a_->highest_level_) + MAX_TIMEOUT/((a_->num_resched_+1) * pow(2,a_->highest_level_))), a_->be_random_);

      //      a_->promo_timeout_ = a_->random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * a_->radius(a_->highest_level_) + MAX_TIMEOUT/((a_->num_resched_+1) * pow(2,a_->highest_level_))), a_->be_random_) + (MAX_ENERGY - (a_->node_)->energy())*200/MAX_ENERGY;

      a_->trace("Node %d: Promotion timeout after wait period in expire1: %f at time %f", a_->myaddr_,a_->promo_timeout_,s.clock());
      a_->num_resched_ = 0;
      a_->promo_start_time_ = s.clock();
      a_->promo_timer_->resched(a_->promo_timeout_);
    }
    else {
      double wait_time = PERIODIC_WAIT_TIME;
      a_->total_wait_time_ += wait_time;
      //      a_->trace("Node %d: Entering wait period in expire1 at time %f, highest level %d", a_->myaddr_,now,a_->highest_level_);
      a_->promo_timer_->resched(wait_time);

      // Demote ourself we do not have any children (excluding ourself) after
      // waiting for 1.5 times the update period
      if(a_->highest_level_ && (a_->total_wait_time_ > (a_->update_period_*1.5))) {
	//	a_->trace("Node %d: cur_pcl's number of children %d",a_->myaddr_,cur_pcl->num_children_);

	// Demote ourself from this level if we have only one children
	// and we have more than one potential parent at lower level
	// If we dont have more than one potl parent at lower level,
	// this node will oscillate between the two levels
	 if(cur_pcl->num_children_ == 1 && lower_level_pcl->num_pparent_ > 1) {
	   a_->trace("Node %d: Demoting from level %d because of no children at time %f",a_->myaddr_,a_->highest_level_,s.clock());

	   // Update appropriate lists
	   int delete_flag = TRUE;
	   int upd_time = (int) now;
	   upd_time = (upd_time << 16) | (lower_level_pcl->seqnum_ & 0xFFFF);
	   ++(lower_level_pcl->seqnum_);
	   lower_level_pcl->UpdatePotlParent(a_->myaddr_, 0, 0, 0, 0, 0, upd_time, delete_flag);

	   upd_time = (int) now;
	   upd_time = (upd_time << 16) | (higher_level_pcl->seqnum_ & 0xFFFF);
	   ++(higher_level_pcl->seqnum_);
	   higher_level_pcl->UpdatePotlChild(a_->myaddr_, 0, 0, 0, 0, 0, upd_time, IS_CHILD, delete_flag,NULL);

	   --(a_->highest_level_);
	   Packet *p = a_->makeUpdate(cur_pcl, HIER_ADVS, DEMOTION);
	   s.schedule(a_->target_,p,0);
	 }
      }
      else if(!(cur_pcl->parent_) && (a_->total_wait_time_ >= (2*PERIODIC_WAIT_TIME))) {
	// We must be the global LM
	a_->global_lm_id_ = a_->myaddr_;
	a_->global_lm_level_ = a_->highest_level_;

	// Get LM addresses if any
	(cur_pcl->mylmaddrs_)->get_lm_addrs(&num_my_lm_addrs,&my_lm_addrs);

	// Start assigning landmark addresses to child nodes; 
	// Shift 1, number of levels * 8 times to left for address of root node
	int64_t *lmaddr = new int64_t[1];
	lmaddr[0] = 1;
	lmaddr[0] = (lmaddr[0] << (cur_pcl->level_ * 8));
	int num_lm_addrs = 1;

	assert(num_my_lm_addrs <= 1);
	if(num_my_lm_addrs == 0) {
	  addr_changed = 1;
	}
	else {
	  if(my_lm_addrs[0] != lmaddr[0])
	    addr_changed = 1;
	}
	
	if(num_my_lm_addrs) delete[] my_lm_addrs;
	
	if(addr_changed) {
	  a_->trace("Node %d: LM addrs being assigned by global LM at time %f, level %d",a_->myaddr_,now,a_->highest_level_);
	  a_->assign_lmaddress(lmaddr, num_lm_addrs, cur_pcl->level_);
	  if(a_->global_lm_)
	    p = a_->makeUpdate(cur_pcl, HIER_ADVS, GLOBAL_ADVERT);
	  else
	    p = a_->makeUpdate(cur_pcl, HIER_ADVS, PERIODIC_ADVERTS);
	  a_->trace("Node %d: Generating ReHash msg at time %f",a_->myaddr_,NOW);
	  a_->GenerateReHashMsg(lmaddr[0],NOW);
	  // Generate updates for LM at lower levels as well since their LM
	  // addresses have also changed
	  ParentChildrenList *tmp_pcl = a_->parent_children_list_;
	  while(tmp_pcl) {
	    if(tmp_pcl->level_ < cur_pcl->level_) {
	      a_->trace("Node %d: Generating level %d update at time %f",a_->myaddr_,tmp_pcl->level_,now);
	      newp = a_->makeUpdate(tmp_pcl, HIER_ADVS, PERIODIC_ADVERTS);
	      s.schedule(a_->target_,newp,0);
	      tmp_pcl->last_update_sent_ = now;
	    }
	    tmp_pcl = tmp_pcl->next_;
	  }
	  s.schedule(a_->target_, p, 0);
	  cur_pcl->last_update_sent_ = now;
	}

	// The advertisements from the root LM need to be broadcast in the hash
	// scheme
	if(num_lm_addrs) delete[] lmaddr;
      }
    }      
    return;
  }    
  
  // Promotion timer is off
  a_->promo_timer_running_ = 0;

  // Only one promotion timer can be running at a node at a given instant. 
  // On expiry, the node will be promoted one level higher to highest_level_+1
  // Add a parentchildrenlist object for the higher level if one doesnt already
  // exist
  higher_level_pcl = NULL;
  pcl = a_->parent_children_list_;
  while(pcl != NULL) {
    new_pcl = pcl;
    if(pcl->level_ == a_->highest_level_+1){
      found = TRUE;
      higher_level_pcl = pcl;
    }
    if(pcl->level_ == (a_->highest_level_)){
      pcl1 = pcl;
    }
    if(pcl->level_ == (a_->highest_level_+2)){
      pcl2 = pcl;
    }
    pcl = pcl->next_;
  }
  
  // highest_level_-1 object should exist
  assert(pcl1);

  if(pcl1->parent_ != NULL) {
    a_->trace("Node %d: Not promoted to higher level %d\n", a_->myaddr_, a_->highest_level_+1);
    return;
  }

  ++(a_->highest_level_);
  assert(a_->highest_level_ < MAX_LEVELS);

  if(!found) {
    new_pcl->next_ = new ParentChildrenList(a_->highest_level_, a_);
    higher_level_pcl = new_pcl->next_;
    new_pcl = new_pcl->next_;
  }

  // Create highest_level_+1 object if it doesnt exist
  if(!pcl2) {
    new_pcl->next_ = new ParentChildrenList((a_->highest_level_)+1, a_);
    pcl2 = new_pcl->next_;
  }

  assert(pcl2);

  if(a_->debug_) {
    a_->trace("Node %d: Promoted to level %d, num_potl_children %d at time %f", a_->myaddr_, a_->highest_level_, num_potl_ch, now);

    //    LMNode *lm = higher_level_pcl->pchildren_;
    //    a_->trace("Potential Children:");
    //    while(lm) {
    //      a_->trace("%d (level %d) Number of hops: %d", lm->id_,lm->level_,lm->num_hops_);
    //      lm = lm->next_;
    //    }
    
    //    lm = higher_level_pcl->pparent_;
    //    a_->trace("Potential Parent:");
    //    while(lm) {
    //      a_->trace("%d (level %d)", lm->id_,lm->level_);
    //      lm = lm->next_;
    //    }
  }


  // Update tag lists and send out corresponding advertisements
  a_->SendChangedTagListUpdate(0,a_->highest_level_-1);

  // start off periodic advertisements for this higher level
  s.schedule(higher_level_pcl->periodic_handler_, higher_level_pcl->periodic_update_event_, 0);
  
  // add myself as potential parent for highest_level-1 and child for 
  // highest_level+1 
  int num_hops = 0;
  int energy = (int)((a_->node_)->energy());
  int child_flag = IS_CHILD;
  int delete_flag = FALSE;

  int upd_time = (int) now;
  upd_time = (upd_time << 16) | (pcl1->seqnum_ & 0xFFFF);
  ++(pcl1->seqnum_);
  pcl1->UpdatePotlParent(a_->myaddr_, a_->myaddr_, num_hops, a_->highest_level_, higher_level_pcl->num_children_, energy, upd_time, delete_flag);

  // tag_list == NULL doesnt matter because we're at a smaller level than
  // pcl2->level_ at this point. periodic_callback_ will update this field
  // correctly
  upd_time = (int) now;
  upd_time = (upd_time << 16) | (pcl2->seqnum_ & 0xFFFF);
  ++(pcl2->seqnum_);
  pcl2->UpdatePotlChild(a_->myaddr_, a_->myaddr_, num_hops, a_->highest_level_,higher_level_pcl->num_children_, energy, upd_time, child_flag, delete_flag, higher_level_pcl->tag_list_);
  
  // If we havent seen a LM that can be our parent at this higher level, start
  // promotion timer for promotion to next level
  a_->num_resched_ = pcl2->num_heard_ - 1;
  if(higher_level_pcl->parent_ == NULL && a_->num_resched_) {
    //    if (a_->debug_) printf("PromotionTimer's expire method: Scheduling timer for promo to level %d\n",a_->highest_level_);
    // Timer's status is TIMER_HANDLING in handle; so we just reschedule to
    // avoid "cant start timer" abort if sched is called

    a_->promo_timer_running_ = 1;
    a_->wait_state_ = 0;
    a_->total_wait_time_ = 0;
    a_->promo_timeout_ = a_->random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * a_->radius(a_->highest_level_+1) + MAX_TIMEOUT/((a_->num_resched_+1) * pow(2,a_->highest_level_+1))), a_->be_random_);
    a_->trace("Node %d: Promotion timeout after wait period in expire2: %f at time %f, num_resched_ %d, energy %f", a_->myaddr_,a_->promo_timeout_,s.clock(),a_->num_resched_,(a_->node_)->energy());
    a_->num_resched_ = 0;
    a_->promo_start_time_ = s.clock();
    a_->promo_timer_->resched(a_->promo_timeout_);
  }
  else {
    double wait_time = PERIODIC_WAIT_TIME;
    a_->promo_timer_running_ = 1;
    a_->total_wait_time_ = 0;
    a_->wait_state_ = 1;
    a_->total_wait_time_ += wait_time;
    //    a_->trace("Node %d: Entering wait period in expire1 at time %f", a_->myaddr_,now);
    a_->promo_timer_->resched(wait_time);
  }
  

  //  if(now > 412.5) {
  //    purify_printf("expire2: %f,%d\n",now,a_->myaddr_);
  //    purify_new_leaks();
  //  }

}



  

int 
LandmarkAgent::command (int argc, const char *const *argv)
{
  if (argc == 2)
    {
      if (strcmp (argv[1], "start") == 0)
	{
	  startUp();
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "stop") == 0)
	{
	  stop();
	  return (TCL_OK);
	}

      if (strcmp (argv[1], "print-nbrs") == 0)
	{
	  get_nbrinfo();
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "enable-caching") == 0)
	{
	  cache_ = 1;
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "unicast-adverts") == 0)
	{
	  adverts_type_ = UNICAST;
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "hard-state-adverts") == 0)
	{
	  adverts_type_ = SUPPRESS;
	  // Entries should never timeout in a hard-state scheme
	  update_timeout_ = 1000000;
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "enable-global-landmark") == 0)
	{
	  global_lm_ = 1;
	  return (TCL_OK);
	}
      else if (strcmp (argv[1], "dumprtab") == 0)
	{
	  Packet *p2 = allocpkt ();
	  hdr_ip *iph2 = HDR_IP(p2);
	  //	  rtable_ent *prte;

          trace ("Table Dump %d[%d]\n----------------------------------\n",
	    myaddr_, iph2->sport());
	  trace ("VTD %.5f %d:%d\n", Scheduler::instance ().clock (),
		 myaddr_, iph2->sport());
	  trace ("Remaining energy: %f", node_->energy());
	  //	  trace ("Energy consumed by queries: %f", node_->qry_energy());

	  /*
	   * Freeing a routing layer packet --> don't need to
	   * call drop here.
	   */
	  trace("Highest Level: %d", highest_level_);
	  Packet::free (p2);
	  ParentChildrenList *pcl = parent_children_list_;
	  LMNode *pch;
	  while(pcl) {
	    trace("Level %d:", pcl->level_);
	    if(pcl->parent_)
	      trace("Parent: %d", (pcl->parent_)->id_);
	    else
	      trace("Parent: NULL");
	    int num_lm_addrs = 0;
	    int64_t *lmaddrs = NULL;
	    (pcl->mylmaddrs_)->get_lm_addrs(&num_lm_addrs,&lmaddrs);
	    for( int i = 0; i < num_lm_addrs; ++i) {
	      int i1,i2,i3,i4,i5,i6,i7,i8;
	      i1 = (lmaddrs[i] >> 56) & 0xFF;
	      i2 = (lmaddrs[i] >> 48) & 0xFF;
	      i3 = (lmaddrs[i] >> 40) & 0xFF;
	      i4 = (lmaddrs[i] >> 32) & 0xFF;
	      i5 = (lmaddrs[i] >> 24) & 0xFF;
	      i6 = (lmaddrs[i] >> 16) & 0xFF;
	      i7 = (lmaddrs[i] >> 8) & 0xFF;
	      i8 = (lmaddrs[i]) & 0xFF;
	      trace("Landmark Address: %d.%d.%d.%d.%d.%d.%d.%d",i1,i2,i3,i4,i5,i6,i7,i8);
	    }
	    if(num_lm_addrs) delete[] lmaddrs;
	    if(myaddr_ == 134) {
	      pch = pcl->pchildren_;
	      while(pch) {
		int num_addrs = 0;
		int64_t *addrs = NULL;
		(pch->lmaddr_)->get_lm_addrs(&num_addrs,&addrs);
		
		int j1=0,j2=0,j3=0,j4=0,j5=0,j6=0,j7=0,j8=0;
		if(num_addrs) {
		  j1 = (addrs[0] >> 56) & 0xFF;
		  j2 = (addrs[0] >> 48) & 0xFF;
		  j3 = (addrs[0] >> 40) & 0xFF;
		  j4 = (addrs[0] >> 32) & 0xFF;
		  j5 = (addrs[0] >> 24) & 0xFF;
		  j6 = (addrs[0] >> 16) & 0xFF;
		  j7 = (addrs[0] >> 8) & 0xFF;
		  j8 = (addrs[0]) & 0xFF;
		}
		trace("Node %d: Potl Child id %d, LM addr %d.%d.%d.%d.%d.%d.%d.%d, next_hop %d, num_children %d",myaddr_,pch->id_,j1,j2,j3,j4,j5,j6,j7,j8,pch->next_hop_,pch->num_children_);
		if(num_addrs) delete[] addrs;
		pch = pch->next_;
	      }
	    }
	    trace("Number of potl children: %d\n", pcl->num_potl_children_);
	    if(myaddr_ == 166) {
	      trace("Number of children: %d\n", pcl->num_children_);
	      trace("Number of level %d nodes heard: %d\n", (pcl->level_)-1, pcl->num_heard_);
	      
	      trace("Number of potl parent: %d\n", pcl->num_pparent_);
	      if(pcl->level_ >= 1 && highest_level_ >= 1) {
		pch = pcl->pchildren_;
		trace("Potential Children (radius %d):",radius(pcl->level_));
		while(pch) {
		  if(pch->child_flag_ != NOT_POTL_CHILD)
		    trace("Node %d (%d hops away)",pch->id_,pch->num_hops_);
		  pch = pch->next_;
		}

		pch = pcl->pparent_;
		trace("Potential parent:");
		while(pch) {
		  trace("Node %d (%d hops away)",pch->id_,pch->num_hops_);
		  pch = pch->next_;
		} 
	      }
	    }
	    pcl = pcl->next_;
	  }

	  Packet::free(p2);
	  return (TCL_OK);
	}
    }
  else if (argc == 3)
    {
      if (strcasecmp (argv[1], "tracetarget") == 0)
	{
	  TclObject *obj;
	if ((obj = TclObject::lookup (argv[2])) == 0)
	    {
	      fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		       argv[2]);
	      return TCL_ERROR;
	    }
	  tracetarget_ = (Trace *) obj;
	  return TCL_OK;
	}
      else if (strcasecmp (argv[1], "addr") == 0) {
	int temp;
	temp = Address::instance().str2addr(argv[2]);
	myaddr_ = temp;
	return TCL_OK;
      }
      else if (strcasecmp (argv[1], "set-update-period") == 0) {
	update_period_ = atof(argv[2]);
	if(adverts_type_ != SUPPRESS)
	  update_timeout_ = update_period_ + 4 * LM_STARTUP_JITTER;
	return TCL_OK;
      }
      else if (strcasecmp (argv[1], "set-update-timeout") == 0) {
	update_timeout_ = atof(argv[2]);
	return TCL_OK;
      }
      else if (strcasecmp (argv[1], "start-tag-motion") == 0) {
	mobility_period_ = atof(argv[2]);
	Scheduler::instance().schedule(tag_mobility_,tag_mobility_event_,Random::uniform(mobility_period_));
	return (TCL_OK);
      }
      else if (strcasecmp (argv[1], "attach-tag-dbase") == 0)
	{
	  TclObject *obj;
	if ((obj = TclObject::lookup (argv[2])) == 0)
	    {
	      fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		       argv[2]);
	      return TCL_ERROR;
	    }
	  tag_dbase_ = (tags_database *) obj;
	  return TCL_OK;
	}
      else if (strcasecmp (argv[1], "node") == 0)
	{
	  assert(node_ == NULL);
	  TclObject *obj;
	if ((obj = TclObject::lookup (argv[2])) == 0)
	    {
	      fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		       argv[2]);
	      return TCL_ERROR;
	    }
	  node_ = (MobileNode *) obj;
	  return TCL_OK;
	}
      else if (strcmp (argv[1], "query-debug") == 0)
	{
	  qry_debug_ = atoi(argv[2]);
	  return (TCL_OK);
	}
      else if (strcasecmp (argv[1], "ll-queue") == 0)
	{
	if (!(ll_queue = (PriQueue *) TclObject::lookup (argv[2])))
	    {
	      fprintf (stderr, "Landmark_Agent: ll-queue lookup of %s failed\n", argv[2]);
	      return TCL_ERROR;
	    }

	  return TCL_OK;
	}
    }

return (Agent::command (argc, argv));
}




void
LandmarkAgent::startUp()
{
  int i,ntags, j = 0, read_new_mobile_tags = 0;
  Scheduler &s = Scheduler::instance();
  compr_taglist *local_tags0, *local_tags1, *local_tags2, *t_ptr;
  compr_taglist *tag_ptr1, *tag_ptr2;
  God *gd = God::instance();
  //  AgentList *alist = AgentList::instance();
  int *nbrs;
  int num_nbrs = 0, num_nodes = 0;
  
  // Adding ourself to global tag agent database
  //  alist->AddAgent(myaddr_,this);

  //  num_nodes = gd->numNodes()-1;

  //  nbrs = new int[num_nodes];
  
  //  for(i = 1; i <= num_nodes; ++i) {
      // God sees node id as id+1 ...
  //    if(gd->hops(myaddr_+1,i) == 1) {
  //      nbrs[num_nbrs++] = i-1;
  //    }
  //  }


  //  trace("Node %d: Number of nbrs %d, Neighbours:",myaddr_,num_nbrs);
  //  num_nbrs_ = num_nbrs;
  //  nbrs_ = new int[num_nbrs_];
  //  for(i = 0; i < num_nbrs_; ++i) {
  //    nbrs_[i] = nbrs[i];
    //    trace("%d",nbrs_[i]);
  //  }
  //  if(nbrs) delete[] nbrs;
  
  trace("Node %d: LM Agent starting up at time %f",myaddr_,NOW);

  // Set node to be alive (this method might be called after a call to reset
  node_dead_ = 0;
  
  double x,y,z;
  node_->getLoc(&x,&y,&z);
  //  printf("Node %d position: (%f,%f,%f)\n",myaddr_,x,y,z);

  // Detection range smaller than transmission range. This is because, if 
  // the tags are passive, they may not have sufficient energy to re-radiate
  // information to the sensor
  double r = 60;

  local_tags0 = tag_dbase_->Gettags(x,y,r);
  //  trace("Node %d's at (%f,%f,%f) senses tags:",myaddr_,x,y,z);

  t_ptr = local_tags0;
  ntags = 0;
  while(t_ptr) {
    //    trace("tag name: %d.%d.%d",(t_ptr->obj_name_ >> 24) & 0xFF,(t_ptr->obj_name_ >> 16) & 0xFF,(t_ptr->obj_name_) & 0xFFFF);
    ++ntags;
    if(!(t_ptr->next_) && mobile_tags_ && !read_new_mobile_tags) {

      // Update our tag list with any new tags that have come into our range
      // while we were dead
      read_new_mobile_tags = 1;
      t_ptr->next_ = mobile_tags_;
      mobile_tags_ = NULL;
    }
    t_ptr = t_ptr->next_;
  }


  //  trace("Number of tags: %d",ntags);

  /*
  int agg_level = 1;
  int num_tags = 0;
  local_tags1 = aggregate_tags(local_tags0,agg_level,&num_tags);
  trace("Level 1 aggregates, num = %d",num_tags);
  t_ptr = local_tags1;
  while(t_ptr) {
    trace("tag name: %d.%d.%d",(t_ptr->obj_name_ >> 24) & 0xFF,(t_ptr->obj_name_ >> 16) & 0xFF,(t_ptr->obj_name_) & 0xFFFF);
    t_ptr = t_ptr->next_;
  }

  agg_level = 2;
  num_tags = 0;
  local_tags2 = aggregate_tags(local_tags1,agg_level,&num_tags);
  trace("Level 2 aggregates, num = %d",num_tags);
  t_ptr = local_tags2;
  while(t_ptr) {
    trace("tag name: %d.%d.%d",(t_ptr->obj_name_ >> 24) & 0xFF,(t_ptr->obj_name_ >> 16) & 0xFF,(t_ptr->obj_name_) & 0xFFFF);
    t_ptr = t_ptr->next_;
  }

  // Delete local_tags1
  tag_ptr1 = local_tags1;
  while(tag_ptr1) {
    tag_ptr2 = tag_ptr1;
    tag_ptr1 = tag_ptr1->next_;
    delete tag_ptr2;
  }

  // Delete local_tags2
  tag_ptr1 = local_tags2;
  while(tag_ptr1) {
    tag_ptr2 = tag_ptr1;
    tag_ptr1 = tag_ptr1->next_;
    delete tag_ptr2;
  }
  */

  assert(highest_level_ == 0);
  assert(parent_children_list_ == NULL);
  parent_children_list_ = new ParentChildrenList(highest_level_, this);
  ParentChildrenList **pcl = &parent_children_list_;

  // Start off periodic LM advertisements
  assert(highest_level_ == 0);
  s.schedule((*pcl)->periodic_handler_, (*pcl)->periodic_update_event_, INITIAL_WAIT_TIME + jitter(LM_STARTUP_JITTER, be_random_));
  (*pcl)->tag_list_ = local_tags0;
  (*pcl)->num_tags_ = ntags;

  // Start off promotion timer
  //  if (debug_) printf("startUp: Scheduling timer\n");
  promo_timer_running_ = 1;
  num_resched_ = 0;

  // Node enters "wait" state where it waits to receive other node's 
  // advertisements; Wait for 5 * radius(level) seconds; Should be
  // atleast the same as the period of LM advts (10s)
  total_wait_time_ = 0;
  wait_state_ = 1;
  double wait_time = WAIT_TIME * radius(highest_level_) + INITIAL_WAIT_TIME + LM_STARTUP_JITTER;
  total_wait_time_ += wait_time;
  //  trace("Node %d: Wait time at startUp: %f",myaddr_,wait_time);
  promo_timer_->sched(wait_time);

  //  promo_timer_->sched(promo_timeout_);
}




compr_taglist *
LandmarkAgent::aggregate_taginfo(compr_taglist *unagg_tags, int agg_level, int *num_tags)
{
  compr_taglist *agg_tags, *agg_ptr1, *agg_ptr2,  *last_agg_ptr;
  int found;
  
  *num_tags = 0;
  // agg_level is 1 implies ignore the last field
  // agg_level >= 2 implies ignore the last two fields
  agg_ptr1 = unagg_tags;
  agg_tags = NULL;
  
  while(agg_ptr1) {
    if(agg_level == 1) {
      found = FALSE;
      if(agg_tags) {
	agg_ptr2 = agg_tags;
	while(agg_ptr2) {
	  if((((agg_ptr2->obj_name_ >> 24) & 0xFF) == ((agg_ptr1->obj_name_ >> 24) & 0xFF)) && (((agg_ptr2->obj_name_ >> 16) & 0xFF) == ((agg_ptr1->obj_name_ >> 16) & 0xFF))) {
	    found = TRUE;
	    break;
	  }
	  last_agg_ptr = agg_ptr2;
	  agg_ptr2 = agg_ptr2->next_;
	}
      }

      if(!found) {
	++(*num_tags);
	if(!agg_tags) {
	  agg_tags = new compr_taglist;
	  last_agg_ptr = agg_tags;
	}
	else {
	  last_agg_ptr->next_ = new compr_taglist;
	  last_agg_ptr = last_agg_ptr->next_;
	}
	last_agg_ptr->obj_name_ = (agg_ptr1->obj_name_ & 0xFFFF0000);
      }
    }
    else if(agg_level >= 2) {
      found = FALSE;
      if(agg_tags) {
	agg_ptr2 = agg_tags;
	while(agg_ptr2) {
	  if(((agg_ptr2->obj_name_ >> 24) & 0xFF) == ((agg_ptr1->obj_name_ >> 24) & 0xFF)) {
	    found = TRUE;
	    break;
	  }
	  last_agg_ptr = agg_ptr2;
	  agg_ptr2 = agg_ptr2->next_;
	}
      }

      if(!found) {
	++(*num_tags);
	if(!agg_tags) {
	  agg_tags = new compr_taglist;
	  last_agg_ptr = agg_tags;
	}
	else {
	  last_agg_ptr->next_ = new compr_taglist;
	  last_agg_ptr = last_agg_ptr->next_;
	}
	last_agg_ptr->obj_name_ = (agg_ptr1->obj_name_ & 0xFF000000);
      }
    }
    agg_ptr1 = agg_ptr1->next_;	
  }
  return(agg_tags);
}





compr_taglist *
LandmarkAgent::aggregate_tags(compr_taglist *unagg_tags, int agg_level, int *num_tags)
{
  compr_taglist *agg_tags = NULL, *tag_ptr;
  aggreg_taglist *t1, *t2, *t3, *tmp_ptr;
  aggreg_taglist *list1 = NULL, *list2 = NULL, *list3 = NULL, *list = NULL;
  aggreg_taglist *prev_tag, *next_tag, *old_list;
  int found;
  int p1,p2,p3,q1,q2,q3,object_name;
  

  // Tag names have 3 fields
  // List 1 is list of tags with first field > 0, last 2 fields = 0
  // List 2 is list of tags with first two fields > 0 and last field = 0
  // List 3 is list of tags with all three fields > 0 

  tag_ptr = unagg_tags;
  while(tag_ptr) {
    p1 = (tag_ptr->obj_name_ >> 24) & 0xFF;
    p2 = (tag_ptr->obj_name_ >> 16) & 0xFF;
    p3 = tag_ptr->obj_name_ & 0xFFFF;
    found = 0;

    if(p1 && p2 && p3) {

      // Check if p1.p2.0 is already in list2; If so, goto next object
      object_name = (int)((p1 * pow(2,24)) + (p2 * pow(2,16))) ;
      old_list = list2;
      while(old_list) {
	if(old_list->obj_name_ == object_name) {
	  found = TRUE;
	  break;
	}
	old_list = old_list->next_;
      }

      // Check if p1.0.0 is already in list1; If so, goto next object
      old_list = list1;
      while(old_list) {
	q1 = (old_list->obj_name_ >> 24) & 0xFF;
	if(p1 == q1) {
	  found = TRUE;
	  break;
	}
	old_list = old_list->next_;
      }
      
      tmp_ptr = list3;
      while(tmp_ptr && !found) {
	q1 = (tmp_ptr->obj_name_ >> 24) & 0xFF;
	q2 = (tmp_ptr->obj_name_ >> 16) & 0xFF;
	q3 = tmp_ptr->obj_name_ & 0xFFFF;
	// If 2 objects have same value for first two fields, store the
	// aggregate p1.p2.0 in list2; We have already checked if p1.p2.0
	// is already in list2 or not
	if(p1 == q1 && p2 == q2 && p3 != q3) {
	  if(!list2) {
	    list2 = new aggreg_taglist;
	    t2 = list2;
	  }
	  else {
	    t2->next_ = new aggreg_taglist;
	    t2 = t2->next_;
	  }
	  t2->obj_name_ = object_name;

	  // Indicate that this is a new aggregate
	  t2->marked_ = 1;
	  // Remove this object from list3; We simply set the obj_name_ to 1
	  // to indicate that this tag object is not valid
	  tmp_ptr->obj_name_ = -1;
	  found = TRUE;
	  break;
	}
	else if(p1 == q1 && p2 == q2 && p3 == q3) {
	  found = TRUE;
	  break;
	}
	tmp_ptr = tmp_ptr->next_;
      }

      if(found) {
	tag_ptr = tag_ptr->next_;
	continue;
      }
      
      if(!list3) {
	list3 = new aggreg_taglist;
	t3 = list3;
      }
      else {
	t3->next_ = new aggreg_taglist;
	t3 = t3->next_;
      }
      t3->obj_name_ = tag_ptr->obj_name_;
    }
    else if(p1 && p2 && !p3) {

      // Check if p1.0.0 is already in list1; If so, goto next object
      object_name = (int)(p1 * pow(2,24)) ;
      if(list1) {
	old_list = list1;
	while(old_list) {
	  if(old_list->obj_name_ == object_name) {
	    found = TRUE;
	    break;
	  }
	  old_list = old_list->next_;
	}
      }

      tmp_ptr = list2;
      while(tmp_ptr && !found) {
	q1 = (tmp_ptr->obj_name_ >> 24) & 0xFF;
	q2 = (tmp_ptr->obj_name_ >> 16) & 0xFF;
	// If 2 objects have same value for the first field, store the
	// aggregate in list1 provided the other object is not a new aggregate
	if(p1 == q1 && p2 != q2 && !tmp_ptr->marked_) {
	  if(!list1) {
	    list1 = new aggreg_taglist;
	    t1 = list1;
	  }
	  else {
	    t1->next_ = new aggreg_taglist;
	    t1 = t1->next_;
	  }
	  t1->obj_name_ = object_name;
	  
	  // Indicate that this is a new aggregate
	  t1->marked_ = 1;
	  // Remove this object from list3; We simply set the obj_name_ to 1
	  // to indicate that this tag object is not valid
	  tmp_ptr->obj_name_ = -1;

	  // Remove any elements p1.*.* from list3 i.e., set obj_name_ to -1
	  old_list = list3;
	  while(old_list) {
	    q1 = (old_list->obj_name_ >> 24) & 0xFF;
	    if(p1 == q1)
	      old_list->obj_name_ = -1;
	    old_list = old_list->next_;
	  }

	  found = TRUE;
	  break;
	}
	else if(p1 == q1 && p2 == q2) {
	  found = TRUE;
	  break;
	}
	tmp_ptr = tmp_ptr->next_;
      }
      
      if(found) {
	tag_ptr = tag_ptr->next_;
	continue;
      }

      if(!list2) {
	list2 = new aggreg_taglist;
	t2 = list2;
      }
      else {
	t2->next_ = new aggreg_taglist;
	t2 = t2->next_;
      }
      t2->obj_name_ = tag_ptr->obj_name_;

      // Remove any elements p1.p2.* from list3 i.e., set obj_name_ to -1
      old_list = list3;
      while(old_list) {
	q1 = (old_list->obj_name_ >> 24) & 0xFF;
	q2 = (old_list->obj_name_ >> 16) & 0xFF;       
	if(p1 == q1 && p2 == q2)
	  old_list->obj_name_ = -1;
	old_list = old_list->next_;
      }
    }
    else if(p1 && !p2 && !p3) {
      // Check if object p1.0.0 already in list; If so, goto next object
      tmp_ptr = list1;
      while(tmp_ptr) {
	if(tmp_ptr->obj_name_ == tag_ptr->obj_name_) {
	  found = TRUE;
	  break;
	}
	tmp_ptr = tmp_ptr->next_;
      }
      
      // Add object to list1
      if(!found) {
	if(!list1) {
	  list1 = new aggreg_taglist;
	  t1 = list1;
	}
	else {
	  t1->next_ = new aggreg_taglist;
	  t1 = t1->next_;
	}
	t1->obj_name_ = tag_ptr->obj_name_;
      }

      // Remove any elements p1.*.* from list2 i.e., set obj_name_ to -1
      old_list = list2;
      while(old_list) {
	q1 = (old_list->obj_name_ >> 24) & 0xFF;
	if(p1 == q1)
	  old_list->obj_name_ = -1;
	old_list = old_list->next_;
      }

      // Remove any elements p1.*.* from list3 i.e., set obj_name_ to -1
      old_list = list3;
      while(old_list) {
	q1 = (old_list->obj_name_ >> 24) & 0xFF;
	if(p1 == q1)
	  old_list->obj_name_ = -1;
	old_list = old_list->next_;
      }
    }
    else
      assert(0);
    tag_ptr = tag_ptr->next_;
  }

  // Make list1, list2, list3 into one list
  list = NULL;
  if(list3) {
    list = list3;
    if(list2) {
      t3->next_ = list2;
      if(list1) {
	t2->next_ = list1;
      }
    }
    else if(list1)
      t3->next_ = list1;
  }
  else if(list2) {
    list = list2;
    if(list1)
      t2->next_ = list1;
  }
  else if(list1)
    list = list1;
  
  

  // Return the list of aggregated tags
  *num_tags = 0;
  agg_tags = NULL;
  tmp_ptr = list;
  while(tmp_ptr) {
    if(tmp_ptr->obj_name_ != -1) {
      if(!agg_tags) {
	agg_tags = new compr_taglist;
	tag_ptr = agg_tags;
      }
      else {
	tag_ptr->next_ = new compr_taglist;
	tag_ptr = tag_ptr->next_;
      }
      ++(*num_tags);
      tag_ptr->obj_name_ = tmp_ptr->obj_name_;
    }
    tmp_ptr = tmp_ptr->next_;
  }

  // Delete list
  list1 = NULL;
  list2 = NULL;
  list1 = list;
  while(list1) {
    list2 = list1;
    list1 = list1->next_;
    delete list2;
  }

  return(agg_tags);
}






void
LandmarkAgent::recv(Packet *p, Handler *)
{
  hdr_ip *iph = HDR_IP(p);
  hdr_cmn *cmh = HDR_CMN(p);

  /*
   *  Must be a packet being originated by the query agent at my node?
   */

  if(node_dead_) {
    Packet::free(p);
    return;
  }

  if(iph->saddr() == myaddr_ && iph->sport() == 0) {
    /*
     * Add the IP Header
     */
    cmh->size() += IP_HDR_LEN;    

  }
  /*
   *  I received a packet that I sent.  Probably
   *  a routing loop.
   */
  else if(iph->saddr() == myaddr_) {
    Packet::free(p);
    //   drop(p, DROP_RTR_ROUTE_LOOP);
    return;
  }
  /*
   *  Packet I'm forwarding...
   */

  // Move the ttl check to the following methods?
  //    if(--iph->ttl_ == 0) {
  //      drop(p, DROP_RTR_TTL);
  //      return;
  //    }
  
  // Packet will be forwarded down (if it's not dropped)
  cmh->direction_ = hdr_cmn::DOWN;
  
  unsigned char *walk = p->accessdata();
  int action = *walk++;

  int data_pkt = 0;
  if(action == QUERY_PKT || action == HASH_PKT || action == HASH_ACK_PKT || action == REHASH_PKT || action == DIR_QUERY_PKT || action == DIR_RESPONSE_PKT || action == OBJECT_QUERY_PKT || action == OBJECT_RESPONSE_PKT) 
    data_pkt = 1;

  if ((iph->saddr() != myaddr_) && (iph->dport() == ROUTER_PORT) && !data_pkt)
    {
      ProcessHierUpdate(p);
    }
  else
    {
      ForwardPacket(p);
    }
}

void
LandmarkAgent::ForwardPacket(Packet *p)
{
  hdr_ip *iph = HDR_IP(p);
  hdr_cmn *cmh = HDR_CMN(p);
  Packet *newp;
  hdr_ip *new_iph;
  hdr_cmn *new_cmh;
  unsigned char *walk, *X_ptr, *Y_ptr, *level_ptr, *num_src_hops_ptr;
  unsigned char *last_hop_ptr, *pkt_end_ptr;
  int X, Y, next_hop_level, prev_hop_level, obj_name, num_src_hops;
  double local_x, local_y, local_z;
  int num_dst = 0, action, origin_time;
  NodeIDList *dst_nodes = NULL, *dst_ptr = NULL;
  int query_for_us = FALSE;
  Scheduler &s = Scheduler::instance();
  double now = s.clock();
  nsaddr_t last_hop_id;
  int cache_index = -1; // index into cache if object is found
  int found = FALSE; // whether object has been found in cache

  walk = p->accessdata();

  // Type of advertisement
  action = *walk++;

  X = 0;
  X_ptr = walk;
  X = *walk++;
  X = (X << 8) | *walk++;

  Y_ptr = walk;
  Y = *walk++;
  Y = (Y << 8) | *walk++;

  // level of our parent/child node that forwarded the query to us
  level_ptr = walk;
  next_hop_level = *walk++;

  obj_name = *walk++;
  obj_name = (obj_name << 8) | *walk++;
  obj_name = (obj_name << 8) | *walk++;
  obj_name = (obj_name << 8) | *walk++;

    // origin time of advertisement
  origin_time = *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;
  
  num_src_hops_ptr = walk;
  num_src_hops = *walk++;
  num_src_hops = (num_src_hops << 8) | *walk++;

  assert(num_src_hops <= 30);

  last_hop_ptr = NULL;
  for(int i = 0; i < num_src_hops; ++i) {
    last_hop_ptr = walk;
    walk += 3;
  }

  if(last_hop_ptr) {
    prev_hop_level = *(last_hop_ptr+2);
    last_hop_id = *last_hop_ptr;
    last_hop_id = (last_hop_id << 8) | *(last_hop_ptr+1);
  }
  else {
    prev_hop_level = 0;
    last_hop_id = NO_NEXT_HOP;
  }

  // Used to add source route to packet
  pkt_end_ptr = walk;
  
  // If this is a response pkt, cache this information
  if(cache_) {
    if(X != 65000 && Y != 65000) {
      if(num_cached_items_ < MAX_CACHE_ITEMS) {
	
	int replace_index = num_cached_items_;
	// If object already exists in cache, update info if necessary
	for(int i = 0; i < num_cached_items_; ++i) {
	  if(tag_cache_[i].obj_name_ == obj_name && tag_cache_[i].origin_time_ < origin_time) {
	  replace_index = i;
	  break;
	  }
	}
	
	tag_cache_[replace_index].obj_name_ = obj_name;
	tag_cache_[replace_index].origin_time_ = origin_time;
	tag_cache_[replace_index].X_ = X;
	tag_cache_[replace_index].Y_ = Y;
	++num_cached_items_;
      }
      else {
	// Use LRU cache replacement
	int replace_index = 0;
	int least_time = tag_cache_[replace_index].origin_time_;
	for(int i = 0; i < MAX_CACHE_ITEMS; ++i) {
	  if(tag_cache_[i].origin_time_ < least_time)
	    replace_index = i;
      }
	tag_cache_[replace_index].obj_name_ = obj_name;
	tag_cache_[replace_index].origin_time_ = origin_time;
	tag_cache_[replace_index].X_ = X;
	tag_cache_[replace_index].Y_ = Y;
      }
    }
    else {
      // If this is a query pkt; check if we have the relevant information 
      // cached. TTL = 600 seconds for the cache entries
      found = FALSE;
      for(int i = 0; i < num_cached_items_; ++i) {
	if(tag_cache_[i].obj_name_ == obj_name && tag_cache_[i].origin_time_ > origin_time - 600) {
	  found = TRUE;
	  cache_index = i;
	  break;
	}
      }
    }
  }


  // Loop check i.e., if response to our query agent has looped back
  // Following not the correct condition to detect a loop!
  //  assert(!(iph->daddr() == myaddr_ && iph->dport() == 0));

  // Reduce the source route to just parent-children (O(#levels))
  // This is possible since parent and child in each others vicinity
  cmh->direction() = hdr_cmn::DOWN;
  if(iph->daddr() == myaddr_)
    query_for_us = TRUE;

  // Query pkt if X and Y are 65000
  if(X == 65000 && Y == 65000) {

    if(query_for_us || found) {
      if(qry_debug_)
	trace("Node %d: Rcved qry for us from node %d at time %f",myaddr_,last_hop_id,s.clock());
      if(!found)
	dst_nodes = search_tag(obj_name,prev_hop_level,next_hop_level,last_hop_id,&num_dst);

      if((num_dst == 0 && dst_nodes) || found) {
	delete dst_nodes;
	// if num_dst = 0 but dst_nodes is not NULL, we sense the
	// requested tag; add X,Y info and send response
	// if found is true, we have the cached information
	if(found) {
	  (*X_ptr++) = ((int)tag_cache_[cache_index].X_ >> 8) & 0xFF;
	  (*X_ptr) = ((int)tag_cache_[cache_index].X_) & 0xFF;
	  (*Y_ptr++) = ((int)tag_cache_[cache_index].Y_ >> 8) & 0xFF;
	  (*Y_ptr) = ((int)tag_cache_[cache_index].Y_) & 0xFF;
	}
	else {
	  node_->getLoc(&local_x, &local_y, &local_z);
	  (*X_ptr++) = ((int)local_x >> 8) & 0xFF;
	  (*X_ptr) = ((int)local_x) & 0xFF;
	  (*Y_ptr++) = ((int)local_y >> 8) & 0xFF;
	  (*Y_ptr) = ((int)local_y) & 0xFF;
	}

	// Send response
	iph->ttl_ = 1000;
	// Add 50 bytes to response 
	cmh->size() += 50;
	// Query from an agent at our node
	if(!num_src_hops) {
	  iph->daddr() = myaddr_;
	  iph->dport() = 0;
	  cmh->next_hop_ = myaddr_;
	}
	else {
	  --num_src_hops;
	  *num_src_hops_ptr = (num_src_hops >> 8) & 0xFF;
	  *(num_src_hops_ptr + 1) = num_src_hops & 0xFF;
	  // Decr pkt size
	  cmh->size() -= 4;

	  iph->daddr() = *last_hop_ptr++;
	  iph->daddr() = (iph->daddr() << 8) | *last_hop_ptr++;
	  if(!num_src_hops)
	    iph->dport() = 0;
	  else
	    iph->dport() = ROUTER_PORT;
	  
	  int relevant_level = *last_hop_ptr;
	  cmh->next_hop_ = get_next_hop(iph->daddr(),relevant_level);
	  //	  assert(cmh->next_hop_ != NO_NEXT_HOP);	  
	  if(cmh->next_hop_ == NO_NEXT_HOP) {
	    Packet::free(p);
	    trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	    return;
	  }
	  *level_ptr = *last_hop_ptr;	  
	}

	//	if(found) 
	  //	  trace("Node %d: Gen response from cache at time %f to node %d",myaddr_,s.clock(),iph->daddr());
	//	else
	  //	  trace("Node %d: Gen response at time %f to node %d",myaddr_,s.clock(),iph->daddr() >> 8);

	if(!num_src_hops && iph->daddr() == myaddr_) {
	  // TEMPORARY HACK! Cant forward from routing agent to some other
	  // agent on our node!
	  Packet::free(p);
	  trace("Node %d: Found object %d.%d.%d at (%d,%d) at time %f",myaddr_, (obj_name >> 24) & 0xFF, (obj_name >> 16) & 0xFF, obj_name & 0xFFFF,X,Y,s.clock());
	  return;
	}
	else if(iph->daddr() == myaddr_) {
	  ForwardPacket(p);
	}
	else {
	  s.schedule(target_,p,0);
	}
      }
      else if(num_dst >= 1) {

	if(--iph->ttl_ == 0) {
	  drop(p, DROP_RTR_TTL);
	  return;
	}
	
	// Add ourself to source route and increase the number of src hops
	//	if(last_hop_id != myaddr_) {
	++num_src_hops;
	*num_src_hops_ptr = (num_src_hops >> 8) & 0xFF;
	*(num_src_hops_ptr+1) = num_src_hops & 0xFF;
	*pkt_end_ptr++ = (myaddr_ >> 8) & 0xFF;
	*pkt_end_ptr++ = myaddr_ & 0xFF;
	// Indicate the level of the pcl object that a node should look-up
	// to find the relevant routing table entry
	*pkt_end_ptr = (next_hop_level+1) & 0xFF;
	// Incr pkt size
	cmh->size() += 4;

	dst_ptr = dst_nodes;
	// Replicate pkt to each destination
	iph->daddr() = dst_ptr->dst_node_;
	iph->dport() = ROUTER_PORT;
	cmh->next_hop_ = dst_ptr->dst_next_hop_;
	cmh->addr_type_ = NS_AF_INET;
	// Copy next hop variable to this variable temporarily
	// Copy it back into packet before sending the packet
	int tmp_next_hop_level = dst_ptr->next_hop_level_;

	if(qry_debug_)
	  trace("Node %d: Forwarding qry to node %d at time %f",myaddr_,dst_ptr->dst_node_,s.clock());

	dst_ptr = dst_ptr->next_;
	delete dst_nodes;
	dst_nodes = dst_ptr;

	for(int i = 1; i < num_dst; ++i) {
	  if(qry_debug_)
	    trace("Node %d: Forwarding qry to node %d at time %f",myaddr_,dst_ptr->dst_node_,s.clock());

	  // Change level and copy the packet
	  *level_ptr = dst_ptr->next_hop_level_;

	  newp = p->copy();

	  new_iph = HDR_IP(newp);
	  new_cmh = HDR_CMN(newp);

	  new_iph->daddr() = dst_ptr->dst_node_;
	  new_iph->dport() = ROUTER_PORT;
	  new_cmh->next_hop_ = dst_ptr->dst_next_hop_;
	  new_cmh->addr_type_ = NS_AF_INET;

	  if(new_iph->daddr() == myaddr_)
	    ForwardPacket(newp);
	  else
	    s.schedule(target_,newp,0);

	  dst_ptr = dst_ptr->next_;
	  delete dst_nodes;
	  dst_nodes = dst_ptr;
	}
	
	*level_ptr = tmp_next_hop_level;

	if(iph->daddr() == myaddr_) {
	  ForwardPacket(p);
	}
	else
	  s.schedule(target_,p,0);
      }
      else if(num_dst == 0) {
	// Free packet if we dont have any dst to forward packet
	if(qry_debug_)
	  trace("Node %d: Dropping query from %d at time %f,num_src_hops %d",myaddr_,iph->saddr(),s.clock(),num_src_hops);
	
	Packet::free(p);
	return;
      }
    }
    else {
      // simply forward to next hop
      if(qry_debug_)
	trace("Node %d: Forwarding query to node %d at time %f,num_src_hops %d",myaddr_,iph->daddr(),s.clock(),num_src_hops);

      if(--iph->ttl_ == 0) {
	drop(p, DROP_RTR_TTL);
        return;
      }

      cmh->next_hop_ = get_next_hop(iph->daddr(),next_hop_level+1);
      //      assert(cmh->next_hop_ != NO_NEXT_HOP);	  
      if(cmh->next_hop_ == NO_NEXT_HOP) {
	Packet::free(p);
	trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	return;
      }
      s.schedule(target_,p,0);
    }
  }
  else {
    // Forward the response packet
    if(qry_debug_)
      trace("Node %d: Forwarding response to node %d at time %f,num_src_hops %d",myaddr_,iph->daddr(),s.clock(),num_src_hops);
    if(--iph->ttl_ == 0) {
      drop(p, DROP_RTR_TTL);
      return;
    }

    // Check if query from an agent at our node
    if(query_for_us) {
      if(!num_src_hops) {
	iph->daddr() = myaddr_;
	iph->dport() = 0;
	cmh->next_hop_ = myaddr_;
      }
      else {
	--num_src_hops;
	*num_src_hops_ptr = (num_src_hops >> 8) & 0xFF;
	*(num_src_hops_ptr + 1) = num_src_hops & 0xFF;
	// Decr pkt size
	cmh->size() -= 4;
	
	iph->daddr() = *last_hop_ptr++;
	iph->daddr() = (iph->daddr() << 8) | *last_hop_ptr++;

	if(!num_src_hops)
	  iph->dport() = 0;
	else
	  iph->dport() = ROUTER_PORT;

	int relevant_level = *last_hop_ptr;
	cmh->next_hop_ = get_next_hop(iph->daddr(),relevant_level);
	//	assert(cmh->next_hop_ != NO_NEXT_HOP);	  
	if(cmh->next_hop_ == NO_NEXT_HOP) {
	  Packet::free(p);
	  trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	  return;
	}
	
	*level_ptr = *last_hop_ptr;	  
      }
      if(!num_src_hops && iph->daddr() == myaddr_) {
	// TEMPORARY HACK! Cant forward from routing agent to some other
	// agent on our node!
	Packet::free(p);
	trace("Node %d: Found object %d.%d.%d at (%d,%d) at time %f",myaddr_, (obj_name >> 24) & 0xFF, (obj_name >> 16) & 0xFF, obj_name & 0xFFFF,X,Y,s.clock());
	return;
      }
      else if(iph->daddr() == myaddr_)
	ForwardPacket(p);
      else
	s.schedule(target_,p,0);      
    }
    else {
      cmh->next_hop_ = get_next_hop(iph->daddr(),next_hop_level);
      //      assert(cmh->next_hop_ != NO_NEXT_HOP);	  
      if(cmh->next_hop_ == NO_NEXT_HOP) {
	Packet::free(p);
	trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	return;
      }
      s.schedule(target_,p,0);
    }
  }
}




NodeIDList *
LandmarkAgent::search_tag(int obj_name, int prev_hop_level, int next_hop_level,nsaddr_t last_hop_id, int *num_dst)
{
  ParentChildrenList *pcl = parent_children_list_;
  LMNode *child;
  compr_taglist *tag_ptr;
  int forward = FALSE;
  NodeIDList *nlist = NULL, *nlist_ptr = NULL;
  int p1, p2, p3, q1, q2, q3;
  int match = 0, exact_match = 0;

  *num_dst = 0;

  // Check if our node senses the requested tag
  while(pcl) {
    if(pcl->level_ == 0)
      break;
    pcl = pcl->next_;
  }

  if(!pcl)
    return(NULL);

  // if our node senses the tag, add the node to nlist but do not increase
  // num_dst
  tag_ptr = pcl->tag_list_;
  while(tag_ptr) {
    if(tag_ptr->obj_name_ == obj_name) {
      nlist = new NodeIDList;
      nlist->dst_node_ = myaddr_;
      nlist->dst_next_hop_ = myaddr_;
      return(nlist);
    }
    tag_ptr = tag_ptr->next_;
  }

  // If next_hop_level = 2, lookup would be done in the level 2 object
  // that would have level 1 tag aggregates
  //  if(next_hop_level == 2)
  //    obj_name = obj_name & 0xFFFF0000;
  //  else if(next_hop_level >= 3)
  //    obj_name = obj_name & 0xFF000000;
  p1 = (obj_name >> 24) & 0xFF;
  p2 = (obj_name >> 16) & 0xFF;
  p3 = obj_name & 0xFFFF;

  pcl = parent_children_list_;
  while(pcl) {
    if(pcl->level_ == next_hop_level)
      break;
    pcl = pcl->next_;
  }

  if(!pcl)
    return(NULL);

  //  assert(pcl);
  child = pcl->pchildren_;
  while(child) {
    // Dont forward back to child if child forwarded this query to us
    // We should forward to all children though if the message is going
    // down the hierarchy
    forward = FALSE;
    if(next_hop_level < prev_hop_level || (child->id_ != last_hop_id && next_hop_level >= prev_hop_level))
      forward = TRUE;
    if(child->child_flag_ == IS_CHILD && forward) {
      tag_ptr = child->tag_list_;
      match = 0;
      exact_match = 0;
      while(tag_ptr) {
	q1 = (tag_ptr->obj_name_ >> 24) & 0xFF;
	q2 = (tag_ptr->obj_name_ >> 16) & 0xFF;
	q3 = tag_ptr->obj_name_ & 0xFFFF;
	if(p1 == q1 && p2 == q2 && p3 == q3)
	  exact_match = 1;
	else if((p1 == q1 && p2 == q2 && !q3) || (p1 == q1 && !q2 && !q3))
	  match = 1;
	if(match) {
	  if(!nlist) {
	    nlist = new NodeIDList;
	    nlist_ptr = nlist;
	  }
	  else {
	    nlist_ptr->next_ = new NodeIDList;
	    nlist_ptr = nlist_ptr->next_;
	  }
	  nlist_ptr->dst_node_ = child->id_;
	  nlist_ptr->dst_next_hop_ = child->next_hop_;
	  nlist_ptr->next_hop_level_= next_hop_level - 1;
	  ++(*num_dst);
	  break;
	}
	else if(exact_match) {
	  // Delete all old elements
	  NodeIDList *n1, *n2;
	  n1 = nlist;
	  while(n1) {
	    n2 = n1;
	    n1 = n1->next_;
	    delete n2;
	  }
	  // Return just single element i.e., the ID of the child with an
	  // exact match for the object name
	  nlist = new NodeIDList;
	  nlist->dst_node_ = child->id_;
	  nlist->dst_next_hop_ = child->next_hop_;
	  nlist->next_hop_level_= next_hop_level - 1;
	  (*num_dst) = 1;
	  return(nlist);
	}
	tag_ptr = tag_ptr->next_;
      }
    }
    child = child->next_;
  }

  // Add parent if query is travelling up the hierarchy
  if(next_hop_level >= prev_hop_level && pcl->parent_) {
    if(!nlist) {
      nlist = new NodeIDList;
      nlist_ptr = nlist;
    }
    else {
      nlist_ptr->next_ = new NodeIDList;
      nlist_ptr = nlist_ptr->next_;
    }
    nlist_ptr->dst_node_ = (pcl->parent_)->id_;
    nlist_ptr->dst_next_hop_ = (pcl->parent_)->next_hop_;
    nlist_ptr->next_hop_level_= next_hop_level + 1;
    ++(*num_dst);
  }

  return(nlist);
}



nsaddr_t
LandmarkAgent::get_next_hop(nsaddr_t dst, int next_hop_level)
{
  ParentChildrenList *pcl = parent_children_list_;
  LMNode *pchild;

  while(pcl->level_ != next_hop_level) {
    pcl = pcl->next_;
  }
  
  assert(pcl);
  pchild = pcl->pchildren_;
  while(pchild) {
    if(pchild->id_ == dst)
      return(pchild->next_hop_);
    pchild = pchild->next_;
  }

  return(NO_NEXT_HOP);
}


void
LandmarkAgent::get_nbrinfo()
{
  ParentChildrenList *pcl;
  LMNode *pchild;
  int num_nbrs = 0;

  pcl = parent_children_list_;
  
  if(!pcl) {
    trace("Node %d: Neighbour info not available; perhaps the node is down");
    return;
  }

  while(pcl) {
    if(pcl->level_ == 1)
      break;
    pcl = pcl->next_;
  }

  //  assert(pcl);

  if(!pcl) {
    trace("Node %d: Neighbour info not available; perhaps the node is down");
    return;
  }

  pchild = pcl->pchildren_;
  //  assert(pchild);

  while(pchild) {
    if(pchild->num_hops_ == 1)
      ++num_nbrs;
    pchild = pchild->next_;
  }
  
  trace("Node %d: Number of neighbours: %d",myaddr_,num_nbrs);
}




void
LandmarkAgent::MoveTags() {
  ParentChildrenList *pcl = parent_children_list_;
  compr_taglist *tag = NULL, *prev_tag, *next_tag;
  int removed_tag = 0, our_tags_changed = 0;

  trace("Node %d: Moving tags at time %f", myaddr_,NOW);
  
  if(!pcl && !mobile_tags_) {
    Scheduler::instance().schedule(tag_mobility_,tag_mobility_event_,tag_rng_->uniform(mobility_period_));
    return;
  }
  
  if(pcl) {
    // Get level 0 pcl object
    while(pcl) {
      if(pcl->level_ == 0)
	break;
      pcl = pcl->next_;
    }
    assert(pcl);
  }    
  
  // Pick tag(s) at random and move them to one of the neighbours
  // Only tags with the last field > 5 are mobile.
  if(pcl)
    tag = pcl->tag_list_;
  else
    tag = mobile_tags_;
  
  prev_tag = tag;
  while(tag) {
    removed_tag = 0;
    // Tags with last field < 30 are not mobile
    if((tag->obj_name_ & 0xFFFF) > 30) {
      // Move tag to neighbouring node with probability of 0.3
      int n = tag_rng_->uniform(10);
      if(n <= 2) {
	assert(nbrs_);
	int nbr_index = tag_rng_->uniform(num_nbrs_);
	LandmarkAgent *nbr_agent = (LandmarkAgent *)AgentList::instance()->GetAgent(nbrs_[nbr_index]);
	assert(nbr_agent);

	trace("Node %d: Moving tag %d.%d.%d at time %f to nbr %d",myaddr_,(tag->obj_name_ >> 24) & 0xFF,(tag->obj_name_ >> 16) & 0xFF,tag->obj_name_ & 0xFFFF,NOW,nbrs_[nbr_index]);

	// Remove tag from our list
	removed_tag = 1;
	our_tags_changed = 1;
	if(prev_tag == tag) {
	  if(pcl)
	    pcl->tag_list_ = tag->next_;
	  else if(mobile_tags_)
	    mobile_tags_ = tag->next_;
	  prev_tag = tag->next_;
	}
	else
	  prev_tag->next_ = tag->next_;
	
	next_tag = tag->next_;

	if(pcl)
	  --(pcl->num_tags_);
	
	// Add this tag to neighbouring node
	nbr_agent->AddMobileTag(tag);
      }
    }
    if(!removed_tag) {
      prev_tag = tag;
      tag = tag->next_;
    }
    else {
      tag = next_tag;
    }
  }

  // Trigger hierarchy advertisement if our taglist has changed
  if(our_tags_changed)
    SendChangedTagListUpdate(our_tags_changed,0);
  
  Scheduler::instance().schedule(tag_mobility_,tag_mobility_event_,tag_rng_->uniform(mobility_period_));
}





void
LandmarkAgent::AddMobileTag(void *mobile_tag)
{
  ParentChildrenList *pcl = parent_children_list_;
  compr_taglist *tag = NULL, *new_tag = (compr_taglist *)mobile_tag;

  // Make sure that this tag object is not pointing to next member on
  // the previous list that this tag was part of
  new_tag->next_ = NULL;

  if(pcl) {
    // Get level 0 pcl object
    while(pcl) {
      if(pcl->level_ == 0)
	break;
      pcl = pcl->next_;
    }
    assert(pcl);

    ++(pcl->num_tags_);
    
    if(!pcl->tag_list_) {
      pcl->tag_list_ = new_tag;
    }
    else {
      tag = pcl->tag_list_;
      while(tag->next_) {
	tag = tag->next_;
      }
      tag->next_ = new_tag;
    }
  }
  else {
    if(!mobile_tags_) {
      mobile_tags_ = new_tag;
    }
    else {
      tag = mobile_tags_;
      while(tag->next_) {
	tag = tag->next_;
      }
      tag->next_ = new_tag;
    }
  }

  // Trigger hierarchy advertisements after a mean of 5 seconds if
  // the advt event has not already been scheduled
  if(tag_advt_event_->uid_ < 0)
    Scheduler::instance().schedule(tag_advt_handler_, tag_advt_event_, Random::uniform(10));
}





// new tag info received for specified level; Send updates if necessary
// i.e., if aggregates have changed etc.
void
LandmarkAgent::SendChangedTagListUpdate(int our_tag_changed, int level)
{
  ParentChildrenList *pcl = parent_children_list_;
  ParentChildrenList *child_pcl, *parent_pcl;
  compr_taglist *tag_ptr1 = NULL, *tag_ptr2 = NULL, *tag_list = NULL;
  compr_taglist *agg_tags = NULL;
  LMNode *lmnode;
  int upd_time, num_tags = 0;
  Scheduler &s = Scheduler::instance();
  double now = s.clock();

  if(node_dead_ || !pcl || level >= highest_level_)
    return;

  if(myaddr_ == 45)
    trace("Node %d: SendChangedTagListUpdate, level %d at time %f",myaddr_,level,now);

  while(pcl) {
    if(pcl->level_ == level)
      child_pcl = pcl;
    else if(pcl->level_ == level + 1) 
      parent_pcl = pcl;
    pcl = pcl->next_;
  }
  
  if(our_tag_changed) {
    assert(level == 0);
    upd_time = (int) now;
    upd_time = (upd_time << 16) | (parent_pcl->seqnum_ & 0xFFFF);
    ++(parent_pcl->seqnum_);
    parent_pcl->UpdatePotlChild(myaddr_, myaddr_,0,0,0,(int) node_->energy(),upd_time,IS_CHILD,FALSE,child_pcl->tag_list_);

    // Send out hierarchy advertisement since the tag list has changed
    Packet *newp = makeUpdate(child_pcl,HIER_ADVS,PERIODIC_ADVERTS);
    child_pcl->last_update_sent_ = now;
    s.schedule(target_,newp,0);
  }
  
  while(level < highest_level_) {

    if(myaddr_ == 45)
      trace("Node %d: Updating tag lists, level %d",myaddr_,level);
    
    lmnode = parent_pcl->pchildren_;
    tag_list = NULL;

    // Loop through all the children and add tags to tag_list
    while(lmnode) {
      if(lmnode->child_flag_ == IS_CHILD) {
	tag_ptr1 = lmnode->tag_list_;
    
	while(tag_ptr1) {
	  if(!tag_list) {
	    tag_list = new compr_taglist;
	    tag_ptr2 = tag_list;
	  }
	  else {
	    tag_ptr2->next_ = new compr_taglist;
	    tag_ptr2 = tag_ptr2->next_;
	  }
	  //        trace("tag name: %d.%d.%d",(tag_ptr1->obj_name_ >> 24) & 0xFF,(tag_ptr1->obj_name_ >> 16) & 0xFF,(tag_ptr1->obj_name_) & 0xFFFF);
	  tag_ptr2->obj_name_ = tag_ptr1->obj_name_;
	  tag_ptr1 = tag_ptr1->next_;
	}
      }
      lmnode = lmnode->next_;
    }
    
    // Aggregate tag_list
    agg_tags = aggregate_taginfo(tag_list,parent_pcl->level_,&num_tags);
    
    // Delete tag_list
    tag_ptr1 = tag_list;
    while(tag_ptr1) {
      tag_ptr2 = tag_ptr1;
      tag_ptr1 = tag_ptr1->next_;
      delete tag_ptr2;
    }
    
    if(!compare_tag_lists(parent_pcl->tag_list_,parent_pcl->num_tags_,agg_tags,num_tags)) {
      // Delete parent_pcl's tag_list and update with new tag_list
      tag_ptr1 = parent_pcl->tag_list_;
      while(tag_ptr1) {
	tag_ptr2 = tag_ptr1;
	tag_ptr1 = tag_ptr1->next_;
	delete tag_ptr2;
      }

      parent_pcl->tag_list_ = agg_tags;
      parent_pcl->num_tags_ = num_tags;

      // Send out hierarchy advertisement since the tag list has changed
      Packet *newp = makeUpdate(parent_pcl,HIER_ADVS,PERIODIC_ADVERTS);
      parent_pcl->last_update_sent_ = now;
      s.schedule(target_,newp,0);
   
      ++level;
      // Update our tag list in the higher level pcl object
      pcl = parent_children_list_;
      parent_pcl = NULL;
      child_pcl = NULL;
      while(pcl) {
	if(pcl->level_ == level)
	  child_pcl = pcl;
	else if(pcl->level_ == level + 1) 
	  parent_pcl = pcl;
	pcl = pcl->next_;
      }
      upd_time = (int) now;
      upd_time = (upd_time << 16) | (parent_pcl->seqnum_ & 0xFFFF);
      ++(parent_pcl->seqnum_);
      parent_pcl->UpdatePotlChild(myaddr_, myaddr_,0,0,0,(int) node_->energy(),upd_time,IS_CHILD,FALSE,child_pcl->tag_list_);
    }
    else {

      // Delete agg_tags
      tag_ptr1 = agg_tags;
      while(tag_ptr1) {
	tag_ptr2 = tag_ptr1;
	tag_ptr1 = tag_ptr1->next_;
	delete tag_ptr2;
      }
      break;
    }
  }
}




int
LandmarkAgent::compare_tag_lists(compr_taglist *tag_list1, int num_tags1, compr_taglist *tag_list2, int num_tags2)
{
  compr_taglist *tag1 = tag_list1, *tag2 = tag_list2;
  int found;

  // if num_tags == -1, it means that the number of tags was not computed
  if(num_tags1 == -1) {
    num_tags1 = 0;
    while(tag1) {
      ++num_tags1;
      tag1 = tag1->next_;
    }
    tag1 = tag_list1;
  }

  if(num_tags2 == -1) {
    num_tags2 = 0;
    while(tag2) {
      ++num_tags2;
      tag2 = tag2->next_;
    }
    tag2 = tag_list2;
  }

  if(num_tags1 != num_tags2)
    return(FALSE);

  while(tag1) {
    found = 0;
    while(tag2) {
      if(tag1->obj_name_ == tag2->obj_name_) {
	found = 1;
	break;
      }
      tag2 = tag2->next_;
    }
    if(!found)
      return(FALSE);
    tag1 = tag1->next_;
  }

  return(TRUE);
}

