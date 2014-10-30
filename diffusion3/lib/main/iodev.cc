// 
// iodev.cc      : Diffusion IO Devices
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iodev.cc,v 1.8 2005/09/13 04:53:49 tomh Exp $
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

#include "iodev.hh"

DiffusionIO::DiffusionIO()
{
  num_out_descriptors_ = 0;
  num_in_descriptors_ = 0;
  max_in_descriptor_ = 0;
}

void DiffusionIO::addInFDS(fd_set *fds, int *max)
{
  list<int>::iterator itr;

  for (itr = in_fds_.begin(); itr != in_fds_.end(); ++itr){
    FD_SET(*itr, fds);
  }

  if (max_in_descriptor_ > *max)
    *max = max_in_descriptor_;
}

int DiffusionIO::checkInFDS(fd_set *fds)
{
  list<int>::iterator itr;
  int fd;

  for (itr = in_fds_.begin(); itr != in_fds_.end(); ++itr){
    fd = *itr;
    if (FD_ISSET(fd, fds)){
      return (fd);
    }
  }
  return -1;
}

bool DiffusionIO::hasFD(int fd)
{
  list<int>::iterator itr;
  int device_fd;

  for (itr = in_fds_.begin(); itr != in_fds_.end(); ++itr){
    device_fd = *itr;
    if (device_fd == fd)
      return true;
  }
  return false;
}
