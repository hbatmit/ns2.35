Class Topology

Topology instproc node? n {
    $self instvar node_
    if [info exists node_($n)] {
        set ret $node_($n)
    } else {
        set ret ""
    }
    set ret
}

Topology  instproc init {ns noNodes noRouters} {
    $self next
    $self instvar node_
    
    for {set i 0} {$i < $noNodes} {incr i} {
	set node_(s$i) [$ns node]
	set node_(d$i) [$ns node]
    }

    for {set i 0} {$i < $noRouters} {incr i} {
	set node_(r$i) [$ns node]
    }
    
}

Topology instproc set_red_params { redq psize qlim min_th max_th bytes wait gentle} {
    $redq set mean_pktsize_ $psize
    $redq set limit_ $qlim
    $redq set bytes_ $bytes
    $redq set wait_ $wait
    $redq set gentle_ $gentle
    
    $redq set thresh_ $min_th
    $redq set maxthresh_ $max_th
}    

Class Topology/netAllUDP -superclass Topology
Topology/netAllUDP instproc init ns {
    $self next $ns 1 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_ 

    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 2ms DropTail
    $ns duplex-link $node_(d0) $node_(r1) 100Mb 2ms DropTail
    
    $ns simplex-link $node_(r0) $node_(r1) 10Mb 20ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb 20ms DropTail
    
	$self instvar bandwidth_
	set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    $self set_red_params $redpdq_ 1000 50 10 33 false false true
    
	#   $redpdq_ set debug_ true
    #	$redpdq_ set auto_ true
    #	$redpdq_ set global_target_ true
    #	set appropriate queue-limits
    #	$ns queue-limit $node_(r1) $node_(r2) 25
    
    #	$ns queue-limit $node_(r2) $node_(r1) 25
}

Class Topology/netMix -superclass Topology
Topology/netMix instproc init ns {
    $self next $ns 3 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 3ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 100Mb 13ms DropTail
    $ns duplex-link $node_(s2) $node_(r0) 100Mb 23ms DropTail
    
    $ns duplex-link $node_(d0) $node_(r1) 100Mb 1ms DropTail
    $ns duplex-link $node_(d1) $node_(r1) 100Mb 1.5ms DropTail
    $ns duplex-link $node_(d2) $node_(r1) 100Mb 2ms DropTail
    
    $ns simplex-link $node_(r0) $node_(r1) 10Mb 10ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb 10ms DropTail
    
	$self instvar bandwidth_
	set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    $self set_red_params $redpdq_ 1000 50 10 33 true false true
#    $redpdq_ set debug_ true
}

Class Topology/netTFRC -superclass Topology
Topology/netTFRC instproc init ns {
    $self next $ns 9 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 4ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 100Mb 49ms DropTail
     
    set delay 1
    for {set i 0} {$i < 9} {incr i} {
		$ns duplex-link $node_(d$i) $node_(r1) 100Mb ${delay}ms DropTail
		set delay [expr $delay+0.2]
    }
     
    $ns simplex-link $node_(r0) $node_(r1) 10Mb 10ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb 10ms DropTail
    
	$self instvar bandwidth_
	set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    $self set_red_params $redpdq_ 1000 50 10 33 false false true
#    $redpdq_ set debug_ true
}

Class Topology/netAllTCP -superclass Topology
Topology/netAllTCP instproc init ns {
    $self next $ns 4 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 9ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 100Mb 29ms DropTail
    $ns duplex-link $node_(s2) $node_(r0) 100Mb 49ms DropTail
    $ns duplex-link $node_(s3) $node_(r0) 100Mb 69ms DropTail
    
    $ns duplex-link $node_(d0) $node_(r1) 100Mb 1ms DropTail
    $ns duplex-link $node_(d1) $node_(r1) 100Mb 2ms DropTail
        
    $ns simplex-link $node_(r0) $node_(r1) 10Mb 10ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb 10ms DropTail
    
	$self instvar bandwidth_
	set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    $self set_red_params $redpdq_ 1000 100 10 33 false false true
    
#    $redpdq_ set debug_ true
}

# For Web Traffic Simulations
Class Topology/netWeb -superclass Topology
Topology/netWeb instproc init ns {
    $self next $ns 5 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 10ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 100Mb 20ms DropTail
    $ns duplex-link $node_(s2) $node_(r0) 100Mb 30ms DropTail
    $ns duplex-link $node_(s3) $node_(r0) 100Mb 40ms DropTail
    $ns duplex-link $node_(s4) $node_(r0) 100Mb 50ms DropTail

    $ns duplex-link $node_(d0) $node_(r1) 100Mb 1ms DropTail
    $ns duplex-link $node_(d1) $node_(r1) 100Mb 1.2ms DropTail
    $ns duplex-link $node_(d2) $node_(r1) 100Mb 1.4ms DropTail
    $ns duplex-link $node_(d3) $node_(r1) 100Mb 1.6ms DropTail
    $ns duplex-link $node_(d4) $node_(r1) 100Mb 1.8ms DropTail
     
    $ns simplex-link $node_(r0) $node_(r1) 10Mb 1ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb 1ms DropTail
    
	$self instvar bandwidth_
	set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_ Fid]
    $self set_red_params $redpdq_ 500 100 10 33 false false false
    
	#$redpdq_ set debug_ true
    
    return $redpdq_
}

#For multiple congested links
Class Topology/netMulti -superclass Topology
Topology/netMulti instproc init ns {
	
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_ noLinks_
    
	global topo_para1_
    set noLinks_ $topo_para1_

	if { $noLinks_ <=0 } {
		puts stderr "Invalid number of links $noLinks_"
	}

    set noNodes $noLinks_
    set noRouters [expr $noLinks_+1]

    $self next $ns $noNodes $noRouters
    
    for {set i 0} {$i<$noLinks_} {incr i} {
	set node_(s1$i) [$ns node]
	set node_(s2$i) [$ns node]
	set node_(s3$i) [$ns node]
    }

    set node_(source) [$ns node]
    set node_(sink) [$ns node]

    #create the necessary connections and stuff
    for {set i 0} {$i < $noNodes} {incr i} {
	
	$ns duplex-link $node_(s$i) $node_(r$i) 100Mb 10ms DropTail
	$ns duplex-link $node_(s1$i) $node_(r$i) 100Mb 20ms DropTail
	$ns duplex-link $node_(s2$i) $node_(r$i) 100Mb 30ms DropTail
	$ns duplex-link $node_(s3$i) $node_(r$i) 100Mb 40ms DropTail

	set j [expr $i+1]
	$ns duplex-link $node_(d$i) $node_(r$j) 100Mb 1ms DropTail
	
	$ns simplex-link $node_(r$i) $node_(r$j) 10Mb 1ms RED/PD MEDROP EDROP
	$ns simplex-link $node_(r$j) $node_(r$i) 10Mb 1ms DropTail
	    
	$self instvar bandwidth_
	set bandwidth_ 10000

	set redpdlink_($i) [$ns link $node_(r$i) $node_(r$j)]
	set redpdq_($i) [$redpdlink_($i) queue]
	set redpdflowmon_($i) [$redpdq_($i) makeflowmon $redpdlink_($i)]
	$self set_red_params $redpdq_($i) 1000 50 10 33 false false false
#	$redpdq_($i) set debug_ true
    }

    set node_(source) [$ns node]
    set node_(sink) [$ns node]
    set delay [expr 40 - $noLinks_]
    $ns duplex-link $node_(source) $node_(r0) 100Mb ${delay}ms DropTail
    $ns duplex-link $node_(sink) $node_(r$noLinks_) 100Mb 1ms DropTail
}

Class Topology/netTestFRp -superclass Topology
Topology/netTestFRp instproc init ns {
#    puts "In netTestFRp ..."

    $self next $ns 6 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
#    puts "In netTestFRp ..."

    global traf_para1_ traf_para2_ target_rtt_
#    puts "Topology parametsrs: $target_rtt_ ftype=$traf_para1_ gamma=$traf_para2_" 
    switch -exact $traf_para1_ {
	"tcp" {
	    set delay [expr ($target_rtt_/(2*$traf_para2_))*1000]
	}
	default {
	    set delay 10
	}
    }

    if { $delay <= 0 } {
	puts stderr "Invalid Link Delay"
	exit 1
    }

#    puts stderr "Delay = $delay ms"

    for {set i 0} {$i < 5} {incr i} {
	$ns duplex-link $node_(s$i) $node_(r0) 1000Mb [expr ${delay}+0.05*$i]ms DropTail
    }

    $ns duplex-link $node_(d0) $node_(r1) 100Mb 0ms DropTail

    $ns simplex-link $node_(r0) $node_(r1) 1000Mb 0ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 1000Mb 0ms DropTail
    
    $self instvar bandwidth_
    set bandwidth_ 1000000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    #excessively high values so that pkts are never dropped because of RED dynamics
    $self set_red_params $redpdq_ 1000 1000 1000 1000 false false true
    
    #    $redpdq_ set debug_ true
    
    #set the P in FRp while running the simulation for fixed drop rate
    #(-1 to operate in normal mode). 
    global topo_para1_
    $redpdq_ set P_testFRp_ $topo_para1_
}


Class Topology/netSingleVsMulti -superclass Topology
Topology/netSingleVsMulti instproc init ns {
    $self next $ns 5 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 0.2ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 100Mb 0.4ms DropTail
    $ns duplex-link $node_(s2) $node_(r0) 100Mb 0.6ms DropTail
    $ns duplex-link $node_(s3) $node_(r0) 100Mb 0.8ms DropTail
    $ns duplex-link $node_(s4) $node_(r0) 100Mb 1ms DropTail

    $ns duplex-link $node_(d0) $node_(r1) 100Mb 1ms DropTail
    
    global topo_para1_ 
    if {$topo_para1_ <= 0} {
	puts stderr "Invalid Round Trip Time $traf_para1_"
    }
    
    set one_way_delay [expr $topo_para1_*500 - 1]
    $ns simplex-link $node_(r0) $node_(r1) 10Mb ${one_way_delay}ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb ${one_way_delay}ms DropTail
    
    $self instvar bandwidth_
    set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    #RED parameters likely to show a bursty drop behavior
    set bw_delay_prod [expr ($one_way_delay+2.0)/(0.4)]
    puts "product = $bw_delay_prod"
    $self set_red_params $redpdq_ 1000 [expr {$bw_delay_prod/4}] [expr {$bw_delay_prod/8}] \
	[expr {$bw_delay_prod/4}] false false true
    
#    $redpdq_ set debug_ true
}


Class Topology/netPktsVsBytes -superclass Topology
Topology/netPktsVsBytes instproc init ns {
    $self next $ns 9 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 4ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 100Mb 34ms DropTail
     
    set delay 1
    for {set i 0} {$i < 9} {incr i} {
		$ns duplex-link $node_(d$i) $node_(r1) 100Mb ${delay}ms DropTail
		set delay [expr $delay+0.2]
    }
     
    $ns simplex-link $node_(r0) $node_(r1) 10Mb 5ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb 5ms DropTail

    $self instvar bandwidth_
    set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    $self set_red_params $redpdq_ 1000 50 10 33 true false true
    
#    global topo_para1_
#    $redpdq_ set P_testFRp_ $topo_para1_

#   $redpdq_ set debug_ true
}

Class Topology/netPktsVsBytes1 -superclass Topology
Topology/netPktsVsBytes1 instproc init ns {
    $self next $ns 9 2
    $self instvar node_ redpdq_ redpdflowmon_ redpdlink_
    
    $ns duplex-link $node_(s0) $node_(r0) 100Mb 4ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 100Mb 34ms DropTail
     
    set delay 1
    for {set i 0} {$i < 9} {incr i} {
		$ns duplex-link $node_(d$i) $node_(r1) 100Mb ${delay}ms DropTail
		set delay [expr $delay+0.2]
    }
     
    $ns simplex-link $node_(r0) $node_(r1) 10Mb 5ms RED/PD MEDROP EDROP
    $ns simplex-link $node_(r1) $node_(r0) 10Mb 5ms DropTail

    $self instvar bandwidth_
    set bandwidth_ 10000

    set redpdlink_ [$ns link $node_(r0) $node_(r1)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    $self set_red_params $redpdq_ 1000 50 10 33 false false true
    
#    global topo_para1_
#    $redpdq_ set P_testFRp_ $topo_para1_

#   $redpdq_ set debug_ true
}
