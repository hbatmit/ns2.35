# test-alloc-address.tcl 
# a simple test to check if ns-address formatting API's are working. 

set ns [new Simulator]
Simulator set EnableMcast_ 1
# 5 possible address formatting APIs and their combo thereof:

$ns set-address-format def
#$ns set-address-format expanded
#$ns set-address-format hierarchical
#$ns set-address-format hierarchical 3 3 3 5

#$ns expand-port-field-bits 24

set Mcastshift [AddrParams McastShift]
set Portmask [AddrParams PortMask]
set Portshift [AddrParams PortShift]
set Nodemask [AddrParams NodeMask 1]
set Nodeshift [AddrParams NodeShift 1]

puts "mcastshift = $Mcastshift"
puts "portmask= $Portmask"
puts "portshift = $Portshift"
puts "nodemask(1) = $Nodemask"
puts "nodeshift(1) = $Nodeshift"
