#source ../../../lan/ns-lan.tcl
source ../../../ex/asym/util.tcl
source ../../../ex/tcp-fs/web.tcl

Queue/RED set q_weight_ 0.1

# set up simulation
set ns [new Simulator]
set numsrc 31
set numdst 31
for {set i 0} {$i < $numsrc} {incr i} {
	set n(0$i) [$ns node]
}
set n(1) [$ns node]
set n(2) [$ns node]
for {set i 0} {$i < $numdst} {incr i} {
	set n(3$i) [$ns node]
}

set dir "."
set trace_opened 0
set graph 0
set phttpAllFlag 0
set httpseqAllFlag 0
set connGraphFlag("") 0
set maxdelack 25
set maxburst 0
set upwin 0
set tcpTick 0.1
set ackSize 40
set fbw 1.5Mb
set rbw 28.8Kb
set fqsize -1
set rqsize -1
set qsize 10
set fgw "DropTail"
set rgw "DropTail"
set acksfirst false
set filteracks false
set replace_head false
set midtime 0
set window 100
set burstwin 0
set webwin 0
set nonfifo 0
set priority_drop false
set random_drop false
set random_ecn false
set count_bytes_acked false
set fixed_period 0
set fixed_period_except_first 0
set fgw_q_weight -1
set rgw_q_weight -1
set burstsz 5
set firstburstsz 0
set pause 5
set monitorq 0
set donttrace 0
set traceq 0
set duration 30
set delay 50ms
set seed -1
set topology "fs"
set turnontime -1
set turnofftime -1
set schedDisp $FINE_ROUND_ROBIN
set overhead 0
set f 0
set tcptrace 0
set sinktrace 0
set redtrace 0
set queuetrace 0
set numimg 4
set reqsize 0.3
set htmlsize 1
set imgsize 10

proc finish {ns traceall tcptrace redtrace queuetrace graph connGraphFlag midtime turnontime turnofftime { qtraceflag false } } {
	upvar $connGraphFlag graphFlag
	global dir
	global qm
	global donttrace

	puts stderr "[$qm(12) set pdrops_] [$qm(21) set pdrops_] "
	if {!$donttrace} {
		$ns flush-trace
		close $traceall
		flush $tcptrace
		close $tcptrace
		flush $redtrace
		close $redtrace
		flush $queuetrace
		close $queuetrace
		processtrace $midtime $turnontime $turnofftime $qtraceflag $dir
	}
#	plotgraph $graph graphFlag $midtime $turnontime $turnofftime $qtraceflag $dir
	exit 0
}

proc burst_finish { ns ftp burstsz pause } {
	$ns at [expr [$ns now] + $pause] "$ftp producemore $burstsz"
}

proc periodic_burst { ns ftp burstsz pause } {
	$ftp producemore $burstsz
	$ns at [expr [$ns now] + $pause] "periodic_burst $ns $ftp $burstsz $pause"
}

# read user options and act accordingly
set count 0
while {$count < $argc} {
	set arg [lindex $argv $count]
	incr count 2
	switch -exact '$arg' {
		'-null' {
			incr count -1
			continue
		}
		'-graph' {
			set graph 1
			incr count -1
			continue
		}
		'-phttp' {
			set phttpAllFlag 1
			incr count -1
			continue
		}
		'-httpseq' {
			set httpseqAllFlag 1
			incr count -1
			continue
		}
		'-red' {
			set fgw "RED"
			set rgw "RED"
			incr count -1
			continue
		}
		'-sred' {
			set fgw "RED/Semantic"
			set rgw "RED/Semantic"
			incr count -1
			continue
		}
		'-pred' {
			set fgw "PRED"
			incr count -1
			continue
		}
		'-nonfifo' {
			set nonfifo 1
			incr count -1
			continue
		}
		'-pdrop' {
			set priority_drop true
			incr count -1
			continue
		}
		'-rdrop' {
			set random_drop true
			incr count -1
			continue
		}
		'-recn' {
			set random_ecn true
			incr count -1
			continue
		}
		'-cba' {
			set count_bytes_acked true
			incr count -1
			continue
		}
		'-fp' {
			set fixed_period 1
			incr count -1
			continue
		}
		'-fpef' {
			set fixed_period_except_first 1
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
		'-noflr' {
			Agent/TCP set fast_loss_recov_ false
			incr count -1
			continue
		}
		'-nofrt' {
			Agent/TCP set fast_reset_timer_ false
			incr count -1
			continue
		}
		'-fixediw' {
			Agent/TCP/Session set fixedIw_ true
			incr count -1
			continue
		}
		'-proxy' {
			Agent/TCP/Session set proxyopt_ true
			incr count -1
			continue
		}
		'-monitorq' {
			set monitorq 1
			incr count -1
			continue
		}
		'-donttrace' {
			set donttrace 1
			incr count -1
			continue
		}
		'-traceq' {
			if {[regexp {^[0-9]} [expr $count -1]]} {
				set fromNode [lindex $argv [expr $count -1]]
				set toNode [lindex $argv $count]
				set traceqFromNode($fromNode,$toNode) $fromNode
				set traceqToNode($fromNode,$toNode) $toNode
				incr count 1
			} else {
				set traceq 1
				incr count -1
			}
			continue
		}
		'-numimg' {
			set numimg [lindex $argv [expr $count-1]]
			continue
		}
		'-reqsz' {
			set reqsize [lindex $argv [expr $count-1]]
			continue
		}
		'-htmlsz' {
			set htmlsize [lindex $argv [expr $count-1]]
			continue
		}
		'-imgsz' {
			set imgsize [lindex $argv [expr $count-1]]
			continue
		}
		'-mda' {
			set maxdelack [lindex $argv [expr $count-1]]
			continue
		}
		'-mb' {
			set maxburst [lindex $argv [expr $count-1]]
			continue
		}
		'-acksz' {
			set ackSize [lindex $argv [expr $count-1]]
			continue
		}
		'-tk' {
			set tcpTick [lindex $argv [expr $count-1]]
			Agent/TCP/Session set tcpTick_ [lindex $argv [expr $count-1]]
			continue
		}
		'-fbw' {
			set fbw [lindex $argv [expr $count-1]]
			continue
		}
		'-rbw' {
			set rbw [lindex $argv [expr $count-1]]
			continue
		}
		'-fq' {
			set fqsize [lindex $argv [expr $count-1]]
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
		'-bs' {
			set burstsz [lindex $argv [expr $count-1]]
			continue
		}
		'-fbs' {
			set firstburstsz [lindex $argv [expr $count-1]]
			continue
		}
		'-pause' {
			set pause [lindex $argv [expr $count-1]]
			continue
		}
		'-win' {
			set window [lindex $argv [expr $count-1]]
			continue
		}
		'-bwin' {
			set burstwin [lindex $argv [expr $count-1]]
			continue
		}
		'-webwin' {
			set webwin [lindex $argv [expr $count-1]]
			continue
		}
		'-upwin' {
			set upwin [lindex $argv [expr $count-1]]
			continue
		}
		'-redwt' {
			set fgw_q_weight [lindex $argv [expr $count-1]]
			set rgw_q_weight [lindex $argv [expr $count-1]]
			continue
		}
		'-dur' {
			set duration [lindex $argv [expr $count-1]]
			continue
		}
		'-del' {
			set delay [lindex $argv [expr $count-1]]
			continue
		}
		'-src' {
			set src $n([lindex $argv [expr $count-1]])
			continue
		}
		'-dst' {
			set dst $n([lindex $argv [expr $count-1]])
			continue
		}
		'-ton' {
			set turnontime [lindex $argv [expr $count-1]]
			continue
		}
		'-toff' {
			set turnofftime [lindex $argv [expr $count-1]]
			continue
		}
		'-dir' {
			set dir [lindex $argv [expr $count-1]]
			continue
		}
		'-seed' {
			set seed [lindex $argv [expr $count-1]]
			continue
		}
		'-topo' {
			set topology [lindex $argv [expr $count-1]]
			continue
		}
		'-sched' {
			set sched [lindex $argv [expr $count-1]]
			switch -exact '$sched' {
				'frr' {set schedDisp $FINE_ROUND_ROBIN}
				'crr' {set schedDisp $COARSE_ROUND_ROBIN}
				'rand' {set schedDisp $RANDOM}
				'null' {}
			}
			continue
		}
		'-overhead' {
			Agent/TCP set overhead_ [lindex $argv [expr $count-1]]
			continue
		}
		default {
			incr count -2
		}
	}

	if {!$trace_opened && !$donttrace} {
		# set up trace files
		set f [open $dir/out.tr w]
		$ns trace-all $f
		set tcptrace [open $dir/tcp-raw.tr w] 
		set sinktrace [open $dir/sink.tr w]
		set redtrace [open $dir/red.tr w]
		set queuetrace [open $dir/q.tr w]
		set trace_opened 1
	}

	if {$turnontime == -1} {
		set turnontime 0
	}
	if {$turnofftime == -1} {
		set turnofftime $duration
	}

	set burstflag 0
	set webFlag false
	set phttpFlag 0
	set httpseqFlag 0
	set direction ""
	while {$count < $argc} {
		set arg [lindex $argv $count]
		incr count 1
		switch -exact '$arg' {
			'-burst' {
				set burstflag 1
				continue
			}
			'-dn' {
				set direction "down"
				continue
			}
			'-up' {
				set direction "up"
				continue
			}
			'-web' {
				set webFlag true
				continue
			}
			'-phttp' {
				set phttpFlag 1
				continue
			}
			'-httpseq' {
				set httpseqFlag 1
				continue
			}
			default {
				incr count -1
				break
			}
		}
	}

	if { $direction == "down" } {
		set src $n(00)
		set dst $n(30)
	} elseif { $direction == "up" } {
		set src $n(30)
		set dst $n(00)
	} else {
	}
	
	set arg [lindex $argv $count]
	set slow_start_restart true
	set fs_enable false
	set srctype "TCP/Fack"
	set sinktype "TCPSink"
	set sessionFlag false
	switch -exact '$arg' {
		'-null' {
		}
		'-tahoe' {
			set srctype "TCP"
		}
		'-tahoefs' {
			set srctype "TCP/FS"
			set slow_start_restart false
			set fs_enable true
		}
		'-reno' {
			set srctype "TCP/Reno"
		}
		'-renofs' {
			set srctype "TCP/Reno/FS"
			set slow_start_restart false
			set fs_enable true
		}
		'-newreno' {
			set srctype "TCP/Newreno"
		}
		'-newrenofs' {
			set srctype "TCP/Newreno/FS"
			set slow_start_restart false
			set fs_enable true
		}
		'-fack' {
			set srctype "TCP/Fack"
			set sinktype "TCPSink/Sack1"
		}
		'-fackfs' {
			set srctype "TCP/Fack/FS"
			set sinktype "TCPSink/Sack1"
			set slow_start_restart false
			set fs_enable true
		}
		'-renoasym' {
			set srctype "TCP/Reno/Asym"
			set sinktype "TCPSink/Asym"
		}
		'-newrenoasym' {
			set srctype "TCP/Newreno/Asym"
			set sinktype "TCPSink/Asym"
		}
		'-newrenoasymfs' {
			set srctype "TCP/Newreno/Asym/FS"
			set sinktype "TCPSink/Asym"
			set slow_start_restart false
			set fs_enable true
		}
		'-int' {
			set srctype "TCP/Int"
			set sessionFlag true
		}
		'-intfs' {
			set srctype "TCP/Int"
			set sessionFlag true
			set slow_start_restart false
			set fs_enable true
		}
		default {
			incr count -1
		}
	}
	set randTime 0
	incr count 1
	set arg [lindex $argv $count]
	set startTime $arg
	incr count 1
	set connGraph 0 
	set arg [lindex $argv $count]
        if { $arg == "-graph" } {
                set connGraph 1
                incr count 1
        } elseif { $arg == "-rand" } {
		incr count 1
		set arg [lindex $argv $count]
		set randTime $arg
		incr count 1
	}
	if {($direction == "up") && ($upwin > 0)} {
		set win $upwin
	} elseif {($burstflag == 1) && ($burstwin > 0)} {
		set win $burstwin
	} elseif {$webFlag && $webwin > 0} {
		set win $webwin
	} else {
		set win $window
	}

	if {$webFlag} {
		set web [new WebCS $ns $src $dst "$srctype $maxburst $tcpTick $win $slow_start_restart $fs_enable" $tcptrace "$sinktype $sinktrace" [expr $phttpAllFlag || $phttpFlag] [expr $httpseqAllFlag || $httpseqFlag] $sessionFlag "$count_bytes_acked $schedDisp $fs_enable"]
		$web set finish_ "finish $ns $f $tcptrace $redtrace $queuetrace $graph connGraphFlag 0 $turnontime $turnofftime [expr $monitorq || $traceq]"
		$ns at $startTime "$web start"
	} else {
		set tcp [createTcpSource $srctype $maxburst $tcpTick $win $slow_start_restart $fs_enable]
		set sink [createTcpSink $sinktype $sinktrace]
		set ftp [createFtp $ns $src $tcp $dst $sink]
		set newSessionFlag false
		if {$sessionFlag} {
			set newSessionFlag [setupTcpSession $tcp $count_bytes_acked $schedDisp $fs_enable]
			#		set d [expr ([$tcp set dst_]/256)*256]
			#		set session [[$tcp set node_] getTcpSession $d]
		}
		setupTcpTracing $tcp $tcptrace $newSessionFlag
		setupGraphing $tcp $connGraph connGraphFlag $newSessionFlag
		if { $burstflag == 0 } {
			$ns at $startTime "$ftp start"
		} elseif { $fixed_period } {
			if {$firstburstsz > 0} {
				$ns at $startTime "$ftp produce $firstburstsz"
			} else {
				$ns at $startTime "$ftp produce $burstsz"
			}
			$ns at [expr $startTime+$pause] "periodic_burst $ns $ftp $burstsz $pause"
		} elseif { $fixed_period_except_first } {
			if {$firstburstsz > 0} {
				$ns at $startTime "$ftp produce $firstburstsz"
			} else {
				$ns at $startTime "$ftp produce $burstsz"
			}
			$ns at [expr $pause+$randTime] "periodic_burst $ns $ftp $burstsz $pause"
		} else {
			$tcp proc done {} "burst_finish $ns $ftp $burstsz $pause"
			if {$firstburstsz > 0} {
				$ns at $startTime "$ftp produce $firstburstsz"
			} else {
				$ns at $startTime "$ftp produce $burstsz"
			}
		}
		set ok 1
		while {$ok == 1 && $count < $argc} {
			set arg [lindex $argv $count]
			switch -exact '$arg' {
				'-at' {
					incr count 1
					set time [lindex $argv $count]
					incr count 1
					set cmd [lindex $argv $count]
					incr count 1
					$ns at $time "$tcp $cmd"
				}
				default {
					set ok 0
				}
			}
		}
	}

}

# seed random number generator
if {$seed >= 0} {
	ns-random $seed
}
if {$topology == "fs"} {
	# topology
	#
	#     
	#  n00                         n30
	#    \                        /
	#     \  10Mb, 1ms           / 10Mb, 1ms
	#      \                    /
	#       \    1Mb, 50ms     /
	#        n1 ------------ n2
	#       /                  \
	#      /                    \
	#     /  10Mb, 1ms           \ 10Mb, 1ms
	#    /                        \
	#  n01                         n31
	for {set i 0} {$i < $numsrc} {incr i 1} {
		$ns duplex-link $n(0$i) $n(1) 10Mb 1ms DropTail
	}
	$ns duplex-link $n(1) $n(2) $fbw $delay $fgw
	for {set i 0} {$i < $numdst} {incr i 1} {
		$ns duplex-link $n(2) $n(3$i) 10Mb 1ms DropTail
	}
	# configure forward bottleneck queue
	configQueue $ns $n(1) $n(2) $fgw $queuetrace $fqsize $nonfifo false false false $priority_drop $random_drop $random_ecn
	configQueue $ns $n(2) $n(1) $rgw $queuetrace $rqsize $nonfifo false false false $priority_drop $random_drop $random_ecn
	set l(12) [$ns link $n(1) $n(2)]
	set qm(12) [$l(12) set qMonitor_]
	set l(21) [$ns link $n(2) $n(1)]
	set qm(21) [$l(21) set qMonitor_]
	#configQueue $ns $n(1) $n(2) $fgw 0 $fqsize $nonfifo false false false $priority_drop $random_drop $random_ecn
	configREDQueue $ns $n(1) $n(2) $redtrace $fgw_q_weight 1 0.15 0.6 
	configREDQueue $ns $n(2) $n(1) $redtrace $rgw_q_weight 1 0.15 0.6
} elseif {$topology == "asym"} {
	# topology
	#
	#      10Mb, 1ms       10Mb, 2ms       10Mb, 1ms
	#  n00 ------------ n1 ------------ n2 ------------ n30
	#                     28.8Kb, 50ms 
	#
	for {set i 0} {$i < $numsrc} {incr i 1} {
		$ns duplex-link $n(0$i) $n(1) 10Mb 1ms DropTail
	}
	$ns simplex-link $n(1) $n(2) 10Mb 2ms DropTail
	$ns simplex-link $n(2) $n(1) $rbw 50ms $rgw
	for {set i 0} {$i < $numdst} {incr i 1} {
		$ns duplex-link $n(2) $n(3$i) 10Mb 1ms DropTail
	}
	
	# configure reverse bottleneck queue
	configQueue $ns $n(2) $n(1) $rgw 0 $rqsize $nonfifo $acksfirst $filteracks $replace_head
	configREDQueue $ns $n(2) $n(1) $redtrace $rgw_q_weight
}

# trace queues
foreach i [array names traceqFromNode] {
	trace_queue $ns $n($traceqFromNode($i)) $n($traceqToNode($i)) $queuetrace
	set traceq 1
}

		

#end simulation
$ns at $duration "finish $ns $f $tcptrace $redtrace $queuetrace $graph connGraphFlag 0 $turnontime $turnofftime [expr $monitorq || $traceq]"
$ns run
