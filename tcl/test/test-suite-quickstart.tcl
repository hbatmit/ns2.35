#
# Copyright (c) 2003  International Computer Science Institute
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
#      This product includes software developed by ACIRI, the AT&T
#      Center for Internet Research at ICSI (the International Computer
#      Science Institute).
# 4. Neither the name of ACIRI nor of ICSI may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

source misc_simple.tcl
source support.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP QS ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set bugfix_ss_ 0

set wrap 90
set wrap1 [expr $wrap * 512 + 40]

Queue set util_weight_ 2  
# 18 seconds
Agent/QSAgent set alloc_rate_ 0.5    
# up to 0.5 of link bandwidth.
Agent/QSAgent set qs_enabled_ 1
Agent/QSAgent set state_delay_ 0.35  
# 0.35 seconds for past approvals
Agent/QSAgent set rate_function_ 1

Agent/TCPSink set qs_enabled_ true
Agent/TCP set qs_enabled_ true

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish {file stoptime} {
        global quiet PERL wrap wrap1

        set space 512
        if [string match {*full*} $file] {
                exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
                   $PERL ../../bin/raw2xg -c -n $space -s 0.01 -m $wrap1 -t $file > temp.rands
                #exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
                #   $PERL ../../bin/raw2xg -a -c -f -p -y -n $space -s 0.01 -m $wrap1 -t $file >> temp.rands
        } else {
                exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
                  $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
                #exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
                #  $PERL ../../bin/raw2xg -a -c -p -y -s 0.01 -m $wrap -t $file \
                #  >> temp.rands
        } 
#	exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
#	  $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
        exec echo $stoptime 0 >> temp.rands 
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
        }
	#exec csh gnuplotA.com temp.rands quickstart
	#exec csh gnuplotA2.com temp.rands quickstart
        exit 0
}


TestSuite instproc emod {} {
        $self instvar topo_
        $topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        return $errmodule
}


TestSuite instproc drop_pkts pkts {
    $self instvar ns_ errmodel
    set emod [$self emod]
    set errmodel [new ErrorModel/List]
    $errmodel droplist $pkts
    $emod insert $errmodel
    $emod bind $errmodel 1
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
    $ns duplex-link $node_(s1) $node_(r1) 1000Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 1000Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 100Mb 500ms RED
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100 
    $ns duplex-link $node_(s3) $node_(r2) 1000Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 1000Mb 5ms DropTail

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(s1) $node_(r1)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass
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
    $ns duplex-link $node_(s1) $node_(r1) 1000Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 1000Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 100Mb 500ms RED
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100 
    $ns duplex-link $node_(s3) $node_(r2) 1000Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 1000Mb 5ms DropTail
}

Class Topology/net4 -superclass Topology
Topology/net4 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 1000Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 1000Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 10Mb 200ms RED
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50 
    $ns duplex-link $node_(s3) $node_(r2) 1000Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 1000Mb 5ms DropTail
}

Class Test/no_quickstart -superclass TestSuite
Test/no_quickstart instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ no_quickstart	
    set guide_  "Two TCPs, no quickstart."
    set sndr TCP/Newreno
    set rcvr TCPSink
    set qs OFF
    Agent/QSAgent set qs_enabled_ 0
    $self next pktTraceFile
}
Test/no_quickstart instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 6

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s3) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/quickstart -superclass TestSuite
Test/quickstart instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ quickstart	
    set guide_  "Two TCPs, TCP/Newreno, with QuickStart."
    set sndr TCP/Newreno
    set rcvr TCPSink
    set qs ON
    Test/quickstart instproc run {} [Test/no_quickstart info instbody run ]
    $self next pktTraceFile
}

Class Test/quickstart1 -superclass TestSuite
Test/quickstart1 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ quickstart1	
    set guide_  "Two TCPs, plain TCP, with QuickStart."
    set sndr TCP
    set rcvr TCPSink
    set qs ON
    Test/quickstart1 instproc run {} [Test/no_quickstart info instbody run ]
    $self next pktTraceFile
}

Class Test/quickstart2 -superclass TestSuite
Test/quickstart2 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ quickstart2	
    set guide_  "Two TCPs, Reno TCP, with QuickStart."
    set sndr TCP/Reno
    set rcvr TCPSink
    set qs ON
    Test/quickstart2 instproc run {} [Test/no_quickstart info instbody run ]
    $self next pktTraceFile
}

Class Test/quickstart3 -superclass TestSuite
Test/quickstart3 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ quickstart3	
    set guide_  "Two TCPs, NewReno TCP, with QuickStart."
    set sndr TCP/Newreno
    set rcvr TCPSink
    set qs ON
    Test/quickstart3 instproc run {} [Test/no_quickstart info instbody run ]
    $self next pktTraceFile
}

Class Test/quickstart4 -superclass TestSuite
Test/quickstart4 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ quickstart4	
    set guide_  "Two TCPs, Sack TCP, with QuickStart."
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    set qs ON
    Test/quickstart4 instproc run {} [Test/no_quickstart info instbody run ]
    $self next pktTraceFile
}

Class Test/quickstart4full -superclass TestSuite
Test/quickstart4full instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net2
    set test_ quickstart4	
    set guide_  "Two TCPs, Sack Full TCP.  QuickStart not added to Full TCP yet."
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    set qs ON
    $self next pktTraceFile
}
Test/quickstart4full instproc run {} {
    global quiet wrap1 wrap
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 6

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set wrap $wrap1
    set fid 1
    set tcp2 [new Agent/TCP/FullTcp/Sack]
    set sink [new Agent/TCP/FullTcp/Sack]
    $ns_ attach-agent $node_(s1) $tcp2
    $ns_ attach-agent $node_(s3) $sink
    $tcp2 set fid_ $fid
    $sink set fid_ $fid
    $ns_ connect $tcp2 $sink
    # set up TCP-level connections
    $sink listen ; # will figure out who its peer is

    #set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s3) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}


Class Test/high_request -superclass TestSuite
Test/high_request instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ high_request	
    set guide_  "A high Quick-Start request."
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    Agent/QSAgent set alloc_rate_ 0.01
    Agent/QSAgent set algorithm_ 1
    Agent/TCP set qs_request_mode_ 0
    set qs ON
    $self next pktTraceFile
}
Test/high_request instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 6

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s3) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 1000
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/bad_router -superclass TestSuite
Test/bad_router instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ bad_router	
    set guide_  "Not all routers support quickstart."
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    set qs ON
    $self next pktTraceFile
}
Test/bad_router instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set router $node_(r2)
    [$router qs-agent] set qs_enabled_ 0
    set stopTime 6

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s3) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/changing_rtt -superclass TestSuite
Test/changing_rtt instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ changing_rtt	
    set guide_  "Changing round-trip times."
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    set qs ON
    $self next pktTraceFile
}
Test/changing_rtt instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 6

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s3) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"
    $ns_ at 3.1 "$ns_ delay $node_(r1) $node_(r2) 100ms duplex"
    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/changing_rtt1 -superclass TestSuite
Test/changing_rtt1 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ changing_rtt1	
    set guide_  "Changing round-trip times."
    set sndr TCP/Newreno
    set rcvr TCPSink
    set qs ON
    Test/changing_rtt1 instproc run {} [Test/changing_rtt info instbody run ]
    $self next pktTraceFile
}

Class Test/no_acks_back -superclass TestSuite
Test/no_acks_back instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ no_acks_back	
    set guide_  "After the first exchange, sender receives no acks."
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    set qs ON
    $self next pktTraceFile
}

Test/no_acks_back instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 10

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s4) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"
    $ns_ at 3.0 "$ns_ delay $node_(r2) $node_(s4) 10000ms duplex"
    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/pkt_drops -superclass TestSuite
Test/pkt_drops instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net2
    set test_ pkt_drops	
    set guide_  "Packets are dropped in the initial window after quickstart."
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    set qs ON
    $self next pktTraceFile
}

Test/pkt_drops instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 20

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s4) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    $tcp2 set tcp_qs_recovery_ true
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"

    $self drop_pkts {5 6}

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

proc printdrops { fid fmon } {
        set fcl [$fmon classifier]; # flow classifier
        set flow [$fcl lookup auto 0 0 $fid]
        if {$flow != ""} {
          puts "fid: $fid per-link total_packets [$flow set pdepartures_]"
          puts "fid: $fid per-link total_bytes [$flow set bdepartures_]"
          puts "fid: $fid per-link total_drops [$flow set pdrops_]"
          puts "fid: $fid per-link qs_pkts [$flow set qs_pkts_]"
          puts "fid: $fid per-link qs_bytes [$flow set qs_bytes_]"
          puts "fid: $fid per-link qs_drops [$flow set qs_drops_]"
        }
}

Class Test/many_requests -superclass TestSuite
Test/many_requests instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net4
    set test_ many_requests	
    set guide_  "Many Quick-Start requests."
    set sndr TCP/Newreno
    set rcvr TCPSink
    set qs ON
    $self next pktTraceFile
}
Test/many_requests instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 10

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s3) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 100
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 400"

    set tcp3 [$ns_ create-connection $sndr $node_(s2) $rcvr $node_(s4) 2]
    $tcp3 set window_ 1000
    $tcp3 set rate_request_ 100
    set ftp3 [new Application/FTP]
    $ftp3 attach-agent $tcp3
    $ns_ at 3.0 "$ftp3 produce 200"

    set tcp4 [$ns_ create-connection $sndr $node_(s2) $rcvr $node_(s4) 3]
    $tcp4 set window_ 1000
    $tcp4 set rate_request_ 100
    set ftp4 [new Application/FTP]
    $ftp4 attach-agent $tcp4
    $ns_ at 4.0 "$ftp4 produce 100"

    set tcp5 [$ns_ create-connection $sndr $node_(s2) $rcvr $node_(s4) 4]
    $tcp5 set window_ 1000
    $tcp5 set rate_request_ 100
    set ftp5 [new Application/FTP]
    $ftp5 attach-agent $tcp5
    $ns_ at 6.0 "$ftp5 produce 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/many_requests3 -superclass TestSuite
Test/many_requests3 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net4
    set test_ many_requests3
    set guide_  "Many Quick-Start requests, generous router."
    set sndr TCP/Newreno
    set rcvr TCPSink
    set qs ON
    Agent/QSAgent set alloc_rate_ 0.95 
    Agent/QSAgent set threshold_ 0.95
    Test/many_requests3 instproc run {} [Test/many_requests info instbody run ]
    $self next pktTraceFile
}

Class Test/stats -superclass TestSuite
Test/stats instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_  net2
    set test_ stats   
    set guide_  "Two TCPs, statistics."
    set sndr TCP/Newreno
    set rcvr TCPSink
    Agent/TCP set print_request_ true
    set qs ON
    $self next pktTraceFile
}
Test/stats instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    if {$quiet == "false"} {puts $guide_}
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 6

    set slink [$ns_ link $node_(r1) $node_(r2)] 
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s3) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"

    $ns_ at $stopTime "printdrops 1 $fmon;"
    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/stats1 -superclass TestSuite
Test/stats1 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net2
    set test_ stats1	
    set guide_  "Quick-Start packet drops, statistics."
    Agent/TCP set print_request_ true
    set sndr TCP/Sack1
    set rcvr TCPSink/Sack1
    set qs ON
    $self next pktTraceFile
}

Test/stats1 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 20

    set slink [$ns_ link $node_(s1) $node_(r1)] 
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 8
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    set tcp2 [$ns_ create-connection $sndr $node_(s1) $rcvr $node_(s4) 1]
    $tcp2 set window_ 1000
    $tcp2 set rate_request_ 20
    $tcp2 set tcp_qs_recovery_ true
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 2.0 "$ftp2 produce 80"

    $self drop_pkts {5 6}

    $ns_ at $stopTime "printdrops 1 $fmon;"
    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

# 20 KBps = 20 pkts per second
Class Test/rate_request -superclass TestSuite
Test/rate_request instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ rate_request	
    set guide_  "Quick-Start, request of 20 Kbps."
    set qs ON
    Agent/TCP set qs_request_mode_ 0
    $self next pktTraceFile
}
Test/rate_request instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 2
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 10000
    $tcp1 set rate_request_ 20
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 
    $ns_ run
}

# The request isn't limited by the TCP window, but
#   the actual sending rate is.
Class Test/rate_request1 -superclass TestSuite
Test/rate_request1 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ rate_request1	
    set guide_  "Quick-Start, request not limited by TCP window."
    set qs ON
    Agent/TCP set qs_request_mode_ 1
    $self next pktTraceFile
}
Test/rate_request1 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 2
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 10
    $tcp1 set rate_request_ 20
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 start"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 
    $ns_ run
}

# Only make requesst if at least qs_thresh_ packets to send.
Class Test/rate_request3 -superclass TestSuite
Test/rate_request3 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ rate_request3	
    set guide_  "Quick-Start, no request because of insufficient data."
    set qs ON
    Agent/TCP set qs_request_mode_ 1
    Agent/TCP set qs_thresh_ 20
    Agent/TCP set qs_rtt_ 1000
    $self next pktTraceFile
}
Test/rate_request3 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 2
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 100
    $tcp1 set rate_request_ 2000
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 produce 10"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 
    $ns_ run
}

# Requesst limited by available data.
Class Test/rate_request4 -superclass TestSuite
Test/rate_request4 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ rate_request4	
    set guide_  "Quick-Start, rate request limited by available data."
    set qs ON
    Agent/TCP set qs_request_mode_ 1
    Agent/TCP set qs_thresh_ 5
    Agent/TCP set qs_rtt_ 1000
    $self next pktTraceFile
}
Test/rate_request4 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 2
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }

    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 100
    $tcp1 set rate_request_ 2000
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 0.0 "$ftp1 produce 10"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 
    $ns_ run
}

Class Test/routers1 -superclass TestSuite
Test/routers1 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ routers1	
    set guide_  "Quick-Start, rate request 100KBps."
    set qs ON
    set sndr TCP/Newreno
    set rcvr TCPSink
    Agent/QSAgent set algorithm_ 3
    Agent/QSAgent set threshold_ 0.9
    Agent/QSAgent set alloc_rate_ 0.9
    $self next pktTraceFile
}
Test/routers1 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 6
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }

    Agent/TCP set window_ 10000
    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set rate_request_ 100
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 1.0 "$ftp1 produce 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/routers2 -superclass TestSuite
Test/routers2 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ routers2	
    set guide_  "Quick-Start, routers approve only 62 KBps"
    # 100 Mbps * 0.005 = 500 Kbps = 62 KBps. 

    set qs ON
    set sndr TCP/Newreno
    set rcvr TCPSink
    Agent/QSAgent set algorithm_ 3
    Agent/QSAgent set threshold_ 0.005
    Agent/QSAgent set alloc_rate_ 0.005
    Test/routers2 instproc run {} [Test/routers1 info instbody run ]
    $self next pktTraceFile
}

Class Test/routers3 -superclass TestSuite
Test/routers3 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ routers3	
    set guide_  "Quick-Start, log-scale for rate encoding"
    # 100 Mbps * 0.005 = 500 Kbps = 62 KBps. 

    set qs ON
    set sndr TCP/Newreno
    set rcvr TCPSink
    Agent/QSAgent set algorithm_ 3
    Agent/QSAgent set threshold_ 0.9
    Agent/QSAgent set alloc_rate_ 0.9
    Agent/QSAgent set rate_function_ 2
    Test/routers3 instproc run {} [Test/routers1 info instbody run ]
    $self next pktTraceFile
}

# This router allocates the full link bandwidth for QS,
# and the sender requests the full bandwidth for QS.
Class Test/routers4 -superclass TestSuite
Test/routers4 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ routers4	
    set guide_  "Quick-Start, request for full link bandwidth."

    set qs ON
    set sndr TCP/Newreno
    set rcvr TCPSink
    Agent/QSAgent set algorithm_ 3
    Agent/QSAgent set threshold_ 1.0
    Agent/QSAgent set alloc_rate_ 1.0
    Agent/QSAgent set rate_function_ 2
    $self next pktTraceFile
}
Test/routers4 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 4
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }
    $ns_ at 0.0 "$ns_ bandwidth $node_(r1) $node_(r2) 0.64Mbps duplex"

    Agent/TCP set window_ 10000
    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set rate_request_ 80
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 1.0 "$ftp1 produce 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

# This router allocates the full link bandwidth for QS,
Class Test/routers5 -superclass TestSuite
Test/routers5 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ routers5	
    set guide_  "Quick-Start, two requests, total for full link bandwidth."

    set qs ON
    set sndr TCP/Newreno
    set rcvr TCPSink
    Agent/QSAgent set algorithm_ 3
    Agent/QSAgent set threshold_ 1.0
    Agent/QSAgent set alloc_rate_ 1.0
    Agent/QSAgent set rate_function_ 2
    $self next pktTraceFile
}
Test/routers5 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 4
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }
    $ns_ at 0.0 "$ns_ bandwidth $node_(r1) $node_(r2) 0.64Mbps duplex"

    Agent/TCP set window_ 10000
    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set rate_request_ 40
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 1.0 "$ftp1 produce 100"

    set tcp2 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 1]
    $tcp2 set rate_request_ 40
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 1.1 "$ftp2 produce 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

# This router allocates the full link bandwidth for QS,
Class Test/routers6 -superclass TestSuite
Test/routers6 instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ routers6	
    set guide_  "Quick-Start, three requests, total for more than link bandwidth."

    set qs ON
    set sndr TCP/Newreno
    set rcvr TCPSink
    Agent/QSAgent set algorithm_ 3
    #Agent/QSAgent set algorithm_ 2
    Agent/QSAgent set threshold_ 1.0
    Agent/QSAgent set alloc_rate_ 1.0
    Agent/QSAgent set rate_function_ 2
    $self next pktTraceFile
}
Test/routers6 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 4
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }
    $ns_ at 0.0 "$ns_ bandwidth $node_(r1) $node_(r2) 0.64Mbps duplex"

    Agent/TCP set window_ 10000
    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set rate_request_ 40
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 1.0 "$ftp1 produce 100"

    set tcp2 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 1]
    $tcp2 set rate_request_ 40
    set ftp2 [new Application/FTP]
    $ftp2 attach-agent $tcp2
    $ns_ at 1.1 "$ftp2 produce 100"

    set tcp3 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 2]
    $tcp3 set rate_request_ 40
    set ftp3 [new Application/FTP]
    $ftp3 attach-agent $tcp3
    $ns_ at 1.2 "$ftp3 produce 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/small-request -superclass TestSuite
Test/small-request instproc init {} {
    $self instvar net_ test_ guide_ sndr rcvr qs
    set net_	net3
    set test_ small-request	
    set guide_  "Quick-Start, a very small request."
    set qs ON
    set sndr TCP/Newreno
    set rcvr TCPSink
    $self next pktTraceFile
}
Test/small-request instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ sndr rcvr qs
    puts "Guide: $guide_"
    $ns_ node-config -QS $qs
    $self setTopo
    set stopTime 6
    if {$quiet == "false"} {
        Agent/TCP set print_request_ true
    }

    Agent/TCP set window_ 10000
    set tcp1 [$ns_ create-connection TCP/Newreno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set rate_request_ 4
    set ftp1 [new Application/FTP]
    $ftp1 attach-agent $tcp1
    $ns_ at 1.0 "$ftp1 produce 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}


# We still need a test that tests state_delay_:
# Agent/QSAgent set state_delay_  0.3

TestSuite runTest

