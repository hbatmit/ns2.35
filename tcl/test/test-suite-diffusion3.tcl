#
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

# To run all tests: test-all-diffusion3
# to run individual test:
# ns test-suite-diffusion3.tcl simple-ping

# To view a list of available test to run with this script:
# ns test-suite-diffusion3.tcl

# This test validates a simple diffusion (ping) application

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP Diffusion ARP LL Mac 
# hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Queue/RED set bytes_ false
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set exitFastRetrans_ false
Mac/802_11 set bugFix_timer_ false;     # default changed 2006/1/30

if {![TclObject is-class Agent/DiffusionRouting]} {
	puts "Diffusion3 module is not present; validation skipped"
	exit 2
}

# ======================================================================
# Define options
# ======================================================================
global opt
set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna
set opt(ifqlen)		50		;# max packet in ifq
set opt(seed)		0.0
set opt(lm)             "off"           ;# log movement

set opt(x)		670	;# X dimension of the topography
set opt(y)		670     ;# Y dimension of the topography
set opt(stop)		300	;# simulation time
set opt(adhocRouting)   Directed_Diffusion
# ======================================================================

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

Class TestSuite

proc usage {} {
    global argv0
    puts stderr "usage: ns $argv0 <tests> "
    puts "Valid Tests: simple-ping 2pp-5n-1s-1r 2pp-10n-4s-1r 2pp-10n-1s-4r\
push-5n-1s-1r push-10n-4s-1r push-10n-1s-4r 1pp-5n-1s-1r 1pp-10n-4s-1r 1pp-10n-1s-4r gear-2pp-10n-4s-1r gear-push-10n-1s-4r"
    puts " "
    exit 1
}

TestSuite instproc init {} {
    global opt tracefd quiet
    $self instvar ns_
    set ns_ [new Simulator]
    set tracefd [open temp.rands w]
    $ns_ trace-all $tracefd
    # stealing seed from another test-suite
    ns-random 188312339
} 


Class Test/simple-ping -superclass TestSuite

Test/simple-ping instproc init {} {
    global opt
    set opt(nn) 3
    set opt(filters) GradientFilter
    $self instvar ns_ testName_
    set testName_ simple-ping
    $self next
}

Test/simple-ping instproc run {} {
    global opt
    $self instvar ns_ topo god_

    set topo	[new Topography]
    $topo load_flatgrid $opt(x) $opt(y)
    set god_ [create-god $opt(nn)]

    $ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
	         -diffusionFilter $opt(filters) \
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace OFF 
             
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
        $node_($i) color black
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
    }

# defining node positions
    $node_(0) set X_ 500.716707738489
    $node_(0) set Y_ 620.707335765875
    $node_(0) set Z_ 0.000000000000
    
    $node_(1) set X_ 320.192740186325
    $node_(1) set Y_ 450.384818286195
    $node_(1) set Z_ 0.000000000000

    #3rd node NOT in hearing range of other two
    $node_(2) set X_ 177.438972165239
    $node_(2) set Y_ 245.843469852367
    $node_(2) set Z_ 0.000000000000

    #Diffusion application - ping src
    set src_(0) [new Application/DiffApp/PingSender/TPP]
    $ns_ attach-diffapp $node_(0) $src_(0)
    $ns_ at 0.123 "$src_(0) publish"

    #Diffusion application - ping sink
    set snk_(0) [new Application/DiffApp/PingReceiver/TPP]
    $ns_ attach-diffapp $node_(2) $snk_(0)
    $ns_ at 1.456 "$snk_(0) subscribe"

    #
    # Tell nodes when the simulation ends
    #
    for {set i 0} {$i < $opt(nn) } {incr i} {
	$ns_ at $opt(stop).000000001 "$node_($i) reset";
    }
    $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
    $ns_ run
}

Class Test/2pp-5n-1s-1r -superclass TestSuite

Test/2pp-5n-1s-1r instproc init {} {
     global opt
     set opt(nn) 5
     set opt(filters) GradientFilter
     $self instvar ns_ testName_
     set testName_ 2pp-5n-1s-1r
     $self next
 }

Test/2pp-5n-1s-1r instproc run {} {
     global opt
     $self instvar ns_ topo god_

     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
	 -llType $opt(ll) \
	 -macType $opt(mac) \
	 -ifqType $opt(ifq) \
	 -ifqLen $opt(ifqlen) \
	 -antType $opt(ant) \
	 -propType $opt(prop) \
	 -phyType $opt(netif) \
	 -channelType $opt(chan) \
	 -topoInstance $topo \
	 -diffusionFilter $opt(filters) \
	 -agentTrace ON \
	 -routerTrace ON \
	 -macTrace OFF
             
     for {set i 0} {$i < $opt(nn) } {incr i} {
	 set node_($i) [$ns_ node $i]
	 $node_($i) color black
	 $node_($i) random-motion 0		;# disable random motion
	 $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 618.716707738489
     $node_(0) set Y_ 620.707335765875
     $node_(0) set Z_ 0.000000000000
    
     $node_(1) set X_ 524.192740186325
     $node_(1) set Y_ 540.384818286195
     $node_(1) set Z_ 0.000000000000
    
     $node_(2) set X_ 406.438972165239
     $node_(2) set Y_ 425.843469852367
     $node_(2) set Z_ 0.000000000000
    
     $node_(3) set X_ 320.192740186325
     $node_(3) set Y_ 350.384818286195
     $node_(3) set Z_ 0.000000000000
    
     $node_(4) set X_ 177.438972165239
     $node_(4) set Y_ 145.843469852367
     $node_(4) set Z_ 0.000000000000
    
     #Diffusion application - ping sender
     set src_(0) [new Application/DiffApp/PingSender/TPP]
     $ns_ attach-diffapp $node_(0) $src_(0)
     $ns_ at 0.123 "$src_(0) publish"

     #Diffusion application - ping receiver
     set snk_(0) [new Application/DiffApp/PingReceiver/TPP]
     $ns_ attach-diffapp $node_(2) $snk_(0)
     $ns_ at 1.456 "$snk_(0) subscribe"

#     #
#     # Tell nodes when the simulation ends
#     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
	 $ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
 }

Class Test/2pp-10n-4s-1r -superclass TestSuite

 Test/2pp-10n-4s-1r instproc init {} {
     global opt
     set opt(nn) 10
     set opt(sndr) 4
     set opt(rcvr) 1
     set opt(filters) GradientFilter
     $self instvar ns_ testName_
     set testName_ 2pp-10n-4s-1r
     $self next
 }

 Test/2pp-10n-4s-1r instproc run {} {
     global opt
     $self instvar ns_ topo god_

     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
 		 -llType $opt(ll) \
 		 -macType $opt(mac) \
 		 -ifqType $opt(ifq) \
 		 -ifqLen $opt(ifqlen) \
 		 -antType $opt(ant) \
 		 -propType $opt(prop) \
 		 -phyType $opt(netif) \
 		 -channelType $opt(chan) \
 		 -topoInstance $topo \
 	         -diffusionFilter $opt(filters) \
 		 -agentTrace ON \
                  -routerTrace OFF \
                  -macTrace OFF
             
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 18.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
    
     # 4 ping senders
     for {set i 0} {$i < $opt(sndr)} {incr i} {
	 set src_($i) [new Application/DiffApp/PingSender/TPP]
	 $ns_ attach-diffapp $node_([expr $i+2]) $src_($i)
	 $ns_ at [expr 0.123 * $i] "$src_($i) publish"
     }

#     # 1 ping receiver
     set snk_(0) [new Application/DiffApp/PingReceiver/TPP]
     $ns_ attach-diffapp $node_(9) $snk_(0)
     $ns_ at 1.456 "$snk_(0) subscribe"
     
     #     #
     #     # Tell nodes when the simulation ends
     #     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
	 $ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
 }


Class Test/2pp-10n-1s-4r -superclass TestSuite

Test/2pp-10n-1s-4r instproc init {} {
    global opt
    set opt(nn) 10
    set opt(sndr) 1
    set opt(rcvr) 4
    set opt(filters) GradientFilter
    $self instvar ns_ testName_
    set testName_ 2pp-10n-1s-4r
    $self next
}

Test/2pp-10n-1s-4r instproc run {} {
    global opt
    $self instvar ns_ topo god_
    
    set topo	[new Topography]
    $topo load_flatgrid $opt(x) $opt(y)
    set god_ [create-god $opt(nn)]
    
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
	-llType $opt(ll) \
	-macType $opt(mac) \
	-ifqType $opt(ifq) \
	-ifqLen $opt(ifqlen) \
	-antType $opt(ant) \
	-propType $opt(prop) \
	-phyType $opt(netif) \
	-channelType $opt(chan) \
	-topoInstance $topo \
	-diffusionFilter $opt(filters) \
	-agentTrace ON \
	-routerTrace OFF \
	-macTrace OFF 
    
    for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
    }

     # defining node positions
     $node_(0) set X_ 18.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
    
    # 1 ping sender
    for {set i 0} {$i < $opt(sndr)} {incr i} {
 	set src_($i) [new Application/DiffApp/PingSender/TPP]
	$ns_ attach-diffapp $node_([expr $i+2]) $src_($i)
 	$ns_ at [expr 0.123 * [expr 1+$i]] "$src_($i) publish"
    }

     # 4 ping receiver
     for {set i 0} {$i < $opt(rcvr)} {incr i} {
 	set snk_($i) [new Application/DiffApp/PingReceiver/TPP]
 	$ns_ attach-diffapp $node_([expr $opt(nn)-1 -$i]) $snk_($i)
 	$ns_ at [expr 1.1*[expr 1+$i]] "$snk_($i) subscribe"
     }
     #
     # Tell nodes when the simulation ends
     #
    for {set i 0} {$i < $opt(nn) } {incr i} {
	$ns_ at $opt(stop).000000001 "$node_($i) reset";
    }
    $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
    $ns_ run
}

Class Test/push-5n-1s-1r -superclass TestSuite

Test/push-5n-1s-1r instproc init {} {
    global opt
    set opt(nn) 5
     set opt(filters) GradientFilter
     $self instvar ns_ testName_
     set testName_ push-5n-1s-1r
    $self next
}

Test/push-5n-1s-1r instproc run {} {
     global opt
     $self instvar ns_ topo god_

     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

    $ns_ node-config -adhocRouting $opt(adhocRouting) \
	 -llType $opt(ll) \
	 -macType $opt(mac) \
	  		 -ifqType $opt(ifq) \
 		 -ifqLen $opt(ifqlen) \
 		 -antType $opt(ant) \
 		 -propType $opt(prop) \
 		 -phyType $opt(netif) \
 		 -channelType $opt(chan) \
 		 -topoInstance $topo \
 	         -diffusionFilter $opt(filters) \
 		 -agentTrace ON \
                  -routerTrace ON \
                  -macTrace OFF 
             
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
	$node_($i) color black
	$node_($i) random-motion 0		;# disable random motion
	$god_ new_node $node_($i)
    }
    
     # defining node positions
     $node_(0) set X_ 618.716707738489
     $node_(0) set Y_ 620.707335765875
     $node_(0) set Z_ 0.000000000000
    
     $node_(1) set X_ 524.192740186325
     $node_(1) set Y_ 540.384818286195
     $node_(1) set Z_ 0.000000000000
    
     $node_(2) set X_ 406.438972165239
     $node_(2) set Y_ 425.843469852367
     $node_(2) set Z_ 0.000000000000
    
     $node_(3) set X_ 320.192740186325
     $node_(3) set Y_ 350.384818286195
     $node_(3) set Z_ 0.000000000000
    
     $node_(4) set X_ 177.438972165239
     $node_(4) set Y_ 145.843469852367
     $node_(4) set Z_ 0.000000000000
    
     #Diffusion application - ping sender
     set src_(0) [new Application/DiffApp/PushSender]
     $ns_ attach-diffapp $node_(0) $src_(0)
     $ns_ at 0.123 "$src_(0) publish"

     #Diffusion application - ping receiver
     set snk_(0) [new Application/DiffApp/PushReceiver]
     $ns_ attach-diffapp $node_(2) $snk_(0)
     $ns_ at 1.456 "$snk_(0) subscribe"

     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
}

Class Test/push-10n-1s-4r -superclass TestSuite

 Test/push-10n-1s-4r instproc init {} {
     global opt
     set opt(nn) 10
     set opt(sndr) 1
     set opt(rcvr) 4
     set opt(filters) GradientFilter
     $self instvar ns_ testName_
     set testName_ push-10n-1s-4r
     $self next
 }

Test/push-10n-1s-4r instproc run {} {
     global opt
     $self instvar ns_ topo god_

     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
	 -llType $opt(ll) \
	 -macType $opt(mac) \
	 -ifqType $opt(ifq) \
	 -ifqLen $opt(ifqlen) \
	 -antType $opt(ant) \
	 -propType $opt(prop) \
	 -phyType $opt(netif) \
	 -channelType $opt(chan) \
	 -topoInstance $topo \
	 -diffusionFilter $opt(filters) \
	 -agentTrace ON \
	 -routerTrace OFF \
	 -macTrace OFF 
             
     for {set i 0} {$i < $opt(nn) } {incr i} {
	 set node_($i) [$ns_ node $i]
         $node_($i) color black
	 $node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 18.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
    
     # 1 ping sender
     for {set i 0} {$i < $opt(sndr)} {incr i} {
 	set src_($i) [new Application/DiffApp/PushSender]
 	$ns_ attach-diffapp $node_([expr $i+2]) $src_($i)
 	$ns_ at [expr 0.123 * [expr 1+$i]] "$src_($i) publish"
     }

     # 4 ping receiver
     for {set i 0} {$i < $opt(rcvr)} {incr i} {
 	set snk_($i) [new Application/DiffApp/PushReceiver]
 	$ns_ attach-diffapp $node_([expr $opt(nn)-1 -$i]) $snk_($i)
 	$ns_ at [expr 1.1*[expr 1+$i]] "$snk_($i) subscribe"
     }
     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
 }

Class Test/push-10n-4s-1r -superclass TestSuite

Test/push-10n-4s-1r instproc init {} {
    global opt
    set opt(nn) 10
    set opt(sndr) 4
    set opt(rcvr) 1
    set opt(filters) GradientFilter
    $self instvar ns_ testName_
    set testName_ push-10n-4s-1r
    $self next
}

Test/push-10n-4s-1r instproc run {} {
     global opt
     $self instvar ns_ topo god_

     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
	 -llType $opt(ll) \
	 -macType $opt(mac) \
	 -ifqType $opt(ifq) \
	 -ifqLen $opt(ifqlen) \
	 -antType $opt(ant) \
	 -propType $opt(prop) \
	 -phyType $opt(netif) \
	 -channelType $opt(chan) \
	 -topoInstance $topo \
	 -diffusionFilter $opt(filters) \
	 -agentTrace ON \
	 -routerTrace OFF \
	 -macTrace OFF 
             
    for {set i 0} {$i < $opt(nn) } {incr i} {
	 set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
    }

     # defining node positions
     $node_(0) set X_ 18.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
   
     # 4 ping sender
     for {set i 0} {$i < $opt(sndr)} {incr i} {
 	set src_($i) [new Application/DiffApp/PushSender]
 	$ns_ attach-diffapp $node_([expr $i+2]) $src_($i)
 	$ns_ at [expr 0.123 * [expr 1+$i]] "$src_($i) publish"
     }

     # 1 ping receiver
     for {set i 0} {$i < $opt(rcvr)} {incr i} {
 	set snk_($i) [new Application/DiffApp/PushReceiver]
 	$ns_ attach-diffapp $node_([expr $opt(nn)-1 -$i]) $snk_($i)
 	$ns_ at [expr 1.1*[expr 1+$i]] "$snk_($i) subscribe"
     }
     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
}

Class Test/1pp-5n-1s-1r -superclass TestSuite

Test/1pp-5n-1s-1r instproc init {} {
     global opt
     set opt(nn) 5
     set opt(sndr) 1
     set opt(rcvr) 1
     set opt(filters) OnePhasePullFilter
     $self instvar ns_ testName_
     set testName_ 1pp-5n-1s-1r
     $self next
}

Test/1pp-5n-1s-1r instproc run {} {
    global opt
    $self instvar ns_ topo god_

    set topo	[new Topography]
    $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
 		 -llType $opt(ll) \
 		 -macType $opt(mac) \
 		 -ifqType $opt(ifq) \
 		 -ifqLen $opt(ifqlen) \
 		 -antType $opt(ant) \
 		 -propType $opt(prop) \
 		 -phyType $opt(netif) \
 		 -channelType $opt(chan) \
 		 -topoInstance $topo \
 	         -diffusionFilter $opt(filters) \
 		 -agentTrace ON \
                  -routerTrace ON \
                  -macTrace OFF 
             
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 18.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
    
     # 1 ping sender
     for {set i 0} {$i < $opt(sndr)} {incr i} {
 	set src_($i) [new Application/DiffApp/PingSender/OPP]
 	$ns_ attach-diffapp $node_([expr $i+1]) $src_($i)
 	$ns_ at [expr 0.123 * [expr 1+$i]] "$src_($i) publish"
     }

    # 1 ping receiver
     for {set i 0} {$i < $opt(rcvr)} {incr i} {
 	set snk_($i) [new Application/DiffApp/PingReceiver/OPP]
 	$ns_ attach-diffapp $node_([expr $opt(nn)-1 -$i]) $snk_($i)
 	$ns_ at [expr 1.1*[expr 1+$i]] "$snk_($i) subscribe"
     }
     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
}

Class Test/1pp-10n-4s-1r -superclass TestSuite

Test/1pp-10n-4s-1r instproc init {} {
     global opt
     set opt(nn) 10
     set opt(sndr) 4
     set opt(rcvr) 1
     set opt(filters) OnePhasePullFilter
     $self instvar ns_ testName_
     set testName_ 1pp-10n-4s-1r
     $self next
}

Test/1pp-10n-4s-1r instproc run {} {
     global opt
     $self instvar ns_ topo god_

     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
 		 -llType $opt(ll) \
 		 -macType $opt(mac) \
 		 -ifqType $opt(ifq) \
 		 -ifqLen $opt(ifqlen) \
 		 -antType $opt(ant) \
 		 -propType $opt(prop) \
 		 -phyType $opt(netif) \
 		 -channelType $opt(chan) \
 		 -topoInstance $topo \
 	         -diffusionFilter $opt(filters) \
 		 -agentTrace ON \
                  -routerTrace OFF \
                  -macTrace OFF 
             
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 18.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
    
     # 4 ping senders
     for {set i 0} {$i < $opt(sndr)} {incr i} {
 	set src_($i) [new Application/DiffApp/PingSender/OPP]
 	$ns_ attach-diffapp $node_([expr $i+2]) $src_($i)
 	$ns_ at [expr 0.123 * $i] "$src_($i) publish"
     }

     # 1 ping receiver
     set snk_(0) [new Application/DiffApp/PingReceiver/OPP]
     $ns_ attach-diffapp $node_(9) $snk_(0)
     $ns_ at 1.456 "$snk_(0) subscribe"

     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
}


Class Test/1pp-10n-1s-4r -superclass TestSuite

Test/1pp-10n-1s-4r instproc init {} {
     global opt
     set opt(nn) 10
     set opt(sndr) 1
     set opt(rcvr) 4
     set opt(filters) OnePhasePullFilter
     $self instvar ns_ testName_
     set testName_ 1pp-10n-1s-4r
     $self next
}

Test/1pp-10n-1s-4r instproc run {} {
     global opt
     $self instvar ns_ topo god_
    
    set topo	[new Topography]
    $topo load_flatgrid $opt(x) $opt(y)
    set god_ [create-god $opt(nn)]
    
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
	 -llType $opt(ll) \
	 -macType $opt(mac) \
	 -ifqType $opt(ifq) \
	 -ifqLen $opt(ifqlen) \
	 -antType $opt(ant) \
	 -propType $opt(prop) \
	 -phyType $opt(netif) \
	 -channelType $opt(chan) \
	 -topoInstance $topo \
	 -diffusionFilter $opt(filters) \
	 -agentTrace ON \
	 -routerTrace OFF \
	 -macTrace OFF 
             
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 18.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
    
     # 1 ping sender
     for {set i 0} {$i < $opt(sndr)} {incr i} {
 	set src_($i) [new Application/DiffApp/PingSender/OPP]
 	$ns_ attach-diffapp $node_([expr $i+2]) $src_($i)
 	$ns_ at [expr 0.123 * [expr 1+$i]] "$src_($i) publish"
     }

     # 4 ping receiver
     for {set i 0} {$i < $opt(rcvr)} {incr i} {
 	set snk_($i) [new Application/DiffApp/PingReceiver/OPP]
 	$ns_ attach-diffapp $node_([expr $opt(nn)-1 -$i]) $snk_($i)
 	$ns_ at [expr 1.1*[expr 1+$i]] "$snk_($i) subscribe"
     }
     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
}

Class Test/gear-2pp-5n-1s-1r -superclass TestSuite

Test/gear-2pp-5n-1s-1r instproc init {} {
     global opt
     set opt(nn) 5
     set opt(filters) GradientFilter/GeoRoutingFilter
     $self instvar ns_ testName_
     set testName_ gear-2pp-5n-1s-1r
     $self next
}

Test/gear-2pp-5n-1s-1r instproc run {} {
     global opt
     $self instvar ns_ topo god_
     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
 		 -llType $opt(ll) \
 		 -macType $opt(mac) \
 		 -ifqType $opt(ifq) \
 		 -ifqLen $opt(ifqlen) \
 		 -antType $opt(ant) \
 		 -propType $opt(prop) \
 		 -phyType $opt(netif) \
 		 -channelType $opt(chan) \
 		 -topoInstance $topo \
 	         -diffusionFilter $opt(filters) \
 		 -agentTrace ON \
                  -routerTrace OFF \
                  -macTrace OFF 
             
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 618.716707738489
     $node_(0) set Y_ 620.707335765875
     $node_(0) set Z_ 0.000000000000
    
     $node_(1) set X_ 524.192740186325
     $node_(1) set Y_ 540.384818286195
     $node_(1) set Z_ 0.000000000000
   
     $node_(2) set X_ 406.438972165239
     $node_(2) set Y_ 425.843469852367
     $node_(2) set Z_ 0.000000000000
    
     $node_(3) set X_ 320.192740186325
     $node_(3) set Y_ 350.384818286195
     $node_(3) set Z_ 0.000000000000
    
     $node_(4) set X_ 177.438972165239
     $node_(4) set Y_ 145.843469852367
     $node_(4) set Z_ 0.000000000000
    
    for {set i 0} {$i < $opt(nn)} {incr i} {
	 set gearf($i) [new Application/DiffApp/GeoRoutingFilter \
 			   [$node_($i) get-dr]]
	 $gearf($i) start
     }

     # 1 gear sender
     set src_(0) [new Application/DiffApp/GearSenderApp]
     $ns_ attach-diffapp $node_(0) $src_(0)
     $src_(0) push-pull-options pull point 618.716 620.707
     $ns_ at 0.123 "$src_(0) publish"

     #Diffusion application - ping receiver
     set snk_(0) [new Application/DiffApp/GearReceiverApp]
     $ns_ attach-diffapp $node_(4) $snk_(0)
     $snk_(0) push-pull-options pull region 600 650 600 650
     $ns_ at 1.456 "$snk_(0) subscribe"

     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
}
 
Class Test/gear-2pp-10n-4s-1r -superclass TestSuite

Test/gear-2pp-10n-4s-1r instproc init {} {
     global opt
     set opt(nn) 10
     set opt(filters) GradientFilter/GeoRoutingFilter
     $self instvar ns_ testName_
     set testName_ gear-2pp-10n-4s-1r
     $self next
 }

Test/gear-2pp-10n-4s-1r instproc run {} {
     global opt
     $self instvar ns_ topo god_

     set topo	[new Topography]
     $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
 	-llType $opt(ll) \
 	-macType $opt(mac) \
 	-ifqType $opt(ifq) \
 	-ifqLen $opt(ifqlen) \
 	-antType $opt(ant) \
 	-propType $opt(prop) \
 	-phyType $opt(netif) \
 	-channelType $opt(chan) \
 	-topoInstance $topo \
 	-diffusionFilter $opt(filters) \
 	-agentTrace ON \
 	-routerTrace OFF \
 	-macTrace OFF 
    
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 180.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
    
     # starting gear filter separately- this will go away once gear has 
     # callbacks for updating node positions
     for {set i 0} {$i < $opt(nn)} {incr i} {
 	set gearf($i) [new Application/DiffApp/GeoRoutingFilter \
 			   [$node_($i) get-dr]]
 	$gearf($i) start
     }

     # 4 gear sender
     set src_(0) [new Application/DiffApp/GearSenderApp]
     $ns_ attach-diffapp $node_(0) $src_(0)
     $src_(0) push-pull-options pull point 180.587 401.886
     $ns_ at 0.123 "$src_(0) publish"
    
     set src_(1) [new Application/DiffApp/GearSenderApp]
     $ns_ attach-diffapp $node_(1) $src_(1)
     $src_(1) push-pull-options pull point 11.901 36.218
     $ns_ at 0.23 "$src_(1) publish"
    
     set src_(2) [new Application/DiffApp/GearSenderApp]
     $ns_ attach-diffapp $node_(2) $src_(2)
     $src_(2) push-pull-options pull point 224.282 20.774
     $ns_ at 0.3 "$src_(2) publish"
    
     set src_(3) [new Application/DiffApp/GearSenderApp]
     $ns_ attach-diffapp $node_(3) $src_(3)
     $src_(3) push-pull-options pull point 158.840 139.650
     $ns_ at 0.4 "$src_(3) publish"
    
     # 1 ping receiver
     set snk_(0) [new Application/DiffApp/GearReceiverApp]
     $ns_ attach-diffapp $node_(7) $snk_(0)
     $snk_(0) push-pull-options pull region 10 300 10 450
     $ns_ at 1.456 "$snk_(0) subscribe"
    
     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
     $ns_ run
}

Class Test/gear-push-10n-1s-4r -superclass TestSuite

Test/gear-push-10n-1s-4r instproc init {} {
     global opt
     set opt(nn) 10
     set opt(filters) GradientFilter/GeoRoutingFilter
     $self instvar ns_ testName_
     set testName_ gear-push-10n-1s-4r
     $self next
}

Test/gear-push-10n-1s-4r instproc run {} {
     global opt
    $self instvar ns_ topo god_

     set topo	[new Topography]
    $topo load_flatgrid $opt(x) $opt(y)
     set god_ [create-god $opt(nn)]

     $ns_ node-config -adhocRouting $opt(adhocRouting) \
 	-llType $opt(ll) \
 	-macType $opt(mac) \
 	-ifqType $opt(ifq) \
 	-ifqLen $opt(ifqlen) \
 	-antType $opt(ant) \
 	-propType $opt(prop) \
 	-phyType $opt(netif) \
 	-channelType $opt(chan) \
 	-topoInstance $topo \
 	-diffusionFilter $opt(filters) \
 	-agentTrace ON \
 	-routerTrace OFF \
 	-macTrace OFF 
    
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	set node_($i) [$ns_ node $i]
         $node_($i) color black
 	$node_($i) random-motion 0		;# disable random motion
         $god_ new_node $node_($i)
     }

     # defining node positions
     $node_(0) set X_ 180.587605569735
     $node_(0) set Y_ 401.886827012678
     $node_(0) set Z_ 0.000000000000
     $node_(1) set X_ 11.901958617639
     $node_(1) set Y_ 36.218497218617
     $node_(1) set Z_ 0.000000000000
     $node_(2) set X_ 224.282785411393
     $node_(2) set Y_ 20.774608253697
     $node_(2) set Z_ 0.000000000000
     $node_(3) set X_ 158.840938304009
     $node_(3) set Y_ 139.650216383776
     $node_(3) set Z_ 0.000000000000
     $node_(4) set X_ 101.186886005903
     $node_(4) set Y_ 147.993190973633
     $node_(4) set Z_ 0.000000000000
     $node_(5) set X_ 321.560825121175
     $node_(5) set Y_ 472.788096833600
     $node_(5) set Z_ 0.000000000000
     $node_(6) set X_ 149.543901734330
     $node_(6) set Y_ 384.356581531832
     $node_(6) set Z_ 0.000000000000
     $node_(7) set X_ 381.066146464027
     $node_(7) set Y_ 78.723958958779
     $node_(7) set Z_ 0.000000000000
     $node_(8) set X_ 113.578290026963
     $node_(8) set Y_ 410.320583900348
     $node_(8) set Z_ 0.000000000000
     $node_(9) set X_ 258.053977148981
     $node_(9) set Y_ 113.194171851670
     $node_(9) set Z_ 0.000000000000
    
     # starting gear filter separately- this will go away once gear has 
     # callbacks for updating node positions
     for {set i 0} {$i < $opt(nn)} {incr i} {
 	set gearf($i) [new Application/DiffApp/GeoRoutingFilter \
 			   [$node_($i) get-dr]]
	 $gearf($i) start
     }

    # 1 push sender
    set src_(0) [new Application/DiffApp/GearSenderApp]
    $ns_ attach-diffapp $node_(6) $src_(0)
    $src_(0) push-pull-options push region 100 650 100 650
    $ns_ at 0.123 "$src_(0) publish"
    
    # 4 push receiver
    set snk_(0) [new Application/DiffApp/GearReceiverApp]
    $ns_ attach-diffapp $node_(8) $snk_(0)
    $snk_(0) push-pull-options push point 101.186 147.993
    $ns_ at 1.256 "$snk_(0) subscribe"
    
    set snk_(1) [new Application/DiffApp/GearReceiverApp]
    $ns_ attach-diffapp $node_(9) $snk_(1)
     $snk_(1) push-pull-options push point 258.053 113.194
     $ns_ at 1.456 "$snk_(1) subscribe"
    
     set snk_(2) [new Application/DiffApp/GearReceiverApp]
     $ns_ attach-diffapp $node_(7) $snk_(2)
     $snk_(2) push-pull-options push point 321.560 472.788
     $ns_ at 1.56 "$snk_(2) subscribe"
    
     set snk_(3) [new Application/DiffApp/GearReceiverApp]
     $ns_ attach-diffapp $node_(1) $snk_(3)
     $snk_(3) push-pull-options push point 158.840 139.650
     $ns_ at 1.73 "$snk_(3) subscribe"
    
     #
     # Tell nodes when the simulation ends
     #
     for {set i 0} {$i < $opt(nn) } {incr i} {
 	$ns_ at $opt(stop).000000001 "$node_($i) reset";
     }
     $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
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
 runtest $argv
