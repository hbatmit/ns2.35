#
# Copyright (c) Xerox Corporation 1997. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# 
# Linking this file statically or dynamically with other modules is making
# a combined work based on this file.  Thus, the terms and conditions of
# the GNU General Public License cover the whole combination.
# 
# In addition, as a special exception, the copyright holders of this file
# give you permission to combine this file with free software programs or
# libraries that are released under the GNU LGPL and with code included in
# the standard release of ns-2 under the Apache 2.0 license or under
# otherwise-compatible licenses with advertising requirements (or modified
# versions of such code, with unchanged license).  You may copy and
# distribute such a system following the terms of the GNU GPL for this
# file and the licenses of the other code concerned, provided that you
# include the source code of that other code when and as the GNU GPL
# requires distribution of source code.
# 
# Note that people who make modified versions of this file are not
# obligated to grant this special exception for their modified versions;
# it is their choice whether to do so.  The GNU General Public License
# gives permission to release a modified version without this exception;
# this exception also makes it possible to release a modified version
# which carries forward this exception.
#

# updated to use -multicast on by Lloyd Wood

proc uniform01 {} {
	return [expr double(([ns-random] % 10000000) + 1) / 1e7]
}

proc uniform { a b } {
	return [expr ($b - $a) * [uniform01] + $a]
}

proc exponential mean {
	return [expr - $mean * log([uniform01])]
}

proc trunc_exponential lambda {
	while 1 {
		set u [exponential $lambda]
		if { $u < [expr 4 * $lambda] } {
			return $u
		}
	}
}


set packetSize 1000
set runtime 600
set scenario 0
set rlm_debug_flag 1
set seed 1
set rates "32e3 64e3 128e3 256e3 512e3 1024e3 2048e3"
#set rates "32e3 64e3 128e3 256e3"
set level [llength $rates]
set proto rlm
set run_nam 0

#XXX
Queue/DropTail set limit_ 15


Simulator instproc create-agent { node type pktClass } {
	$self instvar Agents PortID 
	set agent [new $type]
	$agent set fid_ $pktClass
	$self attach-agent $node $agent
	$agent proc get var {
		return [$self set $var]
	}
	return $agent
}

Simulator instproc cbr_flow { node fid addr bw } {
	global packetSize
	set agent [$self create-agent $node Agent/UDP $fid]
	set cbr [new Application/Traffic/CBR]
	$cbr attach-agent $agent
	
	#XXX abstraction violation
	$agent set dst_addr_ $addr
	$agent set dst_port_ 0
	
	$cbr set packetSize_ $packetSize
	$cbr set interval_ [expr $packetSize * 8. / $bw]
	$cbr set random_ 1
	return $cbr
}

Simulator instproc build_source_set { mmgName rates addrs baseClass node when } {
	global src_mmg src_rate
	set n [llength $rates]
	for {set i 0} {$i<$n} {incr i} {
		set r [lindex $rates $i]
		set addr [expr [lindex $addrs $i]]

		set src_rate($addr) $r
		set k $mmgName:$i
		set src_mmg($k) [$self cbr_flow $node $baseClass $addr $r]
		$self at $when "$src_mmg($k) start"
		incr baseClass
	}
}

Simulator instproc finish {} {
	#XXX
	global rcvrMMG  proto scenario lossTraceFile debugfile run_nam
	puts finish
	#XXX
	
	#flush_all_trace
	
	if [info exists plots] {
		close $plots
	}

	#XXX
	if [info exists lossTraceFile] {
		close $lossTraceFile
	}
	
	if [info exists debugfile] {
		close $debugfile
	}
	
	
	
	if [info exists rcvrMMG] {
		set br [expr [total_bits $rcvrMMG] / 8.]
		set bd [total_bytes_delivered $rcvrMMG]
		
		puts "loss-frac [expr 1024. * [node_loss $rcvrMMG] / $bd] \
			goodput [expr $bd / [optimal_bytes]]"
	}

	if {$run_nam} {
		puts "running nam..."
		exec nam -g 600x700 -f dynamic-nam.conf out.nam &
	}
	
	exit 0
}

Simulator instproc tick {} {
	puts stderr [$self now]
	$self at [expr [$self now] + 30.] "$self tick"
}


Class Topology

Topology instproc init { simulator } {
	$self instvar ns id
	set ns $simulator
	set id 0
}

Topology instproc mknode nn {
	$self instvar node ns
	if ![info exists node($nn)] {
		set node($nn) [$ns node]
	}
}


#
# build a link between nodes $a and $b
# if either node doesn't exist, create it as a side effect.
# (we don't build any sources)
#
Topology instproc build_link { a b delay bw } {
	global buffers packetSize 
	if { $a == $b } {
		puts stderr "link from $a to $b?"
		exit 1
	}
	$self instvar node ns
	$self mknode $a
	$self mknode $b
	$ns duplex-link $node($a) $node($b) $bw $delay DropTail
}

#
# build a new source (by allocating a new address) and
# place it at node $nn.  start it up at random time 
#
Topology instproc place_source { nn when } {
	#XXX
	global rates 
	$self instvar node ns id addrs caddrs

	incr id
	set caddrs($id) [Node allocaddr]
	set addrs($id) {}
	foreach r $rates {
		lappend addrs($id) [Node allocaddr]
	}

	$ns build_source_set s$id $rates $addrs($id) 1 $node($nn) $when
	
	return $id
}

Topology instproc place_receiver { nn id when } {
	$self instvar ns
	$ns at $when "$self build_receiver $nn $id"
}

#
# build a new receiver for source $id and
# place it at node $nn.
#
Topology instproc build_receiver { nn id } {
	$self instvar node ns addrs caddrs
	set rcvr [new MMG/ns $ns $node($nn) $caddrs($id) $addrs($id)]

	global rlm_debug_flag
	$rcvr set debug_ $rlm_debug_flag
}



Class Scenario0 -superclass Topology

Scenario0 instproc init args {
	#Create the following topology
	#             _____ R1
	#            /100kb
	#     1000kb/
	#   S------N1-------N2------R2
	#            1000kb  \ 250kb
	#                     \
        #               1000kb \
	#                       \N3____R3
	#                         50kb
	#
	eval $self next $args
	$self instvar ns node
	
	$self build_link 0 1 200ms  1000e3
	$self build_link 1 2 200ms  100e3
	$self build_link 1 3 200ms  1000e3
	$self build_link 3 4 200ms 250e3
	$self build_link 3 5 200ms 1000e3
	$self build_link 5 6 200ms 50e3


	$ns duplex-link-op $node(0) $node(1) orient right
	$ns duplex-link-op $node(1) $node(2) orient right
	$ns duplex-link-op $node(1) $node(3) orient right-down
	$ns duplex-link-op $node(3) $node(4) orient right
	$ns duplex-link-op $node(3) $node(5) orient  right-down
	$ns duplex-link-op $node(5) $node(6) orient  right

	$ns duplex-link-op $node(0) $node(1) queuePos 0.5
	$ns duplex-link-op $node(1) $node(2) queuePos 0.5
	$ns duplex-link-op $node(1) $node(3) queuePos 0.5
	$ns duplex-link-op $node(3) $node(4) queuePos 0.5
	$ns duplex-link-op $node(3) $node(5) queuePos 0.5
	$ns duplex-link-op $node(5) $node(6) queuePos 0.5

	set time [expr  double([ns-random] % 10000000) / 1e7 * 60]
	set addr [$self place_source 0 $time]

	$self place_receiver 2 $addr $time
	$self place_receiver 4 $addr $time
	$self place_receiver 6 $addr $time

#mcast set up
	DM set PruneTimeout 1000
	set mproto DM
	set mrthandle [$ns mrtproto $mproto {} ]
}

Class Scenario1 -superclass Topology

Scenario1 instproc init args {
	eval $self next $args
	$self instvar ns node
	
	$self build_link 0 1 200ms  100e3

	$ns duplex-link-op $node(0) $node(1) orient right
	$ns duplex-link-op $node(0) $node(1) queuePos 0.5

	set time [expr  double([ns-random] % 10000000) / 1e7 * 60]
	set id [$self place_source 0 $time]

	$self place_receiver 1 $id $time

#mcast set up
	DM set PruneTimeout 1000
	set mproto DM
	set mrthandle [$ns mrtproto $mproto {} ]
}

foreach a $argv {
	set L [split $a =]
	if {[llength $L] != 2} { continue }
	set var [lindex $L 0]
	set val [lindex $L 1]
	set $var $val
}

#Clean up rectFile
#rlm_init $rectFile $level $runtime  

set ns [new Simulator -multicast on]
#XXXX
proc ns-now {} "return \[$ns now]"

$ns color 1 blue
$ns color 2 green
$ns color 3 red
$ns color 4 white

# prunes, grafts
$ns color 30 orange
$ns color 31 yellow

$ns trace-all [open out.tr w]
$ns namtrace-all [open out.nam w]


set scn [new Scenario$scenario $ns]
$ns at [expr $runtime +1] "$ns finish"
$ns run

