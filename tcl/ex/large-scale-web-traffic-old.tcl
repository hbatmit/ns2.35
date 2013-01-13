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
# Version Date: $Date: 1999/10/26 16:51:56 $
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/large-scale-web-traffic-old.tcl,v 1.1 1999/10/26 16:51:56 haoboy Exp $ (USC/ISI)
#
#
# An example script that simulates large-scale web traffic. 
# See web-traffic.tcl for a smaller scale web traffic simulation.
# Some attributes:
# 1. Topology: ~460 nodes, 420 web clients, 40 web servers
# 2. Traffic: approximately 200,000 TCP connections, heavy-tailed connection 
#             sizes, throughout 4200 second simulation time
# 3. Simulation scale: ~800 MB memory, ~1.5-2 hrs running on FreeBSD 3.0
#              Pentium II Xeon 450 MHz PC with 1GB physical memory
#
puts "Warning!!!! Warning!!!!"
puts "This simulation requires ~800 MB MEMORY to complete!!"
puts "If this machine has less than 800 MB physical memory,"
puts "expect this simulation to finish in a DAY or so."

source ../http/http-mod.tcl

global num_node n verbose
set verbose 0
source varybell.tcl

# Basic ns setup
set ns [new Simulator]
$ns set-address 10 21

# Create generic packet trace
$ns trace-all [open large-scale-web-traffic.out w]

create_topology

########################### Modify From Here #####################
## Number of Sessions
## set numSession 400
set numSession 400

## Inter-session Interval
set interSession [new RandomVariable/Exponential]
$interSession set avg_ 1

## Number of Pages per Session
set sessionSize [new RandomVariable/Constant]
$sessionSize set val_ 250

set launchTime [$ns now]
for {set i 0} {$i < $numSession} {incr i} {
    set numPage [$sessionSize value]
    set httpSession($i) [new HttpSession $ns $numPage [$ns picksrc]]

## Inter-Page Interval
    $httpSession($i) setDistribution interPage_ Exponential 15

## Number of Objects per Page
    $httpSession($i) setDistribution pageSize_ Constant 1
    $httpSession($i) createPage

## Inter-Object Interval
    $httpSession($i) setDistribution interObject_ Exponential 0.01

## Number of Packets per Object
    $httpSession($i) setDistribution objectSize_ ParetoII 12 1.2
    $ns at [expr $launchTime + 0.1] "$httpSession($i) start"
    set launchTime [expr $launchTime + [$interSession value]]
}

## Start the simulation
$ns at 4200.1 "exit 0"
$ns run


