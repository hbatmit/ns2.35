# Testing for CPU/memory efficiency for routing code in ns-2
# using t1000 - a 1000 node ts topology

set ns [new Simulator]
###source /nfs/ruby/haldar/conser/ns-2/tcl/ex/t1000.tcl
source t1000.tcl
set linkBW 5Mb
global n ns

puts "starting to create topology"
puts "time: [clock format [clock seconds] -format %X]"

create-topology ns node $linkBW
puts [Node set nn_]

puts "completed creating topology"
puts "ns run.."
puts "time: [clock format [clock seconds] -format %X]"

$ns run

puts "completed simulation"
puts "time: [clock format [clock seconds] -format %X]"
