# Copyright (c) 2000 by the University of Southern California
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
# All rights reserved.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#
#  The copyright of this module includes the following
#  linking-with-specific-other-licenses addition:
#
#  In addition, as a special exception, the copyright holders of
#  this module give you permission to combine (via static or
#  dynamic linking) this module with free software programs or
#  libraries that are released under the GNU LGPL and with code
#  included in the standard release of ns-2 under the Apache 2.0
#  license or under otherwise-compatible licenses with advertising
#  requirements (or modified versions of such code, with unchanged
#  license).  You may copy and distribute such a system following the
#  terms of the GNU GPL for this module and the licenses of the
#  other code concerned, provided that you include the source code of
#  that other code when and as the GNU GPL requires distribution of
#  source code.
#
#  Note that people who make modified versions of this module
#  are not obligated to grant this special exception for their
#  modified versions; it is their choice whether to do so.  The GNU
#  General Public License gives permission to release a modified
#  version without this exception; this exception also makes it
#  possible to release a modified version which carries forward this
#  exception.
#
# Yuan Li(USC/ISI)
# 
#
#
# run SMAC in a 10-nodes multihop network with DSR routing

# remove-all-packet-headers       ; # removes all except common
# add-packet-header Flags IP TCP Diffusion ARP LL Mac SR
# hdrs reqd for validation test

set opt(chan)           Channel/WirelessChannel
set opt(prop)           Propagation/TwoRayGround
set opt(netif)          Phy/WirelessPhy
set opt(mac)            Mac/SMAC
set opt(ifq)            Queue/DropTail/PriQueue
set opt(ll)             LL
set opt(ant)            Antenna/OmniAntenna
set opt(x)              2000              ;# X dimension of the topography
set opt(y)              2000               ;# Y dimension of the topography
set opt(ifqlen)         50                ;# max packet in ifq
set opt(seed)           0.0
set opt(tr)             temp.rands        ;# trace file
set opt(adhocRouting)   DSR
set opt(filters)        GradientFilter
set opt(nn)             10                ;# how many nodes are simulated
set opt(stop)           1000.0            ;# simulation time
set opt(syncFlag)       1                 ;# is set to 1 when SMAC uses sleep-wakeup cycle
set opt(selfConfigFlag) 1                 ;# is set to 0 when SMAC uses user configurable schedule start time
set val(initialenergy)  100
set val(radiomodel)     Radio/Simple      ;# generic radio hardware
set val(receivepower)   .5                ;# Receiving Power
set val(transmitpower)  .5                ;# Transmitting Power
set val(idlepower)      .05               ;# Idle Power
                                                                                                                                                            
                                                                                                                                                            
# =====================================================================
# Other default settings
                                                                                                                                                            
LL set mindelay_                50us
LL set delay_                   25us
LL set bandwidth_               0         ;
                                                                                                                                                            
Agent/Null set sport_           0
Agent/Null set dport_           0
                                                                                                                                                            
Agent/CBR set sport_            0
Agent/CBR set dport_            0
                                                                                                                                                            
Agent/TCPSink set sport_        0
Agent/TCPSink set dport_        0
                                                                                                                                                            
Agent/TCP set sport_            0
Agent/TCP set dport_            0
Agent/TCP set packetSize_       512
                                                                                                                                                            
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
                                                                                                                                                            
# SMAC settings
Mac/SMAC set syncFlag_ 1        ; # sleep wakeup cycles
Mac/SMAC set SMAC_DUTY_CYCLE 10

ErrorModel set rate_ 0.1
                                                                                                                                                            
proc UniformErrorProc {} {
    global opt
                                                                                                                                                            
    set errObj [new ErrorModel]
    $errObj unit packet
    return $errObj
}
                                                                                                                                                            
Class TestSuite

Class Test/smac-multihop -superclass TestSuite

proc usage {} {
    global argv0
    puts stderr "usage: ns $argv0 <tests>"
    puts "Valid tests: smac-multihop"
}

Test/smac-multihop instproc init {} {
                       
    global opt                                                                                                                                      
# create simulator instance
    $self instvar ns_ 
    set ns_ [new Simulator]
                                                                                                                                                            
# set wireless topography object
    $self instvar wtopo
    set wtopo [new Topography]
                                                                                                                                                            
# create trace object for ns
    $self instvar tracefd
    set tracefd [open $opt(tr) w]
                                                                                                                                                            
    $ns_ trace-all $tracefd
                                                                                                                                                            
# define topology
    $wtopo load_flatgrid $opt(x) $opt(y)
                                                                                                                                                            
#
# Create God
#
    $self instvar god_
    set god_ [create-god $opt(nn)]
                                                                                                                                                            
#
# define how node should be created
#
                                                                                                                                                            
# node setting
                                                                                                                                                            
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
                 -llType $opt(ll) \
                 -macType $opt(mac) \
                 -ifqType $opt(ifq) \
                 -ifqLen $opt(ifqlen) \
                 -antType $opt(ant) \
                 -propType $opt(prop) \
                 -phyType $opt(netif) \
                 -channelType $opt(chan) \
                 -topoInstance $wtopo \
                 -diffusionFilter $opt(filters) \
                 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON
               #  -energyModel "EnergyModel" \
               #  -initialEnergy $val(initialenergy) \
               #  -rxPower $val(receivepower) \
               #  -txPower $val(transmitpower) \
               #  -idlePower $val(idlepower)
                                                                                                                                                            
#
#  Create the specified number of nodes [$opt(nn)] and "attach" them
#  to the channel.
                                                                                                                                                            
    for {set i 0} {$i < $opt(nn) } {incr i} {
        set node_($i) [$ns_ node $i]
        $node_($i) random-motion 0          ;# disable random motion
    }

#
#  Create DSR Agent
#  rt_rq_max_period indicates maximum time between rt reqs
#  rt_rq_period indicates length of one backoff period
#  send_timeout indicates how long a packet can live in sendbuf

for {set i 0} {$i < $opt(nn)} {incr i} {
    set dsr($i) [new Agent/DSRAgent]
    $dsr($i)  node $node_($i)
    $dsr($i) rt_rq_max_period 100
    $dsr($i) rt_rq_period 30
    $dsr($i) send_timeout 300
}

#
#  Create the topology
#
                                                                                                                                                            
$node_(0) set Z_ 0.0
$node_(0) set Y_ 50.0
$node_(0) set X_ 100.0
$node_(1) set Z_ 0.0
$node_(1) set Y_ 250.0
$node_(1) set X_ 100.0
$node_(2) set Z_ 0.0
$node_(2) set Y_ 450.0
$node_(2) set X_ 100.0
$node_(3) set Z_ 0.0
$node_(3) set Y_ 650.0
$node_(3) set X_ 100.0
$node_(4) set Z_ 0.0
$node_(4) set Y_ 850.0
$node_(4) set X_ 100.0
$node_(5) set Z_ 0.0
$node_(5) set Y_ 1050.0
$node_(5) set X_ 100.0
$node_(6) set Z_ 0.0
$node_(6) set Y_ 1250.0
$node_(6) set X_ 100.0
$node_(7) set Z_ 0.0
$node_(7) set Y_ 1450.0
$node_(7) set X_ 100.0
$node_(8) set Z_ 0.0
$node_(8) set Y_ 1650.0
$node_(8) set X_ 100.0
$node_(9) set Z_ 0.0
$node_(9) set Y_ 1850.0
$node_(9) set X_ 100.0
                                                                                                                                                            
#
# Define traffic model
#
puts "Loading connection pattern..."
                                                                                                                                                            
set udp_(0) [new Agent/UDP]
$ns_ attach-agent $node_(0) $udp_(0)
set null_(0) [new Agent/Null]
$ns_ attach-agent $node_(9) $null_(0)
set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize_ 50
# Interval is measured in seconds
$cbr_(0) set interval_ 200
$cbr_(0) set random_ 1
$cbr_(0) set maxpkts_ 10000
$cbr_(0) set fid_ 0
$cbr_(0) attach-agent $udp_(0)
$ns_ connect $udp_(0) $null_(0)
$ns_ at 100.0 "$cbr_(0) start"
                                                                                                                                                            
#
# Tell nodes when the simulation ends
#
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
                                                                                                                                                            
$ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
}

Test/smac-multihop instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}

proc runtest {arg} {
                                                                                                                                                            
    set b [llength $arg]
    if {$b == 1} {
        set test $arg
    } else {
        usage
    }
    set test "smac-multihop"
    set t [new Test/$test]
    $t run
}
                                                                                                                                                            
runtest $argv
