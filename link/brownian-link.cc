#include "brownian-link.h"

static class BrownianLinkClass : public TclClass {
  public:
    BrownianLinkClass() : TclClass("DelayLink/BrownianLink") {}
    TclObject* create(int argc, const char*const* argv) {
      return ( new BrownianLink( 0.0, 1.0, 1 ) );
    }
} class_brownian_link;

BrownianLink::BrownianLink( double min_rate, double max_rate, uint32_t iter ) :
  LinkDelay(),
  _rate( new RNG ),
  _min_rate( min_rate ),
  _max_rate( max_rate ),
  _arrivals( new RNG ),
  _iter( iter )
{
  bind_bw("_min_rate",&_min_rate);
  bind_bw("_max_rate",&_max_rate);
  bind("_iter",&_iter);
  fprintf( stderr, "BrownianLink : _min_rate : %f, _max_rate : %f, _iter :%d \n", _min_rate, _max_rate, _iter );

  for ( uint32_t i=0; i < _iter; i++ ) {
    _rate->reset_next_substream();
    _arrivals->reset_next_substream();
  }
}

double BrownianLink::transmission_time( double num_bits )
{
  auto bandwidth = _rate->uniform( _min_rate, _max_rate );
  printf( "Chosen bandwidth : %f \n", bandwidth );
  return _arrivals->exponential( num_bits / bandwidth );
}

void BrownianLink::recv( Packet* p, Handler* h)
{
  Scheduler& s = Scheduler::instance();
  double tx_time = transmission_time( 8. * hdr_cmn::access(p)->size() ) ;
  printf( " Now: %e Iter :%d , txtime %f \n", s.clock(), _iter, tx_time );
  s.schedule(target_, p, tx_time + delay_ ); /* Propagation delay */
  s.schedule(h, &intr_,  tx_time ); /* Transmission delay */
}
