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
# Maintained by: Polly Huang Tue Feb  2 14:34:54 PST 1999
# Version Date: $Date: 1999/09/09 03:29:46 $
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/shuttle-mcast.tcl,v 1.3 1999/09/09 03:29:46 salehi Exp $ (USC/ISI)
#
# Creating 3 multicast sessions over a partial mbone topology (1996)

source shuttle.tcl
set mcastConnection 3

set ns [new Simulator]
$ns multicast

# set colors for nam
for {set i 0} {$i < 10} {incr i} {
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
        $ns color $i black
    }
}

# config output files
set f [open shuttle-mcast.out w]
$ns trace-all $f
set nf [open shuttle-mcast.nam w]
$ns namtrace-all $nf

# creating mbone shuttle session topology 
# generated from 1996 trace
puts "This simulation will take a long time..."
#Node expandaddr
create-topology

# mcast config
set mproto CtrMcast
set mrthandle [$ns mrtproto $mproto {}]

# create mcast sessions
for {set i 0} {$i < $mcastConnection} {incr i} {
    # groups config
    set g($i) [expr [Node allocaddr]]
    if {$mrthandle != ""} {
	$ns at 0.01 "$mrthandle switch-treetype $g($i)"
    }    

    # source config
    set udp($i) [new Agent/UDP]
    $ns attach-agent $n($i) $udp($i)
    set cbr($i) [new Application/Traffic/CBR]
    $cbr($i) attach-agent $udp($i)
    $udp($i) set dst_addr_ $g($i)
    $udp($i) set dst_port_ 0
    $udp($i) set fid_ $i
    $ns at [expr $i + 1] "$cbr($i) start"

    # member config
    set member1 [expr [expr $num_node - $i] - 1]
    set member2 [expr [expr $num_node / 2 ] - $i]
    set rcvr($i:$member1) [new Agent/Null]
    set rcvr($i:$member2) [new Agent/Null]
    $ns attach-agent $n($member1) $rcvr($i:$member1)
    $ns attach-agent $n($member2) $rcvr($i:$member2)
    $ns at 0.5 "$n($member1) join-group  $rcvr($i:$member1) $g($i)"
    $ns at 0.5 "$n($member2) join-group  $rcvr($i:$member2) $g($i)"
}

proc finish {} {
    global ns f nf
    $ns flush-trace
    close $f
    close $nf

    exec nam shuttle-mcast.nam
    exit
}

$ns at 13.0 "finish"
$ns run
