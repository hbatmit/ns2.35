#
# Copyright (c) 1996 Regents of the University of California.
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
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/mcast.tcl,v 1.12 1999/09/10 22:08:41 haoboy Exp $
# updated to use -multicast on and allocaddr by Lloyd Wood

#
# Simple multicast test.  It's easiest to verify the
# output with the animator.
# We create a four node start; start a CBR traffic generator in the center
# and then at node 3 and exercise the join/leave code.
#
# See tcl/ex/newmcast/mcast*.tcl for more mcast example scripts

set ns [new Simulator -multicast on]

set f [open out.tr w]
$ns trace-all $f
$ns namtrace-all [open out.nam w]

$ns color 1 red
# prune/graft packets
$ns color 30 purple
$ns color 31 bisque

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

# Use automatic layout 
$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n1 $n2 orient right-up
$ns duplex-link-op $n1 $n3 orient right-down
$ns duplex-link-op $n0 $n1 queuePos 0.5

set mproto DM
set mrthandle [$ns mrtproto $mproto {}]
set group0 [Node allocaddr]
set group1 [Node allocaddr]

set udp0 [new Agent/UDP]
$ns attach-agent $n1 $udp0
$udp0 set dst_addr_ $group0
$udp0 set dst_port_ 0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0

set udp1 [new Agent/UDP]
$udp1 set dst_addr_ $group1
$udp1 set dst_port_ 0
$udp1 set class_ 1
$ns attach-agent $n3 $udp1
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $udp1

set rcvr [new Agent/LossMonitor]
$ns attach-agent $n2 $rcvr
$ns at 1.2 "$n2 join-group $rcvr $group1"
$ns at 1.25 "$n2 leave-group $rcvr $group1"
$ns at 1.3 "$n2 join-group $rcvr $group1"
$ns at 1.35 "$n2 join-group $rcvr $group0"

$ns at 1.0 "$cbr0 start"
#$ns at 1.001 "$cbr0 stop"
$ns at 1.1 "$cbr1 start"

#set tcp [new Agent/TCP]
#set sink [new Agent/TCPSink]
#$ns attach-agent $n0 $tcp
#$ns attach-agent $n3 $sink
#$ns connect $tcp $sink
#set ftp [new Application/FTP]
#$ftp attach-agent $tcp
#$ns at 1.2 "$ftp start"

#puts [$cbr0 set packetSize_]
#puts [$cbr0 set interval_]

$ns at 2.0 "finish"

proc finish {} {
	global ns
	$ns flush-trace

	puts "running nam..."
	exec nam out.nam &
	exit 0
}

$ns run
