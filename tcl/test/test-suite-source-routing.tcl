#
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# Copyright (c) 1998 University of Southern California.
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
# Contributed by Rishi Bhargava <rishi_bhargava@yahoo.com> May, 2001.
# 
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-source-routing.tcl,v 1.7 2006/01/24 23:00:07 sallyfloyd Exp $
#

#
# invoked as ns $file $t [QUIET]
# expected to pop up xgraph output (unless QUIET)
# and to leave the plot in temp.rands
#

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP SR Src_rt ; # hdrs reqd for validation

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.

Class TestSuite

Class Test/source_rtg -superclass TestSuite

Test/source_rtg instproc usage {} {
	puts stderr {usage: ns test-suite-source-routing.tcl test [QUIET] Test suites for source routing.
}
	exit 1
}

Test/source_rtg instproc init {} {
$self instvar ns_
set ns_ [new Simulator]
$ns_ src_rting 1
$self setup_topo

}

Test/source_rtg instproc setup_topo {} {


global node_
$self instvar ns_

set f [open "temp.rands" "w"]
$ns_ trace-all $f

# Create nodes
set node_(0) [$ns_ node]
set node_(1) [$ns_ node]
set node_(2) [$ns_ node]
set node_(3) [$ns_ node]
set node_(4) [$ns_ node]

#Create links between the nodes
$ns_ duplex-link $node_(0) $node_(2) 1Mb 10ms DropTail
$ns_ duplex-link $node_(1) $node_(2) 1Mb 10ms DropTail
$ns_ duplex-link $node_(3) $node_(2) 1Mb 10ms DropTail
$ns_ duplex-link $node_(2) $node_(4) 1Mb 10ms DropTail
$ns_ duplex-link $node_(4) $node_(3) 1Mb 10ms DropTail
$ns_ duplex-link $node_(1) $node_(3) 1Mb 10ms DropTail

#Create a TCP agent and attach it to node n0
set cbr0 [new Agent/TCP]
$ns_ attach-agent $node_(0) $cbr0
$cbr0 set fid_ 0

#Create a TCP agent and attach it to node n1
set cbr1 [new Agent/TCP]
$ns_ attach-agent $node_(1) $cbr1
$cbr1 set fid_ 1

$cbr0 target [$node_(0) set src_agent_]
$cbr1 target [$node_(1) set src_agent_]

# install two connections

set temp [$node_(0) set src_agent_]
$temp install_connection [$cbr0 set fid_] 0 1 0 2 4 3 1
set temp [$node_(1) set src_agent_]
$temp install_connection [$cbr1 set fid_] 1 3 1 2 3 

#Create a Null agent (a traffic sink) and attach it to node n3
set null0 [new Agent/TCPSink]
set null1 [new Agent/TCPSink]

$ns_ attach-agent $node_(1) $null0
$ns_ attach-agent $node_(3) $null1

#Connect the traffic sources with the traffic sink

$ns_ connect $cbr0 $null0  
$ns_ connect $cbr1 $null1

set ftp1 [$cbr0 attach-source FTP]
set ftp2 [$cbr1 attach-source FTP]


#Schedule events for the TCP agents
 $ns_ at 0.5 "$ftp1 start"
 $ns_ at 0.5 "$ftp2 start"
 $ns_ at 10.0 "$ftp1 stop"
 $ns_ at 10.5 "$ftp2 stop"

 $ns_ at 11.0 "exit 0"

}

Test/source_rtg instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}


proc runtest {arg} {
	set test "source_rtg"
	set t [new Test/$test]
	$t run
}

runtest $argv
