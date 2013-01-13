#
# Copyright (C) 1997 by USC/ISI
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

#
# Maintainer: Kannan Varadhan <kannan@isi.edu>
#

#
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
#

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

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

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

proc build-tcp { n0 n1 startTime } {
	global ns
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

[build-tcp $n0 $n4 0.7] set class_ 0
[build-tcp $n1 $n4 0.9] set class_ 1

proc finish {} {
	global argv0
	global ns f nf
	$ns flush-trace
	close $f
	close $nf

	if [string match {*.tcl} $argv0] {
		set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
	} else {
		set prog $argv0
	}

	puts "running nam..."
	exec nam -f dynamic-nam.conf out.nam &
	exit 0
}

Agent/rtProto/Direct set preference_ 200

$ns rtmodel Deterministic {.5 .5} $n2 $n4
[$ns link $n2 $n4] trace-dynamics $ns stdout

$ns rtproto DV

$ns at 8.0 "finish"
$ns run 
