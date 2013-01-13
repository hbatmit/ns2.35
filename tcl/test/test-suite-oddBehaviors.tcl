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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-oddBehaviors.tcl,v 1.16 2007/10/24 02:13:50 sallyfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

Agent/TCP set singledup_ 0

Trace set show_tcphdr_ 1

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
        exec $PERL ../../bin/set_flow_id -s all.tr | \
          $PERL ../../bin/getrc -s 2 -d 3 | \
          $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
         exec $PERL ../../bin/set_flow_id -d all.tr | \
          $PERL ../../bin/getrc -s 3 -d 2 | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m $wrap -t $file > temp1.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
			temp1.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
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

TestSuite instproc drop_pkts pkts {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1
}

TestSuite instproc setup {tcptype list} {
	global wrap wrap1
        $self instvar ns_ node_ testName_ guide_
	$self setTopo

        Agent/TCP set bugFix_ false
        puts "Guide: $guide_"
	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Tahoe"} {
      		set tcp1 [$ns_ create-connection TCP $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	} elseif {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "Fack"} {
      		set tcp1 [$ns_ create-connection TCP/Fack $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "SackRH"} {
      		set tcp1 [$ns_ create-connection TCP/SackRH $node_(s1) \
          	TCPSink/Sack1 $node_(k1) $fid]
    	} elseif {$tcptype == "FullTcp"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp]
	        set sink [new Agent/TCP/FullTcp]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpTahoe"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Tahoe]
	        set sink [new Agent/TCP/FullTcp/Tahoe]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpNewreno"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Newreno]
	        set sink [new Agent/TCP/FullTcp/Newreno]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpSack1"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Sack]
	        set sink [new Agent/TCP/FullTcp/Sack]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} else {
      		set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	}
        $tcp1 set window_ 50
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 2.0
        $self drop_pkts $list

        #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
	$ns_ at 2.0 "$self cleanupAll $testName_"
        $ns_ run
}

# Definition of test-suite tests

###################################################
## One drop, numdupacks
###################################################

# cwnd is 4 when a packet is dropped.
# When three dup acks come in, cwnd in halved to 2.  
# dupwnd_ is set # to three, 
# so the sender retransmits the lost packet, 
# and also sends a new packet off the top, because
# cwnd has been "inflated" by the three dup acks.
# This occurs, appropriately, with both Reno and Newreno.
# This does not occur for Sack, because Sack only sends one packet
# in response to the third dup ack.
#
Class Test/onedrop_reno -superclass TestSuite
Test/onedrop_reno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_reno
        set guide_      "Reno, inflated congestion window after Fast Retransmit."
	$self next pktTraceFile
}
Test/onedrop_reno instproc run {} {
        $self setup Reno {3}
}

Class Test/onedrop_sack -superclass TestSuite
Test/onedrop_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_sack
        set guide_      "Sack, no inflated congestion window after Fast Retransmit."
	$self next pktTraceFile
}
Test/onedrop_sack instproc run {} {
        $self setup Sack1 {3}
}

Class Test/onedrop_sack1 -superclass TestSuite
Test/onedrop_sack1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_sack1
	Agent/TCP set singledup_ 1
        set guide_      "Sack, Limited Transmit, inflated congestion window."
	$self next pktTraceFile
}
Test/onedrop_sack1 instproc run {} {
        $self setup Sack1 {3}
}

TestSuite runTest
