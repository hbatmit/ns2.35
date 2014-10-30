// Define classes for routing table representation and lookup
// George F. Riley, Georgia Tech, Winter 2000

// Defines several variations on routing table representations
// and a method to determine the most memory efficient.

#include "config.h"
#ifdef HAVE_STL

#include <stdio.h>
#include <routealgo/rlookup.h>
//#include <routealgo/rbitmap.h>

// Static function for RLookup
// Analyze  a routing table, find best default, and first/last non-default

void RLookup::Analyze( // Analyze the next hop table
    RoutingVec_t& r,
    RoutingVec_t& p, // Population counts (returned)
    nodeid_t& d, // default(returned)
    nodeid_t& n, // number of default entries (returned)
    nodeid_t  o, // table's owner
    nodeid_t& f, // first non-default (returned)
    nodeid_t& l) // last  non-default (returned)
{
  unsigned int i;

  p.erase(p.begin(), p.end());
  p.reserve(r.size()); // This is how many entries we need
  for (i = 0; i < r.size(); i++) p.push_back(0);
  for (i = 0; i < r.size(); i++)
    {
      if (i != o)
        p[r[i]]++; // Count occurrences of each (except oaner)
    }
  // Find the best default
  nodeid_t largest = NODE_NONE;
  if(0)printf("Pop size %lu\n", (unsigned long)p.size());
  nodeid_t nonzero = 0; // Number of non-zero entries
  for (i = 0; i < p.size(); i++)
    {
      if(0)printf("Pop %d = %ld\n", i, p[i]);
      if (p[i]) nonzero++; // Count non-zeros
      if (largest == NODE_NONE || p[i] > p[largest])
        largest = i;
    }
  d = largest; // Set the default
  if (nonzero == 0)
    d = NODE_NONE; // Pathological case, no routes at all
  n = p[largest]; // number of default
  f = NODE_NONE;
  l = NODE_NONE;
  if (nonzero == 1)
    { // Nothing but defaults, easy case
      return;
    }
  for (i = 0; i < r.size(); i++)
    {
      if (i != o)
        {
          if (r[i] != d && f == NODE_NONE) f = i;
          if (r[i] != d) l = i;
        }
    }
}

// RLookup constructor/destructor
RLookup::RLookup()  { }
RLookup::~RLookup() { }

// Function to output a routing table on an ostream
void RLookup::Log( ostream& os)
{
  os << "LOG called";
}

void RLookup::Populate( istream& )
{
  printf("Populate(istream) called\n");
}

#ifdef OLD_WAY
ostream& operator<<(ostream& os , const RLookup& R ) // Output a routing table
{
  int w = R.WhatType();
  os << w ; // Log the type for reconstruction
  switch( w ) {
    case RL_FIXED :
      os << *((FRLookup*)&R);
      break;
    case RL_BITMAP :
      os << *((BMLookup*)&R);
      break;
    case RL_HASH :
      os << *((HMLookup*)&R);
      break;
    case RL_NEXTHOP :
      os << "NHLog not coded";
      break;
  }
  return os;
}
#endif

// Null Routing methods

NOLookup::NOLookup()
{
  if(0)printf("Created NOLookup\n");
}

NOLookup::~NOLookup()
{
}

void NOLookup::Populate(
    RoutingVec_t&,   // NextHop table
    RoutingVec_t&,   // Population counts
    nodeid_t,        // Default route
    nodeid_t,
    nodeid_t,
    nodeid_t)
{

}

RLookup_Types NOLookup::WhatType(void) const // Identifies the type of lookup
{
  return RL_NULL;
}


nodeid_t NOLookup::Lookup(nodeid_t)
{
  return(NODE_NONE);
}

size_t   NOLookup::Size()
{
  return(0);
}

void NOLookup::Log( ostream& os)
{
  os << " " << (int)WhatType();
}

// Fixed Routing methods

FRLookup::FRLookup() : m_Default(NODE_NONE)
{
  if(0)printf("Created FRLookup\n");
}

FRLookup::~FRLookup()
{
}

void FRLookup::Populate(
    RoutingVec_t&,   // NextHop table
    RoutingVec_t&,   // Population counts
    nodeid_t d,      // Default route
    nodeid_t,
    nodeid_t,
    nodeid_t)
{
  m_Default = d;
}

RLookup_Types FRLookup::WhatType(void) const // Identifies the type of lookup
{
  return RL_FIXED;
}


nodeid_t FRLookup::Lookup(nodeid_t)
{
  return(m_Default);
}

size_t   FRLookup::Size()
{
  return sizeof(m_Default);
}

size_t   FRLookup::EstimateSize(
    RoutingVec_t&,   // NextHop table
    RoutingVec_t&,   // Population counts
    nodeid_t,        // Default route
    nodeid_t,        // Number default
    nodeid_t,
    nodeid_t,
    nodeid_t)
{
  return sizeof(u_long);
}

void FRLookup::Log( ostream& os)
{
  os << " " << (int)WhatType();
  os << " " << Default();
}

class BitMap;

// Bitmap routing methods

BMLookup::BMLookup() : m_Default(NODE_NONE), m_FirstNonDefault(NODE_NONE), 
                       m_LastNonDefault(NODE_NONE), m_pBitMap(0)
{
  if(0)printf("Created BMLookup\n");
}

BMLookup::~BMLookup()
{
  if (m_pBitMap) delete m_pBitMap;
}

RLookup_Types BMLookup::WhatType(void) const // Identifies the type of lookup
{
  return RL_BITMAP;
}

void BMLookup::Populate(
    RoutingVec_t& r, // NextHop table
    RoutingVec_t& p, // Population counts
    nodeid_t d, // Default route
    nodeid_t ,
    nodeid_t f,
    nodeid_t l)
{
  unsigned int i;

  m_Default = d; // Set the default route

  // Now for each non-zero NH neighbor, create an entry in the NVec
  for (i = 0; i < p.size(); i++)
    {
      if (p[i])
        {
          m_NVec.push_back((nodeid_t)i);
          if(0)printf("Creating nix entry %d\n", i);
        }
    } 
  u_long c = l - f + 1; // How many entries we need
  if(0)printf("BMLookup::Populate..found %lu NHN\n", (unsigned long)m_NVec.size());
  m_pBitMap = new BitMap(c, BitMap::FindBPE(m_NVec.size()));
  
  // And populate the bitmap   
  for(u_long k = 0; k < c; k++)
    {
      u_long ind;
      for (ind = 0; ind < m_NVec.size(); ind++)
        {
          if (m_NVec[ind] == r[f+k]) break;
        }
      //RoutingVec_it v = find(m_NVec.begin(), m_NVec.end(), r[f + k]);
      m_pBitMap->Set(k, ind);
    }
  m_FirstNonDefault = f;
  m_LastNonDefault = l;
}

nodeid_t BMLookup::Lookup(nodeid_t t)
{
nodeid_t r;

  if (t < m_FirstNonDefault) return(m_Default);
  if (t > m_LastNonDefault ) return(m_Default);
  if (!m_pBitMap) return(NODE_NONE);
  u_long i = m_pBitMap->Get(t - m_FirstNonDefault); // Lookup neighbor index
  r = m_NVec[i];
  if(0)printf("BLookup lookedup entry %ld ind %ld, found %ld\n",
         t - m_FirstNonDefault, i, r);
  return(r);
}

size_t   BMLookup::Size()
{
  return sizeof(nodeid_t) + //m_Default
         sizeof(nodeid_t) + // m_FirstNonDefault
         sizeof(nodeid_t) + // m_LastNonDefault)
         sizeof(BitMap*)  + // m_pBitMap)
         m_pBitMap->Size() + 
         sizeof(RoutingVec_t) + // m_NVec)
         sizeof(nodeid_t)*m_NVec.size();    // items in m_NVec
}

size_t   BMLookup::NumberEntries()
{
  return(m_LastNonDefault - m_FirstNonDefault -1 );
}

size_t   BMLookup::EstimateSize(
    RoutingVec_t& ,   // NextHop table
    RoutingVec_t& p,   // Population counts
    nodeid_t ,        // Default route
    nodeid_t ,        // Number default
    nodeid_t ,
    nodeid_t f,
    nodeid_t l)
{
unsigned int i;
unsigned int k = 0;
  // Count non-zero entries in pop vector
  for (i = 0; i < p.size(); i++)
    if (p[i]) k++;

  u_long c = l - f + 1; // How many entries we need
  return sizeof(nodeid_t) + //m_Default
         sizeof(nodeid_t) + // m_FirstNonDefault
         sizeof(nodeid_t) + // m_LastNonDefault)
         sizeof(BitMap*)  + // m_pBitMap)
         BitMap::EstimateSize(c, BitMap::FindBPE(k)) +
         sizeof(RoutingVec_t) + // m_NVec)
         sizeof(nodeid_t)*k;    // items in m_NVec
}

void BMLookup::Log( ostream& os)
{
RoutingVec_it i;

  os << " " << (int)WhatType();
  os << " " << m_Default;
  os << " " << m_FirstNonDefault;
  os << " " << m_LastNonDefault;
  os << " " << m_NVec.size();
  for (i = m_NVec.begin(); i != m_NVec.end(); i++)
    {
      os << " " << *i;
    }
  m_pBitMap->Log(os);
}

// Hashmap routing methods

HMLookup::HMLookup() : m_Default(NODE_NONE)
{
  if(0)printf("Created HMLookup\n");
}

HMLookup::~HMLookup()
{
}

RLookup_Types HMLookup::WhatType(void) const // Identifies the type of lookup
{
  return RL_HASH;
}

void HMLookup::Populate(
    RoutingVec_t& r, // NextHop table
    RoutingVec_t&,   // Population counts
    nodeid_t d,      // Default route
    nodeid_t,
    nodeid_t,
    nodeid_t)
{
  unsigned int i;

  m_Default = d; // Set the default route
  for (i = 0; i < r.size(); i++)
    {
      if (r[i] != d && r[i] != NODE_NONE)
        { // Not the default, create a HT entry
#ifdef CHANGED_DUE_TO_FREEBSD_PROB
          RoutePair_t* p = new RoutePair_t(i, r[i]);
          m_RouteMap.insert(*p);
#else
          RoutePair_t p = RoutePair_t(i, r[i]);
          m_RouteMap.insert(p);
#endif
        }
    }
}

nodeid_t HMLookup::Lookup(nodeid_t t)
{
  RouteMap_it i;

  i = m_RouteMap.find(t);
  if (i == m_RouteMap.end()) return(m_Default);
  return((*i).second);
}

size_t   HMLookup::Size( void )
{
  return sizeof(u_long) +      // m_Default
         sizeof(RouteMap_t) +  // m_RouteMap
         m_RouteMap.size() * sizeof(nodeid_t); // entries in hash table
}

size_t   HMLookup::NumberEntries()
{
  return(m_RouteMap.size());
}

size_t   HMLookup::EstimateSize(
    RoutingVec_t& r, // NextHop table
    RoutingVec_t& , // Population counts
    nodeid_t d,      // Default route
    nodeid_t ,      // Number default
    nodeid_t,
    nodeid_t,
    nodeid_t)
{
  return sizeof(u_long) +      // m_Default
         sizeof(RouteMap_t) +  // m_RouteMap
         (r.size() - d) * sizeof(nodeid_t); // entries in hash table
}

void HMLookup::Log( ostream& os)
{
RouteMap_it i;

  os << " " << (int)WhatType();
  os << " " << m_Default;
  for (i = m_RouteMap.begin(); i != m_RouteMap.end(); i++)
    {
      os << " " << (*i).first << " " << (*i).second;
    }
}

// NextHop (inefficient) routing methods

NHLookup::NHLookup()
{
  if(0)printf("Created NHLookup\n");
}

NHLookup::~NHLookup()
{
}

RLookup_Types NHLookup::WhatType(void) const // Identifies the type of lookup
{
  return RL_NEXTHOP;
}

void NHLookup::Populate(
    RoutingVec_t& r, // NextHop table
    RoutingVec_t&,   // Population counts
    nodeid_t ,      // Default route
    nodeid_t,
    nodeid_t,
    nodeid_t)
{
  unsigned int i;

  // Just copy the routing vector
  for (i = 0; i < r.size(); i++)
    {
      m_RouteTable.push_back(r[i]);
    }
}

void NHLookup::Populate(istream& is)
{ // Populate from log flie
int count;

  is >> count;
  for (int i = 0; i < count; i++)
    {
      int j;
      is >> j;
      nodeid_t n;
      n = (j < 0) ? NODE_NONE : (nodeid_t)j;
      m_RouteTable.push_back(n);
    }
}

nodeid_t NHLookup::Lookup(nodeid_t t)
{
  if (t >= m_RouteTable.size()) return(NODE_NONE);
  return(m_RouteTable[t]); // Just a table lookup
}

size_t   NHLookup::Size( void )
{
  return sizeof(u_long) * m_RouteTable.size();
}

size_t   NHLookup::NumberEntries()
{
  return(m_RouteTable.size());
}

void NHLookup::Log( ostream& os)
{
RoutingVec_it i;

  os << " " << (int)WhatType();
  os << " " << m_RouteTable.size();
  for (i = m_RouteTable.begin(); i != m_RouteTable.end(); i++)
    {
      if (*i == NODE_NONE)
        os << " " << -1; // The NODE_NONE representation is hard to read
      else
        os << " " << *i;
    }
}

size_t   NHLookup::EstimateSize(
    RoutingVec_t& r, // NextHop table
    RoutingVec_t&,   // Population counts
    nodeid_t ,      // Default route
    nodeid_t,
    nodeid_t,
    nodeid_t,
    nodeid_t)
{
  return sizeof(u_long) * r.size();
}

#endif /* STL */
