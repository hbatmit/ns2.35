Class ScenLib/RM

proc usage {prog} {
	switch $prog {
		"rmcc-1" {
			puts stderr "usage: ns rmcc-1.tcl -test 1 \
					<or 2 or 3 indicating the \
					testnum> \[other options\]
See rmcc-1.tcl for description of each test\
					conditions\n"
			exit 1
		}
		"rmcc-2" {
			puts stderr "usage: ns rmcc-2.tcl \
					\[other options\]"
			exit 1
		}
		"rmcc-3" {
			puts stderr "usage: ns rmcc-3.tcl \
					\[other options\]"
			exit 1
		}
		"rmcc-4" {
			puts stderr "usage: ns rmcc-4.tcl \
					\[other options\]"
			exit 1
		}
	}
}

proc default_options {prog} {
	# check here for other options that may be set for the diffrent
	# experiments
	global opts
	set opts(routingProto) CtrMcast
	set opts(srmSimType) SRM
	set opts(rmSrcType) CBR
	set opts(tcpSrcType) FTP
	set opts(pktSize) 1000
	
	switch $prog {
		rmcc-1 {
			set opts(test) -1
			set opts(tcpType) TCP
			set opts(sinkType) TCPSink
		}
		rmcc-2 {
			set opts(tcpType) TCP/Reno
			set opts(sinkType) TCPSink/DelAck
			set opts(bottleneckBW) 1Mbps
		    set opts(bottleneckDelay) 20ms
			set opts(bottleneckQSize) 60
		}
		rmcc-3 {
			set opts(tcpType) TCP
			set opts(sinkType) TCPSink
			set opts(bottleneckBW) 1Mbps
			set opts(bottleneckDelay) 20ms
			set opts(bottleneckQSize) 30
		}
		rmcc-4 {
			set opts(tcpType) TCP/Sack1
			set opts(sinkType) TCPSink/Sack1
			set opts(bottleneckDelay) 20ms
			set opts(clientNum) 10
			# other options 
		}
	}
}

proc process_args {argv } {
	global prog
	default_options $prog
	global opts
	for {set i 0} {$i < [llength $argv]} {incr i} {
		set key [lindex $argv $i]
		regsub {^-} $key {} key
		if {![info exists opts($key)]} {
			puts stderr "unknown option \"$key\""
		}
		incr i
		set opts($key) [lindex $argv $i]
	}
}

ScenLib/RM instproc set_traces {} {
	global prog opts ns srmStats srmEvents t
	
	$ns trace-all [open out-$t.tr w]
	$ns namtrace-all [open out-$t.nam w]
	
	#set srmStats [open srmStats-$t.tr w]
	#set srmEvents [open srmEvents-$t.tr w]
}

ScenLib/RM instproc init {} {
	$self instvar fid_
	set fid_ 0
	global prog opts ns n
	set ns [new Simulator]
	Simulator set EnableMcast_ 1
	Simulator set NumberInterfaces_ 1

	#Node expandaddr

	$ns color 0 white		;# data packets
	$ns color 40 blue		;# session
	$ns color 41 red		;# request
	$ns color 42 green	        ;# repair
	$ns color 1 azure		;# source node
	$ns color 2 lavender        ;# recvr node
	$ns color 3 navy            ;# traffic
	$ns color 4 SeaGreen
	$ns color 5 green
	$ns color 6 DarkGreen
	$ns color 7 coral
	$ns color 8 OrangeRed
	$ns color 9 maroon
	$ns color 10 DarkBlue
	
	$self set_traces
	return $self
}

proc set_flowMonitor {flowmon output_chan} {

	$flowmon attach $output_chan
	set bytesInt_ [new Integrator]
	set pktsInt_ [new Integrator]
	$flowmon set-bytes-integrator $bytesInt_
	$flowmon set-pkts-integrator  $pktsInt_
	return $flowmon
}

ScenLib/RM instproc make_flowmon { time args } {
	global ns 
    $self instvar flowmon_
	#puts "time = $time"
	#puts "args = $args"
	if {[expr [llength $args] % 3] != 0} {
		puts stderr {Error: Incomplete arguments for make_flowmon..}
	}
	#set c 0
	for {set i 0} {$i < [llength $args]} {incr i} {
		set node1 [lindex $args $i]
		incr i
		set node2 [lindex $args $i]
		incr i
		set fout [lindex $args $i]
	    set a [$node1 id]
	    set b [$node2 id]
		set flow_stat($a:$b) [open $fout w]    
		set flowmon_($a:$b) [$ns makeflowmon Fid]
		$ns attach-fmon [$ns link $node1 $node2] $flowmon_($a:$b) 0
		set_flowMonitor $flowmon_($a:$b) $flow_stat($a:$b)
	    #$ns at $time  "$flowmon_($a:$b) dump"
	    #incr c
	}
}

ScenLib/RM instproc dump_flowmon {n1 n2 time} {
    $self instvar flowmon_
    global ns n
    $ns at $time "$flowmon_([$n1 id]:[$n2 id]) dump"
}



ScenLib/RM instproc make_nodes num {
	global ns n
	# setup the nodes
	for {set i 0} {$i < $num} {incr i} {
		set n($i) [$ns node]
		#puts "node($i) -> $n($i)"
	}
}

ScenLib/RM instproc create_mcast {srcnode switch addgrp time args} {
	global ns n opts srmStats srmEvents mflag
	$self instvar mrthandle_ fid_
	if {$mflag == 0} {
		set mrthandle_ [$ns mrtproto $opts(routingProto) {}]
		#puts "setting the mcastproto- $opts(routingProto)"
		set mflag 1
	}
	set dest [Node allocaddr]
	puts "mcast grp = $dest"
	$ns at $switch "$mrthandle_ switch-treetype $dest"
	#puts "ns at $switch $mrthandle_ switch-treetype $dest"

	set src [new Agent/$opts(srmSimType)]
	$src set dst_ $dest
	$src set fid_ [incr fid_]
	#puts "srcnode $srcnode fid -> [$src set fid_]"
	#$src trace $srmEvents
	#$src log $srmStats
	#$ns at 1.0 "$src start"
	$ns at $addgrp "$src start"
	# puts "ns at $addgrp src start"
	$ns attach-agent $n($srcnode) $src

	for {set i 0} {$i < [llength $args]} {incr i} {
		set j [lindex $args $i]
		set rcvr($j) [new Agent/$opts(srmSimType)]
		$rcvr($j) set dst_ $dest
		$rcvr($j) set fid_ [incr fid_]
		#puts "Srm-recr $j fid -> [$rcvr($j) set fid_]"
		#$rcvr($j) trace $srmEvents
		#$rcvr($j) log $srmStats
		$ns attach-agent $n($j) $rcvr($j)
		#$ns at 1.0 "$rcvr($j) start"
		$ns at $addgrp "$rcvr($j) start"
		# puts "ns at$addgrp rcvr($j) start"
	}
	#setup source
	set cbr [new Agent/$opts(rmSrcType)]
	$cbr set packetSize_ $opts(pktSize)
	$src traffic-source $cbr
	$src set packetSize_ $opts(pktSize)    ;#so repairs are correct
	$cbr set fid_ [incr fid_]
	#puts "cbr fid = [$cbr set fid_]"
	$ns at $time "$src start-source"
}

ScenLib/RM instproc create_cbr { args } {
	if {[expr [llength $args] % 5] != 0} {
		puts stderr {uneven number of tcp endpoints}
	}
	global ns n opts
	$self instvar fid_
	# Attach a CBR source and connect between a sender and recr 
	# node at the given time
	for {set i 0} {$i < [llength $args]} {incr i} {
		set k [lindex $args $i]
		set cbr($k) [new Agent/CBR]
		
		$cbr($k) set fid_ [incr fid_]
		$cbr($k) set packetSize_ $opts(pktSize)
		incr i
		$cbr($k) set interval_ [lindex $args $i]
		$ns attach-agent $n($k) $cbr($k)
		incr i
		set null($k) [new Agent/Null]
		$ns attach-agent $n([lindex $args $i]) $null($k)
		$ns connect $cbr($k) $null($k)
		incr i
		$ns at [lindex $args $i] "$cbr($k) start"
		incr i
		$ns at [lindex $args $i] "$cbr($k) stop"
	}
}


proc loss-model10 {} {
	global ns
	#setup loss model
	#set loss_module [new ErrorModel]
	#$loss_module set rate_ 0.1
	set loss_module [new SRMErrorModel]
	$loss_module set rate_ 0.1
	#$loss_module drop-packet 2 200 1
	$loss_module drop-target [$ns set nullAgent_]
	return $loss_module
}

proc loss-model5 {} {
	global ns
	set loss_module [new SRMErrorModel]
	$loss_module set rate_ 0.05
	#$loss_module drop-packet 2 200 1
	$loss_module drop-target [$ns set nullAgent_]
	return $loss_module
}

ScenLib/RM instproc loss-model-case1 {time a b} {
	global ns n
	set loss_module [loss-model10]
	$ns at $time "$ns lossmodel $loss_module $n($a) $n($b)"
}

ScenLib/RM instproc loss-model-case2 {time1 time2 a b c d} {
	global ns n
	set loss_module1 [loss-model10]
	set loss_module2 [loss-model10]
	$ns at $time1 "$ns lossmodel $loss_module1 $n($a) $n($b)"
	$ns at $time2 "$ns lossmodel $loss_module2 $n($c) $n($d)"
}

ScenLib/RM instproc loss-model-case3 {time1 time2 a b c d} {
	global ns n
	set loss_module1 [loss-model5]
	set loss_module2 [loss-model10]
	$ns at $time1 "$ns lossmodel $loss_module1 $n($a) $n($b)"
	$ns at $time2 "$ns lossmodel $loss_module2 $n($c) $n($d)"
}


ScenLib/RM instproc create_tcp {args} {
	if {[expr [llength $args] % 3] != 0} {
		puts stderr {uneven number of tcp endpoints}
	}
	global ns n opts
	$self instvar fid_
	#Attach a TCP flow between each src and recvr pair
	for {set i 0} {$i < [llength $args]} {incr i} {
		set k [lindex $args $i]
		set tcp($k) [new Agent/$opts(tcpType)]

		$tcp($k) set fid_ [incr fid_]
		$tcp($k) set packetSize_ $opts(pktSize)
		$ns attach-agent $n($k) $tcp($k)
		#puts "ns attach-agent n($k) tcp (fid [$tcp($k) set fid_]"
		
		incr i
		set tcp_sink($k) [new Agent/$opts(sinkType)]
		$ns attach-agent $n([lindex $args $i]) $tcp_sink($k)
		#puts "dest=[lindex $args $i]"
		$ns connect $tcp($k) $tcp_sink($k)
		incr i
		set ftp($k) [new Application/$opts(tcpSrcType)]
		$ftp($k) attach-agent $tcp($k)
		set time [lindex $args $i]
		$ns at $time "$ftp($k) start"
		#puts "At $time, tcp (from node$k) starts.. "
	}
}

proc finish {} {
	global prog ns opts srmEvents srmStats t
	#$src stop
	puts "Running finish.."
	$ns flush-trace
	#close $srmStats
	#close $srmEvents
	#XXX
	#puts "Filtering ..."
	#exec tclsh8.0 ../../../../nam-1/bin/namfilter.tcl out-$t.nam
	puts "running nam..."
	exec nam out-$t.nam &
	exit 0
}









