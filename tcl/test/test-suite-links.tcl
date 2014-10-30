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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-links.tcl,v 1.21 2006/01/24 23:00:06 sallyfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
source support.tcl
Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr 90 * 512 + 40]

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

#
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
    $ns duplex-link $node_(r1) $node_(k1) 80Kb 10ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8
}

Class Topology/net4a -superclass Topology
Topology/net4a instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 100Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 100Mb 0ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 10Mb 10ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 100
    $ns queue-limit $node_(k1) $node_(r1) 100
}


TestSuite instproc finish file {
	global quiet wrap PERL
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m $wrap -t $file > temp.rands
        exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
          $PERL ../../bin/raw2xg -a -c -p -s 0.01 -m $wrap -t $file >> \
          temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
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

TestSuite instproc setup {tcptype} {
}

# Definition of test-suite tests

###################################################
## Plain link
###################################################

Class Test/links -superclass TestSuite
Test/links instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	links
	$self next pktTraceFile
}
Test/links instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

        ###$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Link with changing delay.
###################################################

Class Test/changeDelay -superclass TestSuite
Test/changeDelay instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	changeDelay
	$self next pktTraceFile
}
Test/changeDelay instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
	$ns_ at 2.0 "$ns_ delay $node_(r1) $node_(k1) 1000ms duplex"
	$ns_ at 8.0 "$ns_ delay $node_(r1) $node_(k1) 100ms duplex"

        $self tcpDump $tcp1 5.0

        ##$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Link with changing delay, showing reordering.
###################################################

Class Test/changeDelay1 -superclass TestSuite
Test/changeDelay1 instproc init {} {
	$self instvar net_ test_
	set net_	net4a
	set test_	changeDelay1
	$self next pktTraceFile
}
Test/changeDelay1 instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) $fid]
        $tcp1 set window_ 80
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

	$ns_ at 0.0 "$ns_ delay $node_(r1) $node_(k1) 100ms duplex"
	$ns_ at 2.0 "$ns_ delay $node_(r1) $node_(k1) 1ms duplex"
	$ns_ at 2.2 "$ns_ delay $node_(r1) $node_(k1) 100ms duplex"

        $self tcpDump $tcp1 5.0

	$ns_ at 4.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Link with changing delay, with no reordering.
###################################################

Class Test/changeDelay2 -superclass TestSuite
Test/changeDelay2 instproc init {} {
	$self instvar net_ test_
	set net_	net4a
	set test_	changeDelay2
	DelayLink set avoidReordering_ true
	Test/changeDelay2 instproc run {} [Test/changeDelay1 info instbody run ]
	$self next pktTraceFile
}

###################################################
## Link with changing bandwidth.
###################################################

Class Test/links1 -superclass TestSuite
Test/links1 instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	links1
	$self next pktTraceFile
}
Test/links1 instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

        ##$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

Class Test/changeBandwidth -superclass TestSuite
Test/changeBandwidth instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	changeBandwidth
	$self next pktTraceFile
}
Test/changeBandwidth instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
	$ns_ at 2.0 "$ns_ bandwidth $node_(r1) $node_(k1) 8Kb duplex"
	$ns_ at 8.0 "$ns_ bandwidth $node_(r1) $node_(k1) 800Kb duplex"

        $self tcpDump $tcp1 5.0

        ##$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Two dropped packets.  
###################################################

Class Test/dropPacket -superclass TestSuite
Test/dropPacket instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	dropPacket
	Agent/TCP set minrto_ 1
	$self next pktTraceFile
}
Test/dropPacket instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo
	$self June01defaults

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 1]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
        $self dropPkts [$ns_ link $node_(r1) $node_(k1)] 1 {20 22}
	# dropPkts is in support.tcl

        $self tcpDump $tcp1 5.0

	$ns_ at 5.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Two delayed packets.  
###################################################
Class Test/delayPacket -superclass TestSuite
Test/delayPacket instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	delayPacket
	Agent/TCP set minrto_ 1
	ErrorModel set delay_pkt_ true
	ErrorModel set drop_ false
	ErrorModel set delay_ 0.3
	Test/delayPacket instproc run {} [Test/dropPacket info instbody run ]
	$self next pktTraceFile
}

###################################################
## Two delayed packets with different delays
###################################################
Class Test/delayPacket1 -superclass TestSuite
Test/delayPacket1 instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	delayPacket1
	Agent/TCP set minrto_ 1
	$self next pktTraceFile
}
Test/delayPacket1 instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo
	$self June01defaults

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 1]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self dropPkts [$ns_ link $node_(r1) $node_(k1)] 1 {20} true 0.3
	$self dropPkts [$ns_ link $node_(r1) $node_(k1)] 1 {22} true 0.4
	# dropPkts is in support.tcl

        $self tcpDump $tcp1 5.0

	$ns_ at 5.0 "$self cleanupAll $testName_"
        $ns_ run
}


###################################################
## Delayer module 
###################################################

## Link r1-k1 blocked at time 2, unblocked at time 4.
Class Test/delaySpike -superclass TestSuite
Test/delaySpike instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	delaySpike
	$self next pktTraceFile
}
Test/delaySpike instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_ 
	$self setTopo

	set d1 [new Delayer]
	set d2 [new Delayer]
	$ns_ insert-delayer $node_(k1) $node_(r1) $d2
	$ns_ insert-delayer $node_(r1) $node_(k1) $d1
	$ns_ at 2.0 "$d1 block"
	$ns_ at 2.0 "$d2 block"
	$ns_ at 4.0 "$d1 unblock"
	$ns_ at 4.0 "$d2 unblock"

	set fid 1
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

## Link r1-k1 blocked at time 2, unblocked at time 4.
## TCP connection has a window of 8.
Class Test/delaySpike1 -superclass TestSuite
Test/delaySpike1 instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	delaySpike1
	$self next pktTraceFile
}
Test/delaySpike1 instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_ 
	$self setTopo

	set d1 [new Delayer]
	set d2 [new Delayer]
	$ns_ insert-delayer $node_(k1) $node_(r1) $d2
	$ns_ insert-delayer $node_(r1) $node_(k1) $d1
	$ns_ at 2.0 "$d1 block"
	$ns_ at 2.0 "$d2 block"
	$ns_ at 4.0 "$d1 unblock"
	$ns_ at 4.0 "$d2 unblock"

	set fid 1
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) $fid]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

## link r1-k1 has delay spikes, on average for 0.1 seconds, every 0.5 seconds
Class Test/delaySpikes -superclass TestSuite
Test/delaySpikes instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	delaySpikes
	$self next pktTraceFile
}
Test/delaySpikes instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set d1 [new Delayer]
	$ns_ insert-delayer $node_(r1) $node_(k1) $d1
	set len [new RandomVariable/Exponential]
	$len set avg_ 0.1
	set int [new RandomVariable/Exponential]
	$int set avg_ 0.5

	$d1 spike $int $len

	set fid 1
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

## link r1-k1 has delay spikes, on average for 0.1 seconds, every 0.5 seconds
## TCP connection has a window of 8.
Class Test/delaySpikes1 -superclass TestSuite
Test/delaySpikes1 instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	delaySpikes1
	$self next pktTraceFile
}
Test/delaySpikes1 instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set d1 [new Delayer]
	$ns_ insert-delayer $node_(r1) $node_(k1) $d1
	set len [new RandomVariable/Exponential]
	$len set avg_ 0.1
	set int [new RandomVariable/Exponential]
	$int set avg_ 0.5

	$d1 spike $int $len

	set fid 1
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) $fid]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

## Channel channelAllocDelayation delay when the queue is empty.
Class Test/channelAllocDelay -superclass TestSuite
Test/channelAllocDelay instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	channelAllocDelay
	$self next pktTraceFile
}
Test/channelAllocDelay instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set d1 [new Delayer]
	set d2 [new Delayer]
	$ns_ insert-delayer $node_(k1) $node_(r1) $d1
	$ns_ insert-delayer $node_(r1) $node_(k1) $d2

	set len_dl [new RandomVariable/Uniform]
	$len_dl set min_ 0.16
	$len_dl set max_ 0.19
	set int_dl [new RandomVariable/Uniform]
	$int_dl set min_ 2
	$int_dl set max_ 5

	set len_ul [new RandomVariable/Uniform]
	$len_ul set min_ 0.5
	$len_ul set max_ 0.6
	set int_ul [new RandomVariable/Uniform]
	$int_ul set min_ 0.01
	$int_ul set max_ 0.4

	$d1 alloc $int_ul $len_ul
	$d2 alloc $int_dl $len_dl

	set fid 1
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

	$ns_ at 10.0 "$self cleanupAll $testName_"
	$ns_ run
}

## Channel allocation delay when the queue is empty.
## Add a second connection on the reverse path.
Class Test/channelAllocDelay1 -superclass TestSuite
Test/channelAllocDelay1 instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	channelAllocDelay1
	$self next pktTraceFile
}
Test/channelAllocDelay1 instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set d1 [new Delayer]
	set d2 [new Delayer]
	$ns_ insert-delayer $node_(k1) $node_(r1) $d1
	$ns_ insert-delayer $node_(r1) $node_(k1) $d2

	set len_dl [new RandomVariable/Uniform]
	$len_dl set min_ 0.16
	$len_dl set max_ 0.19
	set int_dl [new RandomVariable/Uniform]
	$int_dl set min_ 2
	$int_dl set max_ 5
	$d2 alloc $int_dl $len_dl

	set len_ul [new RandomVariable/Uniform]
	$len_ul set min_ 0.5
	$len_ul set max_ 0.6
	set int_ul [new RandomVariable/Uniform]
	$int_ul set min_ 0.01
	$int_ul set max_ 0.4
	$d1 alloc $int_ul $len_ul

	# int: time of keeping a channel without data.
 	# len: delay in allocating a new channel.

	set fid 1
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

	set fid 2
	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(k1) TCPSink/Sack1 $node_(s1) $fid]
        $tcp2 set window_ 3
        set ftp2 [$tcp2 attach-app FTP]
        $ns_ at 3.0 "$ftp2 start"

        $self tcpDump $tcp1 5.0

	$ns_ at 10.0 "$self cleanupAll $testName_"
	$ns_ run
}

#
# Does not drop extra packets immediately when queue size is lowered.
#
Class Test/change_queue -superclass TestSuite
Test/change_queue instproc init {} {
        $self instvar net_ test_
        set net_        net4
        set test_       change_queue
        $self next pktTraceFile
}
Test/change_queue instproc run {} {
        global wrap wrap1
        $self instvar ns_ node_ testName_
        $self setTopo

        set fid 1
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
	$tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        set link1 [$ns_ link $node_(r1) $node_(k1)]
        set queue1 [$link1 queue]
        $ns_ at 5.0 "$ns_ queue-limit $node_(r1) $node_(k1) 5"

        $self tcpDump $tcp1 5.0
        $ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

#
# Drops extra packets when queue size is lowered.
# Validation test from Andrei Gurtov.
#
Class Test/queue_shrink -superclass TestSuite
Test/queue_shrink instproc init {} {
        $self instvar net_ test_
        set net_        net4
        set test_       queue_shrink
        $self next pktTraceFile
}
Test/queue_shrink instproc run {} {
        global wrap wrap1
        $self instvar ns_ node_ testName_
        $self setTopo

        set fid 1
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
	$tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        set link1 [$ns_ link $node_(r1) $node_(k1)]
        set queue1 [$link1 queue]
        $ns_ at 5.0 "$ns_ queue-limit $node_(r1) $node_(k1) 5; $queue1 shrink-queue"

        $self tcpDump $tcp1 5.0
        $ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}


TestSuite runTest

