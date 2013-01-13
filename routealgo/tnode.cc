
/*
 * tnode.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: tnode.cc,v 1.5 2005/08/25 18:58:11 johnh Exp $
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
 * Test nodes for testing routing algorithms
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#include "config.h"
#ifdef HAVE_STL

#include <stdio.h>

#include "routealgo/tnode.h"

void Node::AddAdj( nodeid_t a, int w)
{
  Edge* pE;

  pE= new Edge(a, w);
  m_Adj.push_back(pE);
}

const NodeWeight_t Node::NextAdj( const NodeWeight_t& last)
{
Edge* pE;

  static EdgeVec_it prev;
  if (last.first == NODE_NONE)
    {
      prev = m_Adj.begin();
      pE = *prev;
      if(0)printf("NextAdj returning first edge %ld w %d\n",
             pE->m_n, pE->m_w);
      return(NodeWeight_t(pE->m_n, pE->m_w));
    }
  else
    { // See if last is prev
      if (last.first == (*prev)->m_n)
        { //Yep, just advance iterator
          prev++;
          if (prev == m_Adj.end())
            { // No more
              return(NodeWeight_t(NODE_NONE, 0));
            }
          else
            {
              pE = *prev;
              if(0)printf("NextAdj returning next edge %ld w %d\n",
                     pE->m_n, pE->m_w);
              return(NodeWeight_t(pE->m_n, pE->m_w));
            }
        }
      else
        { // Need to code this
          printf("Non-linear advance of NextAdj\n");
          exit(1);
        }
    }
}

#endif //HAVE_STL
