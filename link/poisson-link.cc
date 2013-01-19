#include "poisson-link.h"

static class PoissonLinkClass : public TclClass {
  public:
    PoissonLinkClass() : TclClass("DelayLink/PoissonLink") {}
    TclObject* create(int argc, const char*const* argv) {
      return ( new PoissonLink( 1.0, 1 ) );
    }
} class_poisson_link;

PoissonLink::PoissonLink( double lambda, uint32_t iter ) :
  LinkDelay(),
  _arrivals( new RNG ),
  _lambda( lambda ),
  _iter( iter ),
  _next_rx( 0.0 )
{
  printf( "In PoissonLink constructor \n" );
  bind_bw("bandwidth_",&_lambda); 
  /* call it bandwidth_ to not break ns-link.tcl */ 
  
  bind("_iter",&_iter);
  /* set _iter separately */

  for ( uint32_t i=0; i < _iter; i++ ) {
    _arrivals->reset_next_substream();
  }
}

double PoissonLink::generate_interarrival( void )
{
  return _arrivals->exponential( 1.0/_lambda );
}

void PoissonLink::recv( Packet* p, Handler* h)
{
  Scheduler& s = Scheduler::instance();
  double interarrival = 0.0;
  if ( _next_rx == 0.0 ) {
    /* We haven't seen any deliveries yet hence 
       last reception is 0 */
    _next_rx = s.clock() + delay_ ;
  } else {
    /* Simulate _next_rx as a Poisson process */
    interarrival = generate_interarrival();
    _next_rx += interarrival;
  }
  printf( " Now: %e Iter :%d , Next reception: %e, interarrival: %e, lambda :%e \n", s.clock(), _iter, _next_rx, interarrival, _lambda );
  s.schedule(target_, p, _next_rx - s.clock() );
  s.schedule(h, &intr_,  _next_rx - s.clock() );
}
