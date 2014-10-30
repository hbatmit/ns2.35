
# a simple script with 2 node topology to test working of ARQ/HDLC 
# (goBackN and selrepeat) over a satellite link
# using a single TCP flow
# loss model for urban and rural(open) from Lincoln labs paper added  
#
set val(chan)           Channel/Sat     ;#Channel Type
set val(netif)          Phy/Sat         ;# network interface type
set val(mac)            Mac/Sat         ;# MAC type
set val(ifq)            Queue/DropTail  ;# interface queue type
set val(ll)             LL/Sat/HDLC     ;# link layer type
#set val(ll)              LL/Sat        ;# use this if want to simulate LL
                                         # with no HDLC

# two types of eror models used
set val(err)            UrbanComplexMarkovErrorModel
#set val(err)            RuralComplexMarkovErrorModel

set val(bw_up)          10Mb
set val(bw_down)        10Mb
set val(x)		250
set val(y)		250

set durlistA        "22.0 26.0 2.9 2.7" ; # avg ON/OFF for urban model
set durlistB        "27.0 12.0 0.4 0.4"  ;# for rural model


set BW 10; # in Mb/s
set delay 254; # in ms
# Set the queue length (in packets)
#set qlen [expr round([expr ($BW / 8.0) * $delay * 2])]; # set buffer to pipe size

# set sat link bandwidth and delay
Mac set bandwidth_ 10Mb
LL set delay_ 125ms

set val(bdp) [expr 10000/8.0 * 125/100]
LL/Sat/HDLC set window_size_ $val(bdp) 
LL/Sat/HDLC set queue_size_ $val(bdp)
LL/Sat/HDLC set timeout_ 0.26
# this is set later
#LL/Sat/HDLC set max_timeouts_ 5

# set selective repeat on; its GoBackN by default
#LL/Sat/HDLC set selRepeat_ 1

set qlen $val(bdp)
puts "queue len = $qlen"
set val(ifqlen) $qlen


proc start_time {} {return 0; }
# proc start_time {} {return [expr ([eval ns-random] / 2147483647.0) * 0.1]}

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

	set tracefd     [open simple-hdlc.tr w]
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

	set n(101) [$ns_ node]
	$n(101) set-position 42.3 -71.1; # Boston

	$n(101) add-gsl geo $val(ll) $val(ifq) $val(ifqlen) $val(mac) $val(bw_up) \
	    $val(netif) [$n(0) set downlink_] [$n(0) set uplink_]
	
	$n(101) interface-errormodel [$val(err)]
	
	
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
	
	# TCP flow
	Agent/$tcptype set window_ $val(bdp)

 	set tcp [$ns_ create-connection $tcptype $n(0) TCPSink/Sack1 $n(101) 0]
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
	$tcp trace ndatapack_
	$tcp trace nrexmitpack_
	set tracef [open "$file.tcp_trace.$run" "w"]
	$tcp attach $tracef 

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
	

proc plot-ontime {} {
	exec rm -f ontime
	exec touch ontime
	global file tracef
	if [info exists tracef] { flush [set tracef] }
	exec awk -v field=STATE -v off=1 {
		{
			if (($5 == $field) && ($6 == $off)) {
				{ot += $8};
			} 
			{ print ot >> ontime; } 
		}
	} case0.tr
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
puts "run=$run, retries=$retries, model=$model, sources=$sources, duration=$duration, file=$file, qlen=$qlen, tcptype=$tcptype, queuetype=$queuetype, seed=$seed, c_sources=$c_sources"
setup $tcptype $queuetype $sources $qlen $duration $file $seed $c_sources $delay $rate $ecn $run $retries

