
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-aimd.tcl,v 1.23 2006/01/25 22:02:04 sallyfloyd Exp $
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
#Agent/TCP set oldCode_ true

Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish file {
        global quiet PERL
	$self instvar cwnd_chan_

        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp1.rands
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp1.rands &
        }
        if { [info exists cwnd_chan_] } {
                $self plot_cwnd
    		exec cp temp.cwnd temp.rands
        }
        ## now use default graphing tool to make a data file
        ## if so desired
#       exec csh figure2.com $file
#	exec csh gnuplotA.com temp.rands $file
###        exit 0
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
    Queue/RED set gentle_ true
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 5ms RED
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

Class Test/tcp -superclass TestSuite
Test/tcp instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcp
    set guide_	"Sack TCP."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    $self next pktTraceFile
}
Test/tcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ dumpfile_ sender_ receiver_ guide_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TCP set window_ 20
    set stopTime  20.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection $sender_ $node_(s1) $receiver_ $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $stopTime0 "$ftp1 stop"

    set tcp2 [$ns_ create-connection $sender_ $node_(s2) $receiver_ $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP]
    $ns_ at 10.0 "$ftp2 start"
    $ns_ at 15.0 "$ftp2 stop"


    ###$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    ## $ns_ at $stopTime3 "exec cp temp.cwnd temp.rands; exit 0"
    $ns_ at $stopTime2 "exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tcpA -superclass TestSuite
Test/tcpA instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA{increase_0.41,decrease_0.75}
    set guide_	"Sack TCP, increase_num_ 0.41, decrease_num_ 0.75"
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Test/tcpA instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

Class Test/tcpA_precise -superclass TestSuite
Test/tcpA_precise instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA_precise{increase_0.41,decrease_0.75}
    set guide_	\
    "Sack TCP, increase_num_ 0.41, decrease_num_ 0.75, precisionReduce_ true"
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Agent/TCP set precisionReduce_ true
    Test/tcpA_precise instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

Class Test/tcpB -superclass TestSuite
Test/tcpB instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpB{increase_1.00,decrease_0.875}
    set guide_	\
    "Sack TCP, increase_num_ 1.0, decrease_num_ 0.875"
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set increase_num_ 1.0
    Agent/TCP set decrease_num_ 0.875
    Test/tcpB instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

TestSuite instproc emod {} {
        $self instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        return $errmodule
} 

TestSuite instproc set_lossylink {} {
        $self instvar lossylink_ ns_ node_
        set lossylink_ [$ns_ link $node_(r1) $node_(r2)]
        set em [new ErrorModule Fid]
        set errmodel [new ErrorModel/Periodic]
        $errmodel unit pkt
        $lossylink_ errormodule $em
}


# Drop the specified packet.
TestSuite instproc drop_pkt { number } {
    $self instvar ns_ lossmodel
    set lossmodel [$self setloss]
    $lossmodel set offset_ $number
    $lossmodel set period_ 10000
}

TestSuite instproc drop_pkts pkts {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 0
}

# First retransmit timeout, ssthresh decreased by half.
Class Test/ssthresh -superclass TestSuite
Test/ssthresh instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	ssthresh
    set guide_	"Retransmit Timeout with Sack TCP."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    $self next pktTraceFile
}
Test/ssthresh instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ dumpfile_ sender_ receiver_ guide_
    puts "Guide: $guide_"
    $self setTopo
    $self set_lossylink
    Agent/TCP set window_ 8
    set stopTime  2.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }
    set tcp1 [$ns_ create-connection $sender_ $node_(s1) $receiver_ $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
    $self drop_pkts {30 31 32 33 34 35 36}
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $stopTime0 "$ftp1 stop"
    
    ###$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    ## $ns_ at $stopTime3 "exec cp temp.cwnd temp.rands; exit 0"
    $ns_ at $stopTime2 "exit 0"
    # trace only the bottleneck link
    $ns_ run
}

Class Test/ssthreshA -superclass TestSuite
Test/ssthreshA instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	ssthreshA
    set guide_	\
    "Retransmit Timeout with Sack TCP, increase_num_ 0.41, decrease_num_ 0.75."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Test/ssthreshA instproc run {} [Test/ssthresh info instbody run ]
    $self next pktTraceFile
}

# Second retransmit timeout, ssthresh_second decrease depends on decrease_num_.
Class Test/ssthresh_second -superclass TestSuite
Test/ssthresh_second instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	ssthresh_second
    set guide_	"Two Retransmit Timeouts with Sack TCP."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    $self next pktTraceFile
}
Test/ssthresh_second instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ dumpfile_ sender_ receiver_ guide_
    puts "Guide: $guide_"
    $self setTopo
    $self set_lossylink
    Agent/TCP set window_ 8
    set stopTime  2.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }
    set tcp1 [$ns_ create-connection $sender_ $node_(s1) $receiver_ $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
    $self drop_pkts {30 31 32 33 34 35 36   120 121 122 123 124 125 126}
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $stopTime0 "$ftp1 stop"
    
    ###$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    ## $ns_ at $stopTime3 "exec cp temp.cwnd temp.rands; exit 0"
    $ns_ at $stopTime2 "exit 0"
    # trace only the bottleneck link
    $ns_ run
}

Class Test/ssthresh_secondA -superclass TestSuite
Test/ssthresh_secondA instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	ssthresh_secondA
    set guide_	"Two Retransmit Timeouts with Sack TCP,
    increase_num_ 0.41, decrease_num_ 0.75."
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Test/ssthresh_secondA instproc run {} [Test/ssthresh_second info instbody run ]
    $self next pktTraceFile
}

###################################################3

Class Test/tcp_tahoe -superclass TestSuite
Test/tcp_tahoe instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcp_tahoe
    set guide_	"Tahoe TCP"
    set sender_ TCP
    set receiver_ TCPSink
    Test/tcp_tahoe instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}
Class Test/tcpA_tahoe -superclass TestSuite
Test/tcpA_tahoe instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA_tahoe{increase_0.41,decrease_0.75}
    set guide_  "Tahoe TCP, increase_num_ 0.41, decrease_num_ 0.75"
    set sender_ TCP
    set receiver_ TCPSink
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Test/tcpA_tahoe instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}
Class Test/tcpA_precise_tahoe -superclass TestSuite
Test/tcpA_precise_tahoe instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA_precise_tahoe{increase_0.41,decrease_0.75}
    set guide_  \
    "Tahoe TCP, increase_num_ 0.41, decrease_num_ 0.75, precisionReduce_ true"
    set sender_ TCP
    set receiver_ TCPSink
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Agent/TCP set precisionReduce_ true
    Test/tcpA_precise_tahoe instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

Class Test/tcp_reno -superclass TestSuite
Test/tcp_reno instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcp_reno
    set guide_  "Reno TCP"
    set sender_ TCP/Reno
    set receiver_ TCPSink
    Test/tcp_reno instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}
Class Test/tcpA_reno -superclass TestSuite
Test/tcpA_reno instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA_reno{increase_0.41,decrease_0.75}
    set guide_  "Reno TCP, increase_num_ 0.41, decrease_num_ 0.75"
    set sender_ TCP/Reno
    set receiver_ TCPSink
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Test/tcpA_reno instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}
Class Test/tcpA_precise_reno -superclass TestSuite
Test/tcpA_precise_reno instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA_precise_reno{increase_0.41,decrease_0.75}
    set guide_  \
    "Reno TCP, increase_num_ 0.41, decrease_num_ 0.75, precisionReduce_ true"
    set sender_ TCP/Reno
    set receiver_ TCPSink
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Agent/TCP set precisionReduce_ true
    Test/tcpA_precise_reno instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

Class Test/tcp_newreno -superclass TestSuite
Test/tcp_newreno instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcp_newreno
    set guide_  "NewReno TCP"
    set sender_ TCP/Newreno
    set receiver_ TCPSink
    Test/tcp_newreno instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}
Class Test/tcpA_newreno -superclass TestSuite
Test/tcpA_newreno instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA_newreno{increase_0.41,decrease_0.75}
    set guide_  "NewReno TCP, increase_num_ 0.41, decrease_num_ 0.75"
    set sender_ TCP/Newreno
    set receiver_ TCPSink
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Test/tcpA_newreno instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}
Class Test/tcpA_precise_newreno -superclass TestSuite
Test/tcpA_precise_newreno instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	tcpA_precise_newreno{increase_0.41,decrease_0.75}
    set guide_  \
    "NewReno TCP, increase_num_ 0.41, decrease_num_ 0.75, precisionReduce_ true"
    set sender_ TCP/Newreno
    set receiver_ TCPSink
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Agent/TCP set precisionReduce_ true
    Test/tcpA_precise_newreno instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

############################################################


# IIAD, Inverse Increase Additive Decrease
Class Test/binomial1 -superclass TestSuite
Test/binomial1 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	binomial1{IIAD}
    set guide_	"TCP with IIAD"
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set decrease_num_ 0.33
    Agent/TCP set precisionReduce_ true
    Agent/TCP set k_parameter_ 1.0
    Agent/TCP set l_parameter_ 0.0
    Agent/TCP set windowOption_ 6
    Test/binomial1 instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

# SQRT, Square Root
Class Test/binomial2 -superclass TestSuite
Test/binomial2 instproc init {} {
    $self instvar net_ test_ sender_ receiver_ guide_
    set net_	net2
    set test_	binomial2{SQRT}
    set guide_	"TCP with SQRT"
    set sender_ TCP/Sack1
    set receiver_ TCPSink/Sack1 
    Agent/TCP set decrease_num_ 0.33
    Agent/TCP set precisionReduce_ true
    Agent/TCP set k_parameter_ 0.5
    Agent/TCP set l_parameter_ 0.5
    Agent/TCP set windowOption_ 6
    Test/binomial2 instproc run {} [Test/tcp info instbody run ]
    $self next pktTraceFile
}

TestSuite runTest 

