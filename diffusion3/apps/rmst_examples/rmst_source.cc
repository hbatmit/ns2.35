//
// rmst_source.cc  : RmstSource Class Methods
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_source.cc,v 1.6 2011/10/02 22:32:34 tom_henderson Exp $
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

#include "rmst_source.hh"
#include <unistd.h>

#ifdef NS_DIFFUSION
static class RmstSrcClass : public TclClass {
public:
  RmstSrcClass() : TclClass("Application/DiffApp/RmstSource") {}
    TclObject* create(int , const char*const* ) {
	    return(new RmstSource());
    }
} class_rmst_source;

void RmstSendDataTimer::expire(Event *) {
  a_->send();
}

void RmstSource::send()
{
  int sleep_interval;
  bool sent_first_blob = false;

  if (num_subscriptions_ > 0){
      if (!sent_first_blob){
        sendBlob();
        sent_first_blob = true;
        sleep_interval = 100;
      }
      else
        printf("RMST-SRC::sees subscriptions\n");
    }
    else{
      printf("RMST-SRC::sees no subscriptions\n");
      sleep_interval = 10;
    }
  // re-schedule the timer 
  sdt_.resched(sleep_interval);
}

int RmstSource::command(int argc, const char*const* argv) 
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


void RmstSrcReceive::recv(NRAttrVec *data, NR::handle )
{
  NRSimpleAttribute<char*> *rmst_target_attr = NULL;
  NRSimpleAttribute<int> *nr_class = NULL;
  NRSimpleAttribute<int> *tsprt_ctl_attr = NULL;

  timeval cur_time;

  printf("RMST-SRC::recv got an attr vector.");
  GetTime(&cur_time);
  printf("  time: sec = %d\n", (unsigned int) cur_time.tv_sec);

  nr_class = NRClassAttr.find(data);
  tsprt_ctl_attr = RmstTsprtCtlAttr.find(data);

  if (nr_class){
    switch (nr_class->getVal()){

    case NRAttribute::INTEREST_CLASS:
      if (tsprt_ctl_attr && (tsprt_ctl_attr->getVal() == RMST_RESP)){
        printf("  Source received an INTEREST message\n");
        src_->num_subscriptions_++;
      }
      break;

    case NRAttribute::DISINTEREST_CLASS:
      src_->num_subscriptions_--;
      rmst_target_attr = RmstTargetAttr.find(data);
      if (rmst_target_attr){
        printf("  Source received a DISINTEREST for %s\n",
            rmst_target_attr->getVal());
      }
      else
        printf("  Source received a Disinterest message for unknown Interest!\n");
      break;

    default:
      printf("  Source received an unknown or inappropriate class!(%d)!\n",
          nr_class->getVal());
      break;
    }
  }

  if (tsprt_ctl_attr){
    switch (tsprt_ctl_attr->getVal()){
    case RMST_RESP:
      break;
    case RMST_CONT:
      printf("  Source received a RMST_CONT message\n");
      if(src_->blobs_to_send_ > 0){
        printf ("  Source sending another blob\n");
        src_->sendBlob();
      }
      else
        printf ("  Source done sending blobs\n");
      break;
    default:
      printf("  Source received an unexpected RmstTsprtCtlAttr (%d)!\n",
           tsprt_ctl_attr->getVal());
      break;
    }
  }
}

#ifdef NS_DIFFUSION
RmstSource::RmstSource() : blobs_to_send_(4), sdt_(this)
#else
RmstSource::RmstSource(int argc, char **argv) : blobs_to_send_(4)
#endif // NS_DIFFUSION
{
  mr = new RmstSrcReceive(this);

#ifndef NS_DIFFUSION
  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION
  
  ck_val_ = 100;
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  RmstSource *app;

  app = new RmstSource(argc, argv);
  app->run();

  return 0;
}
#endif // NS_DIFFUSION

void RmstSource::run()
{
#ifndef NS_DIFFUSION
  int sleep_interval;
  bool sent_first_blob = false;
#endif // !NS_DIFFUSION
  
  // Let diffusion know what kinds of interests we "latch." 
  subscribe_handle_ = setupRmstInterest();
  // Let diffusion know what we intend to publish.
  send_handle_ = setupRmstPublication();

#ifndef NS_DIFFUSION
  while(1){
    if (num_subscriptions_ > 0){
      if (!sent_first_blob){
        sendBlob();
        sent_first_blob = true;
        sleep_interval = 100;
      }
      else
        printf("RMST-SRC::sees subscriptions\n");
    }
    else{
      printf("RMST-SRC::sees no subscriptions\n");
      sleep_interval = 10;
    }
    sleep(sleep_interval);
  } //while loop
#else
  send();
#endif // !NS_DIFFUSION
}


handle RmstSource::setupRmstInterest()
{
  NRAttrVec attrs;

  printf("RMST-SRC::sets up local subscription for PCM_SAMPLEs\n");
  attrs.push_back(NRClassAttr.make(NRAttribute::NE, NRAttribute::DATA_CLASS));
  attrs.push_back(NRScopeAttr.make(NRAttribute::IS,
				   NRAttribute::NODE_LOCAL_SCOPE));
  attrs.push_back(RmstTargetAttr.make(NRAttribute::IS, "PCM_SAMPLE"));
  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, RMST_RESP));
  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, RMST_CONT));

  handle h = dr_->subscribe(&attrs, mr);
  ClearAttrs(&attrs);
  return h;
}

handle RmstSource::setupRmstPublication()
{
  NRAttrVec attrs;

  printf("RMST-SRC::publishes PCM_SAMPLE\n");
  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::DATA_CLASS));
  attrs.push_back(RmstTargetAttr.make(NRAttribute::IS, "PCM_SAMPLE"));

  handle h = dr_->publish(&attrs);
  ClearAttrs(&attrs);
  return h;
}


char* RmstSource::createBlob (int ck_val)
{
  char *tmpPtr = new char[2500];

  for (int i = 0; i < 50; i++){
    sprintf(&tmpPtr[i*50], "PCM FragNo: %d of ck_val %d", i, ck_val); 
  }
  return tmpPtr;
}

void RmstSource::sendBlob() {
  char *blob;
  NRAttrVec src_attrs;

  // Retrieve rmsb from the local cache to get pointer to it.
  blob = createBlob(ck_val_);
  ck_val_++;
  src_attrs.push_back(RmstDataAttr.make(NRAttribute::IS, blob, 2500));
  ((DiffusionRouting *)dr_)->sendRmst(send_handle_,
					       &src_attrs, PAYLOAD_SIZE);
  blobs_to_send_--;
  delete blob;
}

void RmstSource::recv(Message *, handle )
{}
