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
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/sat-iridium.tcl,v 1.4 2001/11/06 06:20:11 tomh Exp $
#
# Example of a broadband LEO constellation with orbital configuration 
# similar to that of Iridium.  The script sets up two terminals (one in 
# Boston, one at Berkeley) and sends a packet from Berkeley to Boston
# every second for a whole day-- the script illustrates how the latency
# due to propagation delay changes depending on the satellite configuration. 
#
# This script relies on sourcing two additional files:
# - sat-iridium-nodes.tcl
# - sat-iridium-links.tcl
# Iridium does not have crossseam ISLs-- to enable crossseam ISLs, uncomment 
# the last few lines of "sat-iridium-links.tcl"
#
# Iridium parameters [primary reference:  "Satellite-Based Global Cellular
# Communications by Bruno Pattan (1997-- McGraw-Hill)]
# Altitude = 780 km
# Orbital period = 6026.9 sec
# intersatellite separation = 360/11 deg
# interplane separation = 31.6 deg
# seam separation = 22 deg
# inclination = 86.4
# eccentricity =  0.002 (not modelled)
# minimum elevation angle at edge of coverage = 8.2 deg
# ISL cross-link pattern:  2 intraplane to nearest neighbors in plane, 
#   2 interplane except at seam where only 1 interplane exists


global ns
set ns [new Simulator]

# Global configuration parameters 
HandoffManager/Term set elevation_mask_ 8.2
HandoffManager/Term set term_handoff_int_ 10
HandoffManager/Sat set sat_handoff_int_ 10
HandoffManager/Sat set latitude_threshold_ 60 
HandoffManager/Sat set longitude_threshold_ 10 
HandoffManager set handoff_randomization_ true
SatRouteObject set metric_delay_ true
# Set this to false if opt(wiredRouting) == ON below
SatRouteObject set data_driven_computation_ true
# "ns-random 0" sets seed heuristically; other integers are deterministic
ns-random 1
Agent set ttl_ 32; # Should be > than max diameter in network

# One plane of Iridium-like satellites

global opt
set opt(chan)           Channel/Sat
set opt(bw_down)        1.5Mb; # Downlink bandwidth (satellite to ground)
set opt(bw_up)          1.5Mb; # Uplink bandwidth
set opt(bw_isl)         25Mb
set opt(phy)            Phy/Sat
set opt(mac)            Mac/Sat
set opt(ifq)            Queue/DropTail
set opt(qlim)           50
set opt(ll)             LL/Sat
set opt(wiredRouting) 	OFF

set opt(alt)            780; # Polar satellite altitude (Iridium)
set opt(inc)            86.4; # Orbit inclination w.r.t. equator

# XXX This tracing enabling must precede link and node creation
set outfile [open out.tr w]
$ns trace-all $outfile

# Create the satellite nodes
# Nodes 0-99 are satellite nodes; 100 and higher are earth terminals

$ns node-config -satNodeType polar \
		-llType $opt(ll) \
		-ifqType $opt(ifq) \
		-ifqLen $opt(qlim) \
		-macType $opt(mac) \
		-phyType $opt(phy) \
		-channelType $opt(chan) \
		-downlinkBW $opt(bw_down) \
		-wiredRouting $opt(wiredRouting) 

set alt $opt(alt)
set inc $opt(inc)

source sat-iridium-nodes.tcl

# configure the ISLs
source sat-iridium-links.tcl

# Set up terrestrial nodes
$ns node-config -satNodeType terminal
set n100 [$ns node]
$n100 set-position 37.9 -122.3; # Berkeley
set n101 [$ns node]
$n101 set-position 42.3 -71.1; # Boston 

# Add GSL links
# It doesn't matter what the sat node is (handoff algorithm will reset it)
$n100 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  $opt(phy) [$n0 set downlink_] [$n0 set uplink_]
$n101 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  $opt(phy) [$n0 set downlink_] [$n0 set uplink_]

# Trace all queues
$ns trace-all-satlinks $outfile

# Attach agents
set udp0 [new Agent/UDP]
$ns attach-agent $n100 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0
$cbr0 set interval_ 60.01

set null0 [new Agent/Null]
$ns attach-agent $n101 $null0

$ns connect $udp0 $null0
$ns at 1.0 "$cbr0 start"

# We're using a centralized routing genie-- create and start it here
set satrouteobject_ [new SatRouteObject]
$satrouteobject_ compute_routes

$ns at 86400.0 "finish" ; # one earth rotation 

proc finish {} {
	global ns outfile 
	$ns flush-trace
	close $outfile

	exit 0
}

$ns run

