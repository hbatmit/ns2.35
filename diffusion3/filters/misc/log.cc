//
// log.cc         : Log Filter
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: log.cc,v 1.3 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "log.hh"

char *msg_types[] = {"INTEREST", "POSITIVE REINFORCEMENT",
		     "NEGATIVE REINFORCEMENT", "DATA",
		     "EXPLORATORY DATA", "PUSH EXPLORATORY DATA",
		     "CONTROL", "REDIRECT"};

#ifdef NS_DIFFUSION
static class LogFilterClass : public TclClass {
public:
  LogFilterClass() : TclClass("Application/DiffApp/LogFilter") {}
  TclObject * create(int , const char*const* ) {
    return(new LogFilter());
  }
} class_log_filter;

int LogFilter::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return TCL_OK;
    }
  }
  return DiffApp::command(argc, argv);
}
#endif // NS_DIFFUSION

void LogFilterReceive::recv(Message *msg, handle h)
{
  app_->recv(msg, h);
}

void LogFilter::recv(Message *msg, handle h)
{
  if (h != filter_handle_){
    DiffPrint(DEBUG_ALWAYS,
	      "Error: recv received message for handle %d when subscribing to handle %d !\n", h, filter_handle_);
    return;
  }

  ProcessMessage(msg);

  ((DiffusionRouting *)dr_)->sendMessage(msg, h);
}

void LogFilter::ProcessMessage(Message *msg)
{
  DiffPrint(DEBUG_ALWAYS, "Received a");

  if (msg->new_message_)
    DiffPrint(DEBUG_ALWAYS, " new ");
  else
    DiffPrint(DEBUG_ALWAYS, "n old ");

  if (msg->last_hop_ != LOCALHOST_ADDR)
    DiffPrint(DEBUG_ALWAYS, "%s message from node %d, %d bytes\n",
	      msg_types[msg->msg_type_], msg->last_hop_,
	      CalculateSize(msg->msg_attr_vec_));
  else
    DiffPrint(DEBUG_ALWAYS, "%s message from agent %d, %d bytes\n",
	      msg_types[msg->msg_type_], msg->source_port_,
	      CalculateSize(msg->msg_attr_vec_));
}

handle LogFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // This is a dummy attribute for filtering that matches everything
  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs, LOG_FILTER_PRIORITY,
					   filter_callback_);

  ClearAttrs(&attrs);
  return h;
}

void LogFilter::run()
{
#ifdef NS_DIFFUSION
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_ALWAYS, "Log filter subscribed to *, received handle %d\n",
	    filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "Log filter initialized !\n");
#else
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
#endif // NS_DIFFUSION
}

#ifdef NS_DIFFUSION
LogFilter::LogFilter()
#else
LogFilter::LogFilter(int argc, char **argv)
#endif // NS_DIFFUSION
{
  // Create Diffusion Routing class
#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION

  filter_callback_ = new LogFilterReceive(this);

#ifndef NS_DIFFUSION
  // Set up the filter
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_ALWAYS, "Log filter subscribed to *, received handle %d\n", filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "Log filter initialized !\n");
#endif // !NS_DIFFUSION
}

#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  LogFilter *app;

  // Initialize and run the Log Filter
  app = new LogFilter(argc, argv);
  app->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE
