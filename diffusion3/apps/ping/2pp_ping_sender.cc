//
// ping_sender.cc : Ping Server Main File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: 2pp_ping_sender.cc,v 1.5 2011/10/02 22:32:34 tom_henderson Exp $
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

#include "2pp_ping_sender.hh"
#include <unistd.h>

#ifdef NS_DIFFUSION
static class TPPPingSenderAppClass : public TclClass {
public:
  TPPPingSenderAppClass() : TclClass("Application/DiffApp/PingSender/TPP") {}
  TclObject* create(int , const char*const*) {
    return(new TPPPingSenderApp());
  }
} class_ping_sender;

void TPPPingSendDataTimer::expire(Event *) {
  a_->send();
}

void TPPPingSenderApp::send()
{
  struct timeval tmv;

  // Send data if we have active subscriptions
  if (num_subscriptions_ > 0){
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

int TPPPingSenderApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "publish") == 0) {
      run();
      return TCL_OK;
    }
  }
  return DiffApp::command(argc, argv);
}
#endif // NS_DIFFUSION

void TPPPingSenderReceive::recv(NRAttrVec *data, NR::handle my_handle)
{
  app_->recv(data, my_handle);
}

void TPPPingSenderApp::recv(NRAttrVec *data, NR::handle )
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

handle TPPPingSenderApp::setupSubscription()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::NE, NRAttribute::DATA_CLASS));
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::TWO_PHASE_PULL_ALGORITHM));
  attrs.push_back(NRScopeAttr.make(NRAttribute::IS, NRAttribute::NODE_LOCAL_SCOPE));
  attrs.push_back(TargetAttr.make(NRAttribute::IS, "F117A"));
  attrs.push_back(LatitudeAttr.make(NRAttribute::IS, 60.00));
  attrs.push_back(LongitudeAttr.make(NRAttribute::IS, 54.00));

  handle h = dr_->subscribe(&attrs, mr_);

  ClearAttrs(&attrs);

  return h;
}

handle TPPPingSenderApp::setupPublication()
{
  NRAttrVec attrs;

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::DATA_CLASS));
  attrs.push_back(NRAlgorithmAttr.make(NRAttribute::IS, NRAttribute::TWO_PHASE_PULL_ALGORITHM));
  attrs.push_back(LatitudeAttr.make(NRAttribute::IS, 60.00));
  attrs.push_back(LongitudeAttr.make(NRAttribute::IS, 54.00));
  attrs.push_back(TargetAttr.make(NRAttribute::IS, "F117A"));

  handle h = dr_->publish(&attrs);

  ClearAttrs(&attrs);

  return h;
}

void TPPPingSenderApp::run()
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
  subHandle_ = setupSubscription();
  pubHandle_ = setupPublication();

  // Create time attribute
  GetTime(&tmv);
  lastEventTime_ = new EventTime;
  lastEventTime_->seconds_ = tmv.tv_sec;
  lastEventTime_->useconds_ = tmv.tv_usec;
  timeAttr_ = TimeAttr.make(NRAttribute::IS, (void *) &lastEventTime_,
			    sizeof(EventTime));
  data_attr_.push_back(timeAttr_);

  // Change pointer to point to attribute's payload
  delete lastEventTime_;
  lastEventTime_ = (EventTime *) timeAttr_->getVal();

  // Create counter attribute
  counterAttr_ = AppCounterAttr.make(NRAttribute::IS, last_seq_sent_);
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
    if (num_subscriptions_ > 0){
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

#ifdef NS_DIFFUSION
TPPPingSenderApp::TPPPingSenderApp() : sdt_(this)
#else
TPPPingSenderApp::TPPPingSenderApp(int argc, char **argv)
#endif // NS_DIFFUSION
{
  last_seq_sent_ = 0;
  num_subscriptions_ = 0;

  mr_ = new TPPPingSenderReceive(this);

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  TPPPingSenderApp *app;

  app = new TPPPingSenderApp(argc, argv);
  app->run();

  return 0;
}
#endif // NS_DIFFUSION
