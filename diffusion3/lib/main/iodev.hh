// 
// iodev.hh      : Diffusion IO Devices Include File
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iodev.hh,v 1.8 2005/09/13 04:53:49 tomh Exp $
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
//

// This file defines the basic interface between diffusion device
// drivers and the Diffusion Core (or the Diffusion Routing Library
// API). It provides basic functionality to send and receive packets
// and also keeps track of input/output file descriptors that can be
// used on select().

#ifndef _IODEV_HH_
#define _IODEV_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <list>

#include "header.hh"
#include "tools.hh"

using namespace std;

class DiffusionIO {
public:
  DiffusionIO();

  virtual ~DiffusionIO(){
    // Nothing to do
  };

  virtual void addInFDS(fd_set *fds, int *max);
  virtual bool hasFD(int fd);
  virtual int checkInFDS(fd_set *fds);
  virtual DiffPacket recvPacket(int fd) = 0;
  virtual void sendPacket(DiffPacket p, int len, int dst) = 0;

protected:
  int num_out_descriptors_;
  int num_in_descriptors_;
  int max_in_descriptor_;
  list<int> in_fds_;
  list<int> out_fds_;
};

typedef list<DiffusionIO *> DeviceList;

#endif // !_IODEV_HH_
