source asym/util.tcl

set ns [new Simulator]

$ns color 0 blue
$ns color 1 red
$ns color 2 white

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

puts n0=[$n0 id]
puts n1=[$n1 id]
puts n2=[$n2 id]
puts n3=[$n3 id]

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

Queue set limit_ 5

#$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 10Mb 2ms DropTail
$ns duplex-link $n1 $n2 10Mb 2ms DropTail
$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n2 orient right-up
$ns duplex-link-op $n1 $n2 orient right-down
$ns duplex-link-op $n2 $n3 orient right

$ns duplex-link-op $n2 $n3 queuePos 0.5

set udp0 [new Agent/UDP]
$ns attach-agent $n0 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0

set udp1 [new Agent/UDP]
$ns attach-agent $n3 $udp1
$udp1 set class_ 1
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $udp1

set null0 [new Agent/Null]
$ns attach-agent $n3 $null0

set null1 [new Agent/Null]
$ns attach-agent $n1 $null1

$ns connect $udp0 $null0
$ns connect $udp1 $null1

$ns at 0.0 "$cbr0 start"
$ns at 0.1 "$cbr1 start"

for {set i 0} {$i < 2} {incr i} {
	set tcp($i) [new Agent/TCP/Int]
#	set tcp($i) [new Agent/TCP]
	$tcp($i) set cls [expr $i+1]
	$tcp($i) set rightEdge 0
	$tcp($i) set shift_ 8
	$tcp($i) set mask_ 0x000000ff
	$tcp($i) set uniqTS_ 1
	$tcp($i) set winMult_ 0.5
	$tcp($i) set winInc_ 1

	set sink [new Agent/TCPSink]
	$ns attach-agent $n0 $tcp($i)
	$ns attach-agent $n3 $sink
	$ns connect $tcp($i) $sink
	
	set ftp($i) [new Application/FTP]
	$ftp($i) attach-agent $tcp($i)
	$ns at 0.$i "$ftp($i) start"
}

for {set i 2} {$i < 4} {incr i} {
	set tcp($i) [new Agent/TCP/Int]
#	set tcp($i) [new Agent/TCP]
	$tcp($i) set cls [expr $i+1]
	$tcp($i) set rightEdge 0
	$tcp($i) set shift_ 8
	$tcp($i) set mask_ 0xffffffff
	$tcp($i) set uniqTS_ 1
	$tcp($i) set winMult_ 0.5
	$tcp($i) set winInc_ 1

	set sink [new Agent/TCPSink]
	$ns attach-agent $n0 $tcp($i)
	$ns attach-agent $n2 $sink
	$ns connect $tcp($i) $sink
	
	set ftp($i) [new Application/FTP]
	$ftp($i) attach-agent $tcp($i)
	$ns at 0.$i "$ftp($i) start"
}


#puts [$cbr0 set packetSize_]
#puts [$cbr0 set interval_]

$ns at 5.0 "finish"

proc finish {} {
	puts "running nam..."
	exec nam out.nam &

	puts "done"
	exit 0
}

$ns run

