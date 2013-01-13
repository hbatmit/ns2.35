/* -*- c++ -*-
   packet-stamp.h
   $Id: packet-stamp.h,v 1.3 1999/03/13 03:52:58 haoboy Exp $

   Information carried by a packet to allow a receive to decide if it
   will recieve the packet or not.

*/

#ifndef _cmu_packetstamp_h_
#define _cmu_packetstamp_h_

class MobileNode;
/* to avoid a pretty wild circular dependence among header files
   (between packet.h and queue.h), I can't do the #include here:
   #include <cmu/node.h>
   Since PacketStamp is just a container class, it doesn't really matter .
   -dam 8/8/98
   */
#include <antenna.h>

class PacketStamp {
public:

  PacketStamp() : ant(0), node(0), Pr(-1), lambda(-1) { }

  void init(const PacketStamp *s) {
	  Antenna* ant;
	  if (s->ant != NULL)
		  ant = s->ant->copy();
	  else
		  ant = 0;
	  
	  //Antenna *ant = (s->ant) ? s->ant->copy(): 0;
	  stamp(s->node, ant, s->Pr, s->lambda);
  }

  void stamp(MobileNode *n, Antenna *a, double xmitPr, double lam) {
    ant = a;
    node = n;
    Pr = xmitPr;
    lambda = lam;
  }

  inline Antenna * getAntenna() {return ant;}
  inline MobileNode * getNode() {return node;}
  inline double getTxPr() {return Pr;}
  inline double getLambda() {return lambda;}

  /* WILD HACK: The following two variables are a wild hack.
     They will go away in the next release...
     They're used by the mac-802_11 object to determine
     capture.  This will be moved into the net-if family of 
     objects in the future. */
  double RxPr;			// power with which pkt is received
  double CPThresh;		// capture threshold for recving interface

protected:
  Antenna       *ant;
  MobileNode	*node;
  double        Pr;		// power pkt sent with
  double        lambda;         // wavelength of signal
};

#endif /* !_cmu_packetstamp_h_ */
