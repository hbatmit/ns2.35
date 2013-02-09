#ifndef EMWA_ESTIMATOR_HH
#define EMWA_ESTIMATOR_HH

class EwmaEstimator {
  private :
    double _rate_estimate;
    double _time_constant;

  public:
    double _last_update;

    /* Constructor */
    EwmaEstimator( double K, double initial_rate, double initial_time );

    /* Default Constructor */
    EwmaEstimator();

    /* Update with new values */
    double update( double now, double current_rate );

    /* Return rate */
    double get_rate( void ) const;
};

#endif
