//
// srcrt.cc       : Source Route Filter
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the Unversity of Southern California
// $Id: srcrt.cc,v 1.5 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "srcrt.hh"

#ifdef NS_DIFFUSION
class DiffAppAgent;
#endif // NS_DIFFUSION

#ifdef NS_DIFFUSION
static class SourceRouteFilterClass : public TclClass {
public:
  SourceRouteFilterClass() : TclClass("Application/DiffApp/SourceRouteFilter") {}
  TclObject* create(int , const char*const* ) {
    return(new SrcRtFilter());
  }
} class_source_route_filter;

int SrcRtFilter::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return (TCL_OK);
    }
  }
  return (DiffApp::command(argc, argv));
}
#endif // NS_DIFFUSION

void SrcRtFilterReceive::recv(Message *msg, handle h)
{
  app_->recv(msg, h);
}

void SrcRtFilter::recv(Message *msg, handle h)
{
  Message *return_msg = NULL;

  if (h != filter_handle_){
    DiffPrint(DEBUG_ALWAYS, "Error: Received a message for handle %ld when subscribing to handle %ld !\n", h, filter_handle_);
    return;
  }

  return_msg = ProcessMessage(msg);

  if (return_msg){
    ((DiffusionRouting *)dr_)->sendMessage(msg, h);

    delete msg;
  }
}

Message * SrcRtFilter::ProcessMessage(Message *msg)
{
  char *original_route, *new_route, *p;
  int len;
  int32_t next_hop;
  NRSimpleAttribute<char *> *route = NULL;

  route = SourceRouteAttr.find(msg->msg_attr_vec_);
  if (!route){
    DiffPrint(DEBUG_ALWAYS, "Error: Can't find the route attribute !\n");
    return msg;
  }

  original_route = route->getVal();
  len = strlen(original_route);

  // Check if we are the last hop
  if (len == 0)
    return msg;

  // Get the next hop
  next_hop = atoi(original_route);

  // Remove last hop from source route
  p = strstr(original_route, ":");
  if (!p){
    // There's just one more hop
    new_route = new char[1];
    new_route[0] = '\0';
  }
  else{
    p++;
    len = strlen(p);
    new_route = new char[(len + 1)];
    strncpy(new_route, p, (len + 1));
    if (new_route[len] != '\0')
      DiffPrint(DEBUG_ALWAYS, "Warning: String must end with NULL !\n");
  }

  route->setVal(new_route);

  // Free memory
  delete [] new_route;

  // Send the packet to the next hop
  msg->next_hop_ = next_hop;
  ((DiffusionRouting *)dr_)->sendMessage(msg, filter_handle_);

  delete msg;

  return NULL;
}

handle SrcRtFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // Match all packets with a SourceRoute Attribute
  attrs.push_back(SourceRouteAttr.make(NRAttribute::EQ_ANY, ""));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs, SRCRT_FILTER_PRIORITY,
					   filter_callback_);

  ClearAttrs(&attrs);
  return h;
}

void SrcRtFilter::run()
{
#ifdef NS_DIFFUSION
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_ALWAYS, "SrcRtFilter filter received handle %ld\n",
	    filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "SrcRtFilter filter initialized !\n");
#else
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
#endif // NS_DIFFUSION
}

#ifdef NS_DIFFUSION
SrcRtFilter::SrcRtFilter()
{
#else
SrcRtFilter::SrcRtFilter(int argc, char **argv)
{
#endif // NS_DIFFUSION

  // Create Diffusion Routing class
#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION

  filter_callback_ = new SrcRtFilterReceive(this);

#ifndef NS_DIFFUSION
  // Set up the filter
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_ALWAYS, "SrcRtFilter filter received handle %ld\n",
	    filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "SrcRtFilter filter initialized !\n");
#endif // !NS_DIFFUSION
}

#ifndef NS_DIFFUSION
#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  SrcRtFilter *app;

  // Initialize and run the Source Route Filter
  app = new SrcRtFilter(argc, argv);
  app->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE
#endif // !NS_DIFFUSION
