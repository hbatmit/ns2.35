
/*
 * routealgo.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: routealgo.cc,v 1.5 2005/08/25 18:58:11 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*
 * Generic helpers (all routing algorithms)
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#include "config.h"
#ifdef HAVE_STL

#include <stdio.h>

#include "routealgo/rnode.h"

// Forward declarations

static void PrintPred(nodeid_t, nodeid_t, RoutingVec_t&);
static void NixPred(nodeid_t, nodeid_t, RoutingVec_t&, RNodeVec_t&, NixVec&);

// Globally visible routines

void PrintRoute( nodeid_t s, nodeid_t d, RoutingVec_t& pred)
{ // Print the route, given source s, destination d and predecessor vector pred
  printf("Route to node %ld is : ", d);
  PrintPred(s, d, pred);
  printf("\n");
}

// Create the NixVector
void NixRoute( nodeid_t s, nodeid_t d, RoutingVec_t& pred,
               RNodeVec_t& nodes, NixVec& nv)
{
  NixPred(s, d, pred, nodes, nv);
}


// Print the parent list (debug)
void PrintParents(RoutingVec_t& pred)
{
  printf("Parent vector is");
  for (RoutingVec_it i = pred.begin(); i != pred.end(); i++)
    {
      printf(" %ld", *i);
    }
  printf("\n");
}

// Print the route from the nixvector (debug)
void PrintNix(nodeid_t s, RNodeVec_t nodes, NixVec& nv)
{
  printf("Printing Nv from %ld\n", s);
  while(1)
    {
      printf("%ld\n", s);
      fflush(stdout);
      if (!nodes[s])
        {
          printf("PrintNix Error, no node for %ld\n", s);
          break;
        }
      printf("Node %ld nixl %ld\n", s, nodes[s]->GetNixl());
      Nix_t n = nv.Extract(nodes[s]->GetNixl());
      if (n == NIX_NONE) break; // Done
      nodeid_t s1 = nodes[s]->GetNeighbor(n);
      if (s1 == NODE_NONE)
        {
          printf("Huh?  Node %ld can't get neighbor %ld\n", s, n);
          break;
        }
      s = s1;
    }
  printf("\n");
  nv.Reset();
}



// helpers
static void PrintPred(nodeid_t s, nodeid_t p, RoutingVec_t& pred)
{
  if (s == p)
    {
      printf(" %ld", s);
    }
  else
    {
      if (pred[p] == NODE_NONE)
        {
          printf(" No path..");
        }
      else
        {
          PrintPred(s, pred[p], pred);
          printf(" %ld", p);
        }
    }
}

static void NixPred(nodeid_t s, nodeid_t p, RoutingVec_t& pred,
                    RNodeVec_t& nodes, NixVec& nv)
{
  if (s == p)
    {
      return; // Got there
    }
  else
    {
      if (pred[p] == NODE_NONE)
        {
          if(0)printf(" No path..");
          return;
        }
      else
        {
          NixPred(s, pred[p], pred, nodes, nv);
          NixPair_t n = nodes[pred[p]]->GetNix(p);
          if (n.first == NIX_NONE)
            { // ! Error.
              printf("RouteAlgo::GetNix returned NIX_NONE!\n");
              exit(1);
            }
          if(0)printf("NVAdding ind %ld lth %ld\n", n.first, n.second);
          nv.Add(n);
        }
    }
}

#endif /* STL */
