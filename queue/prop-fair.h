#ifndef QUEUE_PROP_FAIR_HH
#define QUEUE_PROP_FAIR_HH

#include <string.h>
#include "link-aware-queue.h"
#include "config.h"
#include <map>
#include <vector>

/*
 * Proportionally Fair QDISC
 */

class PropFair : public LinkAwareQueue {
  public:
    PropFair();

    /* Override backlogged_flowids from LinkAwareQueue */
    virtual std::vector<uint64_t> backlogged_flowids( void ) const override;

    /* Override get_current_link_rates from LinkAwareQueue */
    virtual std::map<uint64_t,double> get_current_link_rates( void ) const override; 

  protected:
    /* Underlying per flow FIFOs and enque wrapper */
    std::map<uint64_t,PacketQueue*> _packet_queues;
    void enque_packet( Packet* p, uint64_t flow_id );

    /* Hash from packet to flow */
    uint64_t hash( Packet *p ) { return ( hdr_ip::access(p) )->flowid(); };

    /* Override Enque and deque functions from Queue */
    virtual void enque(Packet*) override;
    virtual Packet* deque() override;

    /* Tcl cmd line interface */
    virtual int command( int argc, const char*const* argv ) override;
};

#endif
