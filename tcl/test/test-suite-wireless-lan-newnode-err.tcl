# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
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
# The default is being changed to 1
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-wireless-lan-newnode-err.tcl,v 1.7 2006/01/24 23:00:08 sallyfloyd Exp $

# This test suite is for validating wireless lans with transmission errors 
# To run all tests: test-all-wireless-lan
# to run individual test:
# ns test-suite-wireless-lan-newnode.tcl dsdv-uniform-err
# ns test-suite-wireless-lan-newnode.tcl dsdv-multistate-err


#
# To view a list of available test to run with this script:
# ns test-suite-wireless-lan.tcl

Class TestSuite

# wireless model using destination sequence distance vector
Class Test/dsdv-uniform-err -superclass TestSuite
Class Test/dsdv-multistate-err -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: dsdv-uniform-err dsdv-multistate-err"
	exit 1
}

proc default_options {} {
    global opt
    set opt(chan)	Channel/WirelessChannel
    set opt(prop)	Propagation/TwoRayGround
    set opt(netif)	Phy/WirelessPhy
    set opt(err)        UniformErrorProc
    set opt(FECstrength) 1
    set opt(unit)       pkt
    set opt(mac)	Mac/802_11
    set opt(ifq)	Queue/DropTail/PriQueue
    set opt(ll)		LL
    set opt(ant)        Antenna/OmniAntenna
    set opt(x)		670 ;# X dimension of the topography
    set opt(y)		670;# Y dimension of the topography
    set opt(ifqlen)	50	      ;# max packet in ifq
    set opt(seed)	0.0
    set opt(tr)		temp.rands    ;# trace file
    set opt(lm)         "off"          ;# log movement
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

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	1000

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
Phy/WirelessPhy set Pt_ 0.28183815
Phy/WirelessPhy set freq_ 914e+6 
Phy/WirelessPhy set L_ 1.0
Phy/WirelessPhy set bandwidth_ 1e5

# Initialize the Errormodel

ErrorModule set debug_ false

ErrorModel set enable_ 1
ErrorModel set markecn_ false
ErrorModel set bandwidth_ 2Mb
ErrorModel set rate_ 0.00001

ErrorModel/Trace set good_ 123456789
ErrorModel/Trace set loss_ 0
ErrorModel/Periodic set period_ 3.0
ErrorModel/Periodic set offset_ 0.0
ErrorModel/Periodic set burstlen_ 0.0
ErrorModel/MultiState set curperiod_ 0.0
ErrorModel/MultiState set sttype_ pkt
ErrorModel/MultiState set texpired_ 0

# =====================================================================

TestSuite instproc init {} {
	global opt tracefd topo chan prop 
	global node_ god_ 
	$self instvar ns_ testName_
	set ns_         [new Simulator]

	if {[string compare $testName_ "dsdv-uniform-err"] && \
			[string compare $testName_ "dsdv-multistate-err"]} {
		$ns_ node-config -addressType hierarchical
		AddrParams set domain_num_ 3
		lappend cluster_num 2 1 1
		AddrParams set cluster_num_ $cluster_num
		lappend eilastlevel 1 1 4 1
		AddrParams set nodes_num_ $eilastlevel
	}
	set topo	[new Topography]
	set tracefd	[open $opt(tr) w]

	set namfd       [open "wireless.nam" w]	
	
	$ns_ trace-all $tracefd
	$ns_ namtrace-all $namfd

        # ns-random $opt(seed)

	$topo load_flatgrid $opt(x) $opt(y)
	
	puts $tracefd "M 0.0 nn:$opt(nn) x:$opt(x) y:$opt(y) rp:$opt(rp)"
	puts $tracefd "M 0.0 sc:$opt(sc) seed:$opt(seed)"
	puts $tracefd "M 0.0 prop:$opt(prop) ant:$opt(ant)"

	set god_ [create-god $opt(nn)]
}

Test/dsdv-uniform-err instproc init {} {
    global opt node_ god_ chan topo 
    $self instvar ns_ testName_
    set testName_       dsdv-uniform-err
    set opt(rp)         dsdv
    set opt(sc)		"../mobility/scene/scen-3-test" 
    set opt(nn)		3	      
    set opt(stop)       100.0
    
    $self next
    
    $ns_ node-config -adhocRouting DSDV \
                         -llType $opt(ll) \
                         -macType $opt(mac) \
                         -ifqType $opt(ifq) \
                         -ifqLen $opt(ifqlen) \
                         -antType $opt(ant) \
                         -propType $opt(prop) \
                         -phyType $opt(netif) \
			 -channel [new $opt(chan)] \
			 -topoInstance $topo \
                        -agentTrace ON \
                         -routerTrace OFF \
                         -macTrace OFF \
                         -movementTrace OFF \
			 -IncomingErrProc $opt(err) -OutgoingErrProc $opt(err) 

    for {set i 0} {$i < $opt(nn) } {incr i} {
                set node_($i) [$ns_ node]
                $node_($i) random-motion 0              ;# disable random motion
    }

    $self create-tcp-traffic 1 $node_(0) $node_(1) 0

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

Test/dsdv-uniform-err instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}

Test/dsdv-multistate-err instproc init {} {
    global opt node_ god_ chan topo 
    $self instvar ns_ testName_
    set testName_       dsdv-multistate-err
    set opt(rp)         dsdv
    set opt(sc)		"../mobility/scene/scen-3-test" 
    set opt(nn)		3	      
    set opt(stop)       100.0
    
    $self next

    $ns_ node-config -adhocRouting DSDV \
                         -llType $opt(ll) \
                         -macType $opt(mac) \
                         -ifqType $opt(ifq) \
                         -ifqLen $opt(ifqlen) \
                         -antType $opt(ant) \
                         -propType $opt(prop) \
                         -phyType $opt(netif) \
			 -channel [new $opt(chan)] \
			 -topoInstance $topo \
                        -agentTrace ON \
                         -routerTrace OFF \
                         -macTrace OFF \
                         -movementTrace OFF \
			 -IncomingErrProc MultistateErrorProc \
			 -OutgoingErrProc MultistateErrorProc

    for {set i 0} {$i < $opt(nn) } {incr i} {
                set node_($i) [$ns_ node]
                $node_($i) random-motion 0              ;# disable random motion
    }
    $self create-tcp-traffic 1 $node_(0) $node_(1) 0
    
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

Test/dsdv-multistate-err instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}


TestSuite instproc finish-basenode {} {
	$self instvar ns_
	global quiet opt

	$ns_ flush-trace
        
        set tracefd	[open $opt(tr) r]
        set tracefd2    [open $opt(tr).w w]

        while { [eof $tracefd] == 0 } {

	    set line [gets $tracefd]
	    set items [split $line " "]
	    if { [lindex $items 0] == "M" } {
		puts $tracefd2 $line
	    } else {
		if { [llength $items] > 15} {
		    puts $tracefd2 $line
		}
	    }
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

TestSuite instproc create-tcp-traffic {id src dst start} {
    $self instvar ns_
    set tcp_($id) [new Agent/TCP]
    $tcp_($id) set class_ 2
    set sink_($id) [new Agent/TCPSink]
    $ns_ attach-agent $src $tcp_($id)
    $ns_ attach-agent $dst $sink_($id)
    $ns_ connect $tcp_($id) $sink_($id)
    set ftp_($id) [new Application/FTP]
    $ftp_($id) attach-agent $tcp_($id)
    $ns_ at $start "$ftp_($id) start"
    
}


TestSuite instproc create-udp-traffic {id src dst start} {
    $self instvar ns_
    set udp_($id) [new Agent/UDP]
    $ns_ attach-agent $src $udp_($id)
    set null_($id) [new Agent/Null]
    $ns_ attach-agent $dst $null_($id)
    set cbr_($id) [new Application/Traffic/CBR]
    $cbr_($id) set packetSize_ 512
    $cbr_($id) set interval_ 4.0
    $cbr_($id) set random_ 1
    $cbr_($id) set maxpkts_ 10000
    $cbr_($id) attach-agent $udp_($id)
    $ns_ connect $udp_($id) $null_($id)
    $ns_ at $start "$cbr_($id) start"
}

proc UniformErrorProc {} {
    global opt
	
    set	errObj [new ErrorModel]
    $errObj unit bit
    $errObj FECstrength $opt(FECstrength) 
    $errObj datapktsize 1000
    $errObj cntrlpktsize 80
    return $errObj
}

proc MultistateErrorProc {} {
    set tmp [new ErrorModel/Uniform 0 pkt]
    set tmp1 [new ErrorModel/Uniform .9 pkt]
    set tmp2 [new ErrorModel/Uniform .5 pkt]
    set m_states [list $tmp $tmp1 $tmp2]
    set m_periods [list 0 .0075 .00375]
    set m_transmx {{0.95 0.05 0} {0 0 1} {1 0 0}}
    set m_trunit pkt
    set m_sttype time
    set m_nstates 3
    set m_nstart [lindex $m_states 0]

    set errObj [new ErrorModel/MultiState $m_states $m_periods $m_transmx $m_trunit $m_sttype $m_nstates $m_nstart]

    $errObj FECstrength 2
    return $errObj
}

proc runtest {arg} {
	global quiet tracefd opt
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









