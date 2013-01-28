#ifndef TRACE_LINK_HH
#define TRACE_LINK_HH

#include<string>

/*
 * Read packet arrivals from a trace file
 */

#include "delay.h"
#include "rng.h"
#include <vector>

class TraceLink : public LinkDelay {
  private :
      std::string _trace_file;
      std::vector<double> _pdos;             /* index: packet num, output: packet delivery time in s */
      uint32_t _current_pkt_num;
      uint32_t _bits_dequeued;

  public :
      /* constructor */
      TraceLink();

      /* override the recv function from LinkDelay */
      virtual void recv( Packet* p, Handler* h);

      /* Tcl command interface */
      virtual int command(int argc, const char*const* argv);

      /* Populate PDOs based on a file */
      void populate_pdos( std::string trace_file );

};

#endif
