#
# Copyright (c) 1998,2000 University of Southern California.
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

# This test suite is for validating the diffserv module ported from Nortel
# To run all tests: test-all-diffserv
#
# To run individual test:
# ns test-suite-diffserv.tcl tb | tsw2cm | tsw3cm | srtcm | trtcm
#
# To view a list of available test to run with this script:
# ns test-suite-diffserv.tcl
#

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5 
Queue/RED set maxthresh_ 15
# The RED parameter defaults are being changed for automatic configuration.

# The topology used in the test suite is:
#
#    s1 -------
#        10 Mb  \
#         5 ms   \
#                 e1 ----- core ------ e2 ------ dest
#                /   10 Mb       5 Mb      10 Mb 
#               /     5 ms       5 ms       5 ms
#    s2 -------
#        10 Mb
#         5 ms
#

Class TestSuite

# With Policy as Token Bucket
Class Test/tb -superclass TestSuite

# With Policy as Time Sliding Window Two Color Marker (tsw2cm)
Class Test/tsw2cm -superclass TestSuite

# With Policy as Time Sliding Window Two Color Marker (tsw3cm)
Class Test/tsw3cm -superclass TestSuite

# With Policy as Single Rate Three Color Marker (srtcm)
Class Test/srtcm -superclass TestSuite

# With Policy as Two Rate Three Color Marker (trtcm)
Class Test/trtcm -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: tb tsw2cm tsw3cm srtcm trtcm"
	exit 1
}

# Set up parameters for ds redqs for a link.
TestSuite instproc create_q {qec_ qce_ s1 s2 d1 d2} {
    $self instvar packetSize_ testName_
    $self instvar cir0 cbs0 pir0 pbs0 ebs0 rate0 cir1 cbs1 pir1 pbs1 ebs1 rate1

    # Set DS RED parameters from Edge to Core:
    $qec_ meanPktSize $packetSize_
    $qec_ set numQueues_ 1
    $qec_ setNumPrec 2

    switch $testName_ {
	"tb" {          # Token bucket policy
	    $qec_ addPolicyEntry [$s1 id] [$d1 id] TokenBucket 10 $cir0 $cbs0
	    $qec_ addPolicyEntry [$s2 id] [$d2 id] TokenBucket 10 $cir1 $cbs1
	    $qec_ addPolicerEntry TokenBucket 10 11
	}

	"tsw2cm" { 	# TSW2CM policy
	    $qec_ addPolicyEntry [$s1 id] [$d1 id] TSW2CM 10 $cir0
	    $qec_ addPolicyEntry [$s2 id] [$d2 id] TSW2CM 10 $cir1
	    $qec_ addPolicerEntry TSW2CM 10 11
	}

	"tsw3cm" { 	# TSW3CM policy
	    $qec_ setNumPrec 3
	    $qec_ addPolicyEntry [$s1 id] [$d1 id] TSW3CM 10 $cir0 $pir0
	    $qec_ addPolicyEntry [$s2 id] [$d2 id] TSW3CM 10 $cir1 $pir1
	    $qec_ addPolicerEntry TSW3CM 10 11 12
	    $qec_ addPHBEntry 12 0 2
	    $qec_ configQ 0 2  5 10 0.20
	}

	"srtcm" { 	# SRTCM policy
	    $qec_ setNumPrec 3
	    $qec_ addPolicyEntry [$s1 id] [$d1 id] srTCM 10 $cir0 $cbs0 $ebs0
	    $qec_ addPolicyEntry [$s2 id] [$d2 id] srTCM 10 $cir1 $cbs1 $ebs1
	    $qec_ addPolicerEntry srTCM 10 11 12
	    $qec_ addPHBEntry 12 0 2
	    $qec_ configQ 0 2  5 10 0.20
	}
	
	"trtcm" { 	# TRTCM policy
	    $qec_ setNumPrec 3
	    $qec_ addPolicyEntry [$s1 id] [$d1 id] trTCM 10 $cir0 $cbs0 $pir0 $pbs0
	    $qec_ addPolicyEntry [$s2 id] [$d2 id] trTCM 10 $cir1 $cbs1 $pir1 $pbs1
	    $qec_ addPolicerEntry trTCM 10 11 12
	    $qec_ addPHBEntry 12 0 2
	    $qec_ configQ 0 2  5 10 0.20
	}

	default {error "Unknow policer!!! exist"}
    }

    $qec_ addPHBEntry 10 0 0
    $qec_ addPHBEntry 11 0 1
    $qec_ configQ 0 0 20 40 0.02
    $qec_ configQ 0 1 10 20 0.10

    # Set DS RED parameters from Core to Edge:
    $qce_ meanPktSize $packetSize_
    $qce_ set numQueues_ 1
    $qce_ setNumPrec 2
    $qce_ addPHBEntry 10 0 0
    $qce_ addPHBEntry 11 0 1
    $qce_ configQ 0 0 20 40 0.02
    $qce_ configQ 0 1 10 20 0.10

    if {$testName_ == "tsw3cm" || $testName_ == "srtcm" || $testName_ == "trtcm"} {
	$qce_ setNumPrec 3
	$qce_ addPHBEntry 12 0 2
	$qce_ configQ 0 2  5 10 0.20
    }
}


# Create the simulation scenario for the test suite.
TestSuite instproc create-scenario {} { 
    global quiet
    $self instvar ns_ packetSize_ finishTime_ testName_ rate0 rate1

    # Set up the network topology shown above:
    set s1 [$ns_ node]
    set s2 [$ns_ node]
    set e1 [$ns_ node]
    set core [$ns_ node]
    set e2 [$ns_ node]
    set dest [$ns_ node]

    $ns_ duplex-link $s1 $e1 10Mb 5ms DropTail
    $ns_ duplex-link $s2 $e1 10Mb 5ms DropTail
    
    $ns_ simplex-link $e1 $core 10Mb 5ms dsRED/edge
    $ns_ simplex-link $core $e1 10Mb 5ms dsRED/core
    
    $ns_ simplex-link $core $e2 5Mb 5ms dsRED/core
    $ns_ simplex-link $e2 $core 5Mb 5ms dsRED/edge
    
    $ns_ duplex-link $e2 $dest 10Mb 5ms DropTail

    # Config each RED queue.
    set qE1C [[$ns_ link $e1 $core] queue]
    set qE2C [[$ns_ link $e2 $core] queue]
    set qCE1 [[$ns_ link $core $e1] queue]
    set qCE2 [[$ns_ link $core $e2] queue]

    # Set DS RED parameters for Qs:
    $self create_q $qE1C $qCE1 $s1 $s2 $dest $dest
    $self create_q $qE2C $qCE2 $dest $dest $s1 $s2
    
    # Set up one CBR connection between each source and the destination:
    set udp0 [new Agent/UDP]
    $ns_ attach-agent $s1 $udp0
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 attach-agent $udp0
    $cbr0 set packet_size_ $packetSize_
    $udp0 set packetSize_ $packetSize_
    $cbr0 set rate_ $rate0
    set null0 [new Agent/Null]
    $ns_ attach-agent $dest $null0
    $ns_ connect $udp0 $null0
    $ns_ at 0.0 "$cbr0 start"
    $ns_ at $finishTime_ "$cbr0 stop"

    set udp1 [new Agent/UDP]
    $ns_ attach-agent $s2 $udp1
    set cbr1 [new Application/Traffic/CBR]
    $cbr1 attach-agent $udp1
    $cbr1 set packet_size_ $packetSize_
    $udp1 set packetSize_ $packetSize_
    $cbr1 set rate_ $rate1
    set null1 [new Agent/Null]
    $ns_ attach-agent $dest $null1
    $ns_ connect $udp1 $null1
    $ns_ at 0.0 "$cbr1 start"
    $ns_ at $finishTime_ "$cbr1 stop"

    if {$quiet == 0} {
	$qE1C printPolicyTable
	$qE1C printPolicerTable
	$ns_ at 10.0 "$qCE2 printStats"
	$ns_ at 20.0 "$qCE2 printStats"
    }
}

TestSuite instproc init {} {
    global tracefd quiet
    $self instvar ns_ testName_ finishTime_ packetSize_

    set ns_         [new Simulator]
    set tracefd	[open "temp.rands" w]
    $ns_ trace-all $tracefd

    set finishTime_ 10.0
    set packetSize_ 1000
    
    if {$quiet == 0} {
	$ns_ at $finishTime_ "puts \"NS EXITING...\" ;"
    }

    $ns_ at [expr $finishTime_ + 1.0] "$self finish"
}


Test/tb instproc init {} {
    $self instvar ns_ testName_ cir0 cbs0 rate0 cir1 cbs1 rate1
    set testName_       tb
    set cir0  1000000
    set cbs0     3000
    set rate0 2000000
    set cir1  1000000
    set cbs1    10000
    set rate1 3000000

    $self next
}

Test/tb instproc run {} {
    $self instvar ns_
    
    $self create-scenario
    $ns_ run
}

Test/tsw2cm instproc init {} {
    $self instvar ns_ testName_ cir0 rate0 cir1 rate1
    set testName_       tsw2cm
    set cir0  1000000
    set rate0 2000000
    set cir1  1000000
    set rate1 3000000

    $self next
}

Test/tsw2cm instproc run {} {
    $self instvar ns_

    $self create-scenario
    $ns_ run
}

Test/tsw3cm instproc init {} {
    $self instvar ns_ testName_ cir0 pir0 rate0 cir1 pir1 rate1
    set testName_       tsw3cm
    set cir0   100000
    set pir0   500000
    set rate0 4000000
    set cir1   400000
    set pir1  1000000
    set rate1 2000000

    $self next
}

Test/tsw3cm instproc run {} {
    $self instvar ns_

    $self create-scenario
    $ns_ run
}

Test/srtcm instproc init {} {
    $self instvar ns_ testName_ cir0 cbs0 ebs0 rate0 cir1 cbs1 ebs1 rate1
    set testName_ srtcm
    set cir0  1000000
    set cbs0     2000
    set ebs0     3000 
    set rate0 3000000
    set cir1  1000000
    set cbs1     2000
    set ebs1     6000
    set rate1 3000000

    $self next
}

Test/srtcm instproc run {} {
    $self instvar ns_

    $self create-scenario
    $ns_ run
}

Test/trtcm instproc init {} {
    $self instvar ns_ testName_ cir0 cbs0 pir0 pbs0 rate0 cir1 cbs1 pir1 pbs1 rate1
    set testName_ trtcm

    set cir0  1000000
    set cbs0     2000
    set pir0  2000000
    set pbs0     3000 
    set rate0 3000000
    set cir1  1000000
    set cbs1     2000
    set pir1  1000000
    set pbs1     3000
    set rate1 3000000

    $self next
}

Test/trtcm instproc run {} {
    $self instvar ns_

    $self create-scenario
    $ns_ run
}

TestSuite instproc finish {} {
    global quiet tracefd
    $self instvar ns_
    
    $ns_ flush-trace
    close $tracefd
    
    if {$quiet == 0} {
	puts "finishing.." }

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
	if {$quiet == 0} {
	    puts "Starting Simulation..." }
	$t run
}

global argv arg0
runtest $argv
