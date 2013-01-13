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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-tcpReset.tcl,v 1.15 2010/04/03 20:40:15 tom_henderson Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcp.tcl
#

source misc.tcl
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
source topologies.tcl
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Agent/TCPSink set SYN_immediate_ack_ false
# The default was changed to true 2010/02.

TestSuite instproc finish file {
	global quiet PERL
        exec $PERL ../../bin/set_flow_id -s all.tr | \
          $PERL ../../bin/getrc -s 2 -d 3 | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
	## regsub \\(.+\\) $file {} filename
	## exec csh gnuplotA.com temp.rands $filename
        exit 0
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 20ms bottleneck.
# Queue-limit on bottleneck is 25 packets.
#
Class Topology/net6 -superclass NodeTopology/4nodes
Topology/net6 instproc init ns {
    $self next $ns
    $self instvar node_
    Queue/RED set setbit_ true
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 20ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

# 
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
# 
TestSuite instproc tcpDump { tcpSrc interval } {
        global quiet
        $self instvar dump_inst_ ns_
        if ![info exists dump_inst_($tcpSrc)] {
                set dump_inst_($tcpSrc) 1
                $ns_ at 0.0 "$self tcpDump $tcpSrc $interval"
                return
        }
        $ns_ at [expr [$ns_ now] + $interval] "$self tcpDump $tcpSrc $interval"
        set report [$ns_ now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]
        if {$quiet == "false"} {
                puts $report
        }
}


# Definition of test-suite tests

Class Test/reset -superclass TestSuite
Test/reset instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	reset
	set guide_	"Resetting a TCP connection."
	$self next
}
Test/reset instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 1.1 "$tcp1 reset"
	$ns_ at 1.1 "$sink reset"
	$ns_ at 1.5 "$ftp1 producemore 20"
        $self tcpDump $tcp1 1.0

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetDelAck -superclass TestSuite
Test/resetDelAck instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	resetDelAck
	set guide_	"Resetting a DelAck TCP connection."
	$self next
}
Test/resetDelAck instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink/DelAck $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 1.1 "$tcp1 reset"
	$ns_ at 1.1 "$sink reset"
	$ns_ at 1.5 "$ftp1 producemore 20"

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetDelAck1 -superclass TestSuite
Test/resetDelAck1 instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	resetDelAck1
	set guide_	\
	"Resetting a DelAck TCP connection, different timing between connections."
	$self next
}
Test/resetDelAck1 instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink/DelAck $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.41 "$tcp1 reset"
	$ns_ at 0.41 "$sink reset"
	$ns_ at 5.00 "$ftp1 producemore 20"

	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/reset1 -superclass TestSuite
Test/reset1 instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	reset1
	set guide_	\
	"Resetting a TCP connection."
	$self next
}
Test/reset1 instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.2 "$tcp1 reset"
	$ns_ at 0.2 "$sink reset"
	$ns_ at 0.201 "$ftp1 producemore 20"
        $self tcpDump $tcp1 1.0

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetDelAck2 -superclass TestSuite
Test/resetDelAck2 instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	resetDelAck2
	set guide_	\
	"Resetting a TCP connection when there is unsent data."
	$self next
}
Test/resetDelAck2 instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink/DelAck $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.32 "$tcp1 reset"
	$ns_ at 0.32 "$sink reset"
	$ns_ at 0.321 "$ftp1 producemore 20"

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetDelAck3 -superclass TestSuite
Test/resetDelAck3 instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	resetDelAck3
	set guide_	\
	"Resetting a TCP connection when there is unsent data."
	$self next
}
Test/resetDelAck3 instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink/DelAck $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.32 "$tcp1 reset"
	$ns_ at 0.32 "$sink reset"
	$ns_ at 0.321 "$ftp1 producemore 20"

	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/resetNewreno -superclass TestSuite
Test/resetNewreno instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	resetNewreno
	set guide_	\
	"NewReno TCP, resetting a TCP connection."
	$self next
}
Test/resetNewreno instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP/Newreno $node_(s1) TCPSink $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.2 "$tcp1 reset"
	$ns_ at 0.2 "$sink reset"
	$ns_ at 0.201 "$ftp1 producemore 20"
        $self tcpDump $tcp1 1.0

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetSack1 -superclass TestSuite
Test/resetSack1 instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	resetSack1
	set guide_	\
	"Sack TCP, resetting a TCP connection."
	$self next
}
Test/resetSack1 instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.2 "$tcp1 reset"
	$ns_ at 0.2 "$sink reset"
	$ns_ at 0.201 "$ftp1 producemore 20"
        $self tcpDump $tcp1 1.0

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetReno -superclass TestSuite
Test/resetReno instproc init topo {
	$self instvar net_ defNet_ test_ guide_
	set net_	$topo
	set defNet_	net6
	set test_	resetReno
	set guide_	\
	"Reno TCP, resetting a TCP connection."
	$self next
}
Test/resetReno instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP/Reno $node_(s1) TCPSink $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.2 "$tcp1 reset"
	$ns_ at 0.2 "$sink reset"
	$ns_ at 0.201 "$ftp1 producemore 20"
        $self tcpDump $tcp1 1.0

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}


TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
