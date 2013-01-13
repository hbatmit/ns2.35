#
# tcl/ctr-mcast/CtrMcastComp.tcl
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
# Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 
#
########## CtrMcastComp Class ##################################
########## init {}
########## compute-tree {src group Ttype}
########## compute-branch {src group member Ttype}

Class CtrMcastComp

CtrMcastComp instproc init sim {
	$self instvar ns_

	set ns_ $sim
	$self init-groups
	$ns_ maybeEnableTraceAll $self {}
}

CtrMcastComp instproc id {} {
	return 0
}

CtrMcastComp instproc trace { f nop {op ""} } {
        $self instvar ns_ dynT_
	if {$op == "nam" && [info exists dynT_]} {
		foreach tr $dynT_ {
			$tr namattach $f
		}
	} else {
		lappend dynT_ [$ns_ create-trace Generic $f $self $self $op]
	}
}

##### Main computation functions #####
CtrMcastComp instproc reset-mroutes {} {
	$self instvar ns_

	foreach node [$ns_ all-nodes-list] {
		foreach group [$self groups?] {
		    set class_info [$node info class]
		    if {$class_info != "LanNode"} {
			$node clearReps * $group
		    }
#			*,G match catches all sources.
# 			foreach src [$self sources? $group] {
# 				$node clearReps [$src id] $group
# 			}
		}
	}
}

CtrMcastComp instproc compute-mroutes {} {
	$self reset-mroutes
	foreach group [$self groups?] {
		foreach src [$self sources? $group] {
			$self compute-tree $src $group
		}
	}
}

CtrMcastComp instproc compute-tree { src group } {
	foreach mem [$self members? $group] {
		$self compute-branch $src $group $mem
	}
}


CtrMcastComp instproc compute-branch { src group nodeh } {
	$self instvar ns_

	### set (S,G) join target
	set tt [$self treetype? $group]
	if { $tt == "SPT" } {
		set target $src
	} elseif { $tt == "RPT" } {
		set target [$self get_rp $nodeh $group]
	}

	for {
		set downstreamtmp ""
		set tmp $nodeh
	} { $downstreamtmp != $target } {
		set downstreamtmp $tmp
		set tmp [$tmp rpf-nbr $target]
	} {

		if {[SessionSim set MixMode_] && $downstreamtmp != "" && ![$ns_ detailed-link? [$tmp id] [$downstreamtmp id]]} {
		    # puts "joining into session area"
		    break
		}


		### set iif : RPF link interface label
		if {$tmp == $target} {
		    # at src or RP
		    set iif -1
		} else {
		    set rpfl [$ns_ link [$tmp rpf-nbr $target] $tmp]

		    if {[SessionSim set MixMode_] && $rpfl == ""} {
			# in mix mode: default -1 unless find a 
			# detailed link on the rpf path
			set iif -1
			set ttmp $tmp
			while {$ttmp != $target} {
			    set rpfl [$ns_ link [$ttmp rpf-nbr $target] $ttmp]
			    if {$rpfl != ""} {
				set iif [$rpfl if-label?]
				break
			    }
			    set ttmp [$ttmp rpf-nbr $target]
			}
		    } else {
			# in regular detailed mode
			set iif [$rpfl if-label?]
		    }
		}

		### set oif : RPF link
		set oiflist ""
		if { $downstreamtmp != "" } {
			set rpfnbr [$downstreamtmp rpf-nbr $target]
			if { $rpfnbr == $tmp } {
			    set rpflink [$ns_ link $tmp $downstreamtmp]
			    if {$rpflink != ""} {
				set oiflist [$tmp link2oif $rpflink]
			    } 
			}
		}

		if { [set r [$tmp getReps [$src id] $group]] != "" } {
			if [$r is-active] {
				# puts "reach merging point, $group [$src id] [$target id] [$tmp id] [$nodeh id], iif $iif, oif $oiflist"
				if { $oiflist != "" } {
					$r insert [lindex $oiflist 0]
				}
				break
			} else {
				# puts "hasn't reached merging point, $group [$src id] [$target id] [$tmp id] [$nodeh id], iif $iif, oif $oiflist"
				# so continue to insert the oif
				if { $oiflist != "" } {
					$r insert [lindex $oiflist 0]
				}
			}
		} else {
			# hasn't reached merging point,
			# puts "so keep creating (S,G) like a graft, $group [$src id] [$target id] [$tmp id] [$nodeh id], iif $iif, oif $oiflist"
			$tmp add-mfc [$src id] $group $iif $oiflist
		}
	}
}


CtrMcastComp instproc prune-branch { src group nodeh } {
	$self instvar ns_

	### set (S,G) prune target
	set tt [$self treetype? $group]
	if { $tt == "SPT" } {
		set target $src
	} elseif { $tt == "RPT" } {
		set target [$self get_rp $nodeh $group]
	}

	for {
		set downstreamtmp ""
		set tmp $nodeh
	} { $downstreamtmp != $target } {
		set downstreamtmp $tmp
		set tmp [$tmp rpf-nbr $target]
	} {
		set iif -1
		set oif ""
		if { $downstreamtmp != "" } {
			set rpfnbr [$downstreamtmp rpf-nbr $target]
			if { $rpfnbr == $tmp } {
				set oif [$tmp link2oif [$ns_ link $tmp $downstreamtmp]]
			}
		}

		if { [set r [$tmp getReps [$src id] $group]] != "" } {
			if { $oif != "" } {
				$r disable $oif
			}

			if [$r is-active] {
				break
			}
		} else {
			break
		}
	}
    
}

# notify(): adapt to rtglib dynamics
CtrMcastComp instproc notify {} {
	$self instvar ctrrpcomp

	### need to add a delay before recomputation
	$ctrrpcomp compute-rpset
	$self compute-mroutes
}

#
# utility functions to track Source, Group, and Member State 
#
CtrMcastComp instproc init-groups {} {
	$self set Glist_ ""
}

CtrMcastComp instproc add-new-group group {
    $self instvar Glist_ 
    set group [expr $group]

    if ![info exist Glist_] {
        set Glist_ ""
    }
    if {[lsearch $Glist_ $group] < 0} {
        lappend Glist_ $group
    }
}

CtrMcastComp instproc add-new-member {group node} {
    $self instvar Mlist_ 
    set group [expr $group]

    $self add-new-group $group
    if ![info exist Mlist_($group)] {
        set Mlist_($group) ""
    }

    if {[lsearch $Mlist_($group) $node] < 0} {
        lappend Mlist_($group) $node
    }
}

CtrMcastComp instproc new-source? {group node} {
    $self instvar Slist_ 
    set group [expr $group]

    $self add-new-group $group
    if ![info exist Slist_($group)] {
        set Slist_($group) ""
    }

    if {[lsearch $Slist_($group) $node] < 0} {
        lappend Slist_($group) $node
        return 1
    } else {
        return 0
    }
}

CtrMcastComp instproc groups? {} {
	$self set Glist_
}

CtrMcastComp instproc members? group {
	$self instvar Mlist_
	set group [expr $group]
	if ![info exists Mlist_($group)] {
		set Mlist_($group) ""
	}
	set Mlist_($group)
}

CtrMcastComp instproc sources? group {
	$self instvar Slist_
	set group [expr $group]
	if ![info exists Slist_($group)] {
		set Slist_($group) ""
	}
	set Slist_($group)
}

CtrMcastComp instproc remove-member {group node} {
    $self instvar Mlist_ Glist_
    set group [expr $group]

    set k [lsearch $Mlist_($group) $node]
    if {$k < 0} {
	puts "warning: removing non-member"
    } else {
	set Mlist_($group) [lreplace $Mlist_($group) $k $k]
    }

    if { $Mlist_($group) == "" } {
	set k [lsearch $Glist_ $group]
	if {$k < 0} {
	    puts "warning: removing non-existing group"
	} else {
	    set Glist_ [lreplace $Glist_ $k $k]
	}
    }
}

CtrMcastComp instproc treetype? group {
	$self instvar treetype_
	set group [expr $group]
	if [info exists treetype_($group)] {
		return $treetype_($group)
	} else {
		return ""
	}
}

CtrMcastComp instproc treetype {group tree} {
	$self set treetype_([expr $group]) $tree
}

CtrMcastComp instproc switch-treetype group {
	$self instvar treetype_ dynT_
	set group [expr $group]

	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr annotate "$group switch tree type"
		}
	}
	set treetype_($group) "SPT"
	$self add-new-group $group
	$self compute-mroutes
}

CtrMcastComp instproc set_c_rp args {
	$self instvar ns_
    
    
	foreach n [$ns_ all-nodes-list] {
		set arbiter [$n getArbiter]
		#NS creates a virtual node per LAN.  This node does
		#not have any the multicast features set.
		if {$arbiter != ""} {
			set ctrmcast [$arbiter getType "CtrMcast"]
			$ctrmcast instvar c_rp_
			$ctrmcast unset_c_rp
		}
	}

	foreach node $args {
		set arbiter [$node getArbiter]	   
		set ctrmcast [$arbiter getType "CtrMcast"]
		$ctrmcast set_c_rp
	}
}

CtrMcastComp instproc set_c_bsr args {
	foreach node $args {
		set tmp [split $node :]
		set node [lindex $tmp 0]
		set prior [lindex $tmp 1]
		set arbiter [$node getArbiter]
		set ctrmcast [$arbiter getType "CtrMcast"]
		$ctrmcast set_c_bsr $prior
	}
}

CtrMcastComp instproc get_rp { node group } {
	set ctrmcast [[$node getArbiter] getType "CtrMcast"]
	$ctrmcast get_rp $group
}

CtrMcastComp instproc get_bsr { node } {
	set arbiter [$node getArbiter]
	set ctrmcast [$arbiter getType "CtrMcast"]
	$ctrmcast get_bsr
}
