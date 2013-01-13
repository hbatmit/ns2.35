//
// rmst.cc         : Rmst Class Methods
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst.cc,v 1.2 2005/09/13 04:53:48 tomh Exp $
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

#include <stdio.h>
#include "rmst.hh"

Rmst::Rmst(int id, int mxFrag)
{
  rmst_no_ = id;
  max_frag_rec_ = mxFrag;
  max_frag_sent_ = 0;
  reinf_ = false;
  local_source_ = false;
  target_str_ = NULL;
  acked_ = false;
  resent_last_data_ = false;
  resent_last_exp_ = false;
  wait_for_new_path_ = false;
  watchdog_active_ = false;
  cancel_watchdog_ = false;
  ack_timer_active_ = false;
  sent_exp_req_ = false;
  last_hop_ = 0;
  fwd_hop_ = 0;
  sync_base_ = 0;
  pkts_sent_ = 0;
  pkts_rec_ = 0;
  naks_rec_ = 0;
  last_hop_pkts_sent_ = 0;
}

Rmst::~Rmst()
{
  void *tmp_frag_ptr;
  Int2Frag::iterator frag_iterator;
  for(frag_iterator=frag_map_.begin(); frag_iterator!=frag_map_.end(); ++frag_iterator){
    tmp_frag_ptr = (void*)(*frag_iterator).second;
    delete((char *)tmp_frag_ptr);
  }
  if (target_str_ != NULL)
    delete(target_str_);
}

void* Rmst::getFrag(int frag_no)
{
  Int2Frag::iterator frag_iterator = frag_map_.find(frag_no);
  if(frag_iterator == frag_map_.end())
    return NULL;
  else
    return (*frag_iterator).second;
}

void Rmst::putFrag(int frag_no, void *data)
{
  frag_map_.insert(Int2Frag::value_type(frag_no, data));
  if (frag_no > max_frag_rec_){
    max_frag_rec_ = frag_no;
  }
}

bool Rmst::holeInFragMap()
{
  int count = frag_map_.size();
  if ( (count <= max_frag_rec_) ||
    (hole_map_.size() != 0) )
    return true;
  else
    return false;
}

bool Rmst::rmstComplete()
{
  int count = frag_map_.size();
  if (count <= max_frag_)
    return false;
  else
    return true;
}

bool Rmst::inHoleMap(int hole_no)
{
  Int2Nak::iterator hole_iterator;

  hole_iterator = hole_map_.find(hole_no);
  if (hole_iterator == hole_map_.end() )
    return false;
  return true;
}

void Rmst::putHole(int hole_no)
{
  NakData *nak_ptr = new NakData();
  GetTime(&(nak_ptr->tmv));
  hole_map_.insert(Int2Nak::value_type(hole_no, nak_ptr));
}

NakData* Rmst::getHole(int hole_no)
{
  Int2Nak::iterator hole_iterator = hole_map_.find(hole_no);
  if(hole_iterator == hole_map_.end())
    return NULL;
  else
    return (*hole_iterator).second;
}

void Rmst::delHole(int hole_no)
{
  NakData *nak_ptr;
  Int2Nak::iterator hole_iterator;
  hole_iterator = hole_map_.find(hole_no);
  if (hole_iterator == hole_map_.end() ){
    DiffPrint(DEBUG_ALWAYS, "delHole:: did not find hole num %d\n", hole_no);
    return;
  }
  nak_ptr = (*hole_iterator).second;
  delete nak_ptr;
  hole_map_.erase(hole_iterator);
  return;
}

void Rmst::cleanHoleMap()
{
  NakData *nak_ptr;
  Int2Nak::iterator hole_iterator;
  for(hole_iterator = hole_map_.begin(); hole_iterator != hole_map_.end(); ++hole_iterator){
    if (hole_iterator != hole_map_.end() ){
      nak_ptr = (*hole_iterator).second;
      delete nak_ptr;
      hole_map_.erase(hole_iterator);
    }
  }
}

bool Rmst::holeMapEmpty()
{
  Int2Nak::iterator hole_iterator;

  hole_iterator = hole_map_.begin();
  if (hole_iterator != hole_map_.end())
    return false;
  else
    return true;
}

void Rmst::syncHoleMap()
{
  Int2Nak::iterator nak_iterator;
  Int2Frag::iterator frag_iterator;
  DiffPrint(DEBUG_SOME_DETAILS, "  syncHoleMap - found %d holes.\n",
    (max_frag_rec_+1) - frag_map_.size());

  for (int i = sync_base_; i <= max_frag_rec_; i++){
    frag_iterator = frag_map_.find(i);
    if (frag_iterator == frag_map_.end() ){
      DiffPrint(DEBUG_SOME_DETAILS, "  syncHoleMap - found hole num %d\n", i);
      nak_iterator = hole_map_.find(i);
      if (nak_iterator == hole_map_.end())
        putHole(i);
    }
  }
}

void PrintTime(struct timeval *time)
{
  DiffPrint(DEBUG_ALWAYS, "  time: sec = %d\n", (unsigned int) time->tv_sec);
}
