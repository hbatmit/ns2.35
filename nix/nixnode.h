/* -*-	Mode:C++; c-basic-offset:2; tab-width:2; indent-tabs-mode:f -*- */
/* Nix-Vector routing capable node. */
/* George F. Riley, Georgia Tech, Spring 2000 */
/* Changed to NOT subclass the Node class, suggestion from ns developers */

#ifndef __NIXNODE_H__
#define __NIXNODE_H__

#include "routealgo/rnode.h"
#include "object.h"
#include <map>

// Define the edge class
class Edge {
public :
  Edge( nodeid_t n) : m_n(n) { };
public :
  nodeid_t m_n; // Other end of edge neighbor
};

typedef vector<Edge*>         EdgeVec_t;
typedef EdgeVec_t::iterator   EdgeVec_it;

typedef vector<NsObject*>     ObjVec_t;
typedef ObjVec_t::iterator    ObjVec_it;

// Use a map to keep a table of known NixVectors
typedef map<nodeid_t, NixVec*, less<nodeid_t> > NVMap_t;
typedef NVMap_t::iterator                       NVMap_it;
typedef NVMap_t::value_type                     NVPair_t;

class NixNode : public RNode {
public :
	NixNode();
	void     Id(nodeid_t id) { m_id = id;}
  nodeid_t Id()     const { return m_id;}
  int      Map()    const { return m_Map;}
  void AddAdj(nodeid_t);     // Add a simplex link from this to neighbor
  int  IsNeighbor(nodeid_t); // True if specified node is neighbor
  virtual const NodeWeight_t NextAdj( const NodeWeight_t&); // Return next adjacent
  NixVec* ComputeNixVector(nodeid_t);       // Compute the NixVector to target
  virtual NixPair_t GetNix(nodeid_t);       // Get neighbor index/length
  virtual Nixl_t    GetNixl();              // Get bits needed for nix entry
  virtual nodeid_t  GetNeighbor(Nix_t, NixVec*);     // Get neighbor from nix
	NixVec*           GetNixVector(nodeid_t); // Get a nix vector for a target
  NsObject*         GetNsNeighbor(Nix_t);   // Get the ns nexthop neighbor
	void    PopulateObjects(void);       // Populate NS NextHop objects
  static NixNode*   GetNodeObject(nodeid_t); // Get a node obj. based on id
  static void       PopulateAllObjects(void);// Populate the next hop objects
private :
  EdgeVec_t    m_Adj;             // Adjacent edges
	ObjVec_t     m_AdjObj;          // NS Objects for adjacencies
  int          m_Map;             // Which system this node is mapped to
  NVMap_t*     m_pNixVecs;        // Hash-map list of known NixVectors
};
#endif


