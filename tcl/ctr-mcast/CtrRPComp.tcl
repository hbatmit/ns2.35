#
# tcl/ctr-mcast/CtrRPComp.tcl
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

Class CtrRPComp
CtrRPComp instproc init sim {
	$self set ns_ $sim
	$self next
}
    
CtrRPComp instproc compute-rpset {} {
	$self instvar ns_

	### initialization
	foreach node [$ns_ all-nodes-list] {
		set connected($node) 0
	}
	set urtl [$ns_ get-routelogic]
	
	### connected region algorithm
	foreach node [$ns_ all-nodes-list] {
		foreach {vertix lvertix} [array get ldomain] {
			if {[$urtl lookup [$node id] [$vertix id]] >= 0} {
				lappend ldomain($vertix) $node
				set connected($node) 1
				break
			}
		}
		
		if {!$connected($node)} {
			set ldomain($node) $node
			set connected($node) 1
		}
	}

	### for each region, set rpset
	foreach {vnode lvertix} [array get ldomain] {
		set hasbsr 0
		set rpset ""

		### find rpset for the region
		foreach vertix $lvertix {
			set class_info [$vertix info class]
			if {$class_info != "LanNode"} {
				set ctrdm [[$vertix getArbiter] getType "CtrMcast"]
				if [$ctrdm set c_bsr_] {set hasbsr 1}
				if [$ctrdm set c_rp_] {
					lappend rpset $vertix
				}
			}
		}

		foreach vertix $lvertix {
			set class_info [$vertix info class]
			if {$class_info != "LanNode"} {
				set ctrdm [[$vertix getArbiter] getType "CtrMcast"]
				if $hasbsr {
					$ctrdm set-rpset $rpset
				} else {
					$ctrdm set-rpset ""
					puts "no c_bsr"
				}
			}
		}
	}
}
