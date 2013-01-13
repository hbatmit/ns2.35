//
// gear_attr.hh   : GEAR Filter Attribute Definitions
// authors        : Yan Yu and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// Copyright (C) 2000-2002 by the University of California
// $Id: gear_attr.hh,v 1.2 2005/09/13 04:53:48 tomh Exp $
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

#ifndef _GEAR_ATTR_HH_
#define _GEAR_ATTR_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "diffapp.hh"

// GEAR Attribute's keys
#define	GEO_LONGITUDE_KEY        6000
#define	GEO_LATITUDE_KEY         6001
#define	GEO_REMAINING_ENERGY_KEY 6002
#define	GEO_BEACON_TYPE_KEY      6003
#define	GEO_INTEREST_MODE_KEY    6004
#define	GEO_INTEREST_HOPS_KEY    6005
#define	GEO_HEADER_KEY	         6006
#define	GEO_H_VALUE_KEY		 6007

extern NRSimpleAttributeFactory<double> GeoLongitudeAttr;
extern NRSimpleAttributeFactory<double> GeoLatitudeAttr;
extern NRSimpleAttributeFactory<double> GeoRemainingEnergyAttr;
extern NRSimpleAttributeFactory<int> GeoBeaconTypeAttr;
extern NRSimpleAttributeFactory<void *> GeoHeuristicValueAttr;
extern NRSimpleAttributeFactory<int> GeoInterestModeAttr;
extern NRSimpleAttributeFactory<void *> GeoHeaderAttr;

#endif // !_GEAR_ATTR_HH_
