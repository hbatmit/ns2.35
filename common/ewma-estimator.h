#ifndef EMWA_ESTIMATOR_HH
#define EMWA_ESTIMATOR_HH

class EwmaEstimator {
  private :
    double _estimate;
    double _time_constant;

  public:
    double _last_update;

    /* Constructor */
    EwmaEstimator( double K, double initial_value, double initial_time );

    /* Default Constructor */
    EwmaEstimator();

    /* Update with new values */
    double update( double now, double current_value );

    /* Return estimate */
    double get_estimate( void ) const { return _estimate; }

    /* Return time of last update */
    double get_last_update( void ) const { return _last_update; }

    /* Return time constant for this EWMA */
    double get_time_constant( void ) const { return _time_constant; }
};

#endif
