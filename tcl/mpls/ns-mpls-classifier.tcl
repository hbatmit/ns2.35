# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-09-11 15:34:10 haoboy>
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
#  $Header: /cvsroot/nsnam/ns-2/tcl/mpls/ns-mpls-classifier.tcl,v 1.3 2005/09/16 03:05:45 tomh Exp $

###########################################################################
# Copyright (c) 2000 by Gaeil Ahn                                	  #
# Everyone is permitted to copy and distribute this software.		  #
# Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     #
# this sources.								  #
###########################################################################

#############################################################
#                                                           #
#     File: File for Classifier                             #
#     Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      #
#                                                           #
#############################################################


Classifier/Addr/MPLS instproc init {args} {
	eval $self next $args
	$self set rtable_ ""
}

Classifier/Addr/MPLS instproc set-node { node module } {
	$self set mpls_node_ $node
	$self set mpls_mod_ $module
}

Classifier/Addr/MPLS instproc no-slot args {
	# Do nothing, just don't exit like the default no-slot{} does.
}


Classifier/Addr/MPLS instproc trace-packet-switching { time src dst ptype \
		ilabel op oiface olabel ttl psize } {
	$self instvar mpls_node_ 
	puts "$time [$mpls_node_ id]($src->$dst): $ptype $ilabel $op $oiface $olabel $ttl $psize"
}

# XXX Temporary interfaces
#
# All of the following instprocs should be moved into the MPLS functionality
# part of a Node. The big picture is that MPLS code should be packaged in a 
# module, then a Node should intercept add-route and pass it on to the MPLS 
# module, which then dispatches it to route-new, etc., depending on the 
# MPLS classifier status of the slot. This should not be done as a callback 
# from the classifier.

Classifier/Addr/MPLS instproc ldp-trigger-by-switch { fec } {
	$self instvar mpls_node_ mpls_mod_
	if { [Classifier/Addr/MPLS on-demand?] == 1 } {
		set msgid  1
	} else {
		set msgid -1
	}
	$mpls_mod_ ldp-trigger-by-data $msgid [$mpls_node_ id] $fec *
}

# XXX This is a really bad way to check if routing table is built.
# During initialization of dynamic routing (e.g., DV), at each node, for 
# each possible destination, a nullAgent_ will be added to the routing table.
#
# Since rtable-ready does not know this, it will think that routing table
# is ready and start to compute routes. This is the reason that routing-new{}
# must do a special check (route-nochange{} does not need it because it is 
# only called when an existing slot is replaced, which cannot happen during 
# the initialization phase).
#
# A better way should be let Node to intercept add-route{}, and updated 
# rtable_ only when the target is not the default null agent. However, this 
# is difficult with current node-config design, where node types are exclusive.
# But this will change as time goes by.
Classifier/Addr/MPLS instproc rtable-ready { fec } {
	$self instvar rtable_
	#
	# determine whether or not a routing table is stable status
	#
	set ns [Simulator instance]
	if { [lsearch $rtable_ $fec] == -1 } {
		lappend rtable_ $fec
	}
	set rtlen [llength $rtable_]
	set nodelen [$ns array size Node_]
	if { $rtlen == $nodelen } {
		return 1
	} else {
		return 0
	}
}

Classifier/Addr/MPLS instproc routing-new { slot time } {
	$self instvar mpls_node_ rtable_ mpls_mod_
	if { [$self control-driven?] != 1 } {
		return
	}
	if { [lsearch $rtable_ [$mpls_node_ id]] == -1 } {
		lappend rtable_ [$mpls_node_ id]
	}
	if { [$self rtable-ready $slot] == 1 } {
		# Now, routing table is built.  really ?
		# Check whether static routing or dynamic routing
		set rtlen [llength $rtable_]
		for {set i 0} {$i < $rtlen} {incr i 1} {
			set nodeid [lindex $rtable_ $i]
			if { [$mpls_mod_ get-nexthop $nodeid] == -1 } {
				#
				# It's Dynamic Routing
				#
				set rtable_ "" 
				return
			}
		}
		# XXX The following piece of code is only reached for 
		# static routing but not for dynamic routing. We have to 
		# do the scheduling because this piece may be executed 
		# BEFORE '$ns run' is completed; thus calling ldp-... 
		# will not succeed because other initialization may not 
		# have all finished yet. 
		set rtable_ "" 
		[Simulator instance] at [expr $time] \
				"$mpls_mod_ ldp-trigger-by-routing-table"
	}
}

# XXX Why routing table should be updated when there is no change??
Classifier/Addr/MPLS instproc routing-nochange {slot time} {
	$self instvar mpls_node_ rtable_ mpls_mod_
	
	if { [$self control-driven?] != 1 } {
		return
	}
	if { [lsearch $rtable_ [$mpls_node_ id]] == -1 } {
		lappend rtable_ [$mpls_node_ id]
	}
	if { [$self rtable-ready $slot] == 1 } {
		set rtable_ "" 
  		[Simulator instance] at $time \
				"$mpls_mod_ ldp-trigger-by-routing-table"
	}
}

Classifier/Addr/MPLS instproc routing-update {slot time} {
	$self instvar mpls_mod_ rtable_
	if {[$self control-driven?] != 1} {
		return
	}
	set fec $slot
	set pft_outif [$mpls_mod_ get-outgoing-iface $fec -1]
	set rt_outif  [$mpls_mod_ get-nexthop $fec]
	if { $pft_outif == -1 || $rt_outif == -1 } {
		return
	}
	$mpls_mod_ ldp-trigger-by-control $fec *
	return
}

