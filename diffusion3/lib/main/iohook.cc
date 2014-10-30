//
// iohook.cc     : IO Hook for Diffusion
// Authors       : Fabio Silva and Yutaka Mori
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iohook.cc,v 1.2 2005/09/13 04:53:49 tomh Exp $
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

#include <stdio.h>
#include <stdlib.h>

#include "iohook.hh"

IOHook::IOHook() : DiffusionIO()
{
}

DiffPacket IOHook::recvPacket(int fd)
{
  DeviceList::iterator device_itr;
  DiffPacket incoming_packet = NULL;

  for (device_itr = in_devices_.begin();
       device_itr != in_devices_.end(); ++device_itr){
    if ((*device_itr)->hasFD(fd)){
      // We now have the correct device
      incoming_packet = (*device_itr)->recvPacket(fd);
      break;
    }
  }

  // Make sure packet was received
  if (incoming_packet == NULL){
    DiffPrint(DEBUG_ALWAYS, "Cannot read packet from driver !\n");
    return NULL;
  }

  return incoming_packet;
}

void IOHook::sendPacket(DiffPacket pkt, int len, int dst)
{
  DeviceList::iterator device_itr;

  for (device_itr = out_devices_.begin();
       device_itr != out_devices_.end(); ++device_itr){
    (*device_itr)->sendPacket(pkt, len, dst);
  }
}

void IOHook::addInFDS(fd_set *fds, int *max)
{
  DeviceList::iterator device_itr;

  for (device_itr = in_devices_.begin();
       device_itr != in_devices_.end(); ++device_itr){
    (*device_itr)->addInFDS(fds, max);
  }
}

int IOHook::checkInFDS(fd_set *fds)
{
  DeviceList::iterator device_itr;
  int fd;

  for (device_itr = in_devices_.begin();
       device_itr != in_devices_.end(); ++device_itr){
    fd = (*device_itr)->checkInFDS(fds);
    if (fd != 0)
      return fd;
  }

  return 0;
}

