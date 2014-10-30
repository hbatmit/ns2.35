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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-wireless-lan.tcl,v 1.27 2006/01/24 23:00:08 sallyfloyd Exp $

# THIS TEST IS SUBSUMED BY TEST-SUITE-WIRELESS-LAN-NEWNODE.TCL 
# - Sep 15, 2000

puts "THIS TEST IS SUBSUMED BY TEST-SUITE-WIRELESS-LAN-NEWNODE.TCL" 
exit 0
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

# This test suite is for validating wireless lans 
# To run all tests: test-all-wireless-lan
# to run individual test:
# ns test-suite-wireless-lan.tcl dsdv
# ns test-suite-wireless-lan.tcl dsr
# ns test-suite-wireless-lan.tcl wired-cum-wireless
# ns test-suite-wireless-lan.tcl wireless-mip
# ....
#
# To view a list of available test to run with this script:
# ns test-suite-wireless-lan.tcl
#

Class TestSuite

# wireless model using destination sequence distance vector
Class Test/dsdv -superclass TestSuite

# wireless model using dynamic source routing
Class Test/dsr -superclass TestSuite

# simulation between a wired and a wireless domain through
# gateways called base-stations.
Class Test/dsdv-wired-cum-wireless -superclass TestSuite

# same as above , only with DSR routing. see test-suite-wireless-lan.
# txt for details
#Class Test/dsr-wired-cum-wireless -superclass TestSuite

# Wireless Mobile IP model in which a mobilehost roams between 
# a Home Agent and Foreign Agent. see test-suite-wireless-lan.txt for
# details
Class Test/dsdv-wireless-mip -superclass TestSuite

# same as above, only with DSR routing
#Class Test/dsr-wireless-mip -superclass TestSuite

# XXX The dsr version of the tests are not added to test-suite as their 
# outputs are not consistent (events vary, even with the same random seed 
# value) and thus couldnot be validated. -Padma, may 1999.

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: dsdv dsr"
	exit 1
}

proc default_options {} {
    global opt
    set opt(chan)	Channel/WirelessChannel
    set opt(prop)	Propagation/TwoRayGround
    set opt(netif)	Phy/WirelessPhy
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
Phy/WirelessPhy set Pt_ 0.28183815
Phy/WirelessPhy set freq_ 914e+6 
Phy/WirelessPhy set L_ 1.0

# =====================================================================

TestSuite instproc init {} {
	global opt tracefd topo chan prop 
	global node_ god_ 
	$self instvar ns_ testName_
	set ns_         [new Simulator]
	# This is really confusing. Why not a simple test 
	# ($testName_ != "dsdv") && ($testName_ != "dsr") ?
	if {[string compare $testName_ "dsdv"] && \
			[string compare $testName_ "dsr"]} {
		# XXX We explicitly test HierNode. Since set-address-format
		# is used by both the new and the old code, we can't add a 
		# warning there to say that it's obsolete; nor can we keep 
		# this set node_factory_ stuff there. So the following is 
		# the only solution. Ugly but it works.
		Simulator set node_factory_ HierNode
		$ns_ set-address-format hierarchical
		AddrParams set domain_num_ 3
		lappend cluster_num 2 1 1
		AddrParams set cluster_num_ $cluster_num
		lappend eilastlevel 1 1 4 1
		AddrParams set nodes_num_ $eilastlevel
        }  
	set chan	[new $opt(chan)]
	set prop	[new $opt(prop)]
	set topo	[new Topography]
	set tracefd	[open $opt(tr) w]

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
	
	puts $tracefd "M 0.0 nn:$opt(nn) x:$opt(x) y:$opt(y) rp:$opt(rp)"
	puts $tracefd "M 0.0 sc:$opt(sc) cp:$opt(cp) seed:$opt(seed)"
	puts $tracefd "M 0.0 prop:$opt(prop) ant:$opt(ant)"
}


Test/dsdv instproc init {} {
    global opt node_ god_
    $self instvar ns_ testName_
    set testName_       dsdv
    set opt(rp)         dsdv
    set opt(cp)		"../mobility/scene/cbr-50-20-4-512" 
    set opt(sc)		"../mobility/scene/scen-670x670-50-600-20-0" ;
    set opt(nn)		50	      
    set opt(stop)       1000.0
    
    $self next

    for {set i 0} {$i < $opt(nn) } {incr i} {
	$testName_-create-mobile-node $i
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
    #$ns_ halt"
    $ns_ at $opt(stop).1 "$self finish"
}

Test/dsdv instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}


Test/dsr instproc init {} {
    global opt node_ god_
    $self instvar ns_ testName_
    set testName_       dsr
    set opt(rp)         dsr
    set opt(cp)         "../mobility/scene/cbr-50-20-4-512"
    set opt(sc)         "../mobility/scene/scen-670x670-50-600-20-0" ;
    set opt(nn)         50
    set opt(stop)       1000.0

    $self next

    puts "creating mobile nodes"
    for {set i 0} {$i < $opt(nn) } {incr i} {
        $testName_-create-mobile-node $i
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

	puts "finish.."
	exit 0
	

}

Test/dsr instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}
#

Test/dsdv-wired-cum-wireless instproc init {} {
    global opt god_ node_ 
    $self instvar ns_ testName_
    set testName_ dsdv-wired-cum-wireless
    set opt(rp)            dsdv
    set opt(sc)            "../mobility/scene/scen-3-test"
    set opt(cp)            ""
    set opt(nn)            5
    set opt(stop)          500.0
    set num_wired_nodes    2
    set num_wireless_nodes 3

    $self next

    #setup wired nodes
    set temp {0.0.0 0.1.0}
    for {set i 0} {$i < $num_wired_nodes} {incr i} {
	set W($i) [$ns_ node [lindex $temp $i]] 
    }
    
    # setup base stations & wireless nodes
    set temp {1.0.0 1.0.1 1.0.2 1.0.3}
    set BS(0) [create-base-station-node [lindex $temp 0]]
    set BS(1) [create-base-station-node 2.0.0]

    #provide some co-ord (fixed) to base stations
    $BS(0) set X_ 1.0
    $BS(0) set Y_ 2.0
    $BS(0) set Z_ 0.0
    
    $BS(1) set X_ 650.0
    $BS(1) set Y_ 600.0
    $BS(1) set Z_ 0.0
    
    #create some mobilenodes in the same domain as BS_0
    for {set j 0} {$j < $num_wireless_nodes} {incr j} {
	set node_($j) [ $opt(rp)-create-mobile-node $j [lindex $temp \
		[expr $j+1]] ]
	$node_($j) base-station [AddrParams addr2id \
		[$BS(0) node-addr]]
    }
    
    puts "Loading connection pattern..."
    $self create-udp-traffic 0 $node_(0) $W(0) 240.00000000000000
    $self create-tcp-traffic 0 $W(1) $node_(2) 160.00000000000000
    $self create-tcp-traffic 1 $node_(0) $W(0) 200.00000000000000
    
    puts "Loading scenario file..."
    source $opt(sc)
    puts "Load complete..."
 
    #create links between wired and BS nodes
    
    $ns_ duplex-link $W(0) $W(1) 5Mb 2ms DropTail
    $ns_ duplex-link $W(1) $BS(0) 5Mb 2ms DropTail
    $ns_ duplex-link $W(1) $BS(1) 5Mb 2ms DropTail
    
    $ns_ duplex-link-op $W(0) $W(1) orient down
    $ns_ duplex-link-op $W(1) $BS(0) orient left-down
    $ns_ duplex-link-op $W(1) $BS(1) orient right-down
    
    #
    # Tell all the nodes when the simulation ends
    #
    for {set i 0} {$i < $num_wireless_nodes} {incr i} {
	$ns_ at $opt(stop).000000001 "$node_($i) reset";
    }
    $ns_ at $opt(stop).0000010 "$BS(0) reset";
    $ns_ at $opt(stop).0000010 "$BS(1) reset";
	
    $ns_ at $opt(stop).20 "puts \"NS EXITING...\" ;" 
    $ns_ at $opt(stop).21 "$self finish"
    
}

Test/dsdv-wired-cum-wireless instproc run {} {
    $self instvar ns_ 
    puts "Starting Simulation..."
    $ns_ run
}


Test/dsdv-wireless-mip instproc init {} {
    global opt god_ node_ 
    $self instvar ns_ testName_
    
    set testName_ dsdv-wireless-mip
    set opt(rp)            dsdv
    set opt(sc)            "../mobility/scene/scen-3-test"
    set opt(cp)            ""
    set opt(nn)            3       ;#total no of wireless nodes
    set opt(stop)          250.0
    set num_wired_nodes    2
    set num_wireless_nodes 1
    source ../lib/ns-wireless-mip.tcl
    
    $self next
    
    # set mobileIP flag
    Simulator set mobile_ip_ 1

    ## setup the wired nodes
    set temp {0.0.0 0.1.0}
    for {set i 0} {$i < $num_wired_nodes} {incr i} {
	set W($i) [$ns_ node [lindex $temp $i]] 
    }
    
    ## setup ForeignAgent and HomeAgent nodes
    set HA [create-base-station-node 1.0.0]
    set FA [create-base-station-node 2.0.0]
    
    #provide some co-ord (fixed) to these base-station nodes.
    $HA set X_ 1.000000000000
    $HA set Y_ 2.000000000000
    $HA set Z_ 0.000000000000
    
    $FA set X_ 650.000000000000
    $FA set Y_ 600.000000000000
    $FA set Z_ 0.000000000000
    
    # create a mobilenode that would be moving between HA and FA.
    # note address of MH indicates its in the same domain as HA.
    
    set MH [$opt(rp)-create-mobile-node 0 1.0.2]
    set HAaddress [AddrParams addr2id [$HA node-addr]]
    [$MH set regagent_] set home_agent_ $HAaddress
    
    # movement of MH
    $MH set Z_ 0.000000000000
    $MH set Y_ 2.000000000000
    $MH set X_ 2.000000000000
    # starts to move towards FA
    $ns_ at 100.000000000000 "$MH setdest 640.000000000000 610.000000000000 20.000000000000"
    # goes back to HA
    $ns_ at 200.000000000000 "$MH setdest 2.000000000000 2.000000000000 20.000000000000"
    
    # create links between wired and BaseStation nodes
    $ns_ duplex-link $W(0) $W(1) 5Mb 2ms DropTail
    $ns_ duplex-link $W(1) $HA 5Mb 2ms DropTail
    $ns_ duplex-link $W(1) $FA 5Mb 2ms DropTail
    
    $ns_ duplex-link-op $W(0) $W(1) orient down
    $ns_ duplex-link-op $W(1) $HA orient left-down
    $ns_ duplex-link-op $W(1) $FA orient right-down
    
    # setup TCP connections between a wired node and the MH
    puts "Loading connection pattern..."
    $self create-tcp-traffic 0 $W(0) $MH 100.0
    #
    # Tell all the nodes when the simulation ends
    #
    for {set i 0} {$i < $num_wireless_nodes } {incr i} {
	$ns_ at $opt(stop).0000010 "$node_($i) reset";
    }
    $ns_ at $opt(stop).0000010 "$HA reset";
    $ns_ at $opt(stop).0000010 "$FA reset";
    
    $ns_ at $opt(stop).21 "$self finish"
    $ns_ at $opt(stop).20 "puts \"NS EXITING...\" ; "
}

Test/dsdv-wireless-mip instproc run {} {
    $self instvar ns_ 
    puts "Starting Simulation..."
    $ns_ run
}

proc cmu-trace { ttype atype node } {
	global ns tracefd sc
    
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

TestSuite instproc finish {} {
	$self instvar ns_
	global quiet

	$ns_ flush-trace
        #if { !$quiet } {
        #        puts "running nam..."
        #        exec nam temp.rands.nam &
        #}
	#puts "finishing.."
	
	puts "finishing.."
	exit 0
}




TestSuite instproc create-god { nodes } {
	global tracefd god_
	$self instvar ns_

	set god_ [new God]
	$god_ num_nodes $nodes
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









