# Copyright (c) 2000 by the University of Southern California
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

# To view a list of available tests to run with this script:
# ns test-suite-smac.tcl

#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP TCP Diffusion ARP LL Mac  
# hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.

Class TestSuite

Class Test/brdcast1 -superclass TestSuite
# 2 nodes brdcast ping req/replies to one another

Class Test/brdcast1_sync -superclass TestSuite

Class Test/brdcast2 -superclass TestSuite
# 3 node topology brdcasting ping req/rep that result in collision

Class Test/brdcast2_sync -superclass TestSuite

Class Test/unicast1 -superclass TestSuite
# 2 node topology unicasting with RTS/CTS/DATA/ACK exchange

Class Test/unicast1_sync -superclass TestSuite

Class Test/unicast2 -superclass TestSuite
# 3 node topology where node 0 & 1 unicast and node 2 brdcast.

Class Test/unicast2_sync -superclass TestSuite

Class Test/unicast3 -superclass TestSuite
#3 node topolofy where node 2 is neighbor to the sender node. can hear RTS/DATA, not CTS

Class Test/unicast3_sync -superclass TestSuite

Class Test/unicast4 -superclass TestSuite
#3 node topology where node 2 is neighbor to the recvr node. can hear CTS only.

Class Test/unicast4_sync -superclass TestSuite

Class Test/unicast5 -superclass TestSuite
# 4 node topology. 2 sender-recvr pairs (0->1 & 3->2)
#3 cannot hear 0 & 1. 2 can hear only sender 0.
 
Class Test/unicast5_sync -superclass TestSuite

Class Test/unicast6 -superclass TestSuite
# 4 node topology. 2 sender-recvr pairs (0->1 & 3->2)
#3 cannot hear 0 & 1. 2 can hear only recvr 1.

Class Test/unicast6_sync -superclass TestSuite

Class Test/unicast7 -superclass TestSuite
# 4 node topology. 2 sender-recvr pairs (0->1 & 2->3)
#3 cannot hear 0 & 1. 2 can hear only sender 0.
 
Class Test/unicast7_sync -superclass TestSuite

Class Test/unicast8 -superclass TestSuite
# 4 node topology. 2 sender-recvr pairs (0->1 & 2->3)
#3 cannot hear 0. 2 can hear only recvr 1.

Class Test/unicast8_sync -superclass TestSuite

Class Test/unicast9 -superclass TestSuite
# 4 node topology. 2 sender-recvr pairs (0->1 & 2->3)
#2 & 3 can hear 0 & 1 but cannot recv pkts correctly. a lot of delay

Class Test/unicast9_sync -superclass TestSuite

Class Test/unicast10 -superclass TestSuite
# 4 node topology. 2 sender-recvr pairs (0->1 & 2->3)
#2 & 3 cannot hear and recv from 0 & 1. can send simultaneously.

Class Test/unicast10_sync -superclass TestSuite

Class Test/unicast11 -superclass TestSuite
# 3 node triangle topology.1->2,2->3,3->1.

Class Test/unicast11_sync -superclass TestSuite

Class Test/unicast12 -superclass TestSuite
# 3 node triangle topology.1->2,2->3,3->1.with error model on every incoming interface
# that randomly drops pkts

Class Test/unicast12_sync -superclass TestSuite

proc usage {} {
    global argv0
    puts stderr "usage: ns $argv0 <tests>"
    puts "Valid tests: brdcast1"
}

proc default_options {} {
    global opt
    set opt(chan)           Channel/WirelessChannel    ;# channel type
    set opt(prop)           Propagation/TwoRayGround   ;# radio-propagation model
    set opt(netif)          Phy/WirelessPhy            ;# network interface type
    set opt(mac)            Mac/SMAC                   ;# MAC type
    #set opt(mac)            Mac/802_11                 ;# MAC type
    set opt(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
    set opt(ll)             LL                         ;# link layer type
    set opt(ant)            Antenna/OmniAntenna        ;# antenna model
    set opt(ifqlen)         50                         ;# max packet in ifq
    set opt(x)              800
    set opt(y)              800
    set opt(rp)             DumbAgent               ;# routing protocol
    set opt(tr)             temp.rands
    set opt(stop)           5.0
    set opt(stop-sync)      100.0        ;# extended run time for sync simulations
    set opt(seed)           1
}

TestSuite instproc init {} {
    global opt node_
    $self instvar ns_ topo_ tracefd_ testname_

    set name [split $testname_ _]
    puts "$name"
    if { [lindex $name 1] == "sync" } {
	Mac/SMAC set syncFlag_ 1
    }

    set ns_         [new Simulator]
    
    puts "Seeding Random number generator with $opt(seed)\n"
    ns-random $opt(seed)
    
    set tracefd_	[open $opt(tr) w]
    $ns_ trace-all $tracefd_
    
    set topo_	    [new Topography]
    $topo_ load_flatgrid $opt(x) $opt(y)
    
    create-god $opt(nn)

    $ns_ node-config -adhocRouting $opt(rp) \
                         -macType $opt(mac) \
                         -llType $opt(ll) \
			 -ifqType $opt(ifq) \
			 -ifqLen $opt(ifqlen) \
			 -antType $opt(ant) \
			 -propType $opt(prop) \
			 -phyType $opt(netif) \
			 -channelType $opt(chan) \
			 -topoInstance $topo_ \
			 -agentTrace ON \
			 -routerTrace ON \
			 -macTrace ON

    if {$testname_ == "unicast12" } {
	$ns_ node-config --IncomingErrProc $opt(err)
    }
    
#    if { [lindex $name 1] == "sync" } {
	#$ns_ node-config -macTrace OFF
#    }
    			 
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node]	
	$node_($i) random-motion 0		;# disable random motion
    }
}


TestSuite instproc run {} {
    global opt node_
    $self instvar ns_ testname_

    set n [split $testname_ _]
    if { [lindex $n 1] == "sync" } {
	set opt(stop) $opt(stop-sync)
    }
    puts "$opt(stop)"  
    #
    # Tell nodes when the simulation ends
    #
    for {set i 0} {$i < $opt(nn) } {incr i} {
	$ns_ at $opt(stop) "$node_($i) reset";
    }
    
    $ns_ at $opt(stop) "$self finish"
    $ns_ at $opt(stop) "puts \"NS EXITING...\" ; $ns_ halt"

    puts "Starting Simulation..."
    $ns_  run
}

Test/brdcast1 instproc init {} {
    global opt node_
    $self instvar ns_ testname_
    
    set testname_ brdcast1
    set opt(nn) 2
    $self next
    
    
    $node_(0) set X_ 5.0
    $node_(0) set Y_ 2.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 45.0
    $node_(1) set Z_ 0.0
    
    # all scheduled to brdcast at same time
    
    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    $ns_ at 1.0 "$ping0 start-WL-brdcast"
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    $ns_ at 1.0 "$ping1 start-WL-brdcast"
    
}

Test/brdcast1 instproc run {} {
    $self instvar ns_
    $self next
}

    

Test/brdcast1_sync instproc init {} {
    global opt node_
    $self instvar testname_ ns_
    
    set testname_ brdcast1_sync
    set opt(nn) 2
    $self next
    
    $node_(0) set X_ 5.0
    $node_(0) set Y_ 2.0
    $node_(0) set Z_ 0.0
    
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 45.0
    $node_(1) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    $ns_ at 40.0 "$ping0 start-WL-brdcast"
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    $ns_ at 40.0 "$ping1 start-WL-brdcast"

}

Test/brdcast1_sync instproc run {} {
    $self instvar ns_
    $self next
}

Test/brdcast2 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ brdcast2
    set opt(nn)  3
    $self next
    
    $node_(0) set X_ 5.0
    $node_(0) set Y_ 2.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 45.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 150.0
    $node_(2) set Y_ 150.0
    $node_(2) set Z_ 0.0
    
    # all scheduled to brdcast at same time
    
    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    $ns_ at 1.0 "$ping0 start-WL-brdcast"
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    $ns_ at 1.0 "$ping1 start-WL-brdcast"

    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    $ns_ at 1.0 "$ping2 start-WL-brdcast"
}

Test/brdcast2 instproc run {} {
    $self instvar ns_
    $self next
}

Test/brdcast2_sync instproc init {} {
    global opt node_
    $self instvar ns_ testname_
    
    set testname_ brdcast2_sync
    set opt(nn)  3
    $self next
    
    $node_(0) set X_ 5.0
    $node_(0) set Y_ 2.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 45.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 150.0
    $node_(2) set Y_ 150.0
    $node_(2) set Z_ 0.0
    
    # all scheduled to brdcast at same time
    
    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    $ns_ at 40.0 "$ping0 start-WL-brdcast"
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    $ns_ at 41.0 "$ping1 start-WL-brdcast"

    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    $ns_ at 42.0 "$ping2 start-WL-brdcast"
}

Test/brdcast2_sync instproc run {} {
    $self instvar ns_
    $self next
}


Test/unicast1 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast1
    set opt(nn) 2
    $self next

    $node_(0) set X_ 5.0
    $node_(0) set Y_ 2.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 45.0
    $node_(1) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    
    # connect two agents
    $ns_ connect $ping0 $ping1

    $ns_ at 1.0 "$ping0 send"

}


Test/unicast1 instproc run {} {
    $self instvar ns_
    $self next
}

Test/unicast1_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast1_sync
    set opt(nn) 2
    $self next

    $node_(0) set X_ 5.0
    $node_(0) set Y_ 2.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 45.0
    $node_(1) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    
    # connect two agents
    $ns_ connect $ping0 $ping1

    $ns_ at 40.0 "$ping0 send"

}


Test/unicast1_sync instproc run {} {
    $self instvar ns_
    $self next
}



Test/unicast2 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast2
    set opt(nn) 3
    $self next

    $node_(0) set X_ 300.0
    $node_(0) set Y_ 350.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 450.0
    $node_(1) set Y_ 350.0
    $node_(1) set Z_ 0.0
    
    # 2 can hear and recv from nodes 0 & 1 (sender and recvr)
    $node_(2) set X_ 400.0
    $node_(2) set Y_ 125.0
    $node_(2) set Z_ 0.0
    
    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1

    # connect two agents
    $ns_ connect $ping0 $ping1
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    
    # start times for connections
    $ns_ at 0.1 "$ping0 send"

    # ping2 sends brdcast overlapping with ping0
    $ns_ at 0.2 "$ping2 start-WL-brdcast"

}

Test/unicast2 instproc run {} {
    $self instvar ns_
    $self next
}

Test/unicast2_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast2_sync
    set opt(nn) 3
    $self next

    $node_(0) set X_ 300.0
    $node_(0) set Y_ 350.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 450.0
    $node_(1) set Y_ 350.0
    $node_(1) set Z_ 0.0
    
    # 2 can hear and recv from nodes 0 & 1 (sender and recvr)
    $node_(2) set X_ 400.0
    $node_(2) set Y_ 125.0
    $node_(2) set Z_ 0.0
    
    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1

    # connect two agents
    $ns_ connect $ping0 $ping1
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    
    # start times for connections
    $ns_ at 40.1 "$ping0 send"

    # ping2 sends brdcast overlapping with ping0
    $ns_ at 40.2 "$ping2 start-WL-brdcast"

}

Test/unicast2_sync instproc run {} {
    $self instvar ns_
    $self next
}


Test/unicast3 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast3
    set opt(nn) 3
    $self next
    
    $node_(0) set X_ 300.0
    $node_(0) set Y_ 300.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 150.0
    $node_(1) set Y_ 150.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 470.0
    $node_(2) set Y_ 470.0
    $node_(2) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0

    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1

    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2

    # connect two agents
    $ns_ connect $ping0 $ping1

    # start times for connections
    $ns_ at 0.1 "$ping0 send"
    $ns_ at 0.2 "$ping2 start-WL-brdcast"
    
}

Test/unicast3 instproc run {} {
    $self instvar ns_
    $self next
}


Test/unicast3_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast3_sync
    set opt(nn) 3
    $self next
    
    $node_(0) set X_ 300.0
    $node_(0) set Y_ 300.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 150.0
    $node_(1) set Y_ 150.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 470.0
    $node_(2) set Y_ 470.0
    $node_(2) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0

    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1

    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2

    # connect two agents
    $ns_ connect $ping0 $ping1

    # start times for connections
    $ns_ at 40.1 "$ping0 send"
    $ns_ at 45.2 "$ping2 start-WL-brdcast"
    
}

Test/unicast3_sync instproc run {} {
    $self instvar ns_
    $self next
}


Test/unicast4 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast4
    set opt(nn) 3
    $self next
    
    # can hear recvr, CTS only.
    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    $node_(2) set X_ 470.0
    $node_(2) set Y_ 470.0
    $node_(2) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0

    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1

    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2

    # connect two agents
    $ns_ connect $ping0 $ping1

    # start times for connections
    $ns_ at 0.1 "$ping0 send"
    $ns_ at 0.2 "$ping2 start-WL-brdcast"
    
}

Test/unicast4 instproc run {} {
    $self instvar ns_
    $self next
}

Test/unicast4_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast4_sync
    set opt(nn) 3
    $self next
    
    # can hear recvr, CTS only.
    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    $node_(2) set X_ 470.0
    $node_(2) set Y_ 470.0
    $node_(2) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0

    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1

    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2

    # connect two agents
    $ns_ connect $ping0 $ping1

    # start times for connections
    $ns_ at 40.1 "$ping0 send"
    $ns_ at 40.2 "$ping2 start-WL-brdcast"
    
}

Test/unicast4_sync instproc run {} {
    $self instvar ns_
    $self next
}


Test/unicast5 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast5
    set opt(nn) 4
    $self next

    $node_(0) set X_ 300.0
    $node_(0) set Y_ 300.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 150.0
    $node_(1) set Y_ 150.0
    $node_(1) set Z_ 0.0
    
    # node 2 can hear 0 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 0.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 0.1 "$ping3 send"
    
}

Test/unicast5 instproc run {} {
    $self instvar ns_
    $self next
}

Test/unicast5_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast5_sync
    set opt(nn) 4
    $self next

    $node_(0) set X_ 300.0
    $node_(0) set Y_ 300.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 150.0
    $node_(1) set Y_ 150.0
    $node_(1) set Z_ 0.0
    
    # node 2 can hear 0 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 40.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 40.1 "$ping3 send"
    
}

Test/unicast5_sync instproc run {} {
    $self instvar ns_
    $self next
}


Test/unicast6 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast6
    set opt(nn) 4
    $self next

    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    # node 2 can hear 1 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 0.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 0.1 "$ping3 send"
    
}

Test/unicast6 instproc run {} {
    $self instvar ns_
    $self next
}

Test/unicast6_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast6_sync
    set opt(nn) 4
    $self next

    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    # node 2 can hear 1 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 40.1 "$ping0 send"

    #sender cannot hear other tx
    # node 2 and 3 have perfectly sync'ed cycles
    # due to collision discovers each other after about 60s.
    $ns_ at 60.1 "$ping3 send"
    
}

Test/unicast6_sync instproc run {} {
    $self next
}


Test/unicast7 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast7
    set opt(nn) 4
    $self next

    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    # node 2 can hear 1 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 0.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 0.1 "$ping2 send"
    
}

Test/unicast7 instproc run {} {
    $self next
}

Test/unicast7_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast7_sync
    set opt(nn) 4
    $self next

    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    # node 2 can hear 1 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 40.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 60.1 "$ping2 send"
    
}

Test/unicast7_sync instproc run {} {
    $self next
}


Test/unicast8 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast8
    set opt(nn) 4
    $self next

    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    # node 2 can hear 1 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 0.1 "$ping1 send"

    #sender cannot hear other tx
    $ns_ at 0.1 "$ping2 send"
    
}

Test/unicast8 instproc run {} {
    $self next
}

Test/unicast8_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast8_sync
    set opt(nn) 4
    $self next

    $node_(1) set X_ 300.0
    $node_(1) set Y_ 300.0
    $node_(1) set Z_ 0.0
    
    $node_(0) set X_ 150.0
    $node_(0) set Y_ 150.0
    $node_(0) set Z_ 0.0
    
    # node 2 can hear 1 & 3
    $node_(2) set X_ 450.0
    $node_(2) set Y_ 450.0
    $node_(2) set Z_ 0.0

    # node 3 hears only 2
    $node_(3) set X_ 600.0
    $node_(3) set Y_ 600.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 40.1 "$ping1 send"

    #sender cannot hear other tx
    $ns_ at 60.1 "$ping2 send"
    
}

Test/unicast8_sync instproc run {} {
    $self next
}

Test/unicast9 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast9
    set opt(nn) 4
    $self next

    $node_(0) set X_ 300.0
    $node_(0) set Y_ 300.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 150.0
    $node_(1) set Y_ 150.0
    $node_(1) set Z_ 0.0
    
    # node 2 & 3 cannot  hear 0 & 1
    $node_(2) set X_ 500.0
    $node_(2) set Y_ 550.0
    $node_(2) set Z_ 0.0

    $node_(3) set X_ 650.0
    $node_(3) set Y_ 680.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 0.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 0.1 "$ping3 send"
    
}

Test/unicast9 instproc run {} {
    $self next
}


Test/unicast9_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast9_sync
    set opt(nn) 4
    $self next

    $node_(0) set X_ 300.0
    $node_(0) set Y_ 300.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 150.0
    $node_(1) set Y_ 150.0
    $node_(1) set Z_ 0.0
    
    # node 2 & 3 cannot  hear 0 & 1
    $node_(2) set X_ 500.0
    $node_(2) set Y_ 550.0
    $node_(2) set Z_ 0.0

    $node_(3) set X_ 650.0
    $node_(3) set Y_ 680.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 40.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 60.1 "$ping3 send"
    
}

Test/unicast9_sync instproc run {} {
    $self next
}

Test/unicast10 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast10
    set opt(nn) 4
    $self next

    $node_(0) set X_ 100.0
    $node_(0) set Y_ 100.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 50.0
    $node_(1) set Z_ 0.0
    
    # node 2 & 3 cannot  hear 0 & 1
    $node_(2) set X_ 500.0
    $node_(2) set Y_ 550.0
    $node_(2) set Z_ 0.0

    $node_(3) set X_ 650.0
    $node_(3) set Y_ 680.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 0.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 0.1 "$ping3 send"
    
}

Test/unicast10 instproc run {} {
    $self next
}

Test/unicast10_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast10_sync
    set opt(nn) 4
    $self next

    $node_(0) set X_ 100.0
    $node_(0) set Y_ 100.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 50.0
    $node_(1) set Y_ 50.0
    $node_(1) set Z_ 0.0
    
    # node 2 & 3 cannot  hear 0 & 1
    $node_(2) set X_ 500.0
    $node_(2) set Y_ 550.0
    $node_(2) set Z_ 0.0

    $node_(3) set X_ 650.0
    $node_(3) set Y_ 680.0
    $node_(3) set Z_ 0.0

    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(3) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3
    
    # start times for both connections
    $ns_ at 40.1 "$ping0 send"

    #sender cannot hear other tx
    $ns_ at 40.1 "$ping3 send"
    
}

Test/unicast10_sync instproc run {} {
    $self next
}


Test/unicast11 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast11
    set opt(nn) 3
    $self next

    $node_(0) set X_ 100.0
    $node_(0) set Y_ 100.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 200.0
    $node_(1) set Y_ 200.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 300.0
    $node_(2) set Y_ 100.0
    $node_(2) set Z_ 0.0


    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3

    set ping4 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping4
    set ping5 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping5
    # connect two agents
    $ns_ connect $ping4 $ping5
    
    # start times for connections
    $ns_ at 0.1 "$ping0 send"
    $ns_ at 0.1 "$ping2 send"
    $ns_ at 0.1 "$ping4 send"
    
}

Test/unicast11 instproc run {} {
    $self next
}

Test/unicast11_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast11_sync
    set opt(nn) 3
    $self next

    $node_(0) set X_ 100.0
    $node_(0) set Y_ 100.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 200.0
    $node_(1) set Y_ 200.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 300.0
    $node_(2) set Y_ 100.0
    $node_(2) set Z_ 0.0


    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3

    set ping4 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping4
    set ping5 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping5
    # connect two agents
    $ns_ connect $ping4 $ping5
    
    # start times for connections
    $ns_ at 40.1 "$ping0 send"
    $ns_ at 40.1 "$ping2 send"
    $ns_ at 40.1 "$ping4 send"
    
}

Test/unicast11_sync instproc run {} {
    $self next
}


Test/unicast12 instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast12
    set opt(nn) 3
    set opt(err)            UniformErrorProc
    set opt(FECstrength)    1

    ErrorModel set rate_ 0.1
    ErrorModel set bandwidth_ 1Kb
    
    $self next

    $node_(0) set X_ 100.0
    $node_(0) set Y_ 100.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 200.0
    $node_(1) set Y_ 200.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 300.0
    $node_(2) set Y_ 100.0
    $node_(2) set Z_ 0.0


    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3

    set ping4 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping4
    set ping5 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping5
    # connect two agents
    $ns_ connect $ping4 $ping5
    
    # start times for connections
    $ns_ at 0.1 "$ping0 send"
    $ns_ at 0.1 "$ping2 send"
    $ns_ at 0.1 "$ping4 send"
    
}

Test/unicast12 instproc run {} {
    $self next
}

Test/unicast12_sync instproc init {} {
    global opt node_
    $self instvar ns_ net_ testname_ topo_ tracefd_
    
    set testname_ unicast12_sync
    set opt(nn) 3
    set opt(err)            UniformErrorProc
    set opt(FECstrength)    1

    ErrorModel set rate_ 0.1
    ErrorModel set bandwidth_ 1Kb
    
    $self next

    $node_(0) set X_ 100.0
    $node_(0) set Y_ 100.0
    $node_(0) set Z_ 0.0
    
    $node_(1) set X_ 200.0
    $node_(1) set Y_ 200.0
    $node_(1) set Z_ 0.0
    
    $node_(2) set X_ 300.0
    $node_(2) set Y_ 100.0
    $node_(2) set Z_ 0.0


    set ping0 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping0
    set ping1 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping1
    # connect two agents
    $ns_ connect $ping0 $ping1
    
    
    set ping2 [new Agent/Ping]
    $ns_ attach-agent $node_(1) $ping2
    set ping3 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping3
    # connect two agents
    $ns_ connect $ping2 $ping3

    set ping4 [new Agent/Ping]
    $ns_ attach-agent $node_(2) $ping4
    set ping5 [new Agent/Ping]
    $ns_ attach-agent $node_(0) $ping5
    # connect two agents
    $ns_ connect $ping4 $ping5
    
    # start times for connections
    $ns_ at 40.1 "$ping0 send"
    $ns_ at 40.1 "$ping2 send"
    $ns_ at 40.1 "$ping4 send"
    
}

Test/unicast12_sync instproc run {} {
    $self next
}


#Define a 'recv' function for the class 'Agent/Ping'
Agent/Ping instproc recv {from rtt} {
    $self instvar node_
    puts "node [$node_ id] received ping answer from \
              $from with round-trip-time $rtt ms."
}


TestSuite instproc finish {} {
    $self instvar ns_ tracefd_
    $ns_ flush-trace 
    close $tracefd_
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
