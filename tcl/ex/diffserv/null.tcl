# null.tcl
# Author: Alexander Sayenko <sayenko@cc.jyu.fi>
# Dates: Mon, 24 Nov 2003
# Notes: A script to show the use of Null plicy

set ns [new Simulator]

set cir  1000000
set cir2 500000

set client  [$ns node]
set client2 [$ns node]
set border [$ns node]
set core [$ns node]
set server [$ns node]

$ns simplex-link $client2 $border 10Mb 2ms DropTail
$ns simplex-link $client $border 10Mb 2ms DropTail
$ns simplex-link $border $core 10Mb 2ms dsRED/edge
$ns simplex-link $core $server 10Mb 2ms dsRED/core

set agent  [new Agent/UDP]
set agent2 [new Agent/UDP]

set sink   [new Agent/Null]
set sink2  [new Agent/Null]

set appl [new Application/Traffic/CBR]
$appl set rate_ [expr $cir+10000]

set appl2 [new Application/Traffic/CBR]
$appl2 set rate_ $cir2

$client attach $agent
$client2 attach $agent2

$server attach $sink
$server attach $sink2

$ns connect $agent $sink
$ns connect $agent2 $sink2

$appl attach-agent $agent
$appl2 attach-agent $agent2

set qBC [[$ns link $border $core] queue]
set qCS [[$ns link $core $server] queue]

# -------------------------------------------------
$qBC set NumQueues_ 1
$qBC setNumPrec 2

$qBC addPolicyEntry [$client id] [$server id] TokenBucket 10 $cir 500
$qBC addPolicerEntry TokenBucket 10 11

$qBC addPolicyEntry [$client2 id] [$server id] Null 20
$qBC addPolicerEntry Null 20

$qBC addPHBEntry 10 0 0
$qBC addPHBEntry 11 0 1
$qBC addPHBEntry 20 0 0

$qBC configQ 0 0 4 10 0.1
$qBC configQ 0 1 2 5 0.5
# -------------------------------------------------

$qCS set NumQueues_ 1
$qCS setNumPrec 2

$qCS addPHBEntry 10 0 0
$qCS addPHBEntry 11 0 1
$qCS addPHBEntry 20 0 0

$qCS configQ 0 0 4 10 0.1
$qCS configQ 0 1 2 5 0.5
# -------------------------------------------------

$ns at 0 "$appl start; $appl2 start"
$ns at 5 "finish"

proc finish {} {

  global ns
  global qBC qCS

  $qBC printStats
  $qBC printPolicyTable
  $qBC printPolicerTable

  $ns halt

}

$ns run
