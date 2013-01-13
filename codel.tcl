# Codel test script v120513

# Run this to run CoDel AQM tests.
# ns codel.tcl f w c {b}Mb s d r
# where:
#	f = # ftps
#	w = # PackMime connections per second
#	c = # CBRs
#	b = bottleneck bandwidth in Mbps
#	s = filesize for ftp, -1 for infinite
#	d = dynamic bandwidth, if non-zero, changes (kind of kludgey)
#		have to set the specific change ratios in this file (below)
#	r = number of "reverse" ftps

set stopTime 300
set ns [new Simulator]

# These are defaults if values not set on command line

set num_ftps 1
set web_rate 0
set revftp 0
set num_cbrs 0
#rate and packetSize set in build_cbr
set bottleneck 3Mb
#for a 10MB ftp
set filesize 10000000
set dynamic_bw 0
set greedy 0

# Parse command line

if {$argc >= 1} {
    set num_ftps [lindex $argv 0]
    if {$argc >= 2} {
        set web_rate [lindex $argv 1]
        if {$argc >= 3} {
            set num_cbrs [lindex $argv 2]
            if {$argc >= 4} {
            	set bottleneck [lindex $argv 3]
	    	if {$argc >= 5} {
			set filesize [lindex $argv 4]
	   	 	if {$argc >= 6} {
				set dynamic_bw [lindex $argv 5]
	   	 		if {$argc >= 7} {
					set revftp [lindex $argv 6]
	    			}
	    		}
        	}
    	     }
	}
    }
}

set bw [bw_parse $bottleneck]
if { $revftp >= 1} {
	set num_revs $revftp
} else {
	set num_revs 0
}
puts "ftps $num_ftps webrate $web_rate cbrs $num_cbrs bw $bw filesize $filesize reverse $num_revs"

# experiment settings
set psize 1500
if { $bw < 1000000} { set psize 500 }
set nominal_rtt [delay_parse 100ms]
set accessdly 20
set bdelay 10 
set realrtt [expr 2*(2*$accessdly + $bdelay)]
puts "accessdly $accessdly bneckdly $bdelay realrtt $realrtt bneckbw $bw"

# CoDel values
# interval to keep min over
set interval [delay_parse 100ms]
# target in ms.
set target [delay_parse 5ms]

global defaultRNG
$defaultRNG seed 0
ns-random 0
#$defaultRNG seed 54321
#ns-random 23145

# ------- config info is all above this line ----------

#bdp in packets, based on the nominal rtt
set bdp [expr round($bw*$nominal_rtt/(8*$psize))]

Trace set show_tcphdr_ 1
set startTime 0.0

#TCP parameters - have to set both for FTPs and PackMime

Agent/TCP set window_ [expr $bdp*16]
Agent/TCP set segsize_ [expr $psize-40]
Agent/TCP set packetSize_ [expr $psize-40]
Agent/TCP set windowInit_ 4
Agent/TCP set segsperack_ 1
Agent/TCP set timestamps_ true
set delack 0.4
Agent/TCP set interval_ $delack

Agent/TCP/FullTcp set window_ [expr $bdp*16]
Agent/TCP/FullTcp set segsize_ [expr $psize-40]
Agent/TCP/FullTcp set packetSize_ [expr $psize-40]
Agent/TCP/FullTcp set windowInit_ 4
Agent/TCP/FullTcp set segsperack_ 1
Agent/TCP/FullTcp set timestamps_ true
Agent/TCP/FullTcp set interval_ $delack


Agent/TCP/Linux instproc done {} {
	global ns filesize
#this doesn't seem to work, had to hack tcp-linux.cc to do repeat ftps
	$self set closed_ 0
#needs to be delayed by at least .3sec to slow start
	puts "[$ns now] TCP/Linux proc done called"
	$ns at [expr [$ns now] + 0.3] "$self send $filesize"
}

# problem is that idle() in tcp.cc never seems to get called...
Application/FTP instproc resume {} {
puts "called resume"
        global filesize
        $self send $filesize
#	$ns at [expr [$ns now] + 0.5] "[$self agent] reset"
	$ns at [expr [$ns now] + 0.5] "[$self agent] send $filesize"
}

Application/FTP instproc fire {} {
        global filesize
        $self instvar maxpkts_
        set maxpkts_ $filesize
	[$self agent] set maxpkts_ $filesize
        $self send $maxpkts_
	puts "fire() FTP"
}

#buffersizes
set buffersize [expr $bdp]
set buffersize1 [expr $bdp*10]

Queue/CoDel set target_ $target
Queue/CoDel set interval_ $interval

#set Flow_id 1

proc build_topology { ns which } {
    # nodes n0 and n1 are the server and client side gateways and
    # the link between them is the congested slow link. n0 -> n1
    # handles all the server to client traffic.
    #
    # if the web_rate is non-zero, node n2 will be the packmime server cloud
    # and node n3 will be the client cloud.
    #
    # num_ftps server nodes and client nodes are created for the ftp sessions.
    # the first client node is n{2+w} and the first server node is n{2+f+w}
    # where 'f' is num_ftps and 'w' is 1 if web_rate>0 and 0 otherwise.
    # servers will be even numbered nodes, clients odd
    # Warning: the numbering here is ridiculously complicated

    global bw bdelay accessdly buffersize buffersize1 filesize node_cnt
    set node_cnt 2

    # congested link
    global n0 n1
    set n0 [$ns node]
    set n1 [$ns node]
    $ns duplex-link $n0 $n1 $bw ${bdelay}ms CoDel
    $ns duplex-link-op $n0 $n1 orient right
    $ns duplex-link-op $n0 $n1 queuePos 0.5
    $ns duplex-link-op $n1 $n0 queuePos 1.5
    $ns queue-limit $n0 $n1 $buffersize
    $ns queue-limit $n1 $n0 $buffersize
    set node_cnt 2

    #dynamic bandwidth
    # these are the multipliers for changing bw, times initial set bw
    # edit these values to get different patterns
    global stopTime dynamic_bw
    array names bw_changes
    set bw_changes(1) 0.1
    set bw_changes(2) 0.01
    set bw_changes(3) 0.5
    set bw_changes(4) 0.01
    set bw_changes(5) 1.0

    puts "bottleneck starts at [[[$ns link $n0 $n1] link] set bandwidth_]bps"
    for {set k 1} {$k <= $dynamic_bw} {incr k 1} {
	set changeTime [expr $k*$stopTime/($dynamic_bw+1)]
	set f $bw_changes($k)
	set newBW [expr $f*$bw]
	puts "change at $changeTime to [expr $newBW/1000000.]Mbps"
	$ns at $changeTime "[[$ns link $n0 $n1] link] set bandwidth_ $newBW"
	$ns at $changeTime "[[$ns link $n1 $n0] link] set bandwidth_ $newBW"
	$ns at $changeTime "puts $newBW"
    }

    set li_10 [[$ns link $n1 $n0] queue]
    set li_01 [[$ns link $n0 $n1] queue]

    set tchan_ [open /tmp/redqvar.tr w]
    $li_01 trace curq_
    $li_01 trace d_exp_
    $li_01 attach $tchan_

    global num_ftps web_rate num_cbrs greedy num_revs
    set linkbw [expr $bw*10]

    set w [expr $web_rate > 0]
    if {$w} {
        global n2 n3
	#server
        set n2 [$ns node]
        $ns duplex-link $n2 $n0 $linkbw ${accessdly}ms DropTail
        $ns queue-limit $n2 $n0 $buffersize1
        $ns queue-limit $n0 $n2 $buffersize1

	#client
        set n3 [$ns node]
        $ns duplex-link $n1 $n3 $linkbw ${accessdly}ms DropTail
        $ns queue-limit $n1 $n3 $buffersize1
        $ns queue-limit $n3 $n1 $buffersize1
	set node_cnt 4
    }
#need to fix the angles if use nam
    for {set k 0} {$k < $num_ftps} {incr k 1} {
        # servers
        set j $node_cnt
        global n$j
        set n$j [$ns node]
	if {$greedy > 0 && $k == 0} {
         $ns duplex-link [set n$j] $n0 $linkbw 1ms DropTail
	} else {
         $ns duplex-link [set n$j] $n0 $linkbw ${accessdly}ms DropTail
	}
        $ns queue-limit [set n$j] $n0 $buffersize1
        $ns queue-limit $n0 [set n$j] $buffersize1
        set angle [expr $num_ftps>1? 0.75+($k-1)*.5/($num_ftps-1) : 1]
        $ns duplex-link-op $n0 [set n$j] orient $angle
	incr node_cnt

        # clients
        set j $node_cnt
        global n$j
        set n$j [$ns node]
        set dly [expr ${accessdly} +($k+1)]
        $ns duplex-link $n1 [set n$j] $linkbw  ${dly}ms  DropTail
        $ns queue-limit $n1 [set n$j] $buffersize1
        $ns queue-limit [set n$j] $n1 $buffersize1
        set angle [expr $num_ftps>1? fmod(2.25-($k-1)*.5/($num_ftps-1), 2) : 0]
        $ns duplex-link-op $n1 [set n$j] orient $angle
	incr node_cnt
    }
    for {set k 0} {$k < $num_cbrs} {incr k 1} {
        # servers
        set j $node_cnt
        global n$j
        set n$j [$ns node]
        $ns duplex-link [set n$j] $n0 $linkbw ${accessdly}ms DropTail
        $ns queue-limit [set n$j] $n0 $buffersize1
        $ns queue-limit $n0 [set n$j] $buffersize1
#        set angle [expr $num_cbrs>1? 0.75+($k-1)*.5/($num_cbrs-1) : 1]
        $ns duplex-link-op $n0 [set n$j] orient $angle
	incr node_cnt

        # clients
        set j $node_cnt
        global n$j
        set n$j [$ns node]
        $ns duplex-link $n1 [set n$j] $linkbw  ${accessdly}ms  DropTail
        $ns queue-limit $n1 [set n$j] $buffersize1
        $ns queue-limit [set n$j] $n1 $buffersize1
#        set angle [expr $num_cbrs>1? fmod(2.25-($k-1)*.5/($num_ftps-1), 2) : 0]
        $ns duplex-link-op $n1 [set n$j] orient $angle
	incr node_cnt
    }
#reverse direction ftps
    for {set k 0} {$k < $num_revs} {incr k 1} {
        # clients
        set j $node_cnt
        global n$j
        set n$j [$ns node]
        $ns duplex-link [set n$j] $n0 $linkbw ${accessdly}ms DropTail
        $ns queue-limit [set n$j] $n0 $buffersize1
        $ns queue-limit $n0 [set n$j] $buffersize1
        set angle [expr $num_ftps>1? 0.75+($k-1)*.5/($num_ftps-1) : 1]
        $ns duplex-link-op $n0 [set n$j] orient $angle
	incr node_cnt

        # servers
        set j $node_cnt
        global n$j
        set n$j [$ns node]
        set dly [expr ($accessdly)*1.1 +($k+1)]
        $ns duplex-link $n1 [set n$j] $linkbw  ${dly}ms  DropTail
        $ns queue-limit $n1 [set n$j] $buffersize1
        $ns queue-limit [set n$j] $n1 $buffersize1
        set angle [expr $num_ftps>1? fmod(2.25-($k-1)*.5/($num_ftps-1), 2) : 0]
        $ns duplex-link-op $n1 [set n$j] orient $angle
	incr node_cnt
    }
}

proc build_cbr {cnd snd startTime timeToStop Flow_id} {
    global ns
    set udp [$ns create-connection UDP $snd LossMonitor $cnd $Flow_id]
    set cbr [new Application/Traffic/CBR]
    $cbr attach-agent $udp
    # change these for different types of CBRs
    $cbr set packetSize_ 100
    $cbr set rate_ 0.064Mb
    $ns at $startTime "$cbr start"
    $ns at $timeToStop "$cbr stop"
}

# cnd is client node, snd is server node
proc build_ftpclient {cnd snd startTime timeToStop Flow_id} {
    
    global ns filesize greedy revftp
    set ctcp [$ns create-connection TCP/Linux $snd TCPSink/Sack1 $cnd $Flow_id]
    $ctcp select_ca cubic
    set ftp [$ctcp attach-app FTP]
    $ftp set enableResume_ true
    $ftp set type_ FTP 

#set up a single infinite ftp with smallest RTT
    if {$greedy > 0 || $filesize < 0} {
     $ns at $startTime "$ftp start"
     set greedy 0
    } else {
     $ns at $startTime "$ftp send $filesize"
    }
    $ns at $timeToStop "$ftp stop"
}

proc build_webs {cnd snd rate startTime timeToStop} {
    set CLIENT 0
    set SERVER 1

    # SETUP PACKMIME
    set pm [new PackMimeHTTP]
    $pm set-TCP Sack
    $pm set-client $cnd
    $pm set-server $snd
    $pm set-rate $rate;                    # new connections per second
    $pm set-http-1.1;                      # use HTTP/1.1

    # create RandomVariables
    set flow_arrive [new RandomVariable/PackMimeHTTPFlowArrive $rate]
    set req_size [new RandomVariable/PackMimeHTTPFileSize $rate $CLIENT]
    set rsp_size [new RandomVariable/PackMimeHTTPFileSize $rate $SERVER]

    # assign RNGs to RandomVariables
    $flow_arrive use-rng [new RNG]
    $req_size use-rng [new RNG]
    $rsp_size use-rng [new RNG]

    # set PackMime variables
    $pm set-flow_arrive $flow_arrive
    $pm set-req_size $req_size
    $pm set-rsp_size $rsp_size

    global ns
    $ns at $startTime "$pm start"
    $ns at $timeToStop "$pm stop"
}

proc uniform {a b} {
	expr $a + (($b- $a) * ([ns-random]*1.0/0x7fffffff))
}

proc finish {} {
        global ns
	$ns halt
        $ns flush-trace
        exit 0
}

# $ns namtrace-all [open out.nam w]
# $ns color 2 blue
# $ns color 3 red
# $ns color 4 yellow
# $ns color 5 green

build_topology $ns CoDel

#$ns trace-queue $n0 $n1 [open out_n0ton1.tr w]
#set fname f${num_ftps}w${web_rate}b${bottleneck}.tr
set fname f.tr
puts $fname
$ns trace-queue $n0 $n1 [open /tmp/$fname w]
#reverse direction
#$ns trace-queue $n1 $n0 [open /tmp/$fname w]

set node_cnt 2
if {$web_rate > 0} {
        build_webs $n3 $n2 $web_rate 0 $stopTime
	set node_cnt 4
}

for {set k 1} {$k <= $num_ftps} {incr k 1} {
    set j $node_cnt
    incr node_cnt
    set i $node_cnt
    build_ftpclient [set n$i] [set n$j]  \
 		$startTime $stopTime $i
# 		[expr 1.0*($k-1)] $stopTime $i
# 		[expr $startTime+($k-1)*[uniform 0.0 2.0]] $stopTime $i
    incr node_cnt
}

for {set k 1} {$k <= $num_cbrs} {incr k 1} {
    set j $node_cnt
    incr node_cnt
    set i $node_cnt
    build_cbr [set n$i] [set n$j]  \
 		[expr $startTime+($k-1)*[uniform 0.0 2.0]] $stopTime $i
    incr node_cnt
}

#for reverse direction, give client smaller number
for {set k 1} {$k <= $num_revs} {incr k 1} {
    set j $node_cnt
    incr node_cnt
    set i $node_cnt
    build_ftpclient [set n$j] [set n$i] $startTime $stopTime $j
    incr node_cnt
}

$ns at [expr $stopTime ] "finish"
$ns run
exit 0
