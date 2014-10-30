//
// tag.cc         : Tag Filter
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the Unversity of Southern California
// $Id: tag.cc,v 1.3 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "tag.hh"

//TagFilter *app;

#ifdef NS_DIFFUSION
static class TagFilterClass : public TclClass {
public:
  TagFilterClass() : TclClass("Application/DiffApp/TagFilter") {}
  TclObject * create(int , const char*const* ) {
    return(new TagFilter());
  }
} class_tag_filter;

int TagFilter::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return (TCL_OK);
    }
  }
  return (DiffApp::command(argc, argv));
}
#endif // NS_DIFFUSION

void TagFilterReceive::recv(Message *msg, handle h)
{
  app_->recv(msg, h);
}

void TagFilter::recv(Message *msg, handle h)
{
  if (h != filter_handle_){
    DiffPrint(DEBUG_ALWAYS, "Error: TagFilter::recv received message for handle %ld when subscribing to handle %ld !\n", h, filter_handle_);
    return;
  }

  ProcessMessage(msg);

  ((DiffusionRouting *)dr_)->sendMessage(msg, h);
}

void TagFilter::ProcessMessage(Message *msg)
{
  char *original_route;
  char *new_route;
  int len, total_len;
  NRSimpleAttribute<char *> *route = NULL;

  // Can't do anything if node id unknown
  if (!id_)
    return;

  route = RouteAttr.find(msg->msg_attr_vec_);
  if (!route){
    DiffPrint(DEBUG_ALWAYS, "Error: Can't find the route attribute !\n");
    return;
  }

  original_route = route->getVal();
  len = strlen(original_route);

  if (len == 0){
    total_len = strlen(id_);
    // Route is empty, need to allocate memory
    // for our id and the terminating '\0'
    new_route = new char[(total_len + 1)];
    strcpy(new_route, id_);
    if (new_route[total_len] != '\0')
      DiffPrint(DEBUG_ALWAYS, "Warning: String must end with NULL !\n");
  }
  else{
    // Route already exists. We need to allocate
    // memory for the current route + ':' + our
    // id + the terminating '\0'
    total_len = len + strlen(id_) + 1;
    new_route = new char[(total_len + 1)];
    strcpy(new_route, original_route);
    new_route[len] = ':';
    strcpy(&new_route[len+1], id_);
    if (new_route[total_len] != '\0'){
      DiffPrint(DEBUG_ALWAYS, "Warning: String must end with NULL !\n");
    }
  }

  // Debug
  DiffPrint(DEBUG_ALWAYS, "Tag Filter: Original route : %s\n", original_route);
  DiffPrint(DEBUG_ALWAYS, "Tag Filter: New route : %s\n", new_route);

  route->setVal(new_route);

  // Free memory
  delete [] new_route;
}

handle TagFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // Match all packets with a Route Attribute
  attrs.push_back(RouteAttr.make(NRAttribute::EQ_ANY, ""));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs, TAG_FILTER_PRIORITY,
					   filter_callback_);

  ClearAttrs(&attrs);
  return h;
}

void TagFilter::run()
{
#ifdef NS_DIFFUSION
  // Set up the filter
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_ALWAYS, "Tag Filter subscribed to *, received handle %d\n",
	    (int) filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "Tag Filter initialized !\n");
#else
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
#endif // NS_DIFFUSION
}

void TagFilter::getNodeId()
{
  DiffPrint(DEBUG_ALWAYS, "Tag Filter: getNodeID function not yet implemented !\n");
  DiffPrint(DEBUG_ALWAYS, "Tag Filter: Please set scadds_addr to the node id !\n");
  exit(-1);
  // Future implementation for the inter-module API
}

#ifdef NS_DIFFUSION
TagFilter::TagFilter()
#else
TagFilter::TagFilter(int argc, char **argv)
#endif // NS_DIFFUSION
{
  char *id_env = NULL;
  char buffer[BUFFER_SIZE];
  int flag;
  int node_id;

  id_ = NULL;
  node_id = 0;

  // Create Diffusion Routing class
#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION

  filter_callback_ = new TagFilterReceive(this);

  // Try to figure out the node ID
  id_env = getenv("scadds_addr");
  if (id_env){
    node_id = atoi(id_env);
    flag = snprintf(&buffer[0], BUFFER_SIZE, "%d", node_id);
    if (flag == -1 || flag == BUFFER_SIZE){
      DiffPrint(DEBUG_ALWAYS, "Error: Buffer too small !\n");
      exit(-1);
    }
    id_ = strdup(&buffer[0]);
  }
  else{
    getNodeId();
  }
#ifndef NS_DIFFUSION
  // Set up the filter
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_ALWAYS, "Tag Filter subscribed to *, received handle %ld\n",
	  filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "Tag Filter initialized !\n");
#endif // !NS_DIFFUSION
}

#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  TagFilter *app;

  // Initialize and run the Tag Filter
  app = new TagFilter(argc, argv);
  app->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE
