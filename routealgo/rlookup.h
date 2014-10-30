// Define classes for routing table representation and lookup
// George F. Riley, Georgia Tech, Winter 2000

// Defines several variations on routing table representations
// and a method to determine the most memory efficient.

#ifndef __RLOOKUP_H__
#define __RLOOKUP_H__

#include "routealgo/rnode.h"
#include "routealgo/rbitmap.h"

#include <iostream>
#ifdef USE_HASH_MAP
#include <hash_map>
#include <hash_set>
#else
#include <map>
#include <set>
#endif

typedef enum {
  RL_NULL, RL_FIXED, RL_BITMAP, RL_HASH, RL_NEXTHOP, RL_LAST}
RLookup_Types;

typedef pair<nodeid_t, nodeid_t> LookupPair_t; // Node/NextHop Pair

// Define an abstract base class that specifies functionality needed
class RLookup {
public :
  RLookup();
  virtual ~RLookup();
  virtual RLookup_Types WhatType() const = 0; // Identifies the type of lookup
  virtual void Populate(  // Popluate the table
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d = NODE_NONE, // Default route
                        nodeid_t o = NODE_NONE, // Routing table owner
                        nodeid_t f = NODE_NONE, // First non-default
                        nodeid_t l = NODE_NONE) // Last non-default
                        = 0;
  virtual void Populate(istream& is);           // Populate from log
  virtual nodeid_t Lookup(nodeid_t) = 0;        // Return next hop given target
  virtual size_t   Size() = 0;                  // Estimate size
  virtual size_t   NumberEntries(){return 0;}   // Number of entries in table
  virtual void     Log( ostream&);              // Log to ostream
  static void Analyze(RoutingVec_t&,
                      RoutingVec_t&,            // Population counts
                      nodeid_t&, nodeid_t&, nodeid_t, nodeid_t&,nodeid_t&);
  //  friend ostream& operator<<(ostream&, const RLookup& ); // Output a routing table
};

// The "Null" lookup is used when there is no routing table.
// Handles the pathological case for a node with no neighbors

class NOLookup : public RLookup {
public :
  NOLookup();
  virtual ~NOLookup();
  virtual RLookup_Types WhatType() const;
  virtual void Populate(  // Popluate the table
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d = NODE_NONE, // Default route
                        nodeid_t o = NODE_NONE, // Routing table owner
                        nodeid_t f = NODE_NONE, // First non-default
                        nodeid_t l = NODE_NONE);// Last non-default
  virtual nodeid_t Lookup(nodeid_t);
  virtual size_t Size();
  virtual void   Log( ostream&);              // Log to ostream
};

// The "Fixed" lookup is used when all routes are the same
// Will always be the case for "leaf" nodes with only one neighbor
class FRLookup : public  RLookup {
public :
  FRLookup();
  virtual ~FRLookup();
  nodeid_t Default() const { return m_Default;}
  virtual RLookup_Types WhatType() const;
  virtual void Populate(  // Popluate the table
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d = NODE_NONE, // Default route
                        nodeid_t o = NODE_NONE, // Routing table owner
                        nodeid_t f = NODE_NONE, // First non-default
                        nodeid_t l = NODE_NONE);// Last non-default
  virtual nodeid_t Lookup(nodeid_t);
  virtual size_t Size();
  virtual void   Log( ostream&);              // Log to ostream
  static  size_t EstimateSize(
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d,      // Default route
                        nodeid_t n,      // Number default
                        nodeid_t o,      // Routing table owner
                        nodeid_t f,      // First non-default
                        nodeid_t l);     // Last non-default
  //  friend ostream& operator<<(ostream&, const FRLookup& ); // Output a routing table
private:
  nodeid_t m_Default;  
};

// The "Bitmap" lookup is used when there are many "default" entries
// and a few non-defaults clustered in a small range
class BMLookup : public  RLookup {
public :
  BMLookup();
  virtual ~BMLookup();
  virtual RLookup_Types WhatType() const; // Identifies the type of lookup
  virtual void Populate(  // Popluate the table
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d = NODE_NONE, // Default route
                        nodeid_t o = NODE_NONE, // Routing table owner
                        nodeid_t f = NODE_NONE, // First non-default
                        nodeid_t l = NODE_NONE);// Last non-default
  virtual nodeid_t Lookup(nodeid_t);
  virtual size_t   Size();
  virtual size_t   NumberEntries();             // Number of entries in table
  virtual void     Log( ostream&);              // Log to ostream
  static  size_t   EstimateSize(
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d,      // Default route
                        nodeid_t n,      // Number default
                        nodeid_t o,      // Routing table owner
                        nodeid_t f,      // First non-default
                        nodeid_t l);     // Last non-default
  //  friend ostream& operator<<(ostream&, const BMLookup& ); // Output a bitmap routing table
private:
  nodeid_t     m_Default;  
  nodeid_t     m_FirstNonDefault;
  nodeid_t     m_LastNonDefault;
  RoutingVec_t m_NVec;  // Neighbor vector
  BitMap*      m_pBitMap; // Bitmap of non-default entries
};

// The "HashMap" lookup is used when the previous two methods do not work.
// Uses the STL "hash_map" associative container to store non-default routes.
// By default, this is OFF because hash_map is an SGI extension to STL,
// not part of standard STL.
#ifdef USE_HASH_MAP
typedef hash_map<nodeid_t, nodeid_t,
                 hash<nodeid_t>, equal_to<nodeid_t> > RouteMap_t;
#else
typedef map<nodeid_t, nodeid_t,  equal_to<nodeid_t> > RouteMap_t;
#endif

typedef RouteMap_t::iterator                          RouteMap_it;
typedef RouteMap_t::value_type                        RoutePair_t;

class HMLookup : public  RLookup {
public :
  HMLookup();
  virtual ~HMLookup();
  virtual RLookup_Types WhatType() const; // Identifies the type of lookup
  virtual void Populate(  // Popluate the table
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d = NODE_NONE, // Default route
                        nodeid_t o = NODE_NONE, // Routing table owner
                        nodeid_t f = NODE_NONE, // First non-default
                        nodeid_t l = NODE_NONE);// Last non-default
  virtual nodeid_t Lookup(nodeid_t);
  virtual size_t   Size();
  virtual size_t   NumberEntries();             // Number of entries in table
  virtual void     Log( ostream&);              // Log to ostream
  static  size_t   EstimateSize(
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d,      // Default route
                        nodeid_t n,      // Number default
                        nodeid_t o,      // Routing table owner
                        nodeid_t f,      // First non-default
                        nodeid_t l);     // Last non-default
  //  friend ostream& operator<<(ostream&, const HMLookup& ); // Output a routing table
private:
  nodeid_t     m_Default;  
  RouteMap_t   m_RouteMap;
};

// Also included is the "inefficient" version for memory usage comparisons
// NHLookup (NextHop) lookup just stores the next hop in a lookup array
class NHLookup : public  RLookup {
public :
  NHLookup();
  virtual ~NHLookup();
  virtual RLookup_Types WhatType() const; // Identifies the type of lookup
  virtual void Populate(  // Popluate the table
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d = NODE_NONE, // Default route
                        nodeid_t o = NODE_NONE, // Routing table owner
                        nodeid_t f = NODE_NONE, // First non-default
                        nodeid_t l = NODE_NONE);// Last non-default
  virtual void Populate(istream& is);           // Populate from log
  virtual nodeid_t Lookup(nodeid_t);
  virtual size_t   Size();
  virtual size_t   NumberEntries();             // Number of entries in table
  virtual void     Log( ostream&);              // Log to ostream
  static  size_t   EstimateSize(
                        RoutingVec_t& r, // NextHop table
                        RoutingVec_t& p, // Population counts
                        nodeid_t d,      // Default route
                        nodeid_t n,      // Number default
                        nodeid_t o,      // Routing table owner
                        nodeid_t f,      // First non-default
                        nodeid_t l);     // Last non-default
private:
  RoutingVec_t   m_RouteTable;
};



#endif
