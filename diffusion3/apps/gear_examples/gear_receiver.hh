//
// gear_receiver.hh : Gear Receiver Include File
// author           : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: gear_receiver.hh,v 1.3 2005/09/13 04:53:46 tomh Exp $
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

#ifndef _GEAR_RECEIVER_HH_
#define _GEAR_RECEIVER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "gear_common.hh"

class GearReceiverReceive;

class GearReceiverApp : public DiffApp {
public:
  #ifdef NS_DIFFUSION
  GearReceiverApp();
  int command(int argc, const char*const* argv);
#else
  GearReceiverApp(int argc, char **argv);
#endif //NS_DIFFUSION
  void recv(NRAttrVec *data, NR::handle my_handle);
  void run();

private:
  // NR Specific variables
  GearReceiverReceive *mr_;
  handle subHandle_;

  // Gear Receiver App variables
  bool using_points_;
  bool using_push_;
  int last_seq_recv_;
  int num_msg_recv_;
  int first_msg_recv_;
  float lat_min_;
  float lat_max_;
  float long_min_;
  float long_max_;
  float lat_pt_;
  float long_pt_;

  handle setupSubscription();
  void parseCommandLine(int argc, char **argv);
  void readGeographicCoordinates();
  void usage(char *s);
};

class GearReceiverReceive : public NR::Callback {
public:
  GearReceiverReceive(GearReceiverApp *app) : app_(app) {};
  void recv(NRAttrVec *data, NR::handle my_handle);

  GearReceiverApp *app_;
};

#endif // !_GEAR_RECEIVER_HH_
