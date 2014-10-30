# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set SetCWRonRetransmit_ true
# Changing the default value.
Mac/802_11 set bugFix_timer_ false;     # default changed 2006/1/30
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-WLtutorial.tcl,v 1.18 2006/01/30 21:27:52 mweigle Exp $

###########################################################################
# IMPORTANT NOTE:
# If you are updating any part of this WLtutorial test-suite, please
# update the examples (wireless1.tcl, wireless2.tcl & wireless3.tcl) used
# for wireless tutorial under vint/tutorial/example directory.
###########################################################################

# This test suite is for validating wireless examples used in Greis' tutorial
# To run all tests: test-all-WLtutorial
# to run individual test:
# ns test-suite-WLtutorial.tcl wireless1
# ns test-suite-WLtutorial.tcl wireless2
# ns test-suite-WLtutorial.tcl wireless3
# ....
#
# To view a list of available test to run with this script:
# ns test-suite-WLtutorial.tcl
#

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Class TestSuite

# tutorial example of a simple wireless scenario
Class Test/wireless1 -superclass TestSuite

# tutorial example involving a wired-cum-wireless scenario
Class Test/wireless2 -superclass TestSuite

# tutorial example on wirelessMIP
Class Test/wireless3 -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
        puts "Valid Tests: wireless1 wireless2 wireless3"
	exit 1
}

proc default-options {} {
global opt
set opt(chan)       Channel/WirelessChannel
set opt(prop)       Propagation/TwoRayGround
set opt(netif)      Phy/WirelessPhy
set opt(mac)        Mac/802_11
set opt(ifq)        Queue/DropTail/PriQueue
set opt(ll)         LL
set opt(ant)        Antenna/OmniAntenna
set opt(x)              670   
set opt(y)              670   
set opt(ifqlen)         50           
set opt(tr)          temp.rands
}    

# =====================================================================

Test/wireless1 instproc init {} {
  global opt
  $self instvar ns_
	set opt(adhocRouting)   DSR
	set opt(ifq)            CMUPriQueue
  set opt(nn)             3             
  set opt(cp)             "../mobility/scene/cbr-3-test"
  set opt(sc)             "../mobility/scene/scen-3-test"
  set opt(stop)           400.0           
    
  set ns_         [new Simulator]  
  set topo        [new Topography] 

  set tracefd     [open $opt(tr) w]
  $ns_ trace-all $tracefd

  $topo load_flatgrid $opt(x) $opt(y)
  set god_ [create-god $opt(nn)]

  $ns_ node-config -adhocRouting $opt(adhocRouting) \
                 -llType $opt(ll) \
                 -macType $opt(mac) \
                 -ifqType $opt(ifq) \
                 -ifqLen $opt(ifqlen) \
                 -antType $opt(ant) \
                 -propInstance [new $opt(prop)] \
                 -phyType $opt(netif) \
                 -channel [new $opt(chan)] \
                 -topoInstance $topo \
                 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace OFF
   for {set i 0} {$i < $opt(nn) } {incr i} {
        set node_($i) [$ns_ node]
        $node_($i) random-motion 0              ;# disable random motion
 }
  puts "Loading connection pattern..."
  source $opt(cp) 
  puts "Loading scenario file..."
  source $opt(sc) 
  
  for {set i 0} {$i < $opt(nn)} {incr i} {
    $ns_ initial_node_pos $node_($i) 20
  }
  for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
  }
  $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"  
}

Test/wireless1 instproc run {} {
	$self instvar ns_
	puts "Starting Simulation..."
	$ns_ run
}

Test/wireless2 instproc init {} {
  global opt
  $self instvar ns_
  set opt(nn)             3                       
  set opt(adhocRouting)   DSDV                      
  set opt(cp)             ""                        
  set opt(sc)             "../mobility/scene/scen-3-test"   
  set opt(stop)           250                           
  set num_wired_nodes      2
  set num_bs_nodes         1

  set ns_   [new Simulator]

  # set up for hierarchical routing
  $ns_ node-config -addressType hierarchical
  AddrParams set domain_num_ 2           
  lappend cluster_num 2 1                
  AddrParams set cluster_num_ $cluster_num
  lappend eilastlevel 1 1 4              
  AddrParams set nodes_num_ $eilastlevel 

  set tracefd  [open $opt(tr) w]
  $ns_ trace-all $tracefd

  set topo   [new Topography]
  $topo load_flatgrid $opt(x) $opt(y)
  create-god [expr $opt(nn) + $num_bs_nodes]

  #create wired nodes
  set temp {0.0.0 0.1.0}        
  for {set i 0} {$i < $num_wired_nodes} {incr i} {
      set W($i) [$ns_ node [lindex $temp $i]]
  } 
  $ns_ node-config -adhocRouting $opt(adhocRouting) \
                 -llType $opt(ll) \
                 -macType $opt(mac) \
                 -ifqType $opt(ifq) \
                 -ifqLen $opt(ifqlen) \
                 -antType $opt(ant) \
                 -propInstance [new $opt(prop)] \
                 -phyType $opt(netif) \
                 -channel [new $opt(chan)] \
                 -topoInstance $topo \
                 -wiredRouting ON \
                 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace OFF

  set temp {1.0.0 1.0.1 1.0.2 1.0.3}   
  set BS(0) [$ns_ node [lindex $temp 0]]
  $BS(0) random-motion 0 
  $BS(0) set X_ 1.0
  $BS(0) set Y_ 2.0
  $BS(0) set Z_ 0.0

  #configure for mobilenodes
  $ns_ node-config -wiredRouting OFF

  for {set j 0} {$j < $opt(nn)} {incr j} {
    set node_($j) [ $ns_ node [lindex $temp \
            [expr $j+1]] ]
    $node_($j) base-station [AddrParams addr2id [$BS(0) node-addr]]
  }
  #create links between wired and BS nodes
  $ns_ duplex-link $W(0) $W(1) 5Mb 2ms DropTail
  $ns_ duplex-link $W(1) $BS(0) 5Mb 2ms DropTail
  $ns_ duplex-link-op $W(0) $W(1) orient down
  $ns_ duplex-link-op $W(1) $BS(0) orient left-down

  # setup TCP connections
  set tcp1 [new Agent/TCP]
  $tcp1 set class_ 2
  set sink1 [new Agent/TCPSink]
  $ns_ attach-agent $node_(0) $tcp1
  $ns_ attach-agent $W(0) $sink1
  $ns_ connect $tcp1 $sink1
  set ftp1 [new Application/FTP]
  $ftp1 attach-agent $tcp1
  $ns_ at 160 "$ftp1 start"

  for {set i 0} {$i < $opt(nn)} {incr i} {
      $ns_ initial_node_pos $node_($i) 20
   }

  for {set i } {$i < $opt(nn) } {incr i} {
      $ns_ at $opt(stop).0000010 "$node_($i) reset";
  }
  $ns_ at $opt(stop).0000010 "$BS(0) reset";

  $ns_ at $opt(stop).1 "puts \"NS EXITING...\" ; $ns_ halt"
}

Test/wireless2 instproc run {} {
  $self instvar ns_
  puts "Starting Simulation..."
  $ns_ run 
}
  
Test/wireless3 instproc init {} {
  global opt
  $self instvar ns_
  set opt(nn)             1                      
  set opt(adhocRouting)   DSDV                   
  set opt(cp)             ""                             
  set opt(sc)             ""
  set opt(stop)           250                            
  set num_wired_nodes      2
  set num_bs_nodes         2

  # create simulator instance
  set ns_   [new Simulator]

  # set up for hierarchical routing
  $ns_ node-config -addressType hierarchical
  AddrParams set domain_num_ 3           
  lappend cluster_num 2 1 1              
  AddrParams set cluster_num_ $cluster_num
  lappend eilastlevel 1 1 2 1            
  AddrParams set nodes_num_ $eilastlevel 

  set tracefd  [open $opt(tr) w]
  $ns_ trace-all $tracefd

  set topo   [new Topography]
  $topo load_flatgrid $opt(x) $opt(y)
  # 2 for the FA / the HA
  create-god [expr $opt(nn)  + 2]

  #create wired nodes
  set temp {0.0.0 0.1.0}           
  for {set i 0} {$i < $num_wired_nodes} {incr i} {
      set W($i) [$ns_ node [lindex $temp $i]]
  }
  #Configure for ForeignAgent and HomeAgent nodes
  $ns_ node-config -mobileIP ON \
                 -adhocRouting $opt(adhocRouting) \
                 -llType $opt(ll) \
                 -macType $opt(mac) \
                 -ifqType $opt(ifq) \
                 -ifqLen $opt(ifqlen) \
                 -antType $opt(ant) \
                 -phyType $opt(netif) \
                 -propInstance [new $opt(prop)] \
                 -channel [new $opt(chan)] \
                 -topoInstance $topo \
                 -wiredRouting ON \
                 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace OFF

  # Create HA and FA
  set HA [$ns_ node 1.0.0]
  set FA [$ns_ node 2.0.0]     
  $HA random-motion 0
  $FA random-motion 0
  # Position (fixed) for base-station nodes (HA & FA).
  $HA set X_ 1.000000000000
  $HA set Y_ 2.000000000000
  $HA set Z_ 0.000000000000

  $FA set X_ 650.000000000000
  $FA set Y_ 600.000000000000
  $FA set Z_ 0.000000000000

  $ns_ node-config -wiredRouting OFF
  set MH [$ns_ node 1.0.2]
  set node_(0) $MH
  set HAaddress [AddrParams addr2id [$HA node-addr]]
  [$MH set regagent_] set home_agent_ $HAaddress

  $MH set Z_ 0.000000000000
  $MH set Y_ 2.000000000000
  $MH set X_ 2.000000000000

  # MH starts to move towards FA
  $ns_ at 100.000000000000 "$MH setdest 640.000000000000 \
	610.000000000000 20.000000000000"
  # MH goes back to HA
  $ns_ at 200.000000000000 "$MH setdest 2.000000000000 \
	2.000000000000 20.000000000000"

  # create links between wired and BaseStation nodes
  $ns_ duplex-link $W(0) $W(1) 5Mb 2ms DropTail
  $ns_ duplex-link $W(1) $HA 5Mb 2ms DropTail
  $ns_ duplex-link $W(1) $FA 5Mb 2ms DropTail

  $ns_ duplex-link-op $W(0) $W(1) orient down
  $ns_ duplex-link-op $W(1) $HA orient left-down
  $ns_ duplex-link-op $W(1) $FA orient right-down 

  set tcp1 [new Agent/TCP]
  $tcp1 set class_ 2
  set sink1 [new Agent/TCPSink]
  $ns_ attach-agent $W(0) $tcp1
  $ns_ attach-agent $MH $sink1
  $ns_ connect $tcp1 $sink1
  set ftp1 [new Application/FTP]
  $ftp1 attach-agent $tcp1
  $ns_ at 100.0 "$ftp1 start"

  for {set i 0} {$i < $opt(nn)} {incr i} {
    $ns_ initial_node_pos $node_($i) 20
  }
  for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).0000010 "$node_($i) reset"
  }
  $ns_ at $opt(stop).0000010 "$HA reset"
  $ns_ at $opt(stop).0000010 "$FA reset"
  $ns_ at $opt(stop).1 "puts \"NS EXITING...\" ; $ns_ halt"
}

Test/wireless3 instproc finish {} {
	$self instvar ns_
	$ns_ flush-trace
	exit 0
}

Test/wireless3 instproc run {} {
  $self instvar ns_
  puts "Starting Simulation..."	 
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
default-options
runtest $argv
