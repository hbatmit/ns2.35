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
# An example script that simulates large-scale web traffic. 
# See web-traffic.tcl for a smaller scale web traffic simulation.
#
# Some attributes:
# 1. Topology: ~460 nodes, 420 web clients, 40 web servers
# 2. Traffic: approximately 200,000 TCP connections, heavy-tailed connection 
#             sizes, throughout 4200 second simulation time
# 3. Simulation scale: ~62 MB memory, ~1 hrs running on FreeBSD 3.0
#              Pentium II Xeon 450 MHz PC with 1GB physical memory
#
# Created by Polly Huang (huang@catarina.usc.edu)
# Modified by Haobo Yu (haoboy@isi.edu)

global num_node n verbose
set verbose 0
source varybell.tcl

# Basic ns setup
set ns [new Simulator]

# Create generic packet trace
$ns trace-all [open my-largescale.out w]

# Defined in varybell.tcl
create_topology

########################### Modify From Here #####################

# Create page pool
set pool [new PagePool/WebTraf]

# Setup servers and clients
$pool set-num-client [llength [$ns set src_]]
$pool set-num-server [llength [$ns set dst_]]
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

# Number of Sessions
set numSession 400

# Inter-session Interval
set interSession [new RandomVariable/Exponential]
$interSession set avg_ 1

## Number of Pages per Session
set sessionSize [new RandomVariable/Constant]
$sessionSize set val_ 250

# Random seed at every run
global defaultRNG
$defaultRNG seed 0

# Create sessions
$pool set-num-session $numSession
set launchTime 0
for {set i 0} {$i < $numSession} {incr i} {
	set numPage [$sessionSize value]
	puts "Session $i has $numPage pages"
	set interPage [new RandomVariable/Exponential]
	$interPage set avg_ 15
	set pageSize [new RandomVariable/Constant]
	$pageSize set val_ 1
	set interObj [new RandomVariable/Exponential]
	$interObj set avg_ 0.01
	set objSize [new RandomVariable/ParetoII]
	$objSize set avg_ 12
	$objSize set shape_ 1.2
	$pool create-session $i $numPage [expr $launchTime + 0.1] \
			$interPage $pageSize $interObj $objSize
	set launchTime [expr $launchTime + [$interSession value]]
}

# $pool set-interPageOption 0; # 0 for time between the start of 2 pages
                               # 1 for time between the end of a page and 
                               #   the start of the next
                               # default: 1

## Start the simulation
$ns at 4200.1 "finish"

proc finish {} {
	global ns
	$ns flush-trace
	exit 0
}

puts "ns started"
$ns run

