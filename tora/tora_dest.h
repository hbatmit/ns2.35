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

#ifndef __tora_dest_h__
#define __tora_dest_h__

class TORANeighbor;
class toraAgent;

LIST_HEAD(tn_head, TORANeighbor);

class TORADest {
	friend class toraAgent;
public:
	TORADest(nsaddr_t id, Agent *a);

	TORANeighbor*	nb_add(nsaddr_t id);
	int		nb_del(nsaddr_t id);
	TORANeighbor*	nb_find(nsaddr_t id);

	void		update_height_nb(TORANeighbor* tn,
					 struct hdr_tora_upd *uh);
	void		update_height(double TAU, nsaddr_t OID,
					int R, int DELTA, nsaddr_t ID);

	int		nb_check_same_ref(void);

	/*
	 *  Finds the minimum height neighbor whose lnk_stat is DN.
	 */
	TORANeighbor*	nb_find_next_hop();

	TORANeighbor*	nb_find_height(int R);
	TORANeighbor*	nb_find_max_height(void);
	TORANeighbor*	nb_find_min_height(int R);
	TORANeighbor*	nb_find_min_nonnull_height(Height *h);

	void		dump(void);

private:
	LIST_ENTRY(TORADest) link;

	nsaddr_t	index;		// IP address of destination
	Height		height;		// 5-tuple height of this node
	int		rt_req;		// route required flag
	double		time_upd;	// time last UPD packet was sent

	double		time_tx_qry;	// time sent last QUERY
	double		time_rt_req;	// time rt_req last set - JGB 

	tn_head		nblist;		// List of neighbors.
	int		num_active;	// # active links
	int		num_down;	// # down stream links
	int		num_up;		// # up stream links

        toraAgent     *agent;
};

#endif // __tora_dest_h__

