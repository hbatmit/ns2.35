/*
 * Copyright (c) 2006-2009 by the Protocol Engineering Lab, U of Delaware
 * All rights reserved.
 * 
 * Protocol Engineering Lab web page : http://pel.cis.udel.edu/
 *
 * Paul D. Amer        <amer@@cis,udel,edu>
 * Armando L. Caro Jr. <acaro@@cis,udel,edu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University nor of the Laboratory may be used
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/sctp/sctp-newreno.h,v 1.3 2009/11/16 05:51:27 tom_henderson Exp $ (UD/PEL)
 */

/* NewReno extension introduces a recover state variable per destination
 * similar to avoid multiple cwnd cuts in a single window (ie, round
 * trip). The concept is borrowed from TCP NewReno.
 */

#ifndef ns_sctp_newreno_h
#define ns_sctp_newreno_h

#include "sctp.h"

class NewRenoSctpAgent : public virtual SctpAgent 
{
public:
  NewRenoSctpAgent();
	
protected:
  virtual void  delay_bind_init_all();
  virtual int   delay_bind_dispatch(const char *varName, const char *localName,
				    TclObject *tracer);

  /* initialization stuff
   */
  virtual void   OptionReset();

  /* sending functions
   */
  virtual void  SendBufferDequeueUpTo(u_int);
  virtual void FastRtx();

  /* processing functions
   */
  virtual Boolean_E  ProcessGapAckBlocks(u_char *, Boolean_E);
  virtual void       ProcessSackChunk(u_char *);

  u_int            uiRecover;
  u_int            uiHighestTsnNewlyAcked;  /* global now for NewReno's HTNA */
};

#endif
