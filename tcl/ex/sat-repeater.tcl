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
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/sat-repeater.tcl,v 1.5 2002/07/10 02:29:50 tomh Exp $
#
# Simple script with a geostationary satellite and two terminals
# and an error module on the receiving terminal.  The traffic consists of
# a FTP source and a CBR stream  
# 

global ns
set ns [new Simulator]

# Global configuration parameters
# We'll set these global options for the satellite terminals

global opt
set opt(chan)           Channel/Sat
set opt(bw_up)		2Mb
set opt(bw_down)	2Mb
set opt(phy)            Phy/Sat
set opt(mac)            Mac/Sat
set opt(ifq)            Queue/DropTail
set opt(qlim)		50
set opt(ll)             LL/Sat
set opt(wiredRouting)   OFF

# XXX This tracing enabling must precede link and node creation 
set outfile [open out.tr w]
$ns trace-all $outfile

# Set up satellite and terrestrial nodes

# Configure the node generator for bent-pipe satellite
# geo-repeater uses type Phy/Repeater
$ns node-config -satNodeType geo-repeater \
		-phyType Phy/Repeater \
		-channelType $opt(chan) \
		-downlinkBW $opt(bw_down)  \
		-wiredRouting $opt(wiredRouting)

# GEO satellite at 95 degrees longitude West
set n1 [$ns node]
$n1 set-position -95

# Configure the node generator for satellite terminals
$ns node-config -satNodeType terminal \
                -llType $opt(ll) \
                -ifqType $opt(ifq) \
                -ifqLen $opt(qlim) \
                -macType $opt(mac) \
                -phyType $opt(phy) \
                -channelType $opt(chan) \
                -downlinkBW $opt(bw_down) \
                -wiredRouting $opt(wiredRouting)

# Two terminals: one in NY and one in SF 
set n2 [$ns node]
$n2 set-position 40.9 -73.9; # NY
set n3 [$ns node]
$n3 set-position 37.8 -122.4; # SF

# Add GSLs to geo satellites
$n2 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
    $opt(phy) [$n1 set downlink_] [$n1 set uplink_]
$n3 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
    $opt(phy) [$n1 set downlink_] [$n1 set uplink_]

# Add an error model to the receiving terminal node
set em_ [new ErrorModel]
$em_ unit pkt
$em_ set rate_ 0.02
$em_ ranvar [new RandomVariable/Uniform]
$n3 interface-errormodel $em_ 

$ns trace-all-satlinks $outfile

# Attach agents for CBR traffic generator 
set udp0 [new Agent/UDP]
$ns attach-agent $n2 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0
$cbr0 set interval_ 6

set null0 [new Agent/Null]
$ns attach-agent $n3 $null0

$ns connect $udp0 $null0

# Attach agents for FTP  
set tcp1 [$ns create-connection TCP $n2 TCPSink $n3 0]
set ftp1 [$tcp1 attach-app FTP]
$ns at 7.0 "$ftp1 produce 100"

# We use centralized routing
set satrouteobject_ [new SatRouteObject]
$satrouteobject_ compute_routes

$ns at 1.0 "$cbr0 start"

$ns at 100.0 "finish"

proc finish {} {
	global ns outfile
	$ns flush-trace
	close $outfile

	exit 0
}

$ns run

