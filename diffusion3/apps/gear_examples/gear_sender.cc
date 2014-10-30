//
// gear_sender.cc : Gear Sender Main File
// author         : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: gear_sender.cc,v 1.7 2011/10/02 22:32:34 tom_henderson Exp $
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

#include "gear_sender.hh"
#include <unistd.h>

#ifdef NS_DIFFUSION
static class GearSenderAppClass : public TclClass {
public:
  GearSenderAppClass() : TclClass("Application/DiffApp/GearSenderApp") {}
  TclObject * create(int , const char*const* ) {
    return (new GearSenderApp());
  }
} class_gear_sender_app_class;

void GearSendDataTimer::expire(Event *) {
  a_->send();
}

void GearSenderApp::send()
{
  struct timeval tmv;
  
  // Send data if we have active subscriptions
  if ((num_subscriptions_ > 0) || using_push_)
    {
      // Update time in the packet
      GetTime(&tmv);
      lastEventTime_->seconds_ = tmv.tv_sec;
      lastEventTime_->useconds_ = tmv.tv_usec;
      
      // Send data probe
      DiffPrint(DEBUG_ALWAYS, "Node%d: Sending Data %d\n", ((DiffusionRouting *)dr_)->getNodeId(), last_seq_sent_);
      dr_->send(pubHandle_, &data_attr_);
      
      // Update counter
      last_seq_sent_++;
      counterAttr_->setVal(last_seq_sent_);
    }

  // re-schedule the timer 
  sdt_.resched(SEND_DATA_INTERVAL);
}

int GearSenderApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "publish") == 0) {
      run();
      return TCL_OK;
    }
  }
  else if (argc >= 6) {
    // TCL API $app push-pull-option <push/pull> <point/region> <co-od1> <co-od2> <co-od3> <co-od4>
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

void GearSenderReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app_->recv(data, my_handle);
}

void GearSenderApp::recv(NRAttrVec *data, NR::handle )
{
  NRSimpleAttribute<int> *nrclass = NULL;

  nrclass = NRClassAttr.find(data);

  if (!nrclass){
    DiffPrint(DEBUG_ALWAYS, "Received a BAD packet !\n");
    return;
  }

  switch (nrclass->getVal()){

  case NRAttribute::INTEREST_CLASS:

    DiffPrint(DEBUG_ALWAYS, "Received an Interest message !\n");
    num_subscriptions_++;
    break;

  case NRAttribute::DISINTEREST_CLASS:

    DiffPrint(DEBUG_ALWAYS, "Received a Disinterest message !\n");
    num_subscriptions_--;
    break;

  default:

    DiffPrint(DEBUG_ALWAYS, "Received an unknown message (%d)!\n", nrclass->getVal());
    break;

  }
}

handle GearSenderApp::setupSubscription()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::NE,
				   NRAttribute::DATA_CLASS));
  attrs.push_back(NRScopeAttr.make(NRAttribute::IS,
				   NRAttribute::NODE_LOCAL_SCOPE));
  attrs.push_back(GearTargetAttr.make(NRAttribute::EQ, "F117A"));
  attrs.push_back(LatitudeAttr.make(NRAttribute::IS, lat_pt_));
  attrs.push_back(LongitudeAttr.make(NRAttribute::IS, long_pt_));

  handle h = dr_->subscribe(&attrs, mr_);

  ClearAttrs(&attrs);

  return h;
}

handle GearSenderApp::setupPublication()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS,
				   NRAttribute::DATA_CLASS));
  // Use push or pull semantics
  if (using_push_){
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
  else{
    attrs.push_back(NRScopeAttr.make(NRAttribute::IS,
				     NRAttribute::NODE_LOCAL_SCOPE));

    // Insert point coordinates
    attrs.push_back(LatitudeAttr.make(NRAttribute::IS, lat_pt_));
    attrs.push_back(LongitudeAttr.make(NRAttribute::IS, long_pt_));
  }

  attrs.push_back(GearTargetAttr.make(NRAttribute::IS, "F117A"));

  handle h = dr_->publish(&attrs);

  ClearAttrs(&attrs);

  return h;
}

void GearSenderApp::run()
{
  struct timeval tmv;
#ifndef NS_DIFFUSION
  int retval;
#endif // !NS_DIFFUSION
  
#ifdef INTERACTIVE
  char input;
  fd_set FDS;
#endif // INTERATIVE

  // Setup publication and subscription
  if (!using_push_)
    subHandle_ = setupSubscription();
  pubHandle_ = setupPublication();

  // Create time attribute
  GetTime(&tmv);
  lastEventTime_ = new EventTime;
  lastEventTime_->seconds_ = tmv.tv_sec;
  lastEventTime_->useconds_ = tmv.tv_usec;
  timeAttr_ = GearTimeAttr.make(NRAttribute::IS, (void *) &lastEventTime_,
			    sizeof(EventTime));
  data_attr_.push_back(timeAttr_);

  // Change pointer to point to attribute's payload
  delete lastEventTime_;
  lastEventTime_ = (EventTime *) timeAttr_->getVal();

  // Create counter attribute
  counterAttr_ = GearCounterAttr.make(NRAttribute::IS, last_seq_sent_);
  data_attr_.push_back(counterAttr_);

#ifndef NS_DIFFUSION
  // Main thread will send ping probes
  while(1){
#ifdef INTERACTIVE
    FD_SET(0, &FDS);
    fprintf(stdout, "Press <Enter> to send a ping probe...");
    fflush(NULL);
    select(1, &FDS, NULL, NULL, NULL);
    input = getc(stdin);
#else
    sleep(SEND_DATA_INTERVAL);
#endif // INTERACTIVE

    // Send data packet if we have active subscriptions
    if ((num_subscriptions_ > 0) || using_push_){
      // Update time in the packet
      GetTime(&tmv);
      lastEventTime_->seconds_ = tmv.tv_sec;
      lastEventTime_->useconds_ = tmv.tv_usec;

      // Send data probe
      DiffPrint(DEBUG_ALWAYS, "Node%d: Sending Data %d\n", ((DiffusionRouting *)dr_)->getNodeId(), last_seq_sent_);
      retval = dr_->send(pubHandle_, &data_attr_);

      // Update counter
      last_seq_sent_++;
      counterAttr_->setVal(last_seq_sent_);
    }
  }
#else
  send();
#endif // !NS_DIFFUSION
}

void GearSenderApp::usage(char *s){
  DiffPrint(DEBUG_ALWAYS, "Usage: %s [-d debug] [-p port] [-s] [-r] [-h]\n\n", s);
  DiffPrint(DEBUG_ALWAYS, "\t-d - Sets debug level (0-10)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-p - Uses port 'port' to talk to diffusion\n");
  DiffPrint(DEBUG_ALWAYS, "\t-s - Uses push semantics (default: pull)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-r - Uses regions (default: points)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-h - Prints this information\n");
  DiffPrint(DEBUG_ALWAYS, "\n");
  exit(0);
}

void GearSenderApp::parseCommandLine(int argc, char **argv)
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

void GearSenderApp::readGeographicCoordinates()
{
  char *gear_coord_env;

  if (using_points_ || !using_push_){
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
GearSenderApp::GearSenderApp() : sdt_(this)
#else
GearSenderApp::GearSenderApp(int argc, char **argv)
#endif //NS_DIFFUSION
{
  last_seq_sent_ = 0;
  num_subscriptions_ = 0;

  mr_ = new GearSenderReceive(this);

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  readGeographicCoordinates();
  dr_ = NR::createNR(diffusion_port_);
#endif //!NS_DIFFUSION
}

#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  GearSenderApp *app;

  app = new GearSenderApp(argc, argv);
  app->run();

  return 0;
}
#endif //!USE_SINGLE_ADDRESS_SPACE
