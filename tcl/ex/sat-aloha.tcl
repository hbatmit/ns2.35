#
# Copyright (c) 1999 Regents of the University of California.
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
#       This product includes software developed by the MASH Research
#       Group at the University of California Berkeley.
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
# Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
#
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/sat-aloha.tcl,v 1.5 2001/11/06 06:20:11 tomh Exp $
# 
# Simple script with a geostationary bent-pipe (repeater) satellite and 
# one hundred terminals using unslotted (pure) Aloha random access.  The
# traffic sources consist of exponential on-off traffic generators unless
# indicated below.  With the traffic rates configured below, approximately 
# one tenth of the packets collide and must be retransmitted.
# 
# Tests:
# 1.  basic:  Mac operates in stop-and-wait mode (one outstanding packet
#             at a time).  Collisions and drops are not traced.  This test
#             should provide similar results to the satellite test suite.
# 2.  basic_tracing:  Same as "basic", but drops ("d") and collisions ("c")
#             are instead explicitly traced.  Again, similar to test suite   
# 3.  poisson:  Packets arrive according to Poisson process.  Each source 
#             still operates in stop-and-wait mode, and collisions and 
#             drops are traced.  rtx_limit_ == 0 (no persistence).
#	      This can be used to try to approximate theoretical 
#	      unslotted Aloha results, if the number of terminals is large
#             compared to the arrival rate (so that no packets are queued)
# 4.  [FOR FUTURE WORK].  (larger than one packet rxmit buffer, different
#             traffic generator?)
# 

if { $argc != 1 } {
        puts stderr {usage: ns sat-aloha.tcl [basic basic_tracing poisson]}
        exit 1
}
set test_ $argv
puts "Running test $test_ ..."


global ns
set ns [new Simulator]

# Global configuration parameters for Aloha (also settable in ns-sat.tcl)
Mac/Sat/UnslottedAloha set mean_backoff_ 1s ; # mean exponential backoff time(s)
Mac/Sat/UnslottedAloha set rtx_limit_ 3; # max number of retrans. attempted 
Mac/Sat/UnslottedAloha set send_timeout_ 270ms; # resend if send times out

if { $test_ == "basic"} {
	Mac/Sat set trace_collisions_ false
	Mac/Sat set trace_drops_ false
}

global opt
set opt(chan)           Channel/Sat
set opt(bw_up)		2Mb
set opt(bw_down)	2Mb
set opt(phy)            Phy/Sat
set opt(mac)            Mac/Sat/UnslottedAloha
set opt(ifq)            Queue/DropTail
set opt(qlim)		50
set opt(ll)             LL/Sat
set opt(wiredRouting)   OFF

# XXX This tracing enabling must precede link and node creation 
set outfile [open out.tr w]
$ns trace-all $outfile

# Set up satellite and terrestrial nodes

# GEO satellite at 0 degrees longitude 
$ns node-config -satNodeType geo-repeater \
		-llType $opt(ll) \
		-ifqType $opt(ifq) \
		-ifqLen $opt(qlim) \
		-macType $opt(mac) \
		-phyType $opt(phy) \
		-channelType $opt(chan) \
		-downlinkBW $opt(bw_down) \
		-wiredRouting $opt(wiredRouting)
set n1 [$ns node]
$n1 set-position 0


# Place 50 terminals at 100 different locations
$ns node-config -satNodeType terminal
set num_nodes	100
for {set a 1} {$a <= $num_nodes} {incr a} {
	set n($a) [$ns node]
	$n($a) set-position [expr -15 + $a * 0.3] [expr 15 - $a * 0.3]
	$n($a) add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  		$opt(phy) [$n1 set downlink_] [$n1 set uplink_]
}

for {set a 1} {$a <= $num_nodes} {incr a} {
	set b [expr int($a + (0.5 * $num_nodes))]
	if {$b > $num_nodes} {
		incr b [expr -1 * $num_nodes]
	}

	set udp($a) [new Agent/UDP]
	$ns attach-agent $n($a) $udp($a)
	set exp($a) [new Application/Traffic/Exponential]
	$exp($a) attach-agent $udp($a)
	$exp($a) set rate_ 1Kb
	if {$test_ == "poisson"} {
		$exp($a) set rate_ 10000Mb
		$exp($a) set burst_time_ 0
		$exp($a) set idle_time_ 5 
	}

	set null($a) [new Agent/Null]
	$ns attach-agent $n($b) $null($a)

	$ns connect $udp($a) $null($a)
	$ns at 1.0 "$exp($a) start"
}

$ns trace-all-satlinks $outfile

# We use centralized routing
set satrouteobject_ [new SatRouteObject]
$satrouteobject_ compute_routes

$ns at 100.0 "finish"

proc finish {} {
	global ns outfile 
	$ns flush-trace
	close $outfile

	exit 0
}

$ns run

