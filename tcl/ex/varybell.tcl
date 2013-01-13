#
# Copyright (C) 1999 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

#
# Maintainer: Polly Huang <huang@isi.edu>
# Version Date: $Date: 1999/04/20 22:34:31 $
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/varybell.tcl,v 1.1 1999/04/20 22:34:31 polly Exp $ (USC/ISI)
#
# Unbalanced dumbell topology
# Used by large-scale-web-traffic.tcl
#        clients      servers
#
#            7       2    3
#          \ |       | / 
#         -- 1 ----- 0 -- 4
#          / |        \
#           426         5   

proc create_topology {} {
global ns n verbose num_node
set num_server 40
set num_client 420
set num_node [expr 7 + [expr $num_client + $num_server]]

#setup colors for nam
for {set i 0} {$i <= 100} {incr i} {
    set color [expr $i % 6]
    if {$color == 0} {
        $ns color $i blue
    } elseif {$color == 1} {
        $ns color $i red
    } elseif {$color == 2} {
        $ns color $i green
    } elseif {$color == 3} {
        $ns color $i yellow
    } elseif {$color == 4} {
        $ns color $i brown
    } elseif {$color == 5} {
        $ns color $i purple
    }
}

if {$verbose} { puts "Creating $num_server server, $num_client client dumbell topology..." }

for {set i 0} {$i < $num_node} {incr i} {
	set n($i) [$ns node]
}
# EDGES (from-node to-node length a b):
# node 0 connects to the server pool
# node num-node -1 is the extra node introduced in between 0 and 1
$ns duplex-link $n(0) $n([expr $num_node - 1]) 10Mb 20ms DropTail
# node 1 connects to the client pool
$ns duplex-link $n([expr $num_node - 1]) $n(1) 3Mb 20ms DropTail
$ns duplex-link $n(0) $n(2) 1.5Mb 20ms DropTail
$ns duplex-link $n(0) $n(3) 1.5Mb 30ms DropTail
$ns duplex-link $n(0) $n(4) 1.5Mb 40ms DropTail
$ns duplex-link $n(0) $n(5) 1.5Mb 60ms DropTail

for {set i 0} {$i < $num_server} {incr i} {
    set base [expr $i / 10]
    set delay [uniform 10 100]
    $ns duplex-link $n([expr $base + 2]) $n([expr $i + 6]) 10Mb [expr $delay * 0.001] DropTail
    if {$verbose} {puts "\$ns duplex-link \$n([expr $base + 2]) \$n([expr $i + 6]) 10Mb [expr $delay * 0.001] DropTail"}
}

for {set i 0} {$i < $num_client} {incr i} {
    set bandwidth [uniform 22 32]
    set delay [uniform 10 100]
    $ns duplex-link $n(1) $n([expr [expr $i + $num_server] + 6]) [expr $bandwidth * 1000000] [expr $delay * 0.001] DropTail
    if {$verbose} {puts "\$ns duplex-link \$n(1) \$n([expr [expr $i + $num_server] + 6]) [expr $bandwidth * 1000] [expr $delay * 0.001] DropTail"}
}

$ns set dst_ "";  #define list of web servers
for {set i 0} {$i < $num_server} {incr i} {
    $ns set dst_ "[$ns set dst_] [list [expr $i + 6]]"
}
if {$verbose} {puts "server set: [$ns set dst_]"}

$ns set src_ "";  #define list of web clients 
for {set i 0} {$i < $num_client} {incr i} {
    $ns set src_ "[$ns set src_] [list [expr [expr $i + $num_server] + 6]]"
}
if {$verbose} {puts "client set: [$ns set src_]"}


if {$verbose} { puts "Finished creating topology..." }
}
# end of create_topology

