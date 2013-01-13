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

# ns-random 0

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21

Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	set ns_ [new Simulator]
	# trace-all is only used in more extensive test suites
	# $ns_ trace-all [open all.tr w]
	if {$net_ == ""} {
		set net_ $defNet_
	}
	if ![Topology/$defNet_ info subclass Topology/$net_] {
		global argv0
		puts "$argv0: cannot run test $test_ over topology $net_"
		exit 1
	}

	set topo_ [new Topology/$net_ $ns_]
	foreach i [$topo_ array names node_] {
		# This would be cool, but lets try to be compatible
		# with test-suite.tcl as far as possible.
		#
		# $self instvar $i
		# set $i [$topo_ node? $i]
		#
		set node_($i) [$topo_ node? $i]
	}

	if {$net_ == $defNet_} {
		set testName_ "$test_"
	} else {
		set testName_ "$test_:$net_"
	}

}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> \[<topologies>\]"
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	puts stderr "Valid Topologies are:\t[get-subclasses SkelTopology Topology/]"
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

# Skeleton topology base class
Class SkelTopology

SkelTopology instproc init {} {
    $self next
}

SkelTopology instproc node? n {
    $self instvar node_
    if [info exists node_($n)] {
	set ret $node_($n)
    } else {
	set ret ""
    }
    set ret
}

Class NodeTopology/4nodes -superclass SkelTopology

NodeTopology/4nodes instproc init ns {
    $self next

    $self instvar node_
    set node_(s1) [$ns node]
    set node_(k1) [$ns node]
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 100ms bottleneck.
# Queue-limit on bottleneck is 6 packets.
#
Class Topology/net0 -superclass NodeTopology/4nodes
Topology/net0 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(k1) 10000Mb 20ms DropTail
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

# Definition of test-suite tests

TestSuite instproc print64 { qmon } {
	set f [open temp.rands w]
	puts $f "This test is checking for problems with int64 counters."
	close $f
  	if {[ns-hasint64] ==  1} {
		set bdep [$qmon set bdepartures_]
		puts "This test is checking for problems with int64 counters."
	 	puts "Byte departures in different data formats:"
		puts "Qmon set bdepartures_, or bdep: $bdep"
	        puts "ns-add64 bdep 0:                [ns-add64 $bdep 0]"
	 	set bdepDbl [ns-int64todbl $bdep]
	  	puts "ns-int64todbl bdep:             $bdepDbl"
	 	puts "ns-int64todbl bdep + 0:         [expr $bdepDbl + 0]"
		puts "These will give the wrong answer:" 
	 	puts "bdep + 0:                       [expr $bdep + 0]"
	 	puts "bdep * 1:                       [expr $bdep * 1]"
 	} else {
		puts "This machine doesn't use int64 counters."
	}
}

Class Test/stats64 -superclass TestSuite
Test/stats64 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	Queue/DropTail set summarystats_ true
	set test_	stats64
	$self next
}
Test/stats64 instproc run {} {
	$self instvar ns_ node_ testName_ 

	Agent/TCP set packetSize_ 2000
	set stoptime 75.1
	set printtime [expr $stoptime - 0.1]

	set slink [$ns_ link $node_(s1) $node_(k1)]; # link to collect stats on
#	set fmon [$ns_ makeflowmon Fid]
#	$ns_ attach-fmon $slink $fmon
	set qmon [$ns_ monitor-queue $node_(s1) $node_(k1) ""]

	set tcp0 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp0 set window_ 1000
	set ftp0 [$tcp0 attach-app FTP]
	$ns_ at 0.0 "$ftp0 start"

	$ns_ at $printtime "$self print64 $qmon"
	$ns_ at $stoptime "exit 0"
	# call finish, make an output file.
	$ns_ run
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
