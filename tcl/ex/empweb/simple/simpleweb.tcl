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
# An simple script for testing empweb
#
# Created by Kun-chan Lan (kclang@isi.edu)

global num_node n verbose lan_client lan_server 
source simpletopo.tcl

# Basic ns setup
set ns [new Simulator]

puts "creating topology"
create_topology

#trace files setup (isi: inbound traffic  www: all traffic)
$ns trace-queue $n(1) $n(0) [open test.out w]
$ns trace-queue $n(0) $n(1) [open test.in w]


# Create page pool
set pool [new PagePool/EmpWebTraf]

# Setup servers and clients
$pool set-num-client 1
$pool set-num-server 1

$pool set-num-remote-client 0
$pool set-num-server-lan 0

$pool set-client 0 $n(2)
$pool set-server 0 $n(1)
set i 0

set stopTime 100.1

# Number of Sessions
set numSession 1

# Inter-session Interval
set interSession [new RandomVariable/Empirical]
$interSession loadCDF cdf/sess.inter.cdf

## Number of Pages per Session
set sessionSize [new RandomVariable/Empirical]
$sessionSize loadCDF cdf/pagecnt.cdf


# Random seed at every run
global defaultRNG
$defaultRNG seed 0

# Create sessions
$pool set-num-session $numSession

set interPage [new RandomVariable/Empirical]
$interPage loadCDF cdf/idle.cdf

set pageSize [new RandomVariable/Constant]
$pageSize set val_ 1

set interObj [new RandomVariable/Empirical]
$interObj loadCDF cdf/obj.inter.cdf  

set objSize [new RandomVariable/Empirical]
#for FullTcp
$objSize loadCDF cdf/obj.size.byte.cdf
#for old tcp
#$objSize loadCDF cdf/obj.size.pkt.cdf

set reqSize [new RandomVariable/Empirical]
#for FullTcp
$reqSize loadCDF cdf/req.byte.cdf
#for old tcp
#$reqSize loadCDF cdf/req.pkt.cdf

set persistSel [new RandomVariable/Empirical]
$persistSel loadCDF cdf/persist.cdf

set serverSel [new RandomVariable/Empirical]
$serverSel loadCDF cdf/server.cdf

set windowS [new RandomVariable/Empirical]
$windowS loadCDF cdf/wins.cdf
set windowC [new RandomVariable/Empirical]
$windowC loadCDF cdf/winc.cdf


set mtu [new RandomVariable/Empirical]
$mtu loadCDF cdf/mtu.cdf


set launchTime 0
for {set i 0} {$i < $numSession} {incr i} {
        if {$launchTime <=  $stopTime} {
	   set numPage [$sessionSize value]

           $pool create-session  $i  \
	    	$numPage [expr $launchTime + 0.1] \
	    	$interPage $pageSize $interObj $objSize \
	    	$reqSize $persistSel $serverSel $windowS $windowC $mtu 1


	   set launchTime [expr $launchTime + [$interSession value]]
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

