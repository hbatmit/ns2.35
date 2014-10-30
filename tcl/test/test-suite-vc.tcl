#
# To run all tests:  test-all-algo-routing
#
# To run individual tests:
# ns test-suite-algo-routing.tcl Algo1
# ns test-suite-algo-routing.tcl Algo2
# ...
#
# To view a list of available tests to run with this script:
# ns test-suite-mixmode.tcl
#
#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP RTP TCP rtProtoDV ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
# This test suite is for validating the algorithmic routing support
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1 in ns.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	set ns_ [new Simulator]
	$ns_ use-scheduler List
	Node enable-module VC
        $ns_ multicast

	$ns_ trace-all [open temp.rands w]
	$ns_ namtrace-all [open temp.rands.nam w]
	$ns_ color 1 red
	$ns_ color 0 blue
	$ns_ color 2 yellow
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

TestSuite instproc finish args {
	$self instvar ns_
	
	$ns_ flush-trace
	exit 0
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
	global argc argv

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


Class Topology/net6 -superclass NodeTopology/6nodes

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

Topology/net6 instproc init ns {
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

Class NodeTopology/3nodes -superclass SkelTopology
NodeTopology/3nodes instproc init ns {
    $self next

    $self instvar node_
    set node_(n0) [$ns node]
    set node_(n1) [$ns node]
    set node_(n2) [$ns node]
}

Class Topology/net3 -superclass NodeTopology/3nodes
#
# Create a simple six node topology:
#
#                  n0
#                 /  \                    
#               n1 -- n2
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net3 instproc init ns {
    $self next $ns
    $self instvar node_

    $ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail 
    $ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail 
    $ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail 
    if {[$class info instprocs config] != ""} {
	$self config $ns
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


Class Topology/net4 -superclass NodeTopology/4nodes

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

Topology/net4 instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


# Definition of test-suite tests
# Testing algorithmic routing in a simple topology
Class Test/VC1 -superclass TestSuite
Test/VC1 instproc init net {
	$self instvar defNet_ test_ net_
	set defNet_	net3
	set test_	VC1
	set net_	$net
	$self next
}

Test/VC1 instproc run {} {
	$self instvar ns_ node_ testName_

	set grp0 [Node allocaddr]
        $ns_ rtproto Algorithmic
        set mproto CtrMcast
        set mrthandle [$ns_ mrtproto $mproto {}]
        if {$mrthandle != ""} {
	    $mrthandle set_c_rp $node_(n2)
	}

        if {$mrthandle != ""} {
	    $ns_ at 0.3 "$mrthandle switch-treetype $grp0"
        }

	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n1) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	$udp0 set class_ 1
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr0 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n0) $rcvr0
	set rcvr1 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n1) $rcvr1
	set rcvr2 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n2) $rcvr2

	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 0.1 "$node_(n1) join-group $rcvr1 $grp0"
	$ns_ at 0.1 "$node_(n0) join-group $rcvr0 $grp0"
	$ns_ at 0.1 "$node_(n2) join-group $rcvr2 $grp0"
	
	$ns_ at 0.5 "$self finish [list $rcvr0 $rcvr1 $rcvr2]"
	$ns_ run
}

# Testing algorithmic routing with multicast in a simple topology
Class Test/VC2 -superclass TestSuite
Test/VC2 instproc init net {
	$self instvar defNet_ test_ net_
	set defNet_	net6
	set test_	VC2
	set net_	$net
	$self next
}

Test/VC2 instproc run {} {
	$self instvar ns_ node_ testName_

	set grp0 [Node allocaddr]
        $ns_ rtproto Algorithmic
        set mproto CtrMcast
        set mrthandle [$ns_ mrtproto $mproto {}]
        if {$mrthandle != ""} {
	    $mrthandle set_c_rp $node_(n2)
	}

        if {$mrthandle != ""} {
	    $ns_ at 0.3 "$mrthandle switch-treetype $grp0"
        }

	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n4) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	$udp0 set class_ 1
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr0 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n0) $rcvr0
	set rcvr1 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n1) $rcvr1
	set rcvr2 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n2) $rcvr2
	set rcvr3 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr3
	set rcvr4 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n4) $rcvr4
	set rcvr5 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n5) $rcvr5

	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 0.1 "$node_(n1) join-group $rcvr1 $grp0"
	$ns_ at 0.1 "$node_(n0) join-group $rcvr0 $grp0"
	$ns_ at 0.1 "$node_(n3) join-group $rcvr3 $grp0"
	$ns_ at 0.1 "$node_(n2) join-group $rcvr2 $grp0"
	$ns_ at 0.1 "$node_(n4) join-group $rcvr4 $grp0"
	$ns_ at 0.1 "$node_(n5) join-group $rcvr5 $grp0"
	
	$ns_ at 0.5 "$self finish [list $rcvr0 $rcvr1 $rcvr2 $rcvr3 $rcvr4 $rcvr5]"
	$ns_ run
}

Class Test/VC3 -superclass TestSuite
Test/VC3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	VC3
	$self next
}

Test/VC3 instproc run {} {
    $self instvar ns_ node_ testName_
	
    set udp0 [new Agent/UDP]
    $ns_ attach-agent $node_(n3) $udp0
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 attach-agent $udp0
    
    set udp1 [new Agent/UDP]
    $ns_ attach-agent $node_(n0) $udp1
    $udp1 set class_ 1
    set cbr1 [new Application/Traffic/CBR]
    $cbr1 attach-agent $udp1

    set null0 [new Agent/Null]
    $ns_ attach-agent $node_(n0) $null0

    set null1 [new Agent/Null]
    $ns_ attach-agent $node_(n2) $null1

    $ns_ connect $udp0 $null0
    $ns_ connect $udp1 $null1

    $ns_ at 1.0 "$cbr0 start"
    $ns_ at 1.1 "$cbr1 start"

    set tcp [new Agent/TCP]
    $tcp set class_ 2
    set sink [new Agent/TCPSink]
    $ns_ attach-agent $node_(n0) $tcp
    $ns_ attach-agent $node_(n3) $sink
    $ns_ connect $tcp $sink
    set ftp [new Application/FTP]
    $ftp attach-agent $tcp
    $ns_ at 1.2 "$ftp start"

    $ns_ at 1.35 "$ns_ detach-agent $node_(n0) $tcp ; $ns_ detach-agent $node_(n3) $sink"

    $ns_ at 1.5 "$self finish"
	
    $ns_ run
}


TestSuite runTest

