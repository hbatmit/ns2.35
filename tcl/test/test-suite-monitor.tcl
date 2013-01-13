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


# This test suite is for monitoring TCP variable trace
# To run all tests: test-all-monitor
# to run individual test:
# ns test-suite-monitor.tcl tcp
# ns test-suite-monitor.tcl tcp-monitor
#
# To view a list of available test to run with this script:
# ns test-suite-monitor.tcl
#
# Each of the tests uses 6 nodes 
#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1

Class TestSuite

Class Test/tcp -superclass TestSuite
# Simple tcp without monitoring

Class Test/tcp-monitor -superclass TestSuite
# Simple tcp with monitoring (nam_tracevar_)

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid <tests> : tcp tcp-monitor"
	exit 1
}

TestSuite instproc init {} {

    $self instvar ns_ n_

    set ns_ [new Simulator]

    $ns_ namtrace-all [open temp.rands w]

    foreach i " 0 1 2 3 4 5" {
	set n_($i) [$ns_ node]
    }

    $ns_ duplex-link $n_(0) $n_(2) 1Mb 20ms DropTail
    $ns_ duplex-link $n_(1) $n_(2) 1Mb 20ms DropTail
    $ns_ duplex-link $n_(2) $n_(3) 0.5Mb 20ms DropTail
    $ns_ duplex-link $n_(3) $n_(4) 1Mb 20ms DropTail
    $ns_ duplex-link $n_(3) $n_(5) 1Mb 20ms DropTail

    $ns_ duplex-link-op $n_(0) $n_(2) orient right-down
    $ns_ duplex-link-op $n_(1) $n_(2) orient right-up
    $ns_ duplex-link-op $n_(2) $n_(3) orient right
    $ns_ duplex-link-op $n_(3) $n_(4) orient right-up
    $ns_ duplex-link-op $n_(3) $n_(5) orient right-down

    $ns_ duplex-link-op $n_(2) $n_(3) queuePos 0.5
}

TestSuite instproc finish {} {
	$self instvar ns_
	global quiet

	$ns_ flush-trace
        if { !$quiet } {
                puts "running nam..."
                exec nam temp.rands &
        }
	exit 0
}

Test/tcp instproc init {flag} {
	$self instvar ns_ testName_ flag_
	set testName_ tcp
	set flag_ $flag
	$self next
}

Test/tcp instproc run {} {

	$self instvar ns_ n_ 
        $ns_ queue-limit $n_(2) $n_(3) 20

        ### TCP between n_(0) & n_(4)
        Agent/TCP set maxcwnd_ 8
        set tcp0 [new Agent/TCP]
        $ns_ attach-agent $n_(0) $tcp0
        set sink0 [new Agent/TCPSink]
        $ns_ attach-agent $n_(4) $sink0
        $ns_ connect $tcp0 $sink0
        set ftp0 [new Application/FTP]
        $ftp0 attach-agent $tcp0

        ### CBR traffic between n_(1) & n_(5)
        set udp0 [new Agent/UDP]
        $ns_ attach-agent $n_(1) $udp0
        set cbr0 [new Application/Traffic/CBR]
        $cbr0 set packetSize_ 500
        $cbr0 set interval_ 0.01
        $cbr0 attach-agent $udp0
        set null0 [new Agent/Null]
        $ns_ attach-agent $n_(5) $null0
        $ns_ connect $udp0 $null0

        ### setup operation
        $ns_ at 0.1 "$cbr0 start"
        $ns_ at 1.1 "$cbr0 stop"
        $ns_ at 0.2 "$ftp0 start"
        $ns_ at 1.1 "$ftp0 stop"
        $ns_ at 1.2 "$self finish"
        $ns_ run
}

Test/tcp-monitor instproc init {flag} {
	$self instvar ns_ testName_ flag_
	set testName_ tcp-monitor
	set flag_ $flag
	$self next 
}

Test/tcp-monitor instproc run {} {

	$self instvar ns_ n_
        $ns_ queue-limit $n_(2) $n_(3) 20

        ### TCP between n_(0) & n_(4)

        Agent/TCP set nam_tracevar_ true
        Agent/TCP set maxcwnd_ 8
        set tcp0 [new Agent/TCP]
        $ns_ attach-agent $n_(0) $tcp0
        set sink0 [new Agent/TCPSink]
        $ns_ attach-agent $n_(4) $sink0
        $ns_ connect $tcp0 $sink0
        $ns_ add-agent-trace $tcp0 tcp
        $ns_ monitor-agent-trace $tcp0
        $tcp0 tracevar cwnd_
        $tcp0 tracevar ssthresh_
        set ftp0 [new Application/FTP]
        $ftp0 attach-agent $tcp0

        ### CBR traffic between n_(1) & n_(5)
        set udp0 [new Agent/UDP]
        $ns_ attach-agent $n_(1) $udp0
        set cbr0 [new Application/Traffic/CBR]
        $cbr0 set packetSize_ 500
        $cbr0 set interval_ 0.01
        $cbr0 attach-agent $udp0
        set null0 [new Agent/Null]
        $ns_ attach-agent $n_(5) $null0
        $ns_ connect $udp0 $null0

        ### setup operation
        $ns_ at 0.1 "$cbr0 start"
        $ns_ at 1.1 "$cbr0 stop"
        $ns_ at 0.2 "$ftp0 start"
        $ns_ at 1.1 "$ftp0 stop"
        $ns_ at 1.2 "$self finish"
        $ns_ run
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
	switch $test {
                tcp -
                tcp-monitor {
			set t [new Test/$test 1]
		}
		default {
			stderr "Unknown test $test"
			exit 1
		}
	}
	$t run
}

global argv arg0
runtest $argv


