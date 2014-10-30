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
# An example script that simulates web traffic. 
# See large-scale-web-traffic.tcl for a large-scale web traffic simulation.

# Initial setup
#source ../webcache/webtraf.tcl
source dumbbell.tcl
global num_node n

set ns [new Simulator]

# set up colors for nam 
for {set i 1} {$i <= 30} {incr i} {
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

# Create nam trace and generic packet trace
$ns namtrace-all [open validate.nam w]
# trace-all is the generic trace we've been using
$ns trace-all [open validate.out w]

create_topology

set pool [new PagePool/WebTraf]

# Set up server and client nodes
$pool set-num-client [llength [$ns set src_]]
$pool set-num-server [llength [$ns set dst_]]
global n
set i 0
foreach s [$ns set src_] {
	$pool set-client $i $n($s)
	incr i
}
set i 0
foreach s [$ns set dst_] {
	$pool set-server $i $n($s)
	incr i
}

# Number of Pages per Session
set numPage 10
# Session 0 starts from 0.1s, session 1 starts from 0.2s
$pool set-num-session 2

# XXX Can't initialize instvars using something like this:
# set interPage [new RandomVariable/Exponential -avg_ 1]
# Must explicitly set variable values
set interPage [new RandomVariable/Exponential] 
$interPage set avg_ 1
set pageSize [new RandomVariable/Constant]
$pageSize set val_ 1
set interObj [new RandomVariable/Exponential]
$interObj set avg_ 0.01
set objSize [new RandomVariable/ParetoII]
$objSize set avg_ 10
$objSize set shape_ 1.2
$pool create-session 0 $numPage 0.1 $interPage $pageSize $interObj $objSize

set interPage [new RandomVariable/Exponential]
$interPage set avg_ 1
set pageSize [new RandomVariable/Constant]
$pageSize set val_ 1
set interObj [new RandomVariable/Exponential]
$interObj set avg_ 0.01
set objSize [new RandomVariable/ParetoII]
$objSize set avg_ 10
$objSize set shape_ 1.2
$pool create-session 1 $numPage 0.2 $interPage $pageSize $interObj $objSize

# $pool set-interPageOption 0; # 0 for time between the start of 2 pages
                               # 1 for time between the end of a page and 
                               #   the start of the next
                               # default: 1

$ns at 1000.0 "finish"

proc finish {} {
	global ns
	$ns flush-trace
	puts "running nam..."
	exec nam validate.nam &
	exit 0
}

# Start the simualtion
$ns run


