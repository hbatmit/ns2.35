#ifndef LINK_RAYLEIGH_CHANNEL_H_
#define LINK_RAYLEIGH_CHANNEL_H_

#include "tools/rng.h"

/* Random dropping :
 * Every "tau" ms,
 * randomize the channel by picking
 * a new channel coefficient magnitude
 */

class RayleighChannel {
  private :
    RNG* channel_generator_;
    RNG* noise_generator_;
    int iter_;
    double coherence_time_;
    double noise_sigma_;
    double last_channel_update_;
    double last_channel_magnitude_;

  public :
    /* constructor */
    RayleighChannel(double coherence_time, double noise_sigma);

    /* Set seed */
    void set_iter(int iter);

    /* Calculate channel using Rayleigh fading */
    double get_channel(void);

    /* Generate noise */
    double get_noise(void);
};


#endif  // LINK_RAYLEIGH_CHANNEL_H_
