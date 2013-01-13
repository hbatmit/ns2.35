#!../../../../ns
#source ../../../lan/ns-lan.tcl
source ../../../ex/asym/util.tcl

# set up simulation

set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f

set tcptrace [open tcp.tr w] 
set sinktrace [open sink.tr w]
set redtrace [open red.tr w]
set graph 0
set connGraphFlag("") 0
set maxdelack 25
set maxburst 3
set upwin 0
set tcpTick 0.1
set ackSize 40
set rbw 28.8Kb
set rqsize 10
set qsize 10
set rgw "DropTail"
set nonfifo 0 
set acksfirst false
set filteracks false
set replace_head false
set midtime 0

proc printUsage {} {
	puts "Usage: "
	exit
}

proc finish {ns traceall tcptrace graph connGraphFlag midtime} {
	upvar $connGraphFlag graphFlag 

	$ns flush-trace
	close $traceall
	flush $tcptrace
	close $tcptrace
	plotgraph $graph $graphFlag $midtime
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
		'-acksfirst' {
			set acksfirst true
			incr count -1
			continue
		}
		'-filteracks' {
			set filteracks true
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
		default {
			puts "arg $arg"
			printUsage
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
	} else {
	}
	
	if { $type == "asym" } {
		set tcp [createTcpSource "TCP/Reno/Asym" $maxburst $tcpTick]
		set sink [createTcpSink "TCPSink/Asym" $sinktrace $ackSize $maxdelack]
	} elseif { $type == "asymsrc" } {
		set tcp [createTcpSource "TCP/Reno/Asym" $maxburst $tcpTick]
		set sink [createTcpSink "TCPSink" $sinktrace $ackSize]
	} elseif { $type == "reno" } {
		set tcp [createTcpSource "TCP/Reno" $tcpTick]
		set sink [createTcpSink "TCPSink" $sinktrace $ackSize]
	}
	
	if { $direction == "up" && $upwin != 0 } {
		$tcp set window_ $upwin
	}
	set ftp [createFtp $ns $src $tcp $dst $sink]
	setupTcpTracing $tcp $tcptrace
	setupGraphing $tcp $connGraph connGraphFlag
	$ns at $startTime "$ftp start" 
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
configQueue $ns $n2 $n1 $rgw $redtrace $rqsize $nonfifo $acksfirst $filteracks $replace_head

# end simulation
$ns at 30.0 "finish $ns $f $tcptrace $graph connGraphFlag $midtime"

$ns run
