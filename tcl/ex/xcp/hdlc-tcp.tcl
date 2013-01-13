
# intermittent satellite links running with/without HDLC in LL

set val(chan)           Channel/Sat                 ;#Channel Type
set val(netif)          Phy/Sat            ;# network interface type
set val(mac)            Mac/Sat                 ;# MAC type
set val(ifq)            Queue/DropTail  ;# interface queue type
set val(ll)             LL/Sat/HDLC                    ;# link layer type
#set val(ll)              LL/Sat

# two types of eror models used
#set val(err)            UrbanComplexMarkovErrorModel
set val(err)            RuralComplexMarkovErrorModel

set val(bw_up)          10Mb
set val(bw_down)        10Mb
set val(x)		250
set val(y)		250

set durlistA        "22.0 26.0 2.9 2.7"  ;# unblocked and blocked state duration avg value for urban model
set durlistB        "27.0 12.0 0.4 0.4"  ;# for rurral model


set BW 10; # in Mb/s
set delay 254; # in ms
# Set the queue length (in packets)
#set qlen [expr round([expr ($BW / 8.0) * $delay * 2])]; # set buffer to pipe size

# set sat link bandwidth and delay
Mac set bandwidth_ 10Mb
LL set delay_ 125ms

set val(bdp) [expr 10000/8.0 * 125/100]
LL/Sat/HDLC set window_size_ $val(bdp) 
LL/Sat/HDLC set timeout_ 0.26
LL/Sat/HDLC set max_timeouts_ 0


# set selective repeat on; its GoBackN by default
LL/Sat/HDLC set selRepeat_ 1

set qlen $val(bdp)
puts "queue len = $qlen"
set val(ifqlen) $qlen


proc start_time {} {return 0; }
# proc start_time {} {return [expr ([eval ns-random] / 2147483647.0) * 0.1]}


proc UniformErrorProc {} {
	global val
	set	errObj [new ErrorModel]
	$errObj unit bit
	#$errObj FECstrength $val(FECstrength) 
	#$errObj datapktsize 512
	#$errObj cntrlpktsize 80
	return $errObj
}

proc MarkovErrorModel {} {
	global durlistA
	
	puts [lindex $durlistA 0]
	puts [lindex $durlistA 1]

	set errmodel [new ErrorModel/TwoStateMarkov $durlistA time]
	
	#$errmodel drop-target [new Agent/Null]
	return $errmodel
}
proc UrbanComplexMarkovErrorModel {} {
	global durlistA
	set errmodel [new ErrorModel/ComplexTwoStateMarkov $durlistA time]
	
	$errmodel drop-target [new Agent/Null] 
	return $errmodel
}

proc RuralComplexMarkovErrorModel {} {
	global durlistB
	set errmodel [new ErrorModel/ComplexTwoStateMarkov $durlistB time]
	
	$errmodel drop-target [new Agent/Null] 
	return $errmodel
}

proc stop {} {
    global ns_ tracefd
    $ns_ flush-trace
    close $tracefd
}


proc setup {tcptype queuetype sources qlen duration file seed c_sources delay rate ecn run retries} {
	global val traces ns_ tracefd

	if {$retries == "NULL"} {
		set val(ll) LL/Sat
	} else {
		set val(ll) LL/Sat/HDLC
		LL/Sat/HDLC set max_timeouts_ $retries
	}

	# Initialize Global Variables
	ns-random $seed

	set ns_		[new Simulator]

	set tracefd     [open out.tr w]
	$ns_ trace-all $tracefd
	$ns_ eventtrace-all

	$ns_ rtproto Static
	
	# Create sat n(0)
	
	# configure node, please note the change below.

	$ns_ node-config -satNodeType geo \
	    -llType $val(ll) \
	    -ifqType $val(ifq) \
	    -ifqLen $val(ifqlen) \
	    -macType $val(mac) \
	    -phyType $val(netif) \
	    -channelType $val(chan) \
	    -downlinkBW $val(bw_down) \
	    -wiredRouting ON

	set n(0) [$ns_ node]
	$n(0) set-position -95

	$ns_ node-config -satNodeType terminal

	set n(100) [$ns_ node]
	$n(100) set-position 42.3 -71.1; # Boston

	set n(101) [$ns_ node]
	$n(101) set-position 37.9 -122.3; # Berkeley

	$n(100) add-gsl geo $val(ll) $val(ifq) $val(ifqlen) $val(mac) $val(bw_up) \
	    $val(netif) [$n(0) set downlink_] [$n(0) set uplink_]
	
	$n(101) add-gsl geo $val(ll) $val(ifq) $val(ifqlen) $val(mac) $val(bw_up) \
	    $val(netif) [$n(0) set downlink_] [$n(0) set uplink_]
	
	$n(101) interface-errormodel [$val(err)] ;# intermittent downlink
	
	$ns_ unset satNodeType_
	
	# create regular routers
	set n(99) [$ns_ node]    ;#....node 3
	set n(102) [$ns_ node]   ;#....node 4
	
	# determine the actual queutype and save the one asked for 
	# assign queueing parameters -- simulate random drop with RED
	if { [string match RandomDrop* $queuetype ] } then {
		set queuetype [join \
				   [concat "RED" \
					[string range $queuetype [string length "RandomDrop"] end ]]\
				   "" ]
		Queue/$queuetype set thresh_ [expr 2* $qlen]
		Queue/$queuetype set maxthresh_ [expr 2* $qlen]
		# added for ECN comparison
		Queue/$queuetype set setbit_ $ecn
	} elseif { [string match RED* $queuetype ] } then {
		#added by nirav
		Queue/$queuetype set bytes_ false
		Queue/$queuetype set queue_in_bytes_ false
		# Queue/$queuetype set q_weight_ = -1
		# Queue/$queuetype set max_p = 1
		Queue/$queuetype set gentle_ true
		#added by nirav
		#Queue/$queuetype set thresh_ [expr $qlen * 0.5]
		#Queue/$queuetype set maxthresh_ [expr $qlen * 0.75]
		Queue/$queuetype set thresh_ 0
		Queue/$queuetype set maxthresh_ 0
		# added for ECN comparison
		Queue/$queuetype set setbit_ $ecn
	}
	
	$ns_ duplex-link $n(99) $n(100) 10Mb 1ms $queuetype
	$ns_ duplex-link $n(101) $n(102) 10Mb 1ms $queuetype

# 	# TCP	
	Agent/$tcptype set window_ $val(bdp)


# create nodes for sources and c_sources
	puts "sources = $sources"
	puts "c_sources = $c_sources"
	
	for { set i 1 } {$i <= [expr 2* [expr $sources+$c_sources]]} {incr i} {
	set n($i) [$ns_ node]
	}
	
	# connect sources and sinks to routers at 100 Mb/s
	for {set i 1} {$i<=$sources} {incr i} {
		$ns_ duplex-link $n($i) $n(99) 100Mb 1ms $queuetype
		$ns_ duplex-link $n(102) $n([expr $i+$sources+$c_sources]) 100Mb 1ms $queuetype
	}

	# connect cross traffic sources and sinks
	for {set i [expr $sources+1]} {$i<= [expr $c_sources+$sources]} {incr i} {
		$ns_ duplex-link $n($i) $n(99) 10Mb 2ms $queuetype
		$ns_ duplex-link $n(100) $n([expr $i+$sources+$c_sources]) 10Mb 2ms $queuetype
	}
	
	# Setup traffic flow between nodes
	# Create source and sink Agents and connect them
	for {set i 1} {$i<= [expr $sources+$c_sources]} {incr i} {
	# Create the sender agent
		set T($i) [new Agent/$tcptype]
		# If Ecn is on set it in the sender
		if { $ecn } { $T($i) set ecn_ 1 }
		set hstcpflag 0
		if {$hstcpflag==1} {
			puts "creating high-speed connection"
			set tcp($i) [$ns_ create-highspeed-connection TCP/Reno $n($i) TCPSink $n([expr $i+$c_sources+$sources]) $i]
			if { $ecn } { $tcp($i) set ecn_ 1 }
		} else {
			puts "cerating normal $T($i) connection"
			set tcp($i) [$ns_ create-connection $tcptype $n($i) TCPSink $n([expr $i+$c_sources+$sources]) $i]
			if { $ecn } { $tcp($i) set ecn_ 1 }
		}

		if {$hstcpflag==1} {
			[set tcp($i)] set windowOption_ 8
			[set tcp($i)] set low_window_ 12.5
			[set tcp($i)] set high_window_ 12500
			[set tcp($i)] set high_p_ 0.0000096
			[set tcp($i)] set high_decrease_ 0.1
			puts "windowOption = [$tcp($i) set windowOption_]"
			puts "low_window = [$tcp($i) set low_window_]"
			puts "high_window = [$tcp($i) set high_window_]"
			puts "high_p_ = [$tcp($i) set high_p_]"
			puts "high_decrease = [$tcp($i) set high_decrease_]"
		}
		set F($i) [new Application/FTP]
		$F($i) attach-agent $tcp($i)
		$ns_ at [expr [start_time]+[expr ($i-1)*2]] "$F($i) start"
	}

	# tracing for sat links
	$ns_ trace-all-satlinks $tracefd
	
	# trace for tcp parameters
	if { [lsearch $traces "TCP"] != -1 } {
		for {set i 1} {$i<= [expr $sources+$c_sources]} {incr i} {
			$tcp($i) trace cwnd_
			$tcp($i) trace t_seqno_
			$tcp($i) attach [open "$file.tcp_trace.$i" "w"]
		}
	}
	
	# set queue size to pipe size
	$ns_ queue-limit $n(99) $n(100) $val(ifqlen)
	$ns_ queue-limit $n(101) $n(102) $val(ifqlen)
	
	# trace queue
	if { [lsearch $traces "queue"] != -1 } {
		$ns_ trace-queue $n(99) $n(100) [open "$file.queue1" "w"]
	}
	
	# setup routing
	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	# Stop the sim at $duration

	puts "duration = $duration\n"
	$ns_ at $duration "stop"
	$ns_ at $duration "puts \"NS EXITING...\" ; $ns_ halt"
	
	# start up.
	puts "Starting Simulation..."
	$ns_ run
}
	
# traces are the set of info to be put out
set traces [ list "TCP" "queue" ]

# this boring code sets the given parameters from teh command line in order or
# assigns defaults if they're not given.
if { [llength $argv]>0} then { set run [lindex $argv 0]}\
else {set run 1 }
if { [llength $argv]>1} then { set retries [lindex $argv 1]}\
else {set retries 3 }
if { [llength $argv]>2} then { set model [lindex $argv 2]}\
else {set model "urban" }
if { [llength $argv]>3} then { set sources [lindex $argv 3]}\
else {set sources 1}
if { [llength $argv]>4} then { set duration [lindex $argv 4]}\
else {set duration 300}
if { [llength $argv]>5} then { set file [lindex $argv 5]}\
else {set file "case0"}
if { [llength $argv]>6} then { set qlen [lindex $argv 6]}\
else {set qlen 50 }
if { [llength $argv]>7} then { set tcptype [lindex $argv 7]}\
else {set tcptype "TCP/Newreno"}
if { [llength $argv]>8} then { set queuetype [lindex $argv 8]}\
else {set queuetype "RED"}
if { [llength $argv]>9} then { set seed [lindex $argv 9]}\
else {set seed 12345}
if { [llength $argv]>10} then { set c_sources [lindex $argv 10]}\
else {set  c_sources 0}
if { [llength $argv]>11} then { set delay [lindex $argv 11]}\
else {set delay "10ms"}
if { [llength $argv]>12} then { set rate [lindex $argv 12]}\
else {set rate "100k"}
if { [llength $argv]>13} then { set ecn [lindex $argv 13]}\
else {set ecn 1 }


# Start the sim
puts "run=$run, retries=$retries, sources=$sources, duration=$duration, file=$file, qlen=$qlen, tcptype=$tcptype, queuetype=$queuetype, seed=$seed, c_sources=$c_sources"
setup $tcptype $queuetype $sources $qlen $duration $file $seed $c_sources $delay $rate $ecn $run $retries

