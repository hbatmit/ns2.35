# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-09-07 12:53:04 haoboy>
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-webtraf.tcl,v 1.10 2006/01/24 23:00:08 sallyfloyd Exp $

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  HttpInval ; # hdrs reqd for validation test

# UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.

Class TestSuite

TestSuite instproc init { name } {
	$self instvar ns_ totalpkt_ testname_
	set testname_ $name
	set ns_ [new Simulator]
	set totalpkt_ 0
}

TestSuite instproc finish args {
	$self instvar traceFile_ ns_
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

# Create $ns sessions, each of which has $np pages
# server mode flag: $sm
TestSuite instproc create-session { ns np sm } {
	# Polly's dumbbell topology

	$self instvar ns_

	set num_node 12
	for {set i 0} {$i < $num_node} {incr i} {
		set n($i) [$ns_ node]
	}
	$ns_ set src_ [list 2 3 4 5 6]
	$ns_ set dst_ [list 7 8 9 10 11]
	# EDGES (from-node to-node length a b):
	$ns_ duplex-link $n(0) $n(1) 1.5Mb 40ms DropTail
	$ns_ duplex-link $n(0) $n(2) 10Mb 20ms DropTail
	$ns_ duplex-link $n(0) $n(3) 10Mb 20ms DropTail
	$ns_ duplex-link $n(0) $n(4) 10Mb 20ms DropTail
	$ns_ duplex-link $n(0) $n(5) 10Mb 20ms DropTail
	$ns_ duplex-link $n(0) $n(6) 10Mb 20ms DropTail
	$ns_ duplex-link $n(1) $n(7) 10Mb 20ms DropTail
	$ns_ duplex-link $n(1) $n(8) 10Mb 20ms DropTail
	$ns_ duplex-link $n(1) $n(9) 10Mb 20ms DropTail
	$ns_ duplex-link $n(1) $n(10) 10Mb 20ms DropTail
	$ns_ duplex-link $n(1) $n(11) 10Mb 20ms DropTail

	$ns_ duplex-link-op $n(0) $n(1) orient left
	$ns_ duplex-link-op $n(0) $n(2) orient up
	$ns_ duplex-link-op $n(0) $n(3) orient right-up
	$ns_ duplex-link-op $n(0) $n(4) orient right
	$ns_ duplex-link-op $n(0) $n(5) orient right-down
	$ns_ duplex-link-op $n(0) $n(6) orient down
	$ns_ duplex-link-op $n(1) $n(7) orient up
	$ns_ duplex-link-op $n(1) $n(8) orient left-up
	$ns_ duplex-link-op $n(1) $n(9) orient left 
	$ns_ duplex-link-op $n(1) $n(10) orient left-down
	$ns_ duplex-link-op $n(1) $n(11) orient down

	# Set up server and client nodes
	set pool [new PagePool/WebTraf]
	# Set debug to produce detailed output
	$pool set debug_ 1

	$pool set resp_trace_ 0
	$pool set req_trace_ 0
	
	$pool set-num-client [llength [$ns_ set src_]]
	$pool set-num-server [llength [$ns_ set dst_]]

	set i 0
	foreach s [$ns_ set src_] {
		$pool set-client $i $n($s)
		incr i
	}
	set i 0
	foreach s [$ns_ set dst_] {
		$pool set-server $i $n($s)
		incr i
	}

	# Config server mode: 
	# 0: no server processing delay
	# 1: FCFS server scheduling policy
	# 2: STF server scheduling policy
	$pool set-server-mode $sm

	# Session 0 starts from 0.1s, session 1 starts from 0.2s
	$pool set-num-session $ns
	for {set i 0} {$i < $ns} {incr i} {
		set interPage [new RandomVariable/Exponential]
		$interPage set avg_ 1
		set pageSize [new RandomVariable/Constant]
		$pageSize set val_ $np
		set interObj [new RandomVariable/Exponential]
		$interObj set avg_ 0.01
		set objSize [new RandomVariable/ParetoII]
		$objSize set avg_ 10
		$objSize set shape_ 1.2
		$pool create-session $i $np 0.1 $interPage $pageSize \
				$interObj $objSize
		# from webcache/webtraf.cc
	}
}

# These tests only check the page scheduling mechanism of WebTrafPool.

Class Test/1s-1p -superclass TestSuite

Test/1s-1p instproc init args {
	eval $self next $args
	$self openTrace 100.0
	$self create-session 1 1 0
}

Class Test/1s-2p -superclass TestSuite

Test/1s-2p instproc init args {
	eval $self next $args
	$self openTrace 100.0
	$self create-session 1 2 0
}

Class Test/3s-2p -superclass TestSuite

Test/3s-2p instproc init args {
	eval $self next $args
	$self openTrace 100.0
	$self create-session 3 2 0
}

Class Test/fcfs -superclass TestSuite

Test/fcfs instproc init args {
	eval $self next $args
	$self openTrace 100.0
	$self create-session 3 2 1
}

Class Test/stf -superclass TestSuite

Test/stf instproc init args {
	eval $self next $args
	$self openTrace 100.0
	$self create-session 3 2 2
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
