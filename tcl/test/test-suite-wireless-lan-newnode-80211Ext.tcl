# Copyright (C) 2007
# Mercedes-Benz Research & Development North America, Inc. and
# University of Karlsruhe (TH)
# All rights reserved.
#
# Qi Chen                 : qi.chen@daimler.com
# Felix Schmidt-Eisenlohr : felix.schmidt-eisenlohr@kit.edu
# Daniel Jiang            : daniel.jiang@daimler.com
# 
# For further information see:
# http://dsn.tm.uni-karlsruhe.de/english/Overhaul_NS-2.php

# This test suite is for validating the overhauled implementation of IEEE 802.11 
# To run all tests: test-all-wireless-lan
# to run individual test:
# ns test-suite-wireless-lan-newnode-80211Ext.tcl unicast
# ns test-suite-wireless-lan-newnode-80211Ext.tcl broadcast
#
# To view a list of available test to run with this script:
# ns test-suite-wireless-lan-newnode-80211Ext.tcl

Class TestSuite

# 802.11Ext with unicast
Class Test/unicast -superclass TestSuite

# 802.11Ext with broadcast
Class Test/broadcast -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: unicast broadcast"
	exit 1
}

proc default_options {} {
    global opt
    set opt(seed) 1
    set opt(chan)       Channel/WirelessChannel
    set opt(prop)       Propagation/Nakagami

    set opt(netif)      Phy/WirelessPhyExt
    set opt(mac)        Mac/802_11Ext
    set opt(ifq)        Queue/DropTail/PriQueue
    set opt(ll)         LL
    set opt(ant)        Antenna/OmniAntenna
    set opt(x)          50   	;# X dimension of the topography
    set opt(y)          50   	;# Y dimension of the topography
    set opt(ifqlen)     20           ;# max packet in ifq
    set opt(rtg)        DumbAgent
    set opt(stop)       5         ;# simulation time
    set opt(tr)         temp.rands    ;# trace file
}


# =====================================================================
# Other default settings

Mac/802_11 set CWMin_               15
Mac/802_11 set CWMax_               1023
Mac/802_11 set SlotTime_            0.000009
Mac/802_11 set SIFS_                0.000016
Mac/802_11 set ShortRetryLimit_     7
Mac/802_11 set LongRetryLimit_      4
Mac/802_11 set PreambleLength_      60
Mac/802_11 set PLCPHeaderLength_    60
Mac/802_11 set PLCPDataRate_        6.0e6
Mac/802_11 set RTSThreshold_        2000
Mac/802_11 set basicRate_           6.0e6
Mac/802_11 set dataRate_            6.0e6

Mac/802_11Ext set CWMin_            15
Mac/802_11Ext set CWMax_            1023
Mac/802_11Ext set SlotTime_         0.000009
Mac/802_11Ext set SIFS_             0.000016
Mac/802_11Ext set ShortRetryLimit_  7
Mac/802_11Ext set LongRetryLimit_   4
Mac/802_11Ext set HeaderDuration_   0.000020 
Mac/802_11Ext set SymbolDuration_   0.000004
Mac/802_11Ext set BasicModulationScheme_ 0
Mac/802_11Ext set use_802_11a_flag_ true
Mac/802_11Ext set RTSThreshold_     2000
Mac/802_11Ext set MAC_DBG           0

Phy/WirelessPhy set CSThresh_       6.30957e-12
Phy/WirelessPhy set Pt_             0.01
Phy/WirelessPhy set freq_           5.18e9
Phy/WirelessPhy set L_              1.0
Phy/WirelessPhy set RXThresh_       3.652e-10
Phy/WirelessPhy set bandwidth_      20e6
Phy/WirelessPhy set CPThresh_       10.0

Phy/WirelessPhyExt set CSThresh_           6.30957e-13
Phy/WirelessPhyExt set Pt_                 0.001
Phy/WirelessPhyExt set freq_               5.18e9
Phy/WirelessPhyExt set noise_floor_        2.51189e-13
Phy/WirelessPhyExt set L_                  1.0
Phy/WirelessPhyExt set PowerMonitorThresh_ 0
Phy/WirelessPhyExt set HeaderDuration_     0.000020
Phy/WirelessPhyExt set BasicModulationScheme_ 0
Phy/WirelessPhyExt set PreambleCaptureSwitch_ 1
Phy/WirelessPhyExt set DataCaptureSwitch_  0
Phy/WirelessPhyExt set SINR_PreambleCapture_ 2.5118
Phy/WirelessPhyExt set SINR_DataCapture_   100.0
Phy/WirelessPhyExt set trace_dist_         1e6
Phy/WirelessPhyExt set PHY_DBG_            0
Phy/WirelessPhyExt set CPThresh_           0 ;# not used at the moment
Phy/WirelessPhyExt set RXThresh_           0 ;# not used at the moment

#=====================================================================

#configure RF model parameters
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

Propagation/Nakagami set use_nakagami_dist_ true 
Propagation/Nakagami set gamma0_ 2.0
Propagation/Nakagami set gamma1_ 2.0
Propagation/Nakagami set gamma2_ 2.0

Propagation/Nakagami set d0_gamma_ 200
Propagation/Nakagami set d1_gamma_ 500

Propagation/Nakagami set m0_  1.0
Propagation/Nakagami set m1_  1.0
Propagation/Nakagami set m2_  1.0

Propagation/Nakagami set d0_m_ 80
Propagation/Nakagami set d1_m_ 200

# =====================================================================

TestSuite instproc init {} {
}

Test/unicast instproc init {} {
    global defaultRNG opt chan topo god_ node_
    $self instvar ns_ testName_
    set testName_ unicast
    set ns_		[new Simulator]
    
    set opt(nn)         3

    $defaultRNG seed $opt(seed)
    set topo	[new Topography]
    set tracefd [open $opt(tr) w]
    $ns_ trace-all $tracefd

    $topo load_flatgrid $opt(x) $opt(y)
    set god_ [create-god $opt(nn)]
    $god_ off

    set chan [new $opt(chan)]
    $ns_ node-config -adhocRouting $opt(rtg) \
                     -llType $opt(ll) \
                     -macType $opt(mac) \
                     -ifqType $opt(ifq) \
                     -ifqLen $opt(ifqlen) \
                     -antType $opt(ant) \
                     -propType $opt(prop) \
                     -phyType $opt(netif) \
                     -channel $chan \
            		 -topoInstance $topo \
            		 -agentTrace ON \
                     -routerTrace OFF \
                     -macTrace ON \
                     -phyTrace ON

    for {set i 0} {$i < $opt(nn) } {incr i} {
        set ID_($i) $i
        set node_($i) [$ns_ node]
        $node_($i) set id_  $ID_($i)
        $node_($i) set address_ $ID_($i)
        $node_($i) nodeid $ID_($i)

        set agent_($i) [new Agent/PBC]
        $ns_ attach-agent $node_($i)  $agent_($i)
        $agent_($i) set Pt_ 2.0e-4
        $agent_($i) set payloadSize 1000
        $agent_($i) set modulationScheme 0
        $agent_($i) PeriodicBroadcast OFF
        $ns_ at $opt(stop).0 "$node_($i) reset";
    }   

    $node_(0) set X_ 10.00
    $node_(0) set Y_ 10.00
    $node_(0) set Z_ 10.00
    $node_(1) set X_ 20.00.
    $node_(1) set Y_ 10.00
    $node_(1) set Z_ 10.00
    $node_(2) set X_ 30.00.
    $node_(2) set Y_ 10.00
    $node_(2) set Z_ 10.00

    $ns_ at 0.01 "$agent_(1) singleBroadcast"
    $ns_ at 1.01 "$agent_(1) unicast 2"
    $ns_ at 2.01 "$agent_(1) unicast 3"
    $ns_ at 3.01 "$agent_(1) set payloadSize 2500"
    $ns_ at 3.02 "$agent_(1) unicast 2"

    for {set i 0} {$i < $opt(nn) } {incr i} {
        $ns_ at $opt(stop).0 "$node_($i) reset";
    }
    $ns_ at $opt(stop).1 "$self finish"
}

Test/unicast instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}

Test/broadcast instproc init {} {
    global defaultRNG opt chan topo god_ node_
    $self instvar ns_ testName_
    set testName_ unicast
    set ns_		[new Simulator]

    set opt(nn) 20

    $defaultRNG seed $opt(seed)
    set topo	[new Topography]
    set tracefd [open $opt(tr) w]
    $ns_ trace-all $tracefd

    $topo load_flatgrid $opt(x) $opt(y)
    set god_ [create-god $opt(nn)]
    $god_ off

    set chan [new $opt(chan)]
    $ns_ node-config -adhocRouting $opt(rtg) \
                     -llType $opt(ll) \
                     -macType $opt(mac) \
                     -ifqType $opt(ifq) \
                     -ifqLen $opt(ifqlen) \
                     -antType $opt(ant) \
                     -propType $opt(prop) \
                     -phyType $opt(netif) \
                     -channel $chan \
		             -topoInstance $topo \
		             -agentTrace ON \
                     -routerTrace OFF \
                     -macTrace OFF \
                     -phyTrace OFF

    for {set i 0} {$i < $opt(nn) } {incr i} {
        set ID_($i) $i
        set node_($i) [$ns_ node]
        $node_($i) set id_  $ID_($i)
        $node_($i) set address_ $ID_($i)
        $node_($i) set X_ [expr $i * 5]
        $node_($i) set Y_ 10
        $node_($i) set Z_ 0
        $node_($i) nodeid $ID_($i)

        set agent_($i) [new Agent/PBC]
        $ns_ attach-agent $node_($i)  $agent_($i)
        $agent_($i) set Pt_ 1e-4
        $agent_($i) set payloadSize 1000
        $agent_($i) set peroidcaBroadcastInterval 0.2
        $agent_($i) set peroidcaBroadcastVariance 0.05
        $agent_($i) set modulationScheme 1
        $agent_($i) PeriodicBroadcast ON
        $ns_ at $opt(stop).0 "$node_($i) reset";
    }

    for {set i 0} {$i < $opt(nn) } {incr i} {
        $ns_ at $opt(stop).0 "$node_($i) reset";
    }
    $ns_ at $opt(stop).1 "$self finish"
}

Test/broadcast instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}

TestSuite instproc finish {} {
    $self instvar ns_
    $ns_ flush-trace
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

