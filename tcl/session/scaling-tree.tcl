#! ../../ns
source scaling-defaults.tcl

# Usage tree.tcl -rand 1.0 -det 0.05 -seed -size $max >! tmp.dat
# defaults 
set size 10
set det 0.0
set rand 1.0
set delay 10.0

if {$argc == 0} {
        puts "Usage: $argv0 \[-det $det\] \[-rand $rand\] \[-seed\] \[-size $size\] \[-delay $delay\]"
        exit 1
}

# regexp {^(.+)\..*$} $argv0 match ext

for {set i 0} {$i < $argc} {incr i} {
        set opt [lindex $argv $i]
        if {$opt == "-size"} {
                set size [lindex $argv [incr i]]
        } elseif {$opt == "-det"} {
                set det [lindex $argv [incr i]]
        } elseif {$opt == "-rand"} {
                set rand [lindex $argv [incr i]]
        } elseif {$opt == "-delay"} {
                set delay [lindex $argv [incr i]]
        } elseif {$opt == "-seed"} {
		ns-random 0
        } 

}

#                     S
#                     |
#                     X Packet loss occurs here
#                     | 
#                     | Delta_
#                     |
#                     R
#                   /   \
#                  R     R
#                 / \   / \ delta_
#                R   R R   R
#                     .
#                     .
#                     .
#

set ns [new Simulator]
set tree [new Topology/BTree $size]

$tree set det_ $det
$tree set rand_ $rand
$tree set delay_ $delay

puts "D     = [$tree set det_]"
puts "R     = [$tree set rand_]"
puts "Delay = [$tree set delay_]"
puts "Size  = $size"

$ns at 0.0 "$tree flood 1"
$ns run




