set ns [new Simulator]

set N 8
set B 2500000
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

# Procedure to attach classifier to queues
# for nodes n1 and n2
proc attach-classifiers {ns n1 n2} {
    set fwd_queue [[$ns link $n1 $n2] queue]
    $fwd_queue attach-classifier [$n1 entry]

    set bwd_queue [[$ns link $n2 $n1] queue]
    $bwd_queue attach-classifier [$n2 entry]
}

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
    attach-classifiers $ns $n($i) $nqueue
}

$ns simplex-link $nqueue $nclient $lineRate [expr $RTT/4] $sourceAlg
$ns simplex-link $nclient $nqueue $lineRate [expr $RTT/4] DropTail
attach-classifiers $ns $nqueue $nclient

$ns queue-limit $nqueue $nclient $B
set traceSamplingInterval 0.0001
set queue_fh [open queue.tr w]

for {set i 0} {$i < $N} {incr i} {
    set tcp($i) [new Agent/TCP/FullTcp/Sack]
    set sink($i) [new Agent/TCP/FullTcp/Sack]
    $sink($i) listen

    $ns attach-agent $n($i) $tcp($i)
    $ns attach-agent $nclient $sink($i)

    $tcp($i) set fid_ [expr $i]
    $sink($i) set fid_ [expr $i]

    $ns connect $tcp($i) $sink($i)
}

#### Application: long-running FTP #####

for {set i 0} {$i < $N} {incr i} {
    set ftp($i) [new Application/FTP]
    $ftp($i) attach-agent $tcp($i)
}

for {set i 0} {$i < $N} {incr i} {
    $ns at 0.0 "$ftp($i) send 10000"
    $ns at 0.0 "$ftp($i) start"
    $ns at [expr $simulationTime] "$ftp($i) stop"
}

### Cleanup procedure ###
proc finish {} {
        global ns queue_fh
        $ns flush-trace
	close $queue_fh
	exit 0
}

### Measure throughput ###
set qfile [$ns monitor-queue $nqueue $nclient $queue_fh $traceSamplingInterval]

proc startMeasurement {} {
  global qfile startPacketCount
  set startPacketCount [$qfile set pdepartures_]
}

proc stopMeasurement {} {
  global qfile simulationTime startPacketCount packetSize
  set stopPacketCount [$qfile set pdepartures_]
  puts "Throughput = [expr ($stopPacketCount-$startPacketCount)/(1024.0*1024*($simulationTime))*$packetSize*8] Mbps"
}

$ns at 0 "startMeasurement"
$ns at $simulationTime "stopMeasurement"
$ns at $simulationTime "finish"
$ns run
