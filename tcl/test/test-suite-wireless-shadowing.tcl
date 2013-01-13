                                                                             #
# Copyright (c) 1998,2000 University of Southern California.
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


# This test suite is for validating shadowing propagation model
# To run all tests: test-all-wireless-shadowing
# to run individual test:
# ns test-suite-wireless-shadowing.tcl dsdv
# ns test-suite-wireless-shadowing.tcl dsr
#
# To view a list of available test to run with this script:
# ns test-suite-wireless-shadowing.tcl

Mac/802_11 set bugFix_timer_ false;     # default changed 2006/1/30

Class TestSuite

# wireless model using destination sequence distance vector
Class Test/dsdv -superclass TestSuite

# wireless model using dynamic source routing
Class Test/dsr -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: dsdv dsr"
	exit 1
}


proc default_options {} {
	global opt
	set opt(chan)	Channel/WirelessChannel
	set opt(prop)	Propagation/Shadowing
	set opt(netif)	Phy/WirelessPhy
	set opt(mac)	Mac/802_11
	set opt(ifq)	Queue/DropTail/PriQueue
	set opt(ll)	LL
	set opt(ant)    Antenna/OmniAntenna
	set opt(x)	670 ;# X dimension of the topography
	set opt(y)	670;# Y dimension of the topography
	set opt(ifqlen)	50	      ;# max packet in ifq
	set opt(seed)	0.0
	set opt(tr)	temp.rands    ;# trace file
	set opt(lm)	"off"          ;# log movement

}


# =====================================================================
# Other default settings

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

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
Phy/WirelessPhy set bandwidth_ 2e6
Phy/WirelessPhy set Pt_ 0.28183815
Phy/WirelessPhy set freq_ 914e+6
Phy/WirelessPhy set L_ 1.0

# =====================================================================

TestSuite instproc init {} {
	global opt tracefd topo chan prop_
	global node_ god_
	$self instvar ns_ testName_
	set ns_         [new Simulator]
	if {[string compare $testName_ "dsdv"] && \
			[string compare $testName_ "dsr"]} {
		$ns_ node-config -addressType hierarchical
		AddrParams set domain_num_ 3
		lappend cluster_num 2 1 1
		AddrParams set cluster_num_ $cluster_num
		lappend eilastlevel 1 1 4 1
		AddrParams set nodes_num_ $eilastlevel
	}
	set topo	[new Topography]
	set tracefd	[open $opt(tr) w]
	
	set prop_	[new $opt(prop)]
	$prop_ set pathlossExp_ 2.0
	$prop_ set std_db_ 4.0
	$prop_ set dist0_ 1.0
	$prop_ seed predef 0
	
	$ns_ trace-all $tracefd
	
	$topo load_flatgrid $opt(x) $opt(y)
	
	puts $tracefd "M 0.0 nn:$opt(nn) x:$opt(x) y:$opt(y) rp:$opt(rp)"
	puts $tracefd "M 0.0 sc:$opt(sc) cp:$opt(cp) seed:$opt(seed)"
	puts $tracefd "M 0.0 prop:$opt(prop) ant:$opt(ant)"

	set god_ [create-god $opt(nn)]
}

Test/dsdv instproc init {} {
    global opt node_ god_ chan topo prop_
    $self instvar ns_ testName_
    set testName_       dsdv
    set opt(rp)         dsdv
    set opt(cp)		"../mobility/scene/cbr-50-20-4-512"
    set opt(sc)		"../mobility/scene/scen-670x670-50-600-20-0" ;
    set opt(nn)		50	
    set opt(stop)       500.0

    $self next

    $ns_ node-config -adhocRouting DSDV \
                         -llType $opt(ll) \
                         -macType $opt(mac) \
                         -ifqType $opt(ifq) \
                         -ifqLen $opt(ifqlen) \
                         -antType $opt(ant) \
                         -propInstance $prop_ \
                         -phyType $opt(netif) \
			 -channel [new $opt(chan)] \
			 -topoInstance $topo \
                         -agentTrace ON \
                         -routerTrace OFF \
                         -macTrace OFF \
                         -movementTrace OFF

    for {set i 0} {$i < $opt(nn) } {incr i} {
                set node_($i) [$ns_ node]
                $node_($i) random-motion 0          ;# disable random motion
    }
    puts "Loading connection pattern..."
    source $opt(cp)

    puts "Loading scenario file..."
    source $opt(sc)
    puts "Load complete..."

    #
    # Tell all the nodes when the simulation ends
    #
    for {set i 0} {$i < $opt(nn) } {incr i} {
	$ns_ at $opt(stop).000000001 "$node_($i) reset";
    }

    $ns_ at $opt(stop).000000001 "puts \"NS EXITING...\" ;"
    $ns_ at $opt(stop).1 "$self finish"
}

Test/dsdv instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}

Test/dsr instproc init {} {
    global opt node_ god_ chan topo prop_
    $self instvar ns_ testName_
    set testName_       dsr
    set opt(rp)         dsr
    set opt(ifq)        CMUPriQueue
    set opt(cp)         "../mobility/scene/cbr-50-20-4-512"
    set opt(sc)         "../mobility/scene/scen-670x670-50-600-20-0" ;
    set opt(nn)         50
    set opt(stop)       500.0

    $self next

    $ns_ node-config -adhocRouting $opt(rp) \
                         -llType $opt(ll) \
                         -macType $opt(mac) \
                         -ifqType $opt(ifq) \
                         -ifqLen $opt(ifqlen) \
                         -antType $opt(ant) \
                         -propInstance $prop_ \
                         -phyType $opt(netif) \
			 -channel [new $opt(chan)] \
			 -topoInstance $topo \
                         -agentTrace ON \
                         -routerTrace OFF \
                         -macTrace OFF \
                         -movementTrace OFF

    for {set i 0} {$i < $opt(nn) } {incr i} {
                set node_($i) [$ns_ node]
                $node_($i) random-motion 0          ;# disable random motion
    }
    puts "Loading connection pattern..."
    source $opt(cp)

    puts "Loading scenario file..."
    source $opt(sc)
    puts "Load complete..."

    #
    # Tell all the nodes when the simulation ends
    #
    for {set i 0} {$i < $opt(nn) } {incr i} {
	$ns_ at $opt(stop).000000001 "$node_($i) reset";
    }

    $ns_ at $opt(stop).000000001 "puts \"NS EXITING...\" ;"
    $ns_ at $opt(stop).1 "$self finish"
}

Test/dsr instproc run {} {
	$self instvar ns_
	puts "Starting Simulation..."
	$ns_ run
}

TestSuite instproc finish-dsr {} {
	$self instvar ns_
	global quiet opt tracefd

	$ns_ flush-trace
	flush $tracefd
	close $tracefd

        set tracefd	[open $opt(tr) r]
        set tracefd2    [open $opt(tr).w w]

        while { [eof $tracefd] == 0 } {

	    set line [gets $tracefd]
	    set items [split $line " "]

	    set time [lindex $items 1]
	
	    set times [split $time "."]
	    set time1 [lindex $times 0]
	    set time2 [lindex $times 1]
	    set newtime2 [string range $time2 0 3]
	    set time $time1.$newtime2
	
	    set newline [lreplace $line 1 1 $time]

	    puts $tracefd2 $newline

	}
	
	close $tracefd
	close $tracefd2

	exec mv $opt(tr).w $opt(tr)
	
	puts "finishing.."
	exit 0
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



