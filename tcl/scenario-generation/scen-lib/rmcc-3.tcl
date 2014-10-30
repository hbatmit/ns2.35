# rmcc-3.tcl
# Multiple RM flows scenario
#                  
#             RM sender1   
# topology:    2 o               o 6  RM recv 1
#                 \ 0         1 /
#    RM send2 3o---o-----------o--o 7  RM recv 2
#                 /| B/N link  |\
#                / |           | \   
#               o  o           o  o 8 RM recv 3
#    RM send3 4    5           9   
#                             CBR receiver1
#                CBR sender1 


# RM sender 1 starts to send at t=0s
# RM sender 2 starts to send at t=5s
# RM sender 3 starts to send at t=10
# CBR sender 1 starts to send at t=20s at 500Kbps
# CBR reduces its rate to 200Kbps at t=40s
# comparison of flow bandwidth for RM flows by flow monitors attached on
# links 1-6, 1-7, 1-8 and 1-9

# usage : ns rmcc-3.tcl
#


source rmcc.tcl

ScenLib/RM instproc make_topo3 { time } {
    global ns n t opts
    $self make_nodes 10
    #color them
    $n(0) color "red"
    $n(1) color "red"
    $n(2) color "orchid"
    $n(3) color "orchid"
    $n(4) color "orchid"
    $n(6) color "green"
    $n(7) color "green"
    $n(8) color "green"
    $n(4) color "black"
    $n(9) color "black"

    #now setup topology
	# bottleneck link
    $ns duplex-link $n(0) $n(1) $opts(bottleneckBW) \
			$opts(bottleneckDelay) DropTail
	$ns duplex-link-op $n(0) $n(1) queuePos 0.5
    $ns duplex-link $n(0) $n(2) 10Mb 5ms DropTail
    $ns duplex-link $n(0) $n(3) 10Mb 5ms DropTail
    $ns duplex-link $n(0) $n(4) 10Mb 5ms DropTail
    $ns duplex-link $n(0) $n(5) 10Mb 5ms DropTail
    $ns duplex-link $n(1) $n(6) 10Mb 5ms DropTail
    $ns duplex-link-op $n(1) $n(6) queuePos 0.5
    $ns duplex-link $n(1) $n(7) 10Mb 5ms DropTail
    $ns duplex-link-op $n(1) $n(7) queuePos 0.5
    $ns duplex-link $n(1) $n(8) 10Mb 5ms DropTail
    $ns duplex-link $n(1) $n(9) 10Mb 5ms DropTail
    
    $ns duplex-link-op $n(0) $n(1) orient right
    $ns duplex-link-op $n(0) $n(2) orient left-up
    $ns duplex-link-op $n(0) $n(3) orient left
	$ns duplex-link-op $n(0) $n(4) orient left-down
	$ns duplex-link-op $n(0) $n(5) orient down
	$ns duplex-link-op $n(1) $n(6) orient up
	$ns duplex-link-op $n(1) $n(7) orient right
	$ns duplex-link-op $n(1) $n(8) orient right-down
	$ns duplex-link-op $n(1) $n(9) orient down
	
	$ns queue-limit $n(0) $n(1) $opts(bottleneckQSize)
	$ns queue-limit $n(1) $n(0) $opts(bottleneckQSize)
	
	$self make_flowmon $time $n(1) $n(6) flowStats_1_6.$t \
		    $n(1) $n(7) flowStats_1_7.$t \
		    $n(1) $n(8) flowStats_1_8.$t \
		    $n(1) $n(9) flowStats_1_9.$t
}

proc run {} {
    global ns n
    set test_scen [new ScenLib/RM]
    $test_scen make_topo3 40.0
    $test_scen create_mcast 2 0.1 0.4 1.0 6
    $test_scen create_mcast 3 8.3 9.0 10.0 7
    $test_scen create_mcast 4 18.3 19.0 20.0 8
    $test_scen create_cbr 5 2.0 9 40.0 60.0 5 5.0 9 60.0 80.0 
    $test_scen dump_flowmon $n(1) $n(6) 10.0
    $test_scen dump_flowmon $n(1) $n(6) 20.0
    $test_scen dump_flowmon $n(1) $n(7) 20.0
    $test_scen dump_flowmon $n(1) $n(6) 40.0
    $test_scen dump_flowmon $n(1) $n(7) 40.0
    $test_scen dump_flowmon $n(1) $n(8) 40.0
    $test_scen dump_flowmon $n(1) $n(6) 60.0
    $test_scen dump_flowmon $n(1) $n(9) 60.0
    $test_scen dump_flowmon $n(1) $n(6) 80.0
    $test_scen dump_flowmon $n(1) $n(9) 80.0
    $ns at 80.0 "finish"
    $ns run
}

global argv prog opts t mflag
set mflag 0
if [string match {*.tcl} $argv0] {
    set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
    set prog $argv0
}

process_args $argv
set t $prog
run

