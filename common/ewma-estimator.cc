#include "ewma-estimator.h"
#include <assert.h>
#include <math.h>

EwmaEstimator::EwmaEstimator( double K, double initial_rate, double initial_time ) :
  _rate_estimate( initial_rate ),
  _time_constant( K ),
  _last_update( initial_time )
{}

EwmaEstimator::EwmaEstimator() :
  _rate_estimate( -1 ),
  _time_constant( -1 ),
  _last_update( -1 )
{}

double EwmaEstimator::update( double now, double current_rate )
{
  /* Ensure some time has passed since last update */
  auto inter_update_interval = now - _last_update;

  /* Apply EWMA */
  _last_update = now;
  _rate_estimate =
    (1.0 - exp(-inter_update_interval/_time_constant))*current_rate +
    exp(-inter_update_interval/_time_constant)*_rate_estimate;
  return _rate_estimate;
}

double EwmaEstimator::get_rate( void ) const
{
  return _rate_estimate;
}
