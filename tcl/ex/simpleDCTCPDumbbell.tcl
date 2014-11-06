set ns [new Simulator]

set N 8
set B 250
set K 65
set RTT 0.0001

set simulationTime 1.0

##### Transport defaults, like packet size ######
set packetSize 1460
Agent/TCP set packetSize_ $packetSize
Agent/TCP/FullTcp set segsize_ $packetSize

## Turn on DCTCP ##
set sourceAlg DropTail
source configs/dctcp-defaults.tcl

##### Topology ###########
set lineRate 10Gb
set inputLineRate 11Gb

for {set i 0} {$i < $N} {incr i} {
    set n($i) [$ns node]
}

set nqueue [$ns node]
set nclient [$ns node]

for {set i 0} {$i < $N} {incr i} {
    $ns duplex-link $n($i) $nqueue $inputLineRate [expr $RTT/4] DropTail
}

$ns simplex-link $nqueue $nclient $lineRate [expr $RTT/4] $sourceAlg
$ns simplex-link $nclient $nqueue $lineRate [expr $RTT/4] DropTail
$ns queue-limit $nqueue $nclient $B

for {set i 0} {$i < $N} {incr i} {
    set tcp($i) [new Agent/TCP/FullTcp/Sack]
    set sink($i) [new Agent/TCP/FullTcp/Sack]
    $sink($i) listen

    $ns attach-agent $n($i) $tcp($i)
    $ns attach-agent $nclient $sink($i)
    
    $ns connect $tcp($i) $sink($i)
}

#### Application: long-running FTP #####

for {set i 0} {$i < $N} {incr i} {
    set ftp($i) [new Application/FTP]
    $ftp($i) attach-agent $tcp($i)
}

for {set i 0} {$i < $N} {incr i} {
    $ns at 0.0 "$ftp($i) send 10000"
    $ns at [expr 0.1 + $simulationTime * $i / ($N + 0.0001)] "$ftp($i) start"
    $ns at [expr $simulationTime] "$ftp($i) stop"
}

$ns run
