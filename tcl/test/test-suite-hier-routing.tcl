#
# Copyright (c) 1998 University of Southern California.
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

# XXX Strange strange:
#  There is a order dependence of hierarchical routing and multicast: 
#
#  if you turn on hier routing BEFORE multicast, the trace result is
#  different from you turn on hier routing AFTER multicast. The reason is
#  that set-address-format{} checks multicast from ns-address.tcl; if it's 
#  on, it sets a different address format.
#
#  What I don't understand is, since hier addressing format depends on
#  whether mcast is on, why in the test suite test-suite-hier-routing.tcl,
#  multicast is turned on AFTER hierarchical routing? Is this a bug? Should
#  there be this dependence? 

# This test suite is for validating hierarchical routing
# To run all tests: test-all-hier-routing
# to run individual test:
# ns test-suite-hier-routing.tcl hier-simple
# ns test-suite-hier-routing.tcl hier-cmcast
# ....
#
# To view a list of available test to run with this script:
# ns test-suite-hier-routing.tcl
#
# Every test uses a 10 node hierarchical topology

##remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Class TestSuite

# Simple hierarchical routing
Class Test/hier-simple -superclass TestSuite

# hierarchical routing with CentralisedMcast
Class Test/hier-cmcast -superclass TestSuite

# session simulations using hierarchical routing
Class Test/hier-session -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: hier-simple hier-cmcast hier-deDM \
			hier-session"
	exit 1
}

# A place holder that can be overridden
TestSuite instproc init-hier-routing {} {
	$self instvar ns_
	# By default we use node-config, then we switch to HierNode to 
	# test backward compability
	$ns_ node-config -addressType hierarchical
}

TestSuite instproc init-simulator {} {
}

TestSuite instproc alloc-mcast-addr {} {
}

TestSuite instproc init {} {
	$self instvar ns_ n_ g_ testName_
	$self init-simulator
	$self init-hier-routing
	$self alloc-mcast-addr
	#setup hierarchical topology
	AddrParams set domain_num_ 2
	lappend cluster_num 2 2
	AddrParams set cluster_num_ $cluster_num
	lappend eilastlevel 2 3 2 3
	AddrParams set nodes_num_ $eilastlevel
	set naddr {0.0.0 0.0.1 0.1.0 0.1.1 0.1.2 1.0.0 1.0.1 1.1.0 \
			1.1.1 1.1.2}
	for {set i 0} {$i < 10} {incr i} {
		set n_($i) [$ns_ node [lindex $naddr $i]]
	}
	$ns_ duplex-link $n_(0) $n_(1) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(1) $n_(2) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(2) $n_(3) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(2) $n_(4) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(1) $n_(5) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(5) $n_(6) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(6) $n_(7) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(7) $n_(8) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(8) $n_(9) 5Mb 2ms DropTail

}

TestSuite instproc finish {} {
	$self instvar ns_
	global quiet

	$ns_ flush-trace
        if { !$quiet } {
                puts "running nam..."
                exec nam temp.rands.nam &
        }
	#puts "finishing.."
	exit 0
}

Test/hier-simple instproc init {} {
	$self instvar testName_ 
	set testName_ hier-simple
	$self next
}

Test/hier-simple instproc init-simulator {} {
	$self instvar ns_
	set ns_ [new Simulator]
	$ns_ trace-all [open temp.rands w]
	$ns_ namtrace-all [open temp.rands.nam w]
}

Test/hier-simple instproc run {} {
	$self instvar ns_ n_ 
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n_(0) $udp0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	set udp1 [new Agent/UDP]
	$ns_ attach-agent $n_(2) $udp1
	$udp1 set class_ 1
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1

	set null0 [new Agent/Null]
	$ns_ attach-agent $n_(8) $null0
	set null1 [new Agent/Null]
	$ns_ attach-agent $n_(6) $null1

	$ns_ connect $udp0 $null0
	$ns_ connect $udp1 $null1
	$ns_ at 1.0 "$cbr0 start"
	$ns_ at 1.1 "$cbr1 start"
	$ns_ at 3.0 "$self finish"
	$ns_ run

}

Test/hier-cmcast instproc init {} {
	$self instvar ns_ testName_
	set testName_ hier-cmcast
	$self next 
}

Test/hier-cmcast instproc init-simulator {} {
	$self instvar ns_
	set ns_ [new Simulator]
	$ns_ trace-all [open temp.rands w]
	$ns_ namtrace-all [open temp.rands.nam w]
}

Test/hier-cmcast instproc alloc-mcast-addr {} {
	$self instvar g_ ns_
	# XXX If we first allocate a simulator with -multicast on, then 
	# turn on hierarchical routing, the result trace file is different
	# from we turn multicast on AFTER we turn on hierarchical routing!
	# The reason is that set-address-format{} in ns-address.tcl sets a 
	# different address format if it finds multicast is turned on. 
	$ns_ multicast on
	set g_ [Node allocaddr]
}

Test/hier-cmcast instproc run {} {
	$self instvar ns_ n_ g_
	set mproto CtrMcast
	set mrthandle [$ns_ mrtproto $mproto {}]
	$ns_ at 0.0 "$mrthandle switch-treetype $g_" 
	
	for {set i 1} {$i < 10} {incr i} {
		set rcvr($i) [new Agent/Null]
		$ns_ attach-agent $n_($i) $rcvr($i)
		$ns_ at $i "$n_($i) join-group $rcvr($i) $g_"
		incr i
	}
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n_(0) $udp0
	$udp0 set dst_addr_ $g_
	$udp0 set dst_port_ 0
	$udp0 set class_ 2
	set sender [new Application/Traffic/CBR]
	$sender attach-agent $udp0
	$ns_ at 0.0 "$sender start"
	$ns_ at 10.0 "$self finish"
	
	$ns_ run
	
}
	
#Test/hier-deDM instproc init {flag} {
#	  $self instvar ns_ testName_ flag_
#	  set testName_ hier-detailedDM
#	  set flag_ $flag
#	  $self next 
#}
#
#Test/hier-deDM instproc run {} {
#	  $self instvar ns_ n_ g_
#	  set mproto detailedDM
#	  set mrthandle [$ns_ mrtproto $mproto {}]
#	  
#	  for {set i 1} {$i < 10} {incr i} {
#		  set rcvr($i) [new Agent/LossMonitor]
#		  $ns_ attach-agent $n_($i) $rcvr($i)
#		  $ns_ at $i "$n_($i) join-group $rcvr($i) $g_"
#		  incr i
#	  }
#	  set udp0 [new Agent/UDP]
#	  $ns_ attach-agent $n_(0) $udp0
#	  $udp0 set dst_ $g_
#	  $udp0 set class_ 2
#	  set sender [new Application/Traffic/CBR]
#	  $sender attach-agent $udp0
#	  $ns_ at 0.1 "$sender start"
#	  $ns_ at 10.0 "$self finish"
#	  
#	  $ns_ run
#}


Test/hier-session instproc init {} {
	$self instvar ns_ testName_ 
	set testName_ hier-session
	$self next 
}

Test/hier-session instproc init-simulator {} {
	$self instvar ns_ g_
	set ns_ [new SessionSim]
	set g_ [Node allocaddr]
	$ns_ namtrace-all [open temp.rands w]
}

Test/hier-session instproc run {} {
	$self instvar ns_ n_ g_
	
	for {set i 1} {$i < 10} {incr i} {
		set rcvr($i) [new Agent/LossMonitor]
		$ns_ attach-agent $n_($i) $rcvr($i)
		$ns_ at $i "$n_($i) join-group $rcvr($i) $g_"
		incr i
	}
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n_(0) $udp0
	$udp0 set dst_addr_ $g_
	$udp0 set dst_port_ 0
	set sender [new Application/Traffic/CBR]
	$sender attach-agent $udp0
	$ns_ create-session $n_(0) $udp0
	$ns_ at 0.1 "$sender start"
	$ns_ at 10.0 "$self finish"
	
	$ns_ run
}

#
# Backward compatibility tests 
#
#  Class Test-BackCompat -superclass TestSuite

#  Test-BackCompat instproc init-hier-routing {} {
#  	$self instvar ns_
#  	puts "testing backward compatibility of hierarchical routing"
#  	Simulator set node_factory_ HierNode
#  	$ns_ set-address-format hierarchical
#  }

#  Class Test/hier-simple-bc -superclass {Test-BackCompat Test/hier-simple}
#  Class Test/hier-cmcast-bc -superclass {Test-BackCompat Test/hier-cmcast}
#  Class Test/hier-session-bc -superclass {Test-BackCompat Test/hier-session}

proc runtest {arg} {
	global quiet
	set quiet 0

	set b [llength $arg]
	if {$b == 1} {
		set test $arg
	} elseif {$b == 2} {
		set test [lindex $arg 0]
	    if {[lindex $arg 1] == "QUIET"} {
		set quiet 1
	    }
	} else {
		usage
	}
	set t [new Test/$test]
	$t run
}

global argv arg0
runtest $argv
