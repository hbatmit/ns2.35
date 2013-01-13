
# scenario consists of an on-off exp src.
#
# S 10ms RTT             D
#  o------o------o-------o
#    1G   r1 1G  r2  1G    






#------- Sender Class :
# This is essentially an ftp sender
Class GeneralSender  -superclass Agent 

# otherparams are "startTime TCPclass traffic-type.."
GeneralSender instproc init { id srcnode dstnode otherparams } {
    global  ns n
    $self next
    $self instvar tcp_ id_ ftp_ snode_ dnode_ tcp_rcvr_ tcp_type_
    set id_ $id
    
    if { [llength $otherparams] > 1 } {
	set TCP [lindex $otherparams 1]
    } else { 
	set TCP "TCP/Reno"
    }
    if [string match {TCP/Reno/XCP*} $TCP] {
	set TCPSINK "TCPSink/XCPSink"
    } else {
	set TCPSINK "TCPSink"
    }
    puts "$TCP\n$TCPSINK\n"

    if { [llength $otherparams] > 2 } {
	set traffic_type [lindex $otherparams 2]
    } else {
	set traffic_type FTP
    }
    puts "$traffic_type\n"
    
    set   tcp_type_ $TCP
    set   tcp_ [new Agent/$TCP]
    set   tcp_rcvr_ [new Agent/$TCPSINK]
    $tcp_ set  packetSize_ 1000   
    $tcp_ set  class_  $id

    switch -exact $traffic_type {
	FTP {
	    set traf_ [new Application/FTP]
	    $traf_ attach-agent $tcp_
	}
	EXP {
	    
	    set traf_ [new Application/Traffic/Exponential]
	    $traf_ set packetSize_ 1000
	    $traf_ set burst_time_ 500ms
	    $traf_ set idle_time_ 2000ms
	    $traf_ set rate_ 10Mb
	    $traf_ attach-agent $tcp_
	}
	default {
	    puts "unsupported traffic\n"
	    exit 1
	}
    }
    $ns   attach-agent $srcnode $tcp_
    $ns   attach-agent $dstnode $tcp_rcvr_
    $ns   connect $tcp_  $tcp_rcvr_
    set   startTime [lindex $otherparams 0]
    $ns   at $startTime "$traf_ start"
    
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
    if { -1 < [lsearch $parameters ackno] } { $tcp_ tracevar ack_ }
    if { -1 < [lsearch $parameters rtt]  } { $tcp_ tracevar rtt_ }
    if { -1 < [lsearch $parameters ssthresh]  } { $tcp_ tracevar ssthresh_ }
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


proc plot-xcp { tracevar filename PlotTime} {
    global tracedXCPs
    foreach i $tracedXCPs {
	exec rm -f temp.$filename$i
	exec touch temp.$filename$i
	
	set result [exec awk -v PlotTime=$PlotTime -v what=$tracevar -v file=temp.$filename$i {
	    {
		if (( $6 == what ) && ($1 > PlotTime)) {
		    print $1, $7 >> file ;
		}  
	    }
	    
	} xcp$i.tr]

	#exec gnuplot "temp.$tracevar$i" &
	incr i
    }
}

proc plot-red {varname filename PlotTime} {
    
    exec rm -f temp.$filename
    exec touch temp.$filename
    
    set result [exec awk -v PlotTime=$PlotTime -v what=$varname -v file=temp.$filename {
	{
	    if (( $1 == what ) && ($2 > PlotTime)) {
		print $2, $3 >> file ;
	    }  
	}
    } ft_red_Bottleneck.tr]
}

proc MarkovErrorModel {lossylink} {
	set avglist {11.2 4.0}
	set errmodel [new ErrorModel/TwoStateMarkov {11.2 4.0} time]
	
	$errmodel drop-target [new Agent/Null] 
	$lossylink errormodule $errmodel
}

proc ComplexMarkovErrModel {lossylink} {
	set avglist {27.0 12.0 0.4 0.4}
	set errmodel [new ErrorModel/ComplexTwoStateMarkov $avglist time]
	
	$errmodel drop-target [new Agent/Null] 
	$lossylink errormodule $errmodel
}

#-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- Initializing Simulator -*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-#
# BW is in Mbs and delay is in ms


set seed   472904
set qType  XCP
#set qSize   100000
set nn 3    ;# number of sources
set BW 20
set delay 10
# define avg duration of blocked (state 1) and unblocked (state 0) states
#set bstate 4.0
#set ubstate 11.2
set bstate 1.0
set ubstate 50.0

set qSize  [expr round([expr ($BW / 8.0) * 4 * $delay * 1.0])];#set buffer to the pipe size

set ns          [new Simulator]
$ns             use-scheduler Heap
set rtg         [new RNG]
$rtg seed       $seed

#set f_all [open b2.tr w]
#$ns trace-all $f_all


# create topology
for {set i 0} {$i < $nn} {incr i} {
    set n($i) [$ns node]
    set n([expr $i+30]) [$ns node]
}
set n(b1) [$ns node]
set n(b2) [$ns node]

for {set i 0} {$i < $nn} {incr i} {
    $ns simplex-link $n($i) $n(b1) [set BW]Mb [set delay]ms $qType
    $ns queue-limit $n($i) $n(b1) $qSize
    $ns simplex-link $n(b1) $n($i) [set BW]Mb [set delay]ms $qType
}

$ns duplex-link $n(b1) $n(b2) [set BW]Mb [set delay]ms $qType
$ns queue-limit $n(b1) $n(b2) $qSize

for {set i 0} {$i < $nn} {incr i} {
    $ns duplex-link $n(b2) $n([expr $i+30]) [set BW]Mb [set delay]ms $qType
    $ns queue-limit $n(b2) $n([expr $i+30]) $qSize
}


set tracedXCPs       "0 1 2"
set SimStopTime      60
set PlotTime         0

# Give a global handle to the Bottleneck Queue to allow 
# setting the RED paramters
global Bottleneck rBottleneck
set  Bottleneck  [[$ns link $n(b1) $n(b2)] queue] 
set  rBottleneck [[$ns link $n(b2) $n(b1)] queue]
global l rl
set  l  [$ns link $n(b1) $n(b2)]  
set  rl [$ns link $n(b2) $n(b1)]

global all_links all_queues
set all_links "$l $rl"
set all_queues ""
set i 0
while {$i < $nn} {
    global q$i rq$i l$i rl$i
    
    set  q$i   [[$ns link $n($i) $n(b1)] queue] 
    set  rq$i  [[$ns link $n(b1) $n($i)] queue]
    set  l$i    [$ns link $n($i) $n(b1)]
    set  rl$i   [$ns link $n(b1) $n($i)]
    set all_links "$all_links [set l$i] [set rl$i] "
    set all_queues "$all_queues [set q$i]"
    incr i
}
set i 30
while {$i < [expr 30+$nn]} {
    global q$i rq$i l$i rl$i
    
    set  q$i   [[$ns link $n(b2) $n($i)] queue] 
    set  rq$i  [[$ns link $n($i) $n(b2)] queue]
    set  l$i    [$ns link $n(b2) $n($i)]
    set  rl$i   [$ns link $n($i) $n(b2)]
    set all_links "$all_links [set l$i] [set rl$i] "
    set all_queues "$all_queues [set q$i]"
    incr i
}


#---------- Create the simulation --------------------#

foreach link $all_links {
    set queue [$link queue]
    if {[$queue info class] == "Queue/XCP"} {
	$queue set-link-capacity [[$link set link_] set bandwidth_];  
    }
}


# Create sources:


for {set i 0} {$i < $nn} {incr i} {
	global src$i
	set st [expr [$rtg integer 1000] * 0.001 + $i * 0.01]
	puts "st=$st\n"
	set src$i  [new GeneralSender $i $n($i) $n([expr $i+30]) "$st TCP/Reno/XCP"]
}

for {set i 0} {$i < $nn} {incr i} {
    [[set src$i] set tcp_]  set  packetSize_ [expr 100 * ($i +10)]
    [[set src$i] set tcp_]  set  window_     [expr $qSize]
}

# add lossy link that models a 2 state markov model
set lossylink [$ns link $n(0) $n(b1)]
set durlist {$ubstate $bstate}
MarkovErrorModel $lossylink ;#$durlist


#---------- Trace --------------------#

# Trace sources
#foreach i $tracedXCPs {
	#[set src$i] trace-xcp "cwnd seqno"
#}

# Trace Queues
foreach queue_name "Bottleneck rBottleneck q0" {
	set queue [set "$queue_name"]
	if {[$queue info class] == "Queue/XCP"} {
		global "ft_red_$queue_name"
		set "ft_red_$queue_name" [open ft_red_[set queue_name].tr w]
		$queue attach [set ft_red_$queue_name]

	}
}


#---------------- Run the simulation ------------------------#
proc flush-files {} {
	set files "f_all ft_red_Bottleneck ft_red_rBottleneck"
	global tracedXCPs
	foreach file $files {
		global $file
		if {[info exists $file]} {
			flush [set $file]
			close [set $file]
		}   
	}
	foreach i $tracedXCPs {
		global src$i
		set file [set src$i tcpTrace_]
		if {[info exists $file]} {
			flush [set $file]
			close [set $file]
		}   
	}
}

proc finish {} {
    global ns 

    if {[info exists f]} {
	$ns flush-trace
	close $f
    }   
	flush-files
	$ns halt
}

$ns at $SimStopTime "finish"
$ns run
#flush-files

#------------ Post Processing ---------------#
set PostProcess 1
if { $PostProcess } {
    #--- Traced TCPs
    plot-xcp "throughput" thru 0.0
    plot-xcp "t_seqno_" seq 0.0 
    #plot-xcp "cwnd_" cwnd 0.0

    #plot-red "avg_rtt_" rtt 0.0
    plot-red "u" IBW 0.0
    plot-red "d" drops 0.0
    plot-red "q" qlen 0.0
    #plot-red "high_rtt" high_rtt 0.0
    #plot-red "Qsize" qsize 0.0
    #plot-red "Qavg" qavg 0.0
    #plot-red "H_feedback" fb 0.0
}
