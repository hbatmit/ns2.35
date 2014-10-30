# Scripts for bandwidth-asymmetry simulations.  Uses util.tcl (must be in
# this directory).

source util.tcl

# set up simulation

set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f

#$ns use-scheduler List

set tcptrace [open tcp.tr w] 
set sinktrace [open sink.tr w]
set redtrace [open red.tr w]
set qtrace [open q.tr w]
set graph 0
set connGraphFlag("") 0
set maxdelack 25
set maxburst 0
set upwin 0
set downwin 100
set tcpTick 0.1
set ackSize 6
set rbw 28.8Kb
set rqsize 10
set qsize 10
set rgw "DropTail"
set nonfifo 1
set acksfirst false
set filteracks false
set reconsacks false
set replace_head false
set q_wt 0.1
set rand_drop false
set prio_drop false
set randomize 1

set startTime 0
set midtime 0
set stop 50

Agent/AckReconsClass set deltaAckThresh_ 2
Agent/AckReconsClass set ackInterArr_ 0.010
Agent/AckReconsClass set ackSpacing_ 0.010
Agent/AckReconsClass set delack_ 2
Agent/AckReconsClass set adaptive_ 1
Agent/AckReconsClass set alpha_ 0.25
Agent/AckReconsClass set lastTime_ 0
Agent/AckReconsClass set lastAck_ 0
Agent/AckReconsClass set lastRealTime_ 0
Agent/AckReconsClass set lastRealAck_ 0

Queue set limit_ $qsize 

proc printUsage {} {
	puts "Usage: "
	exit
}

proc finish {ns traceall tcptrace graph connGraphFlag midtime} {
 #	upvar $connGraphFlag graphFlag 
	 
	$ns flush-trace
	close $traceall
	flush $tcptrace
	close $tcptrace
#	exec perl ~/src/ns-2/bin/trsplit -f -pt tcp out.tr
#	exec perl ~/src/ns-2/bin/trsplit -f -pt ack out.tr
	exit 0
}

# read user options and act accordingly
set count 0
while {$count < $argc} {
	set arg [lindex $argv $count]
	incr count 2
	switch -exact '$arg' {
		'-graph' {
			set graph 1
			incr count -1
			continue
		}
		'-r' {
			set randomize 1
			incr count -1
			continue
		}
		'-mda' {
			set maxdelack [lindex $argv [expr $count-1]]
			continue
		}
		'-acksz' {
			set ackSize [lindex $argv [expr $count-1]]
			continue
		}
		'-mb' {
			set maxburst [lindex $argv [expr $count-1]]
			continue
		}
		'-win' {
			set downwin [lindex $argv [expr $count-1]]
			continue
		}
		'-upwin' {
			set upwin [lindex $argv [expr $count-1]]
			continue
		}
		'-tk' {
			set tcpTick [lindex $argv [expr $count-1]]
			continue
		}
		'-rbw' {
			 set rbw [lindex $argv [expr $count-1]]
			continue
		}
		'-rq' {
			set rqsize [lindex $argv [expr $count-1]]
			continue
		}
		'-q' {
			set qsize [lindex $argv [expr $count-1]]
			Queue set limit_ $qsize 
			continue
		}
		'-nonfifo' {
			set nonfifo 1
			incr count -1
			continue
		}
		'-noecn' {
			Agent/TCP set disable_ecn_ 1
			incr count -1
			continue
		}
		'-red' {
			set rgw "RED"
			incr count -1
			continue
		}
		'-rd' {
			set rand_drop true
			incr count -1
			continue
		}
		'-pd' {
			set prio_drop true
			incr count -1
			continue
		}
		'-g' {
			set g [lindex $argv [expr $count-1]]
			Agent/TCP/Asym set g_ g
			continue
		}
		'-qwt' {
			set q_wt [lindex $argv [expr $count-1]]
			continue
		}
		'-acksfirst' {
			set acksfirst true
			incr count -1
			continue
		}
		'-afil' {
			set filteracks true
			incr count -1
			continue
		}
		'-recons' {
			set reconsacks true
			incr count -1
			continue
		}
		'-noadap' {
			Agent/AckReconsClass set adaptive_ 0
			incr count -1
			continue
		}
		'-replace_head' {
			set replace_head true
			incr count -1
			continue
		}
		'-mid' {
			set midtime [lindex $argv [expr $count-1]]
			continue
		}
		'-stop' {
			set stop [lindex $argv [expr $count-1]]
			continue
		}
		default {
			incr count -2
		} 
	}
	switch -exact '$arg' {
		'-dn' {
			set direction "down"
		}
		'-up' {
			set direction "up"
		}
		'-2way' {
			set direction "2way"
		}
		default {
			puts "arg $arg"
			printUsage
			exit
		}
	}
	incr count 1
	set arg [lindex $argv $count]
	switch -exact '$arg' {
		'-asym' {
			set type "asym"
		}
		'-asymsrc' {
			set type "asymsrc"
		}
		'-reno' {
			set type "reno"
		}
		'-reno2' {
			set type "reno2"
		}
		default {
			printUsage
			puts "arg $arg"
			exit
		}
	}
	incr count 1
	set arg [lindex $argv $count]
	set startTime $arg
	incr count 1
	
	set connGraph 0 
	set arg [lindex $argv $count]
	if { $arg == "-graph" } {
		set connGraph 1
		incr count 1
	}

	if { $direction == "down" } {
		set src $n0
		set dst $n3
	} elseif { $direction == "up" } {
		set src $n3
		set dst $n0
	} elseif { $direction == "2way" } {
		puts "2-way"
		set src $n0
		set dst $n3
		set src2 $n3
		set dst2 $n0
	}
	
	if { $randomize } { 
		ns-random 0
	}

	for { set i 0 } { $i < 2 } { incr i } {
		if { $type == "asym" } {
			set tcp($i) [createTcpSource "TCP/Reno/Asym" $maxburst $tcpTick $downwin]
			set rgw "RED"
			$tcp($i) set damp_ 0
			$tcp($i) set g_ 0.125
			set sink($i) [createTcpSink "TCPSink/Asym" $sinktrace $ackSize $maxdelack]
		} elseif { $type == "asymsrc" } {
			 set tcp($i) [createTcpSource "TCP/Reno/Asym" $maxburst $tcpTick $downwin]
			 $tcp($i) set damp_ 0
			 $tcp($i) set g_ 0.125
			 set sink($i) [createTcpSink "TCPSink" $sinktrace $ackSize]
		 } elseif { $type == "reno" } {
			 set tcp($i) [createTcpSource "TCP/Reno" $maxburst $tcpTick $downwin]
			 set sink($i) [createTcpSink "TCPSink" $sinktrace $ackSize]
		 } elseif { $type == "reno2" } {
			 set tcp($i) [createTcpSource "TCP/Reno" 0 $tcpTick $downwin]
			 set sink($i) [createTcpSink "TCPSink/DelAck" $sinktrace $ackSize]
		 }
	 }

	 Agent/AckReconsClass set size_ [$tcp(0) set packetSize_]

	 if { $direction == "up" && $upwin != 0 } {
		 $tcp(0) set window_ $upwin
	 }
	 if { $direction == "2way" } {
		 set ftp(1) [createFtp $ns $src2 $tcp(1) $dst2 $sink(1)]
	 }
	 set ftp(0) [createFtp $ns $src $tcp(0) $dst $sink(0)]

	 setupTcpTracing $tcp(0) $tcptrace
 #	setupGraphing $tcp $connGraph connGraphFlag
 #	puts "ftp $ftp ftp2 $ftp2"
	 $ns at $startTime "$ftp(0) start" 
	 set offset [uniform 5 10]
	 puts "rev conn at $offset"
	 set s2 [expr $startTime + $offset]
	 if { $direction == "2way"} {
		 $ns at $s2 "$ftp(1) start" 
	 }
 #	puts "starting ftp2 at $s2"
 }

if { $nonfifo && $rgw == "RED" } {
	set rgw "RED/Semantic"
}


# topology
#
#      10Mb, 1ms       10Mb, 5ms       10Mb, 1ms
#  n0 ------------ n1 ------------ n2 ------------ n3
#                     28.8Kb, 50ms 
#
$ns duplex-link $n0 $n1 10Mb 1ms DropTail
$ns simplex-link $n1 $n2 10Mb 5ms DropTail
$ns simplex-link $n2 $n1 $rbw 50ms $rgw
$ns duplex-link $n2 $n3 10Mb 1ms DropTail

# configure reverse bottleneck queue
puts "nonfifo $nonfifo rqsize $rqsize fqsize $qsize"
configQueue $ns $n2 $n1 $rgw $qtrace $redtrace $rqsize $nonfifo $acksfirst $filteracks $replace_head $prio_drop $rand_drop
configQueue $ns $n1 $n0 DropTail 0 0 20 $nonfifo false false false false false $reconsacks

# end simulation
$ns at $stop "finish $ns $f $tcptrace $graph connGraphFlag $midtime"

$ns run
