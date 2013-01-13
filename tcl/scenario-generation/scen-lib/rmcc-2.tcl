# rmcc-2.tcl
# Competing with TCP scenario 

#                  RM sender   
# topology:     2 o           o 5  RM recv1
#                  \ 0     1 /
#    TCP send1 3 o--o-------o--o 6  RM rec 2
#                  /        |\
#    TCP send2 4  o         | o 7  TCP recv 1
#                           o 8  TCP recv 2


# TCP sender 1 starts to send at t=0sec
# SRM sender starts to send at t=10s
# tcp sender 2 starts to send at t=20s
# BW of link 0-1 = 1 Mbps and delay is 20ms . For rest of the 
# topology BW = 10Mbps and delay of 5ms.
# flowmonitors compare thruput of TCP and rm flows in links 1-5, 1-6,
# 1-7 and 1-8.
#
# usage:  ns rmcc-2.tcl
#

source rmcc.tcl

ScenLib/RM instproc make_topo2 { time } {
    global ns n t opts
    $self make_nodes 9
    #color them
    $n(0) color "red"
    $n(1) color "red"
    $n(2) color "orchid"
    $n(5) color "orchid"
    $n(6) color "orchid"
    $n(3) color "green"
    $n(4) color "green"
    $n(7) color "green"
    $n(8) color "green"

    #now setup topology
	# First the bottle-neck link
	$ns duplex-link $n(0) $n(1) $opts(bottleneckBW) \
			$opts(bottleneckDelay) DropTail
	$ns duplex-link-op $n(0) $n(1) queuePos 0.5
    $ns duplex-link $n(0) $n(2) 10Mb 10ms DropTail
    $ns duplex-link $n(0) $n(3) 10Mb 10ms DropTail
    $ns duplex-link $n(0) $n(4) 10Mb 10ms DropTail
    $ns duplex-link $n(1) $n(5) 10Mb 10ms DropTail
    $ns duplex-link $n(1) $n(6) 10Mb 10ms DropTail
    $ns duplex-link $n(1) $n(7) 10Mb 10ms DropTail
    $ns duplex-link $n(1) $n(8) 10Mb 10ms DropTail
    
    $ns duplex-link-op $n(0) $n(1) orient right
    $ns duplex-link-op $n(0) $n(2) orient left-up
    $ns duplex-link-op $n(0) $n(3) orient left
    $ns duplex-link-op $n(0) $n(4) orient left-down
	$ns duplex-link-op $n(1) $n(5) orient rightup
    $ns duplex-link-op $n(1) $n(6) orient right
    $ns duplex-link-op $n(1) $n(7) orient right-down
    $ns duplex-link-op $n(1) $n(8) orient down

    $ns queue-limit $n(0) $n(1) $opts(bottleneckQSize)
    $ns queue-limit $n(1) $n(0) $opts(bottleneckQSize)
    
    $self make_flowmon $time $n(1) $n(5) flowStats_1_5.$t \
	    $n(1) $n(6) flowStats_1_6.$t \
	    $n(1) $n(7) flowStats_1_7.$t \
	    $n(1) $n(8) flowStats_1_8.$t
}



proc main {} {
    global ns n
    set test_scen [new ScenLib/RM]
    $test_scen make_topo2 1.0
    $test_scen create_mcast 2 8.3 9.0 10.0 5 6
    $test_scen create_tcp 3 7 0.0
    $test_scen create_tcp 4 8 20.0
    $test_scen dump_flowmon $n(1) $n(7) 10.0
    $test_scen dump_flowmon $n(1) $n(5) 20.0
    $test_scen dump_flowmon $n(1) $n(7) 20.0
    $test_scen dump_flowmon $n(1) $n(5) 30.0
    $test_scen dump_flowmon $n(1) $n(6) 30.0
    $test_scen dump_flowmon $n(1) $n(7) 30.0
    $test_scen dump_flowmon $n(1) $n(8) 30.0
    $ns at 30.0 "finish"
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
main


