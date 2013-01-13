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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-tcpHighspeed.tcl,v 1.21 2006/10/22 15:20:46 sallyfloyd Exp $
#

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
Agent/TCP set numdupacksFrac_ 100 ;	# Default changed on 2006/09/27.
# default changed on 10/14/2004.
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish file {
        global quiet PERL
	$self instvar cwnd_chan_ testName_

        if { [info exists cwnd_chan_] } {
                $self plot_cwnd 1 $testName_ all.cwnd1
    		exec cp temp.cwnd temp.rands
        }
}

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}


Class Topology/net2a -superclass Topology
Topology/net2a instproc init ns {
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
    $ns duplex-link $node_(r1) $node_(r2) 30Mb 30ms RED
    # The delay-bandwidth product of this link is 225 1000-byte packets.
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 100Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 100Mb 5ms DropTail
}

Class Topology/net2b -superclass Topology
Topology/net2b instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $self next
    #Queue/RED set bottom_ 0.001
    Queue/RED set bottom_ 0
    Queue/RED set thresh_ 0
    Queue/RED set maxthresh_ 0
    Queue/RED set q_weight_ 0
    Queue/RED set adaptive_ 1
    $ns duplex-link $node_(s1) $node_(r1) 100Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 100Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 30Mb 30ms RED
    # The delay-bandwidth product of this link is 225 1000-byte packets.
    $ns queue-limit $node_(r1) $node_(r2) 200
    $ns queue-limit $node_(r2) $node_(r1) 200
    $ns duplex-link $node_(s3) $node_(r2) 100Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 100Mb 5ms DropTail
}

############################################################

# To use windows larger than 1024 pkts, it is necessary to set
# MWS in tcp-sink.h. 

Class Test/tcp -superclass TestSuite
Test/tcp instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2a
    set test_	tcp
    set guide_	"Sack TCP, bad queue."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    $self next noTraceFiles
}
Test/tcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ dumpfile_ sender_ receiver_ guide_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TCP set window_ 512
    set stopTime  150.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set tcp1 \
     [$ns_ create-connection $sender_ $node_(s1) $receiver_ $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $stopTime0 "$ftp1 stop"

    set tcp2 \
     [$ns_ create-connection $sender_ $node_(s2) $receiver_ $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp2 all.cwnd1
    $ns_ at 30.0 "$ftp2 start"
    $ns_ at 80.0 "$ftp2 stop"

    $ns_ at $stopTime "$self cleanupAll $testName_" 
    $ns_ at $stopTime2 "exit 0"
    $ns_ run
}

Class Test/tcpHighspeed -superclass TestSuite
Test/tcpHighspeed instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2a
    set test_	tcpHighspeed
    set guide_	"TCP with experimental modification for highspeed TCP, bad queue."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set windowOption_ 8
    Test/tcpHighspeed instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

Class Test/tcp1 -superclass TestSuite
Test/tcp1 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcp
    set guide_	"Sack TCP, good queue."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Test/tcp1 instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}
Class Test/tcp1A -superclass TestSuite
Test/tcp1A instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcp
    set guide_	"Sack TCP, good queue, max_ssthresh=100."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set max_ssthresh_ 100
    Test/tcp1A instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

## tcpHighspeed1 uses max_ssthresh_.
Class Test/tcpHighspeed1 -superclass TestSuite
Test/tcpHighspeed1 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcpHighspeed1
    set guide_	"Highspeed TCP, good queue, max_ssthresh=100."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set windowOption_ 8
    Agent/TCP set max_ssthresh_ 100
    Test/tcpHighspeed1 instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

## tcpHighspeed1A uses cwnd_range_
Class Test/tcpHighspeed1A -superclass TestSuite
Test/tcpHighspeed1A instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcpHighspeed1A
    set guide_	"Highspeed TCP, good queue, max_ssthresh=100, efficient version."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set windowOption_ 8
    Agent/TCP set max_ssthresh_ 100
    Agent/TCP set cwnd_range_ 50
    Test/tcpHighspeed1A instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

## tcpHighspeed2 uses a different value for low_window_.
Class Test/tcpHighspeed2 -superclass TestSuite
Test/tcpHighspeed2 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcpHighspeed2
    set guide_	"Highspeed TCP, low_window_ set to 25."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set windowOption_ 8
    Agent/TCP set low_window_ 25
    Test/tcpHighspeed2 instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

## tcpHighspeed3 uses different values for high_p_ and high_decrease_,
##   to give essentially the same response function as TCP.
Class Test/tcpHighspeed3 -superclass TestSuite
Test/tcpHighspeed3 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcpHighspeed3
    set guide_	"Highspeed TCP, parameters set similar to TCP."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set windowOption_ 8
    #Agent/TCP set high_p_ 0.0000000096 # the TCP formula would say this.
    Agent/TCP set high_p_ 0.00000001
    Agent/TCP set high_decrease_ 0.5
    Test/tcpHighspeed3 instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

## tcpHighspeed4 uses different values for the response function.
Class Test/tcpHighspeed4 -superclass TestSuite
Test/tcpHighspeed4 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcpHighspeed4
    set guide_	"Highspeed TCP, parameters set conservatively."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set windowOption_ 8
    Agent/TCP set low_window_ 50
    Agent/TCP set high_p_ 0.0000001
    Agent/TCP set high_decrease_ 0.25
    Test/tcpHighspeed4 instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

## tcpHighspeed5 uses different values for the response function.
Class Test/tcpHighspeed5 -superclass TestSuite
Test/tcpHighspeed5 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2b
    set test_	tcpHighspeed5
    set guide_	"Highspeed TCP, parameters set aggressively."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set windowOption_ 8
    Agent/TCP set low_window_ 13
    Agent/TCP set high_window_ 12500
    Agent/TCP set high_p_ 0.000001
    Agent/TCP set high_decrease_ 0.1
    Test/tcpHighspeed5 instproc run {} [Test/tcp info instbody run ]
    $self next noTraceFiles
}

TestSuite runTest 

