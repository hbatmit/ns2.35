#ifndef CELL_LINK_HH
#define CELL_LINK_HH

#include<vector>
#include "rng.h"
#include "cdma-rates.h"
/* An implementation of a fading Cellular Link,
 * along with a proportionally fair scheduler .
 * Tcl script calls tick() at slot boundaries
 * tick() functionality :
 * --updates slot number 
 * --generates new rates
 * --checks if the previous user's slots are exhausted
 * ----if so, pick a new user based on prop. fairness
 * --in either case, update EWMA average */

class CellLink : public TclObject {
  private:
    uint32_t _num_users;
    uint32_t _current_user;
    std::vector<double> _current_rates;
    std::vector<double> _average_rates;
    std::vector<RNG*>    _rate_generators;
    uint32_t _iter;
    uint32_t _current_slot;
    double TIME_SLOT_DURATION; /* CDMA, 1.67 ms time slots */
    uint32_t EWMA_SLOTS;

  public :
    CellLink( uint32_t num_users, uint32_t iteration_number );
    /* Called by simulator, every TIME_SLOT_DURATION on CellLink */
    void tick( void );

    /* Find the user to schedule, using the proportional fair scheduler */
    uint32_t pick_user_to_schedule();

    /* Generate new rates from allowed rates */
    void generate_new_rates(); 

    /* Is it time to revise the scheduling decision ? */
    bool time_to_revise();

    /* Update EWMA filter for proportionally fair scheduler */
    void update_average_rates( uint32_t scheduled_user );

    /* Get current user whose packets get sent out on SFD::deque() */
    uint32_t get_current_user();

    /* Tcl interface */
    int command(int argc, const char*const* argv);

};

#endif
