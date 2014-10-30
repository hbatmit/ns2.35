# A simple example for hierarchical routing, by generating
# topology by hand (of 10 nodes)

set ns [new Simulator]
$ns set-address-format hierarchical

$ns namtrace-all [open hier-out-a.nam w]
$ns trace-all [open hier-out-a.tr w]

AddrParams set domain_num_ 2
lappend cluster_num 2 2
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel 2 3 2 3
AddrParams set nodes_num_ $eilastlevel

set naddr {0.0.0 0.0.1 0.1.0 0.1.1 0.1.2 1.0.0 1.0.1 1.1.0 1.1.1 1.1.2}

for {set i 0} {$i < 10} {incr i} {
     set n($i) [$ns node [lindex $naddr $i]]
}

$ns duplex-link $n(0) $n(1) 5Mb 2ms DropTail
$ns duplex-link $n(1) $n(2) 5Mb 2ms DropTail
$ns duplex-link $n(2) $n(3) 5Mb 2ms DropTail
$ns duplex-link $n(2) $n(4) 5Mb 2ms DropTail
$ns duplex-link $n(1) $n(5) 5Mb 2ms DropTail
$ns duplex-link $n(5) $n(6) 5Mb 2ms DropTail
$ns duplex-link $n(6) $n(7) 5Mb 2ms DropTail
$ns duplex-link $n(7) $n(8) 5Mb 2ms DropTail
$ns duplex-link $n(8) $n(9) 5Mb 2ms DropTail

set udp0 [new Agent/UDP]
$ns attach-agent $n(0) $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0

set udp1 [new Agent/UDP]
$udp1 set class_ 1
$ns attach-agent $n(2) $udp1
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $udp1

set null0 [new Agent/Null]
$ns attach-agent $n(8) $null0

set null1 [new Agent/Null]
$ns attach-agent $n(6) $null1

$ns connect $udp0 $null0
$ns connect $udp1 $null1

$ns at 1.0 "$cbr0 start"
$ns at 1.1 "$cbr1 start"

puts [$cbr0 set packetSize_]
puts [$cbr0 set interval_]



$ns at 1.0 {$n(2) label [$n(2) set rtsize_]}
$ns at 1.0 {$n(4) label [$n(4) set rtsize_]}
$ns at 1.0 {$n(6) label [$n(6) set rtsize_]}
$ns at 1.0 {$n(8) label [$n(8) set rtsize_]}

$ns at 3.5 "finish"

proc finish {} {
	global ns 
	$ns flush-trace
	##puts "running nam..."
	##exec nam hier-out-a.nam &
	exit 0
}

$ns run


