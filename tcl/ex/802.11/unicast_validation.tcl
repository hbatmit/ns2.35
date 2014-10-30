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

# Parameters: seed

set val(seed) [lindex $argv 0]

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

Phy/WirelessPhyExt set CSThresh_           6.30957e-12
Phy/WirelessPhyExt set Pt_                 0.001
Phy/WirelessPhyExt set freq_               5.18e9
Phy/WirelessPhyExt set noise_floor_        2.51189e-13
Phy/WirelessPhyExt set L_                  1.0
Phy/WirelessPhyExt set PowerMonitorThresh_ 2.10319e-12
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

Propagation/Nakagami set use_nakagami_dist_ false
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

#=======================================================================


set val(chan)       Channel/WirelessChannel
set val(prop)       Propagation/TwoRayGround

set val(netif)      Phy/WirelessPhyExt
set val(mac)        Mac/802_11Ext
set val(ifq)        Queue/DropTail/PriQueue
set val(ll)         LL
set val(ant)        Antenna/OmniAntenna
set val(x)          50   	;# X dimension of the topography
set val(y)          50   	;# Y dimension of the topography
set val(ifqlen)     20           ;# max packet in ifq
set val(nn)         3         ;# how many nodes are simulated
set val(rtg)        DumbAgent
set val(stop)       5         ;# simulation time
# =====================================================================
# Main Program
# ======================================================================

#
# Initialize Global Variables
#

global defaultRNG
$defaultRNG seed $val(seed)

set ns_		[new Simulator]
set topo	[new Topography]
set tracefd	stdout
$ns_ trace-all $tracefd
#$ns_ use-newtrace

$topo load_flatgrid $val(x) $val(y)
set god_ [create-god $val(nn)]
$god_ off

set chan [new $val(chan)]
$ns_ node-config -adhocRouting $val(rtg) \
                 -llType $val(ll) \
                 -macType $val(mac) \
                 -ifqType $val(ifq) \
                 -ifqLen $val(ifqlen) \
                 -antType $val(ant) \
                 -propType $val(prop) \
                 -phyType $val(netif) \
                 -channel $chan \
		 -topoInstance $topo \
		 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace ON \
                -phyTrace ON

for {set i 0} {$i < $val(nn) } {incr i} {
    set ID_($i) $i
    set vehicle_($i) [$ns_ node]
    $vehicle_($i) set id_  $ID_($i)
    $vehicle_($i) set address_ $ID_($i)
    $vehicle_($i) nodeid $ID_($i)

    set agent_($i) [new Agent/PBC]
    $ns_ attach-agent $vehicle_($i)  $agent_($i)
    $agent_($i) set Pt_ 2.0e-4
    $agent_($i) set payloadSize 1000
    $agent_($i) set modulationScheme 0
    $agent_($i) PeriodicBroadcast OFF
    $ns_ at $val(stop).0 "$vehicle_($i) reset";

}

$vehicle_(0) set X_ 10.00
$vehicle_(0) set Y_ 10.00
$vehicle_(0) set Z_ 10.00
$vehicle_(1) set X_ 20.00.
$vehicle_(1) set Y_ 10.00
$vehicle_(1) set Z_ 10.00
$vehicle_(2) set X_ 30.00.
$vehicle_(2) set Y_ 10.00
$vehicle_(2) set Z_ 10.00

$ns_ at 0.01 " puts \" ------------------------- \" "
$ns_ at 0.01 "puts \"node 1 broadcast\" "
$ns_ at 0.0101 "$agent_(1) singleBroadcast"


$ns_ at 1.0 " puts \" ------------------------- \" "
$ns_ at 1.0 "puts \"node 1 unicast to node 2\""
$ns_ at 1.01  "$agent_(1) unicast 2"
$ns_ at 1.99 " puts \"Note: one Data sent, one Data recved, one Ack sent\" "

$ns_ at 2.0 " puts \" ------------------------- \" "
$ns_ at 2.0 "puts \"node 1 unicast to node 3(not exist)\""
$ns_ at 2.01  "$agent_(1) unicast 3"
$ns_ at 2.99 " puts \"Note: altogether 8 Data sent, one plus seven reties, then retry over limit, Data was dropped for no return ACK \" "

$ns_ at 3.0 " puts \" ------------------------- \" "
$ns_ at 3.0 "puts \"data size > RTSThreshold, using RTS/CTS \""
$ns_ at 3.01 "$agent_(1) set payloadSize 2500"
$ns_ at 3.02 "$agent_(1)  unicast 2"
$ns_ at 3.99 " puts \"Note: RTS sent first, RTS recved, CTS sent followed, CTS recv, DATA sent, DATA recved, finally ACK sent \" "

$ns_ at 3.99 " puts \"Note: MAC control message does not possess a valid uid, Only 4 data messages were sent in this script \" "


for {set i 0} {$i < $val(nn) } {incr i} {
    $ns_ at $val(stop).0 "$vehicle_($i) reset";
}
$ns_ at $val(stop).0002 "puts \"NS EXITING...\" ; $ns_ halt"
$ns_ at $val(stop).0003 "$ns_ flush-trace"
puts "Starting Simulation..."
$ns_ run
