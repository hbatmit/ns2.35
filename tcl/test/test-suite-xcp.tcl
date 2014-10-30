#
# Copyright (c) 2004-2006 University of Southern California.
# All rights reserved.						  
#								 
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.	The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

# To run all tests: test-all-xcp
# to run individual test:
# ns test-suite-xcp.tcl simple-xcp
# ns test-suite-xcp.tcl simple-full-xcp

# To view a list of available test to run with this script:
# ns test-suite-xcp.tcl

# This test-suite validate xcp congestion control scenarios along with
# xcp-tcp mixed flows through routers.

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP XCP ; # hdrs reqd for validation

set fullxcp 1
set halfxcp 1
set validtests ""

if {![TclObject is-class Agent/TCP/Reno/XCP]} {
	puts "xcp (half) module is not present; validation skipped"
	set halfxcp 0
} else {
	set validtests "simple-xcp $validtests"
}
if {![TclObject is-class Agent/TCP/FullTcp/Newreno/XCP]} {
	puts "xcp (full) module is not present; validation skipped"
	set fullxcp 0
	if { $halfxcp == 0 } {
		exit 0; #will skip entire suite
	}
} else {
	set validtests "$validtests simple-full-xcp"
	# The following tests aren't validated because
	#   xcp-tcp isn't really testing anything useful and
	#   parking-lot-topo utilization plot is useless
	#set validtests "$validtests xcp-tcp parking-lot-topo"
}

Class TestSuite

proc usage {} {
	global argv0 validtests
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: $validtests"
	exit 1
}


TestSuite instproc init {} {
	
	$self instvar ns_ rtg_ tracefd_ qType_ qSize_ BW_ delay_ \
	    tracedFlows_
	set ns_ [new Simulator]
	$ns_ use-scheduler Heap
	set rtg_ [new RNG]
	$rtg_ seed 472904
}

TestSuite instproc create-Bottleneck {} {
	global R0 R1 Bottleneck rBottleneck l rl all_links
	$self instvar ns_ qType_ qSize_ BW_ delay_
	
	# create bottleneck nodes
	set R0 [$ns_ node]
	set R1 [$ns_ node]
	
	$ns_ duplex-link $R0 $R1 [set BW_]Mb [set delay_]ms $qType_
	$ns_ queue-limit $R0 $R1 $qSize_
	$ns_ queue-limit $R1 $R0 $qSize_

	# Give a global handle to the Bottleneck Queue to allow 
	# setting the RED paramters

	set	 Bottleneck  [[$ns_ link $R0 $R1] queue] 
	set	 rBottleneck [[$ns_ link $R1 $R0] queue]

	set	 l  [$ns_ link $R0 $R1]	 
	set	 rl [$ns_ link $R1 $R0]

	set all_links "$l $rl "
}

TestSuite instproc create-sidelinks {numSideLinks deltaDelay} {
	global R0 R1 all_links n
	$self instvar ns_ BW_ delay_ qSize_ qType_
	
	set i 0
	while { $i < $numSideLinks } {
		set n($i)  [$ns_ node]
		$ns_ duplex-link $n($i) $R0 [set BW_]Mb [expr $delay_ + $i * $deltaDelay]ms $qType_

		$ns_ queue-limit $n($i)	 $R0  $qSize_
		$ns_ queue-limit $R0 $n($i)   $qSize_
		set  q$i   [[$ns_ link $n($i)  $R0] queue] 
		set  rq$i  [[$ns_ link $R0 $n($i)] queue]
		set  l$i    [$ns_ link $n($i)  $R0] 
		set  rl$i   [$ns_ link $R0 $n($i)]
		set all_links "$all_links [set l$i] [set rl$i] "
		incr i
	}

}

TestSuite instproc create-string-topology {numHops BW_list d_list qtype_list qsize_list } {
	$self instvar ns_
	global quiet n all_links
	set numNodes [expr $numHops + 1 ]
	
	# first sanity check
    if { [llength $BW_list] !=  [llength $d_list] || [llength $qtype_list] !=  [llength $qsize_list]} {
	error "Args sizes don't match with $numHops hops"
	if { [llength $BW_list] != $numHops || $numHops != [llength $qsize_list]} {
	    puts "error in using proc create-string-topology"
	    error "Args sizes don't match with $numHops hops"
	}
    }
	# compute the pipe assuming delays are in msec and BWs are in Mbits
    set i 0; set forwarddelay 0; global minBW; set minBW [lindex $BW_list 0];
    while { $i < $numHops } {
	set forwarddelay [expr $forwarddelay + [lindex $d_list $i]]
	if {$minBW > [lindex $BW_list $i] } { set $minBW [lindex $BW_list $i] }
	incr i
    }
	# pipe in bytes,assuming pktsize as 1000bytes,BW in Mbps,delay in ms
	set pipe [expr round([expr ($minBW * 2.0 * $forwarddelay)/8.0 ])]
	
	set i 0
	while { $i < $numNodes } {
		set n($i) [$ns_ node]
		incr i
	}
	set all_links ""
	set  i 0 
	while { $i < $numHops } {
		set qsize [lindex $qsize_list $i]
		set bw    [lindex $BW_list $i]
		set delay [lindex $d_list $i]
		set qtype [lindex $qtype_list $i]
		if {$quiet == 0} {
			puts "$i bandwidth $bw"
		}
		if {$qsize == 0} { set qsize $pipe}
		$ns_ duplex-link [set n($i)] [set n([expr $i +1])] [set bw]Mb [set delay]ms $qtype
		$ns_ queue-limit [set n($i)] [set n([expr $i + 1])] $qsize
		$ns_ queue-limit [set n([expr $i + 1])] [set n($i)] $qsize
		
		# Give a global handle to the Queues to allow setting the RED paramters
		set  l$i   [$ns_ link [set n($i)] [set n([expr $i + 1])]]
		set  rl$i  [$ns_ link [set n([expr $i + 1])] [set n($i)]]
		set all_links "$all_links [set l$i] [set rl$i] "
		global q$i rq$i
		set q$i  [[$ns_ link [set n($i)] [set n([expr $i + 1])]] queue]
		set rq$i [[$ns_ link [set n([expr $i + 1])] [set n($i)]] queue]
		incr i
	}
}

TestSuite instproc process-parking-lot-data {what name flows PlotTime} {
	global quiet
	set tracedFlows $flows
	exec rm -f temp.rands
	set f [open temp.rands w]
	
	foreach i $tracedFlows {
		exec rm -f temp.c temp.out
		exec touch temp.c temp.out
		set result [exec awk -v PlotTime=$PlotTime -v what=$what {
			{
				if (($1 == what) && ($2 > PlotTime)) {
					print $2, $3 >> "temp.c";
				}
			}
		} ft_red_q$i.tr ]
			
		exec awk -v L=$i \
		    {BEGIN {sum=0.0; f=0;} {sum=sum+$2;f=f+1} \
			 END {printf("%d %.2g\n", L+1, sum/f);} } \
		    "temp.c" >> "temp.out"
	
		exec cat temp.out >@ $f
		flush $f
	}
	close $f
	if {$quiet == 0} {
		exec xgraph -m -x "link ID" -y $name temp.rands &
	}
}


TestSuite instproc post-process {what PlotTime} {
	global quiet
	$self instvar tracedFlows_ src_
	exec rm -f temp.rands
	set f [open temp.rands w]
	
	foreach i $tracedFlows_ {
		exec rm -f temp.c 
		exec touch temp.c
		set result [exec awk -v PlotTime=$PlotTime -v what=$what {
			{
				if (($6 == what) && ($1 > PlotTime)) {
				    if ((what == "throughput"))
                                        print $1, ($7 * 8) >> "temp.c";
                                    else
                                  	print $1, $7 >> "temp.c";
				}
			}
		} xcp$i.tr ]
		
		puts $f \"$what$i
		exec cat temp.c >@ $f
		puts $f "\n"
		flush $f
	}
	close $f
	if {$quiet == 0} {
		exec xgraph -nl -m -x time -y $what temp.rands &
	}
}


TestSuite instproc finish {} {
	$self instvar ns_ tracefd_ tracedFlows_ src_ qtraces_
	if [info exists tracedFlows_] {
		foreach i $tracedFlows_ {
			set file [[set src_($i)] set tcpTrace_]
			if {[info exists file]} {
				flush $file
				close $file
			}	
		}
	}

	if {[info exists tracefd_]} {
		flush $tracefd_
		close $tracefd_
	}
	if {[info exists qtraces_]} {
		foreach file $qtraces_ {
			if {[info exists file]} {
				flush $file
				close $file
			}
		}
	}
	$ns_ halt
}

Class GeneralSender  -superclass Agent 

#   otherparams are "startTime TCPclass .."

GeneralSender instproc init { ns id srcnode dstnode otherparams } {
	global quiet
	$self next
	$self instvar tcp_ id_ ftp_ snode_ dnode_ tcp_rcvr_ tcp_type_
	set id_ $id
	
	if { [llength $otherparams] > 1 } {
		set TCP [lindex $otherparams 1]
	} else { 
		puts stderr "undefined transport protocol type"
		exit 1
	}
	
	switch -exact $TCP {
		TCP/FullTcp/Newreno/XCP {
			set TCPSINK "TCP/FullTcp/Newreno/XCP"
		} 
		TCP/FullTcp/Newreno {
			set TCPSINK "TCP/FullTcp/Newreno"
		}
		TCP/Reno/XCP {
			set TCPSINK "TCPSink/XCPSink"
		}
		TCP/Reno {
			set TCPSINK "TCPSink"
		}
		default {
			puts stderr "unsupported protocol $TCP"
			exit 1
		}
	}
	
	if { [llength $otherparams] > 2 } {
		set traffic_type [lindex $otherparams 2]
	} else {
		set traffic_type FTP
	}
	
	set	  tcp_type_ $TCP
	set	  tcp_ [new Agent/$TCP]
	set	  tcp_rcvr_ [new Agent/$TCPSINK]
	$tcp_ set  packetSize_ 1000	  
	$tcp_ set  segsize_ 1000
	$tcp_ set  class_  $id

	switch -exact $traffic_type {
		FTP {
			set traf_ [new Application/FTP]
			$traf_ attach-agent $tcp_
		}
		EXP {
			
			set traf_ [new Application/Traffic/Exponential]
			$traf_ set packetSize_ 1000
			$traf_ set burst_time_ 250ms
			$traf_ set idle_time_ 250ms
			$traf_ set rate_ 10Mb
			$traf_ attach-agent $tcp_
		}
		default {
			puts "unsupported traffic\n"
			exit 1
		}
	}
	$ns	  attach-agent $srcnode $tcp_
	$ns	  attach-agent $dstnode $tcp_rcvr_
	$ns	  connect $tcp_	 $tcp_rcvr_
        $tcp_rcvr_ listen;

	set	  startTime [lindex $otherparams 0]
	$ns	  at $startTime "$traf_ start"

	if {$quiet == 0} {
		puts  "initialized Sender $id_ at $startTime"
	}
}


GeneralSender instproc trace-xcp parameters {
	$self instvar tcp_ id_ tcpTrace_
	set ftracetcp$id_ [open  xcp$id_.tr w]
	set tcpTrace_ [set ftracetcp$id_]
	$tcp_ attach-trace $tcpTrace_
	if { -1 < [lsearch $parameters cwnd]  } { $tcp_ tracevar cwnd_ }
	if { -1 < [lsearch $parameters seqno] } { $tcp_ tracevar t_seqno_ }
	if { -1 < [lsearch $parameters ackno] } { $tcp_ tracevar ack_ }
	if { -1 < [lsearch $parameters rtt]	 } { $tcp_ tracevar rtt_ }
	if { -1 < [lsearch $parameters ssthresh]  } { $tcp_ tracevar ssthresh_ }
	if { -1 < [lsearch $parameters throughput]  } { $tcp_ tracevar throughput_ }
}


Class Test/simple-xcp -superclass TestSuite

Test/simple-xcp instproc init {} {
	global halfxcp
	if { $halfxcp == 0 } { exit 0 }
	$self instvar ns_ testName_ qType_ qSize_ BW_ delay_ nXCPs_ \
 	    SimStopTime_ tracedFlows_

 	set testName_ simple-xcp
 	set qType_		XCP
 	set BW_			20; # in Mb/s
 	set delay_		10; # in ms
 	set qSize_		[expr round([expr ($BW_ / 8.0) * 4 * $delay_])];#set buffer to the pipe size
 	set SimStopTime_	30
 	set nXCPs_		3
 	set tracedFlows_	"0 1 2"
 	$self next 
}

Test/simple-xcp instproc get-tcpType {} {
	return 	"TCP/Reno/XCP"
}

Test/simple-xcp instproc run {} {
 	global R1 n all_links Bottleneck quiet
 	$self instvar ns_ SimStopTime_ nXCPs_ qSize_ delay_ rtg_ \
 	    tracedFlows_ src_ allchan_

 	set numsidelinks 3
 	set deltadelay 0.0
	
 	$self create-Bottleneck
 	$self create-sidelinks $numsidelinks $deltadelay

 	foreach link $all_links {
 		set queue [$link queue]
 		if {[$queue info class] == "Queue/XCP"} {
 			$queue set-link-capacity [[$link set link_] set bandwidth_];  
 		}
 	}

	# added for troubleshooting purposes - Sally
	if {$quiet == "false"} {
		set allchan_ [open all.tr w]
		$ns_ trace-all $allchan_
	}

 	# Create sources:
 	set i 0
 	while { $i < $nXCPs_  } {
 		set StartTime [expr $i * 10]
 		set src_($i) [new GeneralSender $ns_ $i [set n($i)] $R1 "$StartTime [$self get-tcpType]"]
		set pktSize_  1000
 		[[set src_($i)] set tcp_]  set	 packetSize_ $pktSize_
		[[set src_($i)] set tcp_]  set   segsize_ $pktSize_
 		[[set src_($i)] set tcp_]  set	 window_     [expr $qSize_]
 		incr i
 	}
	
 	# trace bottleneck queue, if needed
 	#foreach queue_name "Bottleneck" {
	#set queue [set "$queue_name"]
	#if {[$queue info class] == "Queue/XCP"} {
	# $queue attach $tracefd_
 	#	} 
 	#}
	
 	# trace sources
 	foreach i $tracedFlows_ {
 		#[set src_($i)] trace-xcp "cwnd"
		[set src_($i)] trace-xcp "throughput"
 	}
	if {$quiet == "false"} {
		$ns_ at $SimStopTime_ "close $allchan_"
	}
       
 	$ns_ at $SimStopTime_ "$self finish"
 	$ns_ run
	
 	#$self post-process cwnd_ 0.0
 	$self post-process throughput 0.0
}

Class Test/simple-full-xcp -superclass Test/simple-xcp

Test/simple-full-xcp instproc init {} {
	global fullxcp
	if { $fullxcp == 0 } { exit 0 }
 	$self set testName_ simple-full-xcp
 	$self next 
}

Test/simple-full-xcp instproc get-tcpType {} {
	return 	"TCP/FullTcp/Newreno/XCP"
}

#disabled on 05/27/06
 Class Test/xcp-tcp -superclass TestSuite

Test/xcp-tcp instproc init {} {
	global halfxcp
	if { $halfxcp == 0 } { exit 0 }
	$self instvar ns_ testName_ qType_ qSize_ BW_ delay_ nXCPs_ \
	    SimStopTime_ tracedFlows_

	# set RED parameters for TCP queue
	Queue/RED set maxthresh_ [expr 0.8 * [Queue set limit_]]
	Queue/RED set thresh_ [expr 0.6 * [Queue set limit_]]
	Queue/RED set q_weight_ 0.001
	Queue/RED set linterm_ 10
	
	set testName_   xcp-tcp
	set qType_	XCP
	set BW_		0; # in Mb/s
	set delay_	10; # in ms
	set qSize_      [expr round([expr ($BW_ / 8.0) * 4 * $delay_])];#set buffer to the pipe size
	set SimStopTime_	  30
	set nXCPs_		  3
	set tracedFlows_	  "0 1 2 3"

	$self next
}

Test/xcp-tcp instproc run {} {
	global R1 n all_links Bottleneck
	$self instvar ns_ tracefd_ SimStopTime_ nXCPs_ qSize_ delay_ rtg_ \
	    tracedFlows_ src_
	
	Queue/XCP set tcp_xcp_on_ 1
	set numsidelinks 4
	set deltadelay 0.0
	
	$self create-Bottleneck
	$self create-sidelinks $numsidelinks $deltadelay

	foreach link $all_links {
		set queue [$link queue]
		if {[$queue info class] == "Queue/XCP"} {
			$queue set-link-capacity [[$link set link_] set bandwidth_];  
		}
	}
	
	# Create sources:
	set i 0
	while { $i < $nXCPs_  } {
		set StartTime [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $delay_) + $i  * 0.0] 
		set src_($i) [new GeneralSender $ns_ $i [set n($i)] $R1 "$StartTime TCP/Reno/XCP"]
		set pktSize_              1000
		[[set src_($i)] set tcp_]  set	 packetSize_ $pktSize_
		[[set src_($i)] set tcp_]  set	 window_     [expr $qSize_ * 10]
		incr i
	}
	
	set StartTime [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $delay_) + $i  * 0.0] 
	set src_($i) [new GeneralSender $ns_ $i [set n($i)] $R1 "$StartTime TCP/Reno"]
	set pktSize_              1000
	[[set src_($i)] set tcp_]  set	 packetSize_ $pktSize_
	[[set src_($i)] set tcp_]  set	 window_     [expr $qSize_ * 10]
	
	
	# trace sources
	foreach i $tracedFlows_ {
		[set src_($i)] trace-xcp "cwnd"
	}
	
	$ns_ at $SimStopTime_ "$self finish"
	$ns_ run

	$self post-process cwnd_ 0.0

}

#disabled on 05/27/06
 Class Test/parking-lot-topo -superclass TestSuite
# This is a downsized version of Dina's original test. We use around 30 flows
# compared to 300 flows in the original version.

Test/parking-lot-topo instproc init {} {
	global halfxcp
	if { $halfxcp == 0 } { exit 0 }
   	$self instvar ns_ testName_ delay_ BW_list_ qType_list_ delay_list_ qSize_list_ nTCPsPerHop_list_ rTCPs_ nAllHopsTCPs_ qEffective_RTT_ numHops_ SimStopTime_ qSize_
	
   	set testName_   parking-lot-topo
   	set qType_	XCP
   	set BW_	        30;                 #in Mb/s
   	set BBW_         [expr $BW_ / 2.0]  ; #BW of bottleneck
   	set delay_	10;                 #in ms
   	set qSize_      [expr round([expr ($BW_ / 8.0) * 2.0 * $delay_ * 4.5])];
   	set SimStopTime_	  20
   	set qEffective_RTT_        [expr  20 * $delay_ * 0.001]
   	set nAllHopsTCPs_          10
   	set numHops_               9
   	set BW_list_     " $BW_ $BW_ $BBW_ $BW_ $BW_ $BW_ $BW_ $BW_ $BW_"
   	set qType_list_  " $qType_ $qType_  $qType_ $qType_  $qType_ $qType_ $qType_ $qType_ $qType_"
   	set delay_list_  "$delay_ $delay_ $delay_ $delay_ $delay_ $delay_ $delay_ $delay_ $delay_"
   	set qSize_list_  "$qSize_ $qSize_ $qSize_ $qSize_ $qSize_ $qSize_ $qSize_ $qSize_ $qSize_"
   	set nTCPsPerHop_list_     "1 1 1 1 1 1 1 1 1"
   	set rTCPs_                1; #traverse all of the reverse path

   	$self next
	
}


Test/parking-lot-topo instproc run {} {
	$self instvar ns_ rtg_ numHops_ BW_list_ qType_list_ delay_ delay_list_ qSize_list_ nTCPsPerHop_list_ rTCPs_ nAllHopsTCPs_ qtraces_ qEffective_RTT_ SimStopTime_ qSize_
	
   	global n all_links
	
	#all except the first are lists
   	$self create-string-topology $numHops_ $BW_list_ $delay_list_ $qType_list_ $qSize_list_;
	
   	 #set BW for xcp queue
   	foreach link $all_links {
   		set queue [$link queue]
   		if {[$queue info class] == "Queue/XCP"} {
   			$queue set-link-capacity [[$link set link_] set bandwidth_];  
   		}
   	}
	
	#Create sources: 1) Long TCPs
   	set i 0
   	while { $i < $nAllHopsTCPs_  } {
   		set StartTime     [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $numHops_ * $delay_)] 
   		set src_($i)      [new GeneralSender $ns_ $i $n(0) [set n($numHops_)] "$StartTime TCP/Reno/XCP"]
		
   		[[set src_($i)] set tcp_]  set  window_     [expr $qSize_ * 10]
   		incr i
   	}

   	 #2) jth Hop TCPs; start at j*1000
   	set i 0;
   	while {$i < $numHops_} {
   		set j [expr (1000 * $i) + 1000 ]; 
   		while { $j < [expr [lindex $nTCPsPerHop_list_ $i] + ($i + 1) * 1000]  } {
   			set StartTime     [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $numHops_ * $delay_)] 
   			set src_($j)      [new GeneralSender $ns_ $j [set n($i)] [set n([expr $i+1])] "$StartTime TCP/Reno/XCP"]
   			[[set src_($j)] set tcp_] set window_ [expr $qSize_ * 10]
   			incr j
   		}
   		incr i   	}
	
   	 #3) reverse TCP; ids follow directly allhops TCPs

	
   	set i 0
   	while {$i < $numHops_} {
   		set l 0
   		while { $l < $rTCPs_} {
   			set s [expr $l + $nAllHopsTCPs_ + ( $i * $rTCPs_ ) ] 
   			set StartTime     [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $numHops_ * $delay_)+ 0.0] 
   			set src_($s)      [new GeneralSender $ns_ $s [set n([expr $i + 1])] [set n($i)] "$StartTime TCP/Reno/XCP"]
		
   			[[set src_($s)] set tcp_] set window_ [expr $qSize_ * 10]
   			incr l
   		}
   		incr i
   	}

   	 #Trace Queues
   	set i 0;
   	while { $i < $numHops_ } {
   		set qtype [lindex $qType_list_ $i]
   		global q$i rq$i
   		foreach queue_name "q$i rq$i" {
   			set queue [set "$queue_name"]
   			switch $qtype {
   				"XCP" {
   					set qtrace [open ft_red_[set queue_name].tr w]
   					$queue attach $qtrace
					lappend qtraces_ $qtrace
   				}
   			}
   		}
 		#sample parameters at queue at a given time interval of qeffective_RTT
   		foreach queue_name "q$i" {
   			set queue [set "$queue_name"]
   			$queue queue-sample-everyrtt $qEffective_RTT_
   		}
   		incr i
   	}
	
   	$ns_ at $SimStopTime_ "$self finish"
   	$ns_ run
	
   	set flows "0 1 2 3 4 5 6 7 8"
	#use utilization as validation output
	$self process-parking-lot-data "u" "Utilization" $flows 0.0
	#$self process-parking-lot-data "q" "Average Queue" $flows 0.0


}

proc runtest {arg} {
	global quiet
	set quiet 0
	
	set b [llength $arg]
	if {$b == 1} {
		set test $arg
	} elseif {$b == 2} {
		set test [lindex $arg 0]
		if {[lindex $arg 1] == "QUIET"} {
			set quiet 1
		}
	} else {
		usage
	}
	set t [new Test/$test]
	$t run
}

global argv arg0
runtest $argv
