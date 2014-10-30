# 
#  Copyright (c) 2000 by the University of Southern California
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

#  Copyright (C) 1998 by Mingzhou Sun. All rights reserved.
#
#  This software is developed at Rensselaer Polytechnic Institute under 
#  DARPA grant No. F30602-97-C-0274
#  Redistribution and use in source and binary forms are permitted
#  provided that the above copyright notice and this paragraph are
#  duplicated in all such forms and that any documentation, advertising
#  materials, and other materials related to such distribution and use
#  acknowledge that the software was developed by Mingzhou Sun at the
#  Rensselaer  Polytechnic Institute.  The name of the University may not 
#  be used to endorse or promote products derived from this software 
#  without specific prior written permission.
#
# Link State Routing Agent
#
# $Header: /cvsroot/nsnam/ns-2/tcl/rtglib/ns-rtProtoLS.tcl,v 1.4 2005/09/16 03:05:46 tomh Exp $

#
# XXX These default values have to stay here because link state module
# may be disabled during configuration time. If these stay in ns-default.tcl,
# Agent/rtProto/LS may be unknown when ns-default.tcl is loaded.
#
Agent/rtProto/LS set UNREACHABLE  [rtObject set unreach_]
Agent/rtProto/LS set preference_        120
Agent/rtProto/LS set INFINITY           [Agent set ttl_]
Agent/rtProto/LS set advertInterval     1800

# like DV's, except $self cmd initialize and cmd setNodeNumber
Agent/rtProto/LS proc init-all args {
	if { [llength $args] == 0 } {
		set nodeslist [[Simulator instance] all-nodes-list]
	} else { 
		eval "set nodeslist $args"
	}
	Agent set-maxttl Agent/rtProto/LS INFINITY
	eval rtObject init-all $nodeslist
	foreach node $nodeslist {
		set proto($node) [[$node rtObject?] add-proto LS $node]
	}
	foreach node $nodeslist {
		foreach nbr [$node neighbors] {
			set rtobj [$nbr rtObject?]
			if { $rtobj == "" } {
				continue
			}
			set rtproto [$rtobj rtProto? LS]
			if { $rtproto == "" } {
				continue
			}
			$proto($node) add-peer $nbr \
					[$rtproto set agent_addr_] \
					[$rtproto set agent_port_]
		}
	}

	# -- LS stuffs --
	set first_node [lindex $nodeslist 0 ]
	foreach node $nodeslist {
		set rtobj [$node rtObject?]
		if { $rtobj == "" } {
			continue
		}
		set rtproto [$rtobj rtProto? LS]
		if { $rtproto == "" } {
			continue
		}
		$rtproto cmd initialize
		if { $node == $first_node } {
			$rtproto cmd setNodeNumber \
				[[Simulator instance] get-number-of-nodes]
		}
	}
}

# like DV's , except LS_ready
Agent/rtProto/LS instproc init node {
	global rtglibRNG

	$self next $node
	$self instvar ns_ rtObject_ ifsUp_ rtsChanged_ rtpref_ nextHop_ \
		nextHopPeer_ metric_ multiPath_
	Agent/rtProto/LS instvar preference_ 
	
	;# -- LS stuffs -- 
	$self instvar LS_ready
	set LS_ready 0
	set rtsChanged_ 1

	set UNREACHABLE [$class set UNREACHABLE]
	foreach dest [$ns_ all-nodes-list] {
		set rtpref_($dest) $preference_
		set nextHop_($dest) ""
		set nextHopPeer_($dest) ""
		set metric_($dest)  $UNREACHABLE
	}
	set ifsUp_ ""
	set multiPath_ [[$rtObject_ set node_] set multiPath_]
	set updateTime [$rtglibRNG uniform 0.0 0.5]
	$ns_ at $updateTime "$self send-periodic-update"
}

# like DV's
Agent/rtProto/LS instproc add-peer {nbr agentAddr agentPort} {
	$self instvar peers_
	$self set peers_($nbr) [new rtPeer $agentAddr $agentPort $class]
}

# like DV's, except cmd sendUpdates, instead of not send-updates
Agent/rtProto/LS instproc send-periodic-update {} {
	global rtglibRNG
	
	$self instvar ns_

	# -- LS stuffs --
	$self cmd sendUpdates

	set updateTime [expr [$ns_ now] + ([$class set advertInterval] * \
			[$rtglibRNG uniform 0.5 1.5])]
	$ns_ at $updateTime "$self send-periodic-update"
}

# like DV's, except cmd computeRoutes
Agent/rtProto/LS instproc compute-routes {} {
	$self instvar node_
	#puts "Node [$node_ id]: Agent/rtProto/LS compute-routes"
	$self cmd computeRoutes
	$self install-routes
}

# like DV's, except cmd intfChanged(), comment out DV stuff
Agent/rtProto/LS instproc intf-changed {} {
	$self instvar ns_ peers_ ifs_ ifstat_ ifsUp_ nextHop_ \
			nextHopPeer_ metric_
	set INFINITY [$class set INFINITY]
	set ifsUp_ ""
	foreach nbr [lsort -dictionary [array names peers_]] {
		set state [$ifs_($nbr) up?]
		if {$state != $ifstat_($nbr)} {
			set ifstat_($nbr) $state
		}
	}
	# -- LS stuffs --
	$self cmd intfChanged
	$self route-changed
}

;# called by C++ whenever a LSA or Topo causes a change in the routing table
Agent/rtProto/LS instproc route-changed {} {
	$self instvar node_ 
	#puts "At [[Simulator instance] now] node [$node_ id]: Agent/rtProto/LS route-changed"

	$self instvar rtObject_  rtsChanged_
	$self install-routes
	set rtsChanged_ 1
	$rtObject_ compute-routes
}

# put the routes computed in C++ into tcl space
Agent/rtProto/LS instproc install-routes {} {
	$self instvar ns_ ifs_ rtpref_ metric_ nextHop_ nextHopPeer_
	$self instvar peers_ rtsChanged_ multiPath_
	$self instvar node_  preference_ 
    
	set INFINITY [$class set INFINITY]
	set MAXPREF  [rtObject set maxpref_]
	set UNREACH  [rtObject set unreach_]
	set rtsChanged_ 1 
	
	foreach dst [$ns_ all-nodes-list] {
		# puts "installing routes for $dst"
		if { $dst == $node_ } {
			set metric_($dst) 32  ;# the magic number
			continue
		}
		#  puts " [$node_ id] looking for route to [$dst id]"
		set path [$self cmd lookup [$dst id]]
		# puts "$path" ;# debug
		if { [llength $path ] == 0 } {
			# no path found in LS
			set rtpref_($dst) $MAXPREF
			set metric_($dst) $UNREACH
			set nextHop_($dst) ""
			continue
		}
		set cost [lindex $path 0]
		set rtpref_($dst) $preference_
		set metric_($dst) $cost
		
		if { ! $multiPath_ } {
			set nhNode [$ns_ get-node-by-id [lindex $path 1]]
			set nextHop_($dst) $ifs_($nhNode)
			continue
		}
		set nextHop_($dst) ""
		set nh ""
		set count [llength $path]
		foreach nbr [lsort -dictionary [array names peers_]] {
			foreach nhId [lrange $path 1 $count ] {
				if { [$nbr id] == $nhId } {
					lappend nextHop_($dst) $ifs_($nbr)
					break
				}
			}
		}
	}
}

Agent/rtProto/LS instproc send-updates changes {
	$self cmd send-buffered-messages
}

Agent/rtProto/LS proc compute-all {} {
	# Because proc methods are not inherited from the parent class.
}

Agent/rtProto/LS instproc get-node-id {} {
	$self instvar node_
	return [$node_ id]
}

Agent/rtProto/LS instproc get-links-status {} {
	$self instvar ifs_ ifstat_ 
	set linksStatus ""
	foreach nbr [array names ifs_] {
		lappend linksStatus [$nbr id]
		if {[$ifs_($nbr) up?] == "up"} {
			lappend linksStatus 1
		} else {
			lappend linksStatus 0
		}
		lappend linksStatus [$ifs_($nbr) cost?]
	}
	set linksStatus
}

Agent/rtProto/LS instproc get-peers {} {
	$self instvar peers_
	set peers ""
	foreach nbr [lsort -dictionary [array names peers_]] {
		lappend peers [$nbr id]
		lappend peers [$peers_($nbr) addr?]
		lappend peers [$peers_($nbr) port?]
	}
	set peers
}

# needed to calculate the appropriate timeout value for retransmission 
# of unack'ed LSA or Topo messages
Agent/rtProto/LS instproc get-delay-estimates {} {
	$self instvar ifs_ ifstat_ 
	set total_delays ""
	set packet_size 8000.0 ;# bits
	foreach nbr [array names ifs_] {
		set intf $ifs_($nbr)
		set q_limit [ [$intf queue ] set limit_]
		set bw [bw_parse [ [$intf link ] set bandwidth_ ] ]
		set p_delay [time_parse [ [$intf link ] set delay_] ]
		set total_delay [expr $q_limit * $packet_size / $bw + $p_delay]
		$self cmd setDelay [$nbr id] $total_delay
	}
}
