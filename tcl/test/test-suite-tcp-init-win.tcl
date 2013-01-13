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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-tcp-init-win.tcl,v 1.35 2006/08/12 23:34:24 sallyfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcp.tcl
#

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for TCP

# FOR UPDATING GLOBAL DEFAULTS:
Queue/RED set bytes_ false
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5
Queue/RED set maxthresh_ 15

set plotacks false

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/net6 -superclass Topology
Topology/net6 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
    
    Queue/RED set setbit_ true
    $ns duplex-link $node_(s1) $node_(r1) 100Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 100Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 10Mb 100ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
}   
    
Class Topology/net7 -superclass Topology
Topology/net7 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
    
    Queue/RED set setbit_ true
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
}   
    
Class Topology/net8 -superclass Topology
Topology/net8 instproc init ns {
    $self instvar node_
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    
    Queue/RED set setbit_ true
    $ns duplex-link $node_(s1) $node_(s2) 1000Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 9.6Kb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 10ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
}   
    
TestSuite instproc finish file {
	global quiet PERL plotacks
        exec $PERL ../../bin/getrc -e -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -v -s 0.01 -m 90 -t $file > temp.rands
        if {$plotacks == "true"} {
	  exec $PERL ../../bin/getrc -e -s 3 -d 2 all.tr | \
	    $PERL ../../bin/raw2xg -v -a -e -s 0.01 -m 90 -t $file > temp1.rands
	}
	if {$quiet == "false"} {
		if {$plotacks == "false"} {
		   exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
                } else {
		   exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
			       temp1.rands &
		}
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

# Definition of test-suite tests

TestSuite instproc run_test {tcp1 tcp2 tcp3 dumptime runtime window} {
	$self instvar ns_ node_ testName_ 

	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 0.0 "$ftp2 start"
	set ftp3 [$tcp3 attach-app FTP]
	$ns_ at 0.0 "$ftp3 start"
	$self runall_test $tcp1 $dumptime $runtime
}

TestSuite instproc runall_test {tcp1 dumptime runtime} {
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"

	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 $dumptime
	$ns_ at $runtime "$self cleanupAll $testName_"
	$ns_ run
}

TestSuite instproc second_test {tcp1 tcp2} {
	$self instvar ns_ node_ testName_ guide_
	puts "Guide: $guide_"
	$tcp1 set window_ 40
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 1.0 "$ftp1 start"

	$tcp2 set window_ 40
	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 0.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0
	$ns_ at 10.0 "$self cleanupAll $testName_"
	$ns_ run
}

TestSuite instproc make_tcp {nodeA nodeB ID type} {
	$self instvar ns_ node_ 
        if {$type == "Tahoe"} {
		set tcp [$ns_ create-connection TCP $node_($nodeA) TCPSink $node_($nodeB) $ID]
	}
        if {$type == "Reno"} {
		set tcp [$ns_ create-connection TCP/Reno $node_($nodeA) TCPSink $node_($nodeB) $ID]
	}
        if {$type == "Newreno"} {
		set tcp [$ns_ create-connection TCP/Newreno $node_($nodeA) TCPSink $node_($nodeB) $ID]
	}
        if {$type == "Sack"} {
		set tcp [$ns_ create-connection TCP/Sack1 $node_($nodeA) TCPSink/Sack1 $node_($nodeB) $ID]
	}
        if {$type == "SackDelAck"} {
		set tcp [$ns_ create-connection TCP/Sack1 $node_($nodeA) TCPSink/Sack1/DelAck $node_($nodeB) $ID]
	}
	return $tcp
}

Class Test/tahoe1 -superclass TestSuite
Test/tahoe1 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	tahoe1(variable_packet_sizes)
	set guide_	"Tahoe TCP, initial windows with different packet sizes."
        $self next pktTraceFile
}
Test/tahoe1 instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
 	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Tahoe]
	$tcp1 set packetSize_ 1000
	set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$tcp2 set packetSize_ 1500
	set tcp3 [$self make_tcp s2 k1 2 Tahoe]
	$tcp3 set packetSize_ 2500
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/tahoe2 -superclass TestSuite
Test/tahoe2 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net6
	set test_	tahoe2(static_initial_windows)
	set guide_	"Tahoe TCP, static initial windows."
	$self next pktTraceFile
}
Test/tahoe2 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
	$tcp1 set windowInit_ 6
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$tcp2 set syn_ true
	$tcp2 set delay_growth_ true
	set tcp3 [$self make_tcp s2 k1 2 Tahoe]
	$tcp3 set windowInit_ 6
	$tcp3 set syn_ true
	$tcp3 set delay_growth_ true
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/tahoe3 -superclass TestSuite
Test/tahoe3 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	tahoe3(dropped_syn)
	set guide_	"Tahoe TCP, initial window after a dropped SYN packet."
        $self next pktTraceFile
}

## Drop the n-th packet for flow on link.
TestSuite instproc drop_pkt { link flow num } {
	set em [new ErrorModule Fid]
	set errmodel [new ErrorModel/Periodic]
	$errmodel unit pkt
	$errmodel set offset_ $num
	$errmodel set period_ 1000.0
	$link errormodule $em
	$em insert $errmodel
	$em bind $errmodel $flow
}

Test/tahoe3 instproc run {} {
        $self instvar ns_ node_ testName_ guide_
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Tahoe]
	$tcp1 set packetSize_ 1000
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 10.0 10.0 
}

Class Test/tahoe4 -superclass TestSuite
Test/tahoe4 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net7
	set test_	tahoe4(fast_retransmit)
	set guide_	"Tahoe TCP, window after a Fast Retransmit."
	Queue/RED set ns1_compat_ true
	$self next pktTraceFile
}
Test/tahoe4 instproc run {} {
	$self instvar ns_ node_ testName_ guide_
	$self setTopo
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2

        set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
        set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$self second_test $tcp1 $tcp2
}

Class Test/reno1 -superclass TestSuite
Test/reno1 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	reno1(variable_packet_sizes)
	set guide_	"Reno TCP, initial windows with different packet sizes."
        $self next pktTraceFile
}
Test/reno1 instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Reno]
	$tcp1 set packetSize_ 1000
	set tcp2 [$self make_tcp s2 k1 1 Reno]
	$tcp2 set packetSize_ 1500
	set tcp3 [$self make_tcp s2 k1 2 Reno]
	$tcp3 set packetSize_ 2500
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/reno2 -superclass TestSuite
Test/reno2 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net6
	set test_	reno2(static_initial_windows)
	set guide_	"Reno TCP, static initial windows."
	$self next pktTraceFile
}
Test/reno2 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	set tcp1 [$self make_tcp s1 k1 0 Reno] 
	$tcp1 set windowInit_ 6
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	set tcp2 [$self make_tcp s2 k1 1 Reno]
	$tcp2 set syn_ true
	$tcp2 set delay_growth_ true
	set tcp3 [$self make_tcp s2 k1 2 Reno]
	$tcp3 set windowInit_ 6
	$tcp3 set syn_ true
	$tcp3 set delay_growth_ true
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/reno3 -superclass TestSuite
Test/reno3 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	reno3(dropped_syn)
	set guide_	"Reno TCP, initial window after a dropped SYN packet."
        $self next pktTraceFile
}

Test/reno3 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Reno]
	$tcp1 set packetSize_ 1000
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 10.0 10.0 
}

Class Test/reno4 -superclass TestSuite
Test/reno4 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net7
	set test_	reno4(fast_retransmit)
	set guide_	"Reno TCP, window after a Fast Retransmit."
	Queue/RED set ns1_compat_ true
	$self next pktTraceFile
}
Test/reno4 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2

        set tcp1 [$self make_tcp s1 k1 0 Reno] 
        set tcp2 [$self make_tcp s2 k1 1 Reno]
	$self second_test $tcp1 $tcp2
}


Class Test/newreno1 -superclass TestSuite
Test/newreno1 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	newreno1(variable_packet_sizes)
	set guide_	"NewReno TCP, initial windows with different packet sizes."
        $self next pktTraceFile
}
Test/newreno1 instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Newreno]
	$tcp1 set packetSize_ 1000
	set tcp2 [$self make_tcp s2 k1 1 Newreno]
	$tcp2 set packetSize_ 1500
	set tcp3 [$self make_tcp s2 k1 2 Newreno]
	$tcp3 set packetSize_ 2500
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/newreno2 -superclass TestSuite
Test/newreno2 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net6
	set test_	newreno2(static_initial_windows)
	set guide_	"NewReno TCP, static initial windows."
	$self next pktTraceFile
}
Test/newreno2 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	set tcp1 [$self make_tcp s1 k1 0 Newreno] 
	$tcp1 set windowInit_ 6
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	set tcp2 [$self make_tcp s2 k1 1 Newreno]
	$tcp2 set syn_ true
	$tcp2 set delay_growth_ true
	set tcp3 [$self make_tcp s2 k1 2 Newreno]
	$tcp3 set windowInit_ 6
	$tcp3 set syn_ true
	$tcp3 set delay_growth_ true
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/newreno3 -superclass TestSuite
Test/newreno3 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	newreno3(dropped_syn)
	set guide_	"NewReno TCP, initial window after a dropped SYN packet."
        $self next pktTraceFile
}

Test/newreno3 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Newreno]
	$tcp1 set packetSize_ 1000
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 10.0 10.0 
}

Class Test/newreno4 -superclass TestSuite
Test/newreno4 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net7
	set test_	newreno4(fast_retransmit)
	set guide_	"NewReno TCP, window after a Fast Retransmit."
	Queue/RED set ns1_compat_ true
	$self next pktTraceFile
}
Test/newreno4 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2

        set tcp1 [$self make_tcp s1 k1 0 Newreno] 
        set tcp2 [$self make_tcp s2 k1 1 Newreno]
	$self second_test $tcp1 $tcp2
}



Class Test/sack1 -superclass TestSuite
Test/sack1 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	sack1(variable_packet_sizes)
	set guide_	"Sack TCP, initial windows with different packet sizes."
        $self next pktTraceFile
}
Test/sack1 instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Sack]
	$tcp1 set packetSize_ 1000
	set tcp2 [$self make_tcp s2 k1 1 Sack]
	$tcp2 set packetSize_ 1500
	set tcp3 [$self make_tcp s2 k1 2 Sack]
	$tcp3 set packetSize_ 2500
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/sack2 -superclass TestSuite
Test/sack2 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net6
	set test_	sack2(static_initial_windows)
	set guide_	"Sack TCP, static initial windows."
	$self next pktTraceFile
}
Test/sack2 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	set tcp1 [$self make_tcp s1 k1 0 Sack] 
	$tcp1 set windowInit_ 6
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	set tcp2 [$self make_tcp s2 k1 1 Sack]
	$tcp2 set syn_ true
	$tcp2 set delay_growth_ true
	set tcp3 [$self make_tcp s2 k1 2 Sack]
	$tcp3 set windowInit_ 6
	$tcp3 set syn_ true
	$tcp3 set delay_growth_ true
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/sack3 -superclass TestSuite
Test/sack3 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	sack3(dropped_syn)
	set guide_	"Sack TCP, initial window after a dropped SYN packet."
        $self next pktTraceFile
}

Test/sack3 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Sack]
	$tcp1 set packetSize_ 1000
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 10.0 10.0 
}

Class Test/sack4 -superclass TestSuite
Test/sack4 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net7
	set test_	sack4(fast_retransmit)
	set guide_	"Sack TCP, window after a Fast Retransmit."
	Queue/RED set ns1_compat_ true
	$self next pktTraceFile
}
Test/sack4 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2

        set tcp1 [$self make_tcp s1 k1 0 Sack] 
        set tcp2 [$self make_tcp s2 k1 1 Sack]
	$self second_test $tcp1 $tcp2
}

TestSuite instproc printtimers { tcp time} {
        global quiet
        if {$quiet == "false"} {
		set srtt [expr [$tcp set srtt_]/8 ]
		set rttvar [expr [$tcp set rttvar_]/4 ]
		set rto [expr $srtt + 2 * $rttvar ]
                puts "time: $time sRTT(in ticks): $srtt RTTvar(in ticks): $rttvar sRTT+2*RTTvar: $rto backoff: [$tcp set backoff_]" 
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


# This test shows the packets and acknowledgements at the source,
# for a path with a 9.6Kbps link, and 1000-byte packets.
Class Test/slowLink -superclass TestSuite
Test/slowLink instproc init {} {
	global plotacks
	$self instvar net_ test_ guide_ 
	set net_	net8
	set test_	slowLink(9.6K-link,1000-byte-pkt)
	set guide_	"Slow link, with statistics for sRTT, RTTvar, backoff"
        set plotacks true
        $self next pktTraceFile
}

Test/slowLink instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set minrto_ 1
	set tcp1 [$self make_tcp s1 k1 0 Sack]
	$tcp1 set packetSize_ 1000
	$ns_ at 0.0 "$self printtimersAll $tcp1 0 1"
	$self runall_test $tcp1 30.0 30.0 
}

# # This test shows the packets and acknowledgements at the source,
# # for a path with a 9.6Kbps link, and 1000-byte packets.
# Class Test/slowLinkDelayAck -superclass TestSuite
# Test/slowLinkDelayAck instproc init {} {
# 	global plotacks
# 	$self instvar net_ test_ guide_ 
# 	set net_	net8
# 	set test_	slowLinkDelayAck(9.6K-link,1000-byte-pkt)
#         set plotacks true
#         $self next pktTraceFile
# }
# 
# Test/slowLinkDelayAck instproc run {} {
#         $self instvar ns_ node_ testName_ 
# 	$self setTopo
# 	Agent/TCP set windowInitOption_ 2
# 	Agent/TCP set minrto_ 1
# 	set tcp1 [$self make_tcp s1 k1 0 SackDelAck]
# 	$tcp1 set packetSize_ 1000
# 	$ns_ at 0.0 "$self printtimersAll $tcp1 0 1"
# 	$self runall_test $tcp1 30.0 30.0 
# }

# This test shows the packets and acknowledgements at the source,
# for a path with a 9.6Kbps link, and 1500-byte packets.
Class Test/slowLink1 -superclass TestSuite
Test/slowLink1 instproc init {} {
	global plotacks
	$self instvar net_ test_ guide_ 
	set net_	net8
	set test_	slowLink1(9.6K-link,1500-byte-pkt)
	set guide_	"Slow link, with larger packets and a smaller initial window."
        set plotacks true
        $self next pktTraceFile
}

Test/slowLink1 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set minrto_ 1
	set tcp1 [$self make_tcp s1 k1 0 Sack]
	$tcp1 set packetSize_ 1500
	$self runall_test $tcp1 30.0 30.0 
}

# This test shows the packets and acknowledgements at the source,
# for a path with a 9.6Kbps link, and 1500-byte packets.
# Initial window of one packet.
Class Test/slowLink2 -superclass TestSuite
Test/slowLink2 instproc init {} {
	global plotacks
	$self instvar net_ test_ guide_ 
	set net_	net8
	set test_	slowLink2(9.6K-link,1500-byte-pkt)
	set guide_	"Slow link, with an initial window of two packets."
        set plotacks true
        $self next pktTraceFile
}

Test/slowLink2 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 1
	Agent/TCP set minrto_ 1
	set tcp1 [$self make_tcp s1 k1 0 Sack]
	$tcp1 set packetSize_ 1500
	$self runall_test $tcp1 30.0 30.0 
}
# time 1.1: RTO, pkt 1 retransmitted, slow-start entered.
# time 1.4: ACK arrives for (first) pkt 1, pkts 2 and 3 transmitted.
# time 2.4: RTO, pkt 2 retransmitted, slow-start entered.
# time 2.6: ACK arrives for (second) pkt 1.
# time 3.7: RTO, pkt 2 retransmitted, slow-start entered.
# time 3.9: ACK arrives for (first) pkt 2, pkt 3 retransmitted, pkt 4
#           transmitted
# time 5.2: ACK arrives for (first) pkt 3, pkt 5 transmitted.

Class Test/droppedSYN -superclass TestSuite
Test/droppedSYN instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	droppedSYN
	set guide_	"NewReno TCP before bugfix_ss_, dropped SYN packet." 
	## static initial window of four packets
   	Agent/TCP set bugfix_ss_ 0
        Agent/TCP set windowInit_ 4
        $self next pktTraceFile
}

Test/droppedSYN instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 1
	set tcp1 [$self make_tcp s1 k1 0 Newreno]
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 5.0 5.0 
}

Class Test/droppedSYN1 -superclass TestSuite
Test/droppedSYN1 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	droppedSYN(bugfix_ss_)
	set guide_	"NewReno TCP, dropped SYN packet." 
	## static initial window of four packets
   	Agent/TCP set bugfix_ss_ 1
        Agent/TCP set windowInit_ 4
        Test/droppedSYN1 instproc run {} [Test/droppedSYN info instbody run ]
        $self next pktTraceFile
}

Class Test/droppedPKT -superclass TestSuite
Test/droppedPKT instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	droppedPKT(Newreno)
	set guide_	"NewReno TCP without bugfix_ss_, dropped initial data packet."
	## static initial window of four packets
   	Agent/TCP set bugfix_ss_ 0
        Agent/TCP set windowInit_ 4
        $self next pktTraceFile
}

Test/droppedPKT instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 1
	set tcp1 [$self make_tcp s1 k1 0 Newreno]
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 2
	$self runall_test $tcp1 5.0 5.0 
}

Class Test/droppedPKT1 -superclass TestSuite
Test/droppedPKT1 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	droppedPKT(NewReno,bugfix_ss_)
	set guide_	"NewReno TCP, dropped initial data packet."
	## static initial window of four packets
   	Agent/TCP set bugfix_ss_ 1
        Agent/TCP set windowInit_ 4
        Test/droppedPKT1 instproc run {} [Test/droppedPKT info instbody run ]
        $self next pktTraceFile
}

Class Test/droppedPKT2 -superclass TestSuite
Test/droppedPKT2 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	droppedPKT2(Sack)
	set guide_	"Sack TCP without bugfix_ss_, dropped initial data packet."
	## static initial window of four packets
   	Agent/TCP set bugfix_ss_ 0
        Agent/TCP set windowInit_ 4
        $self next pktTraceFile
}

Test/droppedPKT2 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set windowInitOption_ 1
	set tcp1 [$self make_tcp s1 k1 0 Sack]
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 2
	$self runall_test $tcp1 5.0 5.0 
}

Class Test/droppedPKT3 -superclass TestSuite
Test/droppedPKT3 instproc init {} {
	$self instvar net_ test_ guide_ 
	set net_	net6
	set test_	droppedPKT3(Sack,bugfix_ss_)
	set guide_	"Sack TCP, dropped initial data packet."
	## static initial window of four packets
   	Agent/TCP set bugfix_ss_ 1
        Agent/TCP set windowInit_ 4
        Test/droppedPKT3 instproc run {} [Test/droppedPKT2 info instbody run ]
        $self next pktTraceFile
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
