source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for TCP

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Agent/TCP set rfc2988_ false
Agent/TCP set windowInit_ 1
Agent/TCP set singledup_ 0
Agent/TCP set minrto_ 0

Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr 90 * 512 + 40]

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/net8 -superclass Topology
Topology/net8 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
    set node_(k2) [$ns node]
    set node_(k3) [$ns node]
    set node_(k4) [$ns node]
    set node_(k5) [$ns node]
    set node_(k6) [$ns node]
    set node_(r2) [$ns node]
    set node_(d1) [$ns node]

    $self next
    $ns duplex-link $node_(s1)   $node_(r1)  10Mb        2ms    DropTail 
    $ns duplex-link $node_(r1)   $node_(k1)  256Kb       10ms    DropTail
    $ns duplex-link $node_(k1)   $node_(k2)  256Kb       10ms    DropTail
    $ns duplex-link $node_(k2)   $node_(k3)  256Kb       10ms    DropTail
    $ns duplex-link $node_(k3)   $node_(k4)  256Kb       10ms    DropTail
    $ns duplex-link $node_(k4)   $node_(k5)  256Kb       10ms    DropTail
    $ns duplex-link $node_(k5)   $node_(k6)  256Kb       10ms    DropTail
    $ns duplex-link $node_(k6)   $node_(r2)  256Kb       10ms    DropTail
    $ns duplex-link $node_(r2)   $node_(d1)  10Mb        2ms    DropTail

    $ns duplex-link-op $node_(r1) $node_(k1) queuePos 0.5
    $ns queue-limit $node_(r1) $node_(k1) 30
    set qmon [$ns monitor-queue $node_(r1) $node_(k1) 1 2]
}


Class Topology/net7 -superclass Topology
Topology/net7 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
    
    $self next
    $ns duplex-link $node_(s1) $node_(r1) 1Mb 10s DropTail
    $ns duplex-link $node_(r1) $node_(k1) 0.9Mb 100ms DropTail
    
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(r1) $node_(k1) orient right
    $ns duplex-link-op $node_(r1) $node_(k1) queuePos 0.5
    $ns queue-limit $node_(r1) $node_(k1) 4000
    set qmon [$ns monitor-queue $node_(r1) $node_(k1)  1 2]
}

Class Topology/net6 -superclass Topology
Topology/net6 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
    
    $self next
    $ns duplex-link $node_(s1) $node_(r1) 1Mb 100ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 10Mb 10ms DropTail
    
    $ns duplex-link-op $node_(s1) $node_(r1) orient left-right
    $ns duplex-link-op $node_(r1) $node_(k1) orient right
    $ns duplex-link-op $node_(r1) $node_(k1) queuePos 0.5
    $ns queue-limit $node_(r1) $node_(k1) 4000
    set qmon [$ns monitor-queue $node_(r1) $node_(k1)  1 2]

    lappend drops 60

    set loss_module [new ErrorModel/List]
    $loss_module droplist $drops
    $loss_module drop-target [new Agent/Null]
    $ns lossmodel $loss_module $node_(r1) $node_(k1)
}

Class Topology/net5 -superclass Topology
Topology/net5 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
    set node_(s3) [$ns node]   
    
    $self next
    $ns duplex-link $node_(s1) $node_(r1) 1Mb 100ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 1Mb 100ms DropTail
    $ns duplex-link $node_(s3) $node_(r1) 1Mb 100ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 0.5Mb 100ms DropTail
    
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right
    $ns duplex-link-op $node_(s3) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(k1) orient right
    $ns duplex-link-op $node_(r1) $node_(k1) queuePos 0.5
    $ns queue-limit $node_(r1) $node_(k1) 200
    set qmon [$ns monitor-queue $node_(r1) $node_(k1)  1 2]
}


TestSuite instproc finish testname {
    global quiet wrap PERL 
    $self instvar trace_
    
    if {$testname == "rtt-rfc793" || $testname == "rtt-jacobson"} {
	close $trace_(rto)
        close $trace_(rtt)
	exec cp rtt.tr temp.rands
	if {$quiet == "false"} {	
	    exec xgraph -x time -y "rtt,rto values"  rtt.tr rto.tr &
	}
    } 

    if {$testname == "rto-karn" || $testname == "rto-nokarn"} {
	close $trace_(srtt)
	exec cp srtt.tr temp.rands
	if {$quiet == "false"} {
	    exec xgraph  -x time -y "Estimated RTT"  srtt.tr &
	}
    }

    if {$testname == "seqno-fastrtx" || $testname == "seqno-nofastrtx" || $testname == "rto-karn" || $testname == "rto-nokarn"} {
        exec $PERL ../../bin/getrc -s 1 -d 2 all.tr | \
		$PERL ../../bin/raw2xg -a -e -s 0.01 -m 10000000 -t $testname > temp.rands
        if {$quiet == "false"} {
	    exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
    } 

    if {$testname == "jacobson88-noss" || $testname ==
    "jacobson88-ss"} {
	exec awk {
                {
                        if (($1 == "+") && ($5 == "tcp") &&\
                            ($3 == "0") && ($4 == "1"))\
                                        print $2, $11
                }
        } all.tr > out.seq
	exec cp out.seq temp.rands
	if {$quiet == "false"} {
        	exec xgraph -P out.seq & 
	}
    }
    
    if {$quiet == "false"} {
    	exec nam all.nam &
    }
    exit 0
}  


TestSuite instproc printtimers { tcp time} {
    global quiet
    if {$quiet == "false"} {
	puts "time: $time sRTT(in ticks): [$tcp set srtt_]/8 RTTvar(in ticks): [$tcp set rttvar_]/4 backoff: [$tcp set backoff_]"
    }
}

TestSuite instproc printtimersAll { tcp time interval } {
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcp)] {
	set dump_inst_($tcp) 1
	$ns_ at $time "$self printtimersAll $tcp $time $interval"
	return
    }
    set newTime [expr [$ns_ now] + $interval]
    $ns_ at $time "$self printtimers $tcp $time"
    $ns_ at $newTime "$self printtimersAll $tcp $newTime $interval"
}


## 
TestSuite instproc setup {tcptype list} {
    global wrap wrap1 quiet
    $self instvar ns_ node_ testName_ guide_
    $self setTopo
    puts "Guide: $guide_"
    
    $ns_ color 1 Red
    $ns_ color 2 Green
    $ns_ color 3 Blue
    $ns_ color 4 Yellow

    set fid 1
    # Set up TCP connection
    if {$tcptype == "rtt-jacobson" || $tcptype == "rtt-rfc793"} {
	
	$self instvar trace_
	set trace_(rtt) [open "rtt.tr" w]
	set trace_(rto) [open "rto.tr" w]

	
	if {$quiet == "false"} {
	puts ""
	puts "                    RTT Test."
	puts "------------------------------------------------------------------------"
	puts "    s1 1Mb"
	puts "      \\             - s1 implements RFC793 estimation or Jacobson's"
	puts "  1MB  \\  0.5Mb     - s1 tx to k; at 1.5 both s2 and s3 tx big pkts for 1sec;"
	puts "    s2--r-------k    - nobody performs slowstart."
	puts "       /             - The queue in r grows fast and so does the RTT seen by s1."
	puts "      /1Mb           RFC793 RTO estimation can't adapt to the variance peak."
	puts "    s3               When the situation becomes normal again, RFC793 estimates"
	puts "                     RTO too pessimistically."
	}
	
	set tcp1 [$ns_ create-connection TCP/RFC793edu $node_(s1) \
		TCPSink $node_(k1) 1]
	set tcp2 [$ns_ create-connection TCP/RFC793edu $node_(s2) \
		TCPSink $node_(k1) 2]
	set tcp3 [$ns_ create-connection TCP/RFC793edu $node_(s3) \
			TCPSink $node_(k1) 3]
	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]
	set ftp3 [$tcp3 attach-app FTP]
	$tcp1 set window_ 50
	$tcp2 set packetSize_ 2000
	$tcp3 set packetSize_ 2000
	
	if {$tcptype == "rtt-jacobson"} { 
	    $tcp1 set add793jacobsonrtt_ true
	}
	$ns_ at 0.0  "$self plotrto $tcp1 0.25"
	$ns_ at 0.5  "$ftp1 start"
	$ns_ at 1.5  "$ftp2 start"
	$ns_ at 1.5  "$ftp3 start"
	$ns_ at 2.5 "$ftp2 stop"
	$ns_ at 2.5 "$ftp3 stop"
	##$self traceQueues $node_(r1) [$self openTrace 20.0 $testName_]
	$ns_ at 20.0 "$self cleanupAll $testName_"
    }	
################################## seqno-{fastrtx, nofastrtx}
    if {$tcptype == "seqno-fastrtx" || $tcptype == "seqno-nofastrtx" } {
	
	if {$quiet == "false"} {
	puts ""
	puts "                      Fast Retransmit"
	puts "------------------------------------------------------------------------"
	puts "         10Mb        - r1: Tahoe with/without fastrtx"
	puts " s1----r1----k1      - r1 tx to k1"
	puts "         10ms        - pkt 60 is dropped"
	puts ""
	puts "Without fast rtx., the source runs out of window and has to wait for a"
	puts "timeout to force the retransmission of the lost packet and the associated"
	puts "acknowledgement to open the window again."
	}

	$self instvar trace_
	set trace_(seqn) [open "seqn.tr" w]

	set tcp1 [$ns_ create-connection TCP/RFC793edu $node_(r1) \
		TCPSink $node_(k1) 1]
	set ftp1 [$tcp1 attach-app FTP]
	#$tcp1 set window_ 40
	if {$tcptype == "seqno-fastrtx"} {$tcp1 set add793fastrtx_ true}
	$tcp1 set add793expbackoff_ true
	$tcp1 set add793karnrtt_ true
	$tcp1 set add793jacobsonrtt_ true
	$tcp1 set add793slowstart_ true
	
	$ns_ at 0.5 "$ftp1 produce 100000"	
	#$self traceQueues $node_(r1) [$self openTrace 1.25 $testName_]
	$ns_ at 1.25 "$self cleanupAll $testName_"
    }

################################## rto-{karn, nokarn}
	if {$tcptype == "rto-karn" || $tcptype == "rto-nokarn" } {
	$self instvar trace_
	set trace_(srtt) [open "srtt.tr" w]
	    
	if {$quiet == "false"} {
	puts ""
	puts "Karn Algorithm --- (Karn's RTT sampling + RTO Exponential Binary Backoff)" 
	puts "------------------------------------------------------------------------"	
	puts "   1Mb   0.9Mb        - s1 is a Tahoe source, (nokarn: without Karn's A.)"
	puts " s1----r1----k1       - s1 tx; ack does not arrive, so s1 rtx;"
	puts "    ^^                  if using karn, it will space exponentially"
	puts "  delay 10s!!           the retx"
	puts "                      - a packet will be rtx around t=18s; ack for the"
	puts "                        first pkt sent arrives at t=20s; if Karn's"
	puts "                        is not used, then we the RTT estimation is 2s!!"
	}
	set tcp1 [$ns_ create-connection TCP/RFC793edu $node_(s1) \
		TCPSink $node_(k1) 1]
	set ftp1 [$tcp1 attach-app FTP]
	$tcp1 set window_ 28
	if {$tcptype == "rto-karn"} {
		$tcp1 set add793karnrtt_ true
		$tcp1 set add793expbackoff_ true
	} else {
		$tcp1 set add793karnrtt_ false 
                $tcp1 set add793expbackoff_ false
	}
	$tcp1 set add793fastrtx_ true
	$tcp1 set add793jacobsonrtt_ false
	$tcp1 set add793slowstart_ true
	$ns_ at 0.0  "$self plotsrtt $tcp1 0.25"
	$ns_ at 0.5 "$ftp1 produce 100000"

	#$self traceQueues $node_(r1) [$self openTrace 50.0 $testName_]
	$ns_ at 50.0 "$self cleanupAll $testName_"
	}

##################################  jacobson88-noss
	if {$tcptype == "jacobson88-noss" || $tcptype == "jacobson88-ss" } {
	
	if {$quiet == "false"} {
	puts ""
	puts "Congestion avoidance and control" 
	puts "------------------------------------------------------------------------"	
	puts ""
	puts " s1---r1---n1---n2---n3---n4---n5---n6---n7---n8---r2---d1"
	puts "    ^    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^    ^"
	puts " (10Mb,2ms)      (256Kb,10ms) all these links      (10Mb,2ms)"
	puts ""
	puts "Experiment based on Jacobson's SIGCOMM'88 paper:"
	puts "- s1 uses a 32 pkt's tx. window and Karn's algorithm"
	puts "- s1 performs slow-start (ss) or not (noss)"	
	puts "- r1 has capacity for 30 packets only"
	puts "- the 8 hops have capacity for a complete window, but r1 not" 
	puts ""
	}

	set tcp1 [$ns_ create-connection TCP/RFC793edu $node_(s1) \
		TCPSink $node_(d1) 1]
	set ftp1 [$tcp1 attach-app FTP]
	$tcp1 set window_ 32
	$tcp1 set packetSize_ 512 
	$tcp1 set add793karnrtt_ true
	$tcp1 set add793expbackoff_ true
	if {$tcptype == "jacobson88-ss"} {
		$tcp1 set add793slowstart_ true
	}
	$ns_ at 0.0 "$ftp1 start"
	#$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
	}

    
    #$self tcpDump $tcp1 1.0
    
    
    $ns_ run
}

TestSuite instproc plotseqn { tcp interval} {
    $self instvar trace_ ns_  
    set now [$ns_ now]
    puts $trace_(seqn) "$now [$tcp set seqno_]"
    $ns_ at [expr $now+$interval] "$self plotseqn $tcp $interval"
}


TestSuite instproc plotrto { tcp interval} {
    $self instvar trace_ ns_  
    set now [$ns_ now]
    puts $trace_(rto) "$now [expr [$tcp set tcpTick_] *[$tcp set rto_]]"
    puts $trace_(rtt) "$now [expr [$tcp set tcpTick_] *[$tcp set rtt_]]"
    $ns_ at [expr $now+$interval] "$self plotrto $tcp $interval"
}

TestSuite instproc plotsrtt { tcp interval} {
    $self instvar trace_ ns_  
    set now [$ns_ now]
    puts $trace_(srtt) "$now [expr [$tcp set tcpTick_] * ( [$tcp set srtt_] >> [$tcp set T_SRTT_BITS])]"
    $ns_ at [expr $now+$interval] "$self plotsrtt $tcp $interval"
}


# Definition of test-suite tests

########## Jacobson/RFC793 RTT
Class Test/rtt-jacobson -superclass TestSuite
Test/rtt-jacobson instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net5
    set test_   rtt-jacobson
    set guide_  "Van Jacobson RTO estimation."
    $self next
}

Test/rtt-jacobson instproc run {} {
    $self setup rtt-jacobson {}
}

Class Test/rtt-rfc793 -superclass TestSuite
Test/rtt-rfc793 instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net5
    set test_       rtt-rfc793
    set guide_  "RFC 793 RTO estimation."
    $self next
}

Test/rtt-rfc793 instproc run {} {
    $self setup rtt-rfc793 {}
}
 
########## Arrival rate with/without fast rtx
Class Test/seqno-nofastrtx -superclass TestSuite   
Test/seqno-nofastrtx instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net6
    set test_   seqno-nofastrtx
    set guide_  "Without Fast Retransmit."
    $self next
}

Test/seqno-nofastrtx instproc run {} {
    $self setup seqno-nofastrtx {}
}

Class Test/seqno-fastrtx -superclass TestSuite
Test/seqno-fastrtx instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net6
    set test_   seqno-fastrtx
    set guide_  "With Fast Retransmit."
    $self next
}

Test/seqno-fastrtx instproc run {} {
    $self setup seqno-fastrtx {}
}

#### Karn Algorithm (RTT sampling + exp. backoff)
Class Test/rto-karn -superclass TestSuite
Test/rto-karn instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net7
    set test_   rto-karn
    set guide_  "With Karn's RTT Sampling and Exponential Backoff."
    $self next
}

Test/rto-karn instproc run {} {
    $self setup rto-karn {}
}

Class Test/rto-nokarn -superclass TestSuite
Test/rto-nokarn instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net7
    set test_   rto-nokarn
    set guide_  "Without Karn's RTT Sampling and Exponential Backoff."
    Agent/TCP set bugfix_ss_ 0
    $self next
}

Test/rto-nokarn instproc run {} {
    $self setup rto-nokarn {}
}



########## Jacobson's SIGCOMM' 88 
Class Test/jacobson88-noss -superclass TestSuite
Test/jacobson88-noss instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net8
    set test_   jacobson88-noss
    set guide_  "Without Slow-Start."
    $self next
}

Test/jacobson88-noss instproc run {} {
    $self setup jacobson88-noss {}
}

Class Test/jacobson88-ss -superclass TestSuite
Test/jacobson88-ss instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net8
    set test_   jacobson88-ss
    set guide_  "With Slow-Start."
    $self next
}

Test/jacobson88-ss instproc run {} {
    $self setup jacobson88-ss {}
}


###
TestSuite runTest











