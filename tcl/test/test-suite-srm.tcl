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


# This test suite is for validating SRM
# To run all tests: test-all-srm
# to run individual test:
# ns test-suite-srm.tcl srm-chain
# ns test-suite-srm.tcl srm-star
# ....
#
# To view a list of available test to run with this script:
# ns test-suite-srm.tcl
#

#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP TCP SRM aSRM SRMEXT; 
# hdrs reqd for validation test

Class TestSuite

Class Test/srm-chain -superclass TestSuite
# Simple chain topology

Class Test/srm-star -superclass TestSuite
# Simple star topology

Class Test/srm-adapt-rep -superclass TestSuite
# simple 8 node star topology, runs for 10s, tests Adaptive repair timers.

Class Test/srm-adapt-req -superclass TestSuite
# simple 8 node star topology, runs for 10s, tests Adaptive request timers.

#Class Test/srm-chain-session -superclass TestSuite
# session simulations using srm in chain topo



proc usage {} {
    global argv0
    puts stderr "usage: ns $argv0 <tests> "
    puts "Valid Tests: srm-chain srm-star adapt-rep-timer adapt-req-timer\
	    srm-chain-session"
    exit 1
}

Class Topology

Topology instproc init {} {
    $self instvar nmax_ n_
}

Topology instproc totalnodes? {} {
    $self instvar nmax_ 
    return $nmax_
}

Topology instproc node? num {
    $self instvar n_
    return $n_($num)
}

Topology instproc src? {} {
    $self instvar src_
    return $src_
}

Class Topology/chain5 -superclass Topology

Topology/chain5 instproc init ns {
    $self instvar nmax_ n_ src_
    set nmax_ 5
    for {set i 0} {$i <= $nmax_} {incr i} {
	set n_($i) [$ns node]
    }
    $self next
    set chainMax [expr $nmax_ - 1]
    set j 0
    for {set i 1} {$i <= $chainMax} {incr i} {
	$ns duplex-link $n_($i) $n_($j) 1.5Mb 10ms DropTail
	$ns duplex-link-op $n_($j) $n_($i) orient right
	set j $i
    }
    set src_ 0
    $ns duplex-link $n_([expr $nmax_ - 2]) $n_($nmax_) 1.5Mb 10ms DropTail
    $ns duplex-link-op $n_([expr $nmax_ - 2]) $n_($nmax_) orient right-up
    $ns duplex-link-op $n_([expr $nmax_ - 2]) $n_([expr $nmax_-1]) orient right-down

    #$ns queue-limit $n_(0) $n_(1) 2	;# q-limit is 1 more than max #packets in q.
    #$ns queue-limit $n_(1) $n_(0) 2 
    
}

Class Topology/star8 -superclass Topology

Topology/star8 instproc init ns {
    $self instvar nmax_ n_ src_
    set nmax_ 8
    for {set i 0} {$i <= $nmax_} {incr i} {
	set n_($i) [$ns node]
    }
    $self next
    for {set i 1} {$i <= $nmax_} {incr i} {
	$ns duplex-link $n_($i) $n_(0) 1.5Mb 10ms DropTail
    }
    set src_ 1
}

TestSuite instproc finish {src} {
    global opts
    $self instvar ns_ n_
    $src stop
    $ns_ flush-trace
    if {$opts(quiet) == "false"} {
    	puts "finishing.."
    }
    exit 0
}

TestSuite instproc set-mcast {src num time} {
    global opts
    $self instvar ns_ n_ g_
    if {$opts(quiet) == "false"} {
    	puts "seting mcast.."
    }
    set mh [$ns_ mrtproto CtrMcast {}]
    $ns_ at 0.3 "$mh switch-treetype $g_"
    
    # now the multicast, and the agents
    #set srmSimType Deterministic
    set fid 0
    for {set i 0} {$i <= $num} {incr i} {
	set srm($i) [new Agent/SRM/Deterministic]
	$srm($i) set dst_addr_ $g_
	$srm($i) set dst_port_ 0
	$srm($i) set fid_ [incr fid]
	$ns_ at 1.0 "$srm($i) start"
	$ns_ attach-agent $n_($i) $srm($i)
    }
    # Attach a data source to srm(1)
    set packetSize 800
    set s [new Agent/CBR]
    $s set interval_ 0.04
    # Agent/CBR is an old form, used in backward compatibility mode only.
    # set s [new Application/Traffic/CBR]
    # 6400 bits/packet, 25 packets per second, 160Kbps
    $s set packetSize_ $packetSize
    # $s set rate_ 160Kb
    # $s attach-agent $srm($src)
    $srm($src) traffic-source $s
    $srm($src) set packetSize_ $packetSize
    $s set fid_ 0
    $ns_ at 3.0 "$srm($src) start-source"

    $ns_ at $time "$self finish $s"
}

TestSuite instproc set-session {src num time } {
    $self instvar ns_ n_ g_
    puts "running session-mcast"
    set fid 0
    for {set i 0} {$i <= $num} {incr i} {
	set srm($i) [new Agent/SRM/Deterministic]
	$srm($i) set dst_addr_ $g_
	$srm($i) set fid_ [incr fid]
	$ns_ at 1.0 "$srm($i) start"
	$ns_ attach-agent $n_($i) $srm($i)
	set sessionhelper($i) [$ns_ create-session $n_($i) $srm($i)]
    }
    # Attach a data source to srm(0)
    set packetSize 800
    set s [new Agent/CBR]
    $s set interval_ 0.04
    $s set packetSize_ $packetSize
    $srm(0) traffic-source $s
    $srm(0) set packetSize_ $packetSize
    $s set fid_ 0
    $ns_ at 3.5 "$srm(0) start-source"
    
    set loss_module [new SRMErrorModel]
    $loss_module drop-packet 2 10 1
    $loss_module drop-target [$ns_ set nullAgent_]
    $ns_ at 1.25 "$sessionhelper(0) insert-depended-loss $loss_module $srm(1) $srm(0) $g_"
    $ns_ at $time "$self finish $s"
}

TestSuite instproc init {} {
    $self instvar ns_ n_ g_ testName_ topo_ net_ time_ num_
    if {$testName_ == "srm-chain-session"} {
	set ns_ [new SessionSim]
	$ns_ namtrace-all [open temp.rands w]
    } else {
	set ns_ [new Simulator -multicast on]
	$ns_ trace-all [open temp.rands w]
	#$ns_ namtrace-all [open out.nam w]
    }
    set g_ [Node allocaddr]
    set topo_ [new Topology/$net_ $ns_]
    set nmax [$topo_ totalnodes?]
    for {set i 0} {$i <= $nmax} {incr i} { 
	set n_($i) [$topo_ node? $i]
    }
    set src [$topo_ src?]
    if {$testName_ == "srm-chain-session"} {
	$self set-session $src $num_ $time_
    } else {
	$self set-mcast $src $num_ $time_
    }
}


Test/srm-chain instproc init {} {
    $self instvar ns_ testName_ net_ time_ num_
    set testName_ srm-chain
    set net_ chain5
    set time_ 4.0
    set num_ 5
    $self next
}

Test/srm-chain instproc run {} {
    $self instvar ns_ n_
    set loss_module [new SRMErrorModel]
    $loss_module drop-packet 2 10 1
    $loss_module drop-target [$ns_ set nullAgent_]
    $ns_ at 1.25 "$ns_ lossmodel $loss_module $n_(0) $n_(1)"
    $ns_ run

}

Test/srm-star instproc init {} {
    $self instvar ns_ testName_ net_ time_ num_
    set testName_ srm-star
    set net_ star8
    set time_ 4.0
    set num_ 8
    $self next
}

Test/srm-star instproc run {} {
    $self instvar ns_ n_
    set loss_module [new SRMErrorModel]
    $loss_module drop-packet 2 10 1
    $loss_module drop-target [$ns_ set nullAgent_]
    $ns_ at 1.25 "$ns_ lossmodel $loss_module $n_(1) $n_(0)"
    $ns_ run
}



Test/srm-adapt-rep instproc init {} {
    $self instvar ns_ testName_ net_ time_ num_
    set testName_ srm-adapt-rep
    set net_ star8
    set time_ 10.0
    set num_ 8
    $self next
}

Test/srm-adapt-rep instproc run {} {
    $self instvar ns_ n_
    $n_(0) shape "other"
    $n_(1) shape "box"
    $ns_ duplex-link-op $n_(0) $n_(1) queuePos 0
    set loss_module [new SRMErrorModel]
    $loss_module drop-packet 2 200 1
    $loss_module drop-target [$ns_ set nullAgent_]
    $ns_ lossmodel $loss_module $n_(0) $n_(2)
    $ns_ run
}

Test/srm-adapt-req instproc init {} {
    $self instvar ns_ testName_ net_ time_ num_
    set testName_ srm-adapt-req
    set net_ star8
    set time_ 10.0
    set num_ 8
    $self next
}

Test/srm-adapt-req instproc run {} {
    $self instvar ns_ n_
    $n_(0) shape "other"
    $n_(1) shape "box"
    $ns_ duplex-link-op $n_(0) $n_(1) queuePos 0
    set loss_module [new SRMErrorModel]
    $loss_module drop-packet 2 200 1
    $loss_module drop-target [$ns_ set nullAgent_]
    $ns_ lossmodel $loss_module $n_(1) $n_(0)
    $ns_ run
}

#Test/srm-chain-session instproc init {} {
#    $self instvar ns_ testName_ net_ time_ num_
#    set testName_ srm-chain-session
#    set net_ chain5
#    set time_ 4.0
#    set num_ 5
#    $self next
#}

#Test/srm-chain-session instproc run {} {
#    $self instvar ns_
#    $ns_ run
#}

proc runtest {arg} {
    global opts
    set opts(quiet) false
    set b [llength $arg]
    if {$b == 1} {
	set test $arg
    } elseif {$b == 2} {
	set test [lindex $arg 0]
	set second [lindex $arg 1]
	if {$second == "QUIET" || $second == "quiet"} {
		set opts(quiet) true
	}
    } else {
	usage
    }
    if [catch {set t [new Test/$test]} result] {
	puts stderr $result
	puts "Error: Unknown test:$test"
	usage
	exit 1
    }
    $t run
}

global argv argv0
runtest $argv



