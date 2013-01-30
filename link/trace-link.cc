#include "trace-link.h"
#include<algorithm>
#include<math.h>

static class TraceLinkClass : public TclClass {
  public:
    TraceLinkClass() : TclClass("DelayLink/TraceLink") {}
    TclObject* create(int argc, const char*const* argv) {
      return ( new TraceLink() );
    }
} class_trace_link;

int TraceLink::command(int argc, const char*const* argv)
{
  if (argc == 2) {
    if ( strcmp(argv[1], "total" ) == 0 ) {
      fprintf( stderr, " Total number of bits is %u \n", _bits_dequeued );
      return TCL_OK;
    }
  }
  if (argc == 3) {
    if ( strcmp(argv[1], "trace-file" ) == 0 ) {
      populate_pdos( std::string( argv[2] ) );
      return TCL_OK;
    }
  }
  return LinkDelay::command( argc, argv );
}

TraceLink::TraceLink() :
  LinkDelay(),
  _trace_file( "" ),
  _pdos( std::vector<double> () ),
  _current_pkt_num( 0 ),
  _bits_dequeued( 0 )
{}

void TraceLink::populate_pdos( std::string trace_file )
{
  assert( _trace_file == "" );
  _trace_file =  trace_file;
  fprintf( stderr, "TraceLink : _trace_file : %s \n", _trace_file.c_str() );
  /* Read from file */
  FILE *f = fopen( _trace_file.c_str() , "r" );
  if ( f == NULL ) {
    perror( "fopen" );
    exit( 1 );
  }
  while ( 1 ) {
    uint64_t ms;
    int num_matched = fscanf( f, "%lu\n", &ms );
    if ( num_matched != 1 ) {
      break;
    }
    if ( !_pdos.empty() ) {
      assert( ms >= _pdos.back() );
    }
    _pdos.push_back( (double)ms/1000.0 );
  }
  fprintf( stderr, "Initialized ns with %d services.\n", (int)_pdos.size() );
}

void TraceLink::recv( Packet* p, Handler* h)
{
  assert( _trace_file != "" );
  Scheduler& s = Scheduler::instance();
  while (s.clock() > _pdos.at( _current_pkt_num ) ) {
    /* Missed opportunities */
    _current_pkt_num ++;
  }
  //  printf( " DIR: %s Now: %e next packet at %f \n", _trace_file.c_str(), s.clock(), _pdos.at( _current_pkt_num ) );

  s.schedule(target_, p, _pdos.at( _current_pkt_num ) - s.clock() ); /* Propagation  delay */
  s.schedule(h, &intr_,  _pdos.at( _current_pkt_num ) - s.clock() ); /* Transmission delay */
  _current_pkt_num ++;
  _bits_dequeued += (8 * hdr_cmn::access(p)->size());
}
