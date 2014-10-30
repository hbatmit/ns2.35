#
# tcl/mcast/DM.tcl
#
# Copyright (C) 1997 by USC/ISI
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
# Ported/Modified by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 

Class DM -superclass McastProtocol

DM set PruneTimeout  0.5
DM set CacheMissMode pimdm ;#or dvmrp (lowercase)

DM instproc init { sim node } {
	$self instvar mctrl_
	set mctrl_ [new Agent/Mcast/Control $self]
	$node attach $mctrl_
	Timer/Iface/Prune set timeout [[$self info class] set PruneTimeout]
	$self next $sim $node
}

DM instproc join-group  { group } {
	$self instvar node_
	$self next $group
	set listOfReps [$node_ getReps * $group]
	foreach r $listOfReps {
		if ![$r is-active] {
			$self send-ctrl "graft" [$r set srcID_] $group
			set nbr [$node_ rpf-nbr [$r set srcID_]]
			set nbrs($nbr) 1
		}
	}
	foreach nbr [array names nbrs] {
		if [$nbr is-lan?] {
			# for each LAN we maintain an array of  counters 
			# of how many local receivers we have on the lan 
			# for a given group
			$nbr instvar receivers_
			if [info exists receivers_($group)] {
				incr receivers_($group)
			} else {
				set receivers_($group) 1
			}
		}
	}
}

DM instproc leave-group { group } {
        $self next $group

	$self instvar node_
	# lan: decrement counters
	set listOfReps [$node_ getReps * $group]
	foreach r $listOfReps {
		set nbr [$node_ rpf-nbr [$r set srcID_]]
		set nbrs($nbr) 1
	}
	foreach nbr [array names nbrs] {
		if [$nbr is-lan?] {
			$nbr instvar receivers_
			if { [info exists receivers_($group)] && \
					$receivers_($group) > 0 } {
				incr receivers_($group) -1
			}
		}
	}
}

DM instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_ ns_
	set inlink  [$node_ iif2link $iface]
	set from [$inlink src]
	$self send-ctrl "prune" $srcID $group [$from id]
	return 0 ;# don't call this method two times
}

DM instproc handle-cache-miss  { srcID group iface } {
	DM instvar CacheMissMode
	$self handle-cache-miss-$CacheMissMode $srcID $group $iface
	return 1 ;#call again
}

DM instproc handle-cache-miss-pimdm { srcID group iface } {
        $self instvar node_ ns_
	
	if { $iface >= 0 } {
		set rpf_nbr [$node_ rpf-nbr $srcID]
		set inlink  [$node_ iif2link $iface]
		set rpflink [$ns_ link $rpf_nbr $node_]

		if { $inlink != $rpflink } {
			set from [$inlink src]
			$self send-ctrl "prune" $srcID $group [$from id]
			return 0; #drop this packet
		}
		set rpfoif [$node_ iif2oif $iface]
	} else {
		set rpfoif ""
	}
	set alloifs [$node_ get-all-oifs]
	set oiflist ""
	foreach oif $alloifs {
		if {$oif == $rpfoif} {
			continue ;#exclude incoming iface
		}
		set dst [[$node_ oif2link $oif] dst]
		if { [$dst is-lan?] && [$dst rpf-nbr $srcID] != $node_  } {
			# exclude also lan oifs for which we are not forwarders
			# this constitutes a form of "centralized" assert mechanism
			continue 
		}
		lappend oiflist $oif
	}
	#set idx [lsearch $oiflist $rpfoif]
	#set oiflist [lreplace $oiflist $idx $idx]

	$node_ add-mfc $srcID $group $iface $oiflist
}

DM instproc handle-cache-miss-dvmrp { srcID group iface } {
        $self instvar node_ ns_

	set oiflist ""
        foreach nbr [$node_ neighbors] {
		# peek into other nodes' routing tables to simulate 
		# child-parent relationship maintained by dvmrp
		set rpfnbr [$nbr rpf-nbr $srcID]
		if { $rpfnbr == $node_ } {
			set link [$ns_ link $node_ $nbr]
			lappend oiflist [$node_ link2oif $link]
		}
		
	}
	$node_ add-mfc $srcID $group $iface $oiflist
}

DM instproc drop { replicator src dst iface} {
	$self instvar node_ ns_

        if { $iface < 0 } {
                # optimization for sender: if no listeners, set the ignore bit, 
		# so this function isn't called for every packet.
		$replicator set ignore_ 1
        } else {
		set from [[$node_ iif2link $iface] src]
		if [$from is-lan?] {
			$self send-ctrl "prune" $src $dst
		} else {
			$self send-ctrl "prune" $src $dst [$from id]
		}
	}
}

DM instproc recv-prune { from src group iface} {
        $self instvar node_ PruneTimer_ ns_

	set r [$node_ getReps $src $group]
	if { $r == "" } { 
		return 0
	}
	set id [$node_ id]
	set tmpoif [$node_ iif2oif $iface]

	if { [$r is-active-target $tmpoif] } {
		$r disable $tmpoif
		if ![$r is-active] {
			if { $src != $id } {
				# propagate prune only if the disabled oif
				# was the last one
				$self send-ctrl prune $src $group
			}
		}
	}
	if ![info exists PruneTimer_($src:$group:$tmpoif)] {
		set PruneTimer_($src:$group:$tmpoif) \
				[new Timer/Iface/Prune $self $src $group $tmpoif $ns_]
	}
	$PruneTimer_($src:$group:$tmpoif) schedule

}

DM instproc recv-graft { from src group iface} {
        $self instvar node_ PruneTimer_ ns_

	set id [$node_ id]
        set r [$node_ getReps $src $group]
	if { $r == "" } {
		if { $id == $src } {
			set iif "?"
		} else {
			set rpfnbr [$node_ rpf_nbr $src]
			set rpflnk [$ns_ link $src $id]
			set iif [$node_ link2iif $rpflnk]
		}
		$node_ add-mfc $src $group $iif ""
	        set r [$node_ getReps $src $group]
	} 
        if { ![$r is-active] && $src != $id } {
                # propagate the graft
                $self send-ctrl graft $src $group
        }
	set tmpoif [$node_ iif2oif $iface]
        $r enable $tmpoif
	if [info exists PruneTimer_($src:$group:$tmpoif)] {
		delete $PruneTimer_($src:$group:$tmpoif)
		unset  PruneTimer_($src:$group:$tmpoif)
	}
}

# send a graft/prune for src/group up to the source or towards $to
DM instproc send-ctrl { which src group { to "" } } {
        $self instvar mctrl_ ns_ node_
	if { $to != "" } {
		set n [$ns_ get-node-by-id $to]
		# we don't want to send anything to a lanNode
		if [$n is-lan?] return
		set toid $to
	} else {
		set toid $src
	}
	set nbr [$node_ rpf-nbr $toid]
	if [$nbr is-lan?] {
		# we're requested to send via a lan: $nbr
		$nbr instvar receivers_
		# send a graft/prune only if there're no other receivers on the lan
		if { [info exists receivers_($group)] && \
				$receivers_($group) > 0 } return 
		# need to send: find the next hope node
		set nbr [$nbr rpf-nbr $toid]
	}
	$ns_ simplex-connect $mctrl_ \
			[[[$nbr getArbiter] getType [$self info class]] set mctrl_]
        if { $which == "prune" } {
                $mctrl_ set class_ 30
        } else {
                $mctrl_ set class_ 31
        }        
        $mctrl_ send $which [$node_ id] $src $group
}

DM instproc timeoutPrune { oif src grp } {
	$self instvar node_ PruneTimer_ ns_
	set r [$node_ getReps $src $grp]

	$r insert $oif
	if [info exists PruneTimer_($src:$grp:$oif)] {
		delete $PruneTimer_($src:$grp:$oif)
		unset PruneTimer_($src:$grp:$oif)
	}
	return
}


Class Timer/Iface/Prune -superclass Timer/Iface
Timer/Iface/Prune set timeout 0.5

Timer/Iface/Prune instproc timeout {} {
	$self instvar proto_ src_ grp_ oif_
	$proto_ timeoutPrune $oif_ $src_ $grp_
}



