#
#  Time-stamp: <2000-09-11 10:21:47 haoboy>
# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#
#  The copyright of this module includes the following
#  linking-with-specific-other-licenses addition:
#
#  In addition, as a special exception, the copyright holders of
#  this module give you permission to combine (via static or
#  dynamic linking) this module with free software programs or
#  libraries that are released under the GNU LGPL and with code
#  included in the standard release of ns-2 under the Apache 2.0
#  license or under otherwise-compatible licenses with advertising
#  requirements (or modified versions of such code, with unchanged
#  license).  You may copy and distribute such a system following the
#  terms of the GNU GPL for this module and the licenses of the
#  other code concerned, provided that you include the source code of
#  that other code when and as the GNU GPL requires distribution of
#  source code.
#
#  Note that people who make modified versions of this module
#  are not obligated to grant this special exception for their
#  modified versions; it is their choice whether to do so.  The GNU
#  General Public License gives permission to release a modified
#  version without this exception; this exception also makes it
#  possible to release a modified version which carries forward this
#  exception.
# 
#  Original source contributed by Gaeil Ahn. See below.
#
#  $Header: /cvsroot/nsnam/ns-2/tcl/mpls/ns-mpls-simulator.tcl,v 1.3 2005/09/16 03:05:45 tomh Exp $

###########################################################################
# Copyright (c) 2000 by Gaeil Ahn                                	  #
# Everyone is permitted to copy and distribute this software.		  #
# Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     #
# this sources.								  #
###########################################################################

#############################################################
#                                                           #
#     File: File for Simulator class                        #
#     Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      #
#                                                           #
#############################################################

Simulator instproc mpls-node args {
	$self node-config -MPLS ON
	set n [$self node]
	$self node-config -MPLS OFF
	return $n
}

Simulator instproc LDP-peer { src dst } {
	# Establish LDP-peering between node $src and $dst. The names src and
	# dst does NOT indicate a single-direction relationship.
        if { ![$src is-neighbor $dst] } {
		return
	}
	set ldpsrc [[$src get-module "MPLS"] make-ldp]
	set ldpdst [[$dst get-module "MPLS"] make-ldp]
	$ldpsrc set-peer [$dst id]
	$ldpdst set-peer [$src id]
        $self connect $ldpsrc $ldpdst
}

Simulator instproc ldp-notification-color {color} {
	$self color 101 $color
}

Simulator instproc ldp-request-color {color} {
	$self color 102 $color
}

Simulator instproc ldp-mapping-color {color} {
	$self color 103 $color
}

Simulator instproc ldp-withdraw-color {color} {
	$self color 104 $color
}

Simulator instproc ldp-release-color {color} {
	$self color 105 $color
}

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
