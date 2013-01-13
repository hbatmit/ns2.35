# Copyright (C) 2001 by USC/ISI
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
# An example script that simulates ISI web traffic. 
#
# Some attributes:
# 1. Topology: ~1000 nodes, 960 web clients, 40 web servers
# 2. Traffic: approximately 1,700,000 packets, heavy-tailed connection 
#             sizes, throughout 3600 second simulation time
# 3. Simulation scale: ~33 MB memory, ~1 hrs running on Red Hat Linux 7.0
#              Pentium II Xeon 450 MHz PC with 1GB physical memory
#
# Created by Kun-chan Lan (kclang@isi.edu)

global num_node n verbose lan_client lan_server 
set verbose 1
source isitopo.tcl

# Basic ns setup
set ns [new Simulator]

$ns rtproto Manual

puts "creating topology"
create_topology

#trace files setup (isi: inbound traffic  www: all traffic)
$ns trace-queue $n(7) $n(1) [open isi.in w]
$ns trace-queue $n(1) $n(7) [open isi.out w]
$ns trace-queue $n(7) $n([expr $num_node - 1]) [open www.out w]
$ns trace-queue $n([expr $num_node - 1]) $n(7) [open www.in w]


# Create page pool
set pool [new PagePool/EmpWebTraf]

# Setup servers and clients
$pool set-num-client [llength [$ns set src_]]
$pool set-num-server [llength [$ns set dst_]]

$pool set-num-client-lan $other_client
$pool set-num-server-lan $lan_server

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

set stopTime 3600.1

# Number of Sessions
set numSessionI 3000
set numSessionO 1500

# Inter-session Interval
set interSessionO [new RandomVariable/Empirical]
$interSessionO loadCDF cdf/2pm.www.out.sess.inter.cdf
set interSessionI [new RandomVariable/Empirical]
$interSessionI loadCDF cdf/2pm.www.in.sess.inter.cdf
#set interSessionO [new RandomVariable/Exponential]
#$interSessionO set avg_ 13.6
#set interSessionI [new RandomVariable/Exponential]
#$interSessionI set avg_ 1.6

## Number of Pages per Session
set sessionSizeO [new RandomVariable/Empirical]
$sessionSizeO loadCDF cdf/2pm.www.out.pagecnt.cdf
set sessionSizeI [new RandomVariable/Empirical]
$sessionSizeI loadCDF cdf/2pm.www.in.pagecnt.cdf


# Random seed at every run
global defaultRNG
$defaultRNG seed 0

# Create sessions
$pool set-num-session [expr $numSessionI + $numSessionO]

puts "creating outbound session"

set interPage [new RandomVariable/Empirical]
$interPage loadCDF cdf/2pm.www.out.idle.cdf

set pageSize [new RandomVariable/Constant]
$pageSize set val_ 1
set interObj [new RandomVariable/Empirical]
$interObj loadCDF cdf/2pm.www.out.obj.cdf  

set objSize [new RandomVariable/Empirical]
$objSize loadCDF cdf/2pm.www.out.pagesize.cdf
set reqSize [new RandomVariable/Empirical]
$reqSize loadCDF cdf/2pm.www.out.req.cdf

set persistSel [new RandomVariable/Empirical]
$persistSel loadCDF cdf/persist.cdf
set serverSel [new RandomVariable/Empirical]
$serverSel loadCDF cdf/outbound.server.cdf

set launchTime 0
for {set i 0} {$i < $numSessionO} {incr i} {
        if {$launchTime <=  $stopTime} {
	   set numPage [$sessionSizeO value]
	   puts "Session Outbound $i has $numPage pages $launchTime"

	   $pool create-session  $i  \
	                $numPage [expr $launchTime + 0.1] \
			$interPage $pageSize $interObj $objSize \
                        $reqSize $persistSel $serverSel 1

	   set launchTime [expr $launchTime + [$interSessionO value]]
	}
}

puts "creating inbound session"

set interPage [new RandomVariable/Empirical]
$interPage loadCDF cdf/2pm.www.in.idle.cdf

set pageSize [new RandomVariable/Constant]
$pageSize set val_ 1
set interObj [new RandomVariable/Empirical]
$interObj loadCDF cdf/2pm.www.in.obj.cdf  

set objSize [new RandomVariable/Empirical]
$objSize loadCDF cdf/2pm.www.in.pagesize.cdf
set reqSize [new RandomVariable/Empirical]
$reqSize loadCDF cdf/2pm.www.in.req.cdf

set persistSel [new RandomVariable/Empirical]
$persistSel loadCDF cdf/persist.cdf
set serverSel [new RandomVariable/Empirical]
$serverSel loadCDF cdf/inbound.server.cdf

set launchTime 0
for {set i 0} {$i < $numSessionI} {incr i} {
        if {$launchTime <=  $stopTime} {
	   set numPage [$sessionSizeI value]
	   puts "Session Inbound $i has $numPage pages $launchTime"

	   $pool create-session [expr $i + $numSessionO] \
	                $numPage [expr $launchTime + 0.1] \
			$interPage $pageSize $interObj $objSize \
                        $reqSize $persistSel $serverSel 0

	   set launchTime [expr $launchTime + [$interSessionI value]]
        }
}


## Start the simulation
$ns at $stopTime "finish"

proc finish {} {
	global ns gf 
	$ns flush-trace

	exit 0
}

puts "ns started"
$ns run

