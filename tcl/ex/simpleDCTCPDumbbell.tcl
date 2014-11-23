set ns [new Simulator]

set N 8
set enable_pause 0
set K 65
set RTT 0.0001

set simulationTime 1.0

assert [expr $simulationTime < 1.5]
# Sequence number wraps around otherwise

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
    [$n1 entry] set enable_pause_ 1

    set bwd_queue [[$ns link $n2 $n1] queue]
    $bwd_queue attach-classifier [$n2 entry]
    [$n2 entry] set enable_pause_ 1
}

##### Topology ###########
set inputLineRate 10Gb

for {set i 0} {$i < $N} {incr i} {
    set n($i) [$ns node]
}

set tor_node [$ns node]

# pause instrumentation and queue monitors
if {$enable_pause == 1} {
    puts "Pause enabled"
    Queue set limit_ 10000000
} else {
    puts "Pause disabled"
    Queue set limit_ 1000
}

for {set i 0} {$i < $N} {incr i} {
    $ns duplex-link $n($i) $tor_node $inputLineRate [expr $RTT/4] DropTail
    if {$enable_pause == 1} {
        attach-classifiers $ns $n($i) $tor_node
    }
    set traceSamplingInterval 0.0001
    set queue_fh [open "/dev/null" w]
    set qmon($i) [$ns monitor-queue $tor_node $n($i) $queue_fh $traceSamplingInterval]
}

## Full mesh of TCP connections
for {set i 0} {$i < $N} {incr i} {
  for {set j 0} {$j < $N} {incr j} {
    if  {$i != $j} {
      set tcp($i,$j) [new Agent/TCP/FullTcp/Sack]
      set sink($i,$j) [new Agent/TCP/FullTcp/Sack]
      $sink($i,$j) listen

      $ns attach-agent $n($i) $tcp($i,$j)
      $ns attach-agent $n($j) $sink($i,$j)

      $ns connect $tcp($i,$j) $sink($i,$j)
    }
  }
}

#### Application: long-running FTP #####

for {set i 0} {$i < $N} {incr i} {
  for {set j 0} {$j < $N} {incr j} {
    if  {$i != $j} {
      set ftp($i,$j) [new Application/FTP]
      $ftp($i,$j) attach-agent $tcp($i,$j)
      $ns at 0.0 "$ftp($i,$j) start"
      $ns at [expr $simulationTime] "$ftp($i,$j) stop"
    }
  }
}

### Cleanup procedure ###
proc finish {} {
        global ns queue_fh
        $ns flush-trace
	close $queue_fh
	exit 0
}

### Measure throughput ###
proc startMeasurement {} {
  global qmon startByteCount N
  for {set i 0} {$i < $N} {incr i} {
    set startByteCount($i) [$qmon($i) set bdepartures_]
  }
}

proc stopMeasurement {} {
  global qmon simulationTime startByteCount N
  for {set i 0} {$i < $N} {incr i} {
    set stopByteCount($i) [$qmon($i) set bdepartures_]
    puts "Throughput($i) = [expr ($stopByteCount($i) - $startByteCount($i))/(1000000 * ($simulationTime - 0.5))*8] Mbits/s"
  }
}

$ns at 0.5 "startMeasurement"
$ns at $simulationTime "stopMeasurement"
$ns at $simulationTime "finish"
$ns run
