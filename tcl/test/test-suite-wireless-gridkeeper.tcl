#
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
Mac/802_11 set bugFix_timer_ false;     # default changed 2006/1/30

# The default is being changed to 1
# Copyright (c) 1998 University of Southern California.
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


# This test suite is for validating wireless lans (CMU extension)
# with gridkeeper 
# To run all tests: test-all-wireless-gridkeeper
#
# To view a list of available test to run with this script:
# ns test-suite-wireless-gridkeeper.tcl
#
#

Class TestSuite

Class Test/dsdv -superclass TestSuite
# wireless model using destination sequence distance vector

#Class Test/dsr -superclass TestSuite
# wireless model using dynamic source routing


proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: dsdv dsr"
	exit 1
}


proc default_options {} {
	global opt

	set opt(chan)		Channel/WirelessChannel
	set opt(prop)		Propagation/TwoRayGround
	set opt(netif)		Phy/WirelessPhy
	set opt(mac)		Mac/802_11
        set opt(ifq)		Queue/DropTail/PriQueue
	set opt(ll)		LL
	set opt(ant)            Antenna/OmniAntenna
	set opt(x)		670 ;# X dimension of the topography
	set opt(y)		670;# Y dimension of the topography
	set opt(cp)		"../mobility/scene/cbr-50-20-4-512" ;# connection pattern file
	set opt(sc)		"../mobility/scene/scen-670x670-50-600-20-0" ;# scenario file
	set opt(ifqlen)		50	      ;# max packet in ifq
	set opt(nn)		50	      ;# number of nodes
	set opt(seed)		0.0
	set opt(stop)		500.0	      ;# simulation time
	set opt(tr)		temp.rands    ;# trace file
	set opt(lm)             "off"          ;# log movement

        set opt(radius)         150            ;# node comm. range
        set opt(nam)            gkeeper.nam    ;# nam trace for node position

}


# =====================================================================
# Other default settings

set AgentTrace			ON
set RouterTrace			OFF
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

# =====================================================================

TestSuite instproc init {} {
	global opt tracefd topo chan prop 
	global node_ god_ gkeeper
	$self instvar ns_ testName_
	set ns_         [new Simulator]
	set chan	[new $opt(chan)]
	set prop	[new $opt(prop)]
	set topo	[new Topography]
	set tracefd	[open $opt(tr) w]

	set opt(rp) $testName_
	$topo load_flatgrid $opt(x) $opt(y)
	$prop topography $topo
	#
	# Create God
	#
	$self create-god $opt(nn)
	
	#
	# log the mobile nodes movements if desired
	#
	if { $opt(lm) == "on" } {
		$self log-movement
	}
	source ../mobility/$testName_.tcl
	for {set i 0} {$i < $opt(nn) } {incr i} {
		$testName_-create-mobile-node $i
	}
	puts "Loading connection pattern..."
	source $opt(cp)
	
	#
	# Tell all the nodes when the simulation ends
	#
	for {set i 0} {$i < $opt(nn) } {incr i} {
		$ns_ at $opt(stop).000000001 "$node_($i) reset";
	}

	$ns_ at $opt(stop).000000001 "puts \"NS EXITING...\" ;" 
	#$ns_ halt"
	$ns_ at $opt(stop).1 "$self finish"
	puts "Loading scenario file..."
	source $opt(sc)
	puts "Load complete..."

        #enable node trace in nam
        
	set nf [open $opt(nam) w]
	$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)
	
        for {set i 0} {$i < $opt(nn)} {incr i} {
             $node_($i) namattach $nf
             # 20 defines the node size in nam, 
	     # must adjust it according to your scenario
             $ns_ initial_node_pos $node_($i) 20
        }
 
	#       
	# Create GridKeeper: OPTIONAL
	#

	$self create_gridkeeper

	puts $tracefd "M 0.0 nn:$opt(nn) x:$opt(x) y:$opt(y) rp:$opt(rp)"
	puts $tracefd "M 0.0 sc:$opt(sc) cp:$opt(cp) seed:$opt(seed)"
	puts $tracefd "M 0.0 prop:$opt(prop) ant:$opt(ant)"
}


TestSuite instproc finish {} {
	$self instvar ns_
	global quiet

	$ns_ flush-trace
        #if { !$quiet } {
        #        puts "running nam..."
        #        exec nam temp.rands.nam &
        #}
	puts "finishing.."
	exit 0
}

Test/dsdv instproc init {} {
    $self instvar ns_ testName_
    set testName_ dsdv
    $self next
}

Test/dsdv instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}

#Test/dsr instproc init {} {
	#$self instvar ns_ testName_ 
	#set testName_ dsr
	#$self next 
#}

#Test/dsr instproc run {} {
	#$self instvar ns_ 
	#puts "Starting Simulation..."
	#$ns_ run
	
#}

proc cmu-trace { ttype atype node } {
	global ns tracefd
    
        set ns [Simulator instance]
	if { $tracefd == "" } {
		return ""
	}
	set T [new CMUTrace/$ttype $atype]
	$T target [$ns set nullAgent_]
	$T attach $tracefd
        $T set src_ [$node id]

        $T node $node

	return $T
}

TestSuite instproc create-god { nodes } {
	global tracefd god_
	$self instvar ns_

	set god_ [new God]
	$god_ num_nodes $nodes
}

TestSuite instproc create_gridkeeper { } {

        global gkeeper opt node_
                
        set gkeeper [new GridKeeper]
        
        #initialize the gridkeeper
                
        $gkeeper dimension $opt(x) $opt(y)
 
        #
        # add mobile node into the gridkeeper, must be added after
        # scenario file
        #       


        for {set i 0} {$i < $opt(nn) } {incr i} {
	    $gkeeper addnode $node_($i)
        
	    $node_($i) radius $opt(radius)
        }       
        
	#dump grid info
	#$gkeeper dump
	
}

TestSuite instproc log-movement {} {
	global ns
	$self instvar logtimer_ ns_

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

	set logtimer_ [new LogTimer]
	$logtimer_ sched 0.1
}


proc runtest {arg} {
	global quiet
	set quiet 0

	set b [llength $arg]
	if {$b == 1} {
		set test $arg
	} elseif {$b == 2} {
		set test [lindex $arg 0]
		if {[lindex $arg 1] == "QUIET"} {
			set quiet 1
		}
	} else {
		usage
	}
	set t [new Test/$test]
	$t run
}

global argv arg0
default_options
runtest $argv









