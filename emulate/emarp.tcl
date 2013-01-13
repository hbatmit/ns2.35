set ns [new Simulator]
$ns use-scheduler RealTime
set x [new ArpAgent]
$x config
set a1 131.243.1.39
set a2 131.243.1.80
set a3 131.243.1.81
$ns at 0.0 "$x resolve $a1"
$ns at 1.0 "$x resolve $a2"
$ns at 2.0 "$x resolve $a3"
#
# put the lookups in {} so the string doesn't get expanded
# right away by the interpreter...
#
$ns at 8.0 {puts "[$ns now] $a1: [$x lookup $a1], $a2: [$x lookup $a2], $a3: [$x lookup $a3]"}
$ns run
