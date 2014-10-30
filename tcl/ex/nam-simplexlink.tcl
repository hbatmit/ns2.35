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
# Testing display of simplex links in nam.
#
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/nam-simplexlink.tcl,v 1.1 1998/09/13 23:39:37 haoboy Exp $

set ns [new Simulator]

set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

$ns trace-all [open out.tr w]
$ns namtrace-all [open out.nam w]

$ns simplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns simplex-link $n2 $n3 1.5Mb 10ms DropTail
$ns simplex-link $n3 $n1 1.5Mb 10ms DropTail

$ns rtproto Session

set tcp1 [new Agent/TCP/FullTcp]
set tcp2 [new Agent/TCP/FullTcp]
$tcp1 set window_ 100
$tcp1 set fid_ 1
$tcp2 set window_ 100
$tcp2 set fid_ 2
$tcp2 set iss_ 1224
$ns attach-agent $n1 $tcp1
$ns attach-agent $n3 $tcp2
$ns connect $tcp1 $tcp2
$tcp2 listen

$ns at 1.0 "$tcp1 send 2048"

$ns at 10.0 "finish"

proc finish {} {
	global ns
	$ns flush-trace
	exit 0
}

$ns run
