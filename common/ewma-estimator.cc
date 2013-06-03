#include "ewma-estimator.h"
#include <assert.h>
#include <math.h>

EwmaEstimator::EwmaEstimator( double K, double initial_value, double initial_time ) :
  _estimate( initial_value ),
  _time_constant( K ),
  _last_update( initial_time )
{}

EwmaEstimator::EwmaEstimator() :
  _estimate( -1 ),
  _time_constant( -1 ),
  _last_update( -1 )
{}

double EwmaEstimator::update( double now, double current_value )
{
  /* Ensure some time has passed since last update */
  auto inter_update_interval = now - _last_update;

  /* Apply EWMA */
  _last_update = now;
  _estimate =
    (1.0 - exp(-inter_update_interval/_time_constant))*current_value +
    exp(-inter_update_interval/_time_constant)*_estimate;
  return _estimate;
}
