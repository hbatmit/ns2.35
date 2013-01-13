
# case2b.tcl

set val(chan)           Channel/Sat                 ;#Channel Type
set val(netif)          Phy/Sat            ;# network interface type
set val(mac)            Mac/Sat                 ;# MAC type
set val(ifq)            Queue/DropTail  ;# interface queue type
#set val(ifq)            Queue/XCP
set val(ll)             LL/Sat/HDLC                    ;# link layer type
#set val(ll)              LL/Sat

set val(err)            MarkovErrorModel
#set val(err)            ComplexMarkovErrorModel
#set val(err)             UniformErrorProc

set val(bw_up)          10Mb
set val(bw_down)        10Mb
set val(x)		250
set val(y)		250

set durlistA        "11.2 4.0"      ;# unblocked and blocked state duration avg value
set durlistB        "27.0 12.0 0.4 0.4"  ;# for complexmarkoverror model


set BW 10; # in Mb/s
set delay 254; # in ms
# Set the queue length (in packets)
#set qlen [expr round([expr ($BW / 8.0) * $delay * 2])]; # set buffer to pipe size

# set sat link bandwidth and delay
Mac set bandwidth_ 10Mb
#LL set delay_ 125ms
LL set delay_ 250ms

set val(bdp) [expr 10000/8.0 * 125/100]
LL/Sat/HDLC set window_size_ $val(bdp) 
LL/Sat/HDLC set timeout_ 0.26
LL/Sat/HDLC set max_timeouts_ 2

# set selective repeat on; its GoBackN by default
#LL/Sat/HDLC set selRepeat_ 1

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

proc ComplexMarkovErrorModel {} {
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


proc setup {tcptype queuetype sources qlen duration file seed c_sources delay rate ecn} {
	global val traces ns_ tracefd

	# Initialize Global Variables
	ns-random $seed

	set ns_		[new Simulator]

	set tracefd     [open case2c.tr w]
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
	
	$n(100) interface-errormodel [$val(err)]  ;#lossy uplink
	$n(101) interface-errormodel [$val(err)]  ;#ditto downlink
	
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
	
	# UDP
#	set udp [new Agent/UDP]
 	# $ns_ attach-agent $n(100) $udp
#  	set cbr [new Application/Traffic/CBR]
#  	$cbr set packetSize_ 512
#  	#$cbr set interval_ 3.75ms
#  	$cbr set rate_ 50Kb
#  	#$cbr set maxpkts_ 2000
#  	$cbr attach-agent $udp

#  	set null [new Agent/Null]
#  	$ns_ attach-agent $n(101) $null

#  	$ns_ connect $udp $null
#  	$ns_ at 0.1 "$cbr start"

	Agent/$tcptype set window_ $val(bdp)
# 	# TCP
 	set tcp [$ns_ create-connection $tcptype $n(100) TCPSink $n(101) 0]
 	set ftp [new Application/FTP]
 	$ftp attach-agent $tcp
 	$ns_ at 0.0 "$ftp start"


	# tracing for sat links
	$ns_ trace-all-satlinks $tracefd

	
	# setup routing
	set satrouteobject_ [new SatRouteObject]
	$satrouteobject_ compute_routes

	# trace for tcp parameters
	$tcp trace cwnd_
	$tcp trace t_seqno_
	$tcp attach [open "$file.tcp_trace.1" "w"]

	# trace queue
	#$ns_ trace-queue $n(0) $n(101) [open "$file.queue1" w]

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
if { [llength $argv]>0} then { set sources [lindex $argv 0]}\
else {set sources 1}
if { [llength $argv]>1} then { set duration [lindex $argv 1]}\
else {set duration 50}
if { [llength $argv]>2} then { set file [lindex $argv 2]}\
else {set file "case2c"}
if { [llength $argv]>3} then { set qlen [lindex $argv 3]}\
else {set qlen 50 }
if { [llength $argv]>4} then { set tcptype [lindex $argv 4]}\
else {set tcptype "TCP/Newreno"}
if { [llength $argv]>5} then { set queuetype [lindex $argv 5]}\
else {set queuetype "RED"}
if { [llength $argv]>6} then { set seed [lindex $argv 6]}\
else {set seed 0}
if { [llength $argv]>7} then { set c_sources [lindex $argv 7]}\
else {set  c_sources 0}
if { [llength $argv]>8} then { set delay [lindex $argv 8]}\
else {set delay "10ms"}
if { [llength $argv]>9} then { set rate [lindex $argv 9]}\
else {set rate "100k"}
if { [llength $argv]>10} then { set ecn [lindex $argv 10]}\
else {set ecn 1 }

# Start the sim
puts "sources=$sources, duration=$duration, file=$file, qlen=$qlen, tcptype=$tcptype, queuetype=$queuetype, seec=$seed, c_sources=$c_sources"
setup $tcptype $queuetype $sources $qlen $duration $file $seed $c_sources $delay $rate $ecn

