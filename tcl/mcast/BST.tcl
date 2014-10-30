#
# tcl/mcast/BST.tcl
#
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
# 
#
#
# Written by Yuri Pryadkin
# Modified by Nader Saelhi
#
###############
# Implementation of a simple shared bi-directional tree protocol.  No
# timers.  Nodes send grafts/prunes toward the RP to join/leave the
# group.  The user needs to set two protocol variables: 
#
# "BST set RP_($group) $node" - indicates that $node 
#                               acts as an RP for the $group

Class BST -superclass McastProtocol

BST instproc init { sim node } {
	$self instvar mctrl_ oiflist_
	BST instvar RP_

	set mctrl_ [new Agent/Mcast/Control $self]
	$node attach $mctrl_
	$self next $sim $node
}

BST instproc start {} {
	$self instvar node_ oiflist_
	BST instvar RP_

	# need to do it in start when unicast routing is computed
	foreach grpx [array names RP_] {
                set grp [expr $grpx]
#	      	$self dbg "BST: grp $grp, node [$node_ id]"
               	if { [string compare $grp $grpx] } {
			set RP_($grp) $RP_(grpx)
			unset RP_($grpx)
               	}
		set rpfiif [$node_ from-node-iface $RP_($grp)]
#		$self dbg "BST: rpfiif $rpfiif"
		if { $rpfiif != "?" } {
			set rpfoif [$node_ iif2oif $rpfiif]
		} else {
			set rpfoif ""
		}

		# initialize with the value of rpfoif
		set oiflist_($grp) $rpfoif
		set neighbors [$node_ set neighbor_]
		if [info exists neighbors] {
			for {set i 0} {$i < [llength $neighbors]} {incr i} {
				set neighbor [lindex $neighbors $i]
				set class_info [$neighbor info class]
				if {$class_info == "LanNode"} {
					# This node is on a LAN, we
					# must designate a router for
					# the mcast group   
					$neighbor designate-ump-router $grp \
					    $RP_($grp)
				}
			}
		}
	}

}

BST instproc join-group  { group {src "x"} } {
	$self instvar node_ ns_ oiflist_
	BST instvar RP_
	
	set nbr [$node_ rpf-nbr $RP_($group)]
	set nbrs($nbr) 1
	$node_ add-mark m1 blue "[$node_ get-shape]"
	foreach nbr [array names nbrs] {
		if [$nbr is-lan?] {
			$nbr instvar receivers_
			if [info exists receivers_($group)] {
				incr receivers_($group)
			} else {
				$self send-ctrl "graft" $RP_($group) $group
				set receivers_($group) 1
			}
		}
		$self next $group ; #annotate
	}

	if { ![$node_ check-local $group] || [$node_ getReps "x" \
					      $group] == ""} { 
# 		$self dbg "Sending join-group"
		$self send-ctrl "graft" $RP_($group) $group
	}

}

BST instproc leave-group { group {src "x"} } {
	BST instvar RP_ 

	$self next $group ;#annotate
	$self instvar node_ oiflist_

	set nbr [$node_ rpf-nbr $RP_($group)]
	if  [$nbr is-lan?] {
		$nbr instvar receivers_
		if [info exists receivers_($group)] {
			if {$receivers_($group) > 0} {
				incr receivers_($group) -1
				if {$receivers_($group) == 0} {
					$node_ delete-mark m1
					$self send-ctrl "prune" $RP_($group) $group
				}
			}
		} else {
			# Nobody has joined yet
			return
		}
	} else {
		set rpfiif [$node_ from-node-iface $RP_($group)]
		if { $rpfiif != "?" } {
			set rpfoif [$node_ iif2oif $rpfiif]
		} else {
			set rpfoif ""
		}
		if { $oiflist_($group) == \
			 $rpfoif && ![$node_ check-local $group] } {
			# propagate
			$self send-ctrl "prune" $RP_($group) $group
			$node_ delete-mark m1
		} else {
			$node_ delete-mark m1
			$node_ add-mark m2 red circle
		}
	}

}

# handle-wrong-iif
# This function does nothing
BST instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_ oiflist_
	BST instvar RP_
	
#    	$self dbg "BST: wrong iif $iface, src $srcID, grp $group"
#    	$self dbg "\t oiflist: $oiflist_($group)"

	set rep [$node_ getReps "x" $group]

	$node_ add-mfc "x" $group $iface $oiflist_($group)
	set iif [$node_ lookup-iface "x" $group]
	if { $iface >= 0 } {
		set oif [$node_ iif2oif $iface]
		set rpfiif [$node_ from-node-iface $RP_($group)]
		if { $iface == $rpfiif } {
			# forward direction: disable oif to RP
			$rep disable [$node_ iif2oif $rpfiif]
		} else {
			# reverse direction: disable where it came from
			$rep disable $oif
			if { $node_ != $RP_($group) } {
				$rep insert [$node_ iif2oif $rpfiif]
			}
		}
	}
	$node_ change-iface "x" $group $iif $iface
	return 1 ;#classify packet again
}

# handle-cache-miss
# Creates a (*, G) entry for a group.  
BST instproc handle-cache-miss { srcID group iface } {
	$self instvar node_  ns_ oiflist_
	BST instvar RP_
	
	if { [$node_ getReps "x" $group] != "" } {
		error
	}

	
#      	$self dbg \
#  	    "handle-cache-miss, src: $srcID, group: $group, iface: $iface"
#     	$self dbg \
#  	    "********* miss: adding <x, $group, $iface, $oiflist_($group)>"

	# Check if the node is on a LAN.  If so we should NOT resume
	# if:
	#	1) The node is not the next upstream router, and
	#	2) The incoming interface is to a LanNode, and
	#	3) The node is not a source.

 	if {$iface != -1} {
		# if the node is not a source (3)
 		set neighbors [$node_ set neighbor_]
		if [info exists neighbors] {
			for {set i 0} {$i < [llength $neighbors]} {incr i} {
				set neighbor [lindex $neighbors $i]
				set nbr [$node_ rpf-nbr $RP_($group)]
				if {[$neighbor is-lan?] && \
					[$nbr info class] != "LanNode"} {
					# The node is directly
					# connected to RP --or at
					# least not via LanNode
					$neighbor instvar up_
					set up [$neighbor set up_($group)]
					if {$node_ != $up} {
						# If not the upstream
						# router (1) 
						if [$self link2lan? $neighbor \
							$iface] {
							# The interface is to 
							# the LAN
							return 0
						}
					}
				}
			}
		}
 	}
    
	$node_ add-mfc "x" $group $iface $oiflist_($group)

	if { $iface > 0 } {
		#disable reverse iface
		set rep [$node_ getReps "x" $group]
		$rep disable [$node_ iif2oif $iface]
	}
	return 1 ;# classify the packet again.
}

BST instproc drop { replicator src dst iface} {
	$self instvar node_ ns_
	BST instvar RP_
	
	# No downstream listeners? Just drop the packet. No purning is
	# necessary  
# 	$self dbg "drops src: $src, dst: $dst, replicator: [$replicator set srcID_]"
	
	if {$iface >= 0} {
		# so, this packet came from outside of the node.
		# Since PIM works based on explicit join mechanism,
		# the only action is to drop unwanted packets.
# 		$self dbg "drops the unwanted packet"
	}
}

BST instproc recv-prune { from src group iface} {
	$self instvar node_ ns_ oiflist_ 
	BST instvar RP_ 
# 	$self dbg "received a prune from: $from, src: $src, grp: $group, if: $iface"

	set rep [$node_ getReps "x" $group]
	if {$rep != ""} {
		set oif [$node_ iif2oif $iface]
		set idx [lsearch $oiflist_($group) $oif]
		if { $idx >= 0 } {
			set oiflist_($group) [lreplace $oiflist_($group) $idx $idx]
			$rep disable $oif
			set rpfiif [$node_ from-node-iface $RP_($group)]
			if { $rpfiif != "?" } {
				set rpfoif [$node_ iif2oif $rpfiif]
			} else {
				set rpfoif ""
			}
			if { $oiflist_($group) == $rpfoif && ![$node_ check-local $group] } {
				# propagate
				$node_ delete-mark m2
				$self send-ctrl "prune" $RP_($group) $group
			}
		}
	}
}

BST instproc recv-graft { from to group iface } {
	$self instvar node_ ns_ oiflist_
	BST instvar RP_
	
#  	$self dbg "received a graft from: $from, to: $to, if: $iface"
	set oif [$node_ iif2oif $iface]
	set rpfiif [$node_ from-node-iface $RP_($group)]
	if { $rpfiif != "?" } {
		set rpfoif [$node_ iif2oif $rpfiif]
	} else {
		set rpfoif ""
	}

	if { $oiflist_($group) == $rpfoif && ![$node_ check-local $group] } {
		# propagate
		$node_ add-mark m2 red circle
		$self send-ctrl "graft" $RP_($group) $group
	}
	if { [lsearch $oiflist_($group) $oif] < 0 } {
		lappend oiflist_($group) $oif
		if { [$node_ lookup-iface "x" $group] != $iface } {
			set rep [$node_ getReps "x" $group]
			if { $rep != "" } {
				$rep insert $oif
			}
		}
	}
}

#
# send a graft/prune for src/group up the RPF tree towards dst
#
BST instproc send-ctrl { which dst group } {
        $self instvar mctrl_ ns_ node_
	
	if {$node_ != $dst} {
		set nbr [$node_ rpf-nbr $dst]
		if [$nbr is-lan?] {
			# we're requested to send via a lan
			$nbr instvar receivers_
			# send graft/prune only if there's no other receiver.
			if { [info exists receivers_($group)] && \
				 $receivers_($group) > 0 } return

			set nbr [$nbr rpf-nbr $dst]
#  			$self dbg "BST::send-ctrl The first hop router to LAN is node [$nbr id]"
		}

#  		$self dbg "BST::send-ctrl $ns_ simplex-connect \[\[\[$nbr getArbiter\] getType \[$self info class\]\] set mctrl_\]"

		$ns_ simplex-connect $mctrl_ \
				[[[$nbr getArbiter] getType [$self info class]] set mctrl_]
		if { $which == "prune" } {
			$mctrl_ set fid_ 2
		} else {
			$mctrl_ set fid_ 3
		}
# 		$self dbg "BST::send-ctrl $mctrl_ ([$mctrl_ info class]) send $which [$node_ id] $dst $group"
		$mctrl_ send $which [$node_ id] $dst $group
	}
}

################ Helpers

BST instproc dbg arg {
	$self instvar ns_ node_
	puts [format "At %.4f : node [$node_ id] $arg" [$ns_ now]]
}


					
# designate-ump-router
# Designates a router on a LAN in order to avoid transmission of
# duplicate and redundant messages.  The node with the highest ID is
# chosen as the designated router.
LanNode instproc designate-ump-router {group dst} {
	$self instvar nodelist_
	$self instvar up_

	set nbr [$self rpf-nbr $dst]
	set up_($group) $nbr
	return
}


# Returns the next hop router to a node
BST instproc next-hop-router {node group} {
	BST instvar RP_

	set nbr [$node rpf-nbr $RP_($group)]
	if [$nbr is-lan?] {
		set nbr [$nbr rpf-nbr $RP_($group)]
	}
	return $nbr
}

# Checks if an mcast group is BST 
BST instproc is-group-bidir? {group} {
	BST instvar RP_

	foreach grp [array names RP_] {
		if {$grp == $group} {
			return 1
		}
	}
	return 0
}

# Finds out which Connector or Vlink corresponds to (link)
BST instproc match-oif {group link} {
	$self instvar oiflist_

 	set oiflist $oiflist_($group)
  	if {$oiflist != ""} {
		foreach oif $oiflist {
			set oiflink [$oif set link_]
			if {$oiflink == $link} {
				return $oiflink
			}
		}
  	}
	return
}

# Finds the outgoing link to the next node (dst)
BST instproc find-oif {dst group} {
	$self instvar node_ ns_

	if {$node_ != $dst} {
		set ns [$self set ns_]
		$ns instvar link_
		set link [$ns set link_([$node_ id]:[$dst id])]
		# Now find which Connector or VLink corresponds to this link
		return [$self match-oif $group $link]
	} else {
		return ""
	}
}

# Finds if the interface (iface) is toward the LAN represented by (neighbor) 
# or not.
BST instproc link2lan? {neighbor iface} {
	$self instvar node_ ns_

	set link1 [[[$node_ iif2oif $iface] set link_] set iif_]
	set link2 [[$ns_ link $node_ $neighbor] set iif_]
    	if {$link1 == $link2} {
		return 1
	} else {
		return 0
	}
}

####################
Class Classifier/Multicast/Replicator/BST -superclass Classifier/Multicast/BST

#
# This method called when a new multicast group/source pair
# is seen by the underlying classifier/mcast object.
# We install a hash for the pair mapping it to a slot
# number in the classifier table and point the slot
# at a replicator object that sends each packet along
# the RPF tree.
#
Classifier/Multicast/BST instproc new-group { src group iface code} {
	$self instvar node_
	$node_ new-group $src $group $iface $code
}

Classifier/Multicast/BST instproc no-slot slot {
	# NOTHING
}

Classifier/Multicast/Replicator/BST instproc init args {
	$self next
	$self instvar nrep_
	set nrep_ 0
}

Classifier/Multicast/Replicator/BST instproc add-rep { rep src group iif } {
	$self instvar nrep_
	$self set-hash $src $group $nrep_ $iif
	$self install $nrep_ $rep
	incr nrep_
}

####################
# match-BST-iif
# Performs the RPF 
Classifier/Multicast/Replicator/BST instproc match-BST-iif {iface group} {
	$self instvar node_

	list retval_
	set agents [$node_ set agents_]
	for {set i 0} {$i < [llength $agents]} {incr i} {
		# Check if you can find a BST agent
		set agent [lindex $agents $i]
		$agent instvar proto_
		if [info exists proto_] {
			set protocol [$agent set proto_]
			if {[$protocol info class] == "BST"} {
				# See if the group is a BST group
				BST instvar RP_
				$protocol instvar oiflist_
				set bidir [$protocol is-group-bidir? $group]
				if {$bidir == 1} {
					if {$node_ == $RP_($group)} {
						return 1
					}

					# Find the incoming interface
					# from RP.
					set iif [$node_ from-node-iface \
						     $RP_($group)]
					if {$iif == $iface} {
						return 1
					} else {
						return 0
					}
				}
			}
		}
	}
	return -1
}

# Classifier/Multicast/Replicator upstream-link
# Finds the link from the next hop upstream router to the current
# node.  This function is called from C++ to provide the classifier
# with appropriate information to set the UMP option.
Classifier/Multicast/Replicator/BST instproc upstream-link {group} {
	$self instvar node_

	list retval_
	set agents [$node_ set agents_]
	for {set i 0} {$i < [llength $agents]} {incr i} {
		# Check if you can find a BST agent
		set agent [lindex $agents $i]
		$agent instvar proto_
		if [info exists proto_] {
			set protocol [$agent set proto_]
			if {[$protocol info class] == "BST"} {
				# See if the group is a BST group
				BST instvar RP_
				$protocol instvar oiflist_
				set bidir [$protocol is-group-bidir? $group]
				if {$bidir == 1} {
					# Get the next node, REAL or VIRTUAL!
					set nbr [$node_ rpf-nbr $RP_($group)]

					# Find the outgoing link to the next
					# node 
					set oif [$protocol find-oif $nbr \
						     $group]

					if {$oif == ""} {
						set oif "self"
					} 
					lappend retval_ $oif

					# Now attach the next node's ID
					# If the neighbor is a virtual
					# name, we need to find the
					# designated router on the
					# LAN. Currently, the
					# designated router is the one
					# which is the closest to the RP.
					if [$nbr is-lan?] {
					    set nbr [$nbr rpf-nbr $RP_($group)]
					}
					lappend retval_ [$nbr id]
					return $retval_

				}
			}
		}
	}
# 	$self dbg "There is no agent for this node!"
	return {}
}

# check-rpf-link 
# Finds out the RPF link for a node
Classifier/Multicast/Replicator/BST instproc check-rpf-link {node group} {
	$self instvar node_

	set agents [$node_ set agents_]
	for {set i 0} {$i < [llength $agents]} {incr i} {
		# Check if you can find a BST agent
		set agent [lindex $agents $i]
		$agent instvar proto_
		if [info exists proto_] {
			set protocol [$agent set proto_]
			set classInfo [$protocol info class]
			if {$classInfo == "BST"} {
				# See if the group is a BST group
				BST instvar RP_
				set rpfiif [$node_ from-node-iface \
						$RP_($group)]
				return $rpfiif
			}
		}
	}
	return -1
}


