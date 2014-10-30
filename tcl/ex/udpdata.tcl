#
# Copyright (C) 2001 by USC/ISI
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

# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/udpdata.tcl,v 1.1 2001/11/20 21:46:38 buchheim Exp $ (USC/ISI)

#
#  This is a simple demonstration of how to send data in UDP datagrams
#

set ns [new Simulator]

$ns color 0 blue
$ns color 1 red


# open trace files
set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf


# create topology.  three nodes in line: (0)--(2)--(1)
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]

$ns duplex-link $n0 $n2 2Mb 5ms DropTail
$ns duplex-link $n2 $n1 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n2 orient right
$ns duplex-link-op $n2 $n1 orient right


# create UDP agents
set udp0 [new Agent/UDP]
$ns attach-agent $n0 $udp0

set udp1 [new Agent/UDP]
$ns attach-agent $n1 $udp1

$ns connect $udp0 $udp1



# The "process_data" instance procedure is what will process received data
# if no application is attached to the agent.
# In this case, we respond to messages of the form "ping(###)".
# We also print received messages to the trace file.
Agent/UDP instproc process_data {size data} {
	global ns
	$self instvar node_

	# note in the trace file that the packet was received
	$ns trace-annotate "[$node_ node-addr] received {$data}"

	# if the message was of the form "ping(###)" then send a response of
	# the form "pong(###)"
	if {[regexp {ping *\(([0-9]+)\)} $data entirematch number]} {
		$self send 100 "pong($number)"
	} elseif {[regexp {countdown *\(([0-9]+)\)} $data entirematch number]
		  && $number > 0 } {
		incr number -1
		$self send 100 "countdown($number)"
	}
}


# Setting the class allows us to color the packets in nam.
$udp0 set class_ 0
$udp1 set class_ 1

# Now, schedule some messages to be sent, using the UDP "send" procedure.
# The first argument is the length of the data and the second is the data
# itself.  Note that you can lie about the length, as we do here.  This allows
# you to send packets of whatever size you need in your simulation without
# actually generating a string of that length.  Also, note that the length
# you specify must not be larger than the maximum UDP packet size (the default
# is 1000 bytes)
# if the send procedure is called without a second argument (e.g. "send 100")
# then a packet of the specified length (or multiple packets, if the number
# is too high) will be sent, but without any data.  In that case, process_data
# will not be called at the receiver.

$ns at 0.1 "$udp0 send 724 ping(42)"
$ns at 0.2 "$udp1 send 100 countdown(5)"
$ns at 0.3 "$udp0 send 500 {ignore this message please}"
$ns at 0.4 "$udp1 send 828 {ping  (12345678)}"

$ns at 1.0 "finish"

proc finish {} {
	global ns f nf
	$ns flush-trace
	close $f
	close $nf

	puts "running nam..."
	exec nam out.nam &
	exit 0
}

$ns run

