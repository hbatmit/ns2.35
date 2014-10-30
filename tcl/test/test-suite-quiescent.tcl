# copyright (c) 1995 The Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-quiescent.tcl,v 1.14 2006/03/15 04:04:28 sallyfloyd Exp $
#

source misc_simple.tcl
source support.tcl
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21

Agent/TCP set packetSize_ 500
Application/Traffic/CBR set packetSize_ 500
Agent/TCP set window_ 1000
Agent/TCP set partial_ack_ 1
Agent/TFRC set SndrType_ 1 
Agent/TFRC set packetSize_ 500 

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish {file stoptime} {
        global quiet PERL
	exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	  $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
        exec echo $stoptime 0 >> temp.rands 
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
        }
        ## now use default graphing tool to make a data file
        ## if so desired
#       exec csh figure2.com $file
#	exec cp temp.rands temp.$file 
#	exec csh gnuplotA.com temp.$file $file
        exit 0
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
    #$ns duplex-link $node_(r1) $node_(r2) 10Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50 
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

Class Test/tfrc_onoff -superclass TestSuite
Test/tfrc_onoff instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ sender 
    set net_	net2
    set test_ tfrc_onoff	
    set guide_  \
    "TFRC with a data source with limited, bursty data, no congestion."
    set sender TFRC
    Agent/TFRC set oldCode_ false
    set stopTime1_ 10
    $self next pktTraceFile
}
Test/tfrc_onoff instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ stopTime1_ sender 
    puts "Guide: $guide_"
    $self setTopo
    set stopTime $stopTime1_

    if {$sender == "TFRC"} {
      set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    } elseif {$sender == "TCP"} {
      set tf1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    }
    set ftp [new Application/FTP]
    $ftp attach-agent $tf1
    $ns_ at 0 "$ftp produce 100"
    $ns_ at 5 "$ftp producemore 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tfrc_onoff_oldcode -superclass TestSuite
Test/tfrc_onoff_oldcode instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ sender 
    set net_	net2
    set test_ tfrc_onoff_oldcode	
    set guide_  \
    "TFRC, bursty data, no congestion, old code."
    set sender TFRC
    Agent/TFRC set oldCode_ true
    set stopTime1_ 10
    Test/tfrc_onoff_oldcode instproc run {} [Test/tfrc_onoff info instbody run ]
    $self next pktTraceFile
}

Class Test/tcp_onoff -superclass TestSuite
Test/tcp_onoff instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ sender 
    set net_	net2
    set test_ tcp_onoff	
    set guide_  \
    "TCP with a data source with limited, bursty data, no congestion."
    set sender TCP
    set stopTime1_ 10
    Test/tcp_onoff instproc run {} [Test/tfrc_onoff info instbody run ]
    $self next pktTraceFile
}

Class Test/tfrc_telnet -superclass TestSuite
Test/tfrc_telnet instproc init {} {
    $self instvar net_ test_ guide_ sender 
    set net_	net2
    set test_ tfrc_telnet	
    set guide_  \
    "TFRC with a Telnet data source, telnet data rate increased at time 4."
    set sender TFRC
    $self next pktTraceFile
}
Test/tfrc_telnet instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sender 
    puts "Guide: $guide_"
    $self setTopo
    set stopTime 10

    if {$sender == "TFRC"} {
      set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    } elseif {$sender == "TCP"} {
      set tf1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    }
    set telnet1 [new Source/Telnet]
    $telnet1 attach-agent $tf1
    $telnet1 set interval_ 0.01
    $ns_ at 0.0 "$telnet1 start"
    $ns_ at 4.0 "$telnet1 set interval_ 0.001"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tcp_telnet -superclass TestSuite
Test/tcp_telnet instproc init {} {
    $self instvar net_ test_ guide_ sender 
    set net_	net2
    set test_ tcp_telnet	
    set guide_  \
    "TCP with a Telnet data source, telnet data rate increased at time 4."
    set sender TCP
    Test/tcp_telnet instproc run {} [Test/tfrc_telnet info instbody run ]
    $self next pktTraceFile
}

Class Test/tcp_telnet_CWV -superclass TestSuite
Test/tcp_telnet_CWV instproc init {} {
    $self instvar net_ test_ guide_ sender 
    set net_	net2
    set test_ tcp_telnet_CWV	
    set guide_  \
    "TCP with a Telnet data source, with Congestion Window Validation."
    set sender TCP
    Agent/TCP set QOption_ 1
    Agent/TCP set control_increase_ 1
    # Agent/TCP set EnblRTTCtr_ 1
    Test/tcp_telnet_CWV instproc run {} [Test/tfrc_telnet info instbody run]
    $self next pktTraceFile
}

Class Test/tfrc_cbr -superclass TestSuite
Test/tfrc_cbr instproc init {} {
    $self instvar net_ test_ guide_ sender 
    set net_	net2
    set test_ tfrc_cbr	
    set guide_  \
    "TFRC with a CBR data source, CBR data rate changes over time."
    set sender TFRC
    $self next pktTraceFile
}
Test/tfrc_cbr instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sender 
    puts "Guide: $guide_"
    $self setTopo
    set stopTime 15

    if {$sender == "TFRC"} {
      set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    } elseif {$sender == "TCP"} {
      set tf1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    }
    set cbr1 [new Application/Traffic/CBR]
    $cbr1 attach-agent $tf1
    # $tf1 set type_ CBR
    $cbr1 set interval_ 0.01
    $ns_ at 0.01 "$cbr1 start"
    $ns_ at 2.0 "$cbr1 set interval_ 0.002"
    $ns_ at 3.0 "$cbr1 set interval_ 0.01"
    $ns_ at 8.0 "$cbr1 set interval_ 0.002"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tfrc_cbr_conservative -superclass TestSuite
Test/tfrc_cbr_conservative instproc init {} {
    $self instvar net_ test_ guide_ sender
    set net_	net2
    set test_ tfrc_cbr_conservative	
    set guide_  \
    "TFRC with a CBR data source, conservative option."
    set sender TFRC
    Agent/TFRC set conservative_ true
    Agent/TFRC set scmult_ 1.2
    Test/tfrc_cbr_conservative instproc run {} [Test/tfrc_cbr info instbody run ]
    $self next pktTraceFile
}
Class Test/tcp_cbr -superclass TestSuite
Test/tcp_cbr instproc init {} {
    $self instvar net_ test_ guide_ sender
    set net_	net2
    set test_ tcp_cbr	
    set guide_  \
    "TCP with a CBR data source, CBR data rate changes over time."
    set sender TCP
    Test/tcp_cbr instproc run {} [Test/tfrc_cbr info instbody run ]
    $self next pktTraceFile
}

Class Test/tcp_cbr_CWV -superclass TestSuite
Test/tcp_cbr_CWV instproc init {} {
    $self instvar net_ test_ guide_ sender 
    set net_	net2
    set test_ tcp_cbr_CWV	
    set guide_  \
    "TCP with a CBR data source, with Congestion Window Validation." 
    set sender TCP
    Agent/TCP set QOption_ 1
    Agent/TCP set control_increase_ 1
    # Agent/TCP set EnblRTTCtr_ 1
    Test/tcp_cbr_CWV instproc run {} [Test/tfrc_cbr info instbody run ]
    $self next pktTraceFile
}

TestSuite runTest

