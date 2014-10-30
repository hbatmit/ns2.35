#! /bin/sh
#
# Copyright (c) 2000 The Regents of the University of California.
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
#	This product includes software developed by the Network Research
#	Group at Lawrence Berkeley National Laboratory.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-plm.tcl,v 1.6 2005/06/11 05:51:37 sfloyd Exp $
#
# Contributed by Arnaud Legout at EURECOM

#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP RTP TCP LRWPAN ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

set packetSize 500
set runtime 80
set plm_debug_flag 3; #from 0 to 3: 0 no output, 3 full output
set rates "20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3"
set level [llength $rates]
set rates_cum [calc_cum $rates]
#puts stderr $rates_cum
set run_nam 0

set check_estimate 1
set Queue_sched_ FQ
set PP_burst_length 2
set PP_estimation_length 3


Class Test/PLM -superclass PLMTopology

#This scenario is for validation purpose. It aims to trigger most of
#the PLM functionalities.
Test/PLM instproc init args {

	$self instvar ns node
	set ns [new Simulator -multicast on]
	$ns color 1 blue
	$ns color 2 green
	$ns color 3 red
	$ns color 4 white
	# prunes, grafts
	$ns color 30 orange
	$ns color 31 yellow

	global f check_estimate nb_plm runtime
    
	set f [open temp.rands w]
	$ns trace-all $f
	$ns at [expr $runtime +1] "$self finish"

	eval $self next $ns
	set nb_src 20
	Queue/DropTail set limit_ 5
 
	$self build_link 0 1 200ms 200e3
	$ns duplex-link-op $node(0) $node(1) queuePos 0.5
  
	#create sources links
	for {set i 2} {$i<=[expr $nb_src + 1]} {incr i} {
		set delay [expr 5 * $i]ms
		set bp ${i}e5
		#	puts stderr "$delay $bp"
		$self build_link $i 0 $delay  $bp
	}

	#create receivers links
	for {set i [expr $nb_src + 2]} {$i<=[expr 2 * $nb_src + 1]} {incr i} {
		set delay [expr 4 * ($i -20)]ms
		set bp [expr $i - 20]e5
		#	puts stderr "$delay $bp"
		$self build_link 1 $i $delay  $bp
	}

	#place three PLM sources
	for {set i 2} {$i <= 4} {incr i} {
		set addr($i) [$self place_source $i 3]
	}
	#    puts stderr "sender placed"

	#place six PLM receiver: 2 receivers per source
	set j 2
	for {set i 2} {$i <=  7} {incr i 2} {
		set time [expr $i/2. + 3]
		$self place_receiver [expr $i + $nb_src] $addr($j) $time $check_estimate $i
		$self place_receiver [expr $i + $nb_src +1] $addr($j) $time $check_estimate [expr $i + 1]
		incr j
	}
	

	for {set i 1} {$i<=10} {incr i} {
		set null($i) [new Agent/Null]
		set udp($i) [new Agent/UDP]
		$udp($i) set fid_ [expr $i + 3]
		$ns attach-agent $node([expr $i + 4]) $udp($i)
		$ns attach-agent $node([expr $i + 27]) $null($i)
		$ns connect $udp($i) $null($i)
		set cbr($i) [new Application/Traffic/CBR]
		$cbr($i) attach-agent $udp($i)
		$cbr($i) set random_ 0
		$cbr($i) set rate_ 1Mb
		$cbr($i) set packet_size_ 1000
	}
	#    puts stderr "receivers placed"  
	
	for {set i 1} {$i<=3} {incr i} {
		$ns at 10 "$cbr($i) start"
		$ns at 15 "$cbr($i) stop"
	}
	
	for {set i 1} {$i<=10} {incr i} {
		$ns at 20 "$cbr($i) start"
		$ns at 25 "$cbr($i) stop"
	}
	#mcast set up
	DM set PruneTimeout 1000
	set mproto DM
	set mrthandle [$ns mrtproto $mproto {} ]
}

Test/PLM instproc finish {} {
    global run_nam PLMrcvr

    #    puts finish
    if {$run_nam} {
	puts "running nam..."
	exec nam -g 600x700 -f dynamic-nam.conf out.nam &
    }
    exit 0
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

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> \[<topologies>\]"
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	puts stderr "Valid Topologies are:\t[get-subclasses SkelTopology Topology/]"
	exit 1
}

Test/PLM instproc run {} {
	$self instvar ns
	$ns run
}

Test/PLM proc runTest {} {
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

			set topo [lindex $argv 1]
			if {$topo == "QUIET"} {
				set quiet true
				set topo ""
			} else {
				isProc? Topology $topo
			}
		}
		3 {
			set test [lindex $argv 0]
			isProc? Test $test

			set topo [lindex $argv 1]
			isProc? Topology $topo

			set extra [lindex $argv 2]
			if {$extra == "QUIET"} {
				set quiet true
			}
		}
		default {
			usage
		}
	}
	set t [new Test/$test $topo]
	$t run
}

Test/PLM runTest
