#set stopTime 120
set stopTime 10
set ns [new Simulator]

if {[llength $argv] > 0} {
	set num_ftps [lindex $argv 0]
} else {
	set num_ftps 1
}
#set num_ftps [lindex $argv 0]


if {[llength $argv] > 1} {
	set num_webs [lindex $argv 1]
} else {
	set num_webs 0
}
if {[llength $argv] > 2} {
	set bottleneck [lindex $argv 2]
} else {
	set bottleneck 1.5Mb
}


#set random_seed  [lindex $argv 3]
set random_seed 0
set rtt 100ms

# ------- config info is all above this line ----------

set rtt [$ns delay_parse $rtt]
set bw [$ns bw_parse $bottleneck]
if {$bw <= 128000} {
	set psize 552
} else {
	set psize 1500
}
set bdp [expr round($bw*$rtt/(8*$psize))]
if {$bdp < 5} {
	set bdp 5
}

# allow lots of nodes (more than 128)
#$ns set-address-format expanded

Trace set show_tcphdr_ 1
set startTime 0.0

#TCP parameters

Agent/TCP set window_ [expr $bdp*4]
set win_size 1
Agent/TCP set windowInit_ $win_size
set segsize [expr $psize-40]
set segperack 2
set delack 0.4

set lastsample 0
set client_addr 0
set no_of_inlines 3
#set filesize [expr $stopTime*$bw/8/2]
set filesize 10000000000


#buffersizes
set buffersize [expr $bdp*2]
set buffersize1 [expr $bdp*2]

#thresholds

set minthresh [expr round(0.3*$bdp)]
set maxthresh [expr round($minthresh + 1.0*$bdp)]

set sampling_interval [expr $psize*8/$bw]
# set sampling_interval 0.007

set si_per_rtt [expr $rtt/$sampling_interval]
for {set ptwo 2} {$ptwo < $si_per_rtt} {set ptwo [expr $ptwo*2]} { }
set QWT [expr 1./$ptwo]


puts "minthresh $minthresh  maxthresh $maxthresh  QWT $QWT  sample $sampling_interval"

#set Flow_id 1
ns-random $random_seed;
puts $random_seed;
#ns-random 0

proc build_topology { ns which } {
	global bw rtt bdp buffersize buffersize1 maxthresh minthresh QWT \
	       sampling_interval

	# congested link
	global n0 n1
	set n0 [$ns node]
	set n1 [$ns node]
	set bdelay [expr $rtt/2. - [$ns delay_parse 25ms]]
        # $ns duplex-link $n0 $n1 $bw $bdelay RED/New
        $ns duplex-link $n0 $n1 $bw $bdelay RED
	$ns duplex-link-op $n0 $n1 orient right
	$ns duplex-link-op $n1 $n0 queuePos 0.5
	$ns queue-limit $n0 $n1 $buffersize
	$ns queue-limit $n1 $n0 $buffersize

	set li_10 [[$ns link $n1 $n0] queue]
	$li_10 set maxthresh_ $maxthresh
	$li_10 set thresh_ $minthresh
	$li_10 set q_weight_ $QWT

#	$li_10 sampling_interval $sampling_interval
#	$li_10 cancel_thresh 0
#	$li_10 drop_interval $buffersize

	# $li_10 set queue-in-bytes_ true
	# $li_10 set bytes_ true
	# $li_10 set mean_pktsize_ 1500

	set li_01 [[$ns link $n0 $n1] queue]
	$li_01 set maxthresh_ $maxthresh
	$li_01 set thresh_ $minthresh
	$li_01 set q_weight_ $QWT
	# $li_01 set queue-in-bytes_ true
	# $li_01 set bytes_ true
	# $li_01 set mean_pktsize_ 1500

	global num_ftps num_webs
	set n [expr $num_ftps+$num_webs]
	for {set k 1} {$k <= $n} {incr k 1} {
	    # clients
	    set j [expr ($k+1)]
	    set dly [expr ($k)]
	    set dly 1
	    global n$j
	    set n$j [$ns node]
	    set linkbw [expr $bw*10]
	    $ns duplex-link [set n$j] $n0 $linkbw ${dly}ms DropTail
	    $ns queue-limit [set n$j] $n0 $buffersize1
	    $ns queue-limit $n0 [set n$j] $buffersize1
	    set angle [expr $n>1? 0.75+($k-1)*.5/($n-1) : 1]
	    $ns duplex-link-op $n0 [set n$j] orient $angle
	    # servers
	    set j [expr $k+$n+1]
	    global n$j
	    set n$j [$ns node]
	    $ns duplex-link $n1 [set n$j] $linkbw  25ms  DropTail
	    $ns queue-limit $n1 [set n$j] $buffersize1
	    $ns queue-limit [set n$j] $n1 $buffersize1
	    set angle [expr $n>1? fmod(2.25-($k-1)*.5/($n-1), 2) : 0]
	    $ns duplex-link-op $n1 [set n$j] orient $angle
	}
}


proc build_ftpclient {cnd snd sftp startTime timeToStop Flow_id} {
    
    global ns
    global stopTime
    set cli [get_ftpclient]
    set ctcp [get_fulltcp]
    $ctcp attach-application $cli
    #set cli [$ctcp attach-app FTP]
    $ctcp set fid_ $Flow_id
    $cli tcp $ctcp
    $ns attach-agent $cnd $ctcp

    set stcp [get_fulltcp]
    $stcp attach-application $sftp
    $stcp set fid_ $Flow_id
    $ns attach-agent $snd $stcp

    $ns connect $ctcp $stcp
    #$ctcp set dst_ [$stcp set addr_]
    $stcp listen
    $ns at $startTime "$cli start"
    $ns at $timeToStop "$cli stop"
    global ftplist
    global ftplist
    set ftplist($ctcp) $stcp
    return $cli
}




proc get_ftpclient {} {
    global client_addr
    set cli [new Agent/BayTcpApp/FtpClient]
    #set cli [new Agent/TcpApp/FtpClient]
    $cli set addr_ [incr client_addr]
    return $cli
}


proc get_fulltcp {} {
    global segperack segsize delack
    set atcp [new Agent/TCP/BayFullTcp]
    #set atcp [new Agent/TCP/FullTcp]
    $atcp set segsperack_ $segperack
    $atcp set segsize_ $segsize
    $atcp set interval_ $delack
    return $atcp
}
    

proc uniform {a b} {
	expr $a + (($b- $a) * ([ns-random]*1.0/0x7fffffff))
}

proc finish {} {
        global ns PERL
	$ns halt
        $ns flush-trace
    set wrap [expr 90 * 1000 + 40]
    set file BayFullTCP
    exec $PERL ../../bin/set_flow_id -s bay-out.tr | \
	    $PERL ../../bin/getrc -e -s 1 -d 0 | \
	    $PERL ../../bin/raw2xg -v -s 0.01 -m $wrap -t $file > temp.rands
    exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
        exit 0
}
$ns trace-all [open bay-out.tr w]
$ns namtrace-all [open bay-out.nam w]
$ns color 2 blue
$ns color 3 red
$ns color 4 yellow
$ns color 5 green

#build_topology $ns RED/New
build_topology $ns RED

set fname f${num_ftps}w${num_webs}b${bottleneck}.tr
#set fname /dev/fd/1
$ns trace-queue $n1 $n0 [open $fname w]

set nn [expr $num_ftps+$num_webs]
for {set k 1} {$k <= $num_ftps} {incr k 1} {
    set j [expr $k+1]
    set i [expr $j+$nn]
    set sftp [new Agent/BayTcpApp/FtpServer]
    #set sftp [new Agent/TcpApp/FtpServer]
    $sftp file_size $filesize
    build_ftpclient [set n$j] [set n$i]  $sftp \
    		[expr ($k-1)*[uniform 0.0 2.0]] 225 $j
}


$ns at [expr $stopTime ] "finish"
$ns run
exit 0












