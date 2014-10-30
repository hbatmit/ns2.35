# db-simple.tcl 
#
# Demonstrates a simple TCP file transfer with DelayBox

# setup ns
remove-all-packet-headers;            # removes all packet headers
add-packet-header IP TCP;             # adds TCP/IP headers
set ns [new Simulator];               # instantiate the simulator

global defaultRNG
$defaultRNG seed 999

# create nodes
set n_src [$ns node]
set db(0) [$ns DelayBox]
set db(1) [$ns DelayBox]
set n_sink [$ns node]

# setup links
$ns duplex-link $db(0) $db(1) 100Mb 1ms DropTail
$ns duplex-link $n_src $db(0) 100Mb 1ms DropTail
$ns duplex-link $n_sink $db(1) 100Mb 1ms DropTail

# trace queues
set qmonf [open "db-simple.trq" w]
$ns trace-queue $db(0) $db(1) $qmonf
$ns trace-queue $db(1) $db(0) $qmonf

set src [new Agent/TCP]
set sink [new Agent/TCPSink]
$src set fid_ 1

# attach agents to nodes
$ns attach-agent $n_src $src
$ns attach-agent $n_sink $sink

# make the connection
$ns connect $src $sink
$sink listen

# create random variables
set recvr_delay [new RandomVariable/Uniform];     # delay 1-20 ms
$recvr_delay set min_ 1 
$recvr_delay set max_ 20
set sender_delay [new RandomVariable/Uniform];    # delay 20-100 ms
$sender_delay set min_ 20
$sender_delay set max_ 100
set recvr_bw [new RandomVariable/Constant];       # bw 100 Mbps
$recvr_bw set val_ 100
set sender_bw [new RandomVariable/Uniform];       # bw 1-20 Mbps
$sender_bw set min_ 1
$sender_bw set max_ 20
set loss_rate [new RandomVariable/Uniform];       # loss 0-1% loss
$loss_rate set min_ 0
$loss_rate set max_ 0.01

# setup rules for DelayBoxes 
$db(0) add-rule [$n_src id] [$n_sink id] $recvr_delay $loss_rate $recvr_bw
$db(1) add-rule [$n_src id] [$n_sink id] $sender_delay $loss_rate $sender_bw

# output delays to files
$db(0) set-delay-file "db-simple-db0.out"
$db(1) set-delay-file "db-simple-db1.out"

# schedule traffic
$ns at 0.5 "$src advance 10000"
$ns at 1000.0 "$ns flush-trace; $db(0) close-delay-file; $db(1) close-delay-file; exit 0"

# start the simulation
$ns run
