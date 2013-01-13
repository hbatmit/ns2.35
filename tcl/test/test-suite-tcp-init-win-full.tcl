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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-tcp-init-win-full.tcl,v 1.15 2006/01/24 23:00:08 sallyfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcp.tcl
#

source misc_simple.tcl
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
set wrap [expr 90 * 1000 + 40]
set scale 0.00001
Agent/TCP/FullTcp set segsize_ 0 ;	# segsize_ will come from packetSize_

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Agent/TCP/Newreno set newreno_changes1_ 0
# The default is being changed to 1 on 5/5/03, to reflect RFC 2582.
Agent/TCP/Newreno set partial_window_deflation_ 0  
# The default is being changed to 1 on 5/5/03, to reflect RFC 2582.
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
set plotacks false

Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

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
    
# The ack sequence number is not printed out.

TestSuite instproc finish file {
	global quiet PERL plotacks wrap scale
	set plotacks false
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s $scale -m $wrap -t $file > temp.rands
        if {$plotacks == "true"} {
          exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
             $PERL ../../bin/raw2xg -a -e -s 0.00001 -m $wrap -t $file > temp1.rands
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
	$self instvar ns_ node_ testName_

	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 $dumptime
	#$self traceQueues $node_(r1) [$self openTrace $runtime $testName_]
	$ns_ at $runtime "$self cleanupAll $testName_"
	$ns_ run
}

TestSuite instproc second_test {tcp1 tcp2} {
	$self instvar ns_ node_ testName_
	$tcp1 set window_ 40
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 1.0 "$ftp1 start"

	$tcp2 set window_ 40
	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 0.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0
	#$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
	$ns_ run
}

TestSuite instproc make_tcp {nodeA nodeB ID type} {
	$self instvar ns_ node_ 
        if {$type == "Tahoe"} {
		set tcp [$ns_ create-connection-listen TCP/FullTcp/Tahoe $node_($nodeA) TCP/FullTcp/Tahoe $node_($nodeB) $ID]
	}
        if {$type == "Reno"} {
		set tcp [$ns_ create-connection-listen TCP/FullTcp $node_($nodeA) TCP/FullTcp $node_($nodeB) $ID]
	}
        if {$type == "Newreno"} {
		set tcp [$ns_ create-connection-listen TCP/FullTcp/Newreno $node_($nodeA) TCP/FullTcp/Newreno $node_($nodeB) $ID]
	}
        if {$type == "Sack"} {
		set tcp [$ns_ create-connection-listen TCP/FullTcp/Sack $node_($nodeA) TCP/FullTcp/Sack $node_($nodeB) $ID]
	}
	return $tcp
}

# generate segment size from packet size.
Class Test/tahoe1 -superclass TestSuite
Test/tahoe1 instproc init {} {
	$self instvar net_ test_ 
	set net_	net6
	set test_	tahoe1(variable_packet_sizes)
        $self next pktTraceFile
}
Test/tahoe1 instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Tahoe]
	$tcp1 set packetSize_ 1000
	set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$tcp2 set packetSize_ 1500
	set tcp3 [$self make_tcp s2 k1 2 Tahoe]
	$tcp3 set packetSize_ 2500
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

# generate packetsize from segment size.
Class Test/tahoe1A -superclass TestSuite
Test/tahoe1A instproc init {} {
	$self instvar net_ test_ 
	set net_	net6
	set test_	tahoe1A(variable_packet_sizes)
        $self next pktTraceFile
}
Test/tahoe1A instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Tahoe]
	$tcp1 set segsize_ 960
	set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$tcp2 set segsize_ 1460
	set tcp3 [$self make_tcp s2 k1 2 Tahoe]
	$tcp3 set segsize_ 2460
	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
}

Class Test/tahoe2 -superclass TestSuite
Test/tahoe2 instproc init {} {
	$self instvar net_ test_
	set net_	net6
	set test_	tahoe2(static_initial_windows)
	$self next pktTraceFile
}
Test/tahoe2 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
	$tcp1 set windowInit_ 6
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
	$self instvar net_ test_ 
	set net_	net6
	set test_	tahoe3(dropped_syn)
        $self next pktTraceFile
}

## Drop the n-th packet for flow on link.
TestSuite instproc drop_pkt { link flow n } {
	set em [new ErrorModule Fid]
	set errmodel [new ErrorModel/Periodic]
	$errmodel unit pkt
	$errmodel set offset_ n
	$errmodel set period_ 1000.0
	$link errormodule $em
	$em insert $errmodel
	$em bind $errmodel $flow
}

Test/tahoe3 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Tahoe]
	$tcp1 set packetSize_ 1000
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 10.0 10.0 
}

Class Test/tahoe4 -superclass TestSuite
Test/tahoe4 instproc init {} {
	$self instvar net_ test_
	set net_	net7
	set test_	tahoe4(fast_retransmit)
	Queue/RED set ns1_compat_ true
	$self next pktTraceFile
}
Test/tahoe4 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true

        set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
        set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$self second_test $tcp1 $tcp2
}

Class Test/reno1 -superclass TestSuite
Test/reno1 instproc init {} {
	$self instvar net_ test_ 
	set net_	net6
	set test_	reno1(variable_packet_sizes)
        $self next pktTraceFile
}
Test/reno1 instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
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
	$self instvar net_ test_
	set net_	net6
	set test_	reno2(static_initial_windows)
	$self next pktTraceFile
}
Test/reno2 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	set tcp1 [$self make_tcp s1 k1 0 Reno] 
	$tcp1 set windowInit_ 6
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
	$self instvar net_ test_ 
	set net_	net6
	set test_	reno3(dropped_syn)
        $self next pktTraceFile
}

## Drop the n-th packet for flow on link.
TestSuite instproc drop_pkt { link flow n } {
	set em [new ErrorModule Fid]
	set errmodel [new ErrorModel/Periodic]
	$errmodel unit pkt
	$errmodel set offset_ n
	$errmodel set period_ 1000.0
	$link errormodule $em
	$em insert $errmodel
	$em bind $errmodel $flow
}

Test/reno3 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Reno]
	$tcp1 set packetSize_ 1000
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 10.0 10.0 
}

Class Test/reno4 -superclass TestSuite
Test/reno4 instproc init {} {
	$self instvar net_ test_
	set net_	net7
	set test_	reno4(fast_retransmit)
	Queue/RED set ns1_compat_ true
	$self next pktTraceFile
}
Test/reno4 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true

        set tcp1 [$self make_tcp s1 k1 0 Reno] 
        set tcp2 [$self make_tcp s2 k1 1 Reno]
	$self second_test $tcp1 $tcp2
}


Class Test/newreno1 -superclass TestSuite
Test/newreno1 instproc init {} {
	$self instvar net_ test_ 
	set net_	net6
	set test_	newreno1(variable_packet_sizes)
        $self next pktTraceFile
}
Test/newreno1 instproc run {} {
        $self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
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
	$self instvar net_ test_
	set net_	net6
	set test_	newreno2(static_initial_windows)
	$self next pktTraceFile
}
Test/newreno2 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	set tcp1 [$self make_tcp s1 k1 0 Newreno] 
	$tcp1 set windowInit_ 6
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
	$self instvar net_ test_ 
	set net_	net6
	set test_	newreno3(dropped_syn)
        $self next pktTraceFile
}

## Drop the n-th packet for flow on link.
TestSuite instproc drop_pkt { link flow n } {
	set em [new ErrorModule Fid]
	set errmodel [new ErrorModel/Periodic]
	$errmodel unit pkt
	$errmodel set offset_ n
	$errmodel set period_ 1000.0
	$link errormodule $em
	$em insert $errmodel
	$em bind $errmodel $flow
}

Test/newreno3 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	Agent/TCP set windowInitOption_ 2
	set tcp1 [$self make_tcp s1 k1 0 Newreno]
	$tcp1 set packetSize_ 1000
	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
	$self runall_test $tcp1 10.0 10.0 
}

Class Test/newreno4 -superclass TestSuite
Test/newreno4 instproc init {} {
	$self instvar net_ test_
	set net_	net7
	set test_	newreno4(fast_retransmit)
	Queue/RED set ns1_compat_ true
	$self next pktTraceFile
}
Test/newreno4 instproc run {} {
	$self instvar ns_ node_ testName_
	$self setTopo
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true

        set tcp1 [$self make_tcp s1 k1 0 Newreno] 
        set tcp2 [$self make_tcp s2 k1 1 Newreno]
	$self second_test $tcp1 $tcp2
}


#
#Class Test/sack1 -superclass TestSuite
#Test/sack1 instproc init {} {
#	$self instvar net_ test_ 
#	set net_	net6
#	set test_	sack1(variable_packet_sizes)
#        $self next pktTraceFile
#}
#Test/sack1 instproc run {} {
#        $self instvar ns_ node_ testName_
#	$self setTopo
#	Agent/TCP set syn_ true
#	Agent/TCP set delay_growth_ true
#	Agent/TCP set windowInitOption_ 2
#	set tcp1 [$self make_tcp s1 k1 0 Sack]
#	$tcp1 set packetSize_ 1000
#	set tcp2 [$self make_tcp s2 k1 1 Sack]
#	$tcp2 set packetSize_ 1500
#	set tcp3 [$self make_tcp s2 k1 2 Sack]
#	$tcp3 set packetSize_ 2500
#	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
#}
#
#Class Test/sack2 -superclass TestSuite
#Test/sack2 instproc init {} {
#	$self instvar net_ test_
#	set net_	net6
#	set test_	sack2(static_initial_windows)
#	$self next pktTraceFile
#}
#Test/sack2 instproc run {} {
#	$self instvar ns_ node_ testName_
#	$self setTopo
#	set tcp1 [$self make_tcp s1 k1 0 Sack] 
#	$tcp1 set windowInit_ 6
#	set tcp2 [$self make_tcp s2 k1 1 Sack]
#	$tcp2 set syn_ true
#	$tcp2 set delay_growth_ true
#	set tcp3 [$self make_tcp s2 k1 2 Sack]
#	$tcp3 set windowInit_ 6
#	$tcp3 set syn_ true
#	$tcp3 set delay_growth_ true
#	$self run_test $tcp1 $tcp2 $tcp3 1.0 1.0 16
#}
#
#Class Test/sack3 -superclass TestSuite
#Test/sack3 instproc init {} {
#	$self instvar net_ test_ 
#	set net_	net6
#	set test_	sack3(dropped_syn)
#        $self next pktTraceFile
#}
#
## Drop the n-th packet for flow on link.
TestSuite instproc drop_pkt { link flow n } {
	set em [new ErrorModule Fid]
	set errmodel [new ErrorModel/Periodic]
	$errmodel unit pkt
	$errmodel set offset_ n
	$errmodel set period_ 1000.0
	$link errormodule $em
	$em insert $errmodel
	$em bind $errmodel $flow
}
#
#Test/sack3 instproc run {} {
#        $self instvar ns_ node_ testName_ 
#	$self setTopo
#	Agent/TCP set syn_ true
#	Agent/TCP set delay_growth_ true
#	Agent/TCP set windowInitOption_ 2
#	set tcp1 [$self make_tcp s1 k1 0 Sack]
#	$tcp1 set packetSize_ 1000
#	$self drop_pkt [$ns_ link $node_(r1) $node_(k1)] 0 1
#	$self runall_test $tcp1 10.0 10.0 
#}
#
# This does not work correctly!

# Class Test/sack4 -superclass TestSuite
# Test/sack4 instproc init {} {
# 	$self instvar net_ test_
# 	set net_	net7
# 	set test_	sack4(fast_retransmit)
# 	Queue/RED set ns1_compat_ true
# 	$self next pktTraceFile
# }
# Test/sack4 instproc run {} {
# 	$self instvar ns_ node_ testName_
# 	$self setTopo
# 	Agent/TCP set packetSize_ 1000
# 	Agent/TCP set windowInitOption_ 2
# 	Agent/TCP set syn_ true
# 	Agent/TCP set delay_growth_ true
#       set tcp1 [$self make_tcp s1 k1 0 Sack] 
#       set tcp2 [$self make_tcp s2 k1 1 Sack]
# 	$self second_test $tcp1 $tcp2
# }

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
	$self instvar net_ test_ 
	set net_	net8
	set test_	slowLink(9.6K-link,1000-byte-pkt)
        set plotacks true
        $self next pktTraceFile
}

Test/slowLink instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
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
# 	$self instvar net_ test_ 
# 	set net_	net8
# 	set test_	slowLinkDelayAck(9.6K-link,1000-byte-pkt)
#         set plotacks true
#         $self next pktTraceFile
# }
# 
# Test/slowLinkDelayAck instproc run {} {
#         $self instvar ns_ node_ testName_ 
# 	$self setTopo
# 	Agent/TCP set syn_ true
# 	Agent/TCP set delay_growth_ true
# 	Agent/TCP set windowInitOption_ 2
# 	Agent/TCP set minrto_ 1
# 	set tcp1 [$self make_tcp s1 k1 0 SackDelAck]
# 	$tcp1 set packetSize_ 1000
# 	$ns_ at 0.0 "$self printtimersAll $tcp1 0 1"
# 	$self runall_test $tcp1 30.0 30.0 
# }

# This test shows the packets and acknowledgements at the source,
# for a path with a 9.6Kbps link, and 1500-byte packets.
#Class Test/slowLink1 -superclass TestSuite
#Test/slowLink1 instproc init {} {
#	global plotacks
#	$self instvar net_ test_ 
#	set net_	net8
#	set test_	slowLink1(9.6K-link,1500-byte-pkt)
#        set plotacks true
#        $self next pktTraceFile
#}
#
#Test/slowLink1 instproc run {} {
#        $self instvar ns_ node_ testName_ 
#	$self setTopo
#	Agent/TCP set syn_ true
#	Agent/TCP set delay_growth_ true
#	Agent/TCP set windowInitOption_ 2
#	Agent/TCP set minrto_ 1
#	set tcp1 [$self make_tcp s1 k1 0 Sack]
#	$tcp1 set packetSize_ 1500
#	$self runall_test $tcp1 30.0 30.0 
#}

# This test shows the packets and acknowledgements at the source,
# for a path with a 9.6Kbps link, and 1500-byte packets.
# Initial window of one packet.
Class Test/slowLink2 -superclass TestSuite
Test/slowLink2 instproc init {} {
	global plotacks
	$self instvar net_ test_ 
	set net_	net8
	set test_	slowLink2(9.6K-link,1500-byte-pkt)
        set plotacks true
        $self next pktTraceFile
}

Test/slowLink2 instproc run {} {
        $self instvar ns_ node_ testName_ 
	$self setTopo
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
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

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
