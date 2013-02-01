#include "cell-link.h"
#include <algorithm>
#include <float.h>

static class CellLinkClass : public TclClass {
  public :
    CellLinkClass() : TclClass("DelayLink/CellLink") {}
    TclObject* create(int argc, const char*const* argv) {
      return ( new CellLink() );
    }
} class_cell_link;

CellLink::CellLink()
{
  bind( "_iter",&_iter );
  bind( "_num_users", &_num_users );
  printf( "CellLink: _num_users %u, iter %u\n", _num_users, _iter );
  fflush( stdout );
  assert( _num_users > 0 );
  assert( _iter > 0 );
  _current_rates = std::vector<double>( _num_users );
  _average_rates = std::vector<double>( _num_users );
  _rate_generators = std::vector<RNG*>( _num_users, new RNG() );
  auto advance_substream = [&] ( RNG *r )
                           { for ( uint32_t i=1; i < _iter ; i++ ) r->reset_next_substream();};
  std::for_each( _rate_generators.begin(), _rate_generators.end(), advance_substream );
}

void CellLink::tick()
{
  generate_new_rates();
}

void CellLink::generate_new_rates()
{
  /* For now, generate new rates uniformly from allowed rates
   * TODO: fix this to exponential or random walk
   * */
  auto rate_generator = [&] ( RNG *r ) { return ALLOWED_RATES[ r->uniform( (int)ALLOWED_RATES.size() )]; };
  std::transform( _rate_generators.begin(), _rate_generators.end(),
                  _current_rates.begin(),
                  rate_generator );
}

std::vector<double> CellLink::get_current_rates()
{
  return _current_rates;
}

void CellLink::recv( Packet* p, Handler* h )
{
  /* Get flow_id from packet */
  hdr_ip *iph=hdr_ip::access( p );
  auto flow_id = iph->flowid();

  /* Find transmission rate for this flow */
  auto tx_rate = _current_rates.at( flow_id );

  /* Schedule transmission */
  Scheduler& s = Scheduler::instance();
  double tx_time = 8. * hdr_cmn::access(p)->size() / tx_rate ;
  s.schedule(target_, p, tx_time + delay_ ); /* Propagation delay */
  s.schedule(h, &intr_,  tx_time ); /* Transmission delay */

  /* Tick to get next set of rates */
  tick();
}
