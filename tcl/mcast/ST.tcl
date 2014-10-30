#
# tcl/mcast/ST.tcl
#
# Copyright (C) 1998 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

# Simple implementation of a shared tree protocol.  No timers.  Nodes
# send grafts/prunes toward the RP to join/leave the group.  The user
# needs to set a protocol variable specifying the RP for a particular
# group (no bootstrap):
#      "ST set RP_($group) $node"
# indicates that $node acts as an RP for the $group
#
# TO DO: local receivers receive packets from local senders only after
# the trip to RP and back.  Would be nice to receive right away using
# Decapsulator's decap-target...

Class ST -superclass McastProtocol

ST instproc init { sim node } {
	ST instvar RP_ decaps_ ;#RPs and decapsulators by group
	if ![info exists RP_] {
		error "ST: 'ST instvar RP_' must be set"
		exit 1
	}
	$self next $sim $node
	$self instvar ns_ node_ id_ encaps_ mctrl_
	set id_ [$node id]

	set mctrl_ [new Agent/Mcast/Control $self]
	$node attach $mctrl_

	foreach grpx [array names RP_] {
		set grp [expr $grpx]
		if [string compare $grp $grpx] {
			set RP_($grp) $RP_($grpx)
			unset RP_($grpx)
		}
		foreach agent [$node set agents_] {
			if {$grp == [$agent set dst_addr_]}  {
				#found an agent that's sending to a group.
				#need to insert an Encapsulator
				$self dbg "attaching a Encapsulator for group $grp"
				set e [new Agent/Encapsulator]
				$e set class_ 32
				$e set status_ 1
				$node attach $e
				$agent target $e

				$e decap-target ""
				lappend encaps_($grp) $e
			}
		}
		#if the node is an RP, need to attach a Decapsulator
		if {$RP_($grp) == $node} {
			$self dbg "attaching a Decapsulator for group $grp"
			set d [new Agent/Decapsulator]
			$node attach $d
			set decaps_($grp) $d
			$node set decaps_($grp) $d 
		}
	}
}

ST instproc start {} {
	ST instvar decaps_     ;# arrays built during init
	$self instvar encaps_  ;# call
	$self instvar ns_

	#interconnect all agents, decapsulators and encapsulators
	#this had better be done in init()
	foreach grp [array names encaps_] {
		foreach e $encaps_($grp) {
			$ns_ simplex-connect $e $decaps_($grp)
		}
	}
	$self next
}

ST instproc join-group  { group {src "x"} } {
	if { $src != "x" } return

	$self instvar node_ ns_
	ST instvar RP_

	set r [$node_ getReps "x" $group]
	if {$r == ""} {
		set iif [$node_ from-node-iface $RP_($group)]
		$self dbg "********* join: adding <x, $group, $iif>"
		$node_ add-mfc "x" $group $iif ""
		set r [$node_ getReps "x" $group]
	}
	if { ![$r is-active] } {
		$self send-ctrl "graft" $RP_($group) $group
	}
	$self next $group ; #annotate
}

ST instproc leave-group { group {src "x"} } {
	if { $src != "x" } return

	$self instvar node_
	ST instvar RP_

	$self next $group
	# check if the rep is active, then send a prune
	set r [$node_ getReps "x" $group]
	if ![$r is-active] {
		$self send-ctrl "prune" $RP_($group) $group
	}
}

ST instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_
	$self dbg "!!!!!ST: wrong iif $iface, src $srcID, grp $group"
	return 0
}


# There should be only one mfc cache miss: the first time it receives a 
# packet to the group (RP). Just install a (x,G) entry.
ST instproc handle-cache-miss { srcID group iface } {
	$self instvar node_
	ST instvar RP_

	# RP gets packets from the local decapsulator, so iface must be "?" (-1)
	if { $iface != -1 } {$self dbg ".....invalid cache miss" }
	$self dbg "cache miss, src: $srcID, group: $group, iface: $iface"
	$self dbg "********* miss: adding <x, $group, $iface, >"
	$node_ add-mfc "x" $group $iface "" 
	return 1
}

ST instproc drop { replicator src dst iface} {
	$self instvar node_ ns_
	ST instvar RP_
	
	# No downstream listeners
	# Send a prune back toward the RP
	$self dbg "drops src: $src, dst: $dst, iface: $iface, replicator: [$replicator set srcID_]"
	
	if {$iface != -1} {
		# so, this packet came from outside of the node
		$self send-ctrl "prune" $RP_($dst) $dst
	}
}

ST instproc recv-prune { from src group iface } {
	$self instvar node_ ns_ id_
	ST instvar RP_ 
	
	set r [$node_ getReps "x" $group]
	if {$r == ""} {
		# it's a cache miss!
		$self dbg "recvd a prune, do nothing........"
		return
	}
	set oif [$node_ iif2oif $iface]
	if ![$r exists $oif] {
		warn "node $id_, got a prune from $from, trying to prune a non-existing interface?"
	} else {
		if [$r is-active-target $oif] {
			$r disable $oif
		}
		# If there are no remaining active output links
		# then send a prune upstream.
		if {![$r is-active]} {
			$self send-ctrl "prune" $RP_($group) $group
		}
	}
}

ST instproc recv-graft { from to group iface} {
	$self instvar node_ ns_ id_
	ST instvar RP_
	
	$self dbg "received a shared graft from: $from, to: $to"
	set r [$node_ getReps "x" $group]
	if { $r == "" } {
		set iif -1
		if { $node_ != $RP_($group) } {
			set nbr [$node_ rpf-nbr $RP_($group)]
			set iiflnk [$ns_ link $nbr $node_] 
			set iif [$node_ link2iif $iiflnk]
		}
		$self dbg "********* graft: adding <x, $group, $iif>"
		$node_ add-mfc "x" $group $iif ""
		set r [$node_ getReps "x" $group]
	}
	if {![$r is-active]} {
		# if this node isn't on the tree, propagate the graft
		# if it's an RP, don't need to, but send-ctrl takes care of that.
		$self send-ctrl "graft" $RP_($group) $group
	}
	# graft on the interface
	$r insert [$node_ iif2oif $iface]
}

#
# send a graft/prune to dst/group up the RPF tree towards dst
#
ST instproc send-ctrl { which dst group } {
        $self instvar mctrl_ ns_ node_
	
	if {$node_ != $dst} {
		set nbr [$node_ rpf-nbr $dst]
		$ns_ simplex-connect $mctrl_ \
				[[[$nbr getArbiter] getType [$self info class]] set mctrl_]
		if { $which == "prune" } {
			$mctrl_ set class_ 30
		} else {
			$mctrl_ set class_ 31
		}        
		$mctrl_ send $which [$node_ id] -1 $group
	}
}

################ Helpers

ST instproc dbg arg {
	$self instvar ns_ node_ id_
	puts [format "At %.4f : node $id_ $arg" [$ns_ now]]
}
