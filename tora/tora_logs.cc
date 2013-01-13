/*
  $Id: tora_logs.cc,v 1.3 2001/05/21 19:27:34 haldar Exp $
  */

#include <agent.h>
#include <random.h>
#include <trace.h>

#include <ll.h>
#include <priqueue.h>
#include <tora/tora_packet.h>
#include <tora/tora.h>

#define CURRENT_TIME    Scheduler::instance().clock()

static const int verbose = 0;

/* ======================================================================
   Logging Functions
   ====================================================================== */
void
toraAgent::log_route_loop(nsaddr_t prev, nsaddr_t next)
{
        if(! logtarget || ! verbose ) return;

        sprintf(logtarget->pt_->buffer(),
                "T %.9f _%d_ routing loop (%d --> %d --> %d)",
                CURRENT_TIME, ipaddr(), prev, ipaddr(), next);
        logtarget->pt_->dump();
}

void
toraAgent::log_link_layer_feedback(Packet *p)
{
        static int link_layer_feedback = 0;
        struct hdr_cmn *ch = HDR_CMN(p);

        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "T %.9f _%d_ LL unable to deliver packet %d to %d (%d) (reason = %d, ifqlen = %d)",
                CURRENT_TIME,
                ipaddr(),
                ch->uid_,
                ch->next_hop_,
                ++link_layer_feedback,
                ch->xmit_reason_,
                ifqueue->length());

	logtarget->pt_->dump();
}


void
toraAgent::log_link_layer_recycle(Packet *p)
{
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);

        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "T %.9f _%d_ recycling packet %d (src = %d, dst = %d, prev = %d, next = %d)",
                CURRENT_TIME,
                ipaddr(),
                ch->uid_,
		//                ih->src_, ih->dst_,
		ih->saddr(),ih->daddr(),
                ch->prev_hop_, ch->next_hop_);
        logtarget->pt_->dump();
}
        
void
toraAgent::log_lnk_del(nsaddr_t dst)
{
        static int link_del = 0;

        if(! logtarget || ! verbose) return;

        /*
         *  If "god" thinks that these two nodes are still
         *  reachable then this is an erroneous deletion.
         */
        sprintf(logtarget->pt_->buffer(),
                "T %.9f _%d_ deleting LL hop to %d (delete %d is %s)",
                CURRENT_TIME,
                ipaddr(),
                dst,
                ++link_del,
                God::instance()->hops(ipaddr(), dst) != 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}

void
toraAgent::log_lnk_kept(nsaddr_t dst)
{
        static int link_kept = 0;

        if(! logtarget || ! verbose) return;

        /*
         *  If "god" thinks that these two nodes are now
         *  unreachable, then we are erroneously keeping
         *  a bad route.
         */
        sprintf(logtarget->pt_->buffer(),
                "T %.9f _%d_ keeping LL hop to %d (keep %d is %s)",
                CURRENT_TIME,
                ipaddr(),
                dst,
                ++link_kept,
                God::instance()->hops(ipaddr(), dst) == 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}

void
toraAgent::log_nb_del(nsaddr_t dst, nsaddr_t id)
{
        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "T %.9f _%d_ destination %d removing neighbor %d",
                CURRENT_TIME,
                ipaddr(),
                dst, id);
        logtarget->pt_->dump();
}

void
toraAgent::log_recv_qry(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_qry *qh = HDR_TORA_QRY(p);

	if(! logtarget || ! verbose) return;

	sprintf(logtarget->pt_->buffer(),
		"T %.9f %d received `QRY` from %d --- %d",
		CURRENT_TIME, ipaddr(), ih->saddr(), qh->tq_dst);
	logtarget->pt_->dump();
}

void
toraAgent::log_recv_upd(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_upd *uh = HDR_TORA_UPD(p);

	if(! logtarget || ! verbose) return;

	sprintf(logtarget->pt_->buffer(),
		"T %.9f _%d_ received `UPD` from %d --- %d (%f %d %d %d %d)",
		CURRENT_TIME,
                ipaddr(),
		// ih->src_, uh->tu_dst,
		ih->saddr(), uh->tu_dst,
		uh->tu_tau, uh->tu_oid, uh->tu_r, uh->tu_delta, uh->tu_id);
	logtarget->pt_->dump();
}

void
toraAgent::log_recv_clr(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_clr *ch = HDR_TORA_CLR(p);

	if(! logtarget || ! verbose) return;

	sprintf(logtarget->pt_->buffer(),
		"T %.9f _%d_ received `CLR` from %d --- %d (%f %d)",
		CURRENT_TIME,
                ipaddr(),
                // ih->src_,
		ih->saddr(),
		ch->tc_dst, ch->tc_tau, ch->tc_oid);
	logtarget->pt_->dump();
}


void 
toraAgent::log_route_table()
{
	TORADest *td;
	TORANeighbor *tn;

        if (!logtarget || ! verbose) return;

	for(td = dstlist.lh_first; td; td = td->link.le_next) {
		tn = td->nb_find_next_hop();

		sprintf(logtarget->pt_->buffer(),
			"T %.9f _%d_ %2d (%9f %2d %2d %2d %2d) ---> %2d (%9f %2d %2d %2d %2d) %d %.9f --- (%2d a, %2d d, %2d u) %d %9f",
			CURRENT_TIME,
                        ipaddr(),
			td->index,
			td->height.tau, td->height.oid, td->height.r,
			td->height.delta, td->height.id,
			tn ? tn->index : -1,
			tn ? tn->height.tau : -1.0,
			tn ? tn->height.oid : -1,
			tn ? tn->height.r : -1,
			tn ? tn->height.delta : -1,
			tn ? tn->height.id: -1,
                        tn ? tn->lnk_stat : -1,
                        tn ? tn->time_act : -1.0,
			td->num_active, td->num_down, td->num_up,
			td->rt_req, td->time_upd);

		logtarget->pt_->dump();
	}

	sprintf(logtarget->pt_->buffer(),
		"T --------------------------------------------------");
	logtarget->pt_->dump();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void
toraAgent::logToraDest(TORADest *td)
{
	if(! verbose) return;

	assert(td);

	sprintf(logtarget->pt_->buffer(),
		"T %.9f _%d_ TD %2d (%9f %2d %2d %2d %2d) --- (%2d a, %2d d, %2d u) %d %9f",
			CURRENT_TIME,
                        ipaddr(),
			td->index,
			td->height.tau, td->height.oid, td->height.r,
			td->height.delta, td->height.id,
			td->num_active, td->num_down, td->num_up,
			td->rt_req, td->time_upd);
	logtarget->pt_->dump();
}


void
toraAgent::logToraNeighbor(TORANeighbor *tn)
{
	if(! verbose) return;

	assert(tn);

	sprintf(logtarget->pt_->buffer(),
		"T %.9f _%d_ TN %2d (%.9f %2d %2d %2d %2d) %d %.9f",
		CURRENT_TIME,
		ipaddr(),
		tn->index,
		tn->height.tau,
		tn->height.oid,
		tn->height.r,
		tn->height.delta,
		tn->height.id,
		tn->lnk_stat,
		tn->time_act);
	logtarget->pt_->dump();
}

void
toraAgent::logNextHopChange(TORADest *td)
{
	if(! verbose) return;

	TORANeighbor *n;

	assert(td);

	logToraDest(td);

	for(n = td->nblist.lh_first; n; n = n->link.le_next)
		logToraNeighbor(n);

	n = td->nb_find_next_hop();
	if(n) {
		sprintf(logtarget->pt_->buffer(), "T %.9f _%d_ nexthop for %d is %d", 
			CURRENT_TIME, ipaddr(), td->index, n->index);
		logtarget->pt_->dump();
	}

	sprintf(logtarget->pt_->buffer(),
		"T %.9f _%d_ --------------------------------------------------",
		CURRENT_TIME,
		ipaddr());
	logtarget->pt_->dump();
}

void
toraAgent::logNbDeletedLastDN(TORADest *td)
{
	if(! verbose) return;

	sprintf(logtarget->pt_->buffer(), "T %.9f _%d_ lost last downstream link for destination %d",
		CURRENT_TIME,
		ipaddr(),
		td->index);
	logtarget->pt_->dump();

	logNextHopChange(td);
}

