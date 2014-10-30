# pm-end-pairs.tcl
#
# Demonstrates the use of PackMime and using the number
# of completed HTTP request-response pairs as a stopping criteria 
# for the simulation

# useful constants
set CLIENT 0
set SERVER 1

set goal_pairs 50

remove-all-packet-headers;             # removes all packet headers
add-packet-header IP TCP;              # adds TCP/IP headers
set ns [new Simulator];                # instantiate the Simulator
$ns use-scheduler Heap;                # use the Heap scheduler

# SETUP TOPOLOGY
# create nodes
set n(0) [$ns node]
set n(1) [$ns node]
# create link
$ns duplex-link $n(0) $n(1) 10Mb 0ms DropTail

# SETUP PACKMIME
set rate 15
set pm [new PackMimeHTTP]
$pm set-client $n(0);                  # name $n(0) as client
$pm set-server $n(1);                  # name $n(1) as server
$pm set-rate $rate;                    # new connections per second
$pm set-http-1.1;                      # use HTTP/1.1

# SETUP PACKMIME RANDOM VARIABLES

# create RNGs (appropriate RNG seeds are assigned automatically)
set flowRNG [new RNG]
set reqsizeRNG [new RNG]
set rspsizeRNG [new RNG]

# create RandomVariables
set flow_arrive [new RandomVariable/PackMimeHTTPFlowArrive $rate]
set req_size [new RandomVariable/PackMimeHTTPFileSize $rate $CLIENT]
set rsp_size [new RandomVariable/PackMimeHTTPFileSize $rate $SERVER]

# assign RNGs to RandomVariables
$flow_arrive use-rng $flowRNG
$req_size use-rng $reqsizeRNG
$rsp_size use-rng $rspsizeRNG

# set PackMime variables
$pm set-flow_arrive $flow_arrive
$pm set-req_size $req_size
$pm set-rsp_size $rsp_size

# record HTTP statistics
$pm set-outfile "pm-end-pairs.dat"

$ns at 0.0 "$pm start"

# force quit after completing desired number of pairs
for {set i 0} {$i < 10000} {incr i} {
    $ns at $i "check_pairs"
}

proc check_pairs {} {
    global pm goal_pairs

    set cur_pairs [$pm get-pairs]
    if {$cur_pairs >= $goal_pairs} {
	#
	# NOTE: "$pm stop" will not stop PackMime immediately.  Those 
	#       connections that have already started will be allowed to 
	#       complete, so pm-end-pairs.dat will likely contain slightly 
	#       more than "goal_pairs" entries.
	#
	puts stderr "Completed $goal_pairs HTTP pairs.  Finishing..."
	$pm stop
	exit 0
    }

}

$ns run
