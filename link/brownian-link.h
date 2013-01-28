#ifndef BROWNIAN_LINK_HH
#define BROWNIAN_LINK_HH

/* A link with an inhomogenous Poisson
 * process that dictates packet arrivals.
 * First, sample a rate from a rate distribution.
 * Then, sample a transmission time using that rate.
 */

#include "delay.h"
#include "rng.h"
#include <vector>

class BrownianLink : public LinkDelay {
  private :
      RNG* _rate;
      double _min_rate;
      double _max_rate;
      RNG* _arrivals;
      uint32_t _iter;
      uint32_t _duration;                    /* Duration of the simulation */
      std::vector<double> _rate_sample_path; /* index: time in ms, output: rate in packets per second */
      std::vector<double> _pdos;             /* index: packet num, output: packet delivery time in s */
      RNG* _rejection_sampler;
      uint32_t _current_pkt_num;
      uint32_t _bits_dequeued;

  public :
      /* constructor */
      BrownianLink();

      /* override the recv function from LinkDelay */
      virtual void recv( Packet* p, Handler* h);

      /* Tcl command interface */
      virtual int command(int argc, const char*const* argv);

};

#endif
