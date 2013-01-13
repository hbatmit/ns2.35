// Define a "routing capable" node
// Insures functionality to run routing algorithms

#ifndef __RNODE_H__
#define __RNODE_H__

#include "nix/nixvec.h"

#include <set>
#include <map>
#include <vector>
#include <deque>
#include <algorithm>

#ifdef USE_HASH
#include <hash_set>
#endif

typedef unsigned long nodeid_t; // Node identifier
typedef unsigned long dist_t;   // Distance
typedef unsigned int  weight_t; // Weight
typedef pair<nodeid_t, weight_t> NodeWeight_t; // Node/Distance Pair

const dist_t   INF  = 0xffffffff;
const nodeid_t NODE_NONE = 0xffffffff;

class RNode {
  public :
    RNode( );
    RNode( nodeid_t id);
    RNode( const RNode& n);
    virtual ~RNode();
    virtual void AddAdj(nodeid_t a, int w = 1);
    // Get next adjacent node..pass in NODE_NONE to get first
    virtual const NodeWeight_t NextAdj( const NodeWeight_t&);
    virtual NixPair_t GetNix(nodeid_t);  // Get neighbor index/length
    virtual Nixl_t    GetNixl();         // Get bits needed for nix entry
    virtual nodeid_t  GetNeighbor(Nix_t);// Get neighbor from nix
    //int const operator=  (const dist_t& n) { return n == m_id;}
    //int const operator!= (const dist_t& n) { return n != m_id;}
  public :
    nodeid_t m_id; // This node identifier
};

// Declare the equal and not= operators
//int operator== (const RNode& N, const dist_t& n) { return N.m_id == n;}
//int operator!= (const RNode& N, const dist_t& n) { return N.m_id != n;}
//int operator== (RNode& N, const dist_t& n) { return N.m_id == n;}
//int operator!= (RNode& N, const dist_t& n) { return N.m_id != n;}

// Define the vector for nodes
typedef vector<RNode*>         RNodeVec_t;
typedef RNodeVec_t::iterator   RNodeVec_it;

// Define the deque for nodes
typedef deque<RNode*>          RNodeDeq_t;
typedef RNodeDeq_t::iterator   RNodeDeq_it;

// Define the vector for next hop neighbors
typedef vector<nodeid_t>       RoutingVec_t;
typedef RoutingVec_t::iterator RoutingVec_it;

// Define the distance vector
typedef vector<dist_t>         DistVec_t;
typedef DistVec_t::iterator    DistVec_it;

#endif
