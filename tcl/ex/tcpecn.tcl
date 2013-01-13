# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#
#  The copyright of this module includes the following
#  linking-with-specific-other-licenses addition:
#
#  In addition, as a special exception, the copyright holders of
#  this module give you permission to combine (via static or
#  dynamic linking) this module with free software programs or
#  libraries that are released under the GNU LGPL and with code
#  included in the standard release of ns-2 under the Apache 2.0
#  license or under otherwise-compatible licenses with advertising
#  requirements (or modified versions of such code, with unchanged
#  license).  You may copy and distribute such a system following the
#  terms of the GNU GPL for this module and the licenses of the
#  other code concerned, provided that you include the source code of
#  that other code when and as the GNU GPL requires distribution of
#  source code.
#
#  Note that people who make modified versions of this module
#  are not obligated to grant this special exception for their
#  modified versions; it is their choice whether to do so.  The GNU
#  General Public License gives permission to release a modified
#  version without this exception; this exception also makes it
#  possible to release a modified version which carries forward this
#  exception.
# 
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/tcpecn.tcl,v 1.8 2005/09/16 03:05:42 tomh Exp $
#
# A simple example for tcp ecn simulation/animation with namgraph support
# This script is adopted from ns-2/tcl/test/test-suite-ecn.tcl
 
set ns [new Simulator]
 
#
#
# Create a simple six node topology:
#
#        s1                 s3
#         \                 /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 --------- r2
# 10Mb,3ms /               \ 10Mb,5ms
#         /                 \
#        s2                 s4
#

proc build_topology { ns } {

    global node_

    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 6ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
 
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0.5
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0.5
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up
 
}

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

build_topology $ns

set redq [[$ns link $node_(r1) $node_(r2)] queue]
$redq set setbit_ true

# Use nam trace format for TCP variable trace 
Agent/TCP set nam_tracevar_ true

set tcp1 [$ns create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
$tcp1 set window_ 15
$tcp1 set ecn_ 1
 
set tcp2 [$ns create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
$tcp2 set window_ 15
$tcp2 set ecn_ 1
 
set ftp1 [$tcp1 attach-app FTP]    
set ftp2 [$tcp2 attach-app FTP]

# Add agent traces and variable trace
$ns add-agent-trace $tcp1 tcp1
$ns add-agent-trace $tcp2 tcp2
$ns monitor-agent-trace $tcp1
$ns monitor-agent-trace $tcp2
$tcp1 tracevar cwnd_
$tcp2 tracevar cwnd_
 
$ns at 0.0 "$ftp1 start"
$ns at 0.0 "$ftp2 start"

$ns at 2.0 "finish"

proc finish {} {
        global ns f nf
        $ns flush-trace
        close $f
        close $nf
 
        #XXX
        puts "Filtering ..."
	exec tclsh8.0 ../nam/bin/namfilter.tcl out.nam

        puts "running nam..."
        exec nam out.nam &
        exit 0
}
 
$ns run


