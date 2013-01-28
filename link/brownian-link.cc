#include "brownian-link.h"
#include<algorithm>
#include<math.h>

static class BrownianLinkClass : public TclClass {
  public:
    BrownianLinkClass() : TclClass("DelayLink/BrownianLink") {}
    TclObject* create(int argc, const char*const* argv) {
      return ( new BrownianLink() );
    }
} class_brownian_link;

int BrownianLink::command(int argc, const char*const* argv)
{
  if (argc == 2) {
    if ( strcmp(argv[1], "total" ) == 0 ) {
      fprintf( stderr, " Total number of bits is %u \n", _bits_dequeued );
      return TCL_OK;
    }
  }
  return LinkDelay::command( argc, argv );
}

BrownianLink::BrownianLink() :
  LinkDelay(),
  _rate( new RNG ),
  _min_rate( 0.0 ),
  _max_rate( 0.0 ),
  _arrivals( new RNG ),
  _iter( 1 ),
  _duration( 1 ),
  _pdos( std::vector<double> () ),
  _rejection_sampler( new RNG ),
  _current_pkt_num( 0 ),
  _bits_dequeued( 0 )
{
  bind_bw("_min_rate",&_min_rate);
  bind_bw("_max_rate",&_max_rate);
  bind("_iter",&_iter);
  bind("_duration",&_duration);
  fprintf( stderr, "BrownianLink : _min_rate : %f, _max_rate : %f, _iter :%d \n", _min_rate, _max_rate, _iter );
  _rate_sample_path = std::vector<double> (_duration*1000);

  for ( uint32_t i=0; i < _iter; i++ ) {
    _rate->reset_next_substream();
    _arrivals->reset_next_substream();
    _rejection_sampler->reset_next_substream();
  }

  /* Generate sample path of rates for this run */
  _rate_sample_path.at( 0 ) = ( _rate->uniform( _min_rate, _max_rate ) );
  fprintf( stderr, "First rate is %f packets per second \n", _rate_sample_path.at( 0 ) );
  for ( uint32_t i=1; i < _duration*1000; i++ ) {
    do {
      _rate_sample_path.at( i ) =  _rate->normal( _rate_sample_path.at( i - 1 ), 1 );
    } while ( _rate_sample_path.at(i) <= _min_rate or _rate_sample_path.at(i) >= _max_rate );
    fprintf( stderr, "Duration %d Next rate at %dth ms is %f packets per second \n", _duration, i, _rate_sample_path.at( i ) );
    /* Rate evolves @ 1 MTU sized packet per second every millisecond */
  }

  auto max_sampled_rate = *std::max_element( _rate_sample_path.begin(), _rate_sample_path.end() );

  /* Generate packet arrivals for this run */
  double tstar = 0; /* tstar is in seconds */
  double U = 0; /* for rejection sampling */
  _pdos.push_back( 0.0 + delay_ );

  /* Generate an exponential random variable E with intensity equal to the max rate */
  while ( tstar < _duration ) {
    do {
      auto E = _arrivals->exponential( 1/_max_rate );
      tstar += E;
      if ( tstar >= _duration ) {
        break ; /* To avoid exception */
      }
      U = _rejection_sampler->uniform( 0.0, 1.0 );
    } while ( U > _rate_sample_path.at( (uint32_t)floor( tstar*1000 ) )/max_sampled_rate );
    _pdos.push_back( tstar + delay_ );
  }
}

void BrownianLink::recv( Packet* p, Handler* h)
{
  Scheduler& s = Scheduler::instance();
  while (s.clock() > _pdos.at( _current_pkt_num ) ) {
    /* Missed opportunities */
    _current_pkt_num ++;
  }
  printf( " Now: %e Iter :%d , next packet at %f \n", s.clock(), _iter, _pdos.at( _current_pkt_num ) );

  s.schedule(target_, p, _pdos.at( _current_pkt_num ) - s.clock() ); /* Propagation  delay */
  s.schedule(h, &intr_,  _pdos.at( _current_pkt_num ) - s.clock() ); /* Transmission delay */
  _current_pkt_num ++;
  _bits_dequeued += (8 * hdr_cmn::access(p)->size());
}
