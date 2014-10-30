//
// rmst_filter.cc  : RmstFilter Class Methods
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_filter.cc,v 1.4 2010/03/08 05:54:49 tom_henderson Exp $
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

#include "rmst_filter.hh"

char *rmstmsg_types[] = {"INTEREST", "POSITIVE REINFORCEMENT",
                     "NEGATIVE REINFORCEMENT", "DATA",
                     "EXPLORATORY DATA", "PUSH EXPLORATORY DATA",
                     "CONTROL", "REDIRECT"};

#ifdef NS_DIFFUSION
class DiffAppAgent;
#endif // NS_DIFFUSION

#ifdef NS_DIFFUSION
static class RmstFilterClass : public TclClass {
public:
  RmstFilterClass() : TclClass("Application/DiffApp/RmstFilter") {}
  TclObject* create(int , const char*const* ) {
    return(new RmstFilter());
  }
} class_rmst_filter;

int RmstFilter::command(int argc, const char*const* argv) {
  //Tcl& tcl =  Tcl::instance();
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return (TCL_OK);
    }
  }
  return (DiffApp::command(argc, argv));
}
#endif // NS_DIFFUSION

class ReinfMessage {
public:
  int32_t rdm_id_;
  int32_t pkt_num_;
};


// RmstFilterCallback::recv
// Called by diffusion core when a message is available for this filter.
// RmstFilterCallback is derived from the abstract class FilterCallback.
// A pointer to the FilterCallback class is required in the API method "addFilter."

void RmstFilterCallback::recv(Message *msg, handle h)
{
  app_->recv(msg, h);
}


// RmstFilter::recv
//
// Called by the Callback::recv method.

void RmstFilter::recv(Message *msg, handle h)
{
  // Process the message handed to us by the core.
  // If true is returned we forward the message. Otherwise it dies here.
  if(processMessage(msg))
    ((DiffusionRouting *)dr_)->sendMessage(msg, h);
}

// RmstFilter::processMessage
//
// Called by the RmstFilter::recv method when this filter gets a message.

bool RmstFilter::processMessage(Message *msg)
{
  NRSimpleAttribute<int> *rmst_id_attr = NULL;
  NRSimpleAttribute<int> *frag_attr = NULL;
  NRSimpleAttribute<int> *pkts_sent_attr = NULL;
  NRSimpleAttribute<int> *tsprt_ctl_attr = NULL;
  NRSimpleAttribute<void *> *reinf_attr = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;
  NRSimpleAttribute<int> *nr_class = NULL;
  NRAttrVec *data;

  Key2ExpLog::iterator exp_iterator;
  Int2Rmst::iterator rmst_iterator;
  int rmst_no;
  int frag_no;
  int class_type;
  int rmst_ctl_type;
  union LlToInt key;
  Rmst *rmst_ptr;

  // If this is a message that uses the transport layer, we process it.
  // Otherwise we send it back to the core (by returning true).
  tsprt_ctl_attr = RmstTsprtCtlAttr.find(msg->msg_attr_vec_);
  if (!tsprt_ctl_attr){
    DiffPrint(DEBUG_SOME_DETAILS, "RmstFilter got non-transport message\n");
    return true;
  }
  rmst_ctl_type = tsprt_ctl_attr->getVal();

  DiffPrint(DEBUG_IMPORTANT, "RmstFilter::processMessage got a");

  if (msg->new_message_)
    DiffPrint(DEBUG_IMPORTANT, " new (%d) ", msg->msg_type_);
  else
    DiffPrint(DEBUG_IMPORTANT, "n old (%d) ", msg->msg_type_);

  if (msg->last_hop_ != LOCALHOST_ADDR)
    DiffPrint(DEBUG_IMPORTANT, "%s message from %d to %d\n",
      rmstmsg_types[msg->msg_type_],
      msg->last_hop_, msg->next_hop_);
  else
    DiffPrint(DEBUG_IMPORTANT, "%s message from local agent\n",
      rmstmsg_types[msg->msg_type_]);

  // We only care about messages we haven't seen before,
  // but we generally let other filters get them (because they may need them).
  // However, if this is an old DATA message arriving at a sink, the sink may
  // negatively reinforce a reinforced path.  This is because we withold the 
  // new messages until we get the entire blob.  The old message is the result
  // of a lost ACK when using SMAC with ARQ.
  if (!msg->new_message_ && msg->msg_type_ == DATA
      && rmst_ctl_type == RMST_RESP){
    DiffPrint(DEBUG_SOME_DETAILS, 
      "  Sink got an old DATA message from node %d\n", msg->last_hop_);
    data = msg->msg_attr_vec_;
    rmst_id_attr = RmstIdAttr.find(data);
    if (!rmst_id_attr){
      DiffPrint(DEBUG_SOME_DETAILS,
        "  Filter received a bad transport packet!\n");
      return false;
    }
    rmst_no = rmst_id_attr->getVal();
    // Find the rmst.
    rmst_iterator = rmst_map_.find(rmst_no);
    if(rmst_iterator == rmst_map_.end()){
      DiffPrint(DEBUG_IMPORTANT,
        "  couldn't find DB entry for Rmst %d\n", rmst_no);
      return false;
    }
    else{
      rmst_ptr = (*rmst_iterator).second;
      if ( (local_sink_) && (msg->last_hop_ == rmst_ptr->last_hop_) ){
        // This is the case where SMAC sent the same DATA message twice to a sink.
        // We suppress this message so we don't kill our reinforced path.
        DiffPrint(DEBUG_IMPORTANT,
          "  We suppress old DATA message from smac retransmission!\n");
        return false;
      }
      else
        return true;
    }
  }
  else if (!msg->new_message_)
    return true;

  // When we get Rmst Fragments we must sync the local cache!
  if ( (rmst_ctl_type == RMST_RESP) && 
       ((msg->msg_type_ == DATA) || (msg->msg_type_ == EXPLORATORY_DATA)) ){
    rmst_ptr = syncLocalCache(msg);
    // syncLocalCache will return NULL if the 
    // attribute set doesn't make sense.
    if (rmst_ptr == NULL)
      return false;
    rmst_no = rmst_ptr->rmst_no_;
    // Mark the time we got some kind of data.
    GetTime (&last_data_rec_);
  }

  //  New exploratory messages are entered into the exp_map_,
  //  so that we can find the last hop if it gets reinforced.
  //  Positive reinforcement messages are used to find the 
  //  corresponding message in the exp_map_, so we know the
  //  current reinforced path to the source of an rmst.
  switch (msg->msg_type_){

    case(EXPLORATORY_DATA):
      ExpLog exp_msg;

      DiffPrint(DEBUG_LOTS_DETAILS,
        "  Exploratory_Msg: ptk_num = %x, rdm_id_ = %x, last_hop = %d\n",
        msg->pkt_num_, msg->rdm_id_, msg->last_hop_);

      // Put the ID for this Exploratory message, along with its last hop,
      // into the exp_map_.  If this message gets reinforced, we will be
      // able to identify the next hop in the back channel.
      DiffPrint(DEBUG_SOME_DETAILS,
        "  Exploratory message for Reliable transport Id = %d\n", rmst_no);
      key.int_val_[0] = msg->pkt_num_;
      key.int_val_[1] = msg->rdm_id_;
      DiffPrint(DEBUG_LOTS_DETAILS, "  Key = %llx\n", key.ll_val_);
      exp_msg.rmst_no_ = rmst_no;
      exp_msg.last_hop_ = msg->last_hop_;
      exp_map_.insert(Key2ExpLog::value_type(key.ll_val_, exp_msg));

      // If this is a new exploratory message arriving at a sink,
      // we assume that this path will get reinforced by the 
      // gradient filter. Sinks don't get positive reinforcement
      // messages, so we must record last_hop_ now.
      if (local_sink_){
        rmst_ptr->last_hop_ = msg->last_hop_; 
        if (rmst_ptr->reinf_){
          DiffPrint(DEBUG_IMPORTANT, "  got a new path exploratory msg at sink.\n");
          rmst_ptr->wait_for_new_path_ = true;
        }
        else{
          rmst_ptr->reinf_ = true;
          DiffPrint(DEBUG_IMPORTANT, "  got an initial exploratory msg at sink.\n");
        }
        DiffPrint(DEBUG_IMPORTANT, "  set last_hop for rmst %d to %d\n",
          rmst_no, rmst_ptr->last_hop_);
        rmst_ptr->pkts_rec_ = 0; 
        rmst_ptr->last_hop_pkts_sent_ = 0; 
      }
      else{
        // If this is not a sink we reset the base fragment that
        // we look for holes from.
        DiffPrint(DEBUG_LOTS_DETAILS,
          "  intermediate node resets sync_base_ and reinf_.\n");
        frag_attr = RmstFragAttr.find(msg->msg_attr_vec_);
        frag_no = frag_attr->getVal();
        rmst_ptr->sync_base_ = frag_no;
        if(rmst_ptr->reinf_)
          rmst_ptr->reinf_ = false;
        rmst_ptr->last_hop_ = 0;
        rmst_ptr->pkts_sent_ = 0;
        rmst_ptr->pkts_rec_ = 0;
        rmst_ptr->last_hop_pkts_sent_ = 0; 
        rmst_ptr->naks_rec_ = 0; 
      }

      // If this is not a sink and a watchdog timer is active, we cancel
      // it because we may not end up on the new reinforced path. We
      // don't want to look for fragments that will never arrive.
      if ((rmst_ptr->watchdog_active_) && (!local_sink_) 
        && (!rmst_ptr->local_source_)){
        rmst_ptr->cancel_watchdog_ = true;
        rmst_ptr->cleanHoleMap();
      }

      // We always forward exploratory data.
      return(true);
      break;

    case(DATA):

      if (rmst_ctl_type != RMST_RESP){
        processCtrlMessage(msg);
        // We don't let Rmst control messages go to the gradient or other filters.
        return false;
      }

      // We have a normal DATA packet.
      rmst_ptr->pkts_rec_++;

      // If we got the upstream send count - update it in Rmst.
      pkts_sent_attr = RmstPktsSentAttr.find(msg->msg_attr_vec_);
      if (pkts_sent_attr){
        rmst_ptr->last_hop_pkts_sent_ = pkts_sent_attr->getVal();
        DiffPrint(DEBUG_SOME_DETAILS,
          "processMessage:: got last_hop_pkts_sent_ = %d packets\n",
          rmst_ptr->last_hop_pkts_sent_);
        if ( (rmst_ptr->last_hop_pkts_sent_ > 20) && 
             (rmst_ptr->pkts_rec_ < (rmst_ptr->last_hop_pkts_sent_ * BLACKLIST_THRESHOLD)) ){
          Blacklist::iterator black_list_iterator;
          black_list_iterator = black_list_.begin();
          while(black_list_iterator != black_list_.end()){
            if(*black_list_iterator == rmst_ptr->last_hop_)
            break;
            black_list_iterator++;
          }
          if(black_list_iterator == black_list_.end()){
            DiffPrint(DEBUG_IMPORTANT, "Adding node %d to black_list_ !!\n",
              rmst_ptr->last_hop_);
            black_list_.push_front(rmst_ptr->last_hop_);
            ((DiffusionRouting *)dr_)->addToBlacklist(rmst_ptr->last_hop_);
            // Now send an EXP_REQ!
            sendExpReqUpstream(rmst_ptr);
            rmst_ptr->sent_exp_req_ = true;
            GetTime(&rmst_ptr->exp_req_time_);
            // We need to send a negative reinforcement on blacklisted link!
            Message *neg_reinf_msg;
            neg_reinf_msg = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
              0, 0, interest_attrs_->size(), pkt_count_, rdm_id_, 
              rmst_ptr->last_hop_, LOCALHOST_ADDR);
            neg_reinf_msg->msg_attr_vec_ = CopyAttrs(interest_attrs_);
            ((DiffusionRouting *)dr_)->sendMessage(neg_reinf_msg, filter_handle_, 1);
            pkt_count_++;
            delete neg_reinf_msg;
          }
        }
      }

      // We suppress new DATA messages that don't arrive on the 
      // reinforced path. 
      if ( msg->last_hop_ != rmst_ptr->last_hop_ ){
        DiffPrint(DEBUG_IMPORTANT,
          "  We suppress new DATA message on non-backchannel path!; backchannel = %d\n",
          rmst_ptr->last_hop_);
        msg->new_message_ = 0;
        return true;
      }

      if (rmst_ptr->wait_for_new_path_){
        rmst_ptr->wait_for_new_path_ = false;
        DiffPrint(DEBUG_SOME_DETAILS, "  node resets wait_for_new_path_.\n");
      }

      if (local_sink_ && rmst_ptr->sent_exp_req_){
        DiffPrint(DEBUG_SOME_DETAILS,
          "  source got a new path, set sent_exp_req_ false.\n");
        rmst_ptr->sent_exp_req_ = false;
      }

      // We forward DATA if we aren't a source or a sink.
      // Sources collect all fragments and send them from a timer.
      // Sinks collect all fragments and send them to the app when they
      // have all arrived.
      if(rmst_ptr->local_source_ || local_sink_)
        return false;
      else{
        rmst_ptr->pkts_sent_++;
        // We need to alter the RmstPktsSentAttr to reflect this node!
        if(pkts_sent_attr)
          pkts_sent_attr->setVal(rmst_ptr->pkts_sent_);
        return true;
      }
      break;

    case(INTEREST):
      data = msg->msg_attr_vec_;
      nr_class = NRClassAttr.find(data);
      if (nr_class){
        class_type = nr_class->getVal();
        if (class_type == NRAttribute::DISINTEREST_CLASS)
          DiffPrint(DEBUG_SOME_DETAILS, "  DISINTEREST_CLASS\n");
      }

      nrscope = NRScopeAttr.find(msg->msg_attr_vec_);
      if(nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE)
        DiffPrint(DEBUG_SOME_DETAILS, "  rmst LOCAL_SCOPE Interest Message\n");
      else if (msg->last_hop_ == LOCALHOST_ADDR){
        DiffPrint(DEBUG_SOME_DETAILS, "  rmst Interest Message from local SINK\n");
        local_sink_ = true;
        local_sink_port_ = msg->source_port_;
        GetTime (&last_sink_time_);
        if (interest_attrs_ == NULL)
          interest_attrs_ = CopyAttrs(msg->msg_attr_vec_);
      }
      else{
        DiffPrint(DEBUG_SOME_DETAILS, "  rmst Interest Message from non-local node\n");
          if (interest_attrs_ == NULL)
            interest_attrs_ = CopyAttrs(msg->msg_attr_vec_);
      }
      break;

    case(POSITIVE_REINFORCEMENT):
      ReinfMessage *reinf_msg;
      ExpLog exp_log;

      DiffPrint(DEBUG_IMPORTANT, "  Positive Reinf arrived\n");
      reinf_attr = ReinforcementAttr.find(msg->msg_attr_vec_);
      reinf_msg = (ReinfMessage*)reinf_attr->getVal();
      DiffPrint(DEBUG_LOTS_DETAILS, "  Pos_Reinf: ptk_num = %x, rdm_id_ = %x\n",
        reinf_msg->pkt_num_, reinf_msg->rdm_id_);

      key.int_val_[0] = reinf_msg->pkt_num_;
      key.int_val_[1] = reinf_msg->rdm_id_;
      exp_iterator = exp_map_.find(key.ll_val_);
      if(exp_iterator != exp_map_.end()){
        exp_log = (*exp_iterator).second;
        DiffPrint(DEBUG_SOME_DETAILS, "  Reinforcement for rmst_no = %d, last_hop_ = %d\n",
          exp_log.rmst_no_, exp_log.last_hop_);

        // Here is where we must update the rmst with back-channel
        // last hop.
        rmst_no = exp_log.rmst_no_;
        rmst_iterator = rmst_map_.find(rmst_no);
        if(rmst_iterator != rmst_map_.end()){
          rmst_ptr = (*rmst_iterator).second;
          rmst_ptr->last_hop_ = exp_log.last_hop_;
          rmst_ptr->fwd_hop_ = msg->last_hop_;
          DiffPrint(DEBUG_SOME_DETAILS, "  Setting rmst_no %d last_hop_ = %d, fwd_hop_ = %d\n",
            rmst_no, rmst_ptr->last_hop_, rmst_ptr->fwd_hop_);
          if(!rmst_ptr->reinf_){
            rmst_ptr->reinf_ = true;
            if(rmst_ptr->local_source_)
              DiffPrint(DEBUG_LOTS_DETAILS, "  Local source got a Reinf\n");
          }
        }
        else{
          DiffPrint(DEBUG_IMPORTANT, "  Reinforcement cant't find rmst_no\n");
          break;
        }
        // We are on the reinforced path, so we must start a timer
        // if one hasn't already been started. Sinks don't get
        // reinforced, so they start a WATCHDOG in syncLocalCache.
        if( (rmst_ptr->watchdog_active_ == false) && (caching_mode_) ){
          TimerCallback *rmst_timer;
          DiffPrint(DEBUG_IMPORTANT,
            "  Set a WATCHDOG_TIMER at caching node for reinforced rmst_no %d\n",
            rmst_no);
          rmst_timer = new RmstTimeout(this, rmst_no, WATCHDOG_TIMER);
          // We check on things every 10 seconds.
          rmst_ptr->watchdog_handle_ = 
            ((DiffusionRouting *)dr_)->addTimer(WATCHDOG_INTERVAL, rmst_timer);
          rmst_ptr->watchdog_active_ = true;
        }
        if (rmst_ptr->wait_for_new_path_){
          DiffPrint(DEBUG_SOME_DETAILS, "  Resetting wait_for_new_path_ for rmst_no %d\n", rmst_no);
          rmst_ptr->wait_for_new_path_ = false;
        }
        if (rmst_ptr->sent_exp_req_){
          DiffPrint(DEBUG_SOME_DETAILS,
            "  intermediate node got a new path, set sent_exp_req_ false.\n");
          rmst_ptr->sent_exp_req_ = false;
        }
      }
      else{
        if(!rmst_ptr->local_source_)
          DiffPrint(DEBUG_IMPORTANT, "  Reinforcement matches no Exploratory msg\n");
      }
      break;

    case(NEGATIVE_REINFORCEMENT):
      bool ret_val;
      if (tsprt_ctl_attr){
        DiffPrint(DEBUG_SOME_DETAILS,
            "  NEGATIVE_REINFORCEMENT, last_hop_ = %d, rmst_ctl_type = %d\n", 
            msg->last_hop_, rmst_ctl_type);
      }
      // We need to check if we got a NEGATIVE REINFORCEMENT from a node that is the
      // next node in the forward direction (downstream).  If so, and we are the source
      // we must send a new EXPLORATORY message;  else if we are not the source,
      // we must send and exp request upstream.
      ret_val = true;
      rmst_iterator = rmst_map_.begin();
      while(rmst_iterator != rmst_map_.end()){
        rmst_ptr = (*rmst_iterator).second;
        DiffPrint(DEBUG_SOME_DETAILS,
            "  searching rmsts - rmst_no_ %d: fwd_hop_ = %d, reinf_ = %d, acked = %d\n",
            rmst_ptr->rmst_no_, rmst_ptr->fwd_hop_, rmst_ptr->reinf_, rmst_ptr->acked_);
        if (rmst_ptr->local_source_ && rmst_ptr->reinf_
            && (rmst_ptr->fwd_hop_ == msg->last_hop_)
            && !rmst_ptr->acked_){
          // If we are reinforced then we never got and EXP_REQ!!
          DiffPrint(DEBUG_SOME_DETAILS, "  local source sees NEG_REINF\n");
          processExpReq(rmst_ptr, rmst_ptr->max_frag_sent_);
        }
        else if (!rmst_ptr->local_source_ && (rmst_ptr->fwd_hop_ == msg->last_hop_)
                 && rmst_ptr->reinf_ && !rmst_ptr->acked_){
          DiffPrint(DEBUG_SOME_DETAILS, "  intermediate node sees NEG_REINF from reinforced node\n");
          DiffPrint(DEBUG_SOME_DETAILS, "  send Exp Request upstream!\n");
          ret_val = false;
          sendExpReqUpstream(rmst_ptr);
        }
        else{
          DiffPrint(DEBUG_SOME_DETAILS,
            "  node sees NEG_REINF from non-reinforced node - let routing layer see it\n");
	  ret_val = true;
	}
        rmst_iterator++;
      }
      if (!ret_val)
        return false;
      break;

    default:
      break;
  } // switch msg->type
  return true;
}


// RmstFilter::syncLocalCache
//
// This routine adds new transport data messages to the local data base.

Rmst* RmstFilter::syncLocalCache (Message *msg)
{
  NRSimpleAttribute<int> *rmst_id_attr = NULL;
  NRSimpleAttribute<int> *frag_attr = NULL;
  NRSimpleAttribute<int> *max_frag_attr = NULL;
  NRSimpleAttribute<void *> *data_buf_attr = NULL;
  NRAttrVec *data = msg->msg_attr_vec_;
  Int2Rmst::iterator rmst_iterator;
  int rmst_no;
  int frag_no;
  int max_frag_no;
  void *blob_ptr;
  int blob_len;
  void *tmp_frag_ptr;
  Rmst *rmst_ptr;

  rmst_id_attr = RmstIdAttr.find(data);
  frag_attr = RmstFragAttr.find(data);
  max_frag_attr = RmstMaxFragAttr.find(data);
  data_buf_attr = RmstDataAttr.find(data);

  if (! (rmst_id_attr && frag_attr && data_buf_attr) ){
    DiffPrint(DEBUG_IMPORTANT, "  Filter received a BAD transport packet!\n");
    return NULL;
  }

  rmst_no = rmst_id_attr->getVal();
  frag_no = frag_attr->getVal();
  if(max_frag_attr)
    max_frag_no = max_frag_attr->getVal();
  else
    max_frag_no = 0;
  blob_ptr = data_buf_attr->getVal();
  blob_len = data_buf_attr->getLen();

  // Here is where I consuslt the Data Base and possibly add a new map,
  // or add to an existing map.

  rmst_iterator = rmst_map_.find(rmst_no);
  if(rmst_iterator == rmst_map_.end()){
    DiffPrint(DEBUG_IMPORTANT, "  creating a new DB entry for Rmst %d\n", rmst_no);
    DiffPrint(DEBUG_SOME_DETAILS, "  Max Fragment number = %d\n", max_frag_no);
    rmst_ptr = new Rmst(rmst_no);
    rmst_ptr->max_frag_ = max_frag_no;
    rmst_map_.insert(Int2Rmst::value_type(rmst_no, rmst_ptr));

    // Artificially initialize last_nak_time_ so it's older
    // than any Naks we may get.
    GetTime(&rmst_ptr->last_nak_time_);

    // Several decisions in this routine relate to messages that emanated
    // from a local source.
    if (msg->last_hop_ == LOCALHOST_ADDR) {
      rmst_ptr->local_source_ = true;
      rmst_ptr->local_source_port_ = msg->source_port_;
      // This is the first fragment of an rmst from a local source.
      // The message will be marked exploratory by this filter.
      // We need to mark the last hop as LOCALHOST_ADDR.
      rmst_ptr->last_hop_ = LOCALHOST_ADDR;
    }
    // We need to capture the RmstTargetAttr for concantenation on sendMessage.
    if((rmst_ptr->local_source_)||(local_sink_)||(caching_mode_)){
      NRSimpleAttribute<char *> *rmst_tgt_attr = NULL;
      rmst_tgt_attr = RmstTargetAttr.find(msg->msg_attr_vec_);
      if (rmst_tgt_attr){
        char *tmp_str = rmst_tgt_attr->getVal();
        rmst_ptr->target_str_ = new char[strlen(tmp_str)+1];
        strcpy (rmst_ptr->target_str_, tmp_str);
        DiffPrint(DEBUG_IMPORTANT, "  RmstTargetAttr = %s\n", rmst_ptr->target_str_);
      }
      else
        DiffPrint(DEBUG_IMPORTANT, "  no RmstTargetAttr Rmst %d !\n", rmst_no);
    }
  }
  else
    rmst_ptr = (*rmst_iterator).second;

  if(!rmst_ptr->local_source_)
    DiffPrint(DEBUG_IMPORTANT, "  Got a blob, rmstId = %d, frag_no = %d\n", rmst_no, frag_no);

  // Update the time we last saw data for this Rmst.
  GetTime(&rmst_ptr->last_data_time_);

  // We cache the fragment at the sink and source,
  // or in caching mode at each node that receives it.
  if((rmst_ptr->local_source_)||(local_sink_)||(caching_mode_)){
    tmp_frag_ptr = rmst_ptr->getFrag(frag_no);
    if (tmp_frag_ptr == NULL){
      if(!rmst_ptr->local_source_)
        DiffPrint(DEBUG_SOME_DETAILS, "  creating a new frag %d entry for Rmst %d\n",
          frag_no, rmst_no);
      if (frag_no == rmst_ptr->max_frag_)
      rmst_ptr->max_frag_len_ = blob_len;
      tmp_frag_ptr = new char[blob_len];
      memcpy(tmp_frag_ptr, blob_ptr, blob_len);
      rmst_ptr->putFrag(frag_no, tmp_frag_ptr);

      // Check to see if this fragment was NAKed.
      // If so, delete from the hole map.
      if(!rmst_ptr->local_source_){
        if ( rmst_ptr->inHoleMap(frag_no) ){
          // We need to see if we sent a NAK for this frag.
          NakData *nak_ptr = rmst_ptr->getHole(frag_no);
          if(nak_ptr->nak_sent_)
            DiffPrint(DEBUG_SOME_DETAILS, "  We sent a NAK_REQ for this fragment.\n");
          DiffPrint(DEBUG_SOME_DETAILS, "  filter removing hole %d from hole_map_\n",frag_no);
          rmst_ptr->delHole(frag_no);
        }
      }
      // We start a WATCHDOG for an rmst here if: this is a local sink,
      // we haven't started a timer, and this is not the initial fragment.
      // Intermediate nodes (in caching mode) start a timer if they are on the
      // reinforced path, which is checked in processMessage.
      if((!rmst_ptr->local_source_)&&(local_sink_)&&(rmst_ptr->watchdog_active_ == false)
        && (frag_no>0)){
        TimerCallback *rmst_timer;
        DiffPrint(DEBUG_IMPORTANT, "  Set a WATCHDOG_TIMER at sink for rmst_no %d\n", rmst_no);
        rmst_timer = new RmstTimeout(this, rmst_no, WATCHDOG_TIMER);
        // We check on things every 10 seconds.
        rmst_ptr->watchdog_handle_ = ((DiffusionRouting *)dr_)->addTimer(WATCHDOG_INTERVAL,
          rmst_timer);
        rmst_ptr->watchdog_active_ = true;
      }
    }
    else
      DiffPrint(DEBUG_SOME_DETAILS, "  got a duplicate frag %d for blob %d\n",
        frag_no, rmst_no);

    // If we have still have a hole in the fragment map, update the hole map.

    if ((!rmst_ptr->local_source_) && (rmst_ptr->holeInFragMap()))
      rmst_ptr->syncHoleMap();

    // If the Rmst is complete, cancell timer, stop timer,
    // send Ack, give to local sinks.
    if(rmst_ptr->rmstComplete()){
      if ((rmst_ptr->watchdog_active_) && (!rmst_ptr->local_source_)){
        DiffPrint(DEBUG_SOME_DETAILS, 
          "  Rmst #%d is complete set cancel_watchdog_ to stop WATCHDOG\n",
          rmst_no);
        rmst_ptr->cancel_watchdog_ = true;
      }

      // Send this Rmst to any local sink
      if(local_sink_ && !(rmst_ptr->acked_)){
        sendRmstToSink(rmst_ptr);
        // We mark the rmst acked at the sink so it will clean up.
        rmst_ptr->acked_ = true;
      }

      // If this is a source, we only send out the fragments when we've got
      // them all from the application. If the Rmst is complete we add the
      // Rmst to the send_list_, and if there is no send timer we start one.
      if(rmst_ptr->local_source_){
        SendMsgData new_send_msg;
        // The Rmst is complete and this is a source - put in send list.
        new_send_msg.rmst_no_ = rmst_no;
        new_send_msg.last_frag_sent_ = -1;
        new_send_msg.exp_base_ = 0;
        send_list_.push_back(new_send_msg);
        if(!send_timer_active_){
          TimerCallback *send_timer;
          // Now add a timer to send this and any NAKS.
          DiffPrint(DEBUG_SOME_DETAILS,
            "  Rmst %d ready to send - Set a SEND_TIMER\n", rmst_no);
          send_timer = new RmstTimeout(this, -1, SEND_TIMER);
          // We check on things every second.
          send_timer_handle_ = 
            ((DiffusionRouting *)dr_)->addTimer(SEND_INTERVAL, send_timer);
          send_timer_active_ = true;
        }
      }
      else
        // We must let upstream nodes know that we got the whole blob.
        sendAckToSource(rmst_ptr);
    }
  }
  else{
    rmst_ptr->max_frag_rec_ = frag_no;
    DiffPrint(DEBUG_LOTS_DETAILS, "  Not caching frag %d entry for Rmst %d\n", frag_no, rmst_no);
  }

  return rmst_ptr;
}

void RmstFilter::processCtrlMessage(Message *msg)
{
  NRSimpleAttribute<int> *tsprt_ctl_attr = NULL;
  NRSimpleAttribute<int> *rmst_id_attr = NULL;
  NRSimpleAttribute<int> *frag_attr = NULL;
  NRAttrVec *data;
  NRAttrVec attrs;
  Int2Rmst::iterator rmst_iterator;
  int rmst_no; 
  int frag_no;
  int rmst_ctl_type;
  Rmst *rmst_ptr;
  void *frag_ptr;
  Message *nak_msg;
  NRAttrVec::iterator place;
  bool forwarding_nak = false;

  data = msg->msg_attr_vec_;

  tsprt_ctl_attr = RmstTsprtCtlAttr.find(data);
  rmst_ctl_type = tsprt_ctl_attr->getVal();

  rmst_id_attr = RmstIdAttr.find(data);
  if(!rmst_id_attr) {
    DiffPrint(DEBUG_SOME_DETAILS, "  Node got a bad Rmst control msg - no RmstIdAttr!\n");
    return;
  }
  rmst_no = rmst_id_attr->getVal();

  // Let's make sure we have this rmst
  rmst_iterator = rmst_map_.begin();
  rmst_iterator = rmst_map_.find(rmst_no);
  if(rmst_iterator != rmst_map_.end())
    rmst_ptr = (*rmst_iterator).second;
  else{
    DiffPrint(DEBUG_IMPORTANT, "  Filter can't find Rmst %d for Rmst control msg\n", rmst_no);
    return;
  }

  switch (rmst_ctl_type){

  case(ACK_RESP):
    DiffPrint(DEBUG_IMPORTANT, "  Got an ACK_RESP\n");
    // For now we automatically forward ACKs if we're not the source.
    rmst_ptr->acked_ = true;
    if(!rmst_ptr->local_source_){
      Message *ack_msg;
      // If we got an ACK and we aren't the source, we must be an
      // intermediate node (Sinks don't get ACKs, they send them).
      // We need to forward ACK toward source if possible.
      if (rmst_ptr->reinf_) {
        DiffPrint(DEBUG_SOME_DETAILS, "  forwarding ACK to %d\n", rmst_ptr->last_hop_);
        attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, ACK_RESP));
        attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_no));
        ack_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0, attrs.size(),
          pkt_count_, rdm_id_, rmst_ptr->last_hop_, LOCALHOST_ADDR);
        ack_msg->msg_attr_vec_ = CopyAttrs(&attrs);
        ((DiffusionRouting *)dr_)->sendMessage(ack_msg, filter_handle_, 1);
        pkt_count_++;
        delete ack_msg;
        ClearAttrs(&attrs);
      }
      else
        DiffPrint(DEBUG_IMPORTANT, "  intermediate node can't forward ACK for Rmst %d\n", rmst_no);
    }
    else{
      DiffPrint(DEBUG_IMPORTANT, "  Source got ACK for Rmst %d\n", rmst_no);
      sendContToSource(rmst_ptr);
    }
    break;

  case(NAK_REQ):

    // Mark the time we got this NAK for cleanup timer.
    GetTime(&rmst_ptr->last_nak_time_);
    rmst_ptr->naks_rec_++;
    DiffPrint(DEBUG_IMPORTANT, "  Got a NAK_REQ; number = %d\n", rmst_ptr->naks_rec_);

    if ((rmst_ptr->naks_rec_ > 10) && (rmst_ptr->naks_rec_ > (.30 * rmst_ptr->max_frag_)) &&
            rmst_ptr->local_source_){
      DiffPrint(DEBUG_IMPORTANT, "  Too many NAKs - send an EXPLORATORY msg!\n");
      processExpReq(rmst_ptr, rmst_ptr->max_frag_sent_);
      return;
    }

    // If we sent an exp request more than 30 seconds ago,
    // we send it again.
    if (rmst_ptr->sent_exp_req_){
      int exp_time = rmst_ptr->last_nak_time_.tv_sec - rmst_ptr->exp_req_time_.tv_sec;
      DiffPrint(DEBUG_SOME_DETAILS, 
        "  Node that sent an EXP_REQ got a NAK: time since last exp = %d\n", exp_time);
      if( (rmst_ptr->last_nak_time_.tv_sec - rmst_ptr->exp_req_time_.tv_sec) > 30){
        // Resend an EXP_REQ!!!
        DiffPrint(DEBUG_IMPORTANT, "  Node resends EXP_REQ up blacklisted stream!\n");
        sendExpReqUpstream(rmst_ptr);
        GetTime(&rmst_ptr->exp_req_time_);
        // Send another negative reinforcement on blacklisted link!
        Message *neg_reinf_msg;
        neg_reinf_msg = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
          0, 0, interest_attrs_->size(), pkt_count_, rdm_id_, 
          rmst_ptr->last_hop_, LOCALHOST_ADDR);
        neg_reinf_msg->msg_attr_vec_ = CopyAttrs(interest_attrs_);
        ((DiffusionRouting *)dr_)->sendMessage(neg_reinf_msg, filter_handle_, 1);
        pkt_count_++;
        delete neg_reinf_msg;
      }
      return;
    }

    // We need to send the naked fragments if we are the source,
    // or a caching node.
    place = data->begin();
    for(;;){
      frag_attr = RmstFragAttr.find_from(data, place, &place);
      if (!frag_attr)
        break;
      frag_no = frag_attr->getVal();
      DiffPrint(DEBUG_IMPORTANT, "  Filter received a NAK_REQ for Rmst %d, frag %d\n",
        rmst_no, frag_no);

      // Check if we have this fragment.
      // If not forward NAK toward source if possible.
      frag_ptr =  rmst_ptr->getFrag(frag_no);
      if (frag_ptr == NULL){
        DiffPrint(DEBUG_SOME_DETAILS, "  Filter can't find frag %d of Rmst %d for NAK\n",
        frag_no, rmst_no);
        // We need to forward NAK toward source if possible.
        if ( (rmst_ptr->reinf_) && (rmst_ptr->last_hop_ != LOCALHOST_ADDR) ){
          forwarding_nak = true;
          DiffPrint(DEBUG_IMPORTANT, "  forwarding NAK to %d\n", rmst_ptr->last_hop_);
          attrs.push_back(RmstFragAttr.make(NRAttribute::IS, frag_no));

          if(caching_mode_){
            // We need to add this fragment to our hole map!
            rmst_ptr->putHole(frag_no);
            NakData *nak_ptr = rmst_ptr->getHole(frag_no);
            // Artificially age this hole so it gets
            // naked immediately.
            nak_ptr->tmv.tv_sec -= 1;
          }
        }
        else
          DiffPrint(DEBUG_IMPORTANT, "  not forwarding NAK! - no place to send it!\n");
      }
      else{
        // We have this fragment so add it to the NakList for sending.
        NakMsgData nak_msg_data;
        NakList::iterator nak_list_iterator;
        nak_list_iterator = nak_list_.begin();
        while(nak_list_iterator != nak_list_.end()){
          if((nak_list_iterator->rmst_no_ == rmst_no) &&
            (nak_list_iterator->frag_no_ == frag_no))
            break;
          nak_list_iterator++;
        }
        if(nak_list_iterator == nak_list_.end()){
          DiffPrint(DEBUG_SOME_DETAILS,
            "  adding NAK for rmst %d frag %d to nak_list_\n", rmst_no, frag_no);
          nak_msg_data.rmst_no_ = rmst_no;
          nak_msg_data.frag_no_ = frag_no;
          nak_list_.push_back(nak_msg_data);
          if(!send_timer_active_){
            TimerCallback *send_timer;
            // Now add a timer to send this and any NAKS.
            DiffPrint(DEBUG_LOTS_DETAILS, 
              "  Set a SEND_TIMER for reinforced rmst_no %d\n", rmst_no);
            send_timer = new RmstTimeout(this, rmst_no, SEND_TIMER);
            // We check on things every second.
            send_timer_handle_ = 
              ((DiffusionRouting *)dr_)->addTimer(SEND_INTERVAL, send_timer);
            send_timer_active_ = true;
          }
        }
      }
      place++;
    }

    if (forwarding_nak){
      attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, NAK_REQ));
      attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_no));
      nak_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
      attrs.size(), pkt_count_, rdm_id_, rmst_ptr->last_hop_,
        LOCALHOST_ADDR);
      nak_msg->msg_attr_vec_ = CopyAttrs(&attrs);
      ((DiffusionRouting *)dr_)->sendMessage(nak_msg, filter_handle_, 1);
      pkt_count_++;
      delete nak_msg;
      ClearAttrs(&attrs);
    }
    break;

  case(EXP_REQ):
    DiffPrint(DEBUG_IMPORTANT, "  Got an EXP_REQ\n");
    if(!rmst_ptr->local_source_){
      DiffPrint(DEBUG_SOME_DETAILS, "  Filter forwarding EXP_REQ for Rmst %d\n", rmst_no);
      // We need to forward EXP_REQ toward source if possible.
      if (rmst_ptr->reinf_)
        sendExpReqUpstream(rmst_ptr);
    }
    else{
      // We need to call a routine that will clean the NAK list of 
      // outstanding NAK responses for this Rmst, put a new expBase
      // in the send list (lowest of nak or send Lists), and set this
      // rmst as not reinforced.
      frag_attr = RmstFragAttr.find(msg->msg_attr_vec_);
      frag_no = frag_attr->getVal();
      DiffPrint(DEBUG_IMPORTANT, "  Source got EXP request for Rmst %d\n", rmst_no);
      if (rmst_ptr->reinf_)
          processExpReq(rmst_ptr, frag_no);
      else
          DiffPrint(DEBUG_IMPORTANT, "  EXP request for non-reinforced Rmst %d\n", rmst_no);
    }
    break;

  default:
    break;
  }  // switch (rmst_ctl_type)
  return;
}

void RmstFilter::setupNak(int rmst_id)
{
  NRAttrVec attrs;
  int frag_id;
  NakData *nak_ptr;
  Rmst *rmst_ptr;
  int nak_count = 0;

  Int2Rmst::iterator rmst_iterator = rmst_map_.find(rmst_id);
  if(rmst_iterator != rmst_map_.end())
    rmst_ptr = (*rmst_iterator).second;
  else{
    DiffPrint(DEBUG_IMPORTANT, "setupNak - can't find Rmst %d\n", rmst_id);
    return;
  }

  Int2Nak::iterator hole_iter = rmst_ptr->hole_map_.begin();
  bool send_new_nak = false;
  timeval cur_time;

  // We now have an iterator to look at each hole (hole_iter),
  // a Rmst Id (rmst_id), a fragment Id ((*hole_iter).first),
  // a NakData pointer ((*hole_iter).second), and an Rmst
  // pointer (rmst_ptr).

  GetTime (&cur_time);

  // The first pass finds holes that haven't been NAKed and should be.
  while(hole_iter != rmst_ptr->hole_map_.end()){
    frag_id = (*hole_iter).first;
    nak_ptr = (*hole_iter).second;
    DiffPrint(DEBUG_SOME_DETAILS,
      "  setupNak - found hole rmst_id %d, frag %d\n", rmst_id, frag_id);

    // If we never NAKed this fragment and it's past due,
    // mark it so it gets NAKed.
    if (!nak_ptr->nak_sent_){
      if ( (cur_time.tv_sec - nak_ptr->tmv.tv_sec) > 3 ){
        nak_ptr->nak_sent_ = true;
        nak_ptr->send_nak_ = true;
        send_new_nak = true;
      }
      else
        DiffPrint(DEBUG_SOME_DETAILS,
          "  setupNak - hole %d not old enough to NAK\n", frag_id);
    }
    // If we NAKed this fragment and the NAK response is past due,
    // NAK it again.
    else if ( (cur_time.tv_sec - nak_ptr->tmv.tv_sec) > NAK_RESPONSE_WAIT ){
      DiffPrint(DEBUG_SOME_DETAILS, "  setupNak - hole %d has an overdue NAK\n", frag_id);
      nak_ptr->send_nak_ = true;
      send_new_nak = true;
    }
    hole_iter++;
  }

  if (send_new_nak){
    Message *nak_msg;
    if ( rmst_ptr->last_hop_ == LOCALHOST_ADDR ){
      DiffPrint(DEBUG_IMPORTANT, "  can't send NAK, no last_hop_!\n");
      return;
    }
    // The second pass adds all holes that should be NAKed to vector.
    hole_iter = rmst_ptr->hole_map_.begin();
    while( (hole_iter != rmst_ptr->hole_map_.end()) && (nak_count <= 10) ){
      frag_id = (*hole_iter).first;
      nak_ptr = (*hole_iter).second;

      if ( nak_ptr->send_nak_ ){
        nak_ptr->send_nak_ = false;
        DiffPrint(DEBUG_SOME_DETAILS,
          "  setupNak - adding a NAK for frag_id %d to attrs\n", frag_id);
        attrs.push_back(RmstFragAttr.make(NRAttribute::IS, frag_id));
        GetTime(&(nak_ptr->tmv));
        nak_count++;
      }
      hole_iter++;
    }
    attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, NAK_REQ));
    attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_id));

    // Code to send a message to last_hop_
    nak_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0, attrs.size(),
      pkt_count_, rdm_id_, rmst_ptr->last_hop_, LOCALHOST_ADDR);
    pkt_count_++;
    nak_msg->msg_attr_vec_ = CopyAttrs(&attrs);

    DiffPrint(DEBUG_IMPORTANT, "  Sending NAK_REQ to node %d\n", rmst_ptr->last_hop_);
    ((DiffusionRouting *)dr_)->sendMessage(nak_msg, filter_handle_, 1);

    delete nak_msg;
    ClearAttrs(&attrs);

    // Mark the time we sent this NAK for cleanup timer.
    GetTime(&rmst_ptr->last_nak_time_);

  }
  else
    DiffPrint(DEBUG_SOME_DETAILS, "  setupNak - no need for a new NAK for rmst_id %d\n", rmst_id);
  return;
}

void RmstFilter::processExpReq(Rmst *rmst_ptr, int frag_no)
{
  NakList::iterator nak_list_iterator;
  SendList::iterator send_list_iterator;
  int rmst_no = rmst_ptr->rmst_no_;

  DiffPrint(DEBUG_IMPORTANT, "  processExpReq called for rmstId %d, frag_no %d\n", rmst_no, frag_no);

  // Indicate that Rmst is not reinforced.
  rmst_ptr->reinf_ = false;
  rmst_ptr->pkts_sent_ = 0;

  // If we have an ACK_TIMER active cancel it. We want to resend some packets.
  if(rmst_ptr->ack_timer_active_){
    ((DiffusionRouting *)dr_)->removeTimer(rmst_ptr->ack_timer_handle_);
    rmst_ptr->ack_timer_active_ = false;
  }

  // Erase any NAKs. We are about to establish a new path.
  nak_list_iterator = nak_list_.begin();
  while (nak_list_iterator != nak_list_.end()){
    if (nak_list_iterator->rmst_no_ == rmst_no){
      DiffPrint(DEBUG_SOME_DETAILS,
        "  processExpReq erasing frag_no %d from nak_list_\n", nak_list_iterator->frag_no_);
      nak_list_iterator = nak_list_.erase(nak_list_iterator);
    }
    else
      nak_list_iterator++;
  }

  DiffPrint(DEBUG_LOTS_DETAILS, "  processExpReq done with nak_list_ for rmstId %d\n", rmst_no);

  // If we are being told to start by resending the last packet, back up by one.
  // When a sink gets an exploratory message, they don't start re-NAKing until they
  // know they are reinforced. Sinks only know they are reinforced when they get DATA.
  if ( (frag_no == rmst_ptr->max_frag_) && (rmst_ptr->max_frag_ > 0) ){
    frag_no--;
    DiffPrint(DEBUG_IMPORTANT, "  processExpReq decrements frag_no to %d\n", frag_no);
  }

  // Update send_list_ entry or add one.
  send_list_iterator = send_list_.begin();
  while (send_list_iterator != send_list_.end()){
    if (send_list_iterator->rmst_no_ == rmst_no)
      break;
    send_list_iterator++;
  }
  if (send_list_iterator != send_list_.end()){
    send_list_iterator->exp_base_ = frag_no;
    DiffPrint(DEBUG_SOME_DETAILS, "  processExpReq sets send_list_ expBase to %d\n", frag_no);
    send_list_iterator->last_frag_sent_ = frag_no-1;
  }
  else{
    SendMsgData new_send_msg;
    DiffPrint(DEBUG_SOME_DETAILS,
      "  processExpReq creating new send_list_ entry for rmstId %d\n", rmst_no);
    DiffPrint(DEBUG_SOME_DETAILS,
      "  processExpReq sets send_list_ expBase to %d\n", frag_no);
    new_send_msg.rmst_no_ = rmst_no;
    new_send_msg.exp_base_ = frag_no;
    new_send_msg.last_frag_sent_ = frag_no-1;
    send_list_.push_front(new_send_msg);
    if(!send_timer_active_){
      TimerCallback *send_timer;
      // Now add a timer to send this and any NAKS.
      DiffPrint(DEBUG_SOME_DETAILS, "  Set a SEND_TIMER for reinforced rmst_no %d\n", rmst_no);
      send_timer = new RmstTimeout(this, rmst_no, SEND_TIMER);
      // We check on things every second.
      send_timer_handle_ = ((DiffusionRouting *)dr_)->addTimer(SEND_INTERVAL, send_timer);
      send_timer_active_ = true;
    }
  }
}

handle RmstFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // This is a dummy attribute for filtering that matches everything
  attrs.push_back(NRClassAttr.make(NRAttribute::IS,
    NRAttribute::INTEREST_CLASS));
  h = ((DiffusionRouting *)dr_)->addFilter(&attrs, RMST_FILTER_PRIORITY, fcb_);
  ClearAttrs(&attrs);
  return h;
}

void RmstFilter::run()
{
#ifdef NS_DIFFUSION
  TimerCallback *stat_timer;
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_LOTS_DETAILS, "RmstFilter:: subscribed to all, received handle %d\n",
	    (int)filter_handle_);
  
  DiffPrint(DEBUG_LOTS_DETAILS, "RmstFilter constructor: start cleanup timer\n");
  stat_timer = new RmstTimeout(this, -1, CLEANUP_TIMER);
  stat_timer_handle_ = ((DiffusionRouting *)dr_)->addTimer(CLEANUP_INTERVAL, stat_timer);
#else
  // Doesn't do anything
  while(1){
      sleep(1000);
  }
#endif // NS_DIFFUSION
}

RmstTimeout::RmstTimeout(RmstFilter *rmst_flt, int no, int type)
{
  filter_ = rmst_flt;
  rmst_no_ = no;
  timer_type_ = type;
}

int RmstTimeout::expire()
{
  int retval;

  retval = filter_->processTimer(rmst_no_, timer_type_);
  if(retval == -1)
    delete this;

  return retval;
}

int RmstFilter::processTimer(int rmst_no, int timer_type)
{
  Rmst *rmst_ptr;
  void *frag_ptr;
  int frag_no;
  Int2Rmst::iterator rmst_iterator;
  timeval cur_time;

  GetTime (&cur_time);

  switch (timer_type){

  case SEND_TIMER:
    DiffPrint(DEBUG_SOME_DETAILS, "RmstFilter::processTimer SEND_TIMER");
    PrintTime(&cur_time);
    // If we haven't got any NAKs pending, send the next fragment of the Rmst
    // in progress. If we've sent all fragments of the current Rmst, we cancel
    // ourself. When we get a NAK, if this timer is active we send NAK responses
    // here. Otherwise we can send them directly from the NAK response routine.
    if (!nak_list_.empty()){
      Message *nak_resp;
      NRAttrVec nak_data_attrs;
      NakMsgData nak_msg_data = nak_list_.front();
      rmst_no = nak_msg_data.rmst_no_;
      frag_no = nak_msg_data.frag_no_;

      rmst_iterator = rmst_map_.find(rmst_no);
      if(rmst_iterator != rmst_map_.end()){
        rmst_ptr = (*rmst_iterator).second;
        // We have the fragment, set the last_data_time_ so that we defer
        // the cleanup of this Rmst, then we send the fragment to last hop.
        if (rmst_ptr->reinf_){
          GetTime(&rmst_ptr->last_data_time_);
          rmst_ptr->pkts_sent_++;
          nak_data_attrs.push_back(RmstTargetAttr.make(NRAttribute::IS,
            rmst_ptr->target_str_));
          nak_data_attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS,
            RMST_RESP));
          nak_data_attrs.push_back(RmstFragAttr.make(NRAttribute::IS, frag_no));
          // We routinely send the packet sent count on NAKs.
          nak_data_attrs.push_back(RmstPktsSentAttr.make(NRAttribute::IS,
            rmst_ptr->pkts_sent_));
          nak_data_attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_no));
          // Add the actual data; the length depends on if it's the last
          // fragment or not.
          frag_ptr =  rmst_ptr->getFrag(frag_no);
          if (frag_no == rmst_ptr->max_frag_)
            nak_data_attrs.push_back(RmstDataAttr.make(NRAttribute::IS,
              frag_ptr, rmst_ptr->max_frag_len_));
          else
            nak_data_attrs.push_back(RmstDataAttr.make(NRAttribute::IS,
              frag_ptr, MAX_FRAG_SIZE));

          DiffPrint(DEBUG_IMPORTANT, "  Filter sending Data for NAKed frag %d of Rmst %d\n",
            frag_no, rmst_no);

          nak_resp = new Message(DIFFUSION_VERSION, DATA, 0, 0,
            nak_data_attrs.size(), pkt_count_, rdm_id_,
            LOCALHOST_ADDR, LOCALHOST_ADDR);
          nak_resp->msg_attr_vec_ = CopyAttrs(&nak_data_attrs);
          ((DiffusionRouting *)dr_)->sendMessage(nak_resp, filter_handle_);
          pkt_count_++;
          delete nak_resp;
          ClearAttrs(&nak_data_attrs);
        }
        else
          DiffPrint(DEBUG_IMPORTANT, 
            "RmstFilter::processTimer sees non-reinforced path for NAK on rmst %d!\n",
            rmst_no);
      }
      else
        DiffPrint(DEBUG_IMPORTANT, "RmstFilter::processTimer can't find Rmst %d for NAK!\n",
          rmst_no);
      nak_list_.pop_front();
    }

    else if (!send_list_.empty()){
      int8_t msg_type;
      int action = DO_NOTHING;
      // Get the rmst and frag_no that is in progress.
      NRAttrVec data_attrs;

      SendMsgData send_data = send_list_.front();
      rmst_no = send_data.rmst_no_;
      rmst_iterator = rmst_map_.find(rmst_no);
      if(rmst_iterator == rmst_map_.end())
        action = DELETE_FROM_QUEUE;
      else{
        rmst_ptr = (*rmst_iterator).second;
        if ((send_data.last_frag_sent_ == rmst_ptr->max_frag_) && rmst_ptr->reinf_)
          action = DELETE_FROM_QUEUE;
        else if ( (send_data.last_frag_sent_ == send_data.exp_base_) &&
          (!rmst_ptr->reinf_) && (exp_gap_ < 10) ){
          action = DO_NOTHING;
          exp_gap_++;
        }
        else
          action = SEND_NEXT_FRAG;
      }

      switch (action){

      case(DELETE_FROM_QUEUE):
        // Delete message data from front. 
        send_list_.pop_front();
        break;

      case(SEND_NEXT_FRAG):
        send_list_.pop_front();
        if (rmst_ptr->reinf_){
          send_data.last_frag_sent_++;
          frag_no = send_data.last_frag_sent_;
        }
        else{
          frag_no = send_data.exp_base_;
          send_data.last_frag_sent_ = frag_no;
        }
        send_list_.push_front(send_data);
        rmst_ptr->max_frag_sent_ = frag_no;

        DiffPrint(DEBUG_IMPORTANT, "  Source Filter sending frag %d of Rmst %d\n",
          frag_no, rmst_no);
        
        // Now make a message.
        data_attrs.push_back(RmstTargetAttr.make(NRAttribute::IS, rmst_ptr->target_str_));
        data_attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, RMST_RESP));
        data_attrs.push_back(RmstFragAttr.make(NRAttribute::IS, frag_no));

        // We send the MaxFragAttr on the first Exploratory packet,
        // and the PktsSentAttr on the last packet.
        if(frag_no == 0)
          data_attrs.push_back(RmstMaxFragAttr.make(NRAttribute::IS,
            rmst_ptr->max_frag_));
        else if (frag_no == rmst_ptr->max_frag_){
          rmst_ptr->pkts_sent_++;
          data_attrs.push_back(RmstPktsSentAttr.make(NRAttribute::IS,
            rmst_ptr->pkts_sent_));
        }
        else
          rmst_ptr->pkts_sent_++;

        data_attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_no));

        // Add the actual data; the length depends on if it's the last
        // fragment or not.
        frag_ptr =  rmst_ptr->getFrag(frag_no);
        if (rmst_ptr->max_frag_ == frag_no)
          data_attrs.push_back(RmstDataAttr.make(NRAttribute::IS, 
            frag_ptr, rmst_ptr->max_frag_len_));
        else
          data_attrs.push_back(RmstDataAttr.make(NRAttribute::IS,
            frag_ptr, MAX_FRAG_SIZE));

        if (frag_no == send_data.exp_base_){
          ExpLog exp_msg;
          union LlToInt key;
          // Insert this Exploratory message in exp_map_.
          // When we get a reinforcement we'll know what rmst it's for.
          key.int_val_[0] = pkt_count_;
          key.int_val_[1] = rdm_id_;
          DiffPrint(DEBUG_LOTS_DETAILS, "  Key = %llx\n", key.ll_val_);
          exp_msg.rmst_no_ = rmst_no;
          exp_msg.last_hop_ = LOCALHOST_ADDR;
          exp_map_.insert(Key2ExpLog::value_type(key.ll_val_, exp_msg));
          msg_type = EXPLORATORY_DATA;
          rmst_ptr->reinf_ = false;
          rmst_ptr->pkts_sent_ = 0;
          rmst_ptr->naks_rec_ = 0;
          exp_gap_ = 0;
          DiffPrint(DEBUG_IMPORTANT,
            "  Source Filter sending EXPLORATORY frag %d of Rmst %d\n",
            frag_no, rmst_no);
        }
        else
          msg_type = DATA;

        Message *new_frag;
        new_frag = new Message(DIFFUSION_VERSION, msg_type, 0, 0,
          data_attrs.size(), pkt_count_, rdm_id_,
          LOCALHOST_ADDR, LOCALHOST_ADDR);
        new_frag->msg_attr_vec_ = CopyAttrs(&data_attrs);
        ((DiffusionRouting *)dr_)->sendMessage(new_frag, filter_handle_);
        pkt_count_++;
        delete new_frag;
        ClearAttrs(&data_attrs);
        // We sent a fragment, set the last_data_time_ for the cleanup timer.
        GetTime(&rmst_ptr->last_data_time_);

        // If this is the last frag, we start an ACK_TIMER.
        if ( (rmst_ptr->max_frag_ == frag_no) &&
          (rmst_ptr->ack_timer_active_ == false) && (rmst_ptr->local_source_) ){
          TimerCallback *rmst_timer;
          DiffPrint(DEBUG_SOME_DETAILS, "  Set an ACK_TIMER at source for rmst_no %d\n",
            rmst_no);
          rmst_timer = new RmstTimeout(this, rmst_no, ACK_TIMER);
          // We check on things every 20 seconds.
          rmst_ptr->ack_timer_handle_ = 
            ((DiffusionRouting *)dr_)->addTimer(ACK_INTERVAL, rmst_timer);
          rmst_ptr->ack_timer_active_ = true;
        }
        break;

      case(DO_NOTHING):
        DiffPrint(DEBUG_LOTS_DETAILS, "  Nothing to do\n");
        break;
      } // Switch on Action
    }

    if (nak_list_.empty() && send_list_.empty()){
      DiffPrint(DEBUG_LOTS_DETAILS, "   Cancelling SEND_TIMER, no NAKS or data to send\n");
      send_timer_active_ = false;
      return -1;
    }
    else
      return 0;
    break;

  case WATCHDOG_TIMER:
    DiffPrint(DEBUG_SOME_DETAILS, "RmstFilter::processTimer WATCHDOG_TIMER for Rmst %d", rmst_no);
    PrintTime(&cur_time);
    rmst_iterator = rmst_map_.find(rmst_no);
    if(rmst_iterator != rmst_map_.end())
      rmst_ptr = (*rmst_iterator).second;
    else{
      DiffPrint(DEBUG_IMPORTANT,
        "RmstFilter::processTimer can't find Rmst %d for WATCHDOG, cancell timer!\n",
        rmst_no);
      return -1;
    }

    if(rmst_ptr->cancel_watchdog_){
      DiffPrint(DEBUG_SOME_DETAILS,
        "  processTimer cancelling WATCHDOG_TIMER for Rmst %d\n", rmst_no);
      rmst_ptr->watchdog_active_ = false;
      rmst_ptr->cancel_watchdog_ = false;
      return -1;
    }

    if (rmst_ptr->wait_for_new_path_){
      DiffPrint (DEBUG_IMPORTANT, "  WATCHDOG_TIMER sees wait_for_new_path_ - suspend NAKs\n");
      return 0;
    }

    // If we sent an exp request more than 20 seconds ago,
    // we send it again.
    if (rmst_ptr->sent_exp_req_){
      int exp_time;
      exp_time = cur_time.tv_sec - rmst_ptr->exp_req_time_.tv_sec;
      DiffPrint(DEBUG_SOME_DETAILS,
        "  Node sent an EXP_REQ: time since last exp = %d\n", exp_time);
      if( exp_time > 20){
        // Resend an EXP_REQ!!!
        DiffPrint(DEBUG_IMPORTANT, "  Node resends EXP_REQ up blacklisted stream!\n");
        sendExpReqUpstream(rmst_ptr);
        GetTime(&rmst_ptr->exp_req_time_);
      }
      else
        DiffPrint(DEBUG_LOTS_DETAILS, "  Node waits to send another EXP_REQ\n");
      return 0;
    }

    if (rmst_ptr->local_source_){
      if(rmst_ptr->acked_){
        DiffPrint(DEBUG_IMPORTANT, 
          "  WATCHDOG_TIMER Local Source sees acked state - cancel timer\n");
        return -1;
      }
      else{
        DiffPrint(DEBUG_LOTS_DETAILS, "  WATCHDOG_TIMER Local Source sees rmst not acked\n");
        return 0;
      }
    }

    // Check if we have waited too long for next fragment.
    if( ((cur_time.tv_sec - rmst_ptr->last_data_time_.tv_sec) > NEXT_FRAG_WAIT) &&
      (!rmst_ptr->rmstComplete()) ){
      int newHole = (rmst_ptr->max_frag_rec_)+1;
      if ( (newHole <= rmst_ptr->max_frag_) && (!rmst_ptr->inHoleMap(newHole)) ){
        DiffPrint(DEBUG_SOME_DETAILS, "  WATCHDOG_TIMER adds new hole, frag %d\n",
          newHole);
        rmst_ptr->putHole(newHole);
        NakData *nak_ptr = rmst_ptr->getHole(newHole);
        // Artificially age this hole so it gets naked immediately
        nak_ptr->tmv.tv_sec -= 4;
      }
    }

    if(rmst_ptr->holeMapEmpty()){
      // There aren't any holes!
      DiffPrint(DEBUG_SOME_DETAILS, "  WATCHDOG_TIMER sees No holes\n");
      return 0;
    }
    else{
      // The WATCHDOG_TIMER expired and we have a hole,
      //   so we may need to construct a NAK from the hole map
      //   If we've fallen off the reinforced path, we must
      //   stop adding NAKs to the NakList.
      DiffPrint(DEBUG_SOME_DETAILS, "  WATCHDOG_TIMER sees holes - check times.\n");
      if (rmst_ptr->reinf_)
        setupNak(rmst_no);
      // Reschedule timer with same value.
      return 0;
    }
    break;

  case ACK_TIMER:
    DiffPrint(DEBUG_SOME_DETAILS, "RmstFilter::processTimer ACK_TIMER for Rmst %d", rmst_no);
    PrintTime(&cur_time);
    rmst_iterator = rmst_map_.find(rmst_no);
    if(rmst_iterator != rmst_map_.end())
      rmst_ptr = (*rmst_iterator).second;
    else{
      DiffPrint(DEBUG_IMPORTANT,
        "RmstFilter::processTimer can't find Rmst %d for ACK_TIMER, cancell timer!\n",
           rmst_no);
      return -1;
    }

    DiffPrint(DEBUG_SOME_DETAILS, "RmstFilter::processTimer ACK_TIMER, at source for rmst_no %d\n",
      rmst_no);

    if (rmst_ptr->acked_){
      DiffPrint(DEBUG_IMPORTANT, "RmstFilter::processTimer cancel ACK_TIMER, Rmst %d ACKed\n",
        rmst_no);
      rmst_ptr->ack_timer_active_ = false;
      return -1;
    }

    // If there has been no data sent for 30 seconds like a NAK response, we need to resend a packet.
    if( (cur_time.tv_sec - rmst_ptr->last_data_time_.tv_sec) > ACK_WAIT ){
      NRAttrVec attrs;
      int8_t msg_type;
      DiffPrint(DEBUG_IMPORTANT, 
        "RmstFilter::processTimer ACK_TIMER, waited too long for Rmst %d ACK!\n", rmst_no);
      if(rmst_ptr->reinf_  && !rmst_ptr->resent_last_data_){
        // We should send the last frag again as an DATA packet.
        DiffPrint(DEBUG_SOME_DETAILS, 
          "RmstFilter::processTimer ACK_TIMER, resend last packet as DATA\n");
        msg_type = DATA;
        rmst_ptr->resent_last_data_ = true;
      }
      else if(rmst_ptr->resent_last_data_ && !rmst_ptr->resent_last_exp_){
        ExpLog exp_msg;
        union LlToInt key;
        // We tried resending last frag as data and it didn't work, try as EXP
        DiffPrint(DEBUG_IMPORTANT,
          "RmstFilter::processTimer ACK_TIMER, resend last packet as EXPLORATORY_DATA\n");
        // Insert this Exploratory message in exp_map_.
        // When we get a reinforcement we'll know what rmst it's for.
        key.int_val_[0] = pkt_count_;
        key.int_val_[1] = rdm_id_;
        DiffPrint(DEBUG_LOTS_DETAILS, "  Key = %llx\n", key.ll_val_);
        exp_msg.rmst_no_ = rmst_no;
        exp_msg.last_hop_ = LOCALHOST_ADDR;
        exp_map_.insert(Key2ExpLog::value_type(key.ll_val_, exp_msg));
        msg_type = EXPLORATORY_DATA;
        rmst_ptr->reinf_ = false;
        rmst_ptr->naks_rec_ = 0;
        rmst_ptr->pkts_sent_ = 0;
        rmst_ptr->resent_last_exp_ = true;
      }
      else if(rmst_ptr->resent_last_data_ && rmst_ptr->resent_last_exp_ && rmst_ptr->reinf_){
        // We should send the last frag again as an DATA packet.
        DiffPrint(DEBUG_IMPORTANT,
          "RmstFilter::processTimer ACK_TIMER, resend last packet on new reinf path as DATA\n");
        msg_type = DATA;
        rmst_ptr->resent_last_data_ = false;
        rmst_ptr->resent_last_exp_ = false;
      }
      else{
        ExpLog exp_msg;
        union LlToInt key;
        DiffPrint(DEBUG_IMPORTANT, 
          "RmstFilter::processTimer ACK_TIMER, resent last packet as EXP and no reinforced path, Try again!\n");
        // Insert this Exploratory message in exp_map_.
        // When we get a reinforcement we'll know what rmst it's for.
        key.int_val_[0] = pkt_count_;
        key.int_val_[1] = rdm_id_;
        DiffPrint(DEBUG_LOTS_DETAILS, "  Key = %llx\n", key.ll_val_);
        exp_msg.rmst_no_ = rmst_no;
        exp_msg.last_hop_ = LOCALHOST_ADDR;
        exp_map_.insert(Key2ExpLog::value_type(key.ll_val_, exp_msg));
        msg_type = EXPLORATORY_DATA;
      }
      attrs.push_back(RmstTargetAttr.make(NRAttribute::IS, rmst_ptr->target_str_));
      attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, RMST_RESP));
      attrs.push_back(RmstFragAttr.make(NRAttribute::IS, rmst_ptr->max_frag_));
      attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_ptr->rmst_no_));
      frag_ptr =  rmst_ptr->getFrag(rmst_ptr->max_frag_);
      attrs.push_back(RmstDataAttr.make(NRAttribute::IS, 
        frag_ptr, rmst_ptr->max_frag_len_));
      Message *new_frag;
      new_frag = new Message(DIFFUSION_VERSION, msg_type, 0, 0, attrs.size(), pkt_count_,
        rdm_id_, LOCALHOST_ADDR, LOCALHOST_ADDR);
      new_frag->msg_attr_vec_ = CopyAttrs(&attrs);
      ((DiffusionRouting *)dr_)->sendMessage(new_frag, filter_handle_);
      pkt_count_++;
      delete new_frag;
      ClearAttrs(&attrs);
      // We sent a fragment, set the last_data_time_ for the cleanup timer.
      GetTime(&rmst_ptr->last_data_time_);
    }
    return 0;
    break;

  case CLEANUP_TIMER:
    DiffPrint(DEBUG_SOME_DETAILS, "RmstFilter::processTimer CLEANUP_TIMER");
    PrintTime(&cur_time);

    DiffPrint(DEBUG_IMPORTANT, "  CLEANUP_TIMER called\n");
    rmst_iterator = rmst_map_.begin();
    while(rmst_iterator != rmst_map_.end()){
      rmst_ptr = (*rmst_iterator).second;

      DiffPrint(DEBUG_SOME_DETAILS, 
        "  CLEANUP_TIMER:: rmst_no %d : pkts_sent_ = %d, pkts_rec_ = %d, last_hop_pkts_sent_ = %d\n",
        rmst_ptr->rmst_no_, rmst_ptr->pkts_sent_, rmst_ptr->pkts_rec_, rmst_ptr->last_hop_pkts_sent_);

      if((!rmst_ptr->reinf_)&&(!rmst_ptr->acked_)&&(!rmst_ptr->local_source_)&&(!local_sink_)){
        if( (cur_time.tv_sec - rmst_ptr->last_data_time_.tv_sec) > LONG_CLEANUP_WAIT )
          cleanUpRmst(rmst_ptr);
      }
      else if (rmst_ptr->acked_){
        if ( ( (cur_time.tv_sec - rmst_ptr->last_data_time_.tv_sec) > SHORT_CLEANUP_WAIT ) &&
           ( (cur_time.tv_sec - rmst_ptr->last_nak_time_.tv_sec) > SHORT_CLEANUP_WAIT ) )
           cleanUpRmst(rmst_ptr);
      }
            
      rmst_iterator++;
    }

    // Check on the BlackList, in case network is partitioned.
    if (!black_list_.empty()){
      if ( (cur_time.tv_sec - last_data_rec_.tv_sec) > RMST_BLACKLIST_WAIT ){
        DiffPrint(DEBUG_IMPORTANT, "  clearing black_list_!\n");
        ((DiffusionRouting *)dr_)->clearBlacklist();
        black_list_.clear();
      }
    }

    if (local_sink_){
      if ( (cur_time.tv_sec - last_sink_time_.tv_sec) > SINK_REFRESH_WAIT ){
        DiffPrint(DEBUG_IMPORTANT, "  local sink timed out\n");
        local_sink_ = false;
      }
      else
        DiffPrint(DEBUG_IMPORTANT, "  local sink still alive.\n");
    }

    return 0;
    break;

  default:
    break;
  }
  return -1;
}

void RmstFilter::sendRmstToSink(Rmst *rmst_ptr)
{
  NRAttrVec attrs;
  Message *rmst_msg;
  NRSimpleAttribute<void *> *rmst_data_attr;
  NRSimpleAttribute<int> *frag_number_attr;
  void *frag_ptr;
  int size, i;

  DiffPrint(DEBUG_IMPORTANT, "RmstFilter::sendRmstToSink - sending rmst %d to local sink\n",
    rmst_ptr->rmst_no_);

  // Prepare attribute vector
  attrs.push_back(RmstTargetAttr.make(NRAttribute::IS, rmst_ptr->target_str_));
  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, RMST_RESP));
  attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_ptr->rmst_no_));
  attrs.push_back(RmstMaxFragAttr.make(NRAttribute::IS, rmst_ptr->max_frag_));
  frag_number_attr = RmstFragAttr.make(NRAttribute::IS, 0);
  attrs.push_back(frag_number_attr);
  // Add the blob fragment
  if (rmst_ptr->max_frag_ == 0)
    size = rmst_ptr->max_frag_len_;
  else
    size = MAX_FRAG_SIZE;
  frag_ptr =  rmst_ptr->getFrag(0);
  rmst_data_attr = RmstDataAttr.make(NRAttribute::IS, frag_ptr, size);
  attrs.push_back(rmst_data_attr);

  // Prepare the message
  rmst_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0, attrs.size(),
    pkt_count_, rdm_id_, LOCALHOST_ADDR, LOCALHOST_ADDR);
  rmst_msg->next_hop_ = LOCALHOST_ADDR;
  rmst_msg->next_port_ = local_sink_port_;
  rmst_msg->msg_attr_vec_= CopyAttrs(&attrs);
  ((DiffusionRouting *)dr_)->sendMessage(rmst_msg, filter_handle_, 1);
  delete rmst_msg;
  pkt_count_++;

  // Send all the fragments
  for (i=1; i <= (rmst_ptr->max_frag_); i++){
    frag_number_attr->setVal(i);
    frag_ptr =  rmst_ptr->getFrag(i);
    if(frag_ptr == NULL)
      DiffPrint(DEBUG_IMPORTANT, "RmstFilter::sendRmstToSink - got a null frag_ptr for frag!%d\n",
      i);
    else{
      if (rmst_ptr->max_frag_ == i)
        rmst_data_attr->setVal(frag_ptr, rmst_ptr->max_frag_len_);
      else
        rmst_data_attr->setVal(frag_ptr, MAX_FRAG_SIZE);
    }
    rmst_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0, attrs.size(),
      pkt_count_, rdm_id_, LOCALHOST_ADDR, LOCALHOST_ADDR);
    rmst_msg->next_hop_ = LOCALHOST_ADDR;
    rmst_msg->next_port_ = local_sink_port_;
    rmst_msg->msg_attr_vec_= CopyAttrs(&attrs);
    ((DiffusionRouting *)dr_)->sendMessage(rmst_msg, filter_handle_, 1);
    delete rmst_msg;
    pkt_count_++;
  }

  ClearAttrs(&attrs);
}

void RmstFilter::sendAckToSource(Rmst *rmst_ptr)
{
  NRAttrVec attrs;
  Message *ack_msg;

  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, ACK_RESP));
  attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_ptr->rmst_no_));

  // New code to send a message to last_hop_
  ack_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
            attrs.size(), pkt_count_, rdm_id_, rmst_ptr->last_hop_,
            LOCALHOST_ADDR);
  ack_msg->msg_attr_vec_ = CopyAttrs(&attrs);

  DiffPrint(DEBUG_IMPORTANT, "  Sending ACK_RESP to node %d\n", rmst_ptr->last_hop_);
  ((DiffusionRouting *)dr_)->sendMessage(ack_msg, filter_handle_, 1);
  pkt_count_++;
  delete ack_msg;
  ClearAttrs(&attrs);
}

void RmstFilter::sendExpReqUpstream(Rmst *rmst_ptr)
{
  NRAttrVec attrs;
  Message *exp_msg;

  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::IS, EXP_REQ));
  attrs.push_back(RmstIdAttr.make(NRAttribute::IS, rmst_ptr->rmst_no_));
  attrs.push_back(RmstFragAttr.make(NRAttribute::IS, rmst_ptr->max_frag_rec_));

  exp_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
            attrs.size(), pkt_count_, rdm_id_, rmst_ptr->last_hop_,
            LOCALHOST_ADDR);
  exp_msg->msg_attr_vec_ = CopyAttrs(&attrs);

  DiffPrint(DEBUG_IMPORTANT, "  Sending EXP_REQ to node %d\n", rmst_ptr->last_hop_);
  ((DiffusionRouting *)dr_)->sendMessage(exp_msg, filter_handle_, 1);
  pkt_count_++;
  delete exp_msg;
  ClearAttrs(&attrs);
}

void RmstFilter::sendContToSource(Rmst *rmst_ptr)
{
  NRAttrVec attrs;
  Message *cont_msg;
  attrs.push_back(RmstTsprtCtlAttr.make(NRAttribute::EQ, RMST_CONT));
  attrs.push_back(NRClassAttr.make(NRAttribute::IS,
    NRAttribute::INTEREST_CLASS));
  DiffPrint(DEBUG_IMPORTANT, "  Sending a RMST_CONT to source\n");
  cont_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0, attrs.size(),
    pkt_count_, rdm_id_, LOCALHOST_ADDR, LOCALHOST_ADDR);
  cont_msg->msg_attr_vec_ = CopyAttrs(&attrs);
  cont_msg->next_hop_ = LOCALHOST_ADDR;
  cont_msg->next_port_ = rmst_ptr->local_source_port_;
  ((DiffusionRouting *)dr_)->sendMessage(cont_msg, filter_handle_, 1);
  pkt_count_++;
  delete cont_msg;
  ClearAttrs(&attrs);
}

void RmstFilter::cleanUpRmst(Rmst *rmst_ptr)
{
  int rmst_no = rmst_ptr->rmst_no_;

  Int2Rmst::iterator rmst_iterator;
  Key2ExpLog::iterator exp_iterator;
  ExpLog exp_msg;
  rmst_no = rmst_ptr->rmst_no_;
  DiffPrint(DEBUG_IMPORTANT, "  cleanUpRmst called to delete Rmst %d\n", rmst_no);

  if(rmst_ptr->watchdog_active_)
    ((DiffusionRouting *)dr_)->removeTimer(rmst_ptr->watchdog_handle_);
  if(rmst_ptr->ack_timer_active_)
    ((DiffusionRouting *)dr_)->removeTimer(rmst_ptr->ack_timer_handle_);

  rmst_iterator = rmst_map_.find(rmst_no);
  if(rmst_iterator != rmst_map_.end()){
    rmst_map_.erase(rmst_iterator);
  }
  delete rmst_ptr;

  // clean up the exp_map_ of any entries base on this rmst
  exp_iterator = exp_map_.begin();
  while(exp_iterator != exp_map_.end()){
    exp_msg = (*exp_iterator).second;
    if(exp_msg.rmst_no_ == rmst_no){
      DiffPrint(DEBUG_LOTS_DETAILS, "  cleanUpRmst deleting exp_map_ entry for Rmst %d\n", rmst_no);
      exp_map_.erase(exp_iterator);
    }
    exp_iterator++;
  }
}

#ifdef NS_DIFFUSION
RmstFilter::RmstFilter()
{
#else
RmstFilter::RmstFilter(int argc, char **argv)

{
  TimerCallback *stat_timer;

  parseCommandLine(argc, argv);
  dr_ = NR::createNR(diffusion_port_);
#endif // NS_DIFFUSION

  fcb_ = new RmstFilterCallback;
  fcb_->app_ = this;
  rdm_id_ = rand();
  pkt_count_ = rand();
  local_sink_ = false;
  caching_mode_ = false;
  send_timer_active_ = false;

  DiffPrint(DEBUG_ALWAYS, "RmstFilter constructor: rdm_id_ = %x, pkt_count_ = %x\n",
	    rdm_id_, pkt_count_);

#ifndef NS_DIFFUSION
  filter_handle_ = setupFilter();
  DiffPrint(DEBUG_LOTS_DETAILS, "RmstFilter:: subscribed to all, received handle %d\n",
	    (int)filter_handle_);
  
  DiffPrint(DEBUG_LOTS_DETAILS, "RmstFilter constructor: start cleanup timer\n");
  stat_timer = new RmstTimeout(this, -1, CLEANUP_TIMER);
  stat_timer_handle_ = ((DiffusionRouting *)dr_)->addTimer(CLEANUP_INTERVAL, stat_timer);
#endif // !NS_DIFFUSION
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  RmstFilter *app;
  app = new RmstFilter(argc, argv);
  app->run();

  return 0;
}
#endif // !NS_DIFFUSION
