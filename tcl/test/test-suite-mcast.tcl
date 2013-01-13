# This test suite is for validating the multicast support in ns.
#
# To run all tests:  test-mcast
#
# To run individual tests:
# ns test-suite-mcast.tcl DM1
# ns test-suite-mcast.tcl DM2
# ...
#
# To view a list of available tests to run with this script:
# ns test-suite-mcast.tcl
#

#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	set ns_ [new Simulator -multicast on]
	#$ns_ use-scheduler List
	
	$ns_ trace-all [open temp.rands w]
	$ns_ namtrace-all [open temp.rands.nam w]
	$ns_ color 0 blue
	$ns_ color 1 red
	$ns_ color 2 yellow
	$ns_ color 30 purple
	$ns_ color 31 green
	
	if {$net_ == ""} {
		set net_ $defNet_
	}
	if ![Topology/$defNet_ info subclass Topology/$net_] {
		global argv0
		puts "$argv0: cannot run test $test_ over topology $net_"
		exit 1
	}
	
	set topo_ [new Topology/$net_ $ns_]
	foreach i [$topo_ array names node_] {
		# This would be cool, but lets try to be compatible
		# with test-suite.tcl as far as possible.
		#
		# $self instvar $i
		# set $i [$topo_ node? $i]
		#
		set node_($i) [$topo_ node? $i]
	}
	
	if {$net_ == $defNet_} {
		set testName_ "$test_"
	} else {
		set testName_ "$test_:$net_"
	}
}

TestSuite instproc finish { file } {
	$self instvar ns_ 
	global quiet
	
	$ns_ flush-trace
	if { !$quiet } {
		puts "running nam..."
		exec nam temp.rands.nam &
	}
	exit 0
}

TestSuite instproc openTrace { stopTime testName } {
	$self instvar ns_
	exec rm -f temp.rands
	set traceFile [open temp.rands w]
	puts $traceFile "v testName $testName"
	$ns_ at $stopTime \
		"close $traceFile ; $self finish $testName"
	return $traceFile
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> \[<topologies>\]"
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	puts stderr "Valid Topologies are:\t[get-subclasses SkelTopology Topology/]"
	exit 1
}

proc isProc? {cls prc} {
	if [catch "Object info subclass $cls/$prc" r] {
		global argv0
		puts stderr "$argv0: no such $cls: $prc"
		usage
	}
}

proc get-subclasses {cls pfx} {
	set ret ""
	set l [string length $pfx]
	
	set c $cls
	while {[llength $c] > 0} {
		set t [lindex $c 0]
		set c [lrange $c 1 end]
		if [string match ${pfx}* $t] {
			lappend ret [string range $t $l end]
		}
		eval lappend c [$t info subclass]
	}
	set ret
}

TestSuite proc runTest {} {
	global argc argv quiet

	set quiet 0
	switch $argc {
		1 {
			set test $argv
			isProc? Test $test

			set topo ""
		}
		2 {
			set test [lindex $argv 0]
			isProc? Test $test
			set a [lindex $argv 1]
			if {$a == "QUIET"} {
				set topo ""
				set quiet 1
			} else {
				set topo $a
				isProc? Topology $topo
			}
		}
		default {
			usage
		}
	}
	set t [new Test/$test $topo]
	$t run
}

# Skeleton topology base class
Class SkelTopology

SkelTopology instproc init {} {
	$self next
}

SkelTopology instproc node? n {
	$self instvar node_
	if [info exists node_($n)] {
		set ret $node_($n)
	} else {
		set ret ""
	}
	set ret
}

SkelTopology instproc add-fallback-links {ns nodelist bw delay qtype args} {
	$self instvar node_
	set n1 [lindex $nodelist 0]
	foreach n2 [lrange $nodelist 1 end] {
		if ![info exists node_($n2)] {
			set node_($n2) [$ns node]
		}
		$ns duplex-link $node_($n1) $node_($n2) $bw $delay $qtype
		foreach opt $args {
			set cmd [lindex $opt 0]
			set val [lindex $opt 1]
			if {[llength $opt] > 2} {
				set x1 [lindex $opt 2]
				set x2 [lindex $opt 3]
			} else {
				set x1 $n1
				set x2 $n2
			}
			$ns $cmd $node_($x1) $node_($x2) $val
			$ns $cmd $node_($x2) $node_($x1) $val
		}
		set n1 $n2
	}
}


Class NodeTopology/4nodes -superclass SkelTopology


NodeTopology/4nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
}


Class Topology/net4a -superclass NodeTopology/4nodes

# Create a simple four node topology:
#
#	              n3
#	             / 
#       1.5Mb,10ms  / 1.5Mb,10ms                              
#    n0 --------- n1
#                  \  1.5Mb,10ms
#	            \ 
#	             n2
#

Topology/net4a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}

Class Topology/net4b -superclass NodeTopology/4nodes

# 4 nodes on the same LAN
#
#           n0   n1
#           |    |
#       -------------
#           |    |
#           n2   n3
#
#
 
Topology/net4b instproc init ns {
	$self next $ns
	$self instvar node_
	$ns multi-link-of-interfaces [list $node_(n0) $node_(n1) $node_(n2) $node_(n3)] 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}

Class NodeTopology/5nodes -superclass SkelTopology

NodeTopology/5nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
	set node_(n4) [$ns node]
}


Class Topology/net5a -superclass NodeTopology/5nodes

#
# Create a simple five node topology:
#
#                  n4
#                 /  \                    
#               n3    n2
#               |     |
#               n0    n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class Topology/net5b -superclass NodeTopology/5nodes

#
# Create a five node topology:
#
#                  n4
#                 /  \                    
#               n3----n2
#               |     |
#               n0    n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5b instproc init ns {
    $self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
    }
}

Class Topology/net5c -superclass NodeTopology/5nodes

#
# Create a five node topology:
#
#                  n4
#                 /  \                    
#               n3----n2
#               | \  /|
#               |  \  |
#               | / \ |
#               n0    n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5c instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class Topology/net5d -superclass NodeTopology/5nodes

#
# Create a five node topology:
#
#                  n4
#                 /  \                    
#               n3----n2
#               | \  /|
#               |  \  |
#               | / \ |
#               n0----n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5d instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class Topology/net5e -superclass NodeTopology/5nodes

#
# Create a five node topology with 4 nodes on a LAN:
#
#                  n4
#                 /  \                    
#               n3    n2
#               |     |
#             -----------
#               |     |
#               n0    n1
#

Topology/net5e instproc init ns {
	$self next $ns
	$self instvar node_
	$ns newLan [list $node_(n0) $node_(n3) $node_(n1) $node_(n2)] 1.5Mb 10ms

	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 3ms DropTail
	$ns duplex-link-op $node_(n3) $node_(n4) orient right-down
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 3ms DropTail
	$ns duplex-link-op $node_(n2) $node_(n4) orient left-down
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class NodeTopology/6nodes -superclass SkelTopology

NodeTopology/6nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
	set node_(n4) [$ns node]
	set node_(n5) [$ns node]
}


Class Topology/net6a -superclass NodeTopology/6nodes

#
# Create a simple six node topology:
#
#                  n0
#                 /  \                    
#               n1    n2
#              /  \  /  \
#             n3   n4   n5
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net6a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n5) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}

Class Topology/net6b -superclass NodeTopology/6nodes

# 6 node topology with nodes n2, n3 and n5 on a LAN.
#
#          n4
#          |
#          n3
#          |
#    --------------
#      |       |
#      n5      n2
#      |       |
#      n0      n1
#
# All point-to-point links have 1.5Mbps Bandwidth, 10ms latency.
#

Topology/net6b instproc init ns {
	$self next $ns
	$self instvar node_
	$ns multi-link-of-interfaces [list $node_(n5) $node_(n2) $node_(n3)] 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n4) $node_(n3) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n5) $node_(n0) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class NodeTopology/8nodes -superclass SkelTopology

NodeTopology/8nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
	set node_(n4) [$ns node]
	set node_(n5) [$ns node]
	set node_(n6) [$ns node]
	set node_(n7) [$ns node]
}

Class Topology/net8a -superclass NodeTopology/8nodes

# 8 node topology with nodes n2, n3, n4 and n5 on a LAN.
#
#      n0----n1     
#      |     |
#      n2    n3
#      |     |
#    --------------
#      |     |
#      n4    n5
#      |     |
#      n6    n7
#
# All point-to-point links have 1.5Mbps Bandwidth, 10ms latency.
#

Topology/net8a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns multi-link-of-interfaces [list $node_(n2) $node_(n3) $node_(n4) $node_(n5)] 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n4) $node_(n6) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n5) $node_(n7) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


# Definition of test-suite tests

# Testing group join/leave in a simple topology
Class Test/DM1 -superclass TestSuite
Test/DM1 instproc init topo {
	source ../mcast/DM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4a
	set test_	DM1
	$self next
}
Test/DM1 instproc run {} {
	$self instvar ns_ node_ testName_

	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto {}]
	
	set grp0 [Node allocaddr]
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n1) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set grp1 [Node allocaddr]
	set udp1 [new Agent/UDP]
	$ns_ attach-agent $node_(n3) $udp1
	$udp1 set dst_addr_ $grp1
	$udp1 set dst_port_ 0
	$udp1 set class_ 1
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n2) $rcvr
	$ns_ at 1.2 "$node_(n2) join-group $rcvr $grp1"
	$ns_ at 1.25 "$node_(n2) leave-group $rcvr $grp1"
	$ns_ at 1.3 "$node_(n2) join-group $rcvr $grp1"
	$ns_ at 1.35 "$node_(n2) join-group $rcvr $grp0"
	
	$ns_ at 1.0 "$cbr0 start"
	$ns_ at 1.1 "$cbr1 start"
	
	$ns_ at 1.8 "$self finish 4a-nam"
	$ns_ run
}

# Testing group join/leave in a richer topology. Testing rcvr join before
# the source starts sending pkts to the group.
Class Test/DM2 -superclass TestSuite
Test/DM2 instproc init topo {
	source ../mcast/DM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	DM2
	$self next
}
Test/DM2 instproc run {} {
	$self instvar ns_ node_ testName_
	
	### Start multicast configuration
	DM set PruneTimeout 0.3
	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	set grp0 [Node allocaddr]
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n0) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.2 "$node_(n3) join-group $rcvr $grp0"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr $grp0"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr $grp0"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr $grp0"
	$ns_ at 0.95 "$node_(n3) join-group $rcvr $grp0"
	
	$ns_ at 0.3 "$cbr0 start"
	$ns_ at 1.0 "$self finish 6a-nam"
	
	$ns_ run
}

#Same as DM2 but with dvmrp-like cache miss rules
Class Test/DM3 -superclass TestSuite
Test/DM3 instproc init topo {
	source ../mcast/DM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	DM3
	$self next
}
Test/DM3 instproc run {} {
	$self instvar ns_ node_ testName_
	
	### Start multicast configuration
	DM set PruneTimeout  0.3
	DM set CacheMissMode dvmrp
	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	set grp0 [Node allocaddr]
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n0) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.2 "$node_(n3) join-group $rcvr $grp0"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr $grp0"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr $grp0"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr $grp0"
	$ns_ at 0.95 "$node_(n3) join-group $rcvr $grp0"
	
	$ns_ at 0.3 "$cbr0 start"
	$ns_ at 1.0 "$self finish 6a-nam"
	
	$ns_ run
}

# Testing dynamics of links going up/down.
Class Test/DM4 -superclass TestSuite
Test/DM4 instproc init topo {
	source ../mcast/DM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	DM4
	$self next
}
Test/DM4 instproc run {} {
	$self instvar ns_ node_ testName_
	$ns_ rtproto Session
	### Start multicast configuration
	DM set PruneTimeout 0.3
	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	set grp0 [Node allocaddr]
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n0) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.2 "$node_(n3) join-group $rcvr $grp0"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr $grp0"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr $grp0"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr $grp0"
	$ns_ at 0.8 "$node_(n3) join-group $rcvr $grp0"
	#### Link between n0 & n1 down at 1.0, up at 1.2
	$ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
	$ns_ rtmodel-at 1.2 up   $node_(n0) $node_(n1)
	####
	
	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 1.6 "$self finish 6a-nam"
	
	$ns_ run
}
# testing lan topologies
#Class Test/DM5 -superclass TestSuite
#Test/DM5 instproc init topo {
	#source ../mcast/DM.tcl

	#$self instvar net_ defNet_ test_
	#set net_	$topo
	#set defNet_	net5e
	#set test_	DM5
	#$self next
#}
#Test/DM5 instproc run {} {
	#$self instvar ns_ node_ testName_
	#$ns_ rtproto Session
	#### Start multicast configuration
	#DM set PruneTimeout 0.3
	#DM set CacheMissMode dvmrp
	#set mproto DM
	#set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	#set udp0 [new Agent/UDP]
	#$ns_ attach-agent $node_(n4) $udp0
	#$udp0 set dst_addr_ 0x8002
	#$udp0 set dst_port_ 0
	#set cbr0 [new Application/Traffic/CBR]
	#$cbr0 attach-agent $udp0
	
	#set rcvr [new Agent/LossMonitor]
	#$ns_ attach-agent $node_(n0) $rcvr
	#$ns_ attach-agent $node_(n1) $rcvr
	#$ns_ attach-agent $node_(n2) $rcvr
	
	#$ns_ at 0.2 "$node_(n0) join-group  $rcvr 0x8002"
	#$ns_ at 0.3 "$node_(n1) join-group  $rcvr 0x8002"
	#$ns_ at 0.4 "$node_(n1) leave-group $rcvr 0x8002"
	#$ns_ at 0.5 "$node_(n2) join-group  $rcvr 0x8002"
	#$ns_ at 0.6 "$node_(n2) leave-group $rcvr 0x8002"
	#$ns_ at 0.7 "$node_(n0) leave-group $rcvr 0x8002"

	####
	
	#$ns_ at 0.1 "$cbr0 start"
#	#$ns_ at 0.11 "$node_(n4) dump-routes stdout"
#	#$ns_ at 0.25 "$node_(n0) dump-routes stdout"
	#$ns_ at 1.0 "$self finish 5e-nam"
	
	#$ns_ run
#}

# Testing group join/leave in a simple topology, changing the RP set. 
# The RP node also has a source.
Class Test/CtrMcast1 -superclass TestSuite
Test/CtrMcast1 instproc init topo {
	source ../ctr-mcast/CtrMcast.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4a
	set test_	CtrMcast1
	$self next
}
# source and RP on same node
Test/CtrMcast1 instproc run {} {
	$self instvar ns_ node_ testName_

	set mproto CtrMcast
	set mrthandle [$ns_ mrtproto $mproto  {}]
	$mrthandle set_c_rp $node_(n2)
	
	set udp1 [new Agent/UDP]
	$ns_ attach-agent $node_(n2) $udp1
	
	set grp [Node allocaddr]
	
	$udp1 set dst_addr_ $grp
	$udp1 set dst_port_ 0
	$udp1 set class_ 1
	##$udp1 set dst_addr_ 0x8003
	
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1

	set udp2 [new Agent/UDP]
	$ns_ attach-agent $node_(n3) $udp2
	$udp2 set dst_addr_ $grp
	$udp2 set dst_port_ 0
	$udp2 set class_ 2
	##$udp2 set dst_addr_ 0x8003
	
	set cbr2 [new Application/Traffic/CBR]
	$cbr2 attach-agent $udp2

	set rcvr0 [new Agent/Null]
	$ns_ attach-agent $node_(n0) $rcvr0
	set rcvr1 [new Agent/Null]
	$ns_ attach-agent $node_(n1) $rcvr1
	set rcvr2  [new Agent/Null]
	$ns_ attach-agent $node_(n2) $rcvr2
	set rcvr3 [new Agent/Null]
	$ns_ attach-agent $node_(n3) $rcvr3
	
	$ns_ at 0.2 "$cbr1 start"
	$ns_ at 0.25 "$cbr2 start"
	$ns_ at 0.3 "$node_(n1) join-group  $rcvr1 $grp"
	$ns_ at 0.4 "$node_(n0) join-group  $rcvr0 $grp"

	$ns_ at 0.45 "$mrthandle switch-treetype $grp"

	$ns_ at 0.5 "$node_(n3) join-group  $rcvr3 $grp"
	$ns_ at 0.65 "$node_(n2) join-group  $rcvr2 $grp"
	
	$ns_ at 0.7 "$node_(n0) leave-group $rcvr0 $grp"
	$ns_ at 0.8 "$node_(n2) leave-group  $rcvr2 $grp"

	$ns_ at 0.9 "$node_(n3) leave-group  $rcvr3 $grp"
	$ns_ at 1.0 "$node_(n1) leave-group $rcvr1 $grp"
	$ns_ at 1.1 "$node_(n1) join-group $rcvr1 $grp"
	$ns_ at 1.2 "$self finish 4a-nam"
	
	$ns_ run
}




# Testing performance in the presence of dynamics. Also testing a rcvr joining
# a group before the src starts sending pkts to the group.
Class Test/CtrMcast2 -superclass TestSuite
Test/CtrMcast2 instproc init topo {
	  $self instvar net_ defNet_ test_
	  set net_	$topo
	  set defNet_	net6a
	  set test_	CtrMcast2
	  $self next
}
Test/CtrMcast2 instproc run {} {
	  $self instvar ns_ node_ testName_

	  $ns_ rtproto Session
	  set mproto CtrMcast
	  set mrthandle [$ns_ mrtproto $mproto  {}]
	  
	  set grp0 [Node allocaddr]
	  set udp0 [new Agent/UDP]
	  $ns_ attach-agent $node_(n0) $udp0
	  $udp0 set dst_addr_ $grp0
	  $udp0 set dst_port_ 0
	  $udp0 set class_ 1
	  set cbr0 [new Application/Traffic/CBR]
	  $cbr0 attach-agent $udp0

	  set rcvr [new Agent/Null]
	  $ns_ attach-agent $node_(n3) $rcvr
	  $ns_ attach-agent $node_(n4) $rcvr
	  $ns_ attach-agent $node_(n5) $rcvr
	  
	  $ns_ at 0.3 "$node_(n3) join-group  $rcvr $grp0"
	  $ns_ at 0.35 "$cbr0 start"
	  $ns_ at 0.4 "$node_(n4) join-group  $rcvr $grp0"
	  $ns_ at 0.5 "$node_(n5) join-group  $rcvr $grp0"

	  ### Link between n2 & n4 down at 0.6, up at 1.2
	  $ns_ rtmodel-at 0.6 down $node_(n2) $node_(n4)
	  $ns_ rtmodel-at 0.8 up $node_(n2) $node_(n4)
	  ###

	  $ns_ at 1.2 "$mrthandle switch-treetype $grp0"

	  ### Link between n0 & n1 down at 1.5, up at 2.0
	  $ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
	  $ns_ rtmodel-at 1.4 up $node_(n0) $node_(n1)
	  ###
	  $ns_ at 1.5 "$self finish 6a-nam"
	  
	  $ns_ run
}

# Testing dynamics of joining and leaving for shared tree
Class Test/ST1 -superclass TestSuite
Test/ST1 instproc init topo {
	source ../mcast/ST.tcl
	global quiet
	if { $quiet } {
		ST instproc dbg arg {}
	}

	$self instvar net_ defNet_ test_ 
	set net_	$topo
	set defNet_	net6a
	set test_	ST1
	$self next
}
Test/ST1 instproc run {} {
	$self instvar ns_ node_ testName_

	set grp3 [Node allocaddr]
	set udp3 [new Agent/UDP]
	$ns_ attach-agent $node_(n3) $udp3
	$udp3 set dst_addr_ $grp3
	$udp3 set dst_port_ 0
	set cbr3 [new Application/Traffic/CBR]
	$cbr3 attach-agent $udp3

	$cbr3 set interval_ 30ms

	set rcvr2 [new Agent/LossMonitor]
	set rcvr4 [new Agent/LossMonitor]
	set rcvr5 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n2) $rcvr2
	$ns_ attach-agent $node_(n4) $rcvr4
	$ns_ attach-agent $node_(n5) $rcvr5
	
	### Start multicast configuration
	ST set RP_($grp3) $node_(n0)
	$ns_ mrtproto ST  ""

	### End of multicast  config

	$ns_ at 0.1 "$cbr3 start"
	$ns_ at 0.2 "$node_(n2) join-group $rcvr2 $grp3"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr4 $grp3"
	$ns_ at 0.6 "$node_(n2) leave-group $rcvr2 $grp3"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr5 $grp3"
	$ns_ at 0.8 "$node_(n2) join-group $rcvr2 $grp3"
	####
	
	$ns_ at 1.6 "$self finish 6a-nam"
	
	$ns_ run
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:


