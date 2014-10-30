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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-tcpLinux.tcl,v 1.3 2010/04/03 20:40:15 tom_henderson Exp $
#


# This is mostly copied from test-suite-tcpHighspeed

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Agent/TCPSink set SYN_immediate_ack_ false
# default changed 02/2010.

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


#net 2a:
# s1 =(2ms)=              =(4ms)= s3
#           r1 -(30ms)- r2 
# s2 =(3ms)=              =(5ms)= s4
#
# =: 100Mbps (DropTail)
# -: 30Mbps (RED)
Class Topology/net2red -superclass Topology
Topology/net2red instproc init ns {
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
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 100Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 100Mb 5ms DropTail
    set bottleneck [[$ns link $node_(r1) $node_(r2)] queue]
    $bottleneck set thresh_ 1
    $bottleneck set maxthresh_ 100
    $bottleneck set q_weight_ 0.01
    $bottleneck set mean_pktsize_ 1500
    $bottleneck set linterm_ 10
    $bottleneck set setbit_ false
    $bottleneck set queue_in_bytes_ false
}

Class Topology/net2droptail -superclass Topology
Topology/net2droptail instproc init ns {
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
    $ns duplex-link $node_(r1) $node_(r2) 30Mb 30ms DropTail
    # The delay-bandwidth product of this link is 225 1000-byte packets.
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 100Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 100Mb 5ms DropTail
}
############################################################

# To use windows larger than 1024 pkts, it is necessary to set
# MWS in tcp-sink.h. 

Class Test/tcpLinuxBase -superclass TestSuite
Test/tcpLinuxBase instproc init {} {
    $self instvar net_ test_ guide_ cc_
    set net_ net2red
    set test_	tcpLinuxBase
    set cc_ "naive_reno"
    set guide_	"TCP Linux, base ($cc_ with $net_)"
    $self next noTraceFiles
}
Test/tcpLinuxBase instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ dumpfile_  guide_ cc_
    if {$quiet == "false"} {puts $guide_}
    $self setTopo
    Agent/TCP set window_ 30000
    set stopTime  240.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set tcp1 \
     [$ns_ create-connection TCP/Linux $node_(s1) TCPSink/Sack1/DelAck $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
    $ns_ at 0.0 "$tcp1 select_ca $cc_"
    $ns_ at 0.0 "$tcp1 set_ca_default_param linux debug_level 2"
    $ns_ at 0.1 "$ftp1 start"
    $ns_ at $stopTime0 "$ftp1 stop"

    set tcp2 \
     [$ns_ create-connection TCP/Linux $node_(s2) TCPSink/Sack1/DelAck $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp2 all.cwnd1
    $ns_ at 0.0 "$tcp2 select_ca $cc_"
    $ns_ at 60.0 "$ftp2 start"
    $ns_ at 160.0 "$ftp2 stop"

    $ns_ at $stopTime "$tcp1 set cwnd_ 1"
    $ns_ at $stopTime "$tcp2 set cwnd_ 1"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    $ns_ at $stopTime2 "exit 0"
    $ns_ run
}

Class Test/tcpLinuxDropTail_reno -superclass TestSuite
Test/tcpLinuxDropTail_reno instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ reno
    set net_	net2droptail
    set test_	tcpLinuxDropTail_reno
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_reno instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_reno -superclass TestSuite
Test/tcpLinuxRED_reno instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ reno
    set net_	net2red
    set test_	tcpLinuxRED_reno
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_reno instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_vegas -superclass TestSuite
Test/tcpLinuxDropTail_vegas instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ vegas 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_vegas
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_vegas instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_vegas -superclass TestSuite
Test/tcpLinuxRED_vegas instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ vegas 
    set net_	net2red
    set test_	tcpLinuxRED_vegas
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_vegas instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_htcp -superclass TestSuite
Test/tcpLinuxDropTail_htcp instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ htcp 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_htcp
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_htcp instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_htcp -superclass TestSuite
Test/tcpLinuxRED_htcp instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ htcp 
    set net_	net2red
    set test_	tcpLinuxRED_htcp
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_htcp instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_bic -superclass TestSuite
Test/tcpLinuxDropTail_bic instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ bic 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_bic
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_bic instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_bic -superclass TestSuite
Test/tcpLinuxRED_bic instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ bic 
    set net_	net2red
    set test_	tcpLinuxRED_bic
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_bic instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_cubic -superclass TestSuite
Test/tcpLinuxDropTail_cubic instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ cubic 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_cubic
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_cubic instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_cubic -superclass TestSuite
Test/tcpLinuxRED_cubic instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ cubic 
    set net_	net2red
    set test_	tcpLinuxRED_cubic
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_cubic instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_hybla -superclass TestSuite
Test/tcpLinuxDropTail_hybla instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ hybla 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_hybla
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_hybla instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_hybla -superclass TestSuite
Test/tcpLinuxRED_hybla instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ hybla 
    set net_	net2red
    set test_	tcpLinuxRED_hybla
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_hybla instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_lp -superclass TestSuite
Test/tcpLinuxDropTail_lp instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ lp 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_lp
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_lp instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_lp -superclass TestSuite
Test/tcpLinuxRED_lp instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ lp 
    set net_	net2red
    set test_	tcpLinuxRED_lp
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_lp instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_compound -superclass TestSuite
Test/tcpLinuxDropTail_compound instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ compound 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_compound
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_compound instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_compound -superclass TestSuite
Test/tcpLinuxRED_compound instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ compound 
    set net_	net2red
    set test_	tcpLinuxRED_compound
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_compound instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_veno -superclass TestSuite
Test/tcpLinuxDropTail_veno instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ veno 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_veno
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_veno instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_veno -superclass TestSuite
Test/tcpLinuxRED_veno instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ veno 
    set net_	net2red
    set test_	tcpLinuxRED_veno
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_veno instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_yeah -superclass TestSuite
Test/tcpLinuxDropTail_yeah instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ yeah 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_yeah
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_yeah instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_yeah -superclass TestSuite
Test/tcpLinuxRED_yeah instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ yeah 
    set net_	net2red
    set test_	tcpLinuxRED_yeah
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_yeah instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_illinois -superclass TestSuite
Test/tcpLinuxDropTail_illinois instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ illinois 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_illinois
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_illinois instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_illinois -superclass TestSuite
Test/tcpLinuxRED_illinois instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ illinois 
    set net_	net2red
    set test_	tcpLinuxRED_illinois
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_illinois instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxDropTail_westwood -superclass TestSuite
Test/tcpLinuxDropTail_westwood instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ westwood 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_westwood
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_westwood instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_westwood -superclass TestSuite
Test/tcpLinuxRED_westwood instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ westwood 
    set net_	net2red
    set test_	tcpLinuxRED_westwood
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_westwood instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}


Class Test/tcpLinuxDropTail_highspeed -superclass TestSuite
Test/tcpLinuxDropTail_highspeed instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ highspeed 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_highspeed
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_highspeed instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_highspeed -superclass TestSuite
Test/tcpLinuxRED_highspeed instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ highspeed 
    set net_	net2red
    set test_	tcpLinuxRED_highspeed
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_highspeed instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}


Class Test/tcpLinuxDropTail_scalable -superclass TestSuite
Test/tcpLinuxDropTail_scalable instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ scalable 
    set net_	net2droptail
    set test_	tcpLinuxDropTail_scalable
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxDropTail_scalable instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}

Class Test/tcpLinuxRED_scalable -superclass TestSuite
Test/tcpLinuxRED_scalable instproc init {} {
    $self instvar test_ guide_ cc_ net_
    set cc_ scalable 
    set net_	net2red
    set test_	tcpLinuxRED_scalable
    set guide_	"TCP Linux congestion control: $cc_ with $net_"
    Test/tcpLinuxRED_scalable instproc run {} [Test/tcpLinuxBase info instbody run ]
    $self next noTraceFiles
}



TestSuite runTest

