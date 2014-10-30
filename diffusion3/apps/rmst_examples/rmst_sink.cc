//
// rmst_sink.cc    : RmstSink Class Methods
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_sink.cc,v 1.4 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "rmst_sink.hh"

#ifdef NS_DIFFUSION
static class RmstSnkClass : public TclClass {
public:
  RmstSnkClass() : TclClass("Application/DiffApp/RmstSink") {}
    TclObject* create(int , const char*const* ) {
	    return(new RmstSink());
    }
} class_rmst_sink;

int RmstSink::command(int argc, const char*const* argv) 
{
  if (argc == 2) {
    if (strcmp(argv[1], "subscribe") == 0) {
      run();
      return TCL_OK;
    }
   }
  return DiffApp::command(argc, argv);
}

#endif // NS_DIFFUSION

void RmstSnkReceive::recv(NRAttrVec *data, NR::handle )
{
  NRSimpleAttribute<int> *rmst_id_attr = NULL;
  NRSimpleAttribute<void *> *data_buf_attr = NULL;
  timeval cur_time;
  int rmst_no;
  int size;
  void *blob_ptr;

  printf("RMST-SNK::recv callback got an NRAttrVec.\n");
  GetTime (&cur_time);
  printf("  time: sec = %d\n", (unsigned int) cur_time.tv_sec);

  rmst_id_attr = RmstIdAttr.find(data);
  data_buf_attr = RmstDataAttr.find(data);

  rmst_no = rmst_id_attr->getVal();
  blob_ptr = data_buf_attr->getVal();
  size = data_buf_attr->getLen();

  printf("  Got a blob, rmstId = %d, size = %d\n", rmst_no, size);
  snk_->recv((void *)blob_ptr, size);
}

void RmstSink::run() {
  rmst_handle_ = setupInterest();
#ifndef NS_DIFFUSION
  while (1){
      sleep (1000);
  }
#endif // !NS_DIFFUSION
}  

void RmstSink::recv(void *blob, int size) {
  int i;
  char *tmpPtr = (char*)blob;
  printf("  Sink received a large blob - size = %d\n", size);
  for(i=0; i<size; i+=50){
    printf("%s\n", &tmpPtr[i]);
  }
  no_rec_++;
  if(no_rec_ >= blobs_to_rec_){
    printf("  Sink unsubscribes\n");
    dr_->unsubscribe(rmst_handle_);
  }
}

handle RmstSink::setupInterest()
{
  NRAttrVec attrs;

  printf("RMST-SNK::subscribing to all PCM_SAMPLEs\n");

  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));
  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::EQ, RMST_RESP));
  attrs.push_back(RmstTargetAttr.make(NRAttribute::EQ, "PCM_SAMPLE"));

  handle h = dr_->subscribe(&attrs, mr);

  ClearAttrs(&attrs);
  return h;
}

#ifdef NS_DIFFUSION
RmstSink::RmstSink() : blobs_to_rec_(4) {
#else
RmstSink::RmstSink(int argc, char **argv) : blobs_to_rec_(4) {
#endif // NS_DIFFUSION
  
  mr = new RmstSnkReceive(this);  
  
#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION
  no_rec_ = 0;
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv){
  RmstSink *app;

  app = new RmstSink(argc, argv);
  app->run();
  return 0;
}
#endif // !NS_DIFFUSION
