source ../lib/ns-queue.tcl
set ns [new Simulator]

$ns color 0 blue
$ns color 1 red
$ns color 2 white

set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

$ns duplex-link $n1 $n2 500kb 2ms DropTail
$ns duplex-link $n2 $n3 1Mb 10ms DropTail

$ns duplex-link-op $n1 $n2 orient right-down
$ns duplex-link-op $n2 $n3 orient right

$ns duplex-link-op $n2 $n3 queuePos 0.5

set udp1 [new Agent/UDP]
$ns attach-agent $n1 $udp1
set cbr1 [new Application/Traffic/CBR]
$cbr1 set packetSize_ 1024
$cbr1 attach-agent $udp1

set null1 [new Agent/Null]
$ns attach-agent $n3 $null1

$ns connect $udp1 $null1

$ns at 0.0 "$cbr1 start"

set tcp [new Agent/TCP]
$tcp set class_ 1
$tcp set packetSize_ 1024
set sink [new Agent/TCPSink]
$ns attach-agent $n1 $tcp
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent_ $tcp
$ns at 2.0 "$ftp start"

$ns at 20.0 "finish"

proc do_nam {} {
	puts "running nam..."
	exec nam out.nam &
}

proc finish {} {
	global ns f nf
	close $f
	close $nf
	$ns flush-trace
	do_nam
	exit 0
}

$ns monitor-queue $n1 $n2 [$ns get-ns-traceall]
set l12 [$ns link $n1 $n2]
$l12 set qBytesEstimate_ 0
$l12 set qPktsEstimate_ 0
set queueSampleInterval 0.5
$ns at [$ns now] "$l12 queue-sample-timeout"

$ns run
