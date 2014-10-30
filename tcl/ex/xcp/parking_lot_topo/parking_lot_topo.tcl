# ------- Test XCP Performance over a a sequence of bottlenecks  ---------------#
#
# Author: Dina Katabi <dk@mit.edu>
# Last Update : 09/09/2002
#
# Descrp: parking lot topo
#         
Agent/TCP set minrto_ 1

 #-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- Utility Functions -*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-#


#--------------- CC Eprimental TOPOLOGY: Control Transfer -------------------------------#
# - This topology is useful for experimenting with the behavior of a 
# connection when its bottleneck moves from one node to another
#         
#    n0 ------------ n1  ....    n[numHops-1]------------n[numHops] 
#                    
proc create-string-topology {numHops BW_list d_list qtype_list qsize_list } {
    global ns 
    
    set numNodes [expr $numHops + 1 ]

    # first sanity check
    if { [llength $BW_list] !=  [llength $d_list] || [llength $qtype_list] !=  [llength $qsize_list]} {
	error "Args sizes don't match with $numHops hops"
	if { [llength $BW_list] != $numHops || $numHops != [llength $qsize_list]} {
	    puts "error in using proc create-string-topology"
	    error "Args sizes don't match with $numHops hops"
	}
    }

	# compute the pipe assuming packet size is 1000 bytes, delays are in msec and BWs are in Mbits
	set forwarddelay 0; 
	global minBW; 
	set minBW [lindex $BW_list 0];
	for { set i 0} { $i < $numHops } { incr i } {
		set forwarddelay [expr $forwarddelay + [lindex $d_list $i]]
		if {$minBW > [lindex $BW_list $i] } { set $minBW [lindex $BW_list $i] }
	}
	set pipe [expr round([expr $minBW /8 * $forwarddelay * 2])]

	for { set i 0} { $i < $numNodes } { incr i } {
		global n$i
		set n$i [$ns node]
	}
    
	for { set i 0} { $i < $numHops } { incr i } {
		set qsize [lindex $qsize_list $i]
		set bw    [lindex $BW_list $i]
		set delay [lindex $d_list $i]
		set qtype [lindex $qtype_list $i]

		puts "$i bandwidth $bw"
		if {$qsize == 0} { set qsize $pipe}
		$ns duplex-link [set n$i] [set n[expr $i +1]] [set bw]Mb [set delay]ms $qtype
		$ns queue-limit [set n$i] [set n[expr $i + 1]] $qsize
		$ns queue-limit [set n[expr $i + 1]] [set n$i] $qsize
		# Give a global handle to the Queues to allow setting the RED paramters
		global q$i;  set  q$i   [ [$ns link [set n$i] [set n[expr $i + 1]]]  queue]
		global rq$i; set  rq$i  [ [$ns link [set n[expr $i + 1]] [set n$i]] queue]
		global l$i;  set  l$i   [$ns link [set n$i] [set n[expr $i + 1]]]
		global rl$i; set  rl$i  [$ns link [set n[expr $i + 1]] [set n$i]]
	}
}


proc set-red-params { qsize } {
	#Queue/RED set thresh_ [expr 0.6 * $qsize]
	#Queue/RED set maxthresh_ [expr 0.8 * $qsize]
	#Queue/RED set q_weight_ 0.001
	Queue/RED set linterm_ 10
	Queue/RED set bytes_ false ;
	Queue/RED set queue_in_bytes_ false ;
	Agent/TCP set old_ecn_ true
	Queue/RED set setbit_     true
}

proc flush-files { files } {
    foreach file $files {
	global "$file"
	if {[info exists $file]} {
	    puts "flush $file"
	    flush [set $file]
	    close [set $file]
	}   
    }
}

#------- Sender Class :
# This is essentially an ftp sender
Class GeneralSender  -superclass Agent 

# otherparams are "startTime TCPclass .."
GeneralSender instproc init { id node rcvrTCP otherparams } {
    global  ns
    $self next
    $self instvar tcp_ id_ ftp_ node_ tcp_rcvr_ tcp_type_
    set id_ $id
    set node_ $node

    if { [llength $otherparams] > 1 } {
	set TCP [lindex $otherparams 1]
    } else { 
	set TCP "TCP/Reno"
    }
    set   tcp_type_ $TCP
    set   tcp_ [new Agent/$TCP]
    $tcp_ set  packetSize_ 1000   
    $tcp_ set  class_  $id
    set   ftp_ [new Source/FTP]
    $ftp_ set agent_ $tcp_
    $ns   attach-agent $node $tcp_
    $ns   connect $tcp_  $rcvrTCP
    set   tcp_rcvr_ $rcvrTCP
    set   startTime [lindex $otherparams 0]
    $ns   at $startTime "$ftp_ start"
    puts  "initialized Sender $id_ at $startTime"
}

GeneralSender instproc trace-xcp parameters {
    $self instvar tcp_ id_ tcpTrace_
    global ftracetcp$id_ 
    set ftracetcp$id_ [open  xcp$id_.tr  w]
    set tcpTrace_ [set ftracetcp$id_]
    $tcp_ attach-trace [set ftracetcp$id_]
    if { -1 < [lsearch $parameters cwnd]  } { $tcp_ tracevar cwnd_ }
    if { -1 < [lsearch $parameters seqno] } { $tcp_ tracevar t_seqno_ }
}



#--- Command line arguments
proc  set-cmd-line-args { list_args } {
    global argv

    set i 0
    foreach a $list_args {
	global $a
	set $a  [lindex $argv $i]
	puts "$a = [set $a]"
	incr i
    }
}

#-------------- Plotting functions -----------#

# plot a xcp traced var
proc plot-xcp { TraceName nXCPs  PlotTime what } {
    exec rm -f  xgraph.tcp
    set f [open xgraph.tcp w]
    puts $f "TitleText: $TraceName"
    puts $f "Device: Postscript"

    foreach i $nXCPs {
	#the TCP traces are flushed when the sources are stopped
	exec rm -f temp.tcp 
        exec touch temp.tcp
	global ftracetcp$i 
	if [info exists ftracetcp$i] { flush [set ftracetcp$i] }
        set result [exec awk -v PlotTime=$PlotTime -v what=$what {
	    {
		if (( $6 == what ) && ($1 > PlotTime)) {
		    print $1, $7 >> "temp.tcp";
		}
	    }
	} xcp$i.tr]
 
	puts "$i : $result"

	puts $f \"$what$i 
	exec cat temp.tcp >@ $f
	puts $f "\n"
	flush $f
    }
    close $f
    exec xgraph  -nl -m  -x time -y $what xgraph.tcp &
    return 
}

# Takes as input the the label on the Y axis, the time it starts plotting, and the trace file tcl var
proc plot-red-queue { TraceName PlotTime traceFile } {
    
    exec rm -f xgraph.red_queue
    exec rm -f temp.q temp.a temp.p temp.avg_enqueued temp.avg_dequeued temp.x temp.y 
    exec touch temp.q temp.a temp.p temp.avg_enqueued temp.avg_dequeued temp.x temp.y 
    
    exec awk -v PT=$PlotTime {
	{
	    if (($1 == "Q" && NF>2) && ($2 > PT)) {
		print $2, $3 >> "temp.q"
	    }
	    else if (($1 == "a" && NF>2) && ($2 > PT)){
		print $2, $3 >> "temp.a"
	    }
	    else if (($1 == "p" && NF>2) && ($2 > PT)){
		print $2, $3 >> "temp.p"
	    }
	}
    }  $traceFile

    set ff [open xgraph.red_queue w]
    puts $ff "TitleText: $TraceName"
    puts $ff "Device: Postscript \n"
    puts $ff \"queue
    exec cat temp.q >@ $ff  
    puts $ff \n\"ave_queue
    exec cat temp.a >@ $ff
    puts $ff \n\"prob_drop
    exec cat temp.p >@ $ff

    close $ff
    exec xgraph  -P -x time -y queue xgraph.red_queue &
}

#-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- Initializing Simulator -*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-#
# BW is in Mbs and delay is in ms

set-cmd-line-args " numHops PostProcess seed qType BW nAllHopsTCPs delay"

set numNodes [expr $numHops + 1 ]
set ns           [new Simulator]
set rtg          [new RNG]
$rtg seed        $seed


#-----Global Variables-----------#
#set  qType  Vq
#set  BW     [expr 10.0 * 1]
#set  delay  50


set tracefd [open out.tr w]
$ns trace-all $tracefd

set  qSize  [expr round([expr ($BW / 8.0) * 2.0 * $delay * 10 ])]
#set  qSize  [expr round([expr ($BW / 8.0) * $delay * 1000 ])]
puts "queue is $qSize" 
set  qEffective_RTT [expr  20 * $delay * 0.001]

#set-red-params $qSize

set BW2                  [expr $BW / 2.0]
puts "BW2 = $BW2"
set BW_list		{}
set qType_list		{}
set delay_list		{}
set qSize_list		{}
set nTCPsPerHop_list	{}
set StartTime_list	{}
for {set i 0} {$i < $numHops} {incr i} {
	lappend		BW_list $BW
	lappend		qType_list $qType
	lappend		delay_list $delay
	lappend		qSize_list $qSize
	lappend		nTCPsPerHop_list 30
	lappend		StartTime_list 0
}
set tracedQueues         ""
set tracedXCPs           "0 1 2 3 4 5 6 7 8"
#set nAllHopsTCPs         30; #num of TCPs crossing all hops
set rTCPs                30; #traverse all of the reverse path

set SimStartTime         0.0
set SimStopTime          20
set PlotTime             0

#---------- Create the simulation --------------------#

# Create topology
create-string-topology $numHops $BW_list $delay_list $qType_list $qSize_list; #all except the first are lists

set i 0;
while { $i < $numHops } {
    set qtype [lindex $qType_list $i]
    set bw [lindex $BW_list $i]
    switch $qtype {
	"RED" { 
	    foreach queue "[set q$i] [set rq$i]" {
		set-red-params $queue $qMinTh $qMaxTh $qWeight $qLinterm
	    }
	}
	"Vq" {
	    foreach link "[set l$i] [set rl$i]" {
		set queue                   [$link queue]
		$queue set ecnlim_          0.98
		$queue set limit_           $qSize
		$queue link_capacity_bits   [[$link set link_] set bandwidth_];
		$queue set queue_in_bytes_  1
		$queue set buflim_          1000
		$queue set alpha_           0.15
	    }
	}
	"REM" {
	    foreach link "[set l$i] [set rl$i]" {
		set queue                [$link queue]
		$queue set reminw_       0.01
		$queue set remgamma_     0.001 
		$queue set remphi_       1.001 
		set cap_in_bits          [[$link set link_] set bandwidth_]
		$queue set remupdtime_   [expr 0.01 / ($cap_in_bits/8000000.0)] 
		$queue set rempktsize_   1000
		$queue set rempbo_       [expr $qSize * 0.33]
	    }
	}
	    "DropTail" { }
	    "XCP" {
		foreach link "[set l$i] [set rl$i]" {
		    set queue                   [$link queue]
		    #set-red-params $queue $qMinTh $qMaxTh $qWeight $qLinterm
			$queue set-link-capacity [[$link set link_] set bandwidth_];
		}
	}
	    "CSFQ" {
		    foreach link "[set l$i]" {
		set queue   [$link queue]
		set ff      0
		while {$ff < $nAllHopsTCPs} {
		    # Second argument is the flow's wait
		    $queue init-flow $ff 1.0 [expr 1000 * $delay * 2 * 4]
		    incr ff
		}
		set j [expr (1000 * $i) + 1000 ]; 
		while { $j < [expr [lindex $nTCPsPerHop_list $i] + ($i + 1) * 1000]  } {
		    $queue init-flow $j 1.0 [expr 1000 * $delay * 2 * 4]
		    incr j
		}
		set kLink [expr 1000 * $delay * 2 * 4]
		set queueSize   [expr  $qSize * 1000 * 8]
		set qsizeThresh [expr  $queueSize * 0.4]
		    $queue init-link 1 $kLink $queueSize $qsizeThresh [[$link set link_] set bandwidth_] 
	    }
		foreach link "[set rl$i]" {
		set queue   [$link queue]
		set ff      $nAllHopsTCPs
		while {$ff < [expr $rTCPs + $nAllHopsTCPs ] } {
		    # Second argument is the flow's wait
		    $queue init-flow $ff 1.0 [expr 1000 * $delay * 2 * 4]
		    incr ff
		}
		set kLink [expr 1000 * $delay * 2 * 4]
		set queueSize   [expr  $qSize * 1000 * 8]
		set qsizeThresh [expr  $queueSize * 0.7]
		$queue init-link 1 $kLink $queueSize $qsizeThresh [[$link set link_] set bandwidth_] 
		}

	}
	    default {}
    }
	incr i
}

# Create sources: 1) Long TCPs
set i 0
while { $i < $nAllHopsTCPs  } {
	set StartTime     [expr [$rtg integer 1000] * 0.001 * (0.01 * $numHops * $delay) + $SimStartTime] 
	if {$qType == "XCP" } {
		set rcvr_TCP      [new Agent/TCPSink/XCPSink]
		$ns attach-agent  [set n$numHops] $rcvr_TCP
		set src$i         [new GeneralSender $i $n0 $rcvr_TCP "$StartTime TCP/Reno/XCP"]
		puts "starttime=$StartTime"
	} else {
		set rcvr_TCP      [new Agent/TCPSink]
		$ns attach-agent  [set n$numHops] $rcvr_TCP
		set src$i         [new GeneralSender $i $n0 $rcvr_TCP "$StartTime TCP/Reno"]
    }
    [[set src$i] set tcp_]  set  window_     [expr $qSize * 10]
    incr i
}

# 2) jth Hop TCPs; start at j*1000
set i 0;
while {$i < $numHops} {
    set j [expr (1000 * $i) + 1000 ]; 
    while { $j < [expr [lindex $nTCPsPerHop_list $i] + ($i + 1) * 1000]  } {
	set StartTime     [expr [lindex $StartTime_list $i]+[$rtg integer 1000] * 0.001 * (0.01 * $numHops * $delay)+ $SimStartTime] 
	if {$qType == "XCP" } {
	    set rcvr_TCP      [new Agent/TCPSink/XCPSink]
		$ns attach-agent  [set  n[expr $i + 1]] $rcvr_TCP
	    set src$j         [new GeneralSender $j [set n$i] $rcvr_TCP "$StartTime TCP/Reno/XCP"]
		puts "starttime=$StartTime"
	} else {
	    set rcvr_TCP      [new Agent/TCPSink]
	    $ns attach-agent  [set  n[expr $i + 1]] $rcvr_TCP
	    set src$j         [new GeneralSender $j [set n$i] $rcvr_TCP "$StartTime TCP/Reno"]
	}
	[[set src$j] set tcp_]  set  window_     [expr $qSize * 10]
	incr j
    }
    incr i
}

# 3) reverse TCP; ids follow directly allhops TCPs


set i 0
while {$i < $numHops} {
    set l 0
    while { $l < $rTCPs} {
	set s [expr $l + $nAllHopsTCPs + ( $i * $rTCPs ) ] 
	#puts "s=$s  rTCPs=$rTCPs  l=$l  All=$nAllHopsTCPs"
	set StartTime     [expr [$rtg integer 1000] * 0.001 * (0.01 * $numHops * $delay)+ 0.0] 
	if {$qType == "XCP" } {
	    set rcvr_TCP      [new Agent/TCPSink/XCPSink]
	    $ns attach-agent  [set  n$i] $rcvr_TCP
		set src$s         [new GeneralSender $s [set  n[expr $i + 1]] $rcvr_TCP "$StartTime TCP/Reno/XCP"]
		puts "starttime=$StartTime"
	} else {
	    set rcvr_TCP      [new Agent/TCPSink]
	    $ns attach-agent  [set  n$i]  $rcvr_TCP
	    set src$s         [new GeneralSender $s [set  n[expr $i + 1]] $rcvr_TCP "$StartTime TCP/Reno"]
	}
	[[set src$s] set tcp_]  set  window_     [expr $qSize * 10]
	incr l
    }
    incr i
}

#---------- Trace --------------------#
# General
#set f_all [open TR/out.tr w]
#$ns trace-queue $n0 $n1 $f_all
#$ns trace-queue $n1 $n2 $f_all
#$ns trace-queue $n2 $n3 $f_all
#$ns trace-queue $n3 $n4 $f_all
#$ns trace-queue $n4 $n5 $f_all
#$ns trace-queue $n5 $n6 $f_all
#$ns trace-queue $n6 $n7 $f_all
#$ns trace-queue $n7 $n8 $f_all
#$ns trace-queue $n8 $n9 $f_all



# Trace sources
foreach i $tracedXCPs {
	[set src$i] trace-xcp "cwnd seqno"
}

# Trace Queues
set i 0;
while { $i < $numHops } {
    set qtype [lindex $qType_list $i]
    foreach queue_name "q$i rq$i" {
	set queue [set "$queue_name"]
	switch $qtype {
	    "DropTail" {}
	    "Vq" {}
	    "REM" {}
	    "CSFQ" {}
	    #"RED/XCP" {}
	    default { #RED and XCP
		global "ft_red_$queue_name"
		    set "ft_red_$queue_name" [open TR/ft_red_[set queue_name].tr w]
		$queue attach       [set ft_red_$queue_name]
		#$queue trace curq_
		#$queue trace ave_
		#$queue trace prob1_
	    }
	}
    }
    
    #The following is done in the Queue Class
    foreach queue_name "q$i" {
	puts "attaching a file to $queue_name $qType"
	set queue [set "$queue_name"]
	$queue queue-sample-everyrtt $qEffective_RTT
	    #global ft_trace_$queue_name
	    #set    ft_trace_$queue_name [open TR/ft_trace_[set queue_name].tr w]
	    #$queue queue-attach   [set ft_trace_$queue_name]
	    #$queue queue-trace-drops  
	    #$queue queue-trace-curq
    }
	### Trace Utilization
	#[set q$i] queue-set-link-capacity [[[$ns link [set n$i] [set n[expr $i +1]]] set link_] set bandwidth_]
	incr i
}
#---------------- Run the simulation ------------------------#

set files "f_all ";set i 0;
while { $i < $numHops } {
    set files "$files ft_trace_q$i ft_trace_rq$i ft_q$i ft_rq$i  ft_red_q$i ft_red_rq$i "
    incr i
}

foreach i $tracedXCPs {
    set file [set src$i tcpTrace_]
    set files "$files $file"
}

proc finish {} {
    global ns 
    if {[info exists f]} {
	$ns flush-trace
	close $f
    }   
    $ns halt
}

$ns at $SimStopTime "finish"
$ns run
flush-files $files
#----------------- Post Processing---------------------------#

if { $PostProcess } {
    #--- Traced Flows
    set TraceName "Flows --$qType-QS$qSize"
    plot-xcp      $TraceName  $tracedXCPs  0.0  "cwnd_"
    plot-xcp      $TraceName  $tracedXCPs  0.0  "t_seqno_"
}



if { $PostProcess } {    
    #---- Queue 1 - Data
    set i 0;
    foreach i $tracedQueues {
	puts "Q$i"
	set TraceName "q$i--QS$qSize-Max$qMaxTh-Min$qMinTh"
	switch $qType {
	    "DropTail" {}
	    "Vq" {
		#plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "vq_len"
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "virtual_cap"
	    }
	    "XCP" {
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "input_traffic_bytes_ BTA_ BTF_"
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "BTA_not_allocated BTF_not_reclaimed"
		plot-red-queue1    $TraceName     0   TR/ft_red_q$i.tr
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "Queue_bytes_"  
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "avg_rtt_ Tq_"  
	    }
	    "RED" {
		plot-red-queue1       $TraceName  0  TR/ft_red_q$i.tr
	    }
	    "CSFQ" {
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "rateAlpha_ rateTotal_"
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "congested_"
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "alpha_"
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "qsizeCrt_"
		plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "label"
	    }
	    default {}
	}
	queue-plot            $TraceName  0  [set ft_trace_q$i]  TR/ft_trace_q$i.tr
	plot-function-of-time $TraceName  0   TR/ft_trace_q$i.tr "u"
	incr i
    }
}

set i 0 
while { $i < $numHops} {
    puts " q$i drops = [[set q$i]  queue-read-drops]"
    puts " r$i drops = [[set rq$i] queue-read-drops]"
    incr i
}
