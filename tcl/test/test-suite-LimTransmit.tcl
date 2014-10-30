#
# Copyright (c) 1995 The Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-LimTransmit.tcl,v 1.15 2010/04/03 20:40:15 tom_henderson Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for TCP

Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Trace set show_tcphdr_ 1
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set tcpTick_ 0.5
## First scenaio: maxpkts 15, droppkt 4.
## For the paper: droppkt 2.

Agent/TCPSink set SYN_immediate_ack_ false
# The default was changed to true 2010/02.

set wrap 90
set wrap1 [expr 90 * 512 + 40]

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 10ms bottleneck.
# Queue-limit on bottleneck is 2 packets.
#
Class Topology/net4 -superclass Topology
Topology/net4 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    # 800Kb/sec = 100 pkts/sec = 20 pkts/200 ms.
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
}


TestSuite instproc finish file {
	global quiet wrap PERL
        exec $PERL ../../bin/set_flow_id -m all.tr > t
        exec $PERL ../../bin/getrc -s 0 -d 2 t > t1 
        exec  $PERL ../../bin/raw2xg -c -a -s 0.01 -m $wrap -t $file t1 > temp.rands
	exec $PERL ../../bin/getrc -s 2 -d 0 t > t2
        exec $PERL ../../bin/raw2xg -c -a -s 0.01 -m $wrap -t $file t2 > temp1.rands
        exec $PERL ../../bin/getrc -s 2 -d 3 t | \
          $PERL ../../bin/raw2xg -c -d -s 0.01 -m $wrap -t $file > temp2.rands
        #exec $PERL ../../bin/set_flow_id -s all.tr | \
        #  $PERL ../../bin/getrc -s 2 -d 3 | \
        #  $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
		temp1.rands temp2.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
        # exec csh gnuplotC.com temp.rands temp1.rands temp2.rands $file
        exit 0
}

TestSuite instproc printtimers { tcp time} {
	global quiet
	if {$quiet == "false"} {
        	puts "time: $time sRTT(in ticks): [$tcp set srtt_]/8 RTTvar(in ticks): [$tcp set rttvar_]/4 backoff: [$tcp set backoff_]"
	}
}

TestSuite instproc printtimersAll { tcp time interval } {
        $self instvar dump_inst_ ns_
        if ![info exists dump_inst_($tcp)] {
                set dump_inst_($tcp) 1
                $ns_ at $time "$self printtimersAll $tcp $time $interval"
                return
        }
	set newTime [expr [$ns_ now] + $interval]
	$ns_ at $time "$self printtimers $tcp $time"
        $ns_ at $newTime "$self printtimersAll $tcp $newTime $interval"
}


TestSuite instproc emod {} {
        $self instvar topo_
        $topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        return $errmodule
} 

TestSuite instproc drop_pkts { pkts {ecn 0}} {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    if {$ecn == "ECN"} {
    	$errmodel1 set markecn_ true
    }
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1
}

TestSuite instproc setup { tcptype list {ecn 0}} {
	global wrap wrap1
        $self instvar ns_ node_ testName_ guide_
	$self setTopo
	puts "Guide: $guide_"

        Agent/TCP set bugFix_ false
	if {$ecn == "ECN"} {
		 Agent/TCP set ecn_ 1
	}
	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Tahoe"} {
      		set tcp1 [$ns_ create-connection TCP $node_(s1) \
          	TCPSink/DelAck $node_(k1) $fid]
    	} elseif {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1/DelAck  $node_(k1) $fid]
    	} else {
      		set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
          	TCPSink/DelAck $node_(k1) $fid]
    	}
        $tcp1 set window_ 28
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 produce 7"

        $self tcpDump $tcp1 5.0
        $self drop_pkts $list $ecn

        #$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
	$ns_ at 6.0 "$self cleanupAll $testName_"
        $ns_ run
}

# Definition of test-suite tests

###################################################
## One drop
###################################################

Class Test/onedrop_sack -superclass TestSuite
Test/onedrop_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_sack
	set guide_      "Sack TCP, no Limited Transmit, one packet drop, so timeout."
	$self next pktTraceFile
}
Test/onedrop_sack instproc run {} {
        $self setup Sack1 {1}
}

Class Test/onedrop_SA_sack -superclass TestSuite
Test/onedrop_SA_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_SA_sack
	set guide_      "Sack TCP, Limited Transmit, one packet drop, so Fast Retransmit."
	Agent/TCP set singledup_ 1
	Test/onedrop_SA_sack instproc run {} [Test/onedrop_sack info instbody run ]
	$self next pktTraceFile
}

Class Test/onedrop_ECN_sack -superclass TestSuite
Test/onedrop_ECN_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_ECN_sack
	set guide_      "Sack TCP, ECN, one packet drop, so Fast Retransmit."
	Agent/TCP set ecn_ 1
	$self next pktTraceFile
}
Test/onedrop_ECN_sack instproc run {} {
        $self setup Sack1 {1} ECN
}

Class Test/nodrop_sack -superclass TestSuite
Test/nodrop_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	nodrop_sack
	set guide_      "Sack TCP with no packet drops, for comparison."
	$self next pktTraceFile
}
Test/nodrop_sack instproc run {} {
        $self setup Sack1 {1000} 
}

TestSuite runTest
