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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-tcpOptions.tcl,v 1.22 2006/01/25 22:02:05 sallyfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for TCP

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
source support.tcl
Agent/TCP set singledup_ 0
# The default is being changed to 1

Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr $wrap * 512 + 40]

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 10ms bottleneck.
# Queue-limit on bottleneck is 2 packets.
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
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
}

Class Topology/net4a -superclass Topology
Topology/net4a instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8
}


TestSuite instproc finish file {
	global quiet wrap wrap1 PERL
	set space 512
        #exec $PERL ../../bin/set_flow_id -s all.tr | \
        #  $PERL ../../bin/getrc -s 2 -d 3 | \
        #  $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
	if [string match {*full*} $file] {
                exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
                   $PERL ../../bin/raw2xg -c -n $space -s 0.01 -m $wrap1 -t $file > temp.rands
                exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
                   $PERL ../../bin/raw2xg -a -c -f -p -y -n $space -s 0.01 -m $wrap1 -t $file >> temp.rands
	} else {
	        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	          $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
	        exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	          $PERL ../../bin/raw2xg -a -c -p -y -s 0.01 -m $wrap -t $file \
		  >> temp.rands
	}
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


TestSuite instproc emod {} {
        $self instvar topo_
        $topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        return $errmodule
} 

TestSuite instproc drop_pkts pkts {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1
}

TestSuite instproc setup {tcptype list} {
	global wrap wrap1 quiet
        $self instvar ns_ node_ testName_ guide_
	$self setTopo 
	puts "Guide: $guide_"

        Agent/TCP set bugFix_ false
	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Tahoe"} {
      		set tcp1 [$ns_ create-connection TCP $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	} elseif {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "Fack"} {
      		set tcp1 [$ns_ create-connection TCP/Fack $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "SackRH"} {
      		set tcp1 [$ns_ create-connection TCP/SackRH $node_(s1) \
          	TCPSink/Sack1 $node_(k1) $fid]
    	} elseif {$tcptype == "FullTcp"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp]
	        set sink [new Agent/TCP/FullTcp]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpTahoe"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Tahoe]
	        set sink [new Agent/TCP/FullTcp/Tahoe]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpNewreno"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Newreno]
	        set sink [new Agent/TCP/FullTcp/Newreno]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpSack1"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Sack]
	        set sink [new Agent/TCP/FullTcp/Sack]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} else {
      		set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	}
        $tcp1 set window_ 50
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 2.0
        $self drop_pkts $list

        #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
	$ns_ at 2.0 "$self cleanupAll $testName_"
        $ns_ run
}

TestSuite instproc setup1 {tcptype list delay list1 delay1} {
	global wrap wrap1 quiet
        $self instvar ns_ node_ testName_ guide_
	$self setTopo 
	puts "Guide: $guide_"

        Agent/TCP set bugFix_ false
	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Tahoe"} {
      		set tcp1 [$ns_ create-connection TCP $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	} elseif {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "Fack"} {
      		set tcp1 [$ns_ create-connection TCP/Fack $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "SackRH"} {
      		set tcp1 [$ns_ create-connection TCP/SackRH $node_(s1) \
          	TCPSink/Sack1 $node_(k1) $fid]
    	} elseif {$tcptype == "FullTcp"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp]
	        set sink [new Agent/TCP/FullTcp]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpTahoe"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Tahoe]
	        set sink [new Agent/TCP/FullTcp/Tahoe]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpNewreno"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Newreno]
	        set sink [new Agent/TCP/FullTcp/Newreno]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpSack1"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Sack]
	        set sink [new Agent/TCP/FullTcp/Sack]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} else {
      		set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	}
        $tcp1 set window_ 50
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 produce 22"

        $self tcpDump $tcp1 2.0
        $self dropPkts [$ns_ link $node_(k1) $node_(r1)] $fid $list true $delay
        $self dropPkts [$ns_ link $node_(k1) $node_(r1)] $fid $list1 true $delay1

        #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
	$ns_ at 2.0 "$self cleanupAll $testName_"
        $ns_ run
}

# Definition of test-suite tests

###################################################
## One drop, numdupacks
###################################################

Class Test/onedrop_tahoe -superclass TestSuite
Test/onedrop_tahoe instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_tahoe
	set guide_	"Tahoe TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_tahoe instproc run {} {
        $self setup Tahoe {3}
}

Class Test/onedrop_numdup4_tahoe -superclass TestSuite
Test/onedrop_numdup4_tahoe instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_tahoe
	set guide_	"Tahoe TCP, numdupacks set to 4."
	Agent/TCP set numdupacks_ 4
	Test/onedrop_numdup4_tahoe instproc run {} [Test/onedrop_tahoe info instbody run ]
	$self next pktTraceFile
}

Class Test/onedrop_tahoe_full -superclass TestSuite
Test/onedrop_tahoe_full instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_tahoe_full
	set guide_	"Tahoe Full TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_tahoe_full instproc run {} {
        $self setup FullTcpTahoe {4}
}

Class Test/onedrop_numdup4_tahoe_full -superclass TestSuite
Test/onedrop_numdup4_tahoe_full instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_tahoe_full
	set guide_	"Tahoe Full TCP, numdupacks set to 4."
	Agent/TCP/FullTcp set tcprexmtthresh_ 4
	$self next pktTraceFile
}
Test/onedrop_numdup4_tahoe_full instproc run {} {
        $self setup FullTcpTahoe {4}
}

Class Test/onedrop_reno -superclass TestSuite
Test/onedrop_reno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_reno
	set guide_      "Reno TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_reno instproc run {} {
        $self setup Reno {3}
}

Class Test/onedrop_numdup4_reno -superclass TestSuite
Test/onedrop_numdup4_reno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_reno
	set guide_	"Reno TCP, numdupacks set to 4."
	Agent/TCP set numdupacks_ 4
	Test/onedrop_numdup4_reno instproc run {} [Test/onedrop_reno info instbody run ]
	$self next pktTraceFile
}

Class Test/onedrop_reno_full -superclass TestSuite
Test/onedrop_reno_full instproc init {} {

	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_reno_full
	set guide_	"Reno Full TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_reno_full instproc run {} {
        $self setup FullTcp {4}
}

Class Test/onedrop_numdup4_reno_full -superclass TestSuite
Test/onedrop_numdup4_reno_full instproc init {} {

	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_reno_full
	set guide_	"Reno Full TCP, numdupacks set to 4."
	Agent/TCP/FullTcp set tcprexmtthresh_ 4
	$self next pktTraceFile
}
Test/onedrop_numdup4_reno_full instproc run {} {
        $self setup FullTcp {4}
}

Class Test/onedrop_newreno -superclass TestSuite
Test/onedrop_newreno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_newreno
	set guide_      "NewReno TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_newreno instproc run {} {
        $self setup Newreno {3}
}

Class Test/onedrop_numdup4_newreno -superclass TestSuite
Test/onedrop_numdup4_newreno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_newreno
	set guide_      "NewReno TCP, numdupacks set to 4."
	Agent/TCP set numdupacks_ 4
	Test/onedrop_numdup4_newreno instproc run {} [Test/onedrop_newreno info instbody run ]
	$self next pktTraceFile
}

Class Test/onedrop_newreno_full -superclass TestSuite
Test/onedrop_newreno_full instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_newreno_full
	set guide_      "NewReno Full TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_newreno_full instproc run {} {
        $self setup FullTcpNewreno {4}
}

Class Test/onedrop_numdup4_newreno_full -superclass TestSuite
Test/onedrop_numdup4_newreno_full instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_newreno_full
	set guide_      "NewReno Full TCP, numdupacks set to 4."
	Agent/TCP/FullTcp set tcprexmtthresh_ 4
	$self next pktTraceFile
}
Test/onedrop_numdup4_newreno_full instproc run {} {
        $self setup FullTcpNewreno {4}
}

Class Test/onedrop_sack -superclass TestSuite
Test/onedrop_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_sack
	set guide_      "Sack TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_sack instproc run {} {
        $self setup Sack1 {3}
}

Class Test/onedrop_numdup4_sack -superclass TestSuite
Test/onedrop_numdup4_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_sack
	set guide_      "Sack TCP, numdupacks set to 4."
	Agent/TCP set numdupacks_ 4
	Test/onedrop_numdup4_sack instproc run {} [Test/onedrop_sack info instbody run ]
	$self next pktTraceFile
}

# Why does the sender sent two packets for the Fast Restransmit?
Class Test/onedrop_sack_full -superclass TestSuite
Test/onedrop_sack_full instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_sack_full
	set guide_      "Sack Full TCP, numdupacks set to 3."
	$self next pktTraceFile
}
Test/onedrop_sack_full instproc run {} {
       $self setup FullTcpSack1 {4}
}

Class Test/onedrop_numdup4_sack_full -superclass TestSuite
Test/onedrop_numdup4_sack_full instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	onedrop_numdup4_sack_full
	set guide_      "Sack Full TCP, numdupacks set to 4."
	Agent/TCP/FullTcp set tcprexmtthresh_ 4
	$self next pktTraceFile
}
Test/onedrop_numdup4_sack_full instproc run {} {
        $self setup FullTcpSack1 {4}
}

#############################################################
##  Tests for aggressive_maxburst_, for sending packets after
##     invalid ACKs.
#############################################################

Class Test/maxburst_tahoe -superclass TestSuite
Test/maxburst_tahoe instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_tahoe
	set guide_	"Tahoe TCP, maxburst set to 3."
	Agent/TCP set aggressive_maxburst_ 0
	$self next pktTraceFile
}
Test/maxburst_tahoe instproc run {} {
	Agent/TCP set maxburst_ 3
        $self setup1 Tahoe {8 9 10 11 12 } {100} {13} {0.02}
}

Class Test/maxburst_tahoe1 -superclass TestSuite
Test/maxburst_tahoe1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_tahoe1
	set guide_	"Tahoe TCP, maxburst set to 3, aggressive_maxburst_."
	Agent/TCP set aggressive_maxburst_ 1
	Test/maxburst_tahoe1 instproc run {} [Test/maxburst_tahoe info instbody run ]
	$self next pktTraceFile
}

Class Test/maxburst_reno -superclass TestSuite
Test/maxburst_reno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_reno
	set guide_	"Tahoe TCP, maxburst set to 3."
	Agent/TCP set aggressive_maxburst_ 0
	$self next pktTraceFile
}
Test/maxburst_reno instproc run {} {
	Agent/TCP set maxburst_ 3
        $self setup1 Reno {8 9 10 11 12 } {100} {13} {0.02}
}

Class Test/maxburst_reno1 -superclass TestSuite
Test/maxburst_reno1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_reno1
	set guide_	"Reno TCP, maxburst set to 3, aggressive_maxburst_."
	Agent/TCP set aggressive_maxburst_ 1
	Test/maxburst_reno1 instproc run {} [Test/maxburst_reno info instbody run ]
	$self next pktTraceFile
}

Class Test/maxburst_newreno -superclass TestSuite
Test/maxburst_newreno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_newreno
	set guide_	"NewReno TCP, maxburst set to 3."
	Agent/TCP set aggressive_maxburst_ 0
	$self next pktTraceFile
}
Test/maxburst_newreno instproc run {} {
	Agent/TCP set maxburst_ 3
        $self setup1 Newreno {8 9 10 11 12 } {100} {13} {0.02}
}

Class Test/maxburst_newreno1 -superclass TestSuite
Test/maxburst_newreno1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_newreno1
	set guide_	"NewReno TCP, maxburst set to 3, aggressive_maxburst_."
	Agent/TCP set aggressive_maxburst_ 1
	Test/maxburst_newreno1 instproc run {} [Test/maxburst_newreno info instbody run ]
	$self next pktTraceFile
}

Class Test/maxburst_sack -superclass TestSuite
Test/maxburst_sack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_sack
	set guide_	"Sack TCP, maxburst set to 3."
	Agent/TCP set aggressive_maxburst_ 0
	$self next pktTraceFile
}
Test/maxburst_sack instproc run {} {
	Agent/TCP set maxburst_ 3
        $self setup1 Sack1 {8 9 10 11 12 } {100} {13} {0.02}
}

Class Test/maxburst_sack1 -superclass TestSuite
Test/maxburst_sack1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	maxburst_sack1
	set guide_	"Sack TCP, maxburst set to 3, aggressive_maxburst_."
	Agent/TCP set aggressive_maxburst_ 1
	Test/maxburst_sack1 instproc run {} [Test/maxburst_sack info instbody run ]
	$self next pktTraceFile
}

###################################################
## Timeout, from a list of packets dropped.
###################################################

TestSuite instproc setup2 {tcptype list endtime} {
	global wrap wrap1 quiet
        $self instvar ns_ node_ testName_ guide_
	$self setTopo 
	puts "Guide: $guide_"

	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Tahoe"} {
      		set tcp1 [$ns_ create-connection TCP $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	} elseif {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "Fack"} {
      		set tcp1 [$ns_ create-connection TCP/Fack $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} elseif {$tcptype == "SackRH"} {
      		set tcp1 [$ns_ create-connection TCP/SackRH $node_(s1) \
          	TCPSink/Sack1 $node_(k1) $fid]
    	} elseif {$tcptype == "FullTcp"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp]
	        set sink [new Agent/TCP/FullTcp]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpTahoe"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Tahoe]
	        set sink [new Agent/TCP/FullTcp/Tahoe]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpNewreno"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Newreno]
	        set sink [new Agent/TCP/FullTcp/Newreno]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} elseif {$tcptype == "FullTcpSack1"} {
		set wrap $wrap1
	        set tcp1 [new Agent/TCP/FullTcp/Sack]
	        set sink [new Agent/TCP/FullTcp/Sack]
	        $ns_ attach-agent $node_(s1) $tcp1
	        $ns_ attach-agent $node_(k1) $sink
	        $tcp1 set fid_ $fid
	        $sink set fid_ $fid
	        $ns_ connect $tcp1 $sink
	        # set up TCP-level connections
	        $sink listen ; # will figure out who its peer is
    	} else {
      		set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	}
        $tcp1 set window_ 50
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 2.0
        $self drop_pkts $list

        #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
	$ns_ at $endtime "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Timeouts, scenarios that are better without bugfix.
##   This also includes tests for timestamps with ts_resetRTO_,
##   to reset the RTO backoff after a valid RTT measurement.
###################################################

Class Test/timeouts_tahoe -superclass TestSuite
Test/timeouts_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_tahoe
	set guide_      "Tahoe, timeouts, bugfix"
        $self next pktTraceFile
}
Test/timeouts_tahoe instproc run {} {
        $self setup2 Tahoe {7 8 9 10 11 12 13 14 21} 8
}

Class Test/timeouts_tahoe1 -superclass TestSuite
Test/timeouts_tahoe1 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_tahoe1
	set guide_      "Tahoe, timeouts, better without bugfix"
        Agent/TCP set bugFix_ false
	Test/timeouts_tahoe1 instproc run {} [Test/timeouts_tahoe info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_tahoe2 -superclass TestSuite
Test/timeouts_tahoe2 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_tahoe2
	set guide_      "Tahoe, timeouts, bugfix, with timestamps, new version"
	Agent/TCP set timestamps_ true
	Agent/TCP set ts_resetRTO_ true
	Test/timeouts_tahoe2 instproc run {} [Test/timeouts_tahoe info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_tahoe3 -superclass TestSuite
Test/timeouts_tahoe3 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_tahoe3
	set guide_      "Tahoe, timeouts, bugfix, with timestamps, old version"
	Agent/TCP set timestamps_ true
	#Agent/TCP set ts_resetRTO_ true
	Test/timeouts_tahoe3 instproc run {} [Test/timeouts_tahoe info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_reno -superclass TestSuite
Test/timeouts_reno instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_reno
	set guide_      "Reno, timeouts, bugfix"
        $self next pktTraceFile
}
Test/timeouts_reno instproc run {} {
        $self setup2 Reno {7 8 9 10 11 12 13 14 21} 8
}

Class Test/timeouts_reno_noexitFR -superclass TestSuite
Test/timeouts_reno_noexitFR instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_reno_noexitFR
	set guide_      "Reno, timeouts, bugfix, without exitFastRetrans_"
	Agent/TCP set exitFastRetrans_ false
	Test/timeouts_reno_noexitFR instproc run {} [Test/timeouts_reno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_reno1 -superclass TestSuite
Test/timeouts_reno1 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_reno1
	set guide_      "Reno, timeouts, better without bugfix"
        Agent/TCP set bugFix_ false
	Test/timeouts_reno1 instproc run {} [Test/timeouts_reno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_reno2 -superclass TestSuite
Test/timeouts_reno2 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_reno2
	set guide_      "Reno, timeouts, bugfix, with timestamps, old version"
	Agent/TCP set timestamps_ true
	Agent/TCP set ts_resetRTO_ true
	Test/timeouts_reno2 instproc run {} [Test/timeouts_reno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_reno3 -superclass TestSuite
Test/timeouts_reno3 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_reno3
	set guide_      "Reno, timeouts, bugfix, with timestamps, new version"
	Agent/TCP set timestamps_ true
	#Agent/TCP set ts_resetRTO_ true
	Test/timeouts_reno3 instproc run {} [Test/timeouts_reno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_newreno -superclass TestSuite
Test/timeouts_newreno instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_newreno
	set guide_      "NewReno, timeouts, bugfix"
        $self next pktTraceFile
}
Test/timeouts_newreno instproc run {} {
        $self setup2 Newreno {7 8 9 10 11 12 13 14 21} 8
}

Class Test/timeouts_newreno_noexitFR -superclass TestSuite
Test/timeouts_newreno_noexitFR instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_newreno_noexitFR
	set guide_      "NewReno, timeouts, bugfix, without exitFastRetrans_"
	Agent/TCP set exitFastRetrans_ false
	Test/timeouts_newreno_noexitFR instproc run {} [Test/timeouts_newreno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_newreno1 -superclass TestSuite
Test/timeouts_newreno1 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_newreno1
	set guide_      "NewReno, timeouts, better without bugfix"
        Agent/TCP set bugFix_ false
	Test/timeouts_newreno1 instproc run {} [Test/timeouts_newreno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_newreno2 -superclass TestSuite
Test/timeouts_newreno2 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_newreno2
	set guide_      "NewReno, timeouts, bugfix, with timestamps, old version"
	Agent/TCP set timestamps_ true
	Agent/TCP set ts_resetRTO_ true
	Test/timeouts_newreno2 instproc run {} [Test/timeouts_newreno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_newreno3 -superclass TestSuite
Test/timeouts_newreno3 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_newreno3
	set guide_      "NewReno, timeouts, bugfix, with timestamps, new version"
	Agent/TCP set timestamps_ true
	#Agent/TCP set ts_resetRTO_ true
	Test/timeouts_newreno3 instproc run {} [Test/timeouts_newreno info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_sack -superclass TestSuite
Test/timeouts_sack instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_sack
	set guide_      "Sack, timeouts, bugfix"
        $self next pktTraceFile
}
Test/timeouts_sack instproc run {} {
        $self setup2 Sack1 {7 8 9 10 11 12 13 14 21} 8
}

Class Test/timeouts_sack1 -superclass TestSuite
Test/timeouts_sack1 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_sack1
	set guide_      "Sack, timeouts, better without bugfix"
        Agent/TCP set bugFix_ false
	Test/timeouts_sack1 instproc run {} [Test/timeouts_sack info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_sack2 -superclass TestSuite
Test/timeouts_sack2 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_sack2
	set guide_      "Sack, timeouts, bugfix, with timestamps, old version"
	Agent/TCP set timestamps_ true
	Agent/TCP set ts_resetRTO_ true
	Test/timeouts_sack2 instproc run {} [Test/timeouts_sack info instbody run ]
        $self next pktTraceFile
}

Class Test/timeouts_sack3 -superclass TestSuite
Test/timeouts_sack3 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeouts_sack3
	set guide_      "Sack, timeouts, bugfix, with timestamps, new version"
	Agent/TCP set timestamps_ true
	#Agent/TCP set ts_resetRTO_ true
	Test/timeouts_sack3 instproc run {} [Test/timeouts_sack info instbody run ]
        $self next pktTraceFile
}

###################################################
## Timeouts, scenarios that are better with bugfix.
###################################################

Class Test/timeoutsA_tahoe -superclass TestSuite
Test/timeoutsA_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeoutsA_tahoe
	set guide_      "Tahoe, timeoutsA, bugfix"
        #Agent/TCP set bugFix_ false
        $self next pktTraceFile
}
Test/timeoutsA_tahoe instproc run {} {
        $self setup2 Tahoe {20 21 22 23 24 25 26 31 41} 5
}

Class Test/timeoutsA_tahoe1 -superclass TestSuite
Test/timeoutsA_tahoe1 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       timeoutsA_tahoe1
	set guide_      "Tahoe, timeoutsA, worse without bugfix"
        Agent/TCP set bugFix_ false
	Test/timeoutsA_tahoe1 instproc run {} [Test/timeoutsA_tahoe info instbody run ]
        $self next pktTraceFile
}

TestSuite runTest
