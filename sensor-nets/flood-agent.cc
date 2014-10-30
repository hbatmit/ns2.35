/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997,2000 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/sensor-nets/flood-agent.cc,v 1.4 2000/09/01 03:04:11 haoboy Exp $
 */

// Author: Satish Kumar, kkumar@isi.edu

#include <stdarg.h>
#include <float.h>

#include "flood-agent.h"
#include <random.h>
#include <cmu-trace.h>
#include <address.h>

#define OLD_QRY_ENTRY 0
#define OLD_SHORTER_ENTRY 1
#define NEW_QRY_ENTRY 2

#define MAX_CACHE_ITEMS 200

void FloodAgent::
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



static class FloodAgentClass:public TclClass
{
  public:
  FloodAgentClass ():TclClass ("Agent/flood")
  {
  }
  TclObject *create (int, const char *const *)
  {
    return (new FloodAgent ());
  }
} class_flood_agent;



FloodAgent::FloodAgent() : Agent(PT_MESSAGE)
{
  node_ = NULL;
  tag_list_ = NULL;
  query_list_ = NULL;

  cache_ = 0; // Disable caching by default
  tag_cache_ = new TagCache[MAX_CACHE_ITEMS];
  num_cached_items_ = 0;
}




int 
FloodAgent::command (int argc, const char *const *argv)
{
  if (argc == 2)
    {
      if (strcmp (argv[1], "start-floodagent") == 0)
        {
          startUp();
          return (TCL_OK);
        }
      if (strcmp (argv[1], "enable-caching") == 0)
        {
          cache_ = 1;
          return (TCL_OK);
        }
      else if (strcasecmp (argv[1], "ll-queue") == 0)
        {
	  if (!(ll_queue = (PriQueue *) TclObject::lookup (argv[2])))
            {
              fprintf (stderr, "Flood_Agent: ll-queue lookup of %s failed\n", argv[2]);
              return TCL_ERROR;
            }
	  return TCL_OK;
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
    }
  
  return (Agent::command (argc, argv));
}





void
FloodAgent::startUp()
{
  compr_taglist *local_tags0, *t_ptr;
  int ntags = 0;
  double x,y,z;

  node_->getLoc(&x,&y,&z);
  //  printf("Node %d position: (%f,%f,%f)\n",myaddr_,x,y,z);

  // Detection range smaller than transmission range. This is because, if 
  // the tags are passive, they may not have sufficient energy to re-radiate
  // information to the sensor
  double r = 60;

  local_tags0 = tag_dbase_->Gettags(x,y,r);
  trace("Node %d's at (%f,%f,%f) senses tags:",myaddr_,x,y,z);
  t_ptr = local_tags0;
  ntags = 0;
  while(t_ptr) {
    trace("tag name: %d.%d.%d",(t_ptr->obj_name_ >> 24) & 0xFF,(t_ptr->obj_name_ >> 16) & 0xFF,(t_ptr->obj_name_) & 0xFFFF);
    ++ntags;
    t_ptr = t_ptr->next_;
  }
  trace("Number of tags: %d",ntags);

  tag_list_ = local_tags0;

}

void
FloodAgent::recv(Packet *p, Handler *)
{
  hdr_ip *iph = HDR_IP(p);
  hdr_cmn *cmh = HDR_CMN(p);
  unsigned char *walk, *X_ptr, *Y_ptr;
  compr_taglist *tag_ptr;
  int found = FALSE, action, X, Y, obj_name, origin_time, next_hop_level;
  int cached = FALSE, cache_index = -1;
  int num_src_hops;
  double local_x, local_y, local_z;
  nsaddr_t last_hop_id;
  Scheduler &s = Scheduler::instance();
  double now = s.clock();

  walk = p->accessdata();

  // Type of advertisement
  action = *walk++;

  X_ptr = walk;
  X = *walk++;
  X = (X << 8) | *walk++;

  Y_ptr = walk;
  Y = *walk++;
  Y = (Y << 8) | *walk++;

  // Used in LM
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

  num_src_hops = *walk++;
  num_src_hops = (num_src_hops << 8) | *walk++;

  // Query from an agent at our node
  if(iph->saddr() == myaddr_ && iph->sport() == 0 && action == QUERY_PKT) {
    // Increase the number of source hops to 1
    // Add IP header length
    cmh->size_ += 20;
  }
  else {
    ++num_src_hops;
    walk = walk - 2;
    (*walk++) = (num_src_hops >> 8) & 0xFF;
    (*walk++) = (num_src_hops) & 0XFF;
  }
    
  if(num_src_hops) {
    last_hop_id = *walk++;
    last_hop_id = (last_hop_id << 8) | *walk++;
  }
  else {
    last_hop_id = myaddr_;
    walk = walk + 2;
  }

  if(action == QUERY_PKT) {
    walk = walk - 2;
    *walk++ = (myaddr_ >> 8) & 0xFF;
    *walk++ = myaddr_ & 0xFF;
  }

  // Packet will be send down the stack
  cmh->direction() = hdr_cmn::DOWN;


  // Query pkt if X and Y are 65000
  if(X == 65000 && Y == 65000) {

    // Method returns 1 if query seen before. Otherwise returns 0
    // and adds query info to the list
    int query_type = search_queries_list(iph->saddr(),obj_name,origin_time,num_src_hops,last_hop_id);
    if(query_type == OLD_QRY_ENTRY) {
      Packet::free(p);
      return;
    }

    // Check if info is in cache if caching is enabled
    if(cache_) {
            // If this is a query pkt; check if we have the relevant information 
      // cached. TTL = 600 seconds for the cache entries
      cached = FALSE;
      for(int i = 0; i < num_cached_items_; ++i) {
        if(tag_cache_[i].obj_name_ == obj_name && tag_cache_[i].origin_time_ > origin_time - 600) {
          cached = TRUE;
          cache_index = i;
          break;
        }
      }
    }


    // check if our node has the requested information
    tag_ptr = tag_list_;
    while(tag_ptr) {
      if(tag_ptr->obj_name_ == obj_name) {
	found = TRUE;
	break;
      }
      tag_ptr = tag_ptr->next_;
    }
    
    if(query_type == OLD_SHORTER_ENTRY && found) {
      trace("Node %d: Query received along shorter path",myaddr_);
      Packet::free(p);
      return;
    }
    
    if(found || cached) {
      // generate response
      //      trace("Node %d: Generating response",myaddr_);
      if(cached) {
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

      iph->ttl_ = 1000;
      iph->daddr() = iph->saddr();
      iph->dport() = QUERY_PORT;
      cmh->next_hop() = last_hop_id;
      cmh->addr_type_ = NS_AF_INET;
      // Add 50 bytes to response 
      cmh->size() += 50;

      if(last_hop_id == myaddr_) {
	// TEMPORARY HACK! Cant forward from routing agent to some other
	// agent on our node!
	Packet::free(p);
	trace("FloodAgent Found object %d.%d.%d at (%d,%d) at time %f", (obj_name >> 24) & 0xFF, (obj_name >> 16) & 0xFF, obj_name & 0xFFFF,X,Y,now);
	return;
      }

      s.schedule(target_,p,0);
    }
    else {
      // flood pkt 
      //      trace("Node %d: Flooding packet; query type %d",myaddr_,query_type);
      if(--iph->ttl_ == 0) {
	drop(p, DROP_RTR_TTL);
	return;
      }

      cmh->next_hop_ = IP_BROADCAST; // need to broadcast packet
      cmh->addr_type_ = NS_AF_INET;
      iph->daddr() = IP_BROADCAST;  // packet needs to be broadcast
      iph->dport() = ROUTER_PORT;

      s.schedule(target_,p,0);
    }
  }
  else {
    // Forward response
    //    trace("Node %d: Forwarding response",myaddr_);
    if(--iph->ttl_ == 0) {
      drop(p, DROP_RTR_TTL);
      return;
    }
    
    if(cache_) {
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
    
    cmh->next_hop_ = get_next_hop(iph->saddr(),obj_name,origin_time);
    assert(cmh->next_hop_ != NO_NEXT_HOP);
    s.schedule(target_,p,0);
  }
}




int
FloodAgent::search_queries_list(nsaddr_t src, int obj_name, int origin_time, int num_hops, nsaddr_t last_hop_id)
{
  QueryList *ql, *newql = NULL, *replql = NULL;

  ql = query_list_;

  while(ql) {
    if(ql->src_ == src && ql->obj_name_ == obj_name && ql->origin_time_ == origin_time) {
      if(ql->num_hops_ > num_hops) {
	ql->num_hops_ = num_hops;
	ql->last_hop_id_ = last_hop_id;
      return(OLD_SHORTER_ENTRY);
      }
      return(OLD_QRY_ENTRY);
    }
    // Replace very old entries
    if(ql->origin_time_ + 100 < origin_time && !replql)
      replql = ql;
    else if(!replql)
      newql = ql;
    ql = ql->next_;
  }    

  if(!query_list_) {
    query_list_ = new QueryList;
    query_list_->src_ = src;
    query_list_->obj_name_ = obj_name;
    query_list_->origin_time_ = origin_time;
    query_list_->num_hops_ = num_hops;
    query_list_->last_hop_id_ = last_hop_id;
    return(NEW_QRY_ENTRY);
  }

  if(replql) {
    replql->src_ = src;
    replql->obj_name_ = obj_name;
    replql->origin_time_ = origin_time;
    replql->num_hops_ = num_hops;
    replql->last_hop_id_ = last_hop_id;
  }
  else {
    newql->next_ = new QueryList;
    newql = newql->next_;
    newql->src_ = src;
    newql->obj_name_ = obj_name;
    newql->origin_time_ = origin_time;
    newql->num_hops_ = num_hops;
    newql->last_hop_id_ = last_hop_id;
  }
  return(NEW_QRY_ENTRY);
}
  


nsaddr_t
FloodAgent::get_next_hop(nsaddr_t src, int obj_name, int origin_time)
{
 QueryList *ql;

 ql = query_list_;

 while(ql) {
   if(ql->src_ == src && ql->obj_name_ == obj_name && ql->origin_time_ == origin_time) {
     return(ql->last_hop_id_);
   }
   ql = ql->next_;
 }
 
 return(NO_NEXT_HOP);
}

 
