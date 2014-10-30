# test-tmix.tcl
#
# Simulation script to simulate the tmix-ns component
#
# useful variables
set length 10;                         # length of traced simulation (s)
set window 16;                         # max TCP window size in KB
set bw 1000;                           # link speed (Mbps)
set warmup 0;                          # warmup interval (s)
set end $length
set debug 1

set DATADIR "."
set FILE "sample"
set INBOUND "sample-alt.cvec"
set OUTBOUND "sample-alt.cvec";   # same traffic in both directions

#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Setup Simulator
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

remove-all-packet-headers;             # removes all packet headers
add-packet-header IP TCP;              # adds TCP/IP headers
set ns [new Simulator];                # instantiate the Simulator

#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Setup Topology
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

# create nodes
set n(0) [$ns node]
set n(1) [$ns node]
set n(2) [$ns node]
set n(3) [$ns node]

# setup TCP
Agent/TCP/FullTcp set segsize_ 1460;           # set MSS to 1460 bytes
Agent/TCP/FullTcp set nodelay_ true;           # disabling nagle
Agent/TCP/FullTcp set segsperack_ 2;           # delayed ACKs
Agent/TCP/FullTcp set interval_ 0.1;           # 100 ms

#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Setup TmixNode
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

set tmix(0) [new Tmix]
$tmix(0) set-init $n(0);                 # name $n(0) as initiator
$tmix(0) set-acc $n(1);                  # name $n(1) as acceptor
$tmix(0) set-ID 7
$tmix(0) set-cvfile "$INBOUND"
$tmix(0) set-TCP Newreno

set tmix(1) [new Tmix]
$tmix(1) set-init $n(3);                 # name $n(3) as initiator
$tmix(1) set-acc $n(2);                  # name $n(2) as acceptor
$tmix(1) set-ID 8
$tmix(1) set-cvfile "$OUTBOUND"
$tmix(1) set-TCP Newreno

#
# Setup tmixDelayBox
#
set tmixNet(0) [$ns Tmix_DelayBox]
$tmixNet(0) set-cvfile "$INBOUND" [$n(0) id] [$n(1) id]
$tmixNet(0) set-lossless

set tmixNet(1) [$ns Tmix_DelayBox]
$tmixNet(1) set-cvfile "$OUTBOUND" [$n(3) id] [$n(2) id]
$tmixNet(1) set-lossless

# create link
$ns duplex-link $n(0) $tmixNet(0) 1000Mb 0.1ms DropTail
$ns duplex-link $n(2) $tmixNet(0) 1000Mb 0.1ms DropTail
$ns duplex-link $tmixNet(0) $tmixNet(1) 1000Mb 0.1ms DropTail
$ns duplex-link $tmixNet(1) $n(1) 1000Mb 0.1ms DropTail
$ns duplex-link $tmixNet(1) $n(3) 1000Mb 0.1ms DropTail

# set queue buffer sizes (in packets)  (default is 20 packets)
$ns queue-limit $n(0) $tmixNet(0) 500
$ns queue-limit $tmixNet(0) $n(0) 500
$ns queue-limit $n(2) $tmixNet(0) 500
$ns queue-limit $tmixNet(0) $n(2) 500
$ns queue-limit $tmixNet(0) $tmixNet(1) 500
$ns queue-limit $tmixNet(1) $tmixNet(0) 500
$ns queue-limit $tmixNet(1) $n(1) 500
$ns queue-limit $n(1) $tmixNet(1) 500
$ns queue-limit $tmixNet(1) $n(3) 500
$ns queue-limit $n(3) $tmixNet(1) 500

# setup tracing
Trace set show_tcphdr_ 1;  
set qmonf [open "|gzip > tmix-trace.trq.gz" w]
$ns trace-queue $n(0) $tmixNet(0) $qmonf
$ns trace-queue $tmixNet(1) $n(1) $qmonf
$ns trace-queue $tmixNet(0) $n(0) $qmonf
$ns trace-queue $n(1) $tmixNet(1) $qmonf
$ns trace-queue $tmixNet(0) $tmixNet(1) $qmonf
$ns trace-queue $tmixNet(1) $tmixNet(0) $qmonf
$ns trace-queue $n(2) $tmixNet(0) $qmonf
$ns trace-queue $tmixNet(1) $n(3) $qmonf
$ns trace-queue $tmixNet(0) $n(2) $qmonf
$ns trace-queue $n(3) $tmixNet(1) $qmonf

#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Simulation Schedule
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

$ns at 0.0 "$tmix(0) start"
$ns at 0.0 "$tmix(1) start"
$ns at $end "$tmix(0) stop"
$ns at $end "$tmix(1) stop"
$ns at [expr $length + 1] "$ns halt"

#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# Start the Simulation
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

$ns run
