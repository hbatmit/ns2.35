# test for memory usage for 1000 node hier topology using session sim

set ns [new SessionSim]
$ns set-address-format hierarchical
source ./hts1000.tcl
set linkBW 5Mb
global n ns

$ns namtrace-all [open session-hier-1000.nam w]

create-hier-topology $linkBW

set udp [new Agent/UDP]
$ns attach-agent $n(500) $udp
set cbr [new Application/Traffic/CBR]
$cbr attach-agent $udp
$udp set dst_ 0x8002
$ns create-session $n(500) $udp

set rcvr0 [new Agent/LossMonitor]
set rcvr1 [new Agent/LossMonitor]
set rcvr2 [new Agent/LossMonitor]

$ns attach-agent $n(0) $rcvr0
$ns attach-agent $n(1) $rcvr1
$ns attach-agent $n(2) $rcvr2

$ns at 0.2 "$n(0) join-group $rcvr0 0x8002"
$ns at 0.2 "$n(1) join-group $rcvr1 0x8002"
$ns at 0.2 "$n(2) join-group $rcvr2 0x8002"

$ns at 1.0 "$cbr start"

$ns at 3.0 "finish"
proc finish {} {
    global ns
    $ns flush-trace
    puts "running nam..."
    exec nam out.nam &
    exit 0
}

$ns run
