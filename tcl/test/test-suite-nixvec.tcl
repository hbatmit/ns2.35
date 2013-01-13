#
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
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

# This test suite is for validating nixvector routing
# To run the test: test-all-nixvec

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.



Simulator instproc progress { } {
    global progress_interval
    puts [format "Progress to %6.1f seconds" [$self now]]
    if { ![info exists progress_interval] } {
	set progress_interval [$self now]
    }
    $self at [expr [$self now] + $progress_interval] "$self progress"
}

Class TestSuite

# Nixvector tests
Class Test/NixVec -superclass TestSuite
Class Test/NoNixVec -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: NixVec NoNixVec"
	exit 1
}

proc RunNix { ns usenix } {
    global f
    set f [open temp.rands w]

    $ns trace-all $f

    set n0  [$ns node]
    set n1  [$ns node]
    set n2  [$ns node]
    set n3  [$ns node]
    set n4  [$ns node]
    set n5  [$ns node]
    set n6  [$ns node]
    set n7  [$ns node]
    set n8  [$ns node]
    set n9  [$ns node]
    set n10 [$ns node]
    set n11 [$ns node]
    set n12 [$ns node]

    $ns duplex-link    $n0  $n1  10mb 20ms DropTail
    $ns duplex-link-op $n0  $n1  orient 0

    $ns duplex-link    $n1  $n2  10mb 20ms DropTail
    $ns duplex-link-op $n1  $n2  orient 0.5

    $ns duplex-link    $n1  $n3  10mb 20ms DropTail
    $ns duplex-link-op $n1  $n3  orient 0

    $ns duplex-link    $n3  $n4  10mb 20ms DropTail
    $ns duplex-link-op $n3  $n4  orient 0.5

    $ns duplex-link    $n3  $n5  10mb 20ms DropTail
    $ns duplex-link-op $n3  $n5  orient 0

    $ns duplex-link    $n3  $n6  10mb 20ms DropTail
    $ns duplex-link-op $n3  $n6  orient 1.5

    $ns duplex-link    $n5  $n7  10mb 20ms DropTail
    $ns duplex-link-op $n5  $n7  orient 0.5

    $ns duplex-link    $n5  $n8  10mb 20ms DropTail
    $ns duplex-link-op $n5  $n8  orient 0.25

    $ns duplex-link    $n5  $n9  10mb 20ms DropTail
    $ns duplex-link-op $n5  $n9  orient 0

    $ns duplex-link    $n5  $n10 10mb 20ms DropTail
    $ns duplex-link-op $n5  $n10 orient 1.75

    $ns duplex-link    $n5  $n11 10mb 20ms DropTail
    $ns duplex-link-op $n5  $n11 orient 1.5

    $ns duplex-link    $n9  $n12 10mb 20ms DropTail
    $ns duplex-link-op $n9  $n12 orient 0

    set tcp0 [$ns create-connection TCP $n0 TCPSink $n12 0]
    set ftp0 [new Source/FTP]
    $ftp0 attach $tcp0
    $ns at 0.1 "$ftp0 produce 500"
    $ns at 1.0 "$ns progress"
    $ns run
}

TestSuite instproc init {} {
    #puts "Hello from TestSuite::init"
}


Test/NixVec instproc init {} {

# check if nixvector module is disabled
    if {![TclObject is-class RtModule/Nix]} {
	puts "Nixvector module is not present; validation skipped"
	exit 2
    }
    $self instvar ns
    set ns [new Simulator]
    $ns set-nix-routing
    $self next
    #puts "Hello from Test/NixVec::init"
}

Test/NixVec instproc run {} {
    $self instvar ns
    puts "Test/NixVec Starting Simulation..."
    $ns at 10.0 "$self finish"
    RunNix $ns 1
}


Test/NoNixVec instproc init {} {
    $self instvar ns
    set ns [new Simulator]
    $self next
    #puts "Hello from Test/NoNixVec::init"
}

Test/NoNixVec instproc run {} {
    $self instvar ns
    puts "Test/NoNixVec Starting Simulation..."
    $ns at 10.0 "$self finish"
    RunNix $ns 0
}


TestSuite instproc finish {} {
    $self instvar ns
    global quiet
    global f

    puts "finishing.."
    $ns flush-trace
    if { [info exists f] } {
        close $f
    }
    exit 0
}

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









