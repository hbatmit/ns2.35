set ns [new Simulator]
$ns abstract-tcp

set f [open simple-fsm.nam w]
$ns namtrace-all $f

set n(0) [$ns node]
set n(1) [$ns node]

$ns duplex-link $n(0) $n(1) 5Mb 10ms DropTail
$ns duplex-link-op $n(0) $n(1) queuePos 0.5
$ns duplex-link-op $n(0) $n(1) orient right

set tcp [new Agent/AbsTCP/RenoAck]
$tcp set class_ 1
set sink [new Agent/AbsTCPSink]
$ns attach-agent $n(0) $tcp
$ns attach-agent $n(1) $sink
$ns connect $tcp $sink
set ftp [new Application/FTP]
$ftp attach-agent $tcp

$ns at 0.05 "$ftp producemore 31"
$ns at 10 "finish"

proc finish {} {
    global ns f
    $ns flush-trace
    close $f

    puts "running nam..."
    exec nam simple-fsm.nam &
    exit 0
}


$ns run
