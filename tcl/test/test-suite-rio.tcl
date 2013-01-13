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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-rio.tcl,v 1.19 2006/01/24 23:00:07 sallyfloyd Exp $
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., 
# Ns Simulator Tests for Random Early Detection (RED), October 1996.
# URL ftp://ftp.ee.lbl.gov/papers/redsims.ps.Z.
#
# To run all tests: test-all-red

set dir [pwd]
catch "cd tcl/test"
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
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5 
Queue/RED set maxthresh_ 15
# The RED parameter defaults are being changed for automatic configuration.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
catch "cd $dir"
Agent/TCP set oldCode_ true
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

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
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED/RIO
    #$ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
 
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up
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

#
# This test uses priority_method_ 1, so that flows with FlowID 0 have
# priority over other flows.
# OUT packets are dropped before any IN packets are dropped.
#
Class Test/strict -superclass TestSuite
Test/strict instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ strict
    Queue/RED/RIO set in_thresh_ 10
    Queue/RED/RIO set in_maxthresh_ 20
    Queue/RED/RIO set out_thresh_ 3
    Queue/RED/RIO set out_maxthresh_ 9
    Queue/RED/RIO set in_linterm_ 10
    Queue/RED/RIO set linterm_ 10
    Queue/RED/RIO set priority_method_ 1
    #Queue/RED/RIO set debug_ true
    Queue/RED/RIO set debug_ false
    $self next pktTraceFile
}
Test/strict instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo

    set stoptime 20.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 50
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set window_ 50
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]
    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

#
# OUT packets are four times more likely to be dropped than IN packets. 
#
Class Test/proportional -superclass TestSuite
Test/proportional instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ proportional
    Queue/RED/RIO set in_thresh_ 3
    Queue/RED/RIO set in_maxthresh_ 15
    Queue/RED/RIO set out_thresh_ 3
    Queue/RED/RIO set out_maxthresh_ 15
    Queue/RED/RIO set in_linterm_ 3
    Queue/RED/RIO set linterm_ 12
    Queue/RED/RIO set priority_method_ 1
    Test/proportional instproc run {} [Test/strict info instbody run]
    $self next pktTraceFile
}

#
# OUT packets are four times more likely to be dropped than IN packets. 
#
Class Test/gentle -superclass TestSuite
Test/gentle instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ gentle
    Queue/RED/RIO set in_thresh_ 3
    Queue/RED/RIO set in_maxthresh_ 15
    Queue/RED/RIO set out_thresh_ 3
    Queue/RED/RIO set out_maxthresh_ 15
    Queue/RED/RIO set in_linterm_ 50
    Queue/RED/RIO set linterm_ 200
    Queue/RED/RIO set priority_method_ 1
    Queue/RED/RIO set gentle_ false
    Queue/RED/RIO set in_gentle_ true
    Queue/RED/RIO set out_gentle_ true
    Test/gentle instproc run {} [Test/strict info instbody run]
    $self next pktTraceFile
}

#
# OUT packets are four times more likely to be dropped than IN packets. 
#
Class Test/notGentle -superclass TestSuite
Test/notGentle instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ notGentle
    Queue/RED/RIO set in_thresh_ 3
    Queue/RED/RIO set in_maxthresh_ 15
    Queue/RED/RIO set out_thresh_ 3
    Queue/RED/RIO set out_maxthresh_ 15
    Queue/RED/RIO set in_linterm_ 50
    Queue/RED/RIO set linterm_ 200
    Queue/RED/RIO set priority_method_ 1
    Queue/RED/RIO set gentle_ false
    Queue/RED/RIO set in_gentle_ false
    Queue/RED/RIO set out_gentle_ false
    Test/notGentle instproc run {} [Test/strict info instbody run]
    $self next pktTraceFile
}

#
# This test uses priority_method_ 0, with token bucket policing
# and tagging.
#
Class Test/tagging -superclass TestSuite
Test/tagging instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ tagging
    Queue/RED/RIO set in_thresh_ 10
    Queue/RED/RIO set in_maxthresh_ 20
    Queue/RED/RIO set out_thresh_ 3
    Queue/RED/RIO set out_maxthresh_ 9
    Queue/RED/RIO set in_linterm_ 10
    Queue/RED/RIO set linterm_ 10
    Queue/RED/RIO set priority_method_ 0
    $self next pktTraceFile
}
Test/tagging instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo

    set stoptime 20.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 50
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set window_ 50
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    # make token bucket limiter for flow 0
    # Fill rate 100000 Bps, or 100 packets per second.
    set link1 [$ns_ link $node_(s1) $node_(r1)]
    set tcm1 [$ns_ maketbtagger Fid]
    $ns_ attach-tagger $link1 $tcm1
    set fcl1 [$tcm1 classifier]; # flow classifier
    $fcl1 set-flowrate 0 100000 10000 1
    #target_rate_ (fill rate, in Bps), 
    #bucket_depth_, 
    #tbucket_ (current bucket size, in bytes) 
    
    # make token bucket limiter for flow 1
    # Fill rate 1000000 Bps, or 1000 packets per second.
    set link2 [$ns_ link $node_(s2) $node_(r1)]
    set tcm2 [$ns_ maketbtagger Fid]
    $ns_ attach-tagger $link2 $tcm2
    set fcl2 [$tcm2 classifier]; # flow classifier
    $fcl2 set-flowrate 1 1000000 10000 1

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}


TestSuite runTest
