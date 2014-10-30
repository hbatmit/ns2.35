#! /bin/sh
#
# Copyright (c) 2003-2004 Samsung Advanced Institute of Technology and
# The City University of New York. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of Samsung Advanced Institute of Technology nor of 
#    The City University of New York may be used to endorse or promote 
#    products derived from this software without specific prior written 
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE JOINT LAB OF SAMSUNG ADVANCED INSTITUTE
# OF TECHNOLOGY AND THE CITY UNIVERSITY OF NEW YORK ``AS IS'' AND ANY EXPRESS 
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
# NO EVENT SHALL SAMSUNG ADVANCED INSTITUTE OR THE CITY UNIVERSITY OF NEW YORK 
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP ARP LL Mac LRWPAN AODV ;
# hdrs reqd for validation

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Class TestSuite

Class Test/wpan -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <test> "
	exit 1
}

proc default_values {} {
	global val
	set val(chan)           Channel/WirelessChannel    ;# channel type
	set val(prop)           Propagation/TwoRayGround   ;# radio-propagation model
	set val(netif)          Phy/WirelessPhy/802_15_4
	set val(mac)            Mac/802_15_4
	set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
	set val(ll)             LL                         ;# link layer type
	set val(ant)            Antenna/OmniAntenna        ;# antenna model
	set val(ifqlen)         50                         ;# max packet in ifq
	set val(nn)             21                         ;# number of mobilenodes
	set val(rp)             AODV                       ;# routing
	set val(x)              50
	set val(y)              50
	set val(tr)             temp.rands                 ;# trace file
	set val(traffic)        mix                        ;# traffic (mix/cbr/poisson/ftp)

	# For propagation model 'TwoRayGround'
	set dist(5m)  7.69113e-06
	set dist(9m)  2.37381e-06
	set dist(10m) 1.92278e-06
	set dist(11m) 1.58908e-06
	set dist(12m) 1.33527e-06
	set dist(13m) 1.13774e-06
	set dist(14m) 9.81011e-07
	set dist(15m) 8.54570e-07
	set dist(16m) 7.51087e-07
	set dist(20m) 4.80696e-07
	set dist(25m) 3.07645e-07
	set dist(30m) 2.13643e-07
	set dist(35m) 1.56962e-07
	set dist(40m) 1.20174e-07
	Phy/WirelessPhy set CSThresh_ $dist(12m)
	Phy/WirelessPhy set RXThresh_ $dist(12m)
}

TestSuite instproc topology {} {
	global node_

	$node_(0) set X_ 20.0 
	$node_(0) set Y_ 25.0 
	$node_(0) set Z_ 0.0 

	$node_(1) set X_ 10.0 
	$node_(1) set Y_ 25.0
	$node_(1) set Z_ 0.0 

	$node_(2) set X_ 10.0 
	$node_(2) set Y_ 15.0 
	$node_(2) set Z_ 0.0 

	$node_(3) set X_ 10.0 
	$node_(3) set Y_ 5.0 
	$node_(3) set Z_ 0.0 

	$node_(4) set X_ 0.0 
	$node_(4) set Y_ 15.0 
	$node_(4) set Z_ 0.0 

	$node_(5) set X_ 0.0 
	$node_(5) set Y_ 25.0 
	$node_(5) set Z_ 0.0 

	$node_(6) set X_ 0.0 
	$node_(6) set Y_ 35.0 
	$node_(6) set Z_ 0.0
 
	$node_(7) set X_ 10.0 
	$node_(7) set Y_ 35.0 
	$node_(7) set Z_ 0.0 

	$node_(8) set X_ 10.0 
	$node_(8) set Y_ 45.0 
	$node_(8) set Z_ 0.0
 
	$node_(9) set X_ 20.0
	$node_(9) set Y_ 35.0 
	$node_(9) set Z_ 0.0
 
	$node_(10) set X_ 20.0 
	$node_(10) set Y_ 45.0 
	$node_(10) set Z_ 0.0 

	$node_(11) set X_ 30.0 
	$node_(11) set Y_ 35.0 
	$node_(11) set Z_ 0.0 

	$node_(12) set X_ 30.0 
	$node_(12) set Y_ 45.0 
	$node_(12) set Z_ 0.0 

	$node_(13) set X_ 30.0 
	$node_(13) set Y_ 25.0 
	$node_(13) set Z_ 0.0 

	$node_(14) set X_ 40.0 
	$node_(14) set Y_ 25.0 
	$node_(14) set Z_ 0.0 

	$node_(15) set X_ 40.0 
	$node_(15) set Y_ 35.0 
	$node_(15) set Z_ 0.0 

	$node_(16) set X_ 30.0 
	$node_(16) set Y_ 15.0 
	$node_(16) set Z_ 0.0 

	$node_(17) set X_ 40.0 
	$node_(17) set Y_ 15.0 
	$node_(17) set Z_ 0.0 

	$node_(18) set X_ 30.0 
	$node_(18) set Y_ 5.0 
	$node_(18) set Z_ 0.0 

	$node_(19) set X_ 20.0 
	$node_(19) set Y_ 15.0 
	$node_(19) set Z_ 0.0 

	$node_(20) set X_ 20.0 
	$node_(20) set Y_ 5.0 
	$node_(20) set Z_ 0.0
}

TestSuite instproc cbrtraffic { src dst interval starttime } {
	global node_
	$self instvar ns_
	set udp_($src) [new Agent/UDP]
	eval $ns_ attach-agent \$node_($src) \$udp_($src)
	set null_($dst) [new Agent/Null]
	eval $ns_ attach-agent \$node_($dst) \$null_($dst)
	set cbr_($src) [new Application/Traffic/CBR]
	eval \$cbr_($src) set packetSize_ 80
	eval \$cbr_($src) set interval_ $interval
	eval \$cbr_($src) set random_ 0
	#eval \$cbr_($src) set maxpkts_ 10000
	eval \$cbr_($src) attach-agent \$udp_($src)
	eval $ns_ connect \$udp_($src) \$null_($dst)
	$ns_ at $starttime "$cbr_($src) start"
}

TestSuite instproc poissontraffic { src dst interval starttime } {
	global node_
	$self instvar ns_
	set udp($src) [new Agent/UDP]
	eval $ns_ attach-agent \$node_($src) \$udp($src)
	set null($dst) [new Agent/Null]
	eval $ns_ attach-agent \$node_($dst) \$null($dst)
	set expl($src) [new Application/Traffic/Exponential]
	eval \$expl($src) set packetSize_ 70
	eval \$expl($src) set burst_time_ 0
	eval \$expl($src) set idle_time_ [expr $interval*1000.0-70.0/100]ms	;# idle_time + pkt_tx_time = interval
	eval \$expl($src) set rate_ 100k
	eval \$expl($src) attach-agent \$udp($src)
	eval $ns_ connect \$udp($src) \$null($dst)
	$ns_ at $starttime "$expl($src) start"
}

TestSuite instproc ftptraffic { src dst starttime } {
	global node_
	$self instvar ns_
	set tcp($src) [new Agent/TCP]
	eval \$tcp($src) set packetSize_ 60
	set sink($dst) [new Agent/TCPSink]
	eval $ns_ attach-agent \$node_($src) \$tcp($src)
	eval $ns_ attach-agent \$node_($dst) \$sink($dst)
	eval $ns_ connect \$tcp($src) \$sink($dst)
	set ftp($src) [new Application/FTP]
	eval \$ftp($src) attach-agent \$tcp($src)
	$ns_ at $starttime "$ftp($src) start"
}

TestSuite instproc init {} {
	global val tracefd topo
	$self instvar ns_

	set ns_		[new Simulator]
	set tracefd     [open ./$val(tr) w]
	$ns_ trace-all $tracefd
	set topo       [new Topography]
	$topo load_flatgrid $val(x) $val(y)
}

TestSuite instproc finish {} {
	$self instvar ns_
	global tracefd

	$ns_ flush-trace
	close $tracefd
	exit 0
}

Test/wpan instproc init {} {
	global val node_ god_ chan topo
	$self instvar ns_ testName_

	set testName_ wpan
	$self next

	set god_ [create-god $val(nn)]
	set chan [new $val(chan)]

	# configure node
	$ns_ node-config -adhocRouting $val(rp) \
			-llType $val(ll) \
			-macType $val(mac) \
			-ifqType $val(ifq) \
			-ifqLen $val(ifqlen) \
			-antType $val(ant) \
			-propType $val(prop) \
			-phyType $val(netif) \
			-topoInstance $topo \
			-agentTrace OFF \
			-routerTrace OFF \
			-macTrace ON \
			-movementTrace OFF \
	                #-energyModel "EnergyModel" \
	                #-initialEnergy 1 \
	                #-rxPower 0.3 \
	                #-txPower 0.3 \
			-channel $chan

	for {set i 0} {$i < $val(nn) } {incr i} {
		set node_($i) [$ns_ node]
		$node_($i) random-motion 0
	}

	$self topology

	# === actual test begins here =======================

	set appTime1         	10.3
	set appTime2         	10.6
	set stopTime            100

	# start PAN coordinator (beacon enabled):
	#  -- active channel scan and channel selection
	#  -- PAN coordinator startup
	#  -- beacon transmission
	$ns_ at 0.0	"$node_(0) sscs startPANCoord 1"	;# startPANCoord <txBeacon=1> <BO=3> <SO=3>
	
	# start non-beacon enabled coordinator:
	#  -- active channel scan
	#  -- channel selection and coordinator selection
	#  -- association
	#  -- beacon tracking and synchronization
	$ns_ at 0.3	"$node_(1) sscs startDevice 1 1" 	;# startDevice <isFFD=1> <assoPermit=1> <txBeacon=0> <BO=3> <SO=3>
	$ns_ at 1.3	"$node_(9) sscs startDevice 1 1"
	$ns_ at 1.7	"$node_(13) sscs startDevice 1 1"
	$ns_ at 2.3	"$node_(19) sscs startDevice 1 1"
	
	# start beacon enabled coordinator:
	#  -- active channel scan
	#  -- channel selection and coordinator selection
	#  -- association
	#  -- beacon tracking and synchronization
	#  -- beacon transmission
	$ns_ at 3.3	"$node_(2) sscs startDevice 1 1 1"
	$ns_ at 3.5	"$node_(7) sscs startDevice 1 1 1"
	$ns_ at 3.6	"$node_(11) sscs startDevice 1 1 1"
	$ns_ at 3.8	"$node_(16) sscs startDevice 1 1 1"
	
	# start RFD (leaf node)
	$ns_ at 4.3	"$node_(3) sscs startDevice 0 0"
	
	# start non-beacon enabled coordinator
	$ns_ at 4.5	"$node_(6) sscs startDevice 1 1"
	
	# start FFD not supporting association
	$ns_ at 4.8	"$node_(12) sscs startDevice 1 0"
	$ns_ at 5.1	"$node_(17) sscs startDevice 1 0"
	
	# start non-beacon enabled coordinator
	$ns_ at 5.6	"$node_(20) sscs startDevice 1 1"
	$ns_ at 5.8	"$node_(5) sscs startDevice 1 1"
	$ns_ at 6.0	"$node_(10) sscs startDevice 1 1"
	$ns_ at 6.3	"$node_(14) sscs startDevice 1 1"
	
	# start FFD not supporting association
	$ns_ at 6.8	"$node_(18) sscs startDevice 1 0"
	
	# start RFD (leaf node)
	$ns_ at 7.0	"$node_(4) sscs startDevice 0 0"
	$ns_ at 7.3	"$node_(8) sscs startDevice 0 0"
	$ns_ at 7.7	"$node_(15) sscs startDevice 0 0"
	
	# stop beacon transmission (become non-beacon enabled coordinator)
	$ns_ at $appTime1 "$node_(0) sscs stopBeacon"
	$ns_ at $appTime1 "$node_(16) sscs stopBeacon"
	
	# change beacon order and superframe order
	$ns_ at $appTime1 "$node_(2) sscs startBeacon 4 4"
	$ns_ at $appTime1 "$node_(7) sscs startBeacon 4 4"
	
	# setup cbr/poisson/mix traffic flows between nodes:
	#  -- LQD
	#  -- CCA, CSMA/CA and slotted CSMA/CA
	#  -- filtering
	if { ("$val(traffic)" == "mix") || ("$val(traffic)" == "cbr") || ("$val(traffic)" == "poisson") } {
	   if { "$val(traffic)" == "mix" } {
	   	set trafficName "cbr + poisson"
	   	set traffic1 cbr
	   	set traffic2 poisson
	   } else {
	   	set trafficName $val(traffic)
	   	set traffic1 $val(traffic)
	   	set traffic2 $val(traffic)
	   }
	   $self ${traffic1}traffic 3 18 0.2 $appTime1
	   $self ${traffic2}traffic 9 17 0.2 $appTime2
	}
	
	# setup ftp traffic flows between nodes:
	#  -- LQD
	#  -- CCA, CSMA/CA and slotted CSMA/CA
	#  -- filtering
	if { "$val(traffic)" == "ftp" } {
	   $self ftptraffic 3 18 $appTime1
	   $self ftptraffic 9 17 $appTime2
	}
	
	# failure handling
	#  -- orphaning
	#  -- coordinator realignment
	#  -- reassociation
	$ns_ at [expr $appTime1 + 2.0] "Mac/802_15_4 wpanCmd link-down 0 19"
	$ns_ at [expr $appTime2 + 3.0] "$node_(2) node-down"
	$ns_ at [expr $appTime1 + 8.0] "Mac/802_15_4 wpanCmd link-up 0 19"
	$ns_ at [expr $appTime2 + 10.0] "$node_(2) node-up"

	# begin to use indrect data transmission
	$ns_ at [expr $appTime1 + 60.0] "Mac/802_15_4 wpanCmd txOption 4"	;# 0x02=GTS; 0x04=Indirect; 0x00=Direct (only for 802.15.4-unaware upper layer app. packet)

	# tell nodes when the simulation ends
	for {set i 0} {$i < $val(nn) } {incr i} {
	    $ns_ at $stopTime "$node_($i) reset";
	}

	$ns_ at $stopTime.1 "$self finish"
}

Test/wpan instproc run {} {
	$self instvar ns_
	$ns_ run
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
default_values
runtest $argv
