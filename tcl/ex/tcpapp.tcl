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

# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/tcpapp.tcl,v 1.1 2001/11/08 19:43:43 buchheim Exp $ (USC/ISI)


#  This is a simple demonstration of how to use the TcpApp application
#  to send data over a TCP connection.


set namfile	out.nam
set tracefile	out.tr

set ns [new Simulator]


# open trace files and enable tracing
set nf [open $namfile w]
$ns namtrace-all $nf
set f [open $tracefile w]
$ns trace-all $f


# create two nodes
set n0 [$ns node]
set n1 [$ns node]

$ns duplex-link $n0 $n1 2Mb 5ms DropTail
$ns duplex-link-op $n0 $n1 orient right


# create FullTcp agents for the nodes
# TcpApp needs a two-way implementation of TCP
set tcp0 [new Agent/TCP/FullTcp]
set tcp1 [new Agent/TCP/FullTcp]

$ns attach-agent $n0 $tcp0
$ns attach-agent $n1 $tcp1

$ns connect $tcp0 $tcp1
$tcp1 listen


# set up TcpApp
set app0 [new Application/TcpApp $tcp0]
set app1 [new Application/TcpApp $tcp1]
$app0 connect $app1


# install a procedure to print out the received data
Application/TcpApp instproc recv {data} {
	global ns
	$ns trace-annotate "$self received data \"$data\""
}

proc finish {} {
	global ns nf f namfile
	$ns flush-trace
	close $nf
	close $f

	puts "running nam..."
	exec nam $namfile &
	exit 0
}

# send a message via TcpApp
# The string will be interpreted by the receiver as Tcl code.
$ns at 0.5 "$app0 send 100 {$app1 recv {my mesage}}"


$ns at 1.0 "finish"

$ns run
