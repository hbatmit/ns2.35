//
// gear.hh         : GEAR Include File
// authors         : Yan Yu and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// Copyright (C) 2000-2002 by the University of California
// $Id: gear.hh,v 1.3 2005/09/13 04:53:48 tomh Exp $
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

#ifndef _GEAR_HH_
#define _GEAR_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <list>

#ifdef HAVE_HASH_MAP
#include <hash_map>
#else
#ifdef HAVE_EXT_HASH_MAP
#include <ext/hash_map>
#endif // HAVE_EXT_HASH_MAP
#endif // HAVE_HASH_MAP

#ifdef NS_DIFFUSION
#include <mobilenode.h>
#endif // NS_DIFFUSION

#include "gear_attr.hh"
#include "gear_tools.hh"

#include "diffapp.hh"

// Filter priorities for pre-processing and post-processing. The
// gradient's filter priority has to be in between these two values in
// order for GEAR to work properly
#define GEOROUTING_PRE_FILTER_PRIORITY  170
#define GEOROUTING_POST_FILTER_PRIORITY 20

// Values used for defining timers
#define BEACON_REQUEST_TIMER 150
#define NEIGHBOR_TIMER       151

// Energy values for GEAR
#define	GEO_INITIAL_ENERGY          1
#define	GEO_UNIT_ENERGY_FOR_SEND    0.001
#define	GEO_UNIT_ENERGY_FOR_RECV    0.001

// Various beacon types
enum geo_beacons {
  GEO_REQUEST = 1, // Beacon request. Nodes should send a beacon reply
		   // in response
  GEO_REPLY,       // Beacon reply. Sent in response to a beacon
		   // request. It also includes an heuristic value if
		   // requested on the beacon request
  GEO_UPDATE       // Includes updates to a particular heuristic value
		   // (for a given destination). It is send only if
		   // the new value is sufficiently different from the
		   // previous one
};

// Various actions taken when forwarding interests
enum geo_actions {
  BROADCAST = 0,      // We are inside the target region, broadcast
		      // packet
  BROADCAST_SUPPRESS, // All our neighbors are outside the target
		      // region, we should not forward this interest
		      // message
  OUTSIDE_REGION      // We are still outside the target region,
		      // continue forwarding this interest message
		      // towards the region using unicast
};

#define	GEO_BEACON_REPLY_PERIOD	    100 // Sends at most one
					// beacon_reply message every
					// GEO_BEACON_REPLY_PERIOS
					// seconds
#define GEO_NEIGHBOR_DELAY        30000 // In milli-seconds

#define GEO_BEACON_REQUEST_CHECK_PERIOD 100000 // In milli-seconds
#define GEO_NEIGHBOR_UPDATE                300 // In seconds
#define GEO_NEIGHBOR_REQUEST_PERIOD (10 * GEO_NEIGHBOR_UPDATE) // In seconds

#define GEO_NEIGHBOR_EXPIRED (5 * GEO_NEIGHBOR_UPDATE) // In seconds,
					               // this is how
					               // long a
					               // neighbor
					               // entry will
					               // last before
					               // being
					               // deleted

// These values tell GEAR how much time to wait before sending a
// beacon reply in response to a beacon request message
#define GEO_BEACON_REPLY_DELAY   1500 // (msec) between receive and forward
#define GEO_BEACON_REPLY_JITTER  1000 // (msec) jitter

// These values tell GEAR how much to wait before sending a beacon
// request or a beacon update message
#define GEO_BEACON_DELAY   400 // (msec) between receive and forward
#define GEO_BEACON_JITTER  200 // (msec) jitter

#define	INITIAL_ENERGY		1
#define	DEFAULT_VALID_PERIOD	10

#define UNICAST_ORIGINAL 1
#define	BROADCAST_TYPE   2
#define	MAX_INT          10000

#define MAX_PATH_LEN     200

class Region {
public:
  void operator= (Region p) {center_ = p.center_; radius_ = p.radius_;}
  void output()
  {
    center_.output();
    DiffPrint(DEBUG_IMPORTANT, "-%f", radius_);
  }

  GeoLocation center_;
  double radius_;
};

class GeoHeader {
public:
  int16_t pkt_type_;  // BROADCAST or UNICAST_ORIGINAL
  int16_t path_len_;
  Region dst_region_;
};
 
class PktHeader {
public:
  int32_t pkt_num_;
  int32_t rdm_id_;
  int32_t prev_hop_;
  int pkt_type_;
  int path_len_;
  Region dst_region_;
};

class NeighborEntry {
public:
  NeighborEntry(int32_t id, double longitude, double latitude,
		double remaining_energy) :
  id_(id), longitude_(longitude), latitude_(latitude),
  remaining_energy_(remaining_energy){
    valid_period_ = DEFAULT_VALID_PERIOD;
    GetTime(&tv_);
  }

  int32_t id_;
  double longitude_;
  double latitude_;
  double remaining_energy_;
  struct timeval tv_;
  double valid_period_; // in seconds
};

class GeoRoutingFilter;

typedef list<NeighborEntry *> NeighborList;
typedef list<PktHeader *> PacketList;
 
class GeoFilterReceive : public FilterCallback {
public:
  GeoFilterReceive(GeoRoutingFilter *app) : app_(app) {};
  void recv(Message *msg, handle h);

  GeoRoutingFilter *app_;
};

class GeoRoutingFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  GeoRoutingFilter(const char *diffrtg);
  int command(int argc, const char*const* argv);
#else
  GeoRoutingFilter(int argc, char **argv);
#endif // NS_DIFFUSION

  virtual ~GeoRoutingFilter()
  {
    // Nothing but exit
  };

  void run();
  void recv(Message *msg, handle h);

  // Timers
  void messageTimeout(Message *msg);
  void beaconTimeout();
  void neighborTimeout();
  
protected:
  // General Variables
  handle pre_filter_handle_;
  handle post_filter_handle_;
  int pkt_count_;
  int rdm_id_;

  // Keep track when last beacon reply was sent
  struct timeval last_beacon_reply_tv_;

  // Keep track when last beacon request was sent
  struct timeval last_neighbor_request_tv_;
  
  // Statistical data: location and remaining energy level
  double geo_longitude_;
  double geo_latitude_;
  int num_pkt_sent_;
  int num_pkt_recv_;
  double initial_energy_;
  double unit_energy_for_send_;
  double unit_energy_for_recv_;

  // List of all known neighbors, containing their location and energy
  // information
  NeighborList neighbors_list_;

  // List of messages currently being processed
  PacketList message_list_;

  // Forwarding table
  HeuristicValueTable h_value_table_;
  LearnedCostTable learned_cost_table_;
  
  // Receive Callback for the filter
  GeoFilterReceive *filter_callback_;

  // Setup the filter
  handle setupPostFilter();
  handle setupPreFilter();

  // Message Processing functions
  void preProcessFilter(Message *msg);
  void postProcessFilter(Message *msg);

  // Message processing functions
  PktHeader * preProcessMessage(Message *msg);
  PktHeader * stripOutHeader(Message *msg);
  PktHeader * retrievePacketHeader(Message *msg);
  bool extractLocation(Message *msg,
		       float *longitude_min, float *longitude_max,
		       float *latitude_min, float *latitude_max);
  GeoHeader * restoreGeoHeader(PktHeader *pkt_header, Message *msg);
  void takeOutAttr(NRAttrVec *attrs, int32_t key);

  // Neighbors related functions
  NeighborEntry * findNeighbor(int32_t neighbor_id);
  void updateNeighbor(int32_t neighbor_id, double neighbor_longitude,
		      double neighbor_latitude, double neighbor_energy);
  bool checkNeighbors();
  void sendNeighborRequest();

  // Energy related functions
  double remainingEnergy() {return INITIAL_ENERGY;}

  // Cost estimation related functions
  double retrieveLearnedCost(int neighbor_id, GeoLocation dst);
  double estimateCost(int neighbor_id, GeoLocation dst);

  // Routing related functions
  int32_t findNextHop(GeoHeader *geo_header, bool greedy);
  int floodInsideRegion(GeoHeader *geo_header);

  double retrieveHeuristicValue(GeoLocation dst);
  void broadcastHeuristicValue(GeoLocation dst, double new_heuristic_value);

  // GetNodeLocation --> This will move to the library in the future
  void getNodeLocation(double *longitude, double *latitude);
};

class GeoMessageSendTimer : public TimerCallback {
public:
  GeoMessageSendTimer(GeoRoutingFilter *agent, Message *msg) :
    agent_(agent), msg_(msg) {};
  ~GeoMessageSendTimer()
  {
    delete msg_;
  };
  int expire();

  GeoRoutingFilter *agent_;
  Message *msg_;
};

class GeoNeighborsTimer : public TimerCallback {
public:
  GeoNeighborsTimer(GeoRoutingFilter *agent) : agent_(agent) {};
  ~GeoNeighborsTimer() {};
  int expire();

  GeoRoutingFilter *agent_;
};

class GeoBeaconRequestTimer : public TimerCallback {
public:
  GeoBeaconRequestTimer(GeoRoutingFilter *agent) : agent_(agent) {};
  ~GeoBeaconRequestTimer() {};
  int expire();

  GeoRoutingFilter *agent_;
};

#endif // !_GEAR_HH_
