#include "sfd.h"
#include <math.h>
#include "rng.h"
#include <stdint.h>
#include <algorithm>
#include <float.h>

static class SFDClass : public TclClass {
  public:
    SFDClass() : TclClass("Queue/SFD") {}
    TclObject* create(int, const char*const*) {
      return (new SFD(0));
    }
} class_sfd;

int SFD::command(int argc, const char*const* argv) 
{
  if (argc == 3) {
    if (!strcmp(argv[1], "capacity")) {
     _capacity=atof(argv[2]);
     return (TCL_OK);
    }
  }
  return Queue::command(argc, argv);
}

FlowStats::FlowStats() :
  _last_arrival( 0.0 ),
  _acc_pkt_size( 0.0 ),
  _flow_rate( 0.0 )
{}

SFD::SFD( double capacity ) :
  _capacity( capacity )
{ 
  bind( "_capacity", &_capacity );
  _packet_queue = new PacketQueue;
}

void SFD::enque(Packet *p)
{
  /* Compute flow_id using hash */
  uint64_t flow_id = hash( p ); 
  printf( " Packet hashes into flow id %lu \n", flow_id );

  /* Estimate arrival rate */
  double now = Scheduler::instance().clock();
  double arrival_rate = est_flow_rate( flow_id, now, p );
  printf( " Arrival rate estimate is %f \n", arrival_rate );

  /* Estimate fair share */
  _fair_share = est_fair_share() ;
  printf( " Fair share estimate is %f\n", _fair_share );

  double drop_probability = std::max( 0.0 , 1 - _fair_share/arrival_rate );

  /* Toss a coin and drop, for UDP */  
  if ( !should_drop( drop_probability ) ) {
    printf( " Not dropping packet, drop_probability is %f\n", drop_probability );
    _packet_queue->enque( p );
  } else {
    printf( " Dropping packet, drop_probability is %f\n", drop_probability );
    drop( p );
  }
}

Packet* SFD::deque()
{
  /* For now, simply do FIFO */
  return _packet_queue->deque();
}

uint64_t SFD::hash(Packet* pkt)
{
  hdr_ip *iph=hdr_ip::access(pkt);
  uint64_t i;
  i = (uint64_t)iph->saddr();
  return ((i + (i >> 8) + ~(i>>4)) % ((2<<23)-1))+1; // modulo a large prime
}

double SFD::est_flow_rate( uint64_t flow_id, double now, Packet *p ) 
{
  /* Extract packet length in bits from the header */
  double interarrival_time = now - _flow_stats[ flow_id ]._last_arrival;
  hdr_cmn* hdr  = hdr_cmn::access(p); 
  uint32_t packet_size   = hdr->size() << 3;
 
  /* If you have simultaneous arrivals, coalesce packets */ 
  if ( interarrival_time == 0 ) {
    _flow_stats[ flow_id ]._acc_pkt_size += packet_size;
    if ( _flow_stats[ flow_id ]._flow_rate ) {
      return  _flow_stats[ flow_id ]._flow_rate ;
    } else {
      /* first packet, init. rate */
      return ( _flow_stats[ flow_id ]._flow_rate = _fair_share );
    }
  } else {
    packet_size += _flow_stats[ flow_id ]._acc_pkt_size;
    _flow_stats[ flow_id ]._acc_pkt_size = 0;
  }

  /* Apply EWMA with exponential weight, and update _last_arrival */
  _flow_stats[ flow_id ]._last_arrival = now;
  _flow_stats[ flow_id ]._flow_rate = 
    (1.0 - exp(-interarrival_time/K))*(double )packet_size/interarrival_time + exp(-interarrival_time/K)*_flow_stats[ flow_id ]._flow_rate;

  return _flow_stats[ flow_id ]._flow_rate;
}

double SFD::est_fair_share() 
{
  /* Estimate fair share rate based on desired individual rates */
  std::map<uint64_t,double> desired;
  std::map<uint64_t,double> current_share;
  std::map<uint64_t,double> final_share;

  for ( auto it = _flow_stats.begin(); it != _flow_stats.end(); it++ ) {
    auto flow_id = it->first;
    auto flow_rate = it->second._flow_rate;
    desired[ flow_id ] = flow_rate;
    current_share[ flow_id ] = 0;
  }
  auto capacity = _capacity ;

  /* First, determine all the allocations */
  while ( (!current_share.empty()) and (capacity > DBL_MIN ) ) {
    auto total_shares = current_share.size();
    auto unit_share   = capacity / total_shares ;
    std::list<uint64_t> user_list;
    for ( auto it = current_share.begin(); it != current_share.end(); it++ )
      user_list.push_back( it->first );
    for ( auto &user : user_list ) {
      auto fair_share = unit_share ; 
      current_share[ user ] += fair_share;
      if ( current_share[ user ] >= desired[ user ] ) {
        auto spare_capacity = current_share[user] - desired[ user ];
        final_share[ user ] = desired[ user ];
        current_share.erase( current_share.find( user ) );
        capacity = capacity + spare_capacity - fair_share ;
      } else {
        capacity = capacity - fair_share;
      }
    }
  }

  /* finalize allocations */
  for ( auto &user_share : current_share ) {
    final_share[ user_share.first ] = current_share[ user_share.first ];
  }
  /* Determine fair share rate as max over all allocations */
  auto arg_max = std::max_element( final_share.begin(), final_share.end(), 
                 [&] (const std::pair<uint64_t,double> & p1, const std::pair<uint64_t,double> &p2 )
                 { return p1.second < p2.second ;} );
  return arg_max->second ;

}

bool SFD::should_drop( double prob )
{
  /* Toss a biased coin */
  static RNG* dropper = new RNG();
  return dropper->next_double() <= prob ;
}
