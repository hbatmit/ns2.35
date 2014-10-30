
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-friendly.tcl,v 1.78 2007/09/16 17:15:07 sallyfloyd Exp $
#

source misc_simple.tcl
source support.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP TFRC TFRC_ACK ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5 
Queue/RED set maxthresh_ 15
# The RED parameter defaults are being changed for automatic configuration.

##########################
Agent/TFRCSink set numPkts_ 1
# The default for numPkts_ might be changed to 3, at some point.
# But right now, the code for numPkts does not work, except for numPkts_ 1.
# With numPkts set to 1, TFRCSink is not robust to reordering.
##########################
Agent/TFRC set maxHeavyRounds_ 0
Agent/TCP set window_ 100

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish file {
        global quiet PERL
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp1.rands
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp1.rands &
        }
        ## now use default graphing tool to make a data file
        ## if so desired
#       exec csh figure2.com $file
#	exec cp temp1.rands temp.$file 
#	exec csh gnuplotA.com temp.$file $file
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
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
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
    Queue/RED set gentle_ true
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 0.15Kb 2ms RED
    $ns queue-limit $node_(r1) $node_(r2) 2
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

Class Topology/net2d -superclass Topology
Topology/net2d instproc init ns {
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
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    # 1.5Mb, 12.5 1500-byte pkts per 100 ms.
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

Class Topology/net3 -superclass Topology
Topology/net3 instproc init ns {
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
    $ns duplex-link $node_(r1) $node_(r2) 2Mb 300ms DropTail
    $ns queue-limit $node_(r1) $node_(r2) 1000
    $ns queue-limit $node_(r2) $node_(r1) 1000
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

#
# Arrange for TFCC stats to be dumped for $src every
# $interval seconds of simulation time
#
TestSuite instproc tfccDump { label src interval file } {
        $self instvar dump_inst_ ns_ f
        if ![info exists dump_inst_($src)] {
                set dump_inst_($src) 1
                $ns_ at 0.0 "$self tfccDump $label $src $interval $file"
                return
        }
        set nexttime [expr [$ns_ now] + $interval]
        $ns_ at $nexttime "$self tfccDump $label $src $interval $file"
        set report "[$ns_ now] $label [$src set ndatapack_] " 
	puts $file $report
}

#
# Arrange for TCP stats to be dumped for $tcp every
# $interval seconds of simulation time
#
TestSuite instproc pktsDump { label tcp interval file } {
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcp)] {
        set dump_inst_($tcp) 1
        $ns_ at 0.0 "$self pktsDump $label $tcp $interval $file"
        return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self pktsDump $label $tcp $interval $file"
    set report "[$ns_ now] $label [$tcp set ack_]"
    puts $file $report
}

# display graph of results
TestSuite instproc finish_1 testname {
        global quiet
        $self instvar topo_

        set graphfile temp.rands

        set awkCode  {
		{
                if ($2 == fid) { print $1, $3 - last; last = $3 }
		}
        }

        set f [open $graphfile w]
        puts $f "TitleText: $testname"
        puts $f "Device: Postscript"

        exec rm -f temp.p
        exec touch temp.p
        foreach i { 1 2 3 4 5} {
                exec echo "\n\"flow $i" >> temp.p
                exec awk $awkCode fid=$i temp.s > temp.$i
                exec cat temp.$i >> temp.p
                exec echo " " >> temp.p
        }

        exec cat temp.p >@ $f
        close $f
	exec cp -f $graphfile temp2.rands
        if {$quiet == "false"} {
                exec xgraph -bb -tk -x time -y packets temp2.rands &
        }
#	exec csh gnuplotB.com temp2.rands $testname
#       exec csh figure2.com temp.rands $testname

###        exit 0
}

TestSuite instproc runFriendly {} {
    $self instvar ns_ node_ interval_ dumpfile_ guide_

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 3]
    set tf2 [$ns_ create-connection TFRC $node_(s2) TFRCSink $node_(s4) 4]
    $ns_ at 0.0 "$tf1 start"
    $ns_ at 4.0 "$tf2 start"
    $ns_ at 30 "$tf1 stop"
    $ns_ at 30 "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_
}

TestSuite instproc runTcp {} {
    $self instvar ns_ node_ interval_ dumpfile_

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 3]
    set ftp1 [$tcp1 attach-app FTP]
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 4]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 4.0 "$ftp2 start"
    $ns_ at 30 "$ftp1 stop"
    $ns_ at 30 "$ftp2 stop"

    $self pktsDump 1 $tcp1 $interval_ $dumpfile_
    $self pktsDump 2 $tcp2 $interval_ $dumpfile_
}

TestSuite instproc runTcps {} {
    $self instvar ns_ node_ interval_ dumpfile_

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 0]
    set ftp1 [$tcp1 attach-app FTP] 
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP] 
    set tcp3 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 2]
    set ftp3 [$tcp3 attach-app FTP] 
    $ns_ at 8.0 "$ftp1 start"
    $ns_ at 12.0 "$ftp2 start"
    $ns_ at 16.0 "$ftp3 start"
    $ns_ at 24 "$ftp2 stop"
    $ns_ at 20 "$ftp3 stop"

    $self pktsDump 3 $tcp1 $interval_ $dumpfile_
    $self pktsDump 4 $tcp2 $interval_ $dumpfile_
    $self pktsDump 5 $tcp3 $interval_ $dumpfile_
}

Class Test/slowStartDiscount -superclass TestSuite
Test/slowStartDiscount instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	slowStartDiscount
    set guide_  "TFRC with discount_ true, for discounting older loss intervals."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 0
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Queue/RED set gentle_ false
    Test/slowStartDiscount instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Class Test/slowStartDiscount1 -superclass TestSuite
Test/slowStartDiscount1 instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	slowStartDiscount1
    set guide_  "TFRC with minDiscountRatio_ set to 0.25, for stronger discounting."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 0
    Agent/TFRCSink set minDiscountRatio_ 0.25
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Queue/RED set gentle_ false
    Test/slowStartDiscount1 instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Class Test/slowStartDiscountCA -superclass TestSuite
Test/slowStartDiscountCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	slowStartDiscountCA
    set guide_  "TFRC with ca_ true, for Sqrt(RTT) based congestion avoidance mode."
    Agent/TFRCSink set discount_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TFRCSink set discount_ 0
    Agent/TFRCSink set smooth_ 0
    Queue/RED set gentle_ false
    Test/slowStartDiscountCA instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Class Test/smooth -superclass TestSuite
Test/smooth instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	smooth
    set guide_  "TFRC with smooth_ true, for smoothly incorporating new loss intervals."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Test/smooth instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Class Test/smoothCA -superclass TestSuite
Test/smoothCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	smoothCA
    set guide_  "TFRC with smooth_, discount_, and ca_ all true." 
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Test/smoothCA instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Class Test/slowStart -superclass TestSuite
Test/slowStart instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	slowStart
    set guide_  "TFRC with smooth_, discount_, and ca_ all false."
    Agent/TFRCSink set discount_ 0
    Agent/TFRCSink set smooth_ 0
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Queue/RED set gentle_ false
    $self next pktTraceFile
}

Class Test/slowStartCA -superclass TestSuite
Test/slowStartCA instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net2
    set test_	slowStartCA
    set guide_  "TFRC with discount_ false and ca_ true."
    Agent/TFRCSink set discount_ 0
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Test/slowStartCA instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Test/slowStart instproc run {} {
    global quiet 
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"
    $ns_ at 30 "$tf1 stop"
    set tf2 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 1]
    if {$stopTime > 16} {
        $ns_ at 16 "$tf2 start"
        $ns_ at $stopTime "$tf2 stop"
        $self tfccDump 2 $tf2 $interval_ $dumpfile_
    }

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

# This test uses EWMA for estimating the loss event rate.
Class Test/slowStartEWMA -superclass TestSuite
Test/slowStartEWMA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	slowStartEWMA
    set guide_  "TFRC with EWMA for estimating the loss event rate."
    Agent/TFRCSink set algo_ 2
    Test/slowStartEWMA instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

# This test uses Fixed Windows for estimating the loss event rate.
Class Test/slowStartFixed -superclass TestSuite
Test/slowStartFixed instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	slowStartFixed
    set guide_  "TFRC with Fixed Windows for estimating the loss event rate."
    Agent/TFRCSink set algo_ 3
    Test/slowStartFixed instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Class Test/ecn -superclass TestSuite
Test/ecn instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	ecn
    set guide_  "TFRC with ECN."
    Agent/TFRC set ecn_ 1
    Queue/RED set setbit_ true
    Test/ecn instproc run {} [Test/slowStart info instbody run ]
    $self next pktTraceFile
}

Class Test/slowStartTcp -superclass TestSuite
Test/slowStartTcp instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	slowStartTcp
    set guide_  "TCP"
    $self next pktTraceFile
}
Test/slowStartTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 30 "$ftp1 stop"
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 1]
    set ftp2 [$tcp2 attach-app FTP]
    $ns_ at 16 "$ftp2 start"
    $ns_ at $stopTime "$ftp2 stop"

    $self tfccDump 1 $tcp1 $interval_ $dumpfile_
    $self tfccDump 2 $tcp2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/impulseDiscount -superclass TestSuite
Test/impulseDiscount instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseDiscount
    set guide_  "TFRC with discount_ true, for discounting older loss intervals."
    Agent/TFRCSink set discount_ 1
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Agent/TFRCSink set smooth_ 0
    Test/impulseDiscount instproc run {} [Test/impulseCA info instbody run ]
    $self next pktTraceFile
}

Class Test/impulseDiscountCA -superclass TestSuite
Test/impulseDiscountCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseDiscountCA
    set guide_  "TFRC with discount_ and ca_ true, smooth_ false."
    Agent/TFRCSink set discount_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TFRCSink set smooth_ 0
    Test/impulseDiscountCA instproc run {} [Test/impulseCA info instbody run ]
    $self next pktTraceFile
}

Class Test/impulse -superclass TestSuite
Test/impulse instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulse
    set guide_  "TFRC with smooth_, discount_, and ca_ all false."
    Agent/TFRCSink set discount_ 0
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Agent/TFRCSink set smooth_ 0
    Test/impulse instproc run {} [Test/impulseCA info instbody run ]
    $self next pktTraceFile
}

Class Test/impulseCA -superclass TestSuite
Test/impulseCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseCA
    set guide_  "TFRC with ca_ true, for Sqrt(RTT) based congestion avoidance mode."
    Agent/TFRCSink set discount_ 0
    Agent/TFRCSink set smooth_ 0
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
#    Test/impulseCA instproc run {} [Test/impulseCA info instbody run ]
    $self next pktTraceFile
}

Test/impulseCA instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"
    $ns_ at $stopTime "$tf1 stop"
    set tf2 [$ns_ create-connection TFRC $node_(s2) TFRCSink $node_(s4) 1]
    $ns_ at 10.0 "$tf2 start"
    $ns_ at 20.0 "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/impulseMultReport -superclass TestSuite
Test/impulseMultReport instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseMultReport
    set guide_  "TFRC with feedback four times per RTT, discount_ and ca_ false." 
    Agent/TFRCSink set NumFeedback_ 4
    Agent/TFRCSink set discount_ 0
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    $self next pktTraceFile
}
Test/impulseMultReport instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"
    set tf2 [$ns_ create-connection TFRC $node_(s2) TFRCSink $node_(s4) 1]
    $ns_ at 10.0 "$tf2 start"
    $ns_ at 20.0 "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

# Feedback 4 times per roundtrip time.
Class Test/impulseMultReportDiscount -superclass TestSuite
Test/impulseMultReportDiscount instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseMultReportDiscount
    set guide_  "TFRC with feedback four times per round-trip time, discount_ true, ca_ false."
    Agent/TFRCSink set NumFeedback_ 4
    Agent/TFRCSink set discount_ 1
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Test/impulseMultReportDiscount instproc run {} [Test/impulseMultReport info instbody run ]
    $self next pktTraceFile
}

# Feedback 4 times per roundtrip time.
Class Test/impulseMultReportDiscountCA -superclass TestSuite
Test/impulseMultReportDiscountCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseMultReportDiscountCA
    set guide_  "TFRC with feedback four times per RTT, discount_ and ca_ true."
    Agent/TFRCSink set NumFeedback_ 4
    Agent/TFRCSink set discount_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Test/impulseMultReportDiscountCA instproc run {} [Test/impulseMultReport info instbody run ]
    $self next pktTraceFile
}

Class Test/impulseMultReportCA -superclass TestSuite
Test/impulseMultReportCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseMultReportCA
    set guide_  "TFRC with feedback four times per RTT, discount_ false, ca_ true." 
    Agent/TFRCSink set NumFeedback_ 4
    Agent/TFRCSink set discount_ 0
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Test/impulseMultReportCA instproc run {} [Test/impulseMultReport info instbody run ]
    $self next pktTraceFile
}

Class Test/impulseTcp -superclass TestSuite
Test/impulseTcp instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	impulseTcp
    set guide_  "TFRC with discount_, smooth_, and ca_ false."
    $self next pktTraceFile
}
Test/impulseTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP]
    $ns_ at 10.0 "$ftp2 start"
    $ns_ at 20.0 "$ftp2 stop"

    $self tfccDump 1 $tcp1 $interval_ $dumpfile_
    $self tfccDump 2 $tcp2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

# Two TFRC connections and three TCP connections.
Class Test/two-friendly -superclass TestSuite
Test/two-friendly instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	two-friendly
    set guide_  "Two TFRC and three TCP connections, ca_ false."
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Agent/TCP set timerfix_ false
    # The default is being changed to true.
    $self next pktTraceFile
}

# Two TFRC connections and three TCP connections.
Class Test/two-friendlyCA -superclass TestSuite
Test/two-friendlyCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	two-friendlyCA
    Agent/TFRCSink set discount_ 0
    set guide_  "Two TFRC and three TCP connections, ca_ true."
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TCP set timerfix_ false
    # The default is being changed to true.
    Test/two-friendlyCA instproc run {} [Test/two-friendly info instbody run ]
    $self next pktTraceFile
}

Test/two-friendly instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 30.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    $self runFriendly 
    $self runTcps
    
    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

# Only TCP
Class Test/OnlyTcp -superclass TestSuite
Test/OnlyTcp instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	OnlyTcp
    set guide_  "Five TCP connections."
    Agent/TCP set timerfix_ false
    # The default is being changed to true.
    $self next pktTraceFile
}
Test/OnlyTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 30.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    $self runTcp 
    $self runTcps
    
    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

## Random factor added to sending times
Class Test/randomized -superclass TestSuite
Test/randomized instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	randomized
    set guide_  "Random delay added to sending times, ca_ false."
    Agent/TFRC set overhead_ 0.5
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Test/randomized instproc run {} [Test/slowStart info instbody run]
    $self next pktTraceFile
}

## Random factor added to sending times
Class Test/randomizedCA -superclass TestSuite
Test/randomizedCA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	randomizedCA
    set guide_  "Random delay added to sending times, with ca_."
    Agent/TFRC set overhead_ 0.5
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Test/randomizedCA instproc run {} [Test/slowStart info instbody run]
    $self next pktTraceFile
}

## Smaller random factor added to sending times
Class Test/randomized1 -superclass TestSuite
Test/randomized1 instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	randomized1
    set guide_  "Smaller random delay added to sending times, ca_ false."
    Agent/TFRC set overhead_ 0.1
    Agent/TFRC set df_ 0.25
    Agent/TFRC set ca_ 0
    Test/randomized1 instproc run {} [Test/slowStart info instbody run]
    $self next pktTraceFile
}

## Smaller random factor added to sending times
Class Test/randomized1CA -superclass TestSuite
Test/randomized1CA instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	randomized1CA
    set guide_  "Smaller random delay added to sending times, with ca_."
    Agent/TFRC set overhead_ 0.1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Test/randomized1CA instproc run {} [Test/slowStart info instbody run]
    $self next pktTraceFile
}

Class Test/slow -superclass TestSuite
Test/slow instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2a
    set test_	slow
    set guide_  "Very slow path."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    $self next pktTraceFile
}

Test/slow instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
#    [$ns_ link $node_(r1) $node_(r2)] set bandwidth 0.001Mb
#    [$ns_ link $node_(r1) $node_(r2)] set queue-limit 5
    set interval_ 100
    set stopTime 4000.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/twoDrops -superclass TestSuite
Test/twoDrops instproc init {} {
    $self instvar net_ test_ guide_ drops_ stopTime1_
    set net_	net2
    set test_	twoDrops
    set guide_  "TFRC, with two packets dropped near the beginning."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    set drops_ " 2 3 "
    set stopTime1_ 5.0
    $self next pktTraceFile
}

Class Test/manyDrops -superclass TestSuite 
Test/manyDrops instproc init {} { 
    $self instvar net_ test_ guide_ drops_ stopTime1_
    set net_    net2
    set test_   manyDrops 
    set guide_  "TFRC, with many packets dropped in the beginning."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
##  set stopTime1_ 100.0
    set stopTime1_ 100.0
#    set drops_ " 0 1 "
    set drops_ " 0 1 2 3 4 5 6 "
    Test/manyDrops instproc run {} [Test/twoDrops info instbody run ]
    $self next pktTraceFile
}

Test/twoDrops instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ drops_ stopTime1_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 1
    set stopTime $stopTime1_
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set em [new ErrorModule Fid]
    set lossylink_ [$ns_ link $node_(r1) $node_(r2)]
    $lossylink_ errormodule $em
    $em default pass
    set emod [$lossylink_ errormodule]
    set errmodel [new ErrorModel/List]
    $errmodel unit pkt
    $errmodel droplist $drops_
    $emod insert $errmodel
    $emod bind $errmodel 0

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"


    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/HighLoss -superclass TestSuite
Test/HighLoss instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ HighLoss	
    set guide_  "TFRC competing against a CBR flow, with high loss."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    set stopTime1_ 60
    $self next pktTraceFile
}
Test/HighLoss instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ stopTime1_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 1
    set stopTime $stopTime1_
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"

    set udp1 [$ns_ create-connection UDP $node_(s2) UDP $node_(s4) 1]
		set cbr1 [$udp1 attach-app Traffic/CBR]
		$cbr1 set rate_ 3Mb
		$ns_ at [expr $stopTime1_/3.0] "$cbr1 start"   
		$ns_ at [expr 2.0*$stopTime1_/3.0] "$cbr1 stop"   

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}
  
Class Test/HighLossShort -superclass TestSuite
Test/HighLossShort instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ HighLossShort	
    set guide_  "TFRC competing against a CBR flow, with high loss, ShortIntervals_."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    set stopTime1_ 80
    Test/HighLossShort instproc run {} [Test/HighLoss info instbody run ]
    $self next pktTraceFile
}

# PreciseLoss_ is turned off
Class Test/HighLossImprecise -superclass TestSuite
Test/HighLossImprecise instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ HighLossImprecise	
    set guide_  "TFRC competing against a CBR flow, with PreciseLoss_ off."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TFRCSink set PreciseLoss_ 0
    set stopTime1_ 80
    $self next pktTraceFile
}
Test/HighLossImprecise instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ stopTime1_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 1
    set stopTime $stopTime1_
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"

    set udp1 [$ns_ create-connection UDP $node_(s2) UDP $node_(s4) 1]
		set cbr1 [$udp1 attach-app Traffic/CBR]
		$cbr1 set rate_ 3Mb
		$ns_ at [expr $stopTime1_/4.0] "$cbr1 start"   
		$ns_ at [expr 2.0*$stopTime1_/4.0] "$cbr1 stop"   

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}
  
Class Test/HighLossMinRTO -superclass TestSuite
Test/HighLossMinRTO instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ HighLossMinRTO	
    set guide_  "TFRC competing against a CBR flow, with high loss, high MinRTO."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TFRC set minrto_ 1.0
    # Minimum RTO of one second.  This slows down the TFRC flow.
    set stopTime1_ 60
    Test/HighLossMinRTO instproc run {} [Test/HighLoss info instbody run ]
    $self next pktTraceFile
}
  
Class Test/HighLossConservative -superclass TestSuite
Test/HighLossConservative instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ HighLossConservative	
    set guide_  "TFRC and CBR, with a conservative_ response to heavy congestion."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TFRC set conservative_ true
    set stopTime1_ 60
    # Test/HighLossConservative instproc run {} [Test/HighLoss info instbody run ]
    $self next pktTraceFile
}
Test/HighLossConservative instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ stopTime1_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 1
    set stopTime $stopTime1_
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"

    set udp1 [$ns_ create-connection UDP $node_(s2) UDP $node_(s4) 1]
		set cbr1 [$udp1 attach-app Traffic/CBR]
		$cbr1 set rate_ 3Mb
		$ns_ at [expr $stopTime1_/3.0] "$cbr1 start"   
		$ns_ at [expr 2.0*$stopTime1_/3.0] "$cbr1 stop"   

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}


Class Test/HighLossTCP -superclass TestSuite
Test/HighLossTCP instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ HighLossTCP	
    set guide_  "TCP competing against a CBR flow."
    Agent/TFRCSink set discount_ 1
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    set stopTime1_ 60
    $self next pktTraceFile
}
Test/HighLossTCP instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ stopTime1_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 1
    set stopTime $stopTime1_
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    set udp1 [$ns_ create-connection UDP $node_(s2) UDP $node_(s4) 1]
		set cbr1 [$udp1 attach-app Traffic/CBR]
		$cbr1 set rate_ 3Mb
		$ns_ at [expr $stopTime1_/3.0] "$cbr1 start"   
		$ns_ at [expr 2.0*$stopTime1_/3.0] "$cbr1 stop"   

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_"
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}
  
Class Test/TFRC_FTP -superclass TestSuite
Test/TFRC_FTP instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ TFRC_FTP	
    set guide_  "TFRC with a data source with limited, bursty data."
    Agent/TFRC set SndrType_ 1 
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TFRC set discount_ 1
    Agent/TCP set oldCode_ false
    Agent/TFRC set idleFix_ true ;		
    set stopTime1_ 15
    $self next pktTraceFile
}
Test/TFRC_FTP instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ stopTime1_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 1
    set stopTime $stopTime1_
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set ftp [new Application/FTP]
    $ftp attach-agent $tf1
    $ns_ at 0 "$ftp produce 100"
    $ns_ at 5 "$ftp producemore 100"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}
  
Class Test/TFRC_CBR -superclass TestSuite
Test/TFRC_CBR instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_
    set net_	net2
    set test_ TFRC_CBR	
    set guide_  "TFRC with a data source with CBR data."
    Agent/TFRC set SndrType_ 1 
    Agent/TFRCSink set smooth_ 1
    Agent/TFRC set df_ 0.95
    Agent/TFRC set ca_ 1
    Agent/TFRC set discount_ 1
    Agent/TCP set oldCode_ false
    set stopTime1_ 15
    $self next pktTraceFile
}
Test/TFRC_CBR instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ stopTime1_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 1
    set stopTime $stopTime1_
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set cbr [new Application/Traffic/CBR]
    $cbr attach-agent $tf1
    $cbr set rate_ 10Kb
    $ns_ at 0 "$cbr start"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}
  
Class Test/printLosses -superclass TestSuite
Test/printLosses instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	printLosses
    set guide_  "One TFRC flow, with the loss intervals from the TFRC receiver."
    Agent/TFRCSink set printLosses_ 1
    Agent/TFRCSink set printLoss_ 1
    $self next pktTraceFile
}
Test/printLosses instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 3.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_"
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

#
# This test shows that a few lost packets are not counted as lost
#   by the losses array.  I think this is for sequential packet losses.
#
Class Test/printLossesShort -superclass TestSuite
Test/printLossesShort instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	printLossesShort
    set guide_  "A TFRC-SP flow with ShortIntervals_, loss intervals from the TFRC receiver."
    $self next pktTraceFile
}
Test/printLossesShort instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 4.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    # set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set tf1 [new Agent/TFRC]
    set tf1Dest [new Agent/TFRCSink]
    $tf1 set fid_ 0
    $tf1 set voip_ 1
    $tf1 set packetSize_ 125
    $tf1Dest set ShortIntervals_ 1
    $tf1Dest set fid_ 0
    $ns_ attach-agent $node_(s1) $tf1
    $ns_ attach-agent $node_(s3) $tf1Dest
    $tf1Dest set printLosses_ 1
    $tf1Dest set printLoss_ 1
    $ns_ connect $tf1 $tf1Dest
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 125
    $cbr0 set interval_ 0.02
    $cbr0 attach-agent $tf1
    $ns_ at 0.0 "$cbr0 start"

    set tf2 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 1]
    $ns_ at 0.2 "$tf2 start"


    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_"
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/printLossesShort3 -superclass TestSuite
Test/printLossesShort3 instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	printLossesShort3
    set guide_  "A TFRC-SP flow with ShortIntervals_ 3, loss intervals from the TFRC receiver."
    $self next pktTraceFile
}
Test/printLossesShort3 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 4.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    # set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set tf1 [new Agent/TFRC]
    set tf1Dest [new Agent/TFRCSink]
    $tf1 set fid_ 0
    $tf1 set voip_ 1
    $tf1 set packetSize_ 125
    $tf1Dest set ShortIntervals_ 3
    $tf1Dest set ShortRtts_ 3 
    $tf1Dest set fid_ 0
    $ns_ attach-agent $node_(s1) $tf1
    $ns_ attach-agent $node_(s3) $tf1Dest
    $tf1Dest set printLosses_ 1
    $tf1Dest set printLoss_ 1
    $ns_ connect $tf1 $tf1Dest
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 125
    $cbr0 set interval_ 0.02
    $cbr0 attach-agent $tf1
    $ns_ at 0.0 "$cbr0 start"

    set tf2 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 1]
    $ns_ at 0.2 "$tf2 start"


    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_"
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

TestSuite instproc printpkts { label tcp } {
        puts "tcp $label highest_seqment_acked [$tcp set ack_]"
}

TestSuite instproc printTFRCpkts { label src } {
        puts "tfrc $label [$src set ndatapack_] " 
}

Class Test/goodTFRC superclass TestSuite
Test/goodTFRC instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net2
    set test_	goodTFRC
    set guide_  "One TFRC flow, no reordering and no extra drops."
    set period_ 10000.0
    $self next pktTraceFile
}
Test/goodTFRC instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 20.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"
    # $ns_ at $stopTime0 "$self printTFRCpkts 0 $tf1"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/droppedTFRC superclass TestSuite
Test/droppedTFRC instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_    net2
    set test_   droppedTFRC
    set guide_  "One TFRC flow, with extra dropped packets."
    set period_ 40.0
    Test/droppedTFRC instproc run {} [Test/goodTFRC info instbody run ]
    $self next pktTraceFile
}

Class Test/delayedTFRC superclass TestSuite
Test/delayedTFRC instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_    net2
    set test_   delayedTFRC
    set guide_  "One TFRC flow, with some packets delayed 0.03 seconds, numPkts_ 1."
    set period_ 40.0
    ErrorModel set delay_pkt_ true
    ErrorModel set drop_ false
    ErrorModel set delay_ 0.03
    Agent/TFRCSink set numPkts_ 1
    Test/delayedTFRC instproc run {} [Test/goodTFRC info instbody run ]
    $self next pktTraceFile
}

Class Test/delayedTFRC1 superclass TestSuite
Test/delayedTFRC1 instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_    net2
    set test_   delayedTFRC1
    set guide_  "One TFRC flow, with some packets delayed 0.03 seconds, numPkts_ 5."
    set period_ 40.0
    ErrorModel set delay_pkt_ true
    ErrorModel set drop_ false
    ErrorModel set delay_ 0.03
    Agent/TFRCSink set numPkts_ 5
    Test/delayedTFRC1 instproc run {} [Test/goodTFRC info instbody run ]
    $self next pktTraceFile
}

Class Test/delayedTFRC2 superclass TestSuite
Test/delayedTFRC2 instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_    net2
    set test_   delayedTFRC2
    set guide_  "One TFRC flow, with some packets delayed 0.01 seconds, numPkts_ 3."
    set period_ 40.0
    ErrorModel set delay_pkt_ true
    ErrorModel set drop_ false
    ErrorModel set delay_ 0.01
    Agent/TFRCSink set numPkts_ 3
    Test/delayedTFRC2 instproc run {} [Test/goodTFRC info instbody run ]
    $self next pktTraceFile
}

Class Test/goodTCP superclass TestSuite
Test/goodTCP instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_	net2
    set test_	goodTCP
    set guide_  "One TCP flow, no reordering and no extra drops."
    set list_ {50000 50001}
    set period_ 1000.0
    $self next pktTraceFile
}
Test/goodTCP instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ list_ period_
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 20.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"
    $self pktsDump 1 $tcp1 $interval_ $dumpfile_

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 200.0 $period_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"
    $ns_ at $stopTime0 "$self printpkts 0 $tcp1"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/droppedTCP superclass TestSuite
Test/droppedTCP instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_    net2
    set test_   droppedTCP
    set guide_  "One TCP flow, with extra dropped packets."
    set period_ 40.0
    Test/droppedTCP instproc run {} [Test/goodTCP info instbody run ]
    $self next pktTraceFile
}

Class Test/delayedTCP superclass TestSuite
Test/delayedTCP instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_    net2
    set test_   delayedTCP
    set guide_  "One TCP flow, with some packets delayed 0.03 seconds."
    set period_ 40.0
    ErrorModel set delay_pkt_ true
    ErrorModel set drop_ false
    ErrorModel set delay_ 0.03
    Test/delayedTCP instproc run {} [Test/goodTCP info instbody run ]
    $self next pktTraceFile
}

Class Test/delayedTCP2 superclass TestSuite
Test/delayedTCP2 instproc init {} {
    $self instvar net_ test_ guide_ list_ period_
    set net_    net2
    set test_   delayedTCP2
    set guide_  "One TCP flow, with some packets delayed 0.01 seconds."
    set period_ 40.0
    ErrorModel set delay_pkt_ true
    ErrorModel set drop_ false
    ErrorModel set delay_ 0.01
    Test/delayedTCP2 instproc run {} [Test/goodTCP info instbody run ]
    $self next pktTraceFile
}

Class Test/initRate superclass TestSuite
Test/initRate instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net2
    set test_	initRate
    set guide_  "One TFRC flow, initial rate of one packet per RTT."
    set period_ 10000.0
    Agent/TFRC set rate_init_ 1 
    Agent/TFRC set rate_init_option_ 1
    $self next pktTraceFile
}
Test/initRate instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TFRC set SndrType_ 1
    Agent/TFRC set idleFix_ true ;		
    set interval_ 0.1
    set stopTime 2.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set ftp [new Application/FTP]
    $ftp attach-agent $tf1
    $ns_ at 0 "$ftp produce 50"
    $ns_ at 1.5 "$ftp producemore 50"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    #$self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"
    # $ns_ at $stopTime0 "$self printTFRCpkts 0 $tf1"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/initRateLarge superclass TestSuite
Test/initRateLarge instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net2
    set test_	initRateLarge
    set guide_  "One TFRC flow, initial rate of 4 packets per RTT."
    set period_ 10000.0
    Agent/TFRC set rate_init_ 4.0 
    Agent/TFRC set rate_init_option_ 1
    Test/initRateLarge instproc run {} [Test/initRate info instbody run ]
    $self next pktTraceFile
}

Class Test/initRateLarger superclass TestSuite
Test/initRateLarger instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net2
    set test_	initRateLarger
    set guide_  "One TFRC flow, initial rate of 8 packets per RTT."
    set period_ 10000.0
    Agent/TFRC set rate_init_ 8.0 
    Agent/TFRC set rate_init_option_ 1
    Test/initRateLarger instproc run {} [Test/initRate info instbody run ]
    $self next pktTraceFile
}

Class Test/initRateRFC3390 superclass TestSuite
Test/initRateRFC3390 instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net2
    set test_	initRateRFC3390
    set guide_  "One TFRC flow, initial rate from RFC 3390."
    set period_ 10000.0
    Agent/TFRC set rate_init_option_ 2
    Test/initRateRFC3390 instproc run {} [Test/initRate info instbody run ]
    $self next pktTraceFile
}

TestSuite instproc printdrops { fid fmon } {
        set fcl [$fmon classifier]; # flow classifier
        #
        set flow [$fcl lookup auto 0 0 $fid]
        puts "fid: $fid drops [$flow set pdrops_] marks [$flow set pmarks_]"
        puts "fid: $fid packets [$flow set pdepartures_] _bytes [$flow set bdepartures_]"
}

Class Test/tfrcOnly superclass TestSuite
Test/tfrcOnly instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net2d
    set test_	tfrcOnly
    set guide_  "One VoIP TFRC flow."
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    $self next pktTraceFile
}
Test/tfrcOnly instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ 
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 20.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]
    set pktsize 120
    set cbrInterval 0.01

    set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon    

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    $ns_ at 0.0 "$ns_ bandwidth $node_(r1) $node_(r2) 0.2Mbps duplex"
    $ns_ queue-limit $node_(r1) $node_(r2) 50

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $tf1 set packetSize_ $pktsize
    set cbr [new Application/Traffic/CBR]
    $cbr set packetSize_ $pktsize
    $cbr set interval_ $cbrInterval
    $cbr attach-agent $tf1
    $ns_ at 2.0 "$cbr start"
    $ns_ at $stopTime0 "$cbr stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_ 

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime0 "$self printdrops 0 $fmon"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tfrcOnlyHighLoss superclass TestSuite
Test/tfrcOnlyHighLoss instproc init {} {
    $self instvar net_ test_ guide_ voip
    set net_	net2d
    set test_	tfrcOnlyHighLoss
    set guide_  "One VoIP TFRC flow, high loss."
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    $self next pktTraceFile
}
Test/tfrcOnlyHighLoss instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ voip
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 20.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]
    set pktsize 120
    set cbrInterval 0.01
    set droprate 0.2

    set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon    

    set lossylink_ [$ns_ link $node_(r1) $node_(r2)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Uniform $droprate pkt ]
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0 5
    $em default pass

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    $ns_ at 0.0 "$ns_ bandwidth $node_(r1) $node_(r2) 0.2Mbps duplex"
    $ns_ queue-limit $node_(r1) $node_(r2) 50

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $tf1 set packetSize_ $pktsize
    set cbr [new Application/Traffic/CBR]
    $cbr set packetSize_ $pktsize
    $cbr set interval_ $cbrInterval
    $cbr attach-agent $tf1
    $ns_ at 2.0 "$cbr start"
    $ns_ at $stopTime0 "$cbr stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_ 

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime0 "$self printdrops 0 $fmon"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tfrcOnlyHighLoss1 superclass TestSuite
Test/tfrcOnlyHighLoss1 instproc init {} {
    $self instvar net_ test_ guide_ voip
    set net_	net2d
    set test_	tfrcOnlyHighLoss1
    set guide_  "One VoIP TFRC flow, high loss, old measurement of loss event rate."
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 11
    Test/tfrcOnlyHighLoss1 instproc run {} [Test/tfrcOnlyHighLoss info instbody run ]
    $self next pktTraceFile
}

Class Test/voip superclass TestSuite
Test/voip instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net2d
    set test_	voip
    set guide_  "One VoIP TFRC flow and one TCP flow, different packet sizes."
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    $self next pktTraceFile
}
Test/voip instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ 
    puts "Guide: $guide_"
    $self setTopo
    set interval_ 0.1
    set stopTime 20.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]
    set pktsize 120
    set cbrInterval 0.01

    set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon    

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    $ns_ at 0.0 "$ns_ bandwidth $node_(r1) $node_(r2) 0.2Mbps duplex"
    $ns_ queue-limit $node_(r1) $node_(r2) 50 

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $tf1 set packetSize_ $pktsize
    set cbr [new Application/Traffic/CBR]
    $cbr set packetSize_ $pktsize
    $cbr set interval_ $cbrInterval
    $cbr attach-agent $tf1
    $ns_ at 2.0 "$cbr start"
    $ns_ at $stopTime0 "$cbr stop"

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 1]
    $tcp1 set window_ 10
    $tcp1 set packetSize_ 1460
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $stopTime0 "$ftp1 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_ 
    $self pktsDump 2 $tcp1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime0 "$self printdrops 0 $fmon; $self printdrops 1 $fmon"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/voipHeader superclass TestSuite
Test/voipHeader instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net2d
    set test_	voip
    set guide_  "One VoIP TFRC flow with headers, and one TCP flow."
    Agent/TFRC set useHeaders_ true ; 
    Agent/TFRC set headersize_ 32;
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    Test/voipHeader instproc run {} [Test/voip info instbody run ]
    $self next pktTraceFile
}

Class Test/voipNoHeader superclass TestSuite
Test/voipNoHeader instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net2d
    set test_	voip
    set guide_  "One VoIP TFRC flow without headers, and one TCP flow."
    Agent/TFRC set useHeaders_ false ; 
    Agent/TFRC set headersize_ 32;
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    Test/voipNoHeader instproc run {} [Test/voip info instbody run ]
    $self next pktTraceFile
}

Class Test/voipEcn superclass TestSuite
Test/voipEcn instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net2d
    set test_	voipEcn
    set guide_  "One ECN VoIP TFRC flow and one TCP flow, different packet sizes."
    Agent/TFRC set useHeaders_ false ; 
    Agent/TFRC set ecn_ 1
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    Agent/TCP set ecn_ 1
    Queue/RED set setbit_ true
    Test/voipEcn instproc run {} [Test/voip info instbody run ]
    $self next pktTraceFile
}

Class Test/noVoip superclass TestSuite
Test/noVoip instproc init {} {
    $self instvar net_ test_ guide_ 
    set net_	net2d
    set test_	noVoip
    set guide_  "One TFRC flow (not voip) and one TCP flow, different packet sizes."
    Agent/TFRC set voip_ 0
    Test/noVoip instproc run {} [Test/voip info instbody run ]
    $self next pktTraceFile
}

Class Test/idleTfrc superclass TestSuite
Test/idleTfrc instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net3
    set test_	idleTfrc
    set guide_  "TFRC with two idle periods during slow-start."
    set period_ 10000.0
    $self next pktTraceFile
}
Test/idleTfrc instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TFRC set SndrType_ 1
    Agent/TFRC set datalimited_ 1
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    Agent/TFRC set packetSize_ 160
    Agent/TFRC set voip_max_pkt_rate_ 50
    set interval_ 1.0
    set start_time 0.0
    set stopTime 60.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 160
    $cbr0 set interval_ 0.02
    $cbr0 attach-agent $tf1

    $ns_ at $start_time "$cbr0 start"
    $ns_ at 10.0 "$cbr0 stop"
    $ns_ at 20.0 "$cbr0 start"
    $ns_ at 25.0 "$cbr0 stop"
    $ns_ at 35.0 "$cbr0 start"
    $ns_ at $stopTime "$cbr0 stop"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    $ns_ run
}

Class Test/idleTcp superclass TestSuite
Test/idleTcp instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net3
    set test_	idleTcp
    set guide_  "TCP with two idle periods during slow-start."
    set period_ 10000.0
    $self next pktTraceFile
}
Test/idleTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TCP set packetSize_ 160
    Agent/TCP set window_ 30
    set interval_ 1.0
    set start_time 0.0
    set stopTime 60.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s3) 0]
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 160
    $cbr0 set interval_ 0.02
    $cbr0 attach-agent $tcp1

    $ns_ at $start_time "$cbr0 start"
    $ns_ at 10.0 "$cbr0 stop"
    $ns_ at 20.0 "$cbr0 start"
    $ns_ at 25.0 "$cbr0 stop"
    $ns_ at 35.0 "$cbr0 start"
    $ns_ at $stopTime "$cbr0 stop"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self pktsDump 1 $tcp1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    $ns_ run
}

Class Test/idleTfrc1 superclass TestSuite
Test/idleTfrc1 instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net3
    set test_	idleTfrc1
    set guide_  "TFRC with only one idle period during slow-start."
    set period_ 10000.0
    $self next pktTraceFile
}
Test/idleTfrc1 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TFRC set SndrType_ 1
    Agent/TFRC set datalimited_ 1
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    Agent/TFRC set packetSize_ 160
    Agent/TFRC set voip_max_pkt_rate_ 50
    set interval_ 1.0
    set start_time 0.0
    set stopTime 60.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 160
    $cbr0 set interval_ 0.02
    $cbr0 attach-agent $tf1

    $ns_ at $start_time "$cbr0 start"
    $ns_ at 10.0 "$cbr0 stop"
    $ns_ at 20.0 "$cbr0 start"
    $ns_ at 30.0 "$cbr0 stop"
    $ns_ at 40.0 "$cbr0 start"
    $ns_ at $stopTime "$cbr0 stop"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    $ns_ run
}

Class Test/idleTcp1 superclass TestSuite
Test/idleTcp1 instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net3
    set test_	idleTcp1
    set guide_  "TCP with only one idle period during slow-start."
    set period_ 10000.0
    $self next pktTraceFile
}
Test/idleTcp1 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TCP set packetSize_ 160
    Agent/TCP set window_ 30
    set interval_ 1.0
    set start_time 0.0
    set stopTime 60.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s3) 0]
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 160
    $cbr0 set interval_ 0.02
    $cbr0 attach-agent $tcp1

    $ns_ at $start_time "$cbr0 start"
    $ns_ at 10.0 "$cbr0 stop"
    $ns_ at 20.0 "$cbr0 start"
    $ns_ at 30.0 "$cbr0 stop"
    $ns_ at 40.0 "$cbr0 start"
    $ns_ at $stopTime "$cbr0 stop"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self pktsDump 1 $tcp1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    $ns_ run
}

Class Test/shortIdleTcp superclass TestSuite
Test/shortIdleTcp instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net3
    set test_	shortIdleTcp
    set guide_  "TCP with two short idle periods during slow-start."
    set period_ 10000.0
    $self next pktTraceFile
}
Test/shortIdleTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TCP set packetSize_ 160
    Agent/TCP set window_ 30
    set interval_ 1.0
    set start_time 0.0
    set stopTime 25.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s3) 0]
    set ftp [new Application/FTP]
    $ftp attach-agent $tcp1
    $ns_ at 0 "$ftp produce 280"
    $ns_ at 9.2 "$ftp producemore 280"
    $ns_ at 15.8 "$ftp producemore 280"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self pktsDump 1 $tcp1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    $ns_ run
}

Class Test/idleTfrc1 superclass TestSuite
Test/idleTfrc1 instproc init {} {
    $self instvar net_ test_ guide_ period_
    set net_	net3
    set test_	idleTfrc1
    set guide_  "TFRC with only one idle period during slow-start."
    set period_ 10000.0
    $self next pktTraceFile
}
Test/idleTfrc1 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_ guide_ period_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TFRC set SndrType_ 1
    Agent/TFRC set datalimited_ 1
    Agent/TFRC set voip_ 1
    Agent/TFRCSink set ShortIntervals_ 1
    Agent/TFRC set packetSize_ 160
    Agent/TFRC set voip_max_pkt_rate_ 50
    set interval_ 1.0
    set start_time 0.0
    set stopTime 60.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 160
    $cbr0 set interval_ 0.02
    $cbr0 attach-agent $tf1

    $ns_ at $start_time "$cbr0 start"
    $ns_ at 10.0 "$cbr0 stop"
    $ns_ at 20.0 "$cbr0 start"
    $ns_ at 30.0 "$cbr0 stop"
    $ns_ at 40.0 "$cbr0 start"
    $ns_ at $stopTime "$cbr0 stop"

    $self dropPktsPeriodic [$ns_ link $node_(r2) $node_(s3)] 0 1000.0 $period_

    $self tfccDump 1 $tf1 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $ns_ at $stopTime "$self cleanupAll $testName_" 
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    $ns_ run
}

TestSuite runTest

