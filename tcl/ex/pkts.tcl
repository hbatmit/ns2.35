# pkts.tcl

# This script tries to simulate a scnenario involving very large number 
# of packets on the fly so that we can get an idea of the memory usage 
# due to heavyweight pkt-hdrs in every pkt.
# Dmalloc has been used to measure memory consumption for this 
# experiment. 
# With all pkt-headers (default condition in ns) memory consumption
# is ~140MB. with total pkts on the fly at around 60,000, (using cbr 
# pktsize of 210, linkBW of 1Gb, delay of 100ms),
# memory consmp. ~2KB/pkt.
# With (unused) pkt headers removed, memory usage is ~30Mb and the 
# figure for per pkt comes down to ~500bytes
# which is a considerable gain (about 75% reduction of memory use).


# add/remove packet headers as required

remove-all-packet-headers       ;# removes all except common
add-packet-header IP Message     ;# hdrs reqd for cbr traffic
#add-packet-header Flags IP TCP  ;# hdrs reqd for TCP

#setting CBR rate to 1Gb to match the link BWrate
Application/Traffic/CBR set rate_ 1000Mb


set ns [new Simulator]
set linkBW 1000Mb
set delay 100ms

set n0 [$ns node]
set n1 [$ns node]

set f [open pkts.tr w]
$ns trace-all $f
#set nf [open pkts.nam w]
#$ns namtrace-all $nf

$ns duplex-link $n0 $n1 $linkBW $delay DropTail

#set tcp0 [new Agent/TCP]
set udp0 [new Agent/UDP]
$ns attach-agent $n0 $udp0
#set ftp0 [new Application/FTP]
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0


#set sink0 [new Agent/TCPSink]
set null0 [new Agent/Null]
$ns attach-agent $n1 $null0

$ns connect $udp0 $null0

$ns at 0.2 "$cbr0 start"
$ns at 1.0 "finish"

proc finish {} {
    global ns f nf
    $ns flush-trace
    close $f
    #close $nf

    puts "running nam.."
    exec nam pkts.nam &
    exit 0
}

$ns run
