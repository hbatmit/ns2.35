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

# Transit traffic to show pause's collateral damage
# another node connected to nqueue
set n_extra [$ns node]
$ns duplex-link $n_extra $nqueue $inputLineRate [expr $RTT/4] DropTail
attach-classifiers $ns $n_extra $nqueue
set M 10

# Transit TCP agents
for {set j 0} {$j < $M} {incr j} {
  set tcp_left_src($j) [new Agent/TCP/FullTcp/Sack]
  set tcp_right_sink($j) [new Agent/TCP/FullTcp/Sack]
  $tcp_right_sink($j) listen
  $ns attach-agent $n(0) $tcp_left_src($j)
  $ns attach-agent $n_extra $tcp_right_sink($j)
  $ns connect $tcp_left_src($j) $tcp_right_sink($j)
}

# Transit FTP agents
for {set j 0} {$j < $M} {incr j} {
  set ftp_extra($j) [new Application/FTP]
  $ftp_extra($j) attach-agent $tcp_left_src($j)
  $ns at 0.0 "$ftp_extra($j) send 10000"
  $ns at 0.0 "$ftp_extra($j) start"
  $ns at $simulationTime "$ftp_extra($j) stop"
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
set traceSamplingInterval 0.0001
set queue_fh [open queue.tr w]
set qfile [$ns monitor-queue $nqueue $nclient $queue_fh $traceSamplingInterval]

set transit_fh [open transit.tr w]
set transit_monitor [$ns monitor-queue $nqueue $n_extra $transit_fh $traceSamplingInterval]

proc startMeasurement {} {
  global qfile transit_monitor startPacketCount_bneck startPacketCount_transit
  set startPacketCount_bneck [$qfile set pdepartures_]
  set startPacketCount_transit [$transit_monitor set pdepartures_]
}

proc stopMeasurement {} {
  global qfile transit_monitor simulationTime startPacketCount_bneck startPacketCount_transit packetSize
  set stopPacketCount_bneck [$qfile set pdepartures_]
  set stopPacketCount_transit [$transit_monitor set pdepartures_]
  puts "Throughput (bneck) = [expr ($stopPacketCount_bneck -$startPacketCount_bneck)/(1024.0*1024*($simulationTime))*$packetSize*8] Mbps"
  puts "Throughput (transit) = [expr ($stopPacketCount_transit-$startPacketCount_transit)/(1024.0*1024*($simulationTime))*$packetSize*8] Mbps"
}

$ns at 0 "startMeasurement"
$ns at $simulationTime "stopMeasurement"
$ns at $simulationTime "finish"
$ns run
