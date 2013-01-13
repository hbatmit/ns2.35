#
# Copyright (c) 1995-1997 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-adaptive-red.tcl,v 1.27 2006/06/14 01:12:28 sallyfloyd Exp $
#
# To run all tests: test-all-adaptive-red

set dir [pwd]
catch "cd tcl/test"
catch "cd $dir"
source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5 
Queue/RED set maxthresh_ 15
# Queue/RED set bottom_ 0.01
# The RED parameter defaults are being changed for automatic configuration.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set bugfix_ss_ 0
# A new variable on 6/13/06.

set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

TestSuite instproc finish file {
	global quiet PERL
	$self instvar ns_ tchan_ testName_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		$self plotQueue $testName_
	}
	## exec csh gnuplotA1.com temp.rands $testName_
	## exec csh gnuplotQueue.com temp.queue $testName_
	$ns_ halt
}

TestSuite instproc enable_tracequeue ns {
	$self instvar tchan_ node_
	set redq [[$ns link $node_(r1) $node_(r2)] queue]
	set tchan_ [open all.q w]
	$redq trace curq_
	$redq trace ave_
	$redq attach $tchan_
}

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    # A 1500-byte packet has a transmission time of 0.008 seconds.
    # A queue of 25 1500-byte packets would be 0.2 seconds. 
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}   

Class Topology/netfast -superclass Topology
Topology/netfast instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 100Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 100Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 15Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 1000
    $ns queue-limit $node_(r2) $node_(r1) 1000
    $ns duplex-link $node_(s3) $node_(r2) 100Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 100Mb 5ms DropTail
}   

Class Topology/net3 -superclass Topology
Topology/net3 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 10ms RED
    $ns duplex-link $node_(r2) $node_(r1) 1.5Mb 10ms RED
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 2ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 3ms DropTail
}   

Class Topology/net4 -superclass Topology
Topology/net4 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 1000Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 1000Mb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 10Mb 10ms RED
    $ns duplex-link $node_(r2) $node_(r1) 10Mb 10ms RED
    $ns queue-limit $node_(r1) $node_(r2) 1000
    $ns queue-limit $node_(r2) $node_(r1) 1000
    $ns duplex-link $node_(s3) $node_(r2) 1000Mb 2ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 1000Mb 3ms DropTail
}

Class Topology/netlong -superclass Topology
Topology/netlong instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 100ms RED
    $ns duplex-link $node_(r2) $node_(r1) 1.5Mb 100ms RED
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 2ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 3ms DropTail
}   

TestSuite instproc plotQueue file {
	global quiet
	$self instvar tchan_
	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"

	if { [info exists tchan_] } {
		close $tchan_
	}
	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q

	exec awk $awkCode all.q

	puts $f \"queue
	exec cat temp.q >@ $f  
	puts $f \n\"ave_queue
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y queue temp.queue &
	}
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
    global quiet
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcpSrc)] {
	set dump_inst_($tcpSrc) 1
	set report $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]
	if {$quiet == "false"} {
		puts $report
	}
	$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
	return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
    set report time=[$ns_ now]/class=$label/ack=[$tcpSrc set ack_]/packets_resent=[$tcpSrc set nrexmitpack_]
    if {$quiet == "false"} {
    	puts $report
    }
}       

TestSuite instproc maketraffic {{stoptime 50.0} {window 15} {starttime 3.0}} {
    $self instvar ns_ node_ testName_ net_

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ $window

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set window_ $window

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $starttime "$ftp2 start"

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
}

TestSuite instproc maketraffic1 {window time} {
    $self instvar ns_ node_ testName_ net_

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 2]
    $tcp1 set window_ $window

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 3]
    $tcp2 set window_ $window

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at $time "$ftp1 start"
    $ns_ at [expr 2 * $time] "$ftp2 start"
}

TestSuite instproc automatic queue {
    $queue set thresh_ 0
    $queue set maxthresh_ 0
    $queue set q_weight_ 0
}

#####################################################################

Class Test/red1 -superclass TestSuite
Test/red1 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ red1
    $self next pktTraceFile
}
Test/red1 instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 
    $ns_ run
}

Class Test/red1Adapt -superclass TestSuite
Test/red1Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red1Adapt
    $self next pktTraceFile
}
Test/red1Adapt instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    set forwq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $forwq set adaptive_ 1 
    $self automatic $forwq
    $self maketraffic
    $ns_ run   
}

Class Test/red1ECN -superclass TestSuite
Test/red1ECN instproc init {} {
    $self instvar net_ test_
    set net_ net2
    set test_ red1ECN
    Queue/RED set setbit_ true
    Agent/TCP set ecn_ 1
    Test/red1ECN instproc run {} [Test/red1 info instbody run ]
    $self next pktTraceFile
}

Class Test/red1AdaptECN -superclass TestSuite
Test/red1AdaptECN instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2
    set test_ red1AdaptECN
    Queue/RED set setbit_ true
    Agent/TCP set ecn_ 1
    Test/red1AdaptECN instproc run {} [Test/red1Adapt info instbody run ]
    $self next pktTraceFile
}


Class Test/red1AdaptFeng -superclass TestSuite
Test/red1AdaptFeng instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red1AdaptFeng
    Queue/RED set alpha_ 3.0
    Queue/RED set beta_ 2.0
    Queue/RED set feng_adaptive_ 1
    Test/red1AdaptFeng instproc run {} [Test/red1Adapt info instbody run ]
    $self next pktTraceFile
}

#####################################################################

Class Test/fastlink -superclass TestSuite
Test/fastlink instproc init {} {
    $self instvar net_ test_
    set net_ netfast 
    set test_ fastlink
    $self next pktTraceFile
}
Test/fastlink instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 5.0 100 1.5
    $self maketraffic1 100 0.5
    $ns_ run
}

Class Test/fastlinkAutowq -superclass TestSuite
Test/fastlinkAutowq instproc init {} {
    $self instvar net_ test_ ns_
    set net_ netfast 
    set test_ fastlinkAutowq
    Queue/RED set q_weight_ 0
    Test/fastlinkAutowq instproc run {} [Test/fastlink info instbody run ]
    $self next pktTraceFile
}

Class Test/fastlinkAutothresh -superclass TestSuite
Test/fastlinkAutothresh instproc init {} {
    $self instvar net_ test_ ns_
    set net_ netfast 
    set test_ fastlinkAutothresh
    Queue/RED set thresh_ 0 
    Queue/RED set maxthresh_ 0
    Test/fastlinkAutothresh instproc run {} [Test/fastlink info instbody run ]
    $self next pktTraceFile
}

Class Test/fastlinkAdapt -superclass TestSuite
Test/fastlinkAdapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ netfast 
    set test_ fastlinkAdapt
    Queue/RED set adaptive_ 1
    Test/fastlinkAdapt instproc run {} [Test/fastlink info instbody run ]
    $self next pktTraceFile
}

Class Test/fastlinkAllAdapt -superclass TestSuite
Test/fastlinkAllAdapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ netfast 
    set test_ fastlinkAllAdapt
    Queue/RED set adaptive_ 1
    Queue/RED set q_weight_ 0
    Queue/RED set thresh_ 0
    Queue/RED set maxthresh_ 0
    Test/fastlinkAllAdapt instproc run {} [Test/fastlink info instbody run ]
    $self next pktTraceFile
}

Class Test/fastlinkAllAdaptECN -superclass TestSuite
Test/fastlinkAllAdaptECN instproc init {} {
    $self instvar net_ test_ ns_
    set net_ netfast 
    set test_ fastlinkAllAdaptECN
    Queue/RED set adaptive_ 1
    Queue/RED set q_weight_ 0
    Queue/RED set thresh_ 0
    Queue/RED set maxthresh_ 0
    Queue/RED set setbit_ true
    Agent/TCP set ecn_ 1
    Test/fastlinkAllAdaptECN instproc run {} [Test/fastlink info instbody run ]
    $self next pktTraceFile
}

# Changing upper bound for max_p 
Class Test/fastlinkAllAdapt1 -superclass TestSuite
Test/fastlinkAllAdapt1 instproc init {} {
    $self instvar net_ test_ ns_
    set net_ netfast 
    set test_ fastlinkAllAdapt1
    Queue/RED set adaptive_ 1
    Queue/RED set q_weight_ 0
    Queue/RED set thresh_ 0
    Queue/RED set maxthresh_ 0
    Queue/RED set top_ 0.2
    Queue/RED set bottom_ 0.1
    Test/fastlinkAllAdapt1 instproc run {} [Test/fastlink info instbody run ]
    $self next pktTraceFile
}

#####################################################################

Class Test/longlink -superclass TestSuite
Test/longlink instproc init {} {
    $self instvar net_ test_
    set net_ netlong 
    set test_ longlink
    $self next pktTraceFile
}
Test/longlink instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 50.0 100 1.5
    $self maketraffic1 100 0.5
    $ns_ run
}

Class Test/longlinkAdapt -superclass TestSuite
Test/longlinkAdapt instproc init {} {
    $self instvar net_ test_
    set net_ netlong 
    Queue/RED set adaptive_ 1
    Queue/RED set thresh_ 0
    Queue/RED set maxthresh_ 0
    Queue/RED set q_weight_ 0
    set test_ longlinkAdapt
    Test/longlinkAdapt instproc run {} [Test/longlink info instbody run ]
    $self next pktTraceFile
}

Class Test/longlinkAdapt1 -superclass TestSuite
Test/longlinkAdapt1 instproc init {} {
    $self instvar net_ test_
    set net_ netlong 
    Queue/RED set adaptive_ 1
    Queue/RED set thresh_ 0
    Queue/RED set maxthresh_ 0
    Queue/RED set q_weight_ -1.0
    set test_ longlinkAdapt1
    Test/longlinkAdapt1 instproc run {} [Test/longlink info instbody run ]
    $self next pktTraceFile
}

#####################################################################

# This reuses connection state, with $nums effective TCP connections,
#   reusing state from $conns underlying TCP connections.
TestSuite instproc newtraffic { num window packets start interval conns} {
    $self instvar ns_ node_ testName_ net_

    for {set i 0} {$i < $conns } {incr i} {
       	set tcp($i) [$ns_ create-connection-list TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 2]
	set source($i) [lindex $tcp($i) 0]
	set sink($i) [lindex $tcp($i) 1]
       	$source($i) set window_ $window
	set ftp($i) [$source($i) attach-app FTP]
	$ns_ at 0.0 "$ftp($i) produce 0"
    }
    for {set i 0} {$i < $num } {incr i} {
        set tcpnum [ expr $i % $conns ]
	set time [expr $start + $i * $interval]
	$ns_ at $time  "$source($tcpnum) reset"
	$ns_ at $time  "$sink($tcpnum) reset"
        $ns_ at $time  "$ftp($tcpnum) producemore $packets"
    }
}

Class Test/red2 -superclass TestSuite
Test/red2 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ red2
    $self next pktTraceFile
}
Test/red2 instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 
    $self newtraffic 20 20 1000 25 0.1 20
    $ns_ run
}

Class Test/red2Adapt -superclass TestSuite
Test/red2Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red2Adapt
    $self next pktTraceFile
}
Test/red2Adapt instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    set forwq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $forwq set adaptive_ 1 
    $self automatic $forwq
    $self maketraffic
    $self newtraffic 20 20 1000 25 0.1 20
    $ns_ run   
}

Class Test/red2A-Adapt -superclass TestSuite
Test/red2A-Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red2A-Adapt
    Queue/RED set alpha_ 0.02
    Queue/RED set beta_ 0.8
    Test/red2A-Adapt instproc run {} [Test/red2Adapt info instbody run ]
    $self next pktTraceFile
}

Class Test/red2-AdaptFeng -superclass TestSuite
Test/red2-AdaptFeng instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red2-AdaptFeng
    Queue/RED set alpha_ 3
    Queue/RED set beta_ 2
    Queue/RED set feng_adaptive_ 1
    Test/red2-AdaptFeng instproc run {} [Test/red2Adapt info instbody run ]
    $self next pktTraceFile
}


#####################################################################

Class Test/red3 -superclass TestSuite
Test/red3 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ red3
    $self next pktTraceFile
}
Test/red3 instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 
    $self newtraffic 15 20 300 0 0.1 15
    $ns_ run
}

Class Test/red3Adapt -superclass TestSuite
Test/red3Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red3Adapt
    $self next pktTraceFile
}
Test/red3Adapt instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    set forwq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $forwq set adaptive_ 1 
    $self automatic $forwq

    $self maketraffic
    $self newtraffic 15 20 300 0 0.1 15
    $ns_ run   
}

Class Test/red4Adapt -superclass TestSuite
Test/red4Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red4Adapt
    Queue/RED set alpha_ 0.02
    Queue/RED set beta_ 0.8
    Test/red4Adapt instproc run {} [Test/red3Adapt info instbody run ]
    $self next pktTraceFile
}

TestSuite instproc printall { fmon } {
        puts "aggregate per-link total_drops [$fmon set pdrops_]"
        puts "aggregate per-link total_packets [$fmon set pdepartures_]"
}


# Class Test/red5 -superclass TestSuite
# Test/red5 instproc init {} {
#     $self instvar net_ test_ ns_
#     set net_ net2 
#     set test_ red5
#     Queue/RED set alpha_ 0.02
#     Queue/RED set beta_ 0.8
#     $self next pktTraceFile
# }
# Test/red5 instproc run {} {
#     $self instvar ns_ node_ testName_ net_
#     $self setTopo
#     set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
#     set fmon [$ns_ makeflowmon Fid]
#     $ns_ attach-fmon $slink $fmon
#     $self maketraffic
#     $self newtraffic 20 20 300 0 0.001 10
#     # To run many flows:
#     # $self newtraffic 4000 20 300 0 0.005 500
#     # $self newtraffic 40000 20 300 0 0.001 500
#     $ns_ at 49.99 "$self printall $fmon" 
#     $ns_ run
# }

#####################################################################

Class Test/transient -superclass TestSuite
Test/transient instproc init {} {
    $self instvar net_ test_
    set net_ netfast 
    set test_ transient
    $self next pktTraceFile
}
Test/transient instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    set stoptime 5.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 100

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set window_ 1000

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 2.5 "$ftp2 start"

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/transient1 -superclass TestSuite
Test/transient1 instproc init {} {
    $self instvar net_ test_
    set net_ netfast 
    set test_ transient1
    Queue/RED set q_weight_ 0
    Test/transient1 instproc run {} [Test/transient info instbody run ]
    $self next pktTraceFile
}

Class Test/transient2 -superclass TestSuite
Test/transient2 instproc init {} {
    $self instvar net_ test_
    set net_ netfast 
    set test_ transient2
    Queue/RED set q_weight_ 0.0001
    Test/transient2 instproc run {} [Test/transient info instbody run ]
    $self next pktTraceFile
}

Class Test/notcautious -superclass TestSuite
Test/notcautious instproc init {} {
    $self instvar net_ test_
    set net_ net4
    set test_ notcautious
    Queue/RED set cautious_ 0
    $self next pktTraceFile
}
Test/notcautious instproc run {} {
    $self instvar ns_ node_ testName_ net_
    Queue/RED set q_weight_ -1
    Queue/RED set adaptive_ 1
    Queue/RED set thresh 0
    Queue/RED set maxthresh 0
    $self setTopo
    set stoptime 5.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 600

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set window_ 600

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    $self tcpDump $tcp1 5.0
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}
    
Class Test/cautious -superclass TestSuite
Test/cautious instproc init {} {
    $self instvar net_ test_
    set net_ net4
    set test_ cautious
    Queue/RED set cautious_ 1
    Test/cautious instproc run {} [Test/notcautious info instbody run ]
    $self next pktTraceFile
}

Class Test/cautious2 -superclass TestSuite
Test/cautious2 instproc init {} {
    $self instvar net_ test_
    set net_ net4
    set test_ cautious2
    Queue/RED set cautious_ 2
    Test/cautious2 instproc run {} [Test/notcautious info instbody run ]
    $self next pktTraceFile
}

Class Test/cautious3 -superclass TestSuite
Test/cautious3 instproc init {} {
    $self instvar net_ test_
    set net_ net4
    set test_ cautious3
    Queue/RED set cautious_ 3
    Queue/RED set idle_pktsize_ 100
    Test/cautious3 instproc run {} [Test/notcautious info instbody run ]
    $self next pktTraceFile
}


TestSuite runTest
