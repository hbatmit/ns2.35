#
# Copyright (c) Xerox Corporation 1997. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# 
# Linking this file statically or dynamically with other modules is making
# a combined work based on this file.  Thus, the terms and conditions of
# the GNU General Public License cover the whole combination.
# 
# In addition, as a special exception, the copyright holders of this file
# give you permission to combine this file with free software programs or
# libraries that are released under the GNU LGPL and with code included in
# the standard release of ns-2 under the Apache 2.0 license or under
# otherwise-compatible licenses with advertising requirements (or modified
# versions of such code, with unchanged license).  You may copy and
# distribute such a system following the terms of the GNU GPL for this
# file and the licenses of the other code concerned, provided that you
# include the source code of that other code when and as the GNU GPL
# requires distribution of source code.
# 
# Note that people who make modified versions of this file are not
# obligated to grant this special exception for their modified versions;
# it is their choice whether to do so.  The GNU General Public License
# gives permission to release a modified version without this exception;
# this exception also makes it possible to release a modified version
# which carries forward this exception.
#
#@(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/test-rcvr.tcl,v 1.4 2005/08/26 05:05:29 tomh Exp $

#This script demonstrates support for two different delay-adaptive receiver types,
#namely the vat-receiver adaptation algorithm and a conservative adapatation 
#algotrithm (that tries to achieve low jitter at the expense of high playback delay)

#IT creates a simple 2 node topology with a link capacity of 2.0Mb.
#The background traffic on the link comprises of an aggregrate of 40 expo on/off 
#sources with peak rate of 80K and on/off time of 500ms
#The experimental traffic constitutes a single expo on/off source with a peak rate 
#of 80k and on/offtime of 500ms

set sources 40
set rcvrType VatRcvr
set bw 2.0Mb

#propagation delay
set delay 0.01ms
#simulation time
set runtime 100   
#pick up a random seed
set seed 1973272912
set bgsrcType expo
set out [open out.tr w]
set rate 80k
set btime 500ms 
set itime 500ms


#set up a queue limit so that no packets are dropped
Queue set limit_ 1000000

set rcvrType [lindex $argv 0]

if { ($rcvrType != "VatRcvr") && ($rcvrType != "ConsRcvr") } {
	puts "Usage : ns test.rcvr.tcl <rcvrtype>"
	puts "<rcvrtype> = \"VatRcvr\" | \"ConsRcvr\" "
	exit 1
} 

#puts stderr $out

ns-random $seed
$defaultRNG seed $seed

Agent instproc print-delay-stats {sndtime now plytime } {
	global out
	puts $out "$sndtime $now $plytime"
	flush $out
}


Simulator instproc create-receiver {node rcvrType} {
    set v [new Agent/$rcvrType]
    $self attach-agent $node $v
    return $v
}

Simulator instproc create-sender {node rate rcvr fid sndType } {
	global runtime btime itime
	set s [new Agent/UDP]
	$self attach-agent $node $s
	#Create exp on/off source
	if { $sndType == "expo" } {
		set tr [new Application/Traffic/Exponential]
		$tr set packetSize_ 200
		$tr set burst_time_ $btime
		$tr set idle_time_ $itime
		$tr set rate_ $rate
	} else {    
		#create victrace source
		set tfile [new Tracefile]
		$tfile filename "vic.32.200"
		set tr [new Application/Traffic/Trace]
		$tr attach-tracefile $tfile

	}
	
	$tr attach-agent $s
	$s set fid_ $fid
	
	$self connect $s $rcvr
	$self at [expr double([ns-random] % 10000000) / 1e7]  "$tr start"
	$self at $runtime "$tr stop"
}

proc finish {} {
	global env rcvrType
	set f [open temp.rands w]
	puts $f "TitleText: $rcvrType"
	puts $f "Device: Postscript"

	exec rm -f temp.p1 temp.p2

	exec awk {
		{
		print $2,$2-$1
		}
	} out.tr > temp.p1
	exec awk {
		{
		print $2,$3-$1
		}
	} out.tr > temp.p2
	puts $f [format "\n\"Network Delay"]
	flush $f
	exec cat temp.p1 >@ $f
	flush $f
	puts $f [format "\n\"Network+Playback Delay"]
	flush $f
	exec cat temp.p2 >@ $f
	flush $f
	close $f
	
	exec rm -f temp.p1 temp.p2
	exec xgraph -display $env(DISPLAY) -bb -tk -nl -m -x simtime -y delay temp.rands &

	exec rm -f out.tr
	exit 0
} 


set ns [new Simulator]

set n1 [$ns node]
set n2 [$ns node]


$ns duplex-link $n1 $n2 $bw $delay DropTail

set v0 [$ns create-receiver $n2 $rcvrType]

set v2 [new Agent/Null]
$ns attach-agent $n2 $v2

set i 0
while { $i < $sources } {
    $ns create-sender $n1 $rate $v2 0 $bgsrcType
    incr i
}

$ns create-sender $n1 $rate $v0 0 expo

$ns at [expr $runtime+1] "finish"
$ns run

