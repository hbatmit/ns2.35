//
// rmst_filter.hh  : RmstFilter Class
// authors         : Fred Stann
//
// Include file for RmstFilter - Reliable Multi-Segment Transport
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_filter.hh,v 1.3 2005/09/13 04:53:49 tomh Exp $
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

#ifndef RMST_FILTER_HH
#define RMST_FILTER_HH

#include <sys/types.h>
#include <map>
#include <list>
#include "diffapp.hh"
#include "rmst.hh"

#define RMST_FILTER_PRIORITY 190

// Union to convert pkt_num & rdm_id into a 64 bit key
union LlToInt {
  int64_t ll_val_;
  int int_val_[2];
};

// Exploratory message log.  Keep track of the last hop of every
// new exploratory message. Then when one is reinforced we can
// identify the "back-channel" that is the reverse gradient from
// sink to source.
typedef struct _ExpLog{
  int rmst_no_;
  int32_t last_hop_;
} ExpLog;
typedef map<int64_t, ExpLog> Key2ExpLog;

typedef struct _SendMsgData{
  int rmst_no_;
  int last_frag_sent_;
  int exp_base_;
} SendMsgData;
typedef list<SendMsgData> SendList;

typedef struct _NakMsgData{
  int rmst_no_;
  int frag_no_;
} NakMsgData;
typedef list<NakMsgData> NakList;

typedef list<int32_t> Blacklist;

class RmstFilter;

// Method to be called when a message matches rmst-filter attributes.
class RmstFilterCallback: public FilterCallback {
public:
  RmstFilter *app_;                     // initialzed in setupFilter
  void recv(Message *msg, handle h);
};

class RmstFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  RmstFilter();
  int command(int argc, const char*const* argv);
#else
  RmstFilter(int argc, char **argv);
#endif // NS_DIFFUSION
  
  virtual ~RmstFilter(){};
  void run();
  void recv(Message *msg, handle h);
  int processTimer(int rmst_no, int timer_type);
private:
  handle filter_handle_;     // filter handle
  RmstFilterCallback *fcb_;  // filter callback routine
  handle setupFilter();    // routine that adds filter
  handle stat_timer_handle_;
  // Message Processing
  bool processMessage(Message *msg);
  void processCtrlMessage(Message *msg);
  Rmst* syncLocalCache(Message *msg);
  void sendRmstToSink(Rmst *rmst_ptr);
  void sendAckToSource(Rmst *rmst_ptr);
  void sendExpReqUpstream(Rmst *rmst_ptr);
  void sendContToSource(Rmst *rmst_ptr);
  void setupNak(int rmst_id);
  void cleanUpRmst(Rmst *rmst_ptr);
  void processExpReq(Rmst *rmst_ptr, int frag_no);
  Key2ExpLog exp_map_;
  Int2Rmst rmst_map_;
  SendList send_list_;
  bool send_timer_active_;
  handle send_timer_handle_;
  int exp_gap_;
  NakList nak_list_;
  Blacklist black_list_;
  int rdm_id_;
  int pkt_count_;
  bool local_sink_;
  bool caching_mode_;
  u_int16_t local_sink_port_;
  NRAttrVec *interest_attrs_;
  struct timeval last_data_rec_;
  struct timeval last_sink_time_;
};

// Define Timer Types
#define WATCHDOG_TIMER   1
#define SEND_TIMER       2
#define ACK_TIMER        3
#define CLEANUP_TIMER    4

// Define timer intervals. These intervals dictate how fast you send
// packets through the sensor net, how often you check for holes,
// how often you clean up cache residue, how frequently a source
// prompts for a missing ACK. These parameters can be application
// tuned. The send timer in particular can be altered depending on
// the presence of a backpressure mechanism.
// SEND_INTERVAL - Gap between initiating the sending of a fragment.
// WATCHDOG_INTERVAL - How often we look for holes (missing fragments).
// ACK_INTERVAL - How long we wait for an ACK after sending last fragment
//                before re-sending the last fragment.
// CLEANUP_INTERVAL - How often we clean up our cache.
// The time is in milliseconds.
#define SEND_INTERVAL 2000
#define WATCHDOG_INTERVAL 10000
#define ACK_INTERVAL 20000
#define CLEANUP_INTERVAL 90000

// Define time intervals used to make decisions.
// These are application tunable.
// These times are in seconds.
#define RMST_BLACKLIST_WAIT 3600
#define NAK_RESPONSE_WAIT 10
#define NEXT_FRAG_WAIT 15
#define ACK_WAIT 30
#define LONG_CLEANUP_WAIT 90
#define SHORT_CLEANUP_WAIT 30
#define SINK_REFRESH_WAIT 120

// Define the threshold at which a node blacklists an incoming link.
// It is the percentage of upstream packets sent that are actually
// received. If a node gets less than this percentage, the link
// is blacklisted.
#define BLACKLIST_THRESHOLD .60

class RmstTimeout: public TimerCallback {
public:
  RmstTimeout(RmstFilter *rmst_flt, int no, int type);
  int expire();
  RmstFilter *filter_;
  int rmst_no_;
  int timer_type_;
};

// Define Send Timer Actions
#define DELETE_FROM_QUEUE    1
#define SEND_NEXT_FRAG       2
#define DO_NOTHING           3

#endif // RMST_FILTER_HH
