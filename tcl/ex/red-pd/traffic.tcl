Class Traffic/Mix -superclass TestSuite
TestSuite instproc trafficMix {} {
    $self instvar node_
    
    set fid 1
    for {set i 0} {$i< 3} {incr i} {
		for {set j 0} {$j< 3} {incr j} {
			#$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts 
			$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
			incr fid
		}
    }

    #50% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0016 $fid 0   
    incr fid

#    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 500 .0008 $fid 0   
#    incr fid
	
    #45% of the link
#    $self new_Cbr [$self rnd 10] $node_(s1) $node_(d1) 1000 .0018 $fid 0   
#    incr fid

    #40% of the link
#    $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0021 $fid 0   
#    incr fid

    #35% of the link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s2) $node_(s5) 1000 .0023 $fid 0   
#    incr fid 

    #30% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s1) $node_(d1) 1000  .0026 $fid 0   
    incr fid 

    #20% of the link bandwidth
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .004 $fid 0   
#     incr fid
    
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .004 $fid 0   
#     incr fid
    
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 500 .002 $fid 0   
#     incr fid

#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 500 .002 $fid 0   
#     incr fid
   
    #15% of the link bandwidth
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0052 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0052 $fid 0   
#     incr fid
#    $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0052 $fid 0   
#    incr fid
  
  
    #10% of the link bandwidth
	$self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .008 $fid 0   
    incr fid

#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
    
    #2% of link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .04 $fid 0   
#    incr fid
    
    #4% of link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .02 $fid 0   
#    incr fid

    #varying cbr 0.25*f(R,p) -> 4*f(R,p)
#     $self new_VaryingCbr [$self rnd 10] $node_(s2) $node_(d2) 1000 0.02262742 $fid 0   
	
}

Class Traffic/Varying -superclass TestSuite
TestSuite instproc trafficVarying {} {
    $self instvar node_
    
    set fid 1
    for {set i 0} {$i< 3} {incr i} {
		for {set j 0} {$j< 3} {incr j} {
			#$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts 
			$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
			incr fid
		}
    }

    #varying cbr 0.25Mbps -> 4Mbps
	$self new_VaryingCbr [$self rnd 10] $node_(s2) $node_(d2) 1000 0.032 $fid 0   

}

Class Traffic/AllTCP -superclass TestSuite
TestSuite instproc trafficAllTCP {} {
    $self instvar node_
    
    set fid 1
    for {set i 0} {$i< 4} {incr i} {
		for {set j 0} {$j< 2} {incr j} {
			#$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts 
			$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
			incr fid
			if {$i == 3 } {
				for {set k 0} {$k < 3} {incr k} {
					$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
					incr fid
				}
			}
		}
	}
}

Class Traffic/AllUDP -superclass TestSuite
TestSuite instproc trafficAllUDP {} {

    $self instvar node_ 
    
    set fid 1
    
    #1% of the link 
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .08 $fid 0   
    incr fid 	

    #5% of the link 
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .016 $fid 0   
    incr fid 
    
    #10% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .008 $fid 0   
    incr fid 
    
    #15% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0053 $fid 0   
    incr fid

    #20% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .004 $fid 0   
    incr fid 
    
    #25% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0032 $fid 0   
    incr fid 
    
    #30% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0026 $fid 0   
    incr fid 
    
    #35% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0023 $fid 0   
    incr fid 
	
    #40% of the link
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .002 $fid 0   
    incr fid

    #45% of the link
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0018 $fid 0   
    incr fid

    #50% of the link
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0016 $fid 0   
    incr fid
}

Class Traffic/TestIdent -superclass TestSuite
TestSuite instproc trafficTestIdent {} {

    $self instvar node_  topo_
    $topo_ instvar redpdq_

    global traf_para1_ target_rtt_ traf_para2_
    set fid 1

    switch -exact $traf_para1_ {
	"tcp" {
	    for {set i 0} {$i< 5} {incr i} {
		#$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts 
		$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d0) 50 $fid 1 1000 sack 0
		incr fid
	    }
	}
	"cbr" { 
	    set p [$redpdq_ set P_testFRp_]
	    set frp [expr sqrt(1.5)/($target_rtt_*sqrt($p))]
	    set rate [expr {1.0/($traf_para2_*$frp)}]
	    
	    for {set i 0} {$i< 5} {incr i} {
		$self new_Cbr [$self rnd 1] $node_(s$i) $node_(d0) 1000 $rate $fid 0   
		incr fid
	    }
	}
	default {
	    puts stderr "Unknown Traffic Type"
	    exit 1
	}
    }
}

Class Traffic/Web -superclass TestSuite
TestSuite instproc trafficWeb {} {

    $self instvar node_
    
    Agent/TCP set packetSize_ 500
    Agent/UDP set packetSize_ 500
    
    PagePool/WebTraf set TCPTYPE_ Sack1
    PagePool/WebTraf set TCPSINKTYPE_ TCPSink/Sack1
    PagePool/WebTraf set FID_ASSIGNING_MODE_ 1
	PagePool/WebTraf set VERBOSE_ 1
	
   set fid 1
    
    for {set i 0} {$i<5} {incr i} {
	#$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts
	$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$i) 50 $fid 1 500 sack 0
	incr fid
	
	$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d[expr ($i+1)%5]) 50 $fid 1 500 sack 0
	incr fid
	
	#reverse
	# $self new_Tcp [$self rnd 10] $node_(s$j) $node_(s$i) 50 $fid 0 1000 sack 0
	# incr fid
    }
	
    #20% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s3) $node_(d3) 500 .002 $fid 0   
    incr fid 
    
    set pool [new PagePool/WebTraf]
    $pool instvar maxFid_
	
    set maxFid_ $fid

    #something like this should have been there 
    # instead of the ugly hack I put in place in tcl/webcache/webtraf.tcl
    # WebPage set LASTOBJ_ 10

    $pool set-num-client 5
    $pool set-num-server 5

    for {set i 0} {$i < 5} {incr i} {
	$pool set-server $i $node_(s$i)
    }

    for {set i 0} {$i < 5} {incr i} {
	$pool set-client $i $node_(d$i)
    }

    set numSession 50
    $pool set-num-session $numSession
    
    set interSession [new RandomVariable/Exponential]
    $interSession set avg_ 1
    
    set sessionSize [new RandomVariable/Constant]
    $sessionSize set val_ 100
    
    set launchTime 0
    for {set i 0} {$i < $numSession} {incr i} {
	
	    #number of pages in a session
	    set numPage [$sessionSize value]
	    
	    #time between requesting various pages
	    set interPage [new RandomVariable/Exponential]
	    $interPage set avg_ 10
	    #set interPage [new RandomVariable/Constant]
	    #$interPage set val_ 10
	    
	    # number of objects in a page
	    set pageSize [new RandomVariable/Exponential]
	    $pageSize set avg_ 10
	    #set pageSize [new RandomVariable/Constant]
	    #$pageSize set val_ 10
	    
	    # the time between requesting various objects
	    set interObj [new RandomVariable/Exponential]
	    $interObj set avg_ 0.01
	    #set interObj [new RandomVariable/Constant]
	    #$interObj set val_ 0.01
	    
	    #the size of objects being requested
	    set objSize [new RandomVariable/ParetoII]
	    $objSize set avg_ 24
	    $objSize set shape_ 1.2
	    #set objSize [new RandomVariable/Constant]
	    #$objSize set val_ 24
	    
	    
	    $pool create-session $i $numPage [expr $launchTime + 0.1] \
		$interPage $pageSize $interObj $objSize
	    set launchTime [expr $launchTime + [$interSession value]]
	    
    }


}

Class Traffic/Multi -superclass TestSuite
TestSuite instproc trafficMulti {noLinks_} {
	$self instvar node_ 
	
	set fid 1

	for {set link 0} {$link < $noLinks_} {incr link} {
	    for {set i 0} {$i < 2} {incr i} {
		
		$self new_Tcp [$self rnd 10] $node_(s$link) $node_(d$link) 50 $fid 1 1000 sack 0
		incr fid
		
 		$self new_Tcp [$self rnd 10] $node_(s1$link) $node_(d$link) 50 $fid 1 1000 sack 0
 		incr fid

 		$self new_Tcp [$self rnd 10] $node_(s2$link) $node_(d$link) 50 $fid 1 1000 sack 0
 		incr fid

		$self new_Tcp [$self rnd 10] $node_(s3$link) $node_(d$link) 50 $fid 1 1000 sack 0
		incr fid
	}
			
 	    for {set i 0} {$i < 2} {incr i} {
		#4.0 Mbps
			$self new_Cbr [$self rnd 10] $node_(s$link) $node_(d$link) 1000 .0020 $fid 0   
			incr fid
 	    }
	}
	
	global traf_para1_
	switch -exact $traf_para1_ {
		"cbr" {
			#1.0 Mbps
			$self new_Cbr [$self rnd 10] $node_(source) $node_(sink) 1000 .008 $fid 0 
			incr fid
		}
		"tcp" {
			$self new_Tcp [$self rnd 10] $node_(source) $node_(sink) 50 $fid 1 1000 sack 0
			incr fid
		}
		default {
			puts stderr "Unknown flow type"
			exit 1
		}
	}
}

Class Traffic/TFRC -superclass TestSuite
TestSuite instproc trafficTFRC {} {
    $self instvar node_
	global traf_para1_

	set noFlows [expr $traf_para1_/4]
	if {$traf_para1_ <=0 || [expr $noFlows*4] != $traf_para1_} {
		puts stderr "Invalid number of flows $traf_para1_ specified. Has to be a multiple of 4"
	} 

	set fid 1
    for {set i 0} {$i<2} {incr i} {
		for {set j 0} {$j < $noFlows} {incr j} {
			#new_TFRC startTime source dest fid pktSize
			$self new_TFRC [$self rnd 10] $node_(s$i) $node_(d$j) $fid 1000 
			incr fid
			
			$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
			incr fid
			
		}
    }
}

Class Traffic/TestFRp -superclass TestSuite
TestSuite instproc trafficTestFRp {} {

    $self instvar node_ topo_
    $topo_ instvar redpdq_

    set fid 1

    global target_rtt_ traf_para1_ traf_para2_    
    
    switch -exact $traf_para1_ { 
	"tcp" { 
	    $self new_Tcp [$self rnd 10] $node_(s0) $node_(d0) 50 $fid 1 1000 sack 0
	    incr fid
	} 
	"cbr" {
	    set p [$redpdq_ set P_testFRp_]
	    set frp [expr sqrt(1.5)/($target_rtt_*sqrt($p))]
	    set rate [expr {1.0/($traf_para2_*$frp)}]
	    
	    $self new_Cbr [$self rnd 1] $node_(s0) $node_(d0) 1000 $rate $fid 0   
	    incr fid
	}
	default {
	    puts stderr "Unknown Traffic Type"
	    exit 1
	}
    }

}

Class Traffic/Response -superclass TestSuite
TestSuite instproc trafficResponse {} {

    $self instvar node_ topo_
    $topo_ instvar redpdq_

    set fid 1

    global target_rtt_
    set p [$redpdq_ set P_testFRp_]
    set frp [expr sqrt(1.5)/($target_rtt_*sqrt($p))]
    set rate [expr {1.0/(0.25*$frp)}]


    #varying cbr 30->15->7.5->15->30. changes every 50s starting at 30s.
    $self new_VaryingCbr [$self rnd 10] $node_(s0) $node_(d0) 1000 $rate $fid 0 
    incr fid

    for {set i 0} {$i < [expr $traf_para1_ - 1]} {incr i} {
	$self new_VaryingCbr [$self rnd 10] $node_(s0) $node_(d0) 1000 [expr $rate*(2+2*0)] $fid 0 
	incr fid
    }
}

Class Traffic/PktsVsBytes -superclass TestSuite
TestSuite instproc trafficPktsVsBytes {} {

    $self instvar node_
    global traf_para1_

    set noFlows [expr $traf_para1_/4]
    if {$traf_para1_ <=0 || [expr $noFlows*4] != $traf_para1_} {
	puts stderr "Invalid number of flows $traf_para1_ specified. Has to be a multiple of 4"
    } 

    set fid 1
    for {set i 0} {$i < 2} {incr i} { 
	for {set j 0} {$j < $noFlows} {incr j} {
	    
	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
	    incr fid
	    
	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 500 sack 0
	    incr fid
	    
	}
    }
    
    
    
    #20% of the link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .002 $fid 0   
#    incr fid 
    
    #20% of the link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 500 .001 $fid 0   
#    incr fid 
           
}
