# This test suite is for validating the session level simulation support
# in ns.
#
# To run all tests:  test-all-session
#
# To run individual tests:
# ns test-suite-session.tcl Session1
# ns test-suite-session.tcl Session2
# ns test-suite-session.tcl Session3
# ...
#
# To view a list of available tests to run with this script:
# ns test-suite-session.tcl
#

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP RTP ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set SetCWRonRetransmit_ false
# The default is "true".

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	set ns_ [new SessionSim]
	#$ns_ use-scheduler List

	#$ns_ namtrace-all [open all.tr w]
	puts "tracing"
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
	puts "\t#pkt\t#pkt"
	puts "rcvr\trcvd\tlost"
	set i 0
	foreach index $args {
		puts "$i\t[$index set npkts_]\t[$index set nlost_]"
		incr i
	}
#	exec awk -f ../nam-demo/nstonam.awk all.tr > [append file \.tr]
#	puts "running nam ..."
#	exec nam $file &
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

			set topo [lindex $argv 1]
			isProc? Topology $topo
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
    Simulator set NumberInterfaces_ 1
    $ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail
    $ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail
    $ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail
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
    Simulator set NumberInterfaces_ 1
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


# Definition of test-suite tests

# Testing group join for SessionSim in a simple topology
Class Test/Session1 -superclass TestSuite
Test/Session1 instproc init net {
	$self instvar defNet_ test_ net_
	set defNet_	net4
	set test_	Session1
	set net_	$net
	$self next
}

Test/Session1 instproc run {} {
	$self instvar ns_ node_ testName_

	set grp0 [Node allocaddr]
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n2) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	$ns_ create-session $node_(n2) $udp0	
	
	set rcvr0 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n0) $rcvr0
	set rcvr1 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n1) $rcvr1
	set rcvr2 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n2) $rcvr2
	set rcvr3 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr3

	$ns_ at 0.3 "$cbr0 start"
	$ns_ at 0.3 "$node_(n1) join-group $rcvr1 $grp0"
	$ns_ at 0.3 "$node_(n0) join-group $rcvr0 $grp0"
	$ns_ at 0.3 "$node_(n3) join-group $rcvr3 $grp0"
	$ns_ at 0.3 "$node_(n2) join-group $rcvr2 $grp0"
	
	$ns_ at 1.1 "$self finish [list $rcvr0 $rcvr1 $rcvr2 $rcvr3]"
	$ns_ run
}

# Testing group join for SessionSim in a 6-node topology
Class Test/Session2 -superclass TestSuite
Test/Session2 instproc init net {
	$self instvar net_ defNet_ test_
	set defNet_	net6
	set test_	Session2
	set net_	$net
	$self next
}

Test/Session2 instproc run {} {
	$self instvar ns_ node_ testName_

	set grp0 [Node allocaddr]
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n0) $udp0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	$ns_ create-session $node_(n0) $udp0
	
	set rcvr0 [new Agent/LossMonitor]
	set rcvr1 [new Agent/LossMonitor]
	set rcvr2 [new Agent/LossMonitor]
	set rcvr3 [new Agent/LossMonitor]
	set rcvr4 [new Agent/LossMonitor]
	set rcvr5 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n0) $rcvr0
	$ns_ attach-agent $node_(n1) $rcvr1
	$ns_ attach-agent $node_(n2) $rcvr2
	$ns_ attach-agent $node_(n3) $rcvr3
	$ns_ attach-agent $node_(n4) $rcvr4
	$ns_ attach-agent $node_(n5) $rcvr5
	
	$ns_ at 0.2 "$node_(n0) join-group $rcvr0 $grp0"
	$ns_ at 0.2 "$node_(n1) join-group $rcvr1 $grp0"
	$ns_ at 0.2 "$node_(n2) join-group $rcvr2 $grp0"
	$ns_ at 0.2 "$node_(n3) join-group $rcvr3 $grp0"
	$ns_ at 0.2 "$node_(n4) join-group $rcvr4 $grp0"
	$ns_ at 0.2 "$node_(n5) join-group $rcvr5 $grp0"
	
	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 1.6 "$self finish [list $rcvr0 $rcvr1 $rcvr2 $rcvr3 \
$rcvr4 $rcvr5]"
	
	$ns_ run
}

# Testing loss dependency for SessionSim in a 6-node topology
Class Test/Session3 -superclass TestSuite
Test/Session3 instproc init net {
	$self instvar net_ defNet_ test_
	set defNet_	net6
	set test_	Session3
	set net_	$net
	$self next
}

Test/Session3 instproc run {} {
	$self instvar ns_ node_ testName_

	set grp0 [Node allocaddr]
	set udp0 [new Agent/UDP]
	$udp0 set ttl_ 3
	$ns_ attach-agent $node_(n0) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	set sessionhelper [$ns_ create-session $node_(n0) $udp0]
	
	set rcvr0 [new Agent/LossMonitor]
	set rcvr1 [new Agent/LossMonitor]
	set rcvr2 [new Agent/LossMonitor]
	set rcvr3 [new Agent/LossMonitor]
	set rcvr4 [new Agent/LossMonitor]
	set rcvr5 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n0) $rcvr0
	$ns_ attach-agent $node_(n1) $rcvr1
	$ns_ attach-agent $node_(n2) $rcvr2
	$ns_ attach-agent $node_(n3) $rcvr3
	$ns_ attach-agent $node_(n4) $rcvr4
	$ns_ attach-agent $node_(n5) $rcvr5
	
	$ns_ at 0.2 "$node_(n0) join-group $rcvr0 $grp0"
	$ns_ at 0.2 "$node_(n1) join-group $rcvr1 $grp0"
	$ns_ at 0.2 "$node_(n2) join-group $rcvr2 $grp0"
	$ns_ at 0.2 "$node_(n3) join-group $rcvr3 $grp0"
	$ns_ at 0.2 "$node_(n4) join-group $rcvr4 $grp0"
	$ns_ at 0.2 "$node_(n5) join-group $rcvr5 $grp0"
	
	set loss_module1 [new SelectErrorModel]
	$loss_module1 drop-packet 2 20 1
	$loss_module1 drop-target [$ns_ set nullAgent_]

	set loss_module2 [new SelectErrorModel]
	$loss_module2 drop-packet 2 10 1
	$loss_module2 drop-target [$ns_ set nullAgent_]

	set loss_module3 [new SelectErrorModel]
	$loss_module3 drop-packet 2 10 1
	$loss_module3 drop-target [$ns_ set nullAgent_]

	$ns_ insert-loss $loss_module1 $node_(n0) $node_(n1)
	$ns_ insert-loss $loss_module2 $node_(n1) $node_(n3)
	$ns_ insert-loss $loss_module3 $node_(n0) $node_(n2)

	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 1.6 "$self finish [list $rcvr0 $rcvr1 $rcvr2 $rcvr3 \
$rcvr4 $rcvr5]"
	
	$ns_ run
}

# Testing algorithmic routing for SessionSim in a 6-node topology
Class Test/Session4 -superclass TestSuite
Test/Session4 instproc init net {
	$self instvar net_ defNet_ test_
	set defNet_	net6
	set test_	Session4
	set net_	$net
	$self next
}

Test/Session4 instproc run {} {
	$self instvar ns_ node_ testName_

	set grp0 [Node allocaddr]
        $ns_ rtproto Algorithmic
	set udp0 [new Agent/UDP]
	$udp0 set ttl_ 4
	$ns_ attach-agent $node_(n4) $udp0
	$udp0 set dst_addr_ $grp0
	$udp0 set dst_port_ 0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	set sessionhelper [$ns_ create-session $node_(n4) $udp0]
	
	set rcvr0 [new Agent/LossMonitor]
	set rcvr1 [new Agent/LossMonitor]
	set rcvr2 [new Agent/LossMonitor]
	set rcvr3 [new Agent/LossMonitor]
	set rcvr4 [new Agent/LossMonitor]
	set rcvr5 [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n0) $rcvr0
	$ns_ attach-agent $node_(n1) $rcvr1
	$ns_ attach-agent $node_(n2) $rcvr2
	$ns_ attach-agent $node_(n3) $rcvr3
	$ns_ attach-agent $node_(n4) $rcvr4
	$ns_ attach-agent $node_(n5) $rcvr5
	
	$ns_ at 0.2 "$node_(n0) join-group $rcvr0 $grp0"
	$ns_ at 0.2 "$node_(n1) join-group $rcvr1 $grp0"
	$ns_ at 0.2 "$node_(n2) join-group $rcvr2 $grp0"
	$ns_ at 0.2 "$node_(n3) join-group $rcvr3 $grp0"
	$ns_ at 0.2 "$node_(n4) join-group $rcvr4 $grp0"
	$ns_ at 0.2 "$node_(n5) join-group $rcvr5 $grp0"
	
	set loss_module1 [new SelectErrorModel]
	$loss_module1 drop-packet 2 20 1
	$loss_module1 drop-target [$ns_ set nullAgent_]

	set loss_module2 [new SelectErrorModel]
	$loss_module2 drop-packet 2 10 1
	$loss_module2 drop-target [$ns_ set nullAgent_]

	set loss_module3 [new SelectErrorModel]
	$loss_module3 drop-packet 2 10 1
	$loss_module3 drop-target [$ns_ set nullAgent_]

	$ns_ insert-loss $loss_module1 $node_(n0) $node_(n1)
	$ns_ insert-loss $loss_module2 $node_(n1) $node_(n3)
	$ns_ insert-loss $loss_module3 $node_(n0) $node_(n2)

	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 1.6 "$self finish [list $rcvr0 $rcvr1 $rcvr2 $rcvr3 \
$rcvr4 $rcvr5]"
	
	$ns_ run
}

TestSuite runTest

