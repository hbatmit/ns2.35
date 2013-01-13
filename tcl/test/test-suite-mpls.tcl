# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-09-11 15:22:45 haoboy>
#
# Copyright (c) 1995 The Regents of the University of California.
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
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-mpls.tcl,v 1.6 2005/06/11 04:42:09 sfloyd Exp $

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP MPLS LDP ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

Class TestSuite

TestSuite instproc init { name } {
	$self instvar ns_ totalpkt_ testname_
	set testname_ $name
	set ns_ [new Simulator]
	set totalpkt_ 0
}

TestSuite instproc finish args {
	$self instvar traceFile_ ns_
	$self recv-pkts
	global quiet
	if { $quiet != "true" } {
		$ns_ flush-trace
		close $traceFile_
	}
	exit 0
}

TestSuite instproc openTrace { stopTime } {
	$self instvar ns_ traceFile_ testname_
	global quiet
	if { $quiet != "true" } {
		set traceFile_ [open $testname_.nam w]
		$ns_ namtrace-all $traceFile_
	}
	$ns_ at $stopTime "$self finish"
}

TestSuite instproc attach-expoo-traffic { node sink size burst idle rate } {
	$self instvar ns_
		
	set udp [new Agent/UDP]
	$ns_ attach-agent $node $udp
		
	set traffic [new Application/Traffic/Exponential]
	$traffic set packetSize_ $size
	$traffic set burst_time_ $burst
	$traffic set idle_time_ $idle
	$traffic set rate_ $rate
	$traffic attach-agent $udp

	$ns_ connect $udp $sink
	return $traffic
}

TestSuite instproc record {} {
        $self instvar sink0_ totalpkt_ ns_
	# Set the time after which the procedure should be called again
        set time 0.005
	# How many bytes have been received by the traffic sink?
        set bw0 [$sink0_ set bytes_]
	# Get the current time
        set now [$ns_ now]
	# Calculate the bandwidth (in MBit/s) and write it to the file
	puts "$now [expr $bw0/$time*8/1000000]"
	# Reset the bytes_ values on the traffic sink
        $sink0_ set bytes_ 0
	#Re-schedule the procedure
        $ns_ at [expr $now+$time] "$self record"
        set bw0 [expr $bw0 / 200]
        set totalpkt_ [expr $totalpkt_ + $bw0]
}

TestSuite instproc recv-pkts {} {
	$self instvar totalpkt_
	flush stdout
	puts "The Number of Total received packet is $totalpkt_"
}

TestSuite instproc run {} {
	$self instvar ns_
	$ns_ run
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> \[<quiet>\]"
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
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

	set quiet false
	switch $argc {
		1 {
			set test $argv
			isProc? Test $test
			set topo ""
		}
		2 {
			set test [lindex $argv 0]
			isProc? Test $test
			set extra [lindex $argv 1]
			if {$extra == "QUIET"} {
				set quiet true
			}
		}
		default {
			usage
		}
	}
	set t [new Test/$test $test]
	$t run
}

Class Test/simple -superclass TestSuite

Test/simple instproc recv-pkts {} {
	# Nothing, since we do not do record{}.
}

Test/simple instproc init args {
	eval $self next $args
	$self instvar ns_

	Classifier/Addr/MPLS set control_driven_ 1

	# Turn on all traces to stdout
	Agent/LDP set trace_ldp_ 1
	Classifier/Addr/MPLS set trace_mpls_ 1

	$self openTrace 5.0

	set Node0  [$ns_ node]
	set Node1  [$ns_ node]
	$ns_ node-config -MPLS ON
	set LSR2   [$ns_ node]
	set LSR3   [$ns_ node]
	set LSR4   [$ns_ node]
	set LSR5   [$ns_ node]
	set LSR6   [$ns_ node]
	set LSR7   [$ns_ node]
	set LSR8   [$ns_ node]
	$ns_ node-config -MPLS OFF
	set Node9  [$ns_ node]
	set Node10 [$ns_ node]

	$ns_ duplex-link $Node0 $LSR2  1Mb  10ms DropTail
	$ns_ duplex-link $Node1 $LSR2  1Mb  10ms DropTail
	$ns_ duplex-link $LSR2  $LSR3  1Mb  10ms DropTail
	$ns_ duplex-link $LSR3  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR4  $LSR8  1Mb  10ms DropTail
	$ns_ duplex-link $LSR2  $LSR5  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR6  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR6  $LSR7  1Mb  10ms DropTail
	$ns_ duplex-link $LSR6  $LSR8  1Mb  10ms DropTail
	$ns_ duplex-link $LSR7  $LSR8  1Mb  10ms DropTail
	$ns_ duplex-link $LSR7  $Node9  1Mb  10ms DropTail
	$ns_ duplex-link $LSR8  $Node10 1Mb  10ms DropTail

	#
	# configure ldp agents on all mpls nodes
	#
	for {set i 2} {$i < 9} {incr i} {
		for {set j [expr $i+1]} {$j < 9} {incr j} {
			set a LSR$i
			set b LSR$j
			eval $ns_ LDP-peer $$a $$b
		}
	}

	#
	# set ldp-message clolr
	#
	$ns_ ldp-request-color       blue
	$ns_ ldp-mapping-color       red
	$ns_ ldp-withdraw-color      magenta
	$ns_ ldp-release-color       orange
	$ns_ ldp-notification-color  yellow
	
	#
	# make agent to send packets
	#
	set Src0 [new Application/Traffic/CBR]
	set udp0 [new Agent/UDP]
	$Src0 attach-agent $udp0
	$ns_ attach-agent $Node0 $udp0
	$Src0 set packetSize_ 500
	$Src0 set interval_ 0.010
	
	set Src1 [new Application/Traffic/CBR]
	set udp1 [new Agent/UDP]
	$Src1 attach-agent $udp1
	$ns_ attach-agent $Node1 $udp1
	$Src1 set packetSize_ 500
	$Src1 set interval_ 0.010

	set Dst0 [new Agent/Null]
	$ns_ attach-agent $Node9 $Dst0
	set Dst1 [new Agent/Null]
	$ns_ attach-agent $Node10 $Dst1
	$ns_ connect $udp0 $Dst0
	$ns_ connect $udp1 $Dst1
	
	$ns_ at 0.1  "$Src0 start"
	$ns_ at 0.1  "$Src1 start"

	for {set i 2} {$i < 9} {incr i} {
		set a LSR$i
		set m [eval $$a get-module "MPLS"]
		eval set LSR$i $m
	}

	$ns_ at 0.2  "$LSR7 ldp-trigger-by-withdraw 9 -1"
	$ns_ at 0.2  "$LSR8 ldp-trigger-by-withdraw 10 -1"

	$ns_ at 0.3  "$LSR2 flow-aggregation 9 -1  6 -1"
	$ns_ at 0.3  "$LSR2 flow-aggregation 10 -1 6 -1"
	$ns_ at 0.5  "$LSR6 ldp-trigger-by-withdraw 6 -1"
	$ns_ at 0.7  "$Src1 stop"

	$ns_ at 0.7  "$LSR2 make-explicit-route  7  5_4_8_6_7  3000  -1"
	$ns_ at 0.9  "$LSR2 flow-erlsp-install   9 -1   3000"
	$ns_ at 1.1  "$LSR2 ldp-trigger-by-release  7 3000"

	$ns_ at 1.2  "$LSR4 make-explicit-route  8  4_5_6_8       3500  -1"
	$ns_ at 1.4  "$LSR2 make-explicit-route  7  2_3_4_3500_7  3600  -1"
	$ns_ at 1.6  "$LSR2 flow-erlsp-install   9 -1   3600"
	
	$ns_ at 2.0 "$Src0 stop"

	$ns_ at 2.1 "$LSR2 pft-dump"
	$ns_ at 2.1 "$LSR2 erb-dump"
	$ns_ at 2.1 "$LSR2 lib-dump"

	$ns_ at 2.1 "$LSR3 pft-dump"
	$ns_ at 2.1 "$LSR3 erb-dump"
	$ns_ at 2.1 "$LSR3 lib-dump"
	
	$ns_ at 2.1 "$LSR4 pft-dump"
	$ns_ at 2.1 "$LSR4 erb-dump"
	$ns_ at 2.1 "$LSR4 lib-dump"

	$ns_ at 2.1 "$LSR5 pft-dump"
	$ns_ at 2.1 "$LSR5 erb-dump"
	$ns_ at 2.1 "$LSR5 lib-dump"

	$ns_ at 2.1 "$LSR6 pft-dump"
	$ns_ at 2.1 "$LSR6 erb-dump"
	$ns_ at 2.1 "$LSR6 lib-dump"

	$ns_ at 2.1 "$LSR7 pft-dump"
	$ns_ at 2.1 "$LSR7 erb-dump"
	$ns_ at 2.1 "$LSR7 lib-dump"

	$ns_ at 2.1 "$LSR8 pft-dump"
	$ns_ at 2.1 "$LSR8 erb-dump"
	$ns_ at 2.1 "$LSR8 lib-dump"
}

Class Test/control-driven -superclass TestSuite

Test/control-driven instproc init args {
	Classifier/Addr/MPLS set control_driven_ 1

	Agent/LDP set trace_ldp_ 1
	Classifier/Addr/MPLS set trace_mpls_ 1

	eval $self next $args
	$self instvar ns_

	$ns_ use-scheduler List

	$self openTrace 0.7
	$ns_ rtproto DV

	set node0  [$ns_ node]
	$ns_ node-config -MPLS ON
	set LSR1   [$ns_ node]
	set LSR2   [$ns_ node]
	set LSR3   [$ns_ node]
	set LSR4   [$ns_ node]
	set LSR5   [$ns_ node]
	set LSR6   [$ns_ node]
	set LSR7   [$ns_ node]
	$ns_ node-config -MPLS OFF
	set node8  [$ns_ node]

	$ns_ duplex-link $node0 $LSR1  1Mb  10ms DropTail
	$ns_ duplex-link $LSR1 $LSR2 1Mb 10ms DropTail
	$ns_ duplex-link $LSR1 $LSR3 1Mb 10ms DropTail
	$ns_ duplex-link $LSR2  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR4  $LSR6  1Mb  10ms DropTail
	$ns_ duplex-link $LSR6  $LSR7  1Mb  10ms DropTail
	$ns_ duplex-link $LSR3  $LSR5  1Mb  10ms DropTail
	$ns_ duplex-link $LSR3  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR7  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR6  1Mb  10ms DropTail
	$ns_ duplex-link $LSR7  $node8 1Mb  10ms DropTail

	$ns_ duplex-link-op $node0 $LSR1 orient right
	$ns_ duplex-link-op $LSR1 $LSR2 orient down
	$ns_ duplex-link-op $LSR1 $LSR3 orient right
	$ns_ duplex-link-op $LSR2  $LSR4 orient right
	$ns_ duplex-link-op $LSR4  $LSR6 orient right
	$ns_ duplex-link-op $LSR6  $LSR7  orient right-up
	$ns_ duplex-link-op $LSR3  $LSR5 orient right
	$ns_ duplex-link-op $LSR3  $LSR4 orient down
	$ns_ duplex-link-op $LSR5  $LSR7 orient right
	$ns_ duplex-link-op $LSR5  $LSR6 orient down
	$ns_ duplex-link-op $LSR7  $node8 orient right

	#
	# configure ldp agents on all mpls nodes
	#
	for {set i 1} {$i < 8} {incr i} {
		set a LSR$i
		for {set j [expr $i+1]} {$j < 8} {incr j} {
			set b LSR$j
			eval $ns_ LDP-peer $$a $$b
		}
		set m [eval $$a get-module "MPLS"]
		$m enable-reroute "drop"
	}

	#
	# set ldp-message clolr
	#
	$ns_ ldp-request-color       blue
	$ns_ ldp-mapping-color       red
	$ns_ ldp-withdraw-color      magenta
	$ns_ ldp-release-color       orange
	$ns_ ldp-notification-color  yellow

	#Create a traffic sink and attach it to the node node8
	$self instvar sink0_
	set sink0_ [new Agent/LossMonitor]
	$ns_ attach-agent $node8 $sink0_

	# Create a traffic source
	set src0 [$self attach-expoo-traffic $node0 $sink0_ 200 0 0 400k]
	$ns_ at 0.0 "$self record"
	$ns_ at 0.1 "$src0 start"
	$ns_ rtmodel-at 0.3 down $LSR3 $LSR5
	$ns_ rtmodel-at 0.5 up   $LSR3 $LSR5
	$ns_ at 0.6 "$src0 stop"
}

Class Test/data-driven -superclass TestSuite

Test/data-driven instproc init args {
	eval $self next $args
	$self instvar ns_

	Agent/LDP set trace_ldp_ 1
	Classifier/Addr/MPLS set trace_mpls_ 1

	$ns_ use-scheduler List

	$self openTrace 0.7
	$ns_ rtproto DV

	set node0  [$ns_ node]
	$ns_ node-config -MPLS ON
	set LSR1   [$ns_ node]
	set LSR2   [$ns_ node]
	set LSR3   [$ns_ node]
	set LSR4   [$ns_ node]
	set LSR5   [$ns_ node]
	set LSR6   [$ns_ node]
	set LSR7   [$ns_ node]
	$ns_ node-config -MPLS OFF
	set node8  [$ns_ node]

	$ns_ duplex-link $node0 $LSR1  1Mb  10ms DropTail
	$ns_ duplex-link $LSR1  $LSR2  1Mb  10ms DropTail
	$ns_ duplex-link $LSR2  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR4  $LSR6  1Mb  10ms DropTail
	$ns_ duplex-link $LSR6  $LSR7  1Mb  10ms DropTail
	$ns_ duplex-link $LSR1  $LSR3  1Mb  10ms DropTail
	$ns_ duplex-link $LSR3  $LSR5  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR7  1Mb  10ms DropTail
	$ns_ duplex-link $LSR3  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR6  1Mb  10ms DropTail
	$ns_ duplex-link $LSR7  $node8 1Mb  10ms DropTail

	#
	# configure ldp agents on all mpls nodes
	#
	for {set i 1} {$i < 8} {incr i} {
		set a LSR$i
		for {set j [expr $i+1]} {$j < 8} {incr j} {
			set b LSR$j
			eval $ns_ LDP-peer $$a $$b
		}
		set m [eval $$a get-module "MPLS"]
		$m enable-reroute "new"
	}

	#
	# set ldp-message clolr
	#
	$ns_ ldp-request-color       blue
	$ns_ ldp-mapping-color       red
	$ns_ ldp-withdraw-color      magenta
	$ns_ ldp-release-color       orange
	$ns_ ldp-notification-color  yellow

	#
	# set ldp events
	#
	Classifier/Addr/MPLS enable-on-demand
	Classifier/Addr/MPLS enable-ordered-control
	
	[$LSR1 get-module "MPLS"] enable-data-driven
	[$LSR3 get-module "MPLS"] enable-data-driven

	# Create a traffic sink and attach it to the node node8
	$self instvar sink0_
	set sink0_ [new Agent/LossMonitor]
	$ns_ attach-agent $node8 $sink0_
	# Create a traffic source
	set src0 [$self attach-expoo-traffic $node0 $sink0_ 200 0 0 400k]

	$ns_ at 0.0 "$self record"
	$ns_ at 0.1 "$src0 start"
	$ns_ rtmodel-at 0.3 down $LSR3 $LSR5
	$ns_ rtmodel-at 0.5 up   $LSR3 $LSR5
	$ns_ at 0.6 "$src0 stop"
}

Class Test/reroute -superclass TestSuite

Test/reroute instproc init args {
	eval $self next $args
	$self instvar ns_

	Agent/LDP set trace_ldp_ 1
	Classifier/Addr/MPLS set trace_mpls_ 1

	$ns_ use-scheduler List

	$self openTrace 0.7
	$ns_ rtproto DV

	set node0  [$ns_ node]
	$ns_ node-config -MPLS ON
	set LSR1   [$ns_ node]
	set LSR2   [$ns_ node]
	set LSR3   [$ns_ node]
	set LSR4   [$ns_ node]
	set LSR5   [$ns_ node]
	set LSR6   [$ns_ node]
	set LSR7   [$ns_ node]
	$ns_ node-config -MPLS OFF
	set node8  [$ns_ node]

	$ns_ duplex-link $node0 $LSR1  1Mb  10ms DropTail
	$ns_ duplex-link $LSR1  $LSR2  1Mb  10ms DropTail
	$ns_ duplex-link $LSR2  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR4  $LSR6  1Mb  10ms DropTail
	$ns_ duplex-link $LSR6  $LSR7  1Mb  10ms DropTail
	$ns_ duplex-link $LSR1  $LSR3  1Mb  10ms DropTail
	$ns_ duplex-link $LSR3  $LSR5  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR7  1Mb  10ms DropTail
	$ns_ duplex-link $LSR3  $LSR4  1Mb  10ms DropTail
	$ns_ duplex-link $LSR5  $LSR6  1Mb  10ms DropTail
	$ns_ duplex-link $LSR7  $node8 1Mb  10ms DropTail

	#
	# configure ldp agents on all mpls nodes
	#
	for {set i 1} {$i < 8} {incr i} {
		set a LSR$i
		for {set j [expr $i+1]} {$j < 8} {incr j} {
			set b LSR$j
			eval $ns_ LDP-peer $$a $$b
		}
		set m [eval $$a get-module "MPLS"]
		$m enable-reroute "new"
	}

	#
	# set ldp-message clolr
	#
	$ns_ ldp-request-color       blue
	$ns_ ldp-mapping-color       red
	$ns_ ldp-withdraw-color      magenta
	$ns_ ldp-release-color       orange
	$ns_ ldp-notification-color  yellow

	Classifier/Addr/MPLS enable-on-demand
	Classifier/Addr/MPLS enable-ordered-control
	[$LSR1 get-module "MPLS"] enable-data-driven
	[$LSR3 get-module "MPLS"] enable-data-driven

	# Create a traffic sink and attach it to the node node8
	$self instvar sink0_
	set sink0_ [new Agent/LossMonitor]
	$ns_ attach-agent $node8  $sink0_
	# Create a traffic source
	set src0 [$self attach-expoo-traffic $node0 $sink0_ 200 0 0 400k]

	$ns_ at 00 "$self record"
	$ns_ at 0.1  "$src0 start"
	$ns_ at 0.1  "[$LSR1 get-module MPLS] make-explicit-route 7 2_4_6_7 1000 -1"
	$ns_ at 0.2  "[$LSR7 get-module MPLS] make-explicit-route  7  5_3_1_1000  1005  -1"
	$ns_ at 0.3  "[$LSR1 get-module MPLS] reroute-binding      8 -1    1005"
	$ns_ at 0.3  "[$LSR3 get-module MPLS] reroute-binding      8 -1    1005"
	$ns_ at 0.3  "[$LSR5 get-module MPLS] reroute-binding      8 -1    1005"
	$ns_ rtmodel-at 0.3 down $LSR3 $LSR5
	$ns_ rtmodel-at 0.5 up   $LSR3 $LSR5
	$ns_ at 0.6 "$src0 stop"
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
