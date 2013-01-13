// 
// iostats.hh      : Collect various statistics for Diffusion
// authors         : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: iostats.hh,v 1.2 2005/09/13 04:53:47 tomh Exp $
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

#ifndef _STATS_HH_
#define _STATS_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <list>

#include "main/message.hh"
#include "main/tools.hh"

using namespace std;

class NeighborStatsEntry {
public:
  int id_;
  int recv_messages_;
  int recv_bcast_messages_;
  int sent_messages_;

  NeighborStatsEntry(int id) : id_(id)
  {
    recv_messages_ = 0;
    recv_bcast_messages_ = 0;
    sent_messages_ = 0;
  }
};

typedef list<NeighborStatsEntry *> NeighborStatsList;

class DiffusionStats {
public:
  DiffusionStats(int id, int warm_up_time = 0);
  void logIncomingMessage(Message *msg);
  void logOutgoingMessage(Message *msg);
  void printStats(FILE *output);
  bool ignoreEvent();

private:
  int node_id_;
  int warm_up_time_;
  struct timeval start_;
  struct timeval finish_;
  NeighborStatsList nodes_;

  // Counters
  int num_bytes_recv_;
  int num_bytes_sent_;
  int num_packets_recv_;
  int num_packets_sent_;
  int num_bcast_bytes_recv_;
  int num_bcast_bytes_sent_;
  int num_bcast_packets_recv_;
  int num_bcast_packets_sent_;

  // Message counter
  int num_interest_messages_sent_;
  int num_interest_messages_recv_;
  int num_data_messages_sent_;
  int num_data_messages_recv_;
  int num_exploratory_data_messages_sent_;
  int num_exploratory_data_messages_recv_;
  int num_pos_reinforcement_messages_sent_;
  int num_pos_reinforcement_messages_recv_;
  int num_neg_reinforcement_messages_sent_;
  int num_neg_reinforcement_messages_recv_;

  int num_new_messages_sent_;
  int num_new_messages_recv_;
  int num_old_messages_sent_;
  int num_old_messages_recv_;

  NeighborStatsEntry * getNeighbor(int id);
};

#endif // !_STATS_HH_
