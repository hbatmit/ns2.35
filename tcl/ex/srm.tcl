#
# Copyright (C) 1997 by USC/ISI
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
# Maintainer: Kannan Varadhan <kannan@isi.edu>
# Version Date: $Date: 2000/02/18 10:41:50 $
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/srm.tcl,v 1.9 2000/02/18 10:41:50 polly Exp $ (USC/ISI)
#
# updated to use -multicast on and allocaddr by Lloyd Wood

if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
	set prog $argv0
}

source ../mcast/srm-nam.tcl		;# to separate control messages.
#source ../mcast/srm-debug.tcl		;# to trace delay compute fcn. details.

set ns [new Simulator -multicast on]

$ns trace-all [open out.tr w]
$ns namtrace-all [open out.nam w]

$ns color 0 white		;# data source
$ns color 40 blue		;# session
$ns color 41 red		;# request
$ns color 42 green		;# repair
$ns color 1 Khaki		;# source node
$ns color 2 goldenrod
$ns color 3 sienna
$ns color 4 HotPink
$ns color 5 maroon
$ns color 6 orchid
$ns color 7 purple
$ns color 8 snow4
$ns color 9 PeachPuff1

# make the nodes
for {set i 0} {$i <= 3} {incr i} {
	set n($i) [$ns node]
}
$n(0) shape "other"

# now the links
for {set i 1} {$i <= 3} {incr i} {
	$ns duplex-link $n($i) $n(0) 1.5Mb 10ms DropTail
}
$ns duplex-link-op $n(0) $n(1) orient left
$ns duplex-link-op $n(0) $n(2) orient right-up
$ns duplex-link-op $n(0) $n(3) orient right-down

set group [Node allocaddr]
set mh [$ns mrtproto DM {}]
#$ns at 0.3 "$mh switch-treetype $group"	;# for centralised multicast

# now the multicast, and the agents
set srmStats [open srmStats.tr w]
set srmEvents [open srmEvents.tr w]

set fid 0
foreach i [array names n] {
	set srm($i) [new Agent/SRM]
	$srm($i) set dst_addr_ $group
	$srm($i) set dst_port_ 0
	$srm($i) set fid_ [incr fid]
	$srm($i) log $srmStats
	$srm($i) trace $srmEvents
#	$ns at 1.0 "$srm($i) start"

	$ns attach-agent $n($i) $srm($i)
}

# Attach a data source to srm(0)
set packetSize 210
set exp0 [new Application/Traffic/Exponential]
$exp0 set packetSize_ $packetSize
$exp0 set burst_time_ 500ms
$exp0 set idle_time_ 500ms
$exp0 set rate_ 100k
$exp0 attach-agent $srm(0)
$srm(0) set tg_ $exp0
$srm(0) set app_fid_ 0
$srm(0) set packetSize_ $packetSize	;# so repairs are correct

$ns at 0.5 "$srm(0) start; $srm(0) start-source"
$ns at 1.0 "$srm(1) start"
$ns at 1.1 "$srm(2) start"
$ns at 1.2 "$srm(3) start"

proc distDump interval {
	global ns srm
	foreach i [array names srm] {
		set dist [$srm($i) distances?]
		if {$dist != ""} {
			puts "[format %7.4f [$ns now]] distances $dist"
		}
	}
	$ns at [expr [$ns now] + $interval] "distDump $interval"
}
$ns at 0.0 "distDump 1"

proc finish src {
	$src stop

	global prog ns srmStats srmEvents
	$ns flush-trace
	close $srmStats
	close $srmEvents

	puts "running nam..."
	exec nam out.nam &
	exit 0
}
$ns at 5.0 "finish $exp0"

#$ns gen-map
$ns run
