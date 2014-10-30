# pm-delaybox.tcl
#
# Demonstrates the use of PackMime with DelayBox

# useful constants
set CLIENT 0
set SERVER 1

set rate 32
set run 2
set stoptime 25

# setup simulator
remove-all-packet-headers;            # removes all packet headers
add-packet-header IP TCP;             # adds TCP/IP headers
set ns_ [new Simulator];              # instantiate the simulator
$ns_ use-scheduler Heap;              # use the Heap scheduler

#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Setup network links and nodes
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

# build end nodes
set client [$ns_ node]
set server [$ns_ node]

# build delayboxes and links
set db_(0) [$ns_ DelayBox]
set db_(1) [$ns_ DelayBox]
$ns_ duplex-link $db_(0) $db_(1) 10Mb 1ms DropTail
$ns_ duplex-link $client $db_(0) 10Mb 1ms DropTail
$ns_ duplex-link $server $db_(1) 10Mb 1ms DropTail

# trace queues
Trace set show_tcphdr_ 1
set db_qmonf [open "pm-delaybox-db.trq" w]
$ns_ trace-queue $db_(0) $db_(1) $db_qmonf
$ns_ trace-queue $db_(1) $db_(0) $db_qmonf

set cli_qmonf [open "pm-delaybox-cli.trq" w]
$ns_ trace-queue $client $db_(0) $cli_qmonf
$ns_ trace-queue $db_(0) $client $cli_qmonf

set srv_qmonf [open "pm-delaybox-srv.trq" w]
$ns_ trace-queue $server $db_(1) $srv_qmonf
$ns_ trace-queue $db_(1) $server $srv_qmonf

# print drops
$db_(0) set-debug 1
$db_(1) set-debug 1

#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#  Setup PackMime Traffic
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

# set rate per PackMime
set rate_ $rate

# create PackMime and assign clients and server clouds
set pm_ [new PackMimeHTTP]
$pm_ set-client $client
$pm_ set-server $server
$pm_ set-TCP Newreno

# create RNGs (appropriate RNG seeds will be assigned automatically)
for {set i 0} {$i < 5} {incr i} {
    set rng_($i) [new RNG]
    # advance the seeds for different experiment runs
    for {set j 1} {$j < $run} {incr j} {
	$rng_($i) next-substream
    }
}

# create random variables
set req_size [new RandomVariable/PackMimeHTTPFileSize $rate_ $CLIENT]
set rsp_size [new RandomVariable/PackMimeHTTPFileSize $rate_ $SERVER]
set client_delay [new RandomVariable/PackMimeHTTPXmit $rate_ $CLIENT]
set server_delay [new RandomVariable/PackMimeHTTPXmit $rate_ $SERVER]
set client_bw [new RandomVariable/Constant];      # bw 100 Mbps
$client_bw set val_ 100
set server_bw [new RandomVariable/Uniform];      # bw 1-20 Mbps
$server_bw set min_ 1
$server_bw set max_ 20
set flow_arrive [new RandomVariable/PackMimeHTTPFlowArrive $rate_]
set loss_rate [new RandomVariable/Uniform];    # loss at each DB
$loss_rate set min_ 0
$loss_rate set max_ 0.001
set zero_loss_rate [new RandomVariable/Constant];
$zero_loss_rate set val_ 0

# assign RNGs to RandomVariables
$req_size use-rng $rng_(0)
$rsp_size use-rng $rng_(1)
$client_delay use-rng $rng_(2)
$server_delay use-rng $rng_(2)
$client_bw use-rng $rng_(3)
$server_bw use-rng $rng_(3)
$flow_arrive use-rng $rng_(4)

# setup rules for DelayBoxes
$db_(0) add-rule [$client id] [$server id] $client_delay $loss_rate \
    $client_bw
$db_(0) add-rule [$server id] [$client id] $client_delay $zero_loss_rate \
    $client_bw
$db_(1) add-rule [$client id] [$server id] $server_delay $loss_rate \
    $server_bw
$db_(1) add-rule [$server id] [$client id] $server_delay $zero_loss_rate \
    $server_bw

# output delays to files
$db_(0) set-delay-file "pm-delaybox-db0.out"
$db_(1) set-delay-file "pm-delaybox-db1.out"

# set PackMime variables
$pm_ set-rate $rate_
$pm_ set-req_size $req_size
$pm_ set-rsp_size $rsp_size
$pm_ set-flow_arrive $flow_arrive
# record HTTP statistics
$pm_ set-outfile "pm-delaybox.dat"

#::::::::::::::::::
# Schedule Traffic 
#::::::::::::::::::

$ns_ at 0.0 "$pm_ start"
$ns_ at $stoptime "$pm_ stop; $db_(0) close-delay-file; $db_(1) close-delay-file; exit 0"

# start the simulation
$ns_ run
