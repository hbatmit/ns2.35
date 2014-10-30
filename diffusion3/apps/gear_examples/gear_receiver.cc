//
// gear_receiver.cc : Gear Receiver Main File
// author           : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: gear_receiver.cc,v 1.7 2010/03/08 05:54:49 tom_henderson Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.

#include "gear_receiver.hh"

#ifdef NS_DIFFUSION
static class GearReceiverAppClass : public TclClass {
public:
  GearReceiverAppClass() : TclClass("Application/DiffApp/GearReceiverApp") {}
  TclObject * create(int , const char*const* ) {
    return (new GearReceiverApp());
  }
} class_gear_receiver_app_class;

int GearReceiverApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "subscribe") == 0) {
      run();
      return TCL_OK;
    }
  }
  else if (argc >= 6) {
    // TCL API $app push-pull-options <push/pull> <point/region> <co-od1> <co-od2> <co-od3> <co-od4>
    if (strcmp(argv[1], "push-pull-options") == 0) {
      if (strcmp(argv[2], "push") == 0) 
	using_push_ = true;
      else
	using_push_ = false;

      if (strcmp(argv[3], "point") == 0) 
	{
	  using_points_ = true;
	  if (argv[4] != NULL && argv[5] != NULL) 
	    {
	      lat_pt_ = atoi(argv[4]);
	      long_pt_ = atoi(argv[5]);
	    }
	}
      else
	{
	  using_points_ = false;
	  if (argv[4] != NULL && argv[5] != NULL && argv[6] != NULL && argv[7] != NULL)
	    {
	      lat_min_ = atoi(argv[4]);
	      lat_max_ = atoi(argv[5]);
	      long_min_ = atoi(argv[6]);
	      long_max_ = atoi(argv[7]);
	    }
	}
      return TCL_OK;
    }
  }
  return DiffApp::command(argc, argv);
}

#endif //NS_DIFFUSION

void GearReceiverReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app_->recv(data, my_handle);
}

void GearReceiverApp::recv(NRAttrVec *data, NR::handle )
{
  NRSimpleAttribute<int> *counterAttr = NULL;
  NRSimpleAttribute<void *> *timeAttr = NULL;
  EventTime *probe_event;
  long delay_seconds;
  long delay_useconds;
  float total_delay;
  struct timeval tmv;

  GetTime(&tmv);

  counterAttr = GearCounterAttr.find(data);
  timeAttr = GearTimeAttr.find(data);

  if (!counterAttr || !timeAttr){
    DiffPrint(DEBUG_ALWAYS, "Received a BAD packet !\n");
    PrintAttrs(data);
    return;
  }

  // Calculate latency
  probe_event = (EventTime *) timeAttr->getVal();
  delay_seconds = tmv.tv_sec;
  delay_useconds = tmv.tv_usec;

  if ((delay_seconds < probe_event->seconds_) ||
      ((delay_seconds == probe_event->seconds_) &&
       (delay_useconds < probe_event->useconds_))){
    // Time's not synchronized
    delay_seconds = -1;
    delay_useconds = 0;
    DiffPrint(DEBUG_ALWAYS, "Error calculating delay !\n");
  }
  else{
    delay_seconds = delay_seconds - probe_event->seconds_;
    if (delay_useconds < probe_event->useconds_){
      delay_seconds--;
      delay_useconds = delay_useconds + 1000000;
    }
    delay_useconds = delay_useconds - probe_event->useconds_;
  }
  total_delay = (float) (1.0 * delay_seconds) + ((float) delay_useconds / 1000000.0);

  // Check if this is the first message received
  if (first_msg_recv_ < 0){
    first_msg_recv_ = counterAttr->getVal();
  }

  // Print output message
  if (last_seq_recv_ >= 0){
    if (counterAttr->getVal() < last_seq_recv_){
      // Multiple sources detected, disabling statistics
      last_seq_recv_ = -1;
      DiffPrint(DEBUG_ALWAYS, "Node%d: Received data %d, total latency = %f!\n",
		((DiffusionRouting *)dr_)->getNodeId(),
		counterAttr->getVal(), total_delay);
    }
    else{
      last_seq_recv_ = counterAttr->getVal();
      num_msg_recv_++;
      DiffPrint(DEBUG_ALWAYS, "Node%d: Received data: %d, total latency = %f, %% messages received: %f !\n",
		((DiffusionRouting *)dr_)->getNodeId(),
		last_seq_recv_, total_delay,
		(float) ((num_msg_recv_ * 100.00) /
			 ((last_seq_recv_ - first_msg_recv_) + 1)));
    }
  }
  else{
    DiffPrint(DEBUG_ALWAYS, "Node%d: Received data %d, total latency = %f !\n",
	      ((DiffusionRouting *)dr_)->getNodeId(),
	      counterAttr->getVal(), total_delay);
  }
}

handle GearReceiverApp::setupSubscription()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS,
				   NRAttribute::INTEREST_CLASS));
  // Use push or pull semantics
  if (using_push_){
    attrs.push_back(NRScopeAttr.make(NRAttribute::IS,
				     NRAttribute::NODE_LOCAL_SCOPE));

    // Insert point coordinates
    attrs.push_back(LatitudeAttr.make(NRAttribute::IS, lat_pt_));
    attrs.push_back(LongitudeAttr.make(NRAttribute::IS, long_pt_));
  }
  else{
    attrs.push_back(NRScopeAttr.make(NRAttribute::IS,
				     NRAttribute::GLOBAL_SCOPE));

    // Insert point or region coordinates
    if (using_points_){
      attrs.push_back(LatitudeAttr.make(NRAttribute::EQ, lat_pt_));
      attrs.push_back(LongitudeAttr.make(NRAttribute::EQ, long_pt_));
    }
    else{
      attrs.push_back(LatitudeAttr.make(NRAttribute::GE, lat_min_));
      attrs.push_back(LatitudeAttr.make(NRAttribute::LE, lat_max_));
      attrs.push_back(LongitudeAttr.make(NRAttribute::GE, long_min_));
      attrs.push_back(LongitudeAttr.make(NRAttribute::LE, long_max_));
    }
  }

  attrs.push_back(GearTargetAttr.make(NRAttribute::IS, "F117A"));

  handle h = dr_->subscribe(&attrs, mr_);

  ClearAttrs(&attrs);

  return h;
}

void GearReceiverApp::run()
{
  subHandle_ = setupSubscription();

#ifndef NS_DIFFUSION
  // Do nothing
  while (1){
    sleep(1000);
  }
#endif // !NS_DIFFUSION
}

void GearReceiverApp::usage(char *s){
  DiffPrint(DEBUG_ALWAYS, "Usage: %s [-d debug] [-p port] [-s] [-r] [-h]\n\n", s);
  DiffPrint(DEBUG_ALWAYS, "\t-d - Sets debug level (0-10)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-p - Uses port 'port' to talk to diffusion\n");
  DiffPrint(DEBUG_ALWAYS, "\t-s - Uses push semantics (default: pull)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-r - Uses regions (default: points)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-h - Prints this information\n");
  DiffPrint(DEBUG_ALWAYS, "\n");
  exit(0);
}

void GearReceiverApp::parseCommandLine(int argc, char **argv)
{
  u_int16_t diff_port = DEFAULT_DIFFUSION_PORT;
  int debug_level;
  int opt;

  config_file_ = NULL;
  using_points_ = true;
  using_push_ = false;
  opterr = 0;

  while (1){
    opt = getopt(argc, argv, "srhd:p:");
    switch (opt){

    case 'p':

      diff_port = (u_int16_t) atoi(optarg);
      if ((diff_port < 1024) || (diff_port >= 65535)){
	DiffPrint(DEBUG_ALWAYS, "Error: Diffusion port must be between 1024 and 65535 !\n");
	exit(-1);
      }

      break;

    case 'h':

      usage(argv[0]);

      break;

    case 'd':

      debug_level = atoi(optarg);

      if (debug_level < 1 || debug_level > 10){
	DiffPrint(DEBUG_ALWAYS, "Error: Debug level outside range or missing !\n");
	usage(argv[0]);
      }

      global_debug_level = debug_level;

      break;

    case 's':

      using_push_ = true;

      break;

    case 'r':

      using_points_ = false;

      break;

    case '?':

      DiffPrint(DEBUG_ALWAYS,
		"Error: %c isn't a valid option or its parameter is missing !\n", optopt);
      usage(argv[0]);

      break;

    case ':':

      DiffPrint(DEBUG_ALWAYS, "Parameter missing !\n");
      usage(argv[0]);

      break;

    }

    if (opt == -1)
      break;
  }

  diffusion_port_ = diff_port;
}

void GearReceiverApp::readGeographicCoordinates()
{
  char *gear_coord_env;

  if (using_points_ || using_push_){
    // Read latitude coordinate
    gear_coord_env = getenv("lat_pt");
    if (!gear_coord_env){
      DiffPrint(DEBUG_ALWAYS, "Cannot read lat_pt !\n");
      exit(-1);
    }
    lat_pt_ = atoi(gear_coord_env);

    // Read longitude coordinate
    gear_coord_env = getenv("long_pt");
    if (!gear_coord_env){
      DiffPrint(DEBUG_ALWAYS, "Cannot read long_pt !\n");
      exit(-1);
    }
    long_pt_ = atoi(gear_coord_env);
  }
  else{
    // Read latitude_min value
    gear_coord_env = getenv("lat_min");
    if (!gear_coord_env){
      DiffPrint(DEBUG_ALWAYS, "Cannot read lat_min !\n");
      exit(-1);
    }
    lat_min_ = atoi(gear_coord_env);

    // Read latitude_max value
    gear_coord_env = getenv("lat_max");
    if (!gear_coord_env){
      DiffPrint(DEBUG_ALWAYS, "Cannot read lat_max !\n");
      exit(-1);
    }
    lat_max_ = atoi(gear_coord_env);

    // Read longitude_min value
    gear_coord_env = getenv("long_min");
    if (!gear_coord_env){
      DiffPrint(DEBUG_ALWAYS, "Cannot read long_min !\n");
      exit(-1);
    }
    long_min_ = atoi(gear_coord_env);

    // Read longitude_max value
    gear_coord_env = getenv("long_max");
    if (!gear_coord_env){
      DiffPrint(DEBUG_ALWAYS, "Cannot read long_max !\n");
      exit(-1);
    }
    long_max_ = atoi(gear_coord_env);
  }
}

#ifdef NS_DIFFUSION
GearReceiverApp::GearReceiverApp()
#else
GearReceiverApp::GearReceiverApp(int argc, char **argv)
#endif // NS_DIFFUSION
{
  last_seq_recv_ = 0;
  num_msg_recv_ = 0;
  first_msg_recv_ = -1;

  mr_ = new GearReceiverReceive(this);

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  readGeographicCoordinates();
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION
}

#ifndef NS_ADDRESS
#ifndef USE_SINGLE_ADDRESS_SPACE 
int main(int argc, char **argv)
{
  GearReceiverApp *app;

  app = new GearReceiverApp(argc, argv);
  app->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE
#endif // !NS_ADDRESS
