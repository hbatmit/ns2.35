#ifndef CELL_LINK_HH
#define CELL_LINK_HH

#include <map>
#include <vector>
#include <queue>
#include "rng.h"
#include "cdma-rates.h"
#include "delay.h"
#include "flow-stats.h"
#include "common/timer-handler.h"

/* Forward declarations */
class LinkAwareQueue;

/* Generate rates for each user from a set of rates */
class RateGen {
  public:
    RNG* _rng;
    std::vector<double> _allowed_rates;

    RateGen( std::vector<double> allowed_rates )
    {
      _rng = new RNG();
      _allowed_rates = allowed_rates;
    }
};

/* The cellular link inherits from LinkDelay and TimerHandler */
class CellLink : public LinkDelay, public TimerHandler {
  private:
    uint32_t _num_users;
    std::vector<double>  _current_link_rates;
    std::vector<RateGen> _rate_generators;
    uint32_t _iter;
    uint64_t _bits_dequeued;
    std::vector<double> _average_rates;
    static const uint32_t EWMA_SLOTS = 100;
    bool     _activate_link_scheduler;
    const LinkAwareQueue * _link_aware_queue;
    double _slot_duration; /* Scheduling slot in seconds */
    double _last_time; /* Last time a user was scheduled */

  public :
    /* Constructor */
    CellLink();

    /* Called by CellLink on every scheduling slot */
    void tick( void );

    /* Generate new rates from allowed rates */
    void generate_new_rates();

    /* Get current rates of all users for SFD::deque() to use.
       SFD could ignore this as well. */
    std::vector<double> get_current_link_rates() const { return _current_link_rates; }

    /* override the recv function from LinkDelay */
    virtual void recv( Packet* p, Handler* h) override;

    /* override the expire function from TimerHandler */
    virtual void expire(Event *e) override;

    /* Tcl interface */
    int command(int argc, const char*const* argv);

    /* Prop. scheduler */
    uint64_t pick_user_to_schedule() const;
    uint64_t chosen_flow ;

    /* Update averages */
    void update_average_rates( uint32_t scheduled_user );

};

#endif
