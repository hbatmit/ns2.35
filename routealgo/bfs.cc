
/*
 * bfs.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: bfs.cc,v 1.6 2006/02/21 15:20:19 mahrenho Exp $
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
 * Implementation of Breadth First Search algorithm
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 *
 */

#include "config.h"
#ifdef HAVE_STL
#include "routealgo/bfs.h"
#include "routealgo/routealgo.h"
#include "routealgo/rnode.h"
#include "routealgo/tnode.h"
#include "routealgo/rbitmap.h"
#include <stdio.h>

#ifndef TEST_BFS

void BFS(
         RNodeVec_t& N,
         nodeid_t root,
         RoutingVec_t& NextHop,
         RoutingVec_t& Parent)
{  // Compute shortest path to all nodes from node S using BreadthFirstSearch
BitMap B(N.size()); // Make a bitmap for the colors (grey/white) (white = 0)
RNodeDeq_t Q;       // And a vector for the Q list
DistVec_t  D;       // And the distance vector

 // Fill in all "NONE" in the next hop neighbors and predecessors
 NextHop.erase(NextHop.begin(), NextHop.end());
 Parent.erase(Parent.begin(), Parent.end());
 for (unsigned int i = 0; i < N.size(); i++)
   {
     NextHop.push_back(NODE_NONE);
     Parent.push_back(NODE_NONE);
     D.push_back(INF);
     // Debug...print adj lists
     NodeWeight_t v(NODE_NONE, INF);
     //     if(0)printf("Printing adj for node %ld (addr %p)\n", N[i]->m_id, N[i]);
     // if(0)while(1)
     //        {
     //          v = N[i]->NextAdj(v);
     //          if (v.first == NODE_NONE) break;
     //          if(0)printf("Found adj %ld\n", v.first);
     //        }
   }
 B.Set(root); // Color the root grey
 if(0)B.DBPrint();
 Q.push_back(N[root]); // And put the root in Q
 D[root] = 0;
 while(Q.size() != 0)
   {
     RNodeDeq_it it = Q.begin();
     NodeWeight_t v(NODE_NONE, INF);
     RNode* u = *it;
     //if(0)printf("Working on node %ld addr %p\n", u->m_id, u);
     while(1)
       {
         v = u->NextAdj(v);
         if (v.first == NODE_NONE) break;
         if(0)printf("Found adj %ld\n", v.first);
         if (B.Get(v.first) == 0)
           { // White
             Q.push_back(N[v.first]);     // Add to Q set
             B.Set(v.first);              // Change to grey
             D[v.first] = D[u->m_id] + 1; // Set new distance
             Parent[v.first] = u->m_id;   // Set parent
             if (u->m_id == root)
               { // First hop is new node since this is root
                 NextHop[v.first] = v.first;
               }
             else
               { // First hop is same as this one
                 NextHop[v.first] = NextHop[u->m_id];
               }
             if(0)printf("Enqueued %ld\n", v.first);
           }
       }
     Q.pop_front();
   }
}
#endif

#ifdef TEST_BFS
RNodeVec_t Nodes;

int main()
{
  // See the sample BFS search in Fig23.3, p471 CLR Algorithms book
Node N0(0);
Node N1(1);
Node N2(2);
Node N3(3);
Node N4(4);
Node N5(5);
Node N6(6);
Node N7(7);
RoutingVec_t NextHop;
RoutingVec_t Parent;

 N0.AddAdj(1);
 N0.AddAdj(2);

 N1.AddAdj(0);

 N2.AddAdj(0);
 N2.AddAdj(3);

 N3.AddAdj(2);
 N3.AddAdj(4);
 N3.AddAdj(5);

 N4.AddAdj(3);
 N4.AddAdj(5);
 N4.AddAdj(6);

 N5.AddAdj(4);
 N5.AddAdj(7); 

 N6.AddAdj(4);
 N6.AddAdj(7);

 N7.AddAdj(5);
 N7.AddAdj(6);

 Nodes.push_back(&N0);
 Nodes.push_back(&N1);
 Nodes.push_back(&N2);
 Nodes.push_back(&N3);
 Nodes.push_back(&N4);
 Nodes.push_back(&N5);
 Nodes.push_back(&N6);
 Nodes.push_back(&N7);

 for (nodeid_t i = 0; i < Nodes.size(); i++)
   { // Get shortest path for each root node
     printf("\nFrom root %ld\n", i);
     BFS(Nodes, i, NextHop, Parent);
     PrintParents(Parent);
     for (unsigned int k = 0; k < Nodes.size(); k++)
       printf("Next hop for node %d is %ld\n", k, NextHop[k]);
     printf("Printing paths\n");
     for (nodeid_t j = 0; j < Nodes.size(); j++)
       {
         PrintRoute(i, j, Parent);
       }
   }
 return(0);
}
#endif

#endif //HAVE_STL
