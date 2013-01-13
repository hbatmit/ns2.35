# pm-simple.tcl
#
# Demonstrates the use of PackMime to generate HTTP/1.1 traffi

# useful constants
set CLIENT 0
set SERVER 1

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
$pm set-outfile "pm-simple.dat"

$ns at 0.0 "$pm start"
$ns at 300.0 "$pm stop"

$ns run
