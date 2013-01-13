//
// rmst_source.hh  : RmstSource Class
// authors         : Fred Stann
//
// Copyright (C) 2003 by the University of Southern California
// $Id: rmst_source.hh,v 1.4 2010/03/08 05:54:49 tom_henderson Exp $
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

#ifndef RMST_SRC
#define RMST_SRC

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H

#include "diffapp.hh"

#define PAYLOAD_SIZE 50
class RmstSource;

#ifdef NS_DIFFUSION
class RmstSendDataTimer : public TimerHandler {
  public:
  RmstSendDataTimer(RmstSource *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
protected:
  RmstSource *a_;
};
#endif //NS_DIFFUSION

class RmstSrcReceive : public NR::Callback {
public:
  RmstSrcReceive(RmstSource *rmstSrc) {src_ = rmstSrc; }
  void recv(NRAttrVec *data, NR::handle my_handle);
protected:
  RmstSource *src_;
};

class RmstSource : public DiffApp {
public:
#ifdef NS_DIFFUSION
  RmstSource();
  int command(int argc, const char*const* argv);
  void send();
#else
  RmstSource(int argc, char **argv);
#endif // NS_DIFFUSION
  virtual ~RmstSource(){};

  handle setupRmstInterest();
  handle setupRmstPublication();

  void run();
  void recv(Message *msg, handle h); 
  NR* getDr() {return dr_;} 
  int blobs_to_send_;
  char* createBlob(int id);
  void sendBlob();
  handle send_handle_;
  int num_subscriptions_;
 private:
  RmstSrcReceive *mr;
  handle subscribe_handle_;
  int ck_val_;
#ifdef NS_DIFFUSION
  RmstSendDataTimer sdt_;
#endif // NS_DIFFUSION
};

#endif //RMST_SRC
