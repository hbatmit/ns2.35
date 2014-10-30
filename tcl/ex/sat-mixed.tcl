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
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/sat-mixed.tcl,v 1.4 2001/11/06 06:20:10 tomh Exp $
#
# Example script that mixes geo satellites with some polar satellites:
# One plane of Iridium-like satellites, one geo satellite, and two terminals 
# pinging one another on it
# In the script, since this is not a full constellation, the LEO satellites
# eventually move out of range of the terminals and packets get dropped
# 

global ns
set ns [new Simulator]

###########################################################################
# Global configuration parameters                                         #
###########################################################################

HandoffManager/Term set elevation_mask_ 8.2
HandoffManager/Term set term_handoff_int_ 10
HandoffManager set handoff_randomization_ false

global opt
set opt(chan)           Channel/Sat
set opt(bw_down)	1.5Mb; # Downlink bandwidth (satellite to ground)
set opt(bw_up)		1.5Mb; # Uplink bandwidth
set opt(bw_isl)		25Mb
set opt(phy)            Phy/Sat
set opt(mac)            Mac/Sat
set opt(ifq)            Queue/DropTail
set opt(qlim)		50
set opt(ll)             LL/Sat
set opt(wiredRouting)   OFF

set opt(alt)		780; # Polar satellite altitude (Iridium)
set opt(inc)		90; # Orbit inclination w.r.t. equator

# IMPORTANT This tracing enabling (trace-all) must precede link and node 
#           creation.  Then following all node, link, and error model
#           creation, invoke "$ns trace-all-satlinks $outfile" 
set outfile [open out.tr w]
$ns trace-all $outfile

###########################################################################
# Set up satellite and terrestrial nodes                                  #
###########################################################################

# Let's first create a single orbital plane of Iridium-like satellites
# 11 satellites in a plane

# Set up the node configuration

$ns node-config -satNodeType polar \
		-llType $opt(ll) \
		-ifqType $opt(ifq) \
		-ifqLen $opt(qlim) \
		-macType $opt(mac) \
		-phyType $opt(phy) \
		-channelType $opt(chan) \
		-downlinkBW $opt(bw_down) \
		-wiredRouting $opt(wiredRouting)

# Create nodes n0 through n10
set n0 [$ns node]; set n1 [$ns node]; set n2 [$ns node]; set n3 [$ns node] 
set n4 [$ns node]; set n5 [$ns node]; set n6 [$ns node]; set n7 [$ns node] 
set n8 [$ns node]; set n9 [$ns node]; set n10 [$ns node]

# Now provide position information for each of these nodes
# Position arguments are: altitude, incl., longitude, "alpha", and plane
# See documentation for definition of these fields
set plane 1
$n0 set-position $opt(alt) $opt(inc) 0 0 $plane 
$n1 set-position $opt(alt) $opt(inc) 0 32.73 $plane
$n2 set-position $opt(alt) $opt(inc) 0 65.45 $plane
$n3 set-position $opt(alt) $opt(inc) 0 98.18 $plane
$n4 set-position $opt(alt) $opt(inc) 0 130.91 $plane
$n5 set-position $opt(alt) $opt(inc) 0 163.64 $plane
$n6 set-position $opt(alt) $opt(inc) 0 196.36 $plane
$n7 set-position $opt(alt) $opt(inc) 0 229.09 $plane
$n8 set-position $opt(alt) $opt(inc) 0 261.82 $plane
$n9 set-position $opt(alt) $opt(inc) 0 294.55 $plane
$n10 set-position $opt(alt) $opt(inc) 0 327.27 $plane

# This next step is specific to polar satellites
# By setting the next_ variable on polar sats; handoffs can be optimized  
# This step must follow all polar node creation
$n0 set_next $n10; $n1 set_next $n0; $n2 set_next $n1; $n3 set_next $n2
$n4 set_next $n3; $n5 set_next $n4; $n6 set_next $n5; $n7 set_next $n6
$n8 set_next $n7; $n9 set_next $n8; $n10 set_next $n9

# GEO satellite:  above North America-- lets put it at 100 deg. W
$ns node-config -satNodeType geo
set n11 [$ns node]
$n11 set-position -100

# Terminals:  Let's put two within the US, two around the prime meridian
$ns node-config -satNodeType terminal 
set n100 [$ns node]; set n101 [$ns node]
$n100 set-position 37.9 -122.3; # Berkeley
$n101 set-position 42.3 -71.1; # Boston
set n200 [$ns node]; set n201 [$ns node]
$n200 set-position 0 10 
$n201 set-position 0 -10

###########################################################################
# Set up links                                                            #
###########################################################################

# Add any necessary ISLs or GSLs
# GSLs to the geo satellite:
$n100 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
$n101 add-gsl geo $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  $opt(phy) [$n11 set downlink_] [$n11 set uplink_]
# Attach n200 and n201 initially to a satellite on other side of the earth
# (handoff will automatically occur to fix this at the start of simulation)
$n200 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  $opt(phy) [$n5 set downlink_] [$n5 set uplink_]
$n201 add-gsl polar $opt(ll) $opt(ifq) $opt(qlim) $opt(mac) $opt(bw_up) \
  $opt(phy) [$n5 set downlink_] [$n5 set uplink_]

# ISLs for the polar satellites
$ns add-isl intraplane $n0 $n1 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n1 $n2 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n2 $n3 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n3 $n4 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n4 $n5 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n5 $n6 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n6 $n7 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n7 $n8 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n8 $n9 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n9 $n10 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n10 $n0 $opt(bw_isl) $opt(ifq) $opt(qlim)

###########################################################################
# Tracing                                                                 #
###########################################################################
$ns trace-all-satlinks $outfile

###########################################################################
# Attach agents                                                           #
###########################################################################

set udp0 [new Agent/UDP]
$ns attach-agent $n100 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0
$cbr0 set interval_ 60.01

set udp1 [new Agent/UDP]
$ns attach-agent $n200 $udp1
$udp1 set class_ 1
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $udp1
$cbr1 set interval_ 90.5

set null0 [new Agent/Null]
$ns attach-agent $n101 $null0
set null1 [new Agent/Null]
$ns attach-agent $n201 $null1

$ns connect $udp0 $null0
$ns connect $udp1 $null1

###########################################################################
# Satellite routing                                                       #
###########################################################################

set satrouteobject_ [new SatRouteObject]
$satrouteobject_ compute_routes

$ns at 1.0 "$cbr0 start"
$ns at 305.0 "$cbr1 start"

$ns at 9000.0 "finish"

proc finish {} {
	global ns outfile 
	$ns flush-trace
	close $outfile

	exit 0
}

$ns run

