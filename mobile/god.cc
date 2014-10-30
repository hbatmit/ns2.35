/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaim
er in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Header: /cvsroot/nsnam/ns-2/mobile/god.cc,v 1.20 2006/12/27 14:57:23 tom_henderson Exp $
 */

/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

/*
 * god.cc
 *
 * General Operations Director
 *
 * perform operations requiring omnipotence in the simulation
 *
 * NOTE: Tcl node indexs are 0 based, NS C++ node IP addresses (and the
 * node->index() are 1 based.
 *
 */

#include <object.h>
#include <packet.h>
#include <ip.h>
#include <god.h>
#include <sys/param.h>  /* for MIN/MAX */

#include "diffusion/hash_table.h"
#include "mobilenode.h"

God* God::instance_;

static class GodClass : public TclClass {
public:
        GodClass() : TclClass("God") {}
        TclObject* create(int, const char*const*) {
                return (new God);
        }
} class_God;


God::God()
{
        min_hops = 0;
        num_nodes = 0;

        data_pkt_size = 64;
	mb_node = 0;
	next_hop = 0;
	prev_time = -1.0;
	num_alive_node = 0;
	num_connect = 0;
	num_recv = 0;
	num_compute = 0;
	num_data_types = 0;
	source_table = 0;
	sink_table = 0;
	num_send = 0;
	active = false;
	allowTostop = false;
}


// Added by Chalermek 12/1/99

int God::NextHop(int from, int to)
{
  if (active == false) {
    perror("God is off.\n");
    exit(-1);
  }

  if (from >= num_nodes) {
    perror("index from higher than the maximum number of nodes.\n");
    return -1;
  }

  if (to >= num_nodes) {
    perror("index to higher than the maximum number of nodes.\n");
    return -1;
  }

  return NEXT_HOP(from,to);
}


void God::ComputeNextHop()
{
  if (active == false) {
    return;
  }

  int from, to, neighbor;

  for (from=0; from<num_nodes; from++) {
    for (to=0; to<num_nodes; to++) {

      NEXT_HOP(from,to) = UNREACHABLE;

      if (from==to) {
	NEXT_HOP(from,to) = from;     // next hop is itself.
      }

      if (MIN_HOPS(from, to) == UNREACHABLE) {
	continue;
      }

      for (neighbor=0; neighbor<num_nodes; neighbor++){
	if ( MIN_HOPS(from, neighbor) != 1) {
	  continue;
	}

	if ( MIN_HOPS(from, to) == (MIN_HOPS(neighbor,to) +1) ) {
	  NEXT_HOP(from, to) = neighbor;
	  break;
	}
      }

    }
  }
}


void God::UpdateNodeStatus()
{
  int i,j;
  int count, cur, sk, srcid, dt;

   for (i=0; i<num_data_types; i++) {
     for (j=0; j<num_nodes; j++) {
       if (SRC_TAB(i,j) != NULL) {
	 node_status[j].is_source_ = true;
       }
     }
   }

   for (i=0; i<num_data_types; i++) {
     for (j=0; j<num_nodes; j++) {
       if (SK_TAB(i,j) > 0) {
	 node_status[j].is_sink_ = true;
       }
     }
   }

   for (dt=0; dt < num_data_types; dt++) {
     for (srcid=0; srcid < num_nodes; srcid++) {
       if (SRC_TAB(dt,srcid) == NULL) 
	 continue;
       for (sk = 0; sk < num_nodes; sk++) {
	 if (SK_TAB(dt, sk) == 0)
	   continue;
	 cur = srcid;
	 count = 0;
	 node_status[cur].is_on_trees_ = true;
	 while (cur != sk) {
	   if (NextHop(cur, sk) == UNREACHABLE)
	     break;

	   assert(NextHop(cur,sk) >= 0 && NextHop(cur, sk) < num_nodes);

	   cur = NextHop(cur, sk);      
	   node_status[cur].is_on_trees_ = true;

	   count ++;
	   assert(count < num_nodes);
	 }
       }
     }
   }

   Dump();
   DumpNodeStatus();
}


void God::DumpNodeStatus()
{
  for (int i=0; i < num_nodes; i++) {
    printf("Node %d status (sink %d, source %d, on_tree %d)\n", i, 
	   node_status[i].is_sink_, node_status[i].is_source_, 
	   node_status[i].is_on_trees_);
  }
}

void God::DumpNumSend()
{
#ifdef DEBUG_OUTPUT
  for (int i=0; i < num_data_types; i++) {
    fprintf(stdout, "God: data type %d distinct events %d\n", i, num_send[i]);
  }
#endif
}


void God::Dump()
{
   int i, j, k, l;

   // Dump min_hops array

   fprintf(stdout,"Dump min_hops\n");
   for(i = 0; i < num_nodes; i++) {
      fprintf(stdout, "%2d) ", i);
      for(j = 0; j < num_nodes; j++)
          fprintf(stdout, "%2d ", min_hops[i * num_nodes + j]);
          fprintf(stdout, "\n");
  }

   // How many times the god compute routes ?

   fprintf(stdout, "God computes routes %d times.\n", num_compute);


   // The following information can be found only when god is active.

   if (active == false) {
     return;
   }

   // Dump next_hop array

   fprintf(stdout, "Dump next_hop\n");
   for (i = 0; i < num_nodes; i++) {
     for (j = 0; j < num_nodes; j++) {
       fprintf(stdout,"NextHop(%d,%d):%d\n",i,j,NEXT_HOP(i,j));
     }
   }


   // What is inside SRC_TAB ?

   fprintf(stdout, "Dump SRC_TAB\n");
   for (i=0; i<num_data_types; i++) {
     fprintf(stdout,"%2d) ",i);
     for (j=0; j<num_nodes; j++) {
       fprintf(stdout,"%2d ", SRC_TAB(i,j) ? 1:0);
     }
     fprintf(stdout,"\n");
   }


   // What is inside OIF_MAP ?

   int *oif_map;

   fprintf(stdout, "Dump OIF_MAP\n");
   for (i=0; i<num_data_types; i++) {
     for (j=0; j<num_nodes; j++) {
       if (SRC_TAB(i,j)!=NULL) {
	 oif_map = SRC_TAB(i,j);
	 fprintf(stdout,"(%2d,%2d)\n",i,j);
	 for (k=0; k<num_nodes; k++) {
	   for (l=0; l<num_nodes; l++) {
	     fprintf(stdout,"%2d ", oif_map[k*num_nodes +l]);
	   }
	   fprintf(stdout,"\n");
	 }
       }
     }
   }



   // What is inside SK_TAB ?

   fprintf(stdout, "Dump SK_TAB\n");
   for (i=0; i<num_data_types; i++) {
     fprintf(stdout,"%2d) ",i);
     for (j=0; j<num_nodes; j++) {
       fprintf(stdout,"%2d ", SK_TAB(i,j));
     }
     fprintf(stdout,"\n");
   }

}


void God::AddSink(int dt, int skid)
{
  if (active == false) {
    return;
  }

  assert(num_data_types > 0);
  assert(num_nodes > 0);
  assert(dt >= 0 && dt < num_data_types);
  assert(skid >= 0 && skid < num_nodes);

  if (SK_TAB(dt,skid) == 1)
     return;

  SK_TAB(dt,skid) = 1;
  Fill_for_Source(dt, skid);
}


void God::AddSource(int dt, int srcid)
{
  if (active == false) {
    return;
  }

  assert(num_data_types > 0);
  assert(num_nodes > 0);
  assert(dt >= 0 && dt < num_data_types);
  assert(srcid >= 0 && srcid < num_nodes);

  if (SRC_TAB(dt,srcid) != 0)
      return;

  SRC_TAB(dt,srcid) = new int[num_nodes * num_nodes];
  bzero((char*) SRC_TAB(dt, srcid), sizeof(int) * num_nodes * num_nodes);
  Fill_for_Sink(dt, srcid);
  //  Dump();
}


void God::Fill_for_Sink(int dt, int srcid)
{
  int sk, cur, count;
  int *oif_map = SRC_TAB(dt, srcid);

  assert(oif_map != NULL);

  for (sk = 0; sk < num_nodes; sk++) {
    if (SK_TAB(dt, sk) == 0)
      continue;
    cur = srcid;
    count = 0;
    while (cur != sk) {
      if (NextHop(cur, sk) == UNREACHABLE)
	break;

      assert(NextHop(cur,sk) >= 0 && NextHop(cur, sk) < num_nodes);

      oif_map[cur*num_nodes + NextHop(cur, sk)] = 1;
      cur = NextHop(cur, sk);      
      count ++;
      assert(count < num_nodes);
    }
  }
}


void God::Fill_for_Source(int dt, int skid)
{
  int src, cur, count;
  int *oif_map;

  for (src = 0; src < num_nodes; src++) {
    if (SRC_TAB(dt, src) == 0)
      continue;
   
    oif_map = SRC_TAB(dt, src);
    cur = src;
    count = 0;
    while (cur != skid) {
      if (NextHop(cur, skid) == UNREACHABLE)
	break;

      assert(NextHop(cur,skid) >= 0 && NextHop(cur, skid) < num_nodes);

      oif_map[cur*num_nodes + NextHop(cur, skid)] = 1;
      cur = NextHop(cur, skid);      
      count ++;
      assert(count < num_nodes);
    }

  }
}


void God::Rewrite_OIF_Map()
{
  for (int dt = 0; dt < num_data_types; dt++) {
    for (int src = 0; src < num_nodes; src++) {
      if (SRC_TAB(dt, src) == NULL)
	continue;

      memset(SRC_TAB(dt,src),'\x00', sizeof(int) * num_nodes * num_nodes);
      Fill_for_Sink(dt, src);
    }
  }
}


int *God::NextOIFs(int dt, int srcid, int curid, int *ret_num_oif) 
{

  if (active == false) {
    perror("God is inactive.\n");
    exit(-1);
  }  

  int *oif_map = SRC_TAB(dt, srcid);
  int count=0;
  int i;

  for (i=0; i<num_nodes; i++) {
    if (oif_map[curid*num_nodes +i] == 1)
      count++;
  }

  *ret_num_oif = count;

  if (count == 0)
    return NULL;

  int *next_oifs = new int[count];
  int j=0;
  
  for (i=0; i<num_nodes; i++) {
    if (oif_map[curid*num_nodes +i] == 1) {
      next_oifs[j] = i;
      j++;    
    }
  }

  return next_oifs;
}



bool God::IsReachable(int i, int j)
{

//  if (MIN_HOPS(i,j) < UNREACHABLE && MIN_HOPS(i,j) >= 0) 
  if (NextHop(i,j) != UNREACHABLE)
     return true;
  else
     return false;
}


bool God::IsNeighbor(int i, int j)
{
  assert(i<num_nodes && j<num_nodes);

  //printf("i=%d, j=%d\n", i,j);
  if (mb_node[i]->energy_model()->node_on() == false ||
      mb_node[j]->energy_model()->node_on() == false ||
      mb_node[i]->energy_model()->energy() <= 0.0 ||
      mb_node[j]->energy_model()->energy() <= 0.0 ) {
    return false;
  }

  vector a(mb_node[i]->X(), mb_node[i]->Y(), mb_node[i]->Z());
  vector b(mb_node[j]->X(), mb_node[j]->Y(), mb_node[j]->Z());
  vector d = a - b;

  if (d.length() < RANGE)
    return true;
  else
    return false;  
}


void God::CountConnect()
{
  int i,j;

  num_connect = 0;

  for (i=0; i<num_nodes; i++) {
    for (j=i+1; j<num_nodes; j++) {
      if (MIN_HOPS(i,j) != UNREACHABLE) {
	num_connect++;
      }
    }
  }
}


void God::CountAliveNode()
{
  int i;

  num_alive_node = 0;

  for (i=0; i<num_nodes; i++) {
    if (mb_node[i]->energy_model()->energy() > 0.0) {
      num_alive_node++;
    }
  }

}


bool God::ExistSource()
{
  int dtype, i;

  for (dtype = 0; dtype < num_data_types; dtype++) {
    for (i=0; i<num_nodes; i++) {
      if (SRC_TAB(dtype, i) != 0)
	return true;
    }
  }

  return false;
}


bool God::ExistSink()
{
  int dtype, i;

  for (dtype = 0; dtype < num_data_types; dtype++) {
    for (i=0; i<num_nodes; i++) {
      if (SK_TAB(dtype, i) != 0)
	return true;
    }
  }

  return false;
}


bool God::IsPartition()
{
  int dtype, i, j, k;
  int *oif_map;

  for (dtype = 0; dtype < num_data_types; dtype ++) {
    for (i = 0; i < num_nodes; i++) {
      if (SRC_TAB(dtype,i) == NULL)
	continue;
      oif_map = SRC_TAB(dtype, i);
      for (j = 0; j < num_nodes; j++) {
	for (k = 0; k < num_nodes; k++) {
	  if (oif_map[j*num_nodes + k] != 0)
	    return false;
	}
      }
    }
  }

  return true;
}


void God::ComputeRoute() 
{
  if (active == false) {
    return;
  }

  floyd_warshall();
  ComputeNextHop();
  Rewrite_OIF_Map();
  CountConnect();
  CountAliveNode();
  prev_time = NOW;
  num_compute++;

  if (allowTostop == false)
    return;

  if ( ExistSink() == true && ExistSource() == true && IsPartition() == true)
    StopSimulation();
}


void God::CountNewData(int *attr)
{
  if (dtab.GetHash(attr) == NULL) {
    num_send[attr[0]]++;
    dtab.PutInHash(attr);
  }
}


void God::IncrRecv()
{
  num_recv++;

  //  printf("God: num_connect %d, num_alive_node %d at recv pkt %d\n",
  // num_connect, num_alive_node, num_recv);
}


void God::StopSimulation() 
{
  Tcl& tcl=Tcl::instance();

  printf("Network parition !! Exiting... at time %f\n", NOW);
  tcl.evalf("[Simulator instance] at %lf \"finish\"", (NOW)+0.000001);
  tcl.evalf("[Simulator instance] at %lf \"[Simulator instance] halt\"", (NOW)+0.000002);
}


// Modified from setdest.cc -- Chalermek 12/1/99

void God::ComputeW()
{
  int i, j;
  int *W = min_hops;

  memset(W, '\xff', sizeof(int) * num_nodes * num_nodes);

  for(i = 0; i < num_nodes; i++) {
     W[i*num_nodes + i] = 0;     
     for(j = i+1; j < num_nodes; j++) {
	W[i*num_nodes + j] = W[j*num_nodes + i] = 
	                     IsNeighbor(i,j) ? 1 : INFINITY;
     }
  }
}

void God::floyd_warshall()
{
  int i, j, k;

  ComputeW();	// the connectivity matrix

  for(i = 0; i < num_nodes; i++) {
     for(j = 0; j < num_nodes; j++) {
	 for(k = 0; k < num_nodes; k++) {
	    MIN_HOPS(j,k) = MIN(MIN_HOPS(j,k), MIN_HOPS(j,i) + MIN_HOPS(i,k));
	 }
     }
  }


#ifdef SANITY_CHECKS

  for(i = 0; i < num_nodes; i++)
     for(j = 0; j < num_nodes; j++) {
	assert(MIN_HOPS(i,j) == MIN_HOPS(j,i));
	assert(MIN_HOPS(i,j) <= INFINITY);
     }
#endif

}

// --------------------------


int
God::hops(int i, int j)
{
        return min_hops[i * num_nodes + j];
}


void
God::stampPacket(Packet *p)
{
        hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        nsaddr_t src = ih->saddr();
        nsaddr_t dst = ih->daddr();

        assert(min_hops);

        if (!packet_info.data_packet(ch->ptype())) return;

        if (dst > num_nodes || src > num_nodes) return; // broadcast pkt
   
        ch->opt_num_forwards() = min_hops[src * num_nodes + dst];
}


void 
God::recv(Packet *, Handler *)
{
        abort();
}

int
God::load_grid(int x, int y, int size)
{
	maxX =  x;
	maxY =  y;
	gridsize_ = size;
	
	// how many gridx in X direction
	gridX = (int)maxX/size;
	if (gridX * size < maxX) gridX ++;
	
	// how many grid in Y direcion
	gridY = (int)maxY/size;
	if (gridY * size < maxY) gridY ++;

	printf("Grid info:%d %d %d (%d %d)\n",maxX,maxY,gridsize_,
	       gridX, gridY);

	return 0;
}
 
// return the grid that I am in
// start from left bottom corner, 
// from left to right, 0, 1, ...

int
God::getMyGrid(double x, double y)
{
	int xloc, yloc;
	
	if (x > maxX || y >maxY) return(-1);
	
	xloc = (int) x/gridsize_;
	yloc = (int) y/gridsize_;
	
	return(yloc*gridX+xloc);
}

int
God::getMyLeftGrid(double x, double y)
{

	int xloc, yloc;
	
	if (x > maxX || y >maxY) return(-1);
	
	xloc = (int) x/gridsize_;
	yloc = (int) y/gridsize_;

	xloc--;
	// no left grid
	if (xloc < 0) return (-2);
	return(yloc*gridX+xloc);
}

int
God::getMyRightGrid(double x, double y)
{

	int xloc, yloc;
	
	if (x > maxX || y >maxY) return(-1);
	
	xloc = (int) x/gridsize_;
	yloc = (int) y/gridsize_;

	xloc++;
	// no left grid
	if (xloc > gridX) return (-2);
	return(yloc*gridX+xloc);
}

int
God::getMyTopGrid(double x, double y)
{

	int xloc, yloc;
	
	if (x > maxX || y >maxY) return(-1);
	
	xloc = (int) x/gridsize_;
	yloc = (int) y/gridsize_;

	yloc++;
	// no top grid
	if (yloc > gridY) return (-2);
	return(yloc*gridX+xloc);
}

int
God::getMyBottomGrid(double x, double y)
{

	int xloc, yloc;
	
	if (x > maxX || y >maxY) return(-1);
	
	xloc = (int) x/gridsize_;
	yloc = (int) y/gridsize_;

	yloc--;
	// no top grid
	if (yloc < 0 ) return (-2);
	return(yloc*gridX+xloc);
}

int 
God::command(int argc, const char* const* argv)
{
	Tcl& tcl = Tcl::instance(); 
	if ((instance_ == 0) || (instance_ != this))
          	instance_ = this; 

        if (argc == 2) {

	        if(strcmp(argv[1], "update_node_status") == 0) {
		  UpdateNodeStatus();
		  return TCL_OK;
		}

	        if(strcmp(argv[1], "compute_route") == 0) {
		  ComputeRoute();
		  return TCL_OK;
		}

                if(strcmp(argv[1], "dump") == 0) {
		        Dump();
                        return TCL_OK;
                }
		
		if (strcmp(argv[1], "dump_num_send") == 0) {
		  DumpNumSend();
		  return TCL_OK;
		}

		if (strcmp(argv[1], "on") == 0) {
		  active = true;
		  return TCL_OK;
		}

		if (strcmp(argv[1], "off") == 0) {
		  active = false;
		  return TCL_OK;
		}

		if (strcmp(argv[1], "allow_to_stop") == 0) {
		  allowTostop = true;
		  return TCL_OK;
		}

		if (strcmp(argv[1], "not_allow_to_stop") == 0) {
		  allowTostop = false;
		  return TCL_OK;
		}

		/*
                if(strcmp(argv[1], "dump") == 0) {
                        int i, j;

                        for(i = 1; i < num_nodes; i++) {
                                fprintf(stdout, "%2d) ", i);
                                for(j = 1; j < num_nodes; j++)
                                        fprintf(stdout, "%2d ",
                                                min_hops[i * num_nodes + j]);
                                fprintf(stdout, "\n");
                        }
                        return TCL_OK;
                }
		*/

		if(strcmp(argv[1], "num_nodes") == 0) {
			tcl.resultf("%d", nodes());
			return TCL_OK;
		}
        }
        else if(argc == 3) {

	        if (strcasecmp(argv[1], "is_source") == 0) {
		  int node_id = atoi(argv[2]);

		  if (node_status[node_id].is_source_ == true) {
		    tcl.result("1");
		  } else {
		    tcl.result("0");
		  }
		  return TCL_OK;
	        }

	        if (strcasecmp(argv[1], "is_sink") == 0) {
		  int node_id = atoi(argv[2]);

		  if (node_status[node_id].is_sink_ == true) {
		    tcl.result("1");
		  } else {
		    tcl.result("0");
		  }
		  return TCL_OK;
	        }

	        if (strcasecmp(argv[1], "is_on_trees") == 0) {
		  int node_id = atoi(argv[2]);

		  if (node_status[node_id].is_on_trees_ == true) {
		    tcl.result("1");
		  } else {
		    tcl.result("0");
		  }
		  return TCL_OK;
	        }

                if (strcasecmp(argv[1], "num_nodes") == 0) {
                        assert(num_nodes == 0);

                        // index always starts from 0
                        num_nodes = atoi(argv[2]);

			assert(num_nodes > 0);
			
			printf("num_nodes is set %d\n", num_nodes);
			
                        min_hops = new int[num_nodes * num_nodes];
			mb_node = new MobileNode*[num_nodes];
			node_status = new NodeStatus[num_nodes];
			next_hop = new int[num_nodes * num_nodes];

                        bzero((char*) min_hops,
                              sizeof(int) * num_nodes * num_nodes);
			bzero((char*) mb_node,
			      sizeof(MobileNode*) * num_nodes);
			bzero((char*) next_hop,
			      sizeof(int) * num_nodes * num_nodes);

                        instance_ = this;

                        return TCL_OK;
                }

		if (strcasecmp(argv[1], "num_data_types") == 0) {
		  assert(num_data_types == 0);

                  num_data_types = atoi(argv[2]);

		  assert(num_nodes > 0);
		  assert(num_data_types > 0);
			
                  source_table = new int*[num_data_types * num_nodes];
		  sink_table = new int[num_data_types * num_nodes];
		  num_send = new int[num_data_types];

                  bzero((char*) source_table,
                              sizeof(int *) * num_data_types * num_nodes);
                  bzero((char*) sink_table,
                              sizeof(int) * num_data_types * num_nodes);
		  bzero((char*) num_send, sizeof(int) * num_data_types);

		  return TCL_OK;
		}

		if (strcasecmp(argv[1], "new_node") == 0) {
		  assert(num_nodes > 0);
		  MobileNode *obj = (MobileNode *)TclObject::lookup(argv[2]);
		  assert(obj != 0);
		  assert(obj->address() < num_nodes);

		  mb_node[obj->address()] = obj; 
		  return TCL_OK;
		}

		/*
                if (strcasecmp(argv[1], "num_nodes") == 0) {
                        assert(num_nodes == 0);

                        // allow for 0 based to 1 based conversion
                        num_nodes = atoi(argv[2]) + 1;

                        min_hops = new int[num_nodes * num_nodes];
                        bzero((char*) min_hops,
                              sizeof(int) * num_nodes * num_nodes);

                        instance_ = this;

                        return TCL_OK;
                }
		*/

        }
	else if (argc == 4) {

	  if (strcasecmp(argv[1], "is_reachable") == 0) {
	    int n1 = atoi(argv[2]);
	    int n2 = atoi(argv[3]);

	    if (IsReachable(n1,n2) == true) {
	      tcl.result("1");
	    } else {
	      tcl.result("0");
	    }

	    return TCL_OK;
	  }


	  // We can add source from tcl script or call AddSource directly.

	  if (strcasecmp(argv[1], "add_source") == 0) {
	    int dt = atoi(argv[2]);
	    int srcid = atoi(argv[3]);
	    
	    AddSource(dt, srcid);
	    return TCL_OK;
	  }

	  // We can add sink from tcl script or call AddSink directly.

	  if (strcasecmp(argv[1], "add_sink") == 0) {
	    int dt = atoi(argv[2]);
	    int skid = atoi(argv[3]);
	    
	    AddSink(dt, skid);
	    return TCL_OK;
	  }

	}
        else if(argc == 5) {

		/* load for grid-based adaptive fidelity */
		if (strcmp(argv[1], "load_grid") == 0) {
			if(load_grid(atoi(argv[2]), atoi(argv[3]), atoi(argv[4])))
				return TCL_ERROR;
			return TCL_OK;
		}

                if (strcasecmp(argv[1], "set-dist") == 0) {
                        int i = atoi(argv[2]);
                        int j = atoi(argv[3]);
                        int d = atoi(argv[4]);

                        assert(i >= 0 && i < num_nodes);
                        assert(j >= 0 && j < num_nodes);

			if (active == true) {
			  if (NOW > prev_time) {
			    ComputeRoute();
			  }
			}
			else {
			  min_hops[i*num_nodes+j] = d;
			  min_hops[j*num_nodes+i] = d;
			}

			// The scenario file should set the node positions
			// before calling set-dist !!

			assert(min_hops[i * num_nodes + j] == d);
                        assert(min_hops[j * num_nodes + i] == d);
                        return TCL_OK;
                }

		/*
                if (strcasecmp(argv[1], "set-dist") == 0) {
                        int i = atoi(argv[2]);
                        int j = atoi(argv[3]);
                        int d = atoi(argv[4]);

                        assert(i >= 0 && i < num_nodes);
                        assert(j >= 0 && j < num_nodes);

                        min_hops[i * num_nodes + j] = d;
                        min_hops[j * num_nodes + i] = d;
                        return TCL_OK;
                }
		*/

        } 
        return BiConnector::command(argc, argv);
}








