//
// rmst.hh         : Rmst Class
// authors         : Fred Stann
//
// Include file for Reliable Multi-Segment Transport support defines and ADTs.
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst.hh,v 1.2 2005/09/13 04:53:48 tomh Exp $
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

#ifndef RMST
#define RMST

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H

#include <map>
#include "nr/nr.hh"
#include "main/tools.hh"
typedef long handle;

// This dictates how much actual data goes into a packet. Because mote-nic
// will currently only allow 200 byte packets, it is pretty low.
#define MAX_FRAG_SIZE 50

typedef map<int, void*, less<int> > Int2Frag;

class NakData {
public:
  NakData(){nak_sent_ = false; send_nak_ = false;}
  struct timeval tmv;
  bool nak_sent_;
  bool send_nak_;
};

typedef map<int, NakData*, less<int> > Int2Nak;

class Rmst {
public:
  Rmst(int id, int mx_frag = -1);
  ~Rmst();
  void* getFrag(int frag_no);
  void putFrag(int frag_no, void *data);
  bool holeInFragMap();
  bool rmstComplete();
  bool inHoleMap(int hole_no);
  void putHole(int hole_no);
  NakData* getHole(int hole_no);
  void delHole(int hole_no);
  bool holeMapEmpty();
  void cleanHoleMap();
  void syncHoleMap();
  handle watchdog_handle_;
  bool watchdog_active_;
  bool cancel_watchdog_;
  handle ack_timer_handle_;
  bool ack_timer_active_;
  Int2Nak hole_map_;
  int rmst_no_;
  int max_frag_;
  int max_frag_len_;
  int max_frag_rec_;
  int max_frag_sent_;
  int sync_base_;
  int pkts_rec_;
  int pkts_sent_;
  int naks_rec_;
  int32_t last_hop_;
  int32_t fwd_hop_;
  int last_hop_pkts_sent_;
  bool local_source_;
  u_int16_t local_source_port_;
  char *target_str_;
  bool reinf_;
  bool acked_;
  bool resent_last_data_;
  bool resent_last_exp_;
  bool wait_for_new_path_;
  bool sent_exp_req_;
  struct timeval last_data_time_;
  struct timeval last_nak_time_;
  struct timeval exp_req_time_;
private:
  Int2Frag frag_map_;
};

typedef map<int, Rmst*, less<int> > Int2Rmst;

// Utility function
void PrintTime(struct timeval *time);

#endif // RMST
