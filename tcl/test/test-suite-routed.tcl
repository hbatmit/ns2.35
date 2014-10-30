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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-routed.tcl,v 1.10 2006/01/24 23:00:07 sallyfloyd Exp $
#
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., Simulator Tests. July 1995.  
# URL ftp://ftp.ee.lbl.gov/papers/simtests.ps.Z.
#
# To run individual tests:
# ns test-suite.tcl tahoe1
# ns test-suite.tcl tahoe2
# ...
#

set dir [pwd]
catch "cd tcl/test"
source misc.tcl
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
source topologies.tcl
catch "cd $dir"

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.

Class Test/tahoe1 -superclass TestSuite
Test/tahoe1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe
	$self next
}
Test/tahoe1 instproc run {} {
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	
	# Set up FTP source
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	#
	# Actually, we now trace all activity at the node around the
	# bottleneck link.  This allows us to track acks, as well
	# packets taking any alternate paths around the bottleneck
	# link.
	#
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/tahoe2 -superclass TestSuite
Test/tahoe2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe2
	$self next
}
Test/tahoe2 instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 14
	
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/tahoe3 -superclass TestSuite
Test/tahoe3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe3
	$self next
}
Test/tahoe3 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8   
	$ns_ queue-limit $node_(k1) $node_(r1) 8   

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 100
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 16

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 0.5 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 8.0 $testName_]
	$ns_ run
}

Class Test/tahoe4 -superclass TestSuite
Test/tahoe4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe4
	$self next
}
Test/tahoe4 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 200ms
	$ns_ delay $node_(r1) $node_(s2) 200ms
	$ns_ queue-limit $node_(r1) $node_(k1) 11
	$ns_ queue-limit $node_(k1) $node_(r1) 11  

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 30
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 30

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 0.0 "$ftp1 start"
	$ns_ at 0.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/tahoe5 -superclass TestSuite
Test/tahoe5 instproc init topo {
    $self instvar net_ defNet_ test_
    set net_	$topo
    set defNet_	net1
    set test_	tahoe5
    $self next
}
Test/tahoe5 instproc run {} {
    $self instvar ns_ node_ testName_

    $ns_ delay $node_(s1) $node_(r1) 3ms
    $ns_ delay $node_(r1) $node_(s1) 3ms

    set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
    $tcp1 set window_ 50
    $tcp1 set bugFix_ false

    set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
    $tcp2 set window_ 50
    $tcp2 set bugFix_ false

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 1.0 "$ftp1 start"
    $ns_ at 1.75 "$ftp2 produce 100"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
    $ns_ run
}

Class Test/no_bug -superclass TestSuite
Test/no_bug instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net1
	set test_	no_bug
	$self next
}
Test/no_bug instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s1) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s1) 3ms

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 50

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.75 "$ftp2 produce 100"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
	$ns_ run
}

Class Test/bug -superclass TestSuite
Test/bug instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net1
	set test_	bug
	$self next
}
Test/bug instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s1) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s1) 3ms

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	$tcp1 set bugFix_ false
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 50
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.75 "$ftp2 produce 100"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
	$ns_ run
}

Class Test/reno1 -superclass TestSuite
Test/reno1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno1
	$self next
}
Test/reno1 instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 14

	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/reno -superclass TestSuite
Test/reno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno
	$self next
}
Test/reno instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 28
	$tcp1 set maxcwnd_ 14

	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/renoA -superclass TestSuite
Test/renoA instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	renoA
	$self next
}
Test/renoA instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 28
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 1]
	$tcp2 set window_ 4
	set tcp3 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 2]
	$tcp3 set window_ 4

	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 1.0 "$ftp1 start"
	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 1.2 "$ftp2 produce 7"
	set ftp3 [$tcp3 attach-app FTP]
	$ns_ at 1.2 "$ftp3 produce 7"

	$self tcpDump $tcp1 1.0
	$self tcpDump $tcp2 1.0
	$self tcpDump $tcp3 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/reno2 -superclass TestSuite
Test/reno2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno2
	$self next
}
Test/reno2 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 9

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 20

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/reno3 -superclass TestSuite
Test/reno3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno3
	$self next
}
Test/reno3 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8
	$ns_ queue-limit $node_(k1) $node_(r1) 8

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 100
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 16

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 0.5 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 8.0 $testName_]
	$ns_ run
}

Class Test/reno4 -superclass TestSuite
Test/reno4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net2
	set test_	reno4
	$self next
}
Test/reno4 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(r2) 29

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
	$tcp1 set window_ 80
	$tcp1 set maxcwnd_ 40

	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(s1) [$self openTrace 2.0 $testName_]
	$ns_ run
}

Class Test/reno4a -superclass TestSuite
Test/reno4a instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net2
	set test_	reno4a
	$self next
}
Test/reno4a instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(r2) 29

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
	$tcp1 set window_ 40
	$tcp1 set maxcwnd_ 40

	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(s1) [$self openTrace 2.0 $testName_]
	$ns_ run
}

Class Test/reno5 -superclass TestSuite
Test/reno5 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno5
	$self next
}
Test/reno5 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 9

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	$tcp1 set bugFix_ false
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 20
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/reno5a -superclass TestSuite
Test/reno5a instproc init topo {
    $self instvar net_ defNet_ test_
    set net_	$topo
    set defNet_	net1
    set test_	reno5a
    $self next
}
Test/reno5a instproc run {} {
    $self instvar ns_ node_ testName_

    $ns_ delay $node_(s1) $node_(r1) 3ms
    $ns_ delay $node_(r1) $node_(s1) 3ms

    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
    $tcp1 set window_ 50
    $tcp1 set bugFix_ false

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
    $tcp2 set window_ 50
    $tcp2 set bugFix_ false

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 1.0 "$ftp1 start"
    $ns_ at 1.75 "$ftp2 produce 100"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
    $ns_ run
}

Class XTest/telnet -superclass TestSuite
XTest/telnet instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	telnet
	$self next
}
XTest/telnet instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8
	$ns_ queue-limit $node_(k1) $node_(r1) 8

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	set tcp3 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 2]

	set telnet1 [$tcp1 attach-app TELNET]; $telnet1 set interval_ 1
	set telnet2 [$tcp2 attach-app TELNET]; $telnet2 set interval_ 0
	# Interval 0 designates the tcplib telnet interarrival distribution
	set telnet3 [$tcp3 attach-app TELNET]; $telnet3 set interval_ 0

	$ns_ at 0.0 "$telnet1 start"
	$ns_ at 0.0 "$telnet2 start"
	$ns_ at 0.0 "$telnet3 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 50.0 $testName_]

	# use a different seed each time
	puts seed=[$ns_ random 0]

	$ns_ run
}

Class Test/delayed -superclass TestSuite
Test/delayed instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	delayed
	$self next
}
Test/delayed instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink/DelAck $node_(k1) 0]
	$tcp1 set window_ 50

	# lookup up the sink and set it's delay interval
	[$node_(k1) agent [$tcp1 dst-port]] set interval 100ms

	set ftp1 [$tcp1 attach-app FTP];
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
	$ns_ run
}

Class Test/phase -superclass TestSuite
Test/phase instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phase
	$self next
}
Test/phase instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s2) 3ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 32 
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 32 

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/phase1 -superclass TestSuite
Test/phase1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phase1
	$self next
}
Test/phase1 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 9.5ms
	$ns_ delay $node_(r1) $node_(s2) 9.5ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 32 
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 32 

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/phase2 -superclass TestSuite
Test/phase2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phase2
	$self next
}
Test/phase2 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s2) 3ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 32 
	$tcp1 set overhead_ 0.01
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 32 
	$tcp2 set overhead_ 0.01

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/timers -superclass TestSuite
Test/timers instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	timers
	$self next
}
Test/timers instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 2
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink/DelAck $node_(k1) 0]
	$tcp1 set window_ 4
	# look up the sink and set its delay interval
	[$node_(k1) agent [$tcp1 dst-port]] set interval_ 100ms
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink/DelAck $node_(k1) 1]
	$tcp2 set window_ 4
	# look up the sink and set its delay interval
	[$node_(k1) agent [$tcp2 dst-port]] set interval_ 100ms

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.3225 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/stats -superclass TestSuite
Test/stats instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	stats
	$self next
}
Test/stats instproc printpkts { label tcp } {
	puts "tcp $label total_packets_acked [$tcp set ack_]"
}
#XXX Still unfinished in ns-2
Test/stats instproc printdrops { label link } {
	puts "link $label total_drops [$link stat 0 drops]"
	puts "link $label total_packets [$link stat 0 packets]"
	puts "link $label total_bytes [$link stat 0 bytes]"
}
Test/stats instproc printstop { stoptime } {
	puts "stop-time $stoptime"
}
Test/stats instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 200ms
	$ns_ delay $node_(r1) $node_(s2) 200ms
	$ns_ queue-limit $node_(r1) $node_(k1) 10
	$ns_ queue-limit $node_(k1) $node_(r1) 10

	set stoptime 10.1 

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 30
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 30

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDumpAll $tcp1 5.0 tcp1
	$self tcpDumpAll $tcp2 5.0 tcp2

	$ns_ at $stoptime "$self printstop $stoptime"
	$ns_ at $stoptime "$self printpkts 1 $tcp1"
	#XXX Awaiting completion of link stats
	#$ns_ at $stoptime "$self printdrops 1 [$ns_ link $node_(r1) $node_(k1)]"

	# trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
	$ns_ run
}

Class Test/tahoe_long -superclass TestSuite
Test/tahoe_long instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net3
	set test_	tahoe_long
	$self next
}
Test/tahoe_long instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s3) 0]
	$tcp1 set window_ 100
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(s3) 1]
	$tcp2 set window_ 16

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 0.5 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	set traceFile [$self openTrace 15.0 $testName_]
	$self traceQueues $node_(r1) $traceFile
	foreach i [list {r2 r3} {r3 r4}] {
		set L [$ns_ link $node_([lindex $i 0]) $node_([lindex $i 1])]
		$L trace-dynamics $ns_ $traceFile
		$L trace-dynamics $ns_ stdout
	}
	$ns_ run
}

Class Test/reno_long -superclass TestSuite
Test/reno_long instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net3
	set test_	reno_long
	$self next
}
Test/reno_long instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
	$tcp1 set window_ 100
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
	$tcp2 set window_ 16
#	$tcp1 set bugFix_ false
#	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-app FTP]
	set ftp2 [$tcp2 attach-app FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 0.5 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	set traceFile [$self openTrace 15.0 $testName_]
	$self traceQueues $node_(r1) $traceFile
	foreach i [list {r2 r3} {r3 r4}] {
		set L [$ns_ link $node_([lindex $i 0]) $node_([lindex $i 1])]
		$L trace-dynamics $ns_ $traceFile
		$L trace-dynamics $ns_ stdout
	}
	$ns_ run
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
