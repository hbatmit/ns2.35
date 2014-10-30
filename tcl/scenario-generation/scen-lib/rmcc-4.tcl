# Multiple RM flows scenario competing with multiple TCP scenario
#                  
#             RM sender 1   
# topology:  .   o               o   RM recv 1
#            .    \ 0         1 /              
#    RM send N o---o-----------o--o   RM recv N
#                 /| B/N link  |\
#                / |           | \   
#               o  o           o  o  TCP recv 1
#    TCP sender 1                 
#               ...            TCP receiver N
#                TCP sender N 
#
# start N number of rm flows and TCP flows randomly
# after steady state is attained measure thruput ans compare
#
# usage: ns rmcc-4.tcl

source rmcc.tcl

proc get_rand_time {first last number} {
	set times ""
	set interval [expr $last - $first]
	set maxrval [expr pow(2,31)]
	set intrval [expr $interval/$maxrval]

	for { set i 0 } { $i < $number } { incr i } {
		set randtime [expr ([ns-random] * $intrval) + $first]
		# XXX include only 6 decimals (i.e. usec)
		lappend times [format "%.1f" $randtime]
	}
	return $times
}


ScenLib/RM instproc random_start_rm {start stop} {
	global ns n num
	#create mcast trees
	set i [expr $num*2]
	set t [expr $num * 3]
	while {$i < $t} {
		lappend R $i
		incr i
	}
	for {set i 0} {$i < $num} {incr i} {
		set time [get_rand_time $start $stop 1]
		set st [expr $time - 1]
		if {$st < 0} { set st 0}
		set sw [expr $st - 0.7]
		if {$sw < 0} { set sw 0}
		#puts "time = $time"
		eval $self create_mcast $i $sw $st $time $R
		#puts "mcast $i to $R"
	}
}

ScenLib/RM instproc random_start_tcp {start stop} {
	global ns n num
    set start [expr $num*2]
    set end [expr $num*4]
	set i $num
	set j [expr $num*3]
	while {$i < $start || $j < $end} {
		set time [get_rand_time $start $stop 1]
		$self create_tcp $i $j $time
	    #puts "$self create_tcp $i $j $time"
		incr i
		incr j
	}
}


ScenLib/RM instproc make_topo3 {} {
	global ns tcp_model num n opts
	#make the bottleneck link
	set n(l) [$ns node]
	set n(r) [$ns node]
	set num $opts(clientNum)      ;#N actual number flows setup
	$ns duplex-link $n(l) $n(r) [expr $num * 0.5]Mbps \
			$opts(bottleneckDelay) DropTail
	$ns queue-limit $n(l) $n(r) [expr $num * 10]
	
	#make N number of TCP connections
	#set seed 12345
	#set params "-print-drop-rate 1 -debug 0 -trace-filename out"
	#set p1 "-bottle-queue-length $opts(bottleneckQSize) \
			#-bottle-queue-method DropTail"
	#set p2 "-client-arrival-rate 120 \
		#	-bottle-bw $opts(bottleneckBW) \
			#-bottle-delay $opts(bottleneckDelay) \
			#-ns-random-seed $seed" 
	#set p2a "-source-tcp-method TCP/Reno -sink-ack-method \
		#	TCPSink/DelAck"
	#set p3 "-client-mouse-chance 90 -client-mouse-packets 10" 
	#set p4 "-client-bw 100Mb -node-number $num -client-reverse-chance 10"
	#set p5 "-initial-client-count 0"
	#set p6 "-duration 10 graph-results 1 graph-join-queueing 0 -graph-scale 2"
	#set tcp_model [eval new TrafficGen/ManyTCP $params $p1 $p2 \
		#	$p2a $p3 $p4 $p5 $p6]
	

	#now create client nodes on left of bottleneck
	for {set i 0} {$i < [expr $num*2]} {incr i} {
		set n($i) [$ns node]
		$ns duplex-link $n($i) $n(l) 10Mb 5ms DropTail
	}

	# do same for right of B/N link
	for {set i [expr $num*2]} {$i < [expr $num * 4]} {incr i} {
		set n($i) [$ns node] 
		$ns duplex-link $n(r) $n($i) [expr $num * 5]Mb \
				5ms DropTail
	}
	
}

ScenLib/RM instproc add_flowmon { time } {
	global num n t
	#make sure the flow monitors are dumped at the right time
	$self make_flowmon $time $n(r) $n([expr $num*2]) flowStats_1_$num.$t \
			$n(r) $n([expr $num * 3]) \
			flowStats_1_[expr $num + 1].$t \
			
}
	


	
proc run {} {
	global ns n num
	set test_scen [new ScenLib/RM]
	$test_scen make_topo3
    #random start tcp and rm connections between 0 and 10s.
	$test_scen random_start_tcp 0.0 10.0
	$test_scen random_start_rm 0.0 10.0
	$test_scen add_flowmon 30.0
    $test_scen dump_flowmon $n(r) $n([expr $num*2]) 60.0 ;# rm flow
    $test_scen dump_flowmon $n(r) $n([expr $num*3]) 60.0 ;#tcp flow
    $ns at 60.0 "finish"
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


