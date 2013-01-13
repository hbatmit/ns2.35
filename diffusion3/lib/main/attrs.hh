//
// attrs.hh        : Attribute Functions Definitions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: attrs.hh,v 1.7 2005/09/13 04:53:49 tomh Exp $
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

#ifndef _ATTRS_HH_
#define _ATTRS_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <unistd.h>
#include <netinet/in.h>

#include "nr/nr.hh"
#include "header.hh"
#include "tools.hh"

// Here we define a few functions that will help developers create
// and manipulate attribute sets.

// CopyAttrs returns a NRAttrVec containing a copy of the attributes
// given in 'src_attrs' (which are not changed).
NRAttrVec * CopyAttrs(NRAttrVec *src_attrs);

// AddAttrs adds all attributes from 'attr_vec2' to 'attr_vec1. The
// attributes from 'attr_vec2' remain intact and its attributes are
// copied and added to 'attr_vec1'
void AddAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);

// ClearAttrs deletes all attributes from 'attr_vec'. The result is an
// empty NRAttrVec
void ClearAttrs(NRAttrVec *attr_vec);

// PrintAttrs prints the contents of all attributes given in
// 'attr_vec' to stderr. The attribute vector is not changed
void PrintAttrs(NRAttrVec *attr_vec);

// AllocateBuffer returns a buffer (DiffPacket) with sufficient space
// for the attributes in 'attr_vec' to be packaged by the function
// PackAttrs. The attribute vector is not changed
DiffPacket AllocateBuffer(NRAttrVec *attr_vec);

// CalculateSize returns the buffer size necessary to hold all the
// attributes from 'attr_vec' if the PackAttrs function is used
int CalculateSize(NRAttrVec *attr_vec);

// PackAttrs packs all attributes given in 'attr_vec' on the buffer
// starting at 'start_pos'. It assumes there is enough space in the
// buffer to accomodate all attributes
int PackAttrs(NRAttrVec *attr_vec, char *start_pos);

// UnpackAttrs returns an attribute vector containing 'num_attr'
// attributes, which are unpacked from 'pkt'
NRAttrVec * UnpackAttrs(DiffPacket pkt, int num_attr);

// PerfectMatch returns TRUE if the attributes from 'attr_vec1' and
// 'attr_vec2' are identical. For each attribute in each vector, there
// must be the same attribute in the other vector
bool PerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);

// OneWayPerfectMatch returns TRUE if each attribute in 'attr_vec1' is
// also present (identical attribute) in 'attr_vec2'
bool OneWayPerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);

// MatchAttrs returns TRUE if the attributes from 'attr_vec1' and
// 'attr_vec2' match each other (Please refer to the API document for
// a complete description of the matching rules).
bool MatchAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);

// OneWayMatch returns TRUE if the attributes from 'attr_vec2' match
// the attributes in 'attr_vec1'. (I.E. For each attribute in
// 'attr_vec1' that has an operator different than 'IS', we need to
// find a match in 'attr_vec2'
bool OneWayMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);

#endif // !_ATTRS_HH_
