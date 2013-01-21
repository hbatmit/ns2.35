#include "poisson-link.h"

static class PoissonLinkClass : public TclClass {
  public:
    PoissonLinkClass() : TclClass("DelayLink/PoissonLink") {}
    TclObject* create(int argc, const char*const* argv) {
      return ( new PoissonLink( 1.0, 1 ) );
    }
} class_poisson_link;

PoissonLink::PoissonLink( double bandwidth, uint32_t iter ) :
  LinkDelay(),
  _arrivals( new RNG ),
  _bandwidth( bandwidth ),
  _iter( iter )
{
  fprintf( stderr, "In PoissonLink constructor \n" );
  bind_bw("bandwidth_",&_bandwidth);
  /* call it bandwidth_ to not break ns-link.tcl */ 

  bind("_iter",&_iter);
  /* set _iter separately */

  for ( uint32_t i=0; i < _iter; i++ ) {
    _arrivals->reset_next_substream();
  }
}

double PoissonLink::transmission_time( double num_bits )
{
  return _arrivals->exponential( num_bits/_bandwidth );
}

void PoissonLink::recv( Packet* p, Handler* h)
{
  Scheduler& s = Scheduler::instance();
  double tx_time = transmission_time( 8. * hdr_cmn::access(p)->size() ) ;
//  printf( " Now: %e Iter :%d , txtime %f \n", s.clock(), _iter, tx_time );
  s.schedule(target_, p, tx_time + delay_ ); /* Propagation delay */
  s.schedule(h, &intr_,  tx_time ); /* Transmission delay */
}
