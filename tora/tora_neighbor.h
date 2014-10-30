/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *      This product includes software developed by the Computer Systems
 *      Engineering Group at Lawrence Berkeley Laboratory.
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
 */
/* Ported from CMU/Monarch's code*/

/* -*- c++ -*-
   tora_neighbor.h
   $Id: tora_neighbor.h,v 1.4 2006/02/21 15:20:20 mahrenho Exp $
  
   */
#ifndef __tora_neighbor_h__
#define __tora_neighbor_h__

class toraAgent;

enum LinkStatus {
	LINK_UP = 0x0001,	// upstream
	LINK_DN = 0x0002,	// downstream
	LINK_UN = 0x0004	// undirected
};


class TORANeighbor {
	friend class TORADest;
	friend class toraAgent;
public:
	TORANeighbor(nsaddr_t id, Agent *a); 
	void	update_link_status(Height *h);
	void	dump(void);
private:
	LIST_ENTRY(TORANeighbor) link;

	nsaddr_t	index;

	/*
	 *  height:   The height metric of this neighbor.
	 *  lnk_stat: The assigned status of the link to this neighbor.
	 *  time_act: Time the link to this neighbor became active.
	 */
	Height		height;
	LinkStatus	lnk_stat;
	double		time_act;

        toraAgent     *agent;
};

#endif /* __tora_neighbor_h__ */

