#
# Copyright (C) 2000 by USC/ISI
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-linkstate.tcl,v 1.15 2006/01/24 23:00:06 sallyfloyd Exp $

# Simple test for Link State routing contributed by 
# Mingzhou Sun <msun@rainfinity.com> based on Kannan's old equal-cost 
# multi-path routing test code.

# Simple example of an equal cost multi-path routing through
# two equal cost routes.  Equal cost paths are achieved by diddling
# link costs.
#
#
#		$n0       $n3
#		   \     /   \
#		    \   /     \
#		     $n2-------$n4
#		    /
#		   /
#		$n1
#
# However, this is not as simple.  Because $n2 is directly connected to $n4,
# it prefers its ``Direct'' route over multiple equal cost routes learned
# via DV.  Hence,we raise the preference of Direct routes over DV routes.
#
# Furthermore, in this example, link <$n2, $n4> is made dynamic.  This allows
# us to watch traffic between $n2 and $n4 alternate between taking multiple
# equi-cost routes, and the only available route.
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP rtProtoLS ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
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
Agent/TCP set SetCWRonRetransmit_ true
# Changing the default value.

if {![TclObject is-class Agent/rtProto/LS]} {
	puts "Linkstate module is not present; validation skipped"
	exit 2
}

Class TestSuite

Class Test/eqp -superclass TestSuite

Agent/rtProto/Direct set preference_ 200

Test/eqp instproc init {} {
	$self instvar ns
	set ns [new Simulator]
	Node set multiPath_ 1
	set n0 [$ns node]
	set n1 [$ns node]
	set n2 [$ns node]
	set n3 [$ns node]
	set n4 [$ns node]
	$n0 shape "circle"
	$n1 shape "circle"
	$n2 shape "other"
	$n3 shape "other"
	$n4 shape "box"

	set f [open temp.rands w]
	$ns trace-all $f

	global quiet
	if { $quiet == "false" } {
		set nf [open eqp.nam w]
		$ns namtrace-all $nf
	}

	$ns color 0 blue
	$ns color 1 red
	$ns color 2 white

	$ns duplex-link $n0 $n2 10Mb 2ms DropTail
	$ns duplex-link $n1 $n2 10Mb 2ms DropTail
	$ns duplex-link-op $n0 $n2 orient right-down
	$ns duplex-link-op $n1 $n2 orient right-up

	$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail
	$ns duplex-link $n3 $n4 1.5Mb 10ms DropTail
	$ns queue-limit $n2 $n3 5
	$ns duplex-link-op $n2 $n3 orient right-up
	$ns duplex-link-op $n3 $n4 orient right-down
	$ns duplex-link-op $n2 $n3 queuePos 0

	$ns duplex-link $n2 $n4 1.5Mb 10ms DropTail
	$ns queue-limit $n2 $n4 5
	$ns duplex-link-op $n2 $n4 orient right
	
	$ns duplex-link-op $n2 $n3 queuePos 0
	$ns duplex-link-op $n2 $n4 queuePos 0
	
	[$ns link $n2 $n4] cost 2
	[$ns link $n4 $n2] cost 2

	[$self build-tcp $n0 $n4 0.7] set class_ 0
	[$self build-tcp $n1 $n4 0.9] set class_ 1

	$ns rtmodel Deterministic {.35 .25} $n2 $n4
	[$ns link $n2 $n4] trace-dynamics $ns stdout

	$ns rtproto LS
}

Test/eqp instproc build-tcp { n0 n1 startTime } {
	$self instvar ns
	set tcp [new Agent/TCP]
	$ns attach-agent $n0 $tcp
	set snk [new Agent/TCPSink]
	$ns attach-agent $n1 $snk

	$ns connect $tcp $snk

	set ftp [new Application/FTP]
	$ftp attach-agent $tcp
	$ns at $startTime "$ftp start"
	return $tcp
}

Test/eqp instproc finish {} {
	$self instvar ns
	$ns flush-trace
	exit 0
}

Test/eqp instproc run {} {
	$self instvar ns
	$ns at 1.2 "$self finish"
	$ns run
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests>"
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
		}
		2 {
			set test [lindex $argv 0]
			isProc? Test $test
			if {[lindex $argv 1] == "QUIET"} {
				set quiet true
			} 
		}
		default {
			usage
		}
	}
	set t [new Test/$test]
	$t run
}

TestSuite runTest
