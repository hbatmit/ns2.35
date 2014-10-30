# Correlated and Uncorrelated Loss Scenario:

# Topology:

#           0 0 Source
#             |
#           1 0 
#            / \
#           /   \
#       2  0   3 0 
#         / \     \
#      4 0 5 0   6 0
#       R1   R2    R3
#

# 
# Simulation A : All links have 1Mbps BW. There is 10% loss in link 0-1
# No loss in other links.

# Simulation B: Similar to setup of A except that 10% loss now takes 
# place in links 1-2 and 1-3. No loss in other links. 

# Simulation C: 10% loss in link 1-3. and 5% loss in link 1-2. No 
# loss in other links. 
# Flow monitors measure thruput at link 2-4, 2-5 and 3-6.

#usage: ns rmcc-1.tcl -test <#test no>
#test number options: 1,2, 3 or 4


source ../../mcast/srm-nam.tcl		;#to separate control messages
source rmcc.tcl


ScenLib/RM instproc make_topo1 { at } {
    global ns n prog t
    $self make_nodes 7
    #color them
    $n(0) color "red"
    $n(4) color "orchid"
    $n(5) color "orchid"
    $n(6) color "orchid"
    
    #now setup topology
    $ns duplex-link $n(0) $n(1) 1Mb 10ms DropTail
    $ns duplex-link $n(1) $n(2) 1Mb 10ms DropTail
    $ns duplex-link $n(1) $n(3) 1Mb 10ms DropTail
    $ns duplex-link $n(2) $n(4) 1Mb 10ms DropTail
    $ns duplex-link $n(2) $n(5) 1Mb 10ms DropTail
    $ns duplex-link $n(3) $n(6) 1Mb 10ms DropTail
    
    $ns duplex-link-op $n(0) $n(1) orient down
    $ns duplex-link-op $n(1) $n(2) orient left-down
    $ns duplex-link-op $n(1) $n(3) orient right-down
    $ns duplex-link-op $n(2) $n(4) orient left-down
    $ns duplex-link-op $n(2) $n(5) orient right-down
    $ns duplex-link-op $n(3) $n(6) orient right-down
    
    $self make_flowmon $at $n(2) $n(4) flowStats_2_4.$t \
	    $n(2) $n(5) flowStats_2_5.$t \
	    $n(3) $n(6) flowStats_3_6.$t
    # $self make_flowmon $n(2) $n(5) flowStats_2_5.$t
    # $self make_flowmon $n(3) $n(6) flowStats_3_6.$t
}



#ScenLib/RM instproc setup_mcast {} {
 #   global ns n opts srmStats srmEvents

    # mproto options : 5
    # CtrMcast, DM, detailedDM, dynamicDM, pimDM
  #  set mrthandle [$ns mrtproto $opts(routingProto) {}]
    
   # set fid 0
   # set dest [Node allocaddr]
   # $ns at 0.3 "$mrthandle switch-treetype $dest"
    
   # set src [new Agent/$opts(srmSimType)]
   # $src set dst_ $dest
   # $src set fid_ [incr fid]
   # $src trace $srmEvents
   # $src log $srmStats
   # $ns at 1.0 "$src start"
   # $ns attach-agent $n(0) $src
    
   # for {set i 4} {$i < 7} {incr i} {
#	set rcvr($i) [new Agent/$opts(srmSimType)]
#	$rcvr($i) set dst_ $dest
#	$rcvr($i) set fid_ [incr fid]
#	$rcvr($i) trace $srmEvents
#	$rcvr($i) log $srmStats
#	$ns attach-agent $n($i) $rcvr($i)
#	$ns at 1.0 "$rcvr($i) start"
#    }
    
    #setup source
 #   set cbr [new Agent/CBR]
 #   $cbr set packetSize_ 1000
 #   $src traffic-source $cbr
 #   $src set packetSize_ 1000  ;#so repairs are correct
 #   $cbr set fid_ 0
 #   $ns at 3.0 "$src start-source"
#}



#ScenLib/RM instproc attach-tcp {} {
 #   global ns n
    #Attach a TCP flow between src and recvr1
 #   set tcp [new Agent/TCP]
 #   $tcp set fid_ 2
 #   $tcp set packetSize_ 1000
 #   $ns attach-agent $n(0) $tcp
    
 #   set sink [new Agent/TCPSink]
 #   $ns attach-agent $n(4) $sink
 #   $ns connect $tcp $sink
    
 #   set ftp [new Application/FTP]
 #   $ftp attach-agent $tcp
 #   $ns at 1.5 "$ftp start"
    
#}



proc test {num } {
    global ns n prog
    puts "running test$num"
    set test_scen [new ScenLib/RM]
    switch $num {
	"1" { 
	    $test_scen make_topo1 6.0
	    $test_scen create_mcast 0 0.3 1.0 3.0 4 5 6 
	    $test_scen loss-model-case1 2.52 0 1
	    #$ns at 6.0 "$flowmon dump"
	    $ns at 6.0 "finish" 
	}
	"2" {
	    $test_scen make_topo1 12.0
	    $test_scen create_mcast 0 0.3 1.0 3.0 4 5 6
	    $test_scen loss-model-case2 2.52 2.52 1 2 1 3
	    #$ns at 12.0 "$flowmon dump"
	    $ns at 12.0 "finish"
	}
	"3" {
	    $test_scen make_topo1 18.0
	    $test_scen create_mcast 0 0.3 1.0 3.0 4 5 6
	    $test_scen loss-model-case3 2.52 2.52 1 2 1 3
	    #$ns at 18.0 "$flowmon dump"
	    $ns at 18.0 "finish"
	}
	default {
	    puts "Unknown test number $num"
	    usage $prog
	}
    }
    $test_scen create_tcp 0 4 1.5
    $ns run
}

global argv prog mflag
set mflag 0
if [string match {*.tcl} $argv0] {
    set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
    set prog $argv0
}

if {[llength $argv] < 1} {
    puts "Insufficient number of arguments"
    usage $prog
}

global opts t
process_args $argv
set t $prog.$opts(test)
test $opts(test)




