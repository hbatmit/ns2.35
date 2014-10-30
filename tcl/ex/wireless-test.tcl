# Copyright (c) 1997 Regents of the University of California.
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
#      This product includes software developed by the Computer Systems
#      Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/wireless-test.tcl,v 1.5 2000/08/18 18:34:04 haoboy Exp $
#
# A simple wireless example file that simulates a 3-mobilenode 
# topology. Traffic used are CBR and TCP flows.

# ======================================================================
# Default Script Options
# ======================================================================

set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
#set opt(netif)		NetIf/SharedMedia
set opt(netif)		Phy/WirelessPhy
#set opt(mac)		Mac/802_11
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna

set opt(x)		670	;# X dimension of the topography
set opt(y)		670		;# Y dimension of the topography
set opt(cp)		"../mobility/scene/cbr-3-test"
set opt(sc)		"../mobility/scene/scen-3-test"

set opt(ifqlen)		50		;# max packet in ifq
set opt(nn)		3		;# number of nodes
set opt(seed)		0.0
set opt(stop)		2000.0		;# simulation time
set opt(tr)		out-test.tr		;# trace file
set opt(rp)             dsr          ;# routing protocol script
set opt(lm)             "off"           ;# log movement

# ======================================================================

set AgentTrace			ON
set RouterTrace			ON
set MacTrace			OFF

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	1460

Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set CSThresh_ 1.559e-11
Phy/WirelessPhy set RXThresh_ 3.652e-10
Phy/WirelessPhy set Rb_ 2*1e6
Phy/WirelessPhy set Pt_ 0.2818
Phy/WirelessPhy set freq_ 914e+6 
Phy/WirelessPhy set L_ 1.0

# ======================================================================

proc usage { argv0 }  {
	puts "Usage: $argv0"
	puts "\tmandatory arguments:"
	puts "\t\t\[-x MAXX\] \[-y MAXY\]"
	puts "\toptional arguments:"
	puts "\t\t\[-cp conn pattern\] \[-sc scenario\] \[-nn nodes\]"
	puts "\t\t\[-seed seed\] \[-stop sec\] \[-tr tracefile\]\n"
}


proc getopt {argc argv} {
	global opt
	lappend optlist cp nn seed sc stop tr x y

	for {set i 0} {$i < $argc} {incr i} {
		set arg [lindex $argv $i]
		if {[string range $arg 0 0] != "-"} continue

		set name [string range $arg 1 end]
		set opt($name) [lindex $argv [expr $i+1]]
	}
}


proc cmu-trace { ttype atype node } {
	global ns_ tracefd

	if { $tracefd == "" } {
		return ""
	}
	set T [new CMUTrace/$ttype $atype]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
        $T set src_ [$node id]

        $T node $node

	return $T
}


proc create-god { nodes } {
	global ns_ god_ tracefd

	set god_ [new God]
	$god_ num_nodes $nodes
}

proc log-movement {} {
    global logtimer ns_ ns

    set ns $ns_
    source ../mobility/timer.tcl
    Class LogTimer -superclass Timer
    LogTimer instproc timeout {} {
	global opt node_;
	for {set i 0} {$i < $opt(nn)} {incr i} {
	    $node_($i) log-movement
	}
	$self sched 0.1
    }

    set logtimer [new LogTimer]
    $logtimer sched 0.1
}

# ======================================================================
# Main Program
# ======================================================================
getopt $argc $argv

#
# Source External TCL Scripts
#
source ../lib/ns-mobilenode.tcl

#if { $opt(rp) != "" } {
	source ../mobility/$opt(rp).tcl
	#} elseif { [catch { set env(NS_PROTO_SCRIPT) } ] == 1 } {
	#puts "\nenvironment variable NS_PROTO_SCRIPT not set!\n"
	#exit
#} else {
	#puts "\n*** using script $env(NS_PROTO_SCRIPT)\n\n";
        #source $env(NS_PROTO_SCRIPT)
#}
source ../lib/ns-cmutrace.tcl

# do the get opt again incase the routing protocol file added some more
# options to look for
getopt $argc $argv

if { $opt(x) == 0 || $opt(y) == 0 } {
	usage $argv0
	exit 1
}

if {$opt(seed) > 0} {
	puts "Seeding Random number generator with $opt(seed)\n"
	ns-random $opt(seed)
}

#
# Initialize Global Variables
#
set ns_		[new Simulator]
set chan	[new $opt(chan)]
set prop	[new $opt(prop)]
set topo	[new Topography]
set tracefd	[open $opt(tr) w]

set nf [open nam-out-test.nam w]
set f [open trace-out-test.tr w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)
$ns_ trace-all $f

$topo load_flatgrid $opt(x) $opt(y)

$prop topography $topo

#
# Create God
#
create-god $opt(nn)


#
# log the mobile nodes movements if desired
#
if { $opt(lm) == "on" } {
    log-movement
}

#
#  Create the specified number of nodes $opt(nn) and "attach" them
#  the channel.
#  Each routing protocol script is expected to have defined a proc
#  create-mobile-node that builds a mobile node and inserts it into the
#  array global $node_($i)
#

if { [string compare $opt(rp) "dsr"] == 0} { 
	for {set i 0} {$i < $opt(nn) } {incr i} {
		dsr-create-mobile-node $i
	}
} elseif { [string compare $opt(rp) "dsdv"] == 0} { 
	for {set i 0} {$i < $opt(nn) } {incr i} {
		dsdv-create-mobile-node $i
	}
}

#enable node trace in nam

for {set i 0} {$i < $opt(nn)} {incr i} {
    $node_($i) namattach $nf
# 20 defines the node size in nam, must adjust it according to your scenario
   $ns_ initial_node_pos $node_($i) 20
}

#
# Source the Connection and Movement scripts
#
if { $opt(cp) == "" } {
	puts "*** NOTE: no connection pattern specified."
        set opt(cp) "none"
} else {
	puts "Loading connection pattern..."
	source $opt(cp)
}





#
# Tell all the nodes when the simulation ends
#
for {set i } {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
$ns_ at $opt(stop).1 "puts \"NS EXITING...\" ; $ns_ halt"
$ns_ at $opt(stop) "stop"

if { $opt(sc) == "" } {
	puts "*** NOTE: no scenario file specified."
        set opt(sc) "none"
} else {
	puts "Loading scenario file..."
	source $opt(sc)
	puts "Load complete..."
}

puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) rp $opt(rp)"
puts $tracefd "M 0.0 sc $opt(sc) cp $opt(cp) seed $opt(seed)"
puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run

proc stop {} {
    global ns_ f nf
    $ns_ flush-trace
    close $f
    close $nf
}
