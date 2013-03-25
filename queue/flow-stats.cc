#include "flow-stats.h"

FlowStats::FlowStats( double K ) :
  _first_arrival( -1 ),
  _first_service( -1 ),
  _acc_arrivals( 0.0 ),
  _acc_services( 0.0 ),
  _arrival_count( 0 ),
  _service_count( 0 ),
  _arr_est( EwmaEstimator() ),
  _ser_est( EwmaEstimator() ),
  _link_est( EwmaEstimator() ),
  _K( K )
{}

double FlowStats::est_arrival_rate( double now, Packet *p )
{
  /* Increment arrival count and accumulate arrivals */
  _arrival_count ++;
  _acc_arrivals += get_pkt_size( p );

  if ( _arrival_count == 1 ) {
    /* Is this your first arrival */
    _first_arrival = now;
    _acc_arrivals = 0;
    return 0;

  } else if ( _arrival_count == 2 ){
    /* Is this your second arrival ? */
    if ( now == _first_arrival ) {
      _arrival_count --; /* Wait for a proper 2nd arrival */
      return 0;
    } else {
      auto initial_rate = _acc_arrivals/( now - _first_arrival );
      printf( " K is %f, initial arrival rate is %f \n", _K, initial_rate);
      _arr_est = EwmaEstimator( _K, initial_rate, now );
      _acc_arrivals = 0;
      return _arr_est.get_rate();
    }

  } else {
    /* Run EWMA if you have seen more than two arrivals */
    if ( now == _arr_est._last_update ) {
      return _arr_est.get_rate();
    } else {
      assert( now > _arr_est._last_update );
      auto current_rate = _acc_arrivals/ ( now - _arr_est._last_update );
      _acc_arrivals = 0;
      return _arr_est.update( now, current_rate );
    }
  }
}

double FlowStats::est_service_rate( double now, Packet *p )
{
  /* Increment service count and accumulate services */
  _service_count++;
  _acc_services += get_pkt_size( p );

  if ( _service_count == 1 ) {
    /* Is this your first service ? */
    _first_service = now;
    _acc_services=0;
    return 0;

  } else if ( _service_count == 2 ) {
    /* Is this your second service ? */
    if ( now == _first_service ) {
      _service_count --; /* Wait for a proper 2nd service */
      return 0;
    } else {
      auto initial_rate=_acc_services/( now - _first_service );
      printf( " K is %f, initial service rate is %f \n", _K, initial_rate);
      _ser_est = EwmaEstimator( _K, initial_rate, now );
      _acc_services = 0;
      return _ser_est.get_rate();
    }

  } else {
    /* Run EWMA if you have seen more than two services */
    if ( now == _ser_est._last_update ) {
      return _ser_est.get_rate();
    } else {
      assert( now > _ser_est._last_update );
      auto current_rate = _acc_services/( now - _ser_est._last_update );
      _acc_services = 0;
      return _ser_est.update( now, current_rate );
    }
  }
}

double FlowStats::est_link_rate( double now, double current_link_rate )
{
  if ( _link_est.get_rate() == -1 ) {
    /* First rate, simply seed estimator */
    _link_est = EwmaEstimator( _K, current_link_rate, now );
    return _link_est.get_rate();
  } else {
    /* Apply EWMA */
    return _link_est.update( now, current_link_rate );
  }
}

uint32_t FlowStats::get_pkt_size( Packet* p )
{
  /* Extract packet length in bits from the header */
  if ( p != nullptr ) {
    hdr_cmn* hdr = hdr_cmn::access(p);
    return ( hdr->size() << 3 );
  } else {
    return 0;
  }
}
