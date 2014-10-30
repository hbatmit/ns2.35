//
// dr.hh           : Diffusion Routing Class Definitions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: dr.hh,v 1.17 2005/09/13 04:53:49 tomh Exp $
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

// This file defines Diffusion Routing's Publish/Subscribe, Filter and
// Timer APIs. It is included from diffapp.hh (and therefore
// applications and filters should include diffapp.hh instead). For a
// detailed description of the API, please look at the Diffusion
// Routing API document (available at the SCADDS website:
// http://www.isi.edu/scadds).

#ifndef _DR_HH_
#define _DR_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <pthread.h>
#include <string.h>
#include <math.h>
#include <map>

#include "main/timers.hh"
#include "main/filter.hh"
#include "main/config.hh"
#include "main/iodev.hh"
#include "main/tools.hh"

#ifdef NS_DIFFUSION
#include "diffagent.h"
#endif // NS_DIFFUSION

#ifdef UDP
#include "drivers/UDPlocal.hh"
#endif // UDP

#define WAIT_FOREVER       -1
#define POLLING_INTERVAL   10 // seconds
#define SMALL_TIMEOUT      10 // milliseconds

typedef long handle;

class HandleEntry;
class CallbackEntry;
class DiffusionRouting;

typedef list<HandleEntry *> HandleList;
typedef list<CallbackEntry *> CallbackList;

class TimerCallbacks {
public:
  virtual ~TimerCallbacks () {}
  virtual int expire(handle hdl, void *p) = 0;
  virtual void del(void *p) = 0;
};

class InterestCallback : public TimerCallback {
public:
  InterestCallback(DiffusionRouting *drt, HandleEntry *handle_entry) :
    drt_(drt), handle_entry_(handle_entry) {};
  ~InterestCallback() {};
  int expire();

  DiffusionRouting *drt_;
  HandleEntry *handle_entry_;
};

class FilterKeepaliveCallback : public TimerCallback {
public:
  FilterKeepaliveCallback(DiffusionRouting *drt, FilterEntry *filter_entry) :
    drt_(drt), filter_entry_(filter_entry) {};
  ~FilterKeepaliveCallback() {};
  int expire();

  DiffusionRouting *drt_;
  FilterEntry *filter_entry_;
};

class OldAPITimer : public TimerCallback {
public:
  OldAPITimer(TimerCallbacks *cb, void *p) :
    cb_(cb), p_(p) {};
  ~OldAPITimer() {};
  int expire();

  TimerCallbacks *cb_;
  void *p_;
};

// Rmst specific definitions
typedef map<int, void*, less<int> > Int2Frag;
class RecRmst {
public:
  RecRmst(int id){rmst_no_ = id;}
  ~RecRmst(){
    void *tmp_frag_ptr;
    Int2Frag::iterator frag_iterator;
    for(frag_iterator=frag_map_.begin(); frag_iterator!=frag_map_.end(); ++frag_iterator){
      tmp_frag_ptr = (void*)(*frag_iterator).second;
      delete((char *)tmp_frag_ptr);
    }
  }
  int rmst_no_;
  int max_frag_;
  int max_frag_len_;
  int mtu_len_;
  Int2Frag frag_map_;
};
typedef map<int, RecRmst*, less<int> > Int2RecRmst;

class DiffusionRouting : public NR {
public:

#ifdef NS_DIFFUSION
  DiffusionRouting(u_int16_t port, DiffAppAgent *da);
  int getNodeId();               // node-id
  int getAgentId(int id = -1);   // port-id
  MobileNode *getNode(MobileNode *mn = 0)
  {
    if (mn != 0)
      node_ = mn;
    return node_;
  };
#else
  DiffusionRouting(u_int16_t port);
  void run(bool wait_condition, long max_timeout);
#endif // NS_DIFFUSION

  virtual ~DiffusionRouting();

  // NR Publish/Subscribe API functions

  handle subscribe(NRAttrVec *subscribe_attrs, NR::Callback *cb);

  int unsubscribe(handle subscription_handle);

  handle publish(NRAttrVec *publish_attrs);

  int unpublish(handle publication_handle);

  int send(handle publication_handle, NRAttrVec *send_attrs);

  int sendRmst(handle publication_handle, NRAttrVec *send_attrs, int fragment_size);

  // NR Filter API functions

  handle addFilter(NRAttrVec *filter_attrs, u_int16_t priority,
		   FilterCallback *cb);

  int removeFilter(handle filter_handle);

  int sendMessage(Message *msg, handle h, u_int16_t priority = FILTER_KEEP_PRIORITY);

  int addToBlacklist(int32_t node);

  int clearBlacklist();

  // NR Timer API functions
  handle addTimer(int timeout, TimerCallback *callback);

  // This is an old API function that will be discontinued in
  // diffusion's next major release
  handle addTimer(int timeout, void *param, TimerCallbacks *cb);

  bool removeTimer(handle hdl);

  // NR API functions that allow single thread support

  void doIt();

  void doOne(long timeout = WAIT_FOREVER);

  int interestTimeout(HandleEntry *handle_entry);
  int filterKeepaliveTimeout(FilterEntry *filter_entry);

#ifndef NS_DIFFUSION
  // Outside NS, all these can be protected members
protected:
#endif // !NS_DIFFUSION
  void recvPacket(DiffPacket pkt);
  void recvMessage(Message *msg);
#ifdef NS_DIFFUSION
  // In NS, the protected members start here
protected:
  // Handle to MobileNode
  MobileNode *node_;
#endif // NS_DIFFUSION

  void sendMessageToDiffusion(Message *msg);
  void sendPacketToDiffusion(DiffPacket pkt, int len, int dst);

  bool processRmst(Message *msg);
  void processMessage(Message *msg);
  void processControlMessage(Message *msg);

  bool checkSubscription(NRAttrVec *attrs);
  bool checkPublication(NRAttrVec *attrs);
  bool checkSend(NRAttrVec *attrs);
  bool isPushData(NRAttrVec *attrs);

  HandleEntry * removeHandle(handle my_handle, HandleList *hl);
  HandleEntry * findHandle(handle my_handle, HandleList *hl);

  FilterEntry * deleteFilter(handle my_handle);
  FilterEntry * findFilter(handle my_handle);
  bool hasScope(NRAttrVec *attrs);

  // RMST support 
  Int2RecRmst rec_rmst_map_;

  // Handle variables
  int next_handle_;
  HandleList pub_list_;
  HandleList sub_list_;
  FilterList filter_list_;

  // Threads and Mutexes
  pthread_mutex_t *dr_mtx_;

  // Data structures
  TimerManager *timers_manager_;

  // Lists
  DeviceList in_devices_;
  DeviceList local_out_devices_;

  // Node-specific variables
  u_int16_t diffusion_port_;
  int pkt_count_;
  int random_id_;

#ifdef NS_DIFFUSION
  int agent_id_;
#else
  u_int16_t agent_id_;
#endif // NS_DIFFUSION
};

#endif // !_DR_HH_
