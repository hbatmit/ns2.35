#
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set SetCWRonRetransmit_ true
# Changing the default value.
#
# Copyright (c) 1999 Regents of the University of California.
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
#       This product includes software developed by the MASH Research
#       Group at the University of California Berkeley.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-satellite.tcl,v 1.12 2006/01/24 23:00:07 sallyfloyd Exp $
#
# Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
#    
# This test suite is for validating the satellite extensions
#
# To run all tests:  test-all-satellite
#
# To run individual tests:
# ns test-suite-satellite.tcl mixed 
# ns test-suite-satellite.tcl mixed.legacy 
# ns test-suite-satellite.tcl repeater
# ns test-suite-satellite.tcl aloha
# ns test-suite-satellite.tcl aloha.collisions
# ns test-suite-satellite.tcl wired 
#
#
# To view a list of available tests to run with this script:
# ns test-suite-satellite.tcl
#
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

# Some OTcl bound variables
HandoffManager/Term set elevation_mask_ 8.2
HandoffManager/Term set term_handoff_int_ 10
HandoffManager set handoff_randomization_ "false"
Mac/Sat/UnslottedAloha set mean_backoff_ 1s 
Mac/Sat/UnslottedAloha set rtx_limit_ 3
Mac/Sat/UnslottedAloha set send_timeout_ 270ms

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ test_ topo_ node_ 
	set ns_ [new Simulator]
	$ns_ rtproto Dummy

	global f
	set f [open temp.rands w]
	$ns_ trace-all $f

}

TestSuite instproc finish args {
	$self instvar ns_
	
	$ns_ flush-trace
	exit 0
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
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
	global argc argv

	switch $argc {
		1 {
			set test $argv
			isProc? Test $test

		}
		2 {
			set test [lindex $argv 0]
			isProc? Test $test
			set a [lindex $argv 1]
		}
		default {
			usage
		}
	}
	set t [new Test/$test]
	$t run
}

# Global configuration parameters

global opt
set opt(chan)           Channel/Sat
set opt(bw_up)          2Mb; # Uplink bandwidth
set opt(bw_down)        2Mb; # Downlink bandwidth
set opt(phy)            Phy/Sat
set opt(mac)            Mac/Sat
set opt(ifq)            Queue/DropTail
set opt(qlim)           50
set opt(ll)             LL/Sat

# Definition of test-suite tests
# Simple bent-pipe (repeater) satellite
# NOTE:  This test is identical to sat-repeater.tcl in ~ns/tcl/ex

Class Test/repeater -superclass TestSuite
Test/repeater instproc init {} {
	$self instvar test_ 
	set test_	repeater
	$self next
}

Test/repeater instproc run {} {
	$self instvar ns_ node_ 

	global opt f

	set opt(wiredRouting)   OFF

	# GEO satellite at 95 degrees longitude West
	$ns_ node-config -satNodeType geo-repeater \
			-llType $opt(ll) \
			-ifqType $opt(ifq) \
			-ifqLen $opt(qlim) \
			-macType $opt(mac) \
			-phyType $opt(phy) \
			-channelType $opt(chan) \
			-downlinkBW $opt(bw_down) \
			-wiredRouting $opt(wiredRouting)

	set n1 [$ns_ node]
	$n1 set-position -95

	# Two terminals: one in NY and one in SF 
	$ns_ node-config -satNodeType terminal
	set n2 [$ns_ node] 
	$n2 set-position 40.9 -73.9; # NY
	set n3 [$ns_ node] 
	$n3 set-position 37.8 -122.4; # SF

	# Add GSLs to geo satellites
	$n2 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
            $opt(phy) [$n1 set downlink_] [$n1 set uplink_]
	$n3 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up)  \
	    $opt(phy) [$n1 set downlink_] [$n1 set uplink_]

	# Add an error model to the receiving terminal node
	set em_ [new ErrorModel]
	$em_ unit pkt 
	$em_ set rate_ 0.02
	$em_ ranvar [new RandomVariable/Uniform] 
	$n3 interface-errormodel $em_

	$ns_ trace-all-satlinks $f

	# Attach agents for CBR traffic generator
	set udp0 [new Agent/UDP] 
	$ns_ attach-agent $n2 $udp0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	$cbr0 set interval_ 6

	set null0 [new Agent/Null]
	$ns_ attach-agent $n3 $null0
 
	$ns_ connect $udp0 $null0

	# Attach agents for FTP
	set tcp1 [$ns_ create-connection TCP $n2 TCPSink $n3 0]
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 7.0 "$ftp1 produce 100"

	# We use centralized routing
	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	$ns_ at 1.0 "$cbr0 start"

	$ns_ at 100.0 "$self finish"   

	$ns_ run
}

# Testing a geo satellite and a plane of polar satellites
# NOTE:  This test is identical to sat-mixed.tcl in ~ns/tcl/ex

Class Test/mixed -superclass TestSuite
Test/mixed instproc init {} {
	$self instvar test_
	set test_       mixed 
	$self next

}

Test/mixed instproc run {} {
	$self instvar ns_  

	global opt f
	
	# Change some of the options
	set opt(bw_down)        1.5Mb
	set opt(bw_up)          1.5Mb
	set opt(bw_isl)         25Mb

	set opt(alt)            780
	set opt(inc)            90
	set opt(wiredRouting)   OFF

	$ns_ node-config -satNodeType polar \
			 -llType $opt(ll) \
			 -ifqType $opt(ifq) \
			 -ifqLen $opt(qlim) \
			 -macType $opt(mac) \
			 -phyType $opt(phy) \
			 -channelType $opt(chan) \
			 -downlinkBW $opt(bw_down) \
			 -wiredRouting $opt(wiredRouting)

	set n0 [$ns_ node]; set n1 [$ns_ node]; set n2 [$ns_ node];
	set n3 [$ns_ node]; set n4 [$ns_ node]; set n5 [$ns_ node];
	set n6 [$ns_ node]; set n7 [$ns_ node]; set n8 [$ns_ node];
	set n9 [$ns_ node]; set n10 [$ns_ node]

	set plane 1
	$n0 set-position $opt(alt) $opt(inc) 0 0 $plane 
	$n1 set-position $opt(alt) $opt(inc) 0 32.73 $plane 
	$n2 set-position $opt(alt) $opt(inc) 0 65.45 $plane 
	$n3 set-position $opt(alt) $opt(inc) 0 98.18 $plane 
	$n4 set-position $opt(alt) $opt(inc) 0 130.91 $plane 
	$n5 set-position $opt(alt) $opt(inc) 0 163.64 $plane 
	$n6 set-position $opt(alt) $opt(inc) 0 196.36 $plane 
	$n7 set-position $opt(alt) $opt(inc) 0 229.09 $plane 
	$n8 set-position $opt(alt) $opt(inc) 0 261.82 $plane 
	$n9 set-position $opt(alt) $opt(inc) 0 294.55 $plane 
	$n10 set-position $opt(alt) $opt(inc) 0 327.27 $plane 

	# By setting the next_ variable on polar sats; handoffs can be optimized
	# This step must follow all polar node creation
	$n0 set_next $n10; $n1 set_next $n0; $n2 set_next $n1; $n3 set_next $n2
	$n4 set_next $n3; $n5 set_next $n4; $n6 set_next $n5; $n7 set_next $n6
	$n8 set_next $n7; $n9 set_next $n8; $n10 set_next $n9

	# GEO satellite:  above North America
	$ns_ node-config -satNodeType geo
	set n11 [$ns_ node]
	$n11 set-position -100

	# Terminals
	$ns_ node-config -satNodeType terminal
	set n100 [$ns_ node]; set n101 [$ns_ node]
	$n100 set-position 37.9 -122.3; # Berkeley
	$n101 set-position 42.3 -71.1; # Boston
	set n200 [$ns_ node]; set n201 [$ns_ node]
	$n200 set-position 0 10
	$n201 set-position 0 -10

	# Add any necessary ISLs or GSLs
	# GSLs to the geo satellite:
	$n100 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
	  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
	$n101 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
	  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
	# Attach n200 and n201 
	$n200 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) $opt(phy) [$n5 set downlink_] [$n5 set uplink_]
	$n201 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) $opt(phy) [$n5 set downlink_] [$n5 set uplink_]

	#ISL
	$ns_ add-isl intraplane $n0 $n1 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n1 $n2 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n2 $n3 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n3 $n4 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n4 $n5 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n5 $n6 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n6 $n7 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n7 $n8 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n8 $n9 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n9 $n10 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n10 $n0 $opt(bw_isl) $opt(ifq) $opt(qlim)

	# Trace all queues
	$ns_ trace-all-satlinks $f

	# Attach agents
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n100 $udp0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	$cbr0 set interval_ 60.01

	set udp1 [new Agent/UDP]
	$ns_ attach-agent $n200 $udp1
	$udp1 set class_ 1
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1
	$cbr1 set interval_ 90.5

	set null0 [new Agent/Null]
	$ns_ attach-agent $n101 $null0
	set null1 [new Agent/Null]
	$ns_ attach-agent $n201 $null1

	$ns_ connect $udp0 $null0
	$ns_ connect $udp1 $null1

	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	$ns_ at 1.0 "$cbr0 start"
	$ns_ at 305.0 "$cbr1 start"

	$ns_ at 9000.0 "$self finish"

	$ns_ run
}

# Same as mixed, but with wired-satellite integration
# NOTE:  This test is identical to sat-wired.tcl in ~ns/tcl/ex

Class Test/wired -superclass TestSuite
Test/wired instproc init {} {
	$self instvar test_
	set test_       wired 
	$self next

}

Test/wired instproc run {} {
	$self instvar ns_  

	global opt f
	
	# Change some of the options
	set opt(bw_down)        1.5Mb
	set opt(bw_up)          1.5Mb
	set opt(bw_isl)         25Mb

	set opt(alt)            780
	set opt(inc)            90
	set opt(wiredRouting) 	ON

	$ns_ node-config -satNodeType polar \
			 -llType $opt(ll) \
			 -ifqType $opt(ifq) \
			 -ifqLen $opt(qlim) \
			 -macType $opt(mac) \
			 -phyType $opt(phy) \
			 -channelType $opt(chan) \
			 -downlinkBW $opt(bw_down) \
			 -wiredRouting $opt(wiredRouting)

	set n0 [$ns_ node]; set n1 [$ns_ node]; set n2 [$ns_ node];
	set n3 [$ns_ node]; set n4 [$ns_ node]; set n5 [$ns_ node];
	set n6 [$ns_ node]; set n7 [$ns_ node]; set n8 [$ns_ node];
	set n9 [$ns_ node]; set n10 [$ns_ node]

	set plane 1
	$n0 set-position $opt(alt) $opt(inc) 0 0 $plane 
	$n1 set-position $opt(alt) $opt(inc) 0 32.73 $plane 
	$n2 set-position $opt(alt) $opt(inc) 0 65.45 $plane 
	$n3 set-position $opt(alt) $opt(inc) 0 98.18 $plane 
	$n4 set-position $opt(alt) $opt(inc) 0 130.91 $plane 
	$n5 set-position $opt(alt) $opt(inc) 0 163.64 $plane 
	$n6 set-position $opt(alt) $opt(inc) 0 196.36 $plane 
	$n7 set-position $opt(alt) $opt(inc) 0 229.09 $plane 
	$n8 set-position $opt(alt) $opt(inc) 0 261.82 $plane 
	$n9 set-position $opt(alt) $opt(inc) 0 294.55 $plane 
	$n10 set-position $opt(alt) $opt(inc) 0 327.27 $plane 

	# By setting the next_ variable on polar sats; handoffs can be optimized
	# This step must follow all polar node creation
	$n0 set_next $n10; $n1 set_next $n0; $n2 set_next $n1; $n3 set_next $n2
	$n4 set_next $n3; $n5 set_next $n4; $n6 set_next $n5; $n7 set_next $n6
	$n8 set_next $n7; $n9 set_next $n8; $n10 set_next $n9

	# GEO satellite:  above North America
	$ns_ node-config -satNodeType geo
	set n11 [$ns_ node]
	$n11 set-position -100

	# Terminals
	$ns_ node-config -satNodeType terminal
	set n100 [$ns_ node]; set n101 [$ns_ node]
	$n100 set-position 37.9 -122.3; # Berkeley
	$n101 set-position 42.3 -71.1; # Boston
	set n200 [$ns_ node]; set n201 [$ns_ node]
	$n200 set-position 0 10
	$n201 set-position 0 -10

	# Add any necessary ISLs or GSLs
	# GSLs to the geo satellite:
	$n100 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
	  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
	$n101 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
	  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
	# Attach n200 and n201 
	$n200 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) $opt(phy) [$n5 set downlink_] [$n5 set uplink_]
	$n201 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) $opt(phy) [$n5 set downlink_] [$n5 set uplink_]

	#ISL
	$ns_ add-isl intraplane $n0 $n1 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n1 $n2 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n2 $n3 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n3 $n4 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n4 $n5 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n5 $n6 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n6 $n7 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n7 $n8 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n8 $n9 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n9 $n10 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n10 $n0 $opt(bw_isl) $opt(ifq) $opt(qlim)

	# Set up wired nodes
	# Connect $n300<->$n301<->$n302<->$n100<->$n11<->$n101<->$n303
	#                    ^               ^
	#                    |_______________|
	#
	# Packets from n303 to n300 should bypass n302 (node #18 in the trace)
	# (i.e., they should take the following path:  19,13,11,12,17,16)
	#
	$ns_ unset satNodeType_
	set n300 [$ns_ node]; # node 16 in trace
	set n301 [$ns_ node]; # node 17 in trace
	set n302 [$ns_ node]; # node 18 in trace
	set n303 [$ns_ node]; # node 19 in trace
	$ns_ duplex-link $n300 $n301 5Mb 2ms DropTail; # 16 <-> 17
	$ns_ duplex-link $n301 $n302 5Mb 2ms DropTail; # 17 <-> 18
	$ns_ duplex-link $n302 $n100 5Mb 2ms DropTail; # 18 <-> 11
	$ns_ duplex-link $n303 $n101 5Mb 2ms DropTail; # 19 <-> 13
	$ns_ duplex-link $n301 $n100 5Mb 2ms DropTail; # 17 <-> 11



	# Trace all queues
	$ns_ trace-all-satlinks $f

	# Attach agents
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n100 $udp0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	$cbr0 set interval_ 60.01

	set udp1 [new Agent/UDP]
	$ns_ attach-agent $n200 $udp1
	$udp1 set class_ 1
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1
	$cbr1 set interval_ 90.5

	set null0 [new Agent/Null]
	$ns_ attach-agent $n101 $null0
	set null1 [new Agent/Null]
	$ns_ attach-agent $n201 $null1

	$ns_ connect $udp0 $null0
	$ns_ connect $udp1 $null1

	# Set up connection between wired nodes
	set udp2 [new Agent/UDP]
	$ns_ attach-agent $n303 $udp2
	set cbr2 [new Application/Traffic/CBR]
	$cbr2 attach-agent $udp2
	$cbr2 set interval_ 300
	set null2 [new Agent/Null]
	$ns_ attach-agent $n300 $null2

	$ns_ connect $udp2 $null2
	$ns_ at 10.0 "$cbr2 start"
 
	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	$ns_ at 1.0 "$cbr0 start"
	$ns_ at 305.0 "$cbr1 start"

	$ns_ at 9000.0 "$self finish"

	$ns_ run
}

# Testing unslotted aloha
# bent-pipe gka geo satellite and a plane of polar satellites
# NOTE:  This test is similar (fewer sources) to sat-aloha.tcl in ~ns/tcl/ex

Class Test/aloha -superclass TestSuite
Test/aloha instproc init {} {
	$self instvar test_
	set test_ 	aloha	
	$self next
}

Test/aloha instproc run {} {
    $self instvar ns_  
	
	global opt f 

	Mac/Sat set trace_drops_ false
	Mac/Sat set trace_collisions_ false

	set opt(bw_up)          2Mb
	set opt(bw_down)        2Mb
	set opt(mac)            Mac/Sat/UnslottedAloha 
	set opt(wiredRouting)   OFF

	$ns_ node-config -satNodeType geo-repeater \
			-llType $opt(ll) \
			-ifqType $opt(ifq) \
			-ifqLen $opt(qlim) \
			-macType $opt(mac) \
			-phyType $opt(phy) \
			-channelType $opt(chan) \
			-downlinkBW $opt(bw_down) \
			-wiredRouting $opt(wiredRouting)

	# GEO satellite at prime meridian
	set n1 [$ns_ node]
	$n1 set-position 0

	# Place 30 nodes at 30 different locations
	$ns_ node-config -satNodeType terminal
	set num_nodes           30
	for {set a 1} {$a <= $num_nodes} {incr a} {
		set n($a) [$ns_ node]
        	$n($a) set-position [expr -15 + $a ] [expr 15 - $a ]
        	$n($a) add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) \
		    $opt(bw_up) $opt(phy) [$n1 set downlink_] [$n1 set uplink_]
	}

	for {set a 1} {$a <= $num_nodes} {incr a} {
        	set b [expr int($a + (0.5 * $num_nodes))]
        	if {$b > 30} {
                	incr b -30 
        	}

        	set udp($a) [new Agent/UDP]
        	$ns_ attach-agent $n($a) $udp($a)
        	set exp($a) [new Application/Traffic/Exponential]
        	$exp($a) attach-agent $udp($a)
        	$exp($a) set rate_ 3Kb

        	set null($a) [new Agent/Null]
        	$ns_ attach-agent $n($b) $null($a)

        	$ns_ connect $udp($a) $null($a)
        	$ns_ at 1.0 "$exp($a) start"
	}

	$ns_ trace-all-satlinks $f

	# We use centralized routing
	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	$ns_ at 50.0 "$self finish"

	$ns_ run
}

Class Test/aloha.collisions -superclass TestSuite
Test/aloha.collisions instproc init {} {
	$self instvar test_
	set test_ 	aloha	
	$self next
}

Test/aloha.collisions instproc run {} {
    $self instvar ns_  
	
	global opt f 

	Mac/Sat set trace_drops_ true
	Mac/Sat set trace_collisions_ true

	set opt(bw_up)          2Mb
	set opt(bw_down)        2Mb
	set opt(mac)            Mac/Sat/UnslottedAloha 
	set opt(wiredRouting)   OFF

	$ns_ node-config -satNodeType geo-repeater \
			-llType $opt(ll) \
			-ifqType $opt(ifq) \
			-ifqLen $opt(qlim) \
			-macType $opt(mac) \
			-phyType $opt(phy) \
			-channelType $opt(chan) \
			-downlinkBW $opt(bw_down) \
			-wiredRouting $opt(wiredRouting)

	# GEO satellite at prime meridian
	set n1 [$ns_ node]
	$n1 set-position 0

	# Place 30 nodes at 30 different locations
	$ns_ node-config -satNodeType terminal
	set num_nodes           30
	for {set a 1} {$a <= $num_nodes} {incr a} {
		set n($a) [$ns_ node]
        	$n($a) set-position [expr -15 + $a ] [expr 15 - $a ]
        	$n($a) add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) \
		    $opt(bw_up) $opt(phy) [$n1 set downlink_] [$n1 set uplink_]
	}

	for {set a 1} {$a <= $num_nodes} {incr a} {
        	set b [expr int($a + (0.5 * $num_nodes))]
        	if {$b > 30} {
                	incr b -30 
        	}

        	set udp($a) [new Agent/UDP]
        	$ns_ attach-agent $n($a) $udp($a)
        	set exp($a) [new Application/Traffic/Exponential]
        	$exp($a) attach-agent $udp($a)
        	$exp($a) set rate_ 3Kb

        	set null($a) [new Agent/Null]
        	$ns_ attach-agent $n($b) $null($a)

        	$ns_ connect $udp($a) $null($a)
        	$ns_ at 1.0 "$exp($a) start"
	}

	$ns_ trace-all-satlinks $f

	# We use centralized routing
	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	$ns_ at 50.0 "$self finish"

	$ns_ run
}

# Backward compatibility syntax (legacy code) for "mixed"
Class Test/mixed.legacy -superclass TestSuite
Test/mixed.legacy instproc init {} {
	$self instvar test_
	set test_       mixed.legacy 
	$self next

}

Test/mixed.legacy instproc run {} {
	$self instvar ns_  

	global opt f
	
	# Change some of the options
	set opt(bw_down)        1.5Mb
	set opt(bw_up)          1.5Mb
	set opt(bw_isl)         25Mb
	set opt(mac)            Mac/Sat 

	set opt(alt)            780
	set opt(inc)            90

	set linkargs "$opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_down) $opt(phy)"
	set alt $opt(alt)
	set inc $opt(inc)
	set chan $opt(chan)
	set plane 1
	set n0 [$ns_ satnode-polar $alt $inc 0 0 $plane $linkargs $chan]
	set n1 [$ns_ satnode-polar $alt $inc 0 32.73 $plane $linkargs $chan]
	set n2 [$ns_ satnode-polar $alt $inc 0 65.45 $plane $linkargs $chan]
	set n3 [$ns_ satnode-polar $alt $inc 0 98.18 $plane $linkargs $chan]
	set n4 [$ns_ satnode-polar $alt $inc 0 130.91 $plane $linkargs $chan]
	set n5 [$ns_ satnode-polar $alt $inc 0 163.64 $plane $linkargs $chan]
	set n6 [$ns_ satnode-polar $alt $inc 0 196.36 $plane $linkargs $chan]
	set n7 [$ns_ satnode-polar $alt $inc 0 229.09 $plane $linkargs $chan]
	set n8 [$ns_ satnode-polar $alt $inc 0 261.82 $plane $linkargs $chan]
	set n9 [$ns_ satnode-polar $alt $inc 0 294.55 $plane $linkargs $chan]
	set n10 [$ns_ satnode-polar $alt $inc 0 327.27 $plane $linkargs $chan]

	# By setting the next_ variable on polar sats; handoffs can be optimized
	# This step must follow all polar node creation
	$n0 set_next $n10; $n1 set_next $n0; $n2 set_next $n1; $n3 set_next $n2
	$n4 set_next $n3; $n5 set_next $n4; $n6 set_next $n5; $n7 set_next $n6
	$n8 set_next $n7; $n9 set_next $n8; $n10 set_next $n9

	# GEO satellite:  above North America
	set n11 [$ns_ satnode-geo -100 $linkargs $opt(chan)]

	# Terminals
	set n100 [$ns_ satnode-terminal 37.9 -122.3]; # Berkeley
	set n101 [$ns_ satnode-terminal 42.3 -71.1]; # Boston
	set n200 [$ns_ satnode-terminal 0 10]
	set n201 [$ns_ satnode-terminal 0 -10]

	# Add any necessary ISLs or GSLs
	# GSLs to the geo satellite:
	$n100 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
	  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
	$n101 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
	  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
	# Attach n200 and n201 
	$n200 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) $opt(phy) [$n5 set downlink_] [$n5 set uplink_]
	$n201 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) $opt(phy) [$n5 set downlink_] [$n5 set uplink_]

	#ISL
	$ns_ add-isl intraplane $n0 $n1 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n1 $n2 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n2 $n3 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n3 $n4 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n4 $n5 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n5 $n6 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n6 $n7 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n7 $n8 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n8 $n9 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n9 $n10 $opt(bw_isl) $opt(ifq) $opt(qlim)
	$ns_ add-isl intraplane $n10 $n0 $opt(bw_isl) $opt(ifq) $opt(qlim)

	# Trace all queues
	$ns_ trace-all-satlinks $f

	# Attach agents
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n100 $udp0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	$cbr0 set interval_ 60.01

	set udp1 [new Agent/UDP]
	$ns_ attach-agent $n200 $udp1
	$udp1 set class_ 1
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1
	$cbr1 set interval_ 90.5

	set null0 [new Agent/Null]
	$ns_ attach-agent $n101 $null0
	set null1 [new Agent/Null]
	$ns_ attach-agent $n201 $null1

	$ns_ connect $udp0 $null0
	$ns_ connect $udp1 $null1

	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	$ns_ at 1.0 "$cbr0 start"
	$ns_ at 305.0 "$cbr1 start"

	$ns_ at 9000.0 "$self finish"

	$ns_ run
}

TestSuite runTest

