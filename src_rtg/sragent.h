/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 */
/*
 * Contributed by Rishi Bhargava <rishi_bhargava@yahoo.com> May, 2001.
 */


#ifndef _SRAGENT_H
#define _SRAGENT_H

#include <stdarg.h>

#include "object.h"
#include "agent.h"
#include "trace.h"
#include "packet.h"
#include "priqueue.h"
#include "mac.h"
#include "hdr_src.h"


#define SEND_TIMEOUT 30.0	// # seconds a packet can live in sendbuf
#define SEND_BUF_SIZE 64
//define MAXHOPS 24
class Agent;


class SRAgent : public Agent{
public:

  // in the table the first entry is the number of hops... then follows the path 
   
  h_node *table[10];
  
  
  int num_connection;
  void install(int slot, NsObject*);
  void alloc(int);
  NsObject **slot_;
  int nslot_;
  int maxslot_;
  
  virtual int command(int argc, const char*const* argv);
  virtual void recv(Packet*, Handler* callback = 0);


  SRAgent();
  ~SRAgent();

private:

  int off_src_;
  

  Agent *agent_;

};

#endif // _SRAGENT_H
