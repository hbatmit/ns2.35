//
// ping_sender.hh : Ping Sender Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: 2pp_ping_sender.hh,v 1.2 2005/09/13 04:53:46 tomh Exp $
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

#ifndef _PING_SENDER_HH_
#define _PING_SENDER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "ping.hh"

class TPPPingSenderReceive;
class TPPPingSenderApp;

#define SEND_DATA_INTERVAL 5

#ifdef NS_DIFFUSION
class TPPPingSendDataTimer : public TimerHandler {
  public:
  TPPPingSendDataTimer(TPPPingSenderApp *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
protected:
  TPPPingSenderApp *a_;
};
#endif //NS_DIFFUSION

class TPPPingSenderApp : public DiffApp {
public:
#ifdef NS_DIFFUSION
  TPPPingSenderApp();
  int command(int argc, const char*const* argv);
  void send();
#else
  TPPPingSenderApp(int argc, char **argv);
#endif // NS_DIFFUSION

  virtual ~TPPPingSenderApp()
  {
    // Nothing to do
  };

  void run();
  void recv(NRAttrVec *data, NR::handle my_handle);

private:
  // NR Specific variables
  TPPPingSenderReceive *mr_;
  handle subHandle_;
  handle pubHandle_;

  // Ping App variables
  int num_subscriptions_;
  int last_seq_sent_;
  NRAttrVec data_attr_;
  NRSimpleAttribute<void *> *timeAttr_;
  NRSimpleAttribute<int> *counterAttr_;
  EventTime *lastEventTime_;

  handle setupSubscription();
  handle setupPublication();
#ifdef NS_DIFFUSION
  TPPPingSendDataTimer sdt_;
#endif // NS_DIFFUSION
};

class TPPPingSenderReceive : public NR::Callback {
public:
  TPPPingSenderReceive(TPPPingSenderApp *app) : app_(app) {};
  void recv(NRAttrVec *data, NR::handle my_handle);

  TPPPingSenderApp *app_;
};

#endif // !_PING_SENDER_HH_
