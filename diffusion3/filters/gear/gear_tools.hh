//
// gear_tools.hh  : GEAR Tools Include File
// authors        : Yan Yu and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// Copyright (C) 2000-2002 by the University of California
// $Id: gear_tools.hh,v 1.2 2005/09/13 04:53:48 tomh Exp $
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

#ifndef _GEAR_TOOLS_HH_
#define _GEAR_TOOLS_HH_

#include <math.h>
#include "diffapp.hh"

#define FAIL                      -1
#define INITIAL_HEURISTIC_VALUE   -1
#define FORWARD_TABLE_SIZE        100
#define	LEARNED_COST_TABLE_SIZE	  1000

#define	GEO_HEURISTIC_VALUE_UPDATE_THRESHOLD 2

#define	ABS(x)          ((x) >= 0 ? (x): -(x))

class HeuristicValue {
public:
  HeuristicValue(double dst_longitude, double dst_latitude,
		 double heuristic_value) : dst_longitude_(dst_longitude),
					   dst_latitude_(dst_latitude),
					   heuristic_value_(heuristic_value)
  {};

  double dst_longitude_;
  double dst_latitude_;
  double heuristic_value_;
};

class GeoLocation {
public:
  void operator= (GeoLocation loc)
  {
    longitude_ = loc.longitude_;
    latitude_ = loc.latitude_;
  }
  void output()
  {
    DiffPrint(DEBUG_IMPORTANT, "(%f, %f)", longitude_, latitude_ );
  }

  double longitude_;
  double latitude_;
};

class HeuristicValueEntry {
public:
  HeuristicValueEntry() {
    heuristic_value_ = INITIAL_HEURISTIC_VALUE;
  }

  GeoLocation dst_;
  double heuristic_value_;
};

class LearnedCostEntry {
public:
  LearnedCostEntry() {
    learned_cost_value_ = INITIAL_HEURISTIC_VALUE;
  }

  int node_id_;
  GeoLocation dst_;
  double learned_cost_value_;
};

//local forwarding table at each node
class HeuristicValueTable {
public:
  HeuristicValueTable() {num_entries_ = 0; first_ = -1; last_ = -1;}

  HeuristicValueEntry table_[FORWARD_TABLE_SIZE];

  int retrieveEntry(GeoLocation  *dst);
  inline int numEntries() {return num_entries_;}

  void addEntry(GeoLocation dst, double heuristic_value);
  bool updateEntry(GeoLocation dst, double heuristic_value);
  //the return value of UpdateEntry is if there is significant change
  //compared to its old value, if there is(i.e., either the change
  //pass some threshold-GEO_H_VALUE_UPDATE_THRE or it is a new entry),
  //then return TRUE, o/w return FALSE;
  int next(int i) {return ((i+1) % FORWARD_TABLE_SIZE);}
  int last() {return last_;}

private:
  int num_entries_;
  int first_;
  int last_;
};

// Local forwarding table
class LearnedCostTable {
public:
  LearnedCostTable() {num_entries_ = 0; first_ = -1; last_ = -1;}

  LearnedCostEntry table_[LEARNED_COST_TABLE_SIZE];

  int retrieveEntry(int neighbor_id, GeoLocation *dst);
  inline int numEntries() {return num_entries_;}

  void addEntry(int neighbor_id, GeoLocation dst, double learned_cost);
  void updateEntry(int neighbor_id, GeoLocation dst, double learned_cost);
  int next(int i) {return ((i+1) % LEARNED_COST_TABLE_SIZE);}
  int last() {return last_;}

private:
  int num_entries_;
  int first_;
  int last_;
};

double Distance(double long1, double lat1, double long2, double lat2);
bool IsSameLocation(GeoLocation src, GeoLocation dst);

#endif // !_GEAR_TOOLS_HH_
