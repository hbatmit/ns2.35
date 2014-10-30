#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-route.tcl,v 1.38 2005/05/30 05:22:36 sfloyd Exp $
#

RouteLogic instproc register {proto args} {
	$self instvar rtprotos_ node_rtprotos_ default_node_rtprotos_
	if [info exists rtprotos_($proto)] {
		eval lappend rtprotos_($proto) $args
	} else {
		set rtprotos_($proto) $args
	}
	if { [Agent/rtProto/$proto info procs pre-init-all] != "" } {
		Agent/rtProto/$proto pre-init-all $args
	}
}

RouteLogic instproc configure {} {
	$self instvar rtprotos_
	if [info exists rtprotos_] {
		foreach proto [array names rtprotos_] {
			eval Agent/rtProto/$proto init-all $rtprotos_($proto)
		}
	} else {
		Agent/rtProto/Static init-all
	}
}

RouteLogic instproc lookup { nodeid destid } {
	if { $nodeid == $destid } {
		return $nodeid
	}

	set ns [Simulator instance]
	set node [$ns get-node-by-id $nodeid]

	if [Simulator hier-addr?] {
		set dest [$ns get-node-by-id $destid]
		set nh [$self hier-lookup [$node node-addr] [$dest node-addr]]
		return [$ns get-node-id-by-addr $nh]
	}
	set rtobj [$node rtObject?]
	if { $rtobj != "" } {
		$rtobj lookup [$ns get-node-by-id $destid]
	} else {
		$self cmd lookup $nodeid $destid
	} 
}

RouteLogic instproc notify {} {
	$self instvar rtprotos_
	foreach i [array names rtprotos_] {
		Agent/rtProto/$i compute-all
	}

	foreach i [CtrMcastComp info instances] {
		$i notify
	}
	if { [rtObject info instances] == ""} {
		foreach node [[Simulator instance] all-nodes-list] {
			# XXX using dummy 0 for 'changes'
			$node notify-mcast 0
		}
	}
}

#
# routine to create address for hier-route-lookup at each level
# i.e at level 2, address to lookup should be 0.1 and not just 1
#
RouteLogic instproc append-addr {level addrstr} {
	if {$level != 0} {
		set str [lindex $addrstr 0]
		for {set i 1} {$i < $level} {incr i} {
			append str . [lindex $addrstr [expr $i]]
		}
		return $str
	}
}

#
# debugging method to dump table (see route.cc for C++ methods)
#
RouteLogic instproc dump nn {
	set i 0
	while { $i < $nn } {
		set j 0
		while { $j < $nn } {
			puts "$i -> $j via [$self lookup $i $j]"
			incr j
		}
		incr i
	}
}

Simulator instproc rtproto {proto args} {
    $self instvar routingTable_
    if {$proto == "Algorithmic"} {
	set routingTable_ [new RouteLogic/Algorithmic]
    }
    eval [$self get-routelogic] register $proto $args
}

Simulator instproc get-routelogic {} {
	$self instvar routingTable_
	if ![info exists routingTable_] {
		set routingTable_ [new RouteLogic]
	}
	return $routingTable_
}

# Debo
Simulator instproc dump-approx-sim-data {} {

	$self instvar routingTable_ Node_ link_
    
#        if ![info exists routingTable_] {
#	    puts "error: routing table is not computed yet!"
#	    return 0
#	}

	set r [$self get-routelogic]	
	$self cmd get-routelogic $r  ;# propagate rl in C++
	# populate the route logic 
	foreach ln [array names link_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
		if { [$link_($ln) up?] == "up" } {
			$r insert $srcID $dstID [$link_($ln) cost?]
		} else {
			$r reset $srcID $dstID
		}
	}
	# now compute routes
	$r compute

	puts "# Dumping Approx-Sim Data"  

	set n [Node set nn_]
	puts "m [Link set nl_] "
        foreach qn [array names link_] {
                set l $link_($qn)
	        set q [$l queue]
	        set t [$q info class]
	    if {[lindex [split $t "/"] 1] == "DropTail"} {
                puts "link [expr [$l set id_] + 1] [expr [$l bw] / 8000] [expr [$l bw] / 8000] [$l delay] [$l qsize] $t"
	    }
	    if {[lindex [split $t "/"] 1] == "RED"} {
                puts "link [expr [$l set id_] + 1] [expr [$l bw] / 8000] [expr [$l bw] / 8000] [$l delay] [$l qsize] RED [$q set thresh_] 0 [$q set maxthresh_] [expr 1.0 / [$q set linterm_] ]"
	    }
        }

	puts ""
	puts "n $nconn_"
	
	for { set i 0 } { $i < $nconn_ } { incr i } {

		set len 0
		set str ""

		set list [split $conn_($i) ":"] 
		set srcid [lindex $list 0]
		set dstid [lindex $list 1]

		while { $srcid != $dstid } {
			incr len
			# shortened nexthop to nh, to fit add-route in
			# a single line
			set nh [$r lookup $srcid $dstid]
			# print the route 
			append str " " [expr [$link_($srcid:$nh) id] + 1] 
			set srcid  $nh
		}
		
		puts "route [expr $i + 1] $len $str"
		
	}
}

Simulator instproc dump-routelogic-nh {} {
	$self instvar routingTable_ Node_ link_
	if ![info exists routingTable_] {
	    puts "error: routing table is not computed yet!"
	    return 0
	}

	puts "Dumping Routing Table: Next Hop Information"
	set n [Node set nn_]
	set i 0
	puts -nonewline "\t"
	while { $i < $n } {
	    if ![info exists Node_($i)] {
		incr i
		continue
	    }
	    puts -nonewline "$i\t"
	    incr i
	}
	set i 0
	while { $i < $n } {
		if ![info exists Node_($i)] {
		    incr i
		    continue
		}
		puts -nonewline "\n$i\t"
		set n1 $Node_($i)
		set j 0
		while { $j < $n } {
			if { $i != $j } {
				# shortened nexthop to nh, to fit add-route in
				# a single line
				set nh [$routingTable_ lookup $i $j]
				if { $nh >= 0 } {
				    puts -nonewline "$nh\t"
				}
			} else {
				puts -nonewline "--\t"
			}
			incr j
		}
		incr i
	}
	puts ""
}

Simulator instproc dump-routelogic-distance {} {
	$self instvar routingTable_ Node_ link_
	if ![info exists routingTable_] {
	    puts "error: routing table is not computed yet!"
	    return 0
	}

	# puts "Dumping Routing Table: Distance Information"
	set n [Node set nn_]
	set i 0
	puts -nonewline "\t"
	while { $i < $n } {
	    if ![info exists Node_($i)] {
		incr i
		continue
	    }
	    puts -nonewline "$i\t"
	    incr i
	}

	for {set i 0} {$i < $n} {incr i} {
		if ![info exists Node_($i)] {
		    continue
		}
		puts -nonewline "\n$i\t"
		set n1 $Node_($i)
		for {set j 0} {$j < $n} {incr j} {
			if { $i == $j } {
				puts -nonewline "0\t"
				continue
			}
			set nh [$routingTable_ lookup $i $j]
			if { $nh < 0 } {
				puts -nonewline "0\t"
				continue
			}
			set distance 0
			set tmpfrom $i
			set tmpto $j
			while {$tmpfrom != $tmpto} {
				set tmpnext [$routingTable_ lookup \
					$tmpfrom $tmpto]
				set distance [expr $distance + \
					[$link_($tmpfrom:$tmpnext) cost?]]
				set tmpfrom $tmpnext
			}
			puts -nonewline "$distance\t"
		}
	}
	puts ""
}

# Only used by static routing protocols, namely: static and session.
Simulator instproc compute-routes {} {
	#
	# call hierarchical routing, if applicable
	#
	if [Simulator hier-addr?] {
		$self compute-hier-routes 
	} else {
		$self compute-flat-routes
	}
}

Simulator instproc compute-flat-routes {} {
	$self instvar Node_ link_
	#
	# Compute all the routes using the route-logic helper object.
	#
        if { [ Simulator set nix-routing] } {
            puts "Using NixVector routing, skipping route computations"
            return
        }
	#puts "Starting to read link_ array..\
	#	time: [clock format [clock seconds] -format %X]"
	
	set r [$self get-routelogic]
	$self cmd get-routelogic $r  ;# propagate rl in C++
	
	foreach ln [array names link_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
		if { [$link_($ln) up?] == "up" } {
			$r insert $srcID $dstID [$link_($ln) cost?]
		} else {
			$r reset $srcID $dstID
		}
	}
	#puts "Completed reading link_ array.."
	#puts " and starting route-compute at \
	#	time: [clock format [clock seconds] -format %X]"

	$r compute

	#puts "completed route-compute"
	#puts "and starting to populate classifiers at \
	#	time: [clock format [clock seconds] -format %X]"
	set n [Node set nn_]
	
	# classifier-population part moved to C++: this results in > 50% 
        # improvement of simulation run time.
	
	$self populate-flat-classifiers $n
	

	# Set up each classifer (aka node) to act as a router.
	# Point each classifer table to the link object that
	# in turns points to the right node.
	#
	#set i 0
	#set n [Node set nn_]
	#while { $i < $n } {
	#	if ![info exists Node_($i)] {
	#	    incr i
	#	    continue
	#	}
	#	set n1 $Node_($i)
	#	set j 0
	#	while { $j < $n } {
	#	    if { $i != $j } {
			# shortened nexthop to nh, to fit add-route in
			# a single line
	#		set nh [$r lookup $i $j]
	#		if { $nh >= 0 } {
	#		    $n1 sp-add-route $j [$link_($i:$nh) head]
	#		}
	#	    } 
	#	    incr j
	#	}
	#	incr i
	#}

	#puts "Completed populating classifiers at \
	#	time: [clock format [clock seconds] -format %X]"
}

Simulator instproc get-link-head { n1 n2 } {
    $self instvar link_
    return [$link_($n1:$n2) head]
}



#
# Hierarchical routing support
#
Simulator instproc hier-topo {rl} {
	#
	# if topo info not found, use default values
	#
	AddrParams instvar domain_num_ cluster_num_ nodes_num_ 
    
	if ![info exists cluster_num_] {
		if {[AddrParams hlevel] > 1} {
			### set default value of clusters/domain 
			set def [AddrParams set def_clusters]
			puts "Default value for cluster_num set to $def\n"
			for {set i 0} {$i < $domain_num_} {incr i} {
				lappend clusters $def
			}
		} else {
			### how did we reach here instead of flat routing?
			puts stderr "hierarchy level = 1; should use flat-rtg instead of hier-rtg" 
			exit 1
		}
		AddrParams set cluster_num_ $clusters
	}

	### set default value of nodes/cluster
	if ![info exists nodes_num_ ] {
		set total_node 0
		set def [AddrParams set def_nodes]
		puts "Default value for nodes_num set to $def\n"
		for {set i 0} {$i < $domain_num_} {incr i} {
			set total_node [expr $total_node + \
					[lindex $clusters $i]]
		}
		for {set i 0} {$i < $total_node} {incr i} {
			lappend nodes $def
		}
		AddrParams set nodes_num_ $nodes
	}
	# Eval is necessary because these *_num_ are usually lists; eval 
	# will break them into individual arguments
	eval $rl send-num-of-domains $domain_num_
	eval $rl send-num-of-clusters $cluster_num_
	eval $rl send-num-of-nodes $nodes_num_
}

Simulator instproc compute-hier-routes {} {
	$self instvar Node_ link_
	set r [$self get-routelogic]
        $self cmd get-routelogic $r ;# propagate rl in C++    

	# send hierarchical data :
	# array of cluster size, #clusters, #domains
	# assuming 3 levels of hierarchy --> this should be extended to support
	# n-levels of hierarchy
	#
        if ![info exists link_] {
		return
	}
        set level [AddrParams hlevel]
        $r hlevel-is $level
	$self hier-topo $r
	foreach ln [array names link_] {
		set L [split $ln :]
	        set srcID [[$self get-node-by-id [lindex $L 0]] node-addr]
		set dstID [[$self get-node-by-id [lindex $L 1]] node-addr]
		if { [$link_($ln) up?] == "up" } {
			$r hier-insert $srcID $dstID [$link_($ln) cost?]
		} else {
			$r hier-reset $srcID $dstID
		}
	}

	$r hier-compute

	# Set up each classifier (n for n levels of hierarchy) 
	# for every node

	
	# classifier-population part moved to C++: this results in > 50% 
        # improvement of simulation run time. -Padma, 02/01.
	set n [Node set nn_]
	$self populate-hier-classifiers $n

	#$self gen-map

#	for {set i 0} {$i < $n} {incr i} {
#		if ![info exists Node_($i)] {
#			continue
#		}
#		set n1 $Node_($i)
#		set addr [$n1 node-addr]
#		set L [AddrParams split-addrstr $addr]
		#
		# for each hierarchy level, populate classifier for that level
		#
#		for {set k 0} {$k < $level} {incr k} {
#			set csize [AddrParams elements-in-level? $addr $k]
			#
			# for each element in that level (elements maybe 
			# nodes or clusters or domains 
			#
#			if {$k > 0} {
#				set prefix [$r append-addr $k $L]
#			}
#			for {set m 0} {$m < $csize} {incr m} {
#				if { $m == [lindex $L $k]} {
#					continue
#				}
#				if {$k > 0} {
#					set str $prefix
#					append str . $m
#				} else {
#					set str $m
#				}
#				set nh [$r hier-lookup $addr $str]
				# add entry in clasifier only if hier-lookup 
				# returns a value. 
#				if {$nh == -1} { 
#					continue
#				}
#				set node [$self get-node-id-by-addr $nh]
#				if { $node >= 0 } {
#					$n1 sp-add-route $str \
#							[$link_($i:$node) head]
#				}
#			}
#		}
#	}
#}

}

#
# Now source the dynamic routing protocol methods
#
source ../rtglib/route-proto.tcl
source ../rtglib/dynamics.tcl
source ../rtglib/algo-route-proto.tcl

Simulator instproc compute-algo-routes {} {
	$self instvar Node_ link_
	#
	# Compute all the routes using the route-logic helper object.
	#
	set r [$self get-routelogic]

	$r BFS
	$r compute

	#
	# Set up each classifer (aka node) to act as a router.
	# Point each classifer table to the link object that
	# in turns points to the right node.
	#
	set i 0
	set n [Node set nn_]
	while { $i < $n } {
		if ![info exists Node_($i)] {
		    incr i
		    continue
		}
		set n1 $Node_($i)
		$n1 set rtsize_ 1 
		set j 0
		while { $j < $n } {
		    if { $i != $j } {
			# shortened nexthop to nh, to fit add-route in
			# a single line
			set nh [$r lookup $i $j]
			# puts "$i $j $nh"
			if { $nh >= 0 } {
			    $n1 add-route $j [$link_($i:$nh) head]
			    ###$n1 incr-rtgtable-size
			}
		    } 
		    incr j
		}
		incr i
	}
}





# Asim routines 
# Debojyoti Dutta

Simulator instproc asim-run { } {

    $self instvar asim_
    set asim_ [new Asim]
    $self asim-dump ddt
    $asim_ readinput ddt
    $asim_ run

}

Simulator instproc asim-dump { file } {

	$self instvar routingTable_ Node_ link_
        $self instvar nconn_ conn_ sflows_ nsflows_
        
        set tf_ [open "$file" w]
    
	set r [$self get-routelogic]	
	$self cmd get-routelogic $r  ;# propagate rl in C++
	# populate the route logic 
	foreach ln [array names link_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
		if { [$link_($ln) up?] == "up" } {
			$r insert $srcID $dstID [$link_($ln) cost?]
		} else {
			$r reset $srcID $dstID
		}
	}
	# now compute routes
	$r compute

	puts $tf_ "# Dumping Approx-Sim Data"  

	set n [Node set nn_]
	puts $tf_ "m [Link set nl_] "
        foreach qn [array names link_] {
                set l $link_($qn)
	        set q [$l queue]
	        set t [$q info class]
	    if {[lindex [split $t "/"] 1] == "DropTail"} {
                puts $tf_ "link [expr [$l set id_] + 1] [$l delay] [expr [$l bw] / 8000] [expr [$l bw] / 8000]  [$l qsize] $t"
	    }
	    if {[lindex [split $t "/"] 1] == "RED"} {
                puts $tf_ "link [expr [$l set id_] + 1] [$l delay] [expr [$l bw] / 8000] [expr [$l bw] / 8000] [$l qsize] red [$q set thresh_] 0 [$q set maxthresh_] [expr 1.0 / [$q set linterm_] ]"
	    }
        }

	puts $tf_ ""
	puts $tf_ "n [expr $nconn_ + $nsflows_]"
	
	set i 0
	foreach x $conn_ {

		set len 0
		set str ""

		set list [split $x ":"] 
		set srcid [lindex $list 0]
		set dstid [lindex $list 1]

		while { $srcid != $dstid } {
			incr len
			# shortened nexthop to nh, to fit add-route in
			# a single line
			set nh [$r lookup $srcid $dstid]
			# print the route 
			append str " " [expr [$link_($srcid:$nh) id] + 1] 
			set srcid  $nh
		}
		
		puts $tf_ "route [expr $i + 1] $len $str"
		incr i

	}

	foreach x $sflows_ {

		set len 0
		set str ""

		set list [split $x ":"] 
		set srcid [lindex $list 0]
		set dstid [lindex $list 1]
	        set lambda [lindex $list 2]
	        set mu [lindex $list 3]

		while { $srcid != $dstid } {
			incr len
			# shortened nexthop to nh, to fit add-route in
			# a single line
			set nh [$r lookup $srcid $dstid]
			# print the route 
			append str " " [expr [$link_($srcid:$nh) id] + 1] 
			set srcid  $nh
		}
		
		puts $tf_ "route [expr $i + 1] $len $str sh $lambda $mu"
		incr i

	}

	close $tf_
}

Simulator instproc asim-getLinkDelay { link } {

    $self instvar asim_
    set t [$asim_ get-link-delay [$link set id_] ]
    return $t

}

Simulator instproc asim-getLinkDrop { link } {

    $self instvar asim_
    set t [$asim_ get-link-drop [$link set id_] ]
    return $t

}

Simulator instproc asim-getLinkTput { link } {

    $self instvar asim_
    set t [$asim_ get-link-tput [$link set id_] ]
    return $t

}

Simulator instproc asim-getAggrTput { srcnode dstnode } {

    $self instvar conn_
    $self instvar asim_ 
    set src [$srcnode id]
    set dst [$dstnode id]
#    puts "searching $src:$dst in $conn_"
    set tt [lsearch -exact $conn_ $src:$dst]
    set t [$asim_ get-flow-tput $tt ]
    return $t

}

Simulator instproc asim-getFlowTput { agent } {

    $self instvar conn_
    $self instvar asim_

    set srcnode [$self get-node-by-addr [$agent set agent_addr_]]
    set dstnode [$self get-node-by-addr [$agent set dst_addr_]]

    set src [$srcnode id]
    set dst [$dstnode id]
    set sport [$agent set agent_port_]
    set dport [$agent set dst_port_]
#    puts "searching $src:$dst in $conn_"
    set tt [lsearch -exact $conn_ $src:$dst:$sport:$dport]
    set t [$asim_ get-flow-tput $tt ]
    return $t

}


Simulator instproc asim-getFlowDrop { agent } {

    $self instvar conn_
    $self instvar asim_

    set srcnode [$self get-node-by-addr [$agent set agent_addr_]]
    set dstnode [$self get-node-by-addr [$agent set dst_addr_]]

    set src [$srcnode id]
    set dst [$dstnode id]
    set sport [$agent set agent_port_]
    set dport [$agent set dst_port_]
#    puts "searching $src:$dst in $conn_"
    set tt [lsearch -exact $conn_ $src:$dst:$sport:$dport]
    set t [$asim_ get-flow-drop $tt ]
    return $t

}


