#
# Copyright (c) 1997,1998,1999 Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/simple-rtp.tcl,v 1.5 1999/09/10 22:08:45 haoboy Exp $
# updated to use -multicast on and allocaddr by Lloyd Wood

set ns [new Simulator -multicast on]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

$ns color 1 red
# prune/graft packets
$ns color 30 purple
$ns color 31 bisque
# RTCP reports
$ns color 32 green

set f [open rtp-out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n1 $n2 orient right-up
$ns duplex-link-op $n1 $n3 orient right-down
$ns duplex-link-op $n0 $n1 queuePos 0.5

set mproto DM
set mrthandle [$ns mrtproto $mproto {}]
set group [Node allocaddr]

set s0 [new Session/RTP]
set s1 [new Session/RTP]
set s2 [new Session/RTP]
set s3 [new Session/RTP]

$s0 session_bw 400kb/s
$s1 session_bw 400kb/s
$s2 session_bw 400kb/s
$s3 session_bw 400kb/s

$s0 attach-node $n0
$s1 attach-node $n1
$s2 attach-node $n2
$s3 attach-node $n3

$ns at 1.4 "$s0 join-group $group"
$ns at 1.5 "$s0 start"
$ns at 1.6 "$s0 transmit 400kb/s"

$ns at 1.7 "$s1 join-group $group"
$ns at 1.8 "$s1 start"
$ns at 1.9 "$s1 transmit 400kb/s"

$ns at 2.0 "$s2 join-group $group"
$ns at 2.1 "$s2 start"
$ns at 2.2 "$s2 transmit 400kb/s"

$ns at 2.3 "$s3 join-group $group"
$ns at 2.4 "$s3 start"
$ns at 2.5 "$s3 transmit 400kb/s"

$ns at 4.0 "finish"

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
