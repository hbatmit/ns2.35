/* -*-	Mode:C++; c-basic-offset:2; tab-width:2; indent-tabs-mode:f -*- */

/*
 * classifier-nix.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: classifier-nix.cc,v 1.7 2006/02/21 15:20:19 mahrenho Exp $
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
 * NixVector classifier for stateless routing
 * contributed to ns from 
 * George F. Riley, Georgia Tech, Spring 2000
 *
 */

#include "config.h"
#ifdef HAVE_STL


#include <stdio.h>

#include "ip.h"
#include "nixvec.h"
#include "classifier-nix.h"
#include "hdr_nv.h"

/* Define the TCL Class */
static class NixClassifierClass : public TclClass {
public:
	NixClassifierClass() : TclClass("Classifier/Nix") {}
	TclObject* create(int, const char*const*) {
		return (new NixClassifier());
	}
} class_nix_classifier;



/****************** NixClassifier Methods ************/
NsObject* NixClassifier::find(Packet* p)
{
	if (m_NodeId == NODE_NONE)
		{
			printf("NixClassifier::find(), Unknown node id\n");
			return(NULL);
		}
	NixNode* pN = NixNode::GetNodeObject(m_NodeId);
	hdr_ip* ip = hdr_ip::access(p);
	if ((nodeid_t)ip->daddr() == pN->Id())
		{ // Arrived at destination, pass on to the dmux object
			//			if(0)printf("Returning Dmux objet %p\n", m_Dmux);
			return m_Dmux;
		}
	hdr_nv* nv = hdr_nv::access(p);
	NixVec* pNv = nv->nv();
	if (pNv == NULL)
		{
			printf("Error! NixClassifier %s called with no NixVector\n", name());
			return(NULL);
		}
	if (pN == NULL)
		{
			printf("NixClassifier::find(), can't find node id %ld\n", m_NodeId);
			return(NULL);
		}
	Nix_t Nix = pNv->Extract(pN->GetNixl(), nv->used()); // Get the next nix
  nodeid_t n = pN->GetNeighbor(Nix,pNv); // this is just for debug print
  NsObject* nsobj = pN->GetNsNeighbor(Nix);
	if(0)printf("Classifier %s, Node %ld next hop %ld (%s)\n",
				 name(), pN->Id(), n, nsobj->name());
	return(nsobj); // And return the nexthop head object
}

int NixClassifier::command(int argc, const char*const* argv)
{
	if (argc == 3 && strcmp(argv[1],"set-node-id") == 0)
		{
			m_NodeId = atoi(argv[2]);
			return(TCL_OK);
		}
	if (argc == 4 && strcmp(argv[1],"install") == 0)
		{
			int target = atoi(argv[2]);
			NixNode* pN = NixNode::GetNodeObject(m_NodeId);
			if (pN)
				{
					if(0)printf("%s NixClassifier::Command node %ld install %s %s\n",
								 name(), pN->Id(), argv[2], argv[3]);
					if ((nodeid_t)target == pN->Id())
						{ // This is the only case we care about, need to note the dmux obj
							m_Dmux = (NsObject*)TclObject::lookup(argv[3]);
							//if(0)printf("m_Dmux obj is %p\n", m_Dmux);
						}
				}
			return(TCL_OK);
		}
	return (Classifier::command(argc, argv));
}
 
#endif // HAVE_STL
