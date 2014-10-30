//
// ping_receiver.cc : Ping Receiver Main File
// author           : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: 2pp_ping_receiver.cc,v 1.4 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "2pp_ping_receiver.hh"

#ifdef NS_DIFFUSION
static class TPPPingReceiverAppClass : public TclClass {
public:
    TPPPingReceiverAppClass() : TclClass("Application/DiffApp/PingReceiver/TPP") {}
    TclObject* create(int , const char*const* ) {
	    return(new TPPPingReceiverApp());
    }
} class_ping_receiver;

int TPPPingReceiverApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "subscribe") == 0) {
      run();
      return TCL_OK;
    }
   }
  return DiffApp::command(argc, argv);
}

#endif // NS_DIFFUSION

void TPPPingReceiverReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app_->recv(data, my_handle);
}

void TPPPingReceiverApp::recv(NRAttrVec *data, NR::handle )
{
  NRSimpleAttribute<int> *counterAttr = NULL;
  NRSimpleAttribute<void *> *timeAttr = NULL;
  EventTime *probe_event;
  long delay_seconds;
  long delay_useconds;
  float total_delay;
  struct timeval tmv;

  GetTime(&tmv);

  counterAttr = AppCounterAttr.find(data);
  timeAttr = TimeAttr.find(data);

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

handle TPPPingReceiverApp::setupSubscription()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::TWO_PHASE_PULL_ALGORITHM));
  attrs.push_back(LatitudeAttr.make(NRAttribute::GT, 54.78));
  attrs.push_back(LongitudeAttr.make(NRAttribute::LE, 87.32));
  attrs.push_back(TargetAttr.make(NRAttribute::EQ, "F117A"));

  handle h = dr_->subscribe(&attrs, mr_);

  ClearAttrs(&attrs);

  return h;
}

void TPPPingReceiverApp::run()
{
  subHandle_ = setupSubscription();

#ifndef NS_DIFFUSION
  // Do nothing
  while (1){
    sleep(1000);
  }
#endif // !NS_DIFFUSION
}

#ifdef NS_DIFFUSION
TPPPingReceiverApp::TPPPingReceiverApp()
#else
TPPPingReceiverApp::TPPPingReceiverApp(int argc, char **argv)
#endif // NS_DIFFUSION
{
  last_seq_recv_ = 0;
  num_msg_recv_ = 0;
  first_msg_recv_ = -1;

  mr_ = new TPPPingReceiverReceive(this);

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  TPPPingReceiverApp *app;

  app = new TPPPingReceiverApp(argc, argv);
  app->run();

  return 0;
}
#endif // !NS_DIFFUSION
