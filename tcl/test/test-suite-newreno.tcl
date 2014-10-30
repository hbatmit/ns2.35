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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-newreno.tcl,v 1.31 2006/01/25 22:02:05 sallyfloyd Exp $
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
Agent/TCP/Newreno set newreno_changes1_ 0
# The default is being changed to 1 on 5/5/03, to reflect RFC 2582.
Agent/TCP/Newreno set partial_window_deflation_ 0  
# The default is being changed to 1 on 5/5/03, to reflect RFC 2582.

Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr 90 * 512 + 40]

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

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
    $ns queue-limit $node_(r1) $node_(k1) 80
    $ns queue-limit $node_(k1) $node_(r1) 80

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid] 
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
}

Class Topology/net4b -superclass Topology
Topology/net4b instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 200ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid] 
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
}


TestSuite instproc finish file {
	global quiet wrap PERL
        #exec $PERL ../../bin/set_flow_id -s all.tr | \
        #  $PERL ../../bin/getrc -s 2 -d 3 | \
        #  $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
        exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
          $PERL ../../bin/raw2xg -a -c -p -s 0.01 -m $wrap -t $file >> \
          temp.rands
        #exec echo $stoptime 0 >> temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
	# exec csh gnuplotC1.com temp.rands temp1.rands $file
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

TestSuite instproc rfc2582 {} {
    Agent/TCP/Newreno set newreno_changes1_ 1
    Agent/TCP/Newreno set partial_window_deflation_ 1
}

TestSuite instproc setup {tcptype list {settopo 1} {stoptime 6} {window 28}} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	if {$settopo == 1} {$self setTopo}

	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Tahoe"} {
      		set tcp1 [$ns_ create-connection TCP $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	} elseif {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
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
        $tcp1 set window_ $window
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 1.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0
        $self drop_pkts $list

	$ns_ at $stoptime "$self cleanupAll $testName_"
        $ns_ run
}

# Definition of test-suite tests


###################################################
## Three drops, Reno has a retransmit timeout.
###################################################

# With Limited Transmit, without bugfix.
Class Test/reno -superclass TestSuite
Test/reno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	reno
	Agent/TCP set singledup_ 1
	set guide_ "Reno, with Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/reno instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ false
        $self setup Reno {14 26 28}
}

# Without Limited Transmit, without bugfix.
Class Test/reno_noLT -superclass TestSuite
Test/reno_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	reno_noLT
	Agent/TCP set singledup_ 0
	set guide_ "Reno, without Limited Transmit, without bugfix."
	Test/reno_noLT instproc run {} [Test/reno info instbody run ]
	$self next pktTraceFile
}

# Class Test/reno_bugfix -superclass TestSuite
# Test/reno_bugfix instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	reno_bugfix
# 	$self next pktTraceFile
# }
# Test/reno_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
#         $self setup Reno {14 26 28}
# }

# With Limited Transmit
Class Test/newreno -superclass TestSuite
Test/newreno instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno
	Agent/TCP set singledup_ 1
	set guide_ "NewReno, with Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/newreno instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ false
        $self setup Newreno {14 26 28}
}

# Without Limited Transmit
Class Test/newreno_noLT -superclass TestSuite
Test/newreno_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_noLT
	Agent/TCP set singledup_ 0
	set guide_ "NewReno, without Limited Transmit, without bugfix."
	Test/newreno_noLT instproc run {} [Test/newreno info instbody run ]
	$self next pktTraceFile
}

# Class Test/newreno_bugfix -superclass TestSuite
# Test/newreno_bugfix instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	newreno_bugfix
# 	$self next pktTraceFile
# }
# Test/newreno_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
#         $self setup Newreno {14 26 28}
# }

# Class Test/newreno_A -superclass TestSuite
# Test/newreno_A instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	newreno_A
# 	$self next pktTraceFile
# }
# Test/newreno_A instproc run {} {
# 	Agent/TCP set bugFix_ false
# 	Agent/TCP/Newreno set newreno_changes1_ 1
#         $self setup Newreno {14 26 28}
# }

# Class Test/newreno_bugfix_A -superclass TestSuite
# Test/newreno_bugfix_A instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	newreno_bugfix_A
# 	$self next pktTraceFile
# }
# Test/newreno_bugfix_A instproc run {} {
# 	Agent/TCP set bugFix_ true
# 	Agent/TCP/Newreno set newreno_changes1_ 1
#         $self setup Newreno {14 26 28}
# }

# With Limited Transmit
Class Test/newreno_B -superclass TestSuite
Test/newreno_B instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_B
	Agent/TCP set singledup_ 1
	set guide_ "NewReno, with Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/newreno_B instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set newreno_changes1_ 1
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP/Newreno set exit_recovery_fix_ 1
        $self setup Newreno {14 26 28}
}

# Without Limited Transmit
Class Test/newreno_B_noLT -superclass TestSuite
Test/newreno_B_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_B_noLT
	Agent/TCP set singledup_ 0
	set guide_ "NewReno, without Limited Transmit, without bugfix."
	Test/newreno_B_noLT instproc run {} [Test/newreno_B info instbody run ]
	$self next pktTraceFile
}

###################################################
## Many drops, Reno has a retransmit timeout.
###################################################


Class Test/reno1 -superclass TestSuite
Test/reno1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	reno1
	set guide_ "Reno, #1, without Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/reno1 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ false
        $self setup Reno {14 15 16 17 18 19 20 21 25 }
}

# Class Test/reno1_bugfix -superclass TestSuite
# Test/reno1_bugfix instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	reno1_bugfix
# 	$self next pktTraceFile
# }
# Test/reno1_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
#         $self setup Reno {14 15 16 17 18 19 20 21 25 }
# }

Class Test/newreno1 -superclass TestSuite
Test/newreno1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1
	Agent/TCP set bugFix_ false
	set guide_ "NewReno, #1, with Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/newreno1 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
        $self setup Newreno {14 15 16 17 18 19 20 21 25 }
}

Class Test/newreno1_BF -superclass TestSuite
Test/newreno1_BF instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1_BF
	Agent/TCP set bugFix_ true
	set guide_ "NewReno, #1, with Limited Transmit, with bugfix."
	Test/newreno1_BF instproc run {} [Test/newreno1 info instbody run ]

	$self next pktTraceFile
}

# With Limited Transmit, without bugfix, with newreno_changes1_
# 
# With newreno_changes1_, we don't reset the retransmit timer for 
#  later partial ACKS, so the retransmit timer is more likely to
#  expire, leading to more unnecessarily-retransmitted packets.
#  With this example, we just *happen* to only have an unnecessary 
#  Fast Retransmit with Limited Transmit, and not without it.
#  The subsequent Fast Retransmits happen because we are not
#  using bugfix.
Class Test/newreno1_A -superclass TestSuite
Test/newreno1_A instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1_A
	Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ false
	set guide_ \
	"NewReno, #1A, with Limited Transmit, without bugfix.  Bad behavior."
	$self next pktTraceFile
}
Test/newreno1_A instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP/Newreno set newreno_changes1_ 1
        $self setup Newreno {14 15 16 17 18 19 20 21 25 }
}

# Without Limited Transmit, without bugfix.
Class Test/newreno1_A_noLT -superclass TestSuite
Test/newreno1_A_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1_A_noLT
	Agent/TCP set singledup_ 0
	Agent/TCP set bugFix_ false
	set guide_ "NewReno, #1A, without Limited Transmit, without bugfix."
	Test/newreno1_A_noLT instproc run {} [Test/newreno1_A info instbody run ]
	$self next pktTraceFile
}

# With Limited Transmit, with bugfix.
Class Test/newreno1_A_BF -superclass TestSuite
Test/newreno1_A_BF instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       newreno1_A_BF
        Agent/TCP set singledup_ 1
        Agent/TCP set bugFix_ true
        set guide_ \
        "NewReno, #1A, with Limited Transmit, with bugfix."
	Test/newreno1_A_BF instproc run {} [Test/newreno1_A info instbody run ]
        $self next pktTraceFile
}

# With Limited Transmit, with bugfix, with the Less Careful variant.
Class Test/newreno1_A_BF_LC -superclass TestSuite
Test/newreno1_A_BF_LC instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       newreno1_A_BF_LC
        Agent/TCP set singledup_ 1
        Agent/TCP set bugFix_ true
	Agent/TCP set lessCareful_ true
        set guide_ \
        "NewReno, #1A, with Limited Transmit, with Less Careful bugfix."
	Test/newreno1_A_BF_LC instproc run {} [Test/newreno1_A info instbody run ]
        $self next pktTraceFile
}

Class Test/newreno1_B0 -superclass TestSuite
Test/newreno1_B0 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1_B0
	set guide_ "NewReno, #1B0, with Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/newreno1_B0 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP/Newreno set exit_recovery_fix_ 1
        $self setup Newreno {14 15 16 17 18 19 20 21 25 }
}

# With Limited Transmit, without bugfix.
Class Test/newreno1_B -superclass TestSuite
Test/newreno1_B instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1_B
	Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ false
	set guide_ \
	"NewReno, #1B, with Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/newreno1_B instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP/Newreno set newreno_changes1_ 1
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP/Newreno set exit_recovery_fix_ 1
        $self setup Newreno {14 15 16 17 18 19 20 21 25 }
}

# With Limited Transmit, with bugfix.
Class Test/newreno1_B_BF -superclass TestSuite
Test/newreno1_B_BF instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       newreno1_B_BF
        Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ true
        set guide_ \
        "NewReno, #1B, with Limited Transmit, with bugfix." 
	Test/newreno1_B_BF instproc run {} [Test/newreno1_B info instbody run ]
        $self next pktTraceFile
}

# With Limited Transmit, with bugfix, with the Less Careful variant.
Class Test/newreno1_B_BF_LC -superclass TestSuite
Test/newreno1_B_BF_LC instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       newreno1_B_BF_LC
        Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ true
        set guide_ \
        "NewReno, #1B, with Limited Transmit, with Less Careful bugfix." 
	Test/newreno1_B_BF_LC instproc run {} [Test/newreno1_B info instbody run ]
        $self next pktTraceFile
}

# Without Limited Transmit, without bugfix.
Class Test/newreno1_B_noLT -superclass TestSuite
Test/newreno1_B_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1_B_noLT
	Agent/TCP set singledup_ 0
	set guide_ "NewReno, #1B, without Limited Transmit, without bugfix."
	Test/newreno1_B_noLT instproc run {} [Test/reno info instbody run ]
	$self next pktTraceFile
}

# With Limited Transmit, without bugfix, with newreno_changes1_
# With newreno_changes1_
# With newreno_changes1_, we don't reset the retransmit timer for 
#  later partial ACKS, so the retransmit timer is more likely to
#  expire, leading to more unnecessarily-retransmitted packets.
#  With this example, we have an unnecessary Fast Retransmit both
#  with and without Limited Transmit, but the behavior is worse
#  with Limited Transmit.
Class Test/newreno1A_A -superclass TestSuite
Test/newreno1A_A instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1A_A
	Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ false
	set guide_ \
	"NewReno, #1A-A, with Limited Transmit, without bugfix.  Bad behavior."
	$self next pktTraceFile
}
Test/newreno1A_A instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP/Newreno set newreno_changes1_ 1
        $self setup Newreno {24 25 26 27 28 29 31 33 36 }
}

# With Limited Transmit, with bugfix.
Class Test/newreno1A_A_BF -superclass TestSuite
Test/newreno1A_A_BF instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1A_A_BF
	Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ true
	set guide_ "NewReno, #1A-A, with Limited Transmit, with bugfix."
	Test/newreno1A_A_BF instproc run {} [Test/newreno1A_A info instbody run ]
	$self next pktTraceFile
}

# With Limited Transmit, with Less Careful bugfix.
Class Test/newreno1A_A_BF_LC -superclass TestSuite
Test/newreno1A_A_BF_LC instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1A_A_BF_LC
	Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ true
	set guide_ \
	"NewReno, #1A-A, with Limited Transmit, with Less Careful bugfix."
	Test/newreno1A_A_BF_LC instproc run {} [Test/newreno1A_A info instbody run ]
	$self next pktTraceFile
}

# Without Limited Transmit, without bugfix.
Class Test/newreno1A_A_noLT -superclass TestSuite
Test/newreno1A_A_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno1A_A_noLT
	Agent/TCP set singledup_ 0
	Agent/TCP set bugFix_ false
	set guide_ "NewReno, #1A-A, without Limited Transmit, without bugfix."
	Test/newreno1A_A_noLT instproc run {} [Test/newreno1A_A info instbody run ]
	$self next pktTraceFile
}

###################################################
## Multiple fast retransmits.
###################################################

# With Limited Transmit, without bugfix.
Class Test/reno2 -superclass TestSuite
Test/reno2 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	reno2
	Agent/TCP set singledup_ 1
	set guide_ "NewReno, with Limited Transmit, without bugfix."
	$self next pktTraceFile
}
Test/reno2 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ false
        $self setup Reno {24 25 26 28 31 35 40 45 46 47 48 }
}

# Without Limited Transmit, without bugfix.
Class Test/reno2_noLT -superclass TestSuite
Test/reno2_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	reno2_noLT
	Agent/TCP set singledup_ 0
	set guide_ "NewReno, without Limited Transmit, without bugfix."
	Test/reno2_noLT instproc run {} [Test/reno2 info instbody run ]
	$self next pktTraceFile
}

# With Limited Transmit, with bugfix.
Class Test/reno2_bugfix -superclass TestSuite
Test/reno2_bugfix instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	reno2_bugfix
	Agent/TCP set singledup_ 1
	set guide_ "NewReno, with Limited Transmit, with bugfix."
	$self next pktTraceFile
}
Test/reno2_bugfix instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ true
        $self setup Reno {24 25 26 28 31 35 40 45 46 47 48 }
}

# Without Limited Transmit, with bugfix.
Class Test/reno2_bugfix_noLT -superclass TestSuite
Test/reno2_bugfix_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	reno2_bugfix_noLT
	Agent/TCP set singledup_ 0
	set guide_ "NewReno, without Limited Transmit, with bugfix."
	Test/reno2_bugfix_noLT instproc run {} [Test/reno2_bugfix info instbody run ]
	$self next pktTraceFile
}

# With Limited Transmit, without bugfix.
Class Test/newreno2_A -superclass TestSuite
Test/newreno2_A instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_A
	Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set newreno_changes1_ 1
	set guide_ \
	"NewReno, #2A, with Limited Transmit, without bugfix.  Bad behavior."
	$self next pktTraceFile
}
Test/newreno2_A instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
}

# Without Limited Transmit, without bugfix.
Class Test/newreno2_A_noLT -superclass TestSuite
Test/newreno2_A_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_A_noLT
	Agent/TCP set singledup_ 0
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set newreno_changes1_ 1
	set guide_ "NewReno, #2A, without Limited Transmit, without bugfix."
	Test/newreno2_A_noLT instproc run {} [Test/newreno2_A info instbody run ]
	$self next pktTraceFile
}

Class Test/newreno2_A_bugfix -superclass TestSuite
Test/newreno2_A_bugfix instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_A_bugfix
	Agent/TCP set bugFix_ true
	Agent/TCP/Newreno set newreno_changes1_ 1
	set guide_ "NewReno, #2A, with Limited Transmit, with bugfix."
	Test/newreno2_A_bugfix instproc run {} [Test/newreno2_A info instbody run ]
	$self next pktTraceFile
}

Class Test/newreno2_A_bugfix_LC -superclass TestSuite
Test/newreno2_A_bugfix_LC instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_A_bugfix_LC
	set guide_ "NewReno, #2A, with Limited Transmit, with bugfix."
	Agent/TCP set bugFix_ true
	Agent/TCP set lessCareful_ true
	Agent/TCP/Newreno set newreno_changes1_ 1
	Test/newreno2_A_bugfix_LC instproc run {} [Test/newreno2_A info instbody run ]
	$self next pktTraceFile
}

# With Limited Transmit, without bugfix.
Class Test/newreno2_B -superclass TestSuite
Test/newreno2_B instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_B
	Agent/TCP set singledup_ 1
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set newreno_changes1_ 1
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP/Newreno set exit_recovery_fix_ 1
	set guide_ \
	"NewReno, #2B, with Limited Transmit, without bugfix." 
	$self next pktTraceFile
}
Test/newreno2_B instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
}

# Without Limited Transmit, without bugfix.
Class Test/newreno2_B_noLT -superclass TestSuite
Test/newreno2_B_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_B_noLT
	Agent/TCP set singledup_ 0
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set newreno_changes1_ 1
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP/Newreno set exit_recovery_fix_ 1
	set guide_ \
	"NewReno, #2B, without Limited Transmit, without bugfix."
	Test/newreno2_B_noLT instproc run {} [Test/newreno2_B info instbody run ]
	$self next pktTraceFile
}

Class Test/newreno2_B_bugfix -superclass TestSuite
Test/newreno2_B_bugfix instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_B_bugfix
	Agent/TCP set bugFix_ true
	Agent/TCP/Newreno set newreno_changes1_ 1
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP/Newreno set exit_recovery_fix_ 1
	set guide_ "NewReno, #2B, with Limited Transmit, with bugfix."
	Test/newreno2_B_bugfix instproc run {} [Test/newreno2_B info instbody run ]
	$self next pktTraceFile
}

Class Test/newreno2_B_bugfix_LC -superclass TestSuite
Test/newreno2_B_bugfix_LC instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno2_B_bugfix_LC
	Agent/TCP set bugFix_ true
	Agent/TCP set lessCareful_ true
	Agent/TCP/Newreno set newreno_changes1_ 1
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP/Newreno set exit_recovery_fix_ 1
	set guide_ \
	"NewReno, #2B, with Limited Transmit, with Less Careful bugfix."
	Test/newreno2_B_bugfix_LC instproc run {} [Test/newreno2_B info instbody run ]
	$self next pktTraceFile
}

# Class Test/newreno3 -superclass TestSuite
# Test/newreno3 instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	newreno3
# 	$self next pktTraceFile
# }
# Test/newreno3 instproc run {} {
# 	Agent/TCP set bugFix_ false
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

# Class Test/newreno3_bugfix -superclass TestSuite
# Test/newreno3_bugfix instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	newreno3_bugfix
# 	$self next pktTraceFile
# }
# Test/newreno3_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

# Class Test/newreno4_A -superclass TestSuite
# Test/newreno4_A instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	newreno4_A
# 	$self next pktTraceFile
# }
# Test/newreno4_A instproc run {} {
# 	Agent/TCP set bugFix_ false
# 	Agent/TCP/Newreno set newreno_changes1_ 1
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

# Class Test/newreno4_A_bugfix -superclass TestSuite
# Test/newreno4_A_bugfix instproc init {} {
# 	$self instvar net_ test_ guide_
# 	set net_	net4
# 	set test_	newreno4_A_bugfix
# 	$self next pktTraceFile
# }
# Test/newreno4_A_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
# 	Agent/TCP/Newreno set newreno_changes1_ 1
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

TestSuite instproc set_lossylink1 {} {
        $self instvar lossylink1_ ns_ node_ guide_
        set lossylink1_ [$ns_ link $node_(s1) $node_(r1)]
        set em [new ErrorModule Fid]
        #set errmodel [new ErrorModel/Periodic]
        #$errmodel unit pkt
        $lossylink1_ errormodule $em
}

TestSuite instproc emod1 {} {
        $self instvar lossylink1_
        set errmodule [$lossylink1_ errormodule]
        return $errmodule 
}

TestSuite instproc drop_pkts1 pkts {
        $self instvar ns_
        set emod [$self emod1]
        set errmodel1 [new ErrorModel/List]
        $errmodel1 droplist $pkts
        $emod insert $errmodel1
        $emod bind $errmodel1 1
}

#
# With bugfix, there an a single unnecessary Fast Retransmit, and
# a single unnecessary retransmission of a window of data.
#
Class Test/newreno5 -superclass TestSuite
Test/newreno5 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	newreno5
	Agent/TCP set bugFix_ true
	Agent/TCP set singledup_ 1
	set guide_ \
	"NewReno #5, reordering, with Limited Transmit, with bugfix."
	$self next pktTraceFile
}
Test/newreno5 instproc run {} {
	global quiet
	$self instvar guide_ ns_ node_
	puts "Guide: $guide_"
        ErrorModel set delay_pkt_ true
        ErrorModel set drop_ false
        ErrorModel set delay_ 0.05
	$self setTopo
	$self set_lossylink1
	$self drop_pkts1 { 25 }
        $self setup Newreno {100000} 0

}

#
#
Class Test/sack5 -superclass TestSuite
Test/sack5 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	sack5
	Agent/TCP set bugFix_ true
	Agent/TCP set singledup_ 1
	set guide_ \
	"Sack #5, reordering, with Limited Transmit, with bugfix."
	$self next pktTraceFile
}
Test/sack5 instproc run {} {
	global quiet
	$self instvar guide_ ns_ node_
	puts "Guide: $guide_"
        ErrorModel set delay_pkt_ true
        ErrorModel set drop_ false
        ErrorModel set delay_ 0.05
	$self setTopo
	$self set_lossylink1
	$self drop_pkts1 { 25 }
        $self setup Sack1 {100000} 0
}

Class Test/newreno5_LC -superclass TestSuite
Test/newreno5_LC instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	newreno5_LC
	Agent/TCP set bugFix_ true
	Agent/TCP set lessCareful_ true
	Agent/TCP set singledup_ 1
	set guide_ \
	"NewReno #5, reordering, with Limited Transmit, with Less Care. bugfix."
	Test/newreno5_LC instproc run {} [Test/newreno5 info instbody run ]
	$self next pktTraceFile
}

#
# With bugfix, this looks essentially the same with and without
# Limited Transmit (i.e., singledup_ set to 1).
#
Class Test/newreno5_noLT -superclass TestSuite
Test/newreno5_noLT instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	newreno5_noLT
	Agent/TCP set bugFix_ true
	Agent/TCP set singledup_ 0
	set guide_ \
	"NewReno #5, reordering, without Limited Transmit, with bugfix."
	Test/newreno5_noLT instproc run {} [Test/newreno5 info instbody run ]
	$self next pktTraceFile
}

Class Test/newreno5_noLT_LC -superclass TestSuite
Test/newreno5_noLT_LC instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	newreno5_noLT_LC
	Agent/TCP set bugFix_ true
	Agent/TCP set lessCareful_ true
	Agent/TCP set singledup_ 0
	set guide_ \
	"NewReno #5, reordering, without Lim. Transmit, with Less Care. bugfix."
	Test/newreno5_noLT_LC instproc run {} [Test/newreno5 info instbody run ]
	$self next pktTraceFile
}

#
# With Limited Transmit but without bugfix, each unnecessary Fast
# Retransmit results in three dup acks, and a subsequent unnecessary
# Fast Retransmit.
#
Class Test/newreno5_noBF -superclass TestSuite
Test/newreno5_noBF instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	newreno5_noBF
	Agent/TCP set bugFix_ false
	Agent/TCP set singledup_ 1
	set guide_ \
	"NewReno #5, reordering, with Limited Transmit, without bugfix.  Bad."
	Test/newreno5_noBF instproc run {} [Test/newreno5 info instbody run ]
	$self next pktTraceFile
}

#
# Without Limited Transmit and without bugfix, each unnecessary Fast
# Retransmit results in three dup acks, and a subsequent unnecessary
# Fast Retransmit, but the Limited Transmits are there to keep the
# number of packets in flight from shrinking below a certain number.
# So without Limited Transmit, eventually the congestion window is
# reduced to one or two, and the pattern of unnecessary Fast Retransmits
# is halted.
#
Class Test/newreno5_noLT_noBF -superclass TestSuite
Test/newreno5_noLT_noBF instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	newreno5_noLT_noBF
	Agent/TCP set bugFix_ false
	Agent/TCP set singledup_ 0
	set guide_ \
	"NewReno #5, reordering, without Limited Transmit, without bugfix."
	Test/newreno5_noLT_noBF instproc run {} [Test/newreno5 info instbody run ]
	$self next pktTraceFile
}

#########################
# Now Reno tests:
#########################

#
# With Reno, there an a single unnecessary Fast Retransmit, but
#  NO unnecessary retransmission of a window of data.
#
Class Test/reno5 -superclass TestSuite
Test/reno5 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	reno5
	Agent/TCP set bugFix_ true
	Agent/TCP set singledup_ 1
	set guide_ \
	"Reno #5, reordering, with Limited Transmit, with bugfix."
	$self next pktTraceFile
}
Test/reno5 instproc run {} {
	global quiet
	$self instvar guide_ ns_ node_
	puts "Guide: $guide_"
        ErrorModel set delay_pkt_ true
        ErrorModel set drop_ false
        ErrorModel set delay_ 0.05
	$self setTopo
	$self set_lossylink1
	$self drop_pkts1 { 25 }
        $self setup Reno {100000} 0
}

#
# With Reno:
# With Limited Transmit but without bugfix, with reordering there
#  is only one unnecessary Fast Retransmit. 
#
Class Test/reno5_noBF -superclass TestSuite
Test/reno5_noBF instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	reno5_noBF
	Agent/TCP set bugFix_ false
	Agent/TCP set singledup_ 1
	set guide_ \
	"Reno #5, reordering, with Limited Transmit, without bugfix."  
	Test/reno5_noBF instproc run {} [Test/reno5 info instbody run ]
	$self next pktTraceFile
}

#
# Reno:
# Only one unnecessary retransmit
#
Class Test/reno5_noLT_noBF -superclass TestSuite
Test/reno5_noLT_noBF instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	reno5_noLT_noBF
	Agent/TCP set bugFix_ false
	Agent/TCP set singledup_ 0
	set guide_ \
	"Reno #5, reordering, without Limited Transmit, without bugfix."
	Test/reno5_noLT_noBF instproc run {} [Test/reno5 info instbody run ]
	$self next pktTraceFile
}

#
# With multiple reordering events, Reno reduces the congestion
#   window more times than necessary.
#
Class Test/reno5a -superclass TestSuite
Test/reno5a instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	reno5a
	Agent/TCP set bugFix_ true
	Agent/TCP set singledup_ 1
	set guide_ \
	"Reno #5, reordering, with Limited Transmit, with bugfix."
	$self next pktTraceFile
}
Test/reno5a instproc run {} {
	global quiet
	$self instvar guide_ ns_ node_
	puts "Guide: $guide_"
        ErrorModel set delay_pkt_ true
        ErrorModel set drop_ false
        ErrorModel set delay_ 0.05
	$self setTopo
	$self set_lossylink1
	$self drop_pkts1 { 25 31 35 40 }
        $self setup Reno {100000} 0
}

#
# Reno:
#
Class Test/reno5a_noLT_noBF -superclass TestSuite
Test/reno5a_noLT_noBF instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4a
	set test_	reno5a_noLT_noBF
	Agent/TCP set bugFix_ false
	Agent/TCP set singledup_ 0
	set guide_ \
	"Reno #5a, reordering, without Limited Transmit, without bugfix."
	Test/reno5a_noLT_noBF instproc run {} [Test/reno5a info instbody run ]
	$self next pktTraceFile
}

#
# In this scenario, we do a Fast Retransmit after packet 33 is dropped,
# and that seems good.  Packet 33 was sent after the first
# Fast Retransmit was initiated.
#
Class Test/newreno6 -superclass TestSuite
Test/newreno6 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno6
	Agent/TCP set singledup_ 1
	set guide_ "NewReno, a new packet transmitted after FR is dropped."
	$self next pktTraceFile
}
Test/newreno6 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP set bugFix_ false
        $self setup Newreno {14 26 28 33}
}

##########################################################
# New validation tests on timestamps, from Andrei Gurtov
##########################################################

# BugFix prevents a Fast Retransmit for a lost packet.
Class Test/newreno_rto_loss -superclass TestSuite
Test/newreno_rto_loss instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_rto
	set guide_ "NewReno after timeout, lost packet."
	$self next pktTraceFile
}
Test/newreno_rto_loss instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	$self rfc2582
        $self setup Newreno {15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 37}
}
 
# The ACK heuristic allows a productive Fast Retransmit.
Class Test/newreno_rto_loss_ack -superclass TestSuite
Test/newreno_rto_loss_ack instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_rto_loss_ack
	Agent/TCP set bugFix_ack_ true
	set guide_ "NewReno after timeout, lost packet, ACK heuristic."
	$self next pktTraceFile
	Test/newreno_rto_loss_ack instproc run {} [Test/newreno_rto_loss info instbody run]
}
 
Class Test/newreno_rto_loss_ts -superclass TestSuite
Test/newreno_rto_loss_ts instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_rto_loss_ts
	Agent/TCP set timestamps_ true
	Agent/TCPSink set ts_echo_rfc1323_ true
        set guide_ "NewReno after timeout, lost packet, timestamps."
	$self next pktTraceFile
	Test/newreno_rto_loss_ts instproc run {} [Test/newreno_rto_loss info instbody run]
}

Class Test/newreno_rto_loss_tsh -superclass TestSuite
Test/newreno_rto_loss_tsh instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_rto_loss_tsh
	Agent/TCP set bugFix_ts_ true
	Agent/TCP set timestamps_ true
	Agent/TCPSink set ts_echo_rfc1323_ true
	set guide_ "NewReno after timeout, lost packet, timestamp heuristic."
	$self next pktTraceFile
	Test/newreno_rto_loss_tsh instproc run {} [Test/newreno_rto_loss info instbody run]
}
 
Class Test/newreno_rto_dup -superclass TestSuite
Test/newreno_rto_dup instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       newreno_rto_dup
        set guide_ "NewReno after timeout, false dupacks"
        $self next pktTraceFile
}
Test/newreno_rto_dup instproc run {} {
        global quiet; $self instvar guide_
        puts "Guide: $guide_"
        $self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 51}
}
 
## The ACK heuristic doesn't make any difference in this case,
## as one would want.
Class Test/newreno_rto_dup_ack -superclass TestSuite
Test/newreno_rto_dup_ack instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       newreno_rto_dup_ack
	Agent/TCP set bugFix_ack_ true
        set guide_ "NewReno after timeout, false dupacks, ACK heuristic"
        $self next pktTraceFile
	Test/newreno_rto_dup_ack instproc run {} [Test/newreno_rto_dup info instbody run]
}
 
TestSuite instproc drop_acks acks {
    $self instvar ns_ node_
    set emod2 [new ErrorModule Fid]
    [$ns_ link $node_(k1) $node_(r1)] errormodule $emod2
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $acks
    $emod2 insert $errmodel1
    $emod2 bind $errmodel1 1
}

# ACK heuristic fails due to ACK losses.
Class Test/newreno_rto_loss_ackf -superclass TestSuite
Test/newreno_rto_loss_ackf instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net4
        set test_       newreno_rto_loss_ackf
        Agent/TCP set bugFix_ack_ true
        set guide_ "NewReno after timeout, lost packet and ACKs, ACK heuristic fails."      
        $self next pktTraceFile
}
Test/newreno_rto_loss_ackf instproc run {} {
        global quiet
        $self instvar guide_
        puts "Guide: $guide_"
	$self rfc2582
        $self setTopo
        $self drop_acks {23 24 25 26}
        $self setup Newreno {15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 43} 0
} 

Class Test/newreno_rto_dup_ts -superclass TestSuite
Test/newreno_rto_dup_ts instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_rto_dup_ts
	Agent/TCP set timestamps_ true
	set guide_ "NewReno after timeout, false dupacks, timestamps."
	$self next pktTraceFile
	Test/newreno_rto_dup_ts instproc run {} [Test/newreno_rto_dup info instbody run]
}

## The timestamp heuristic doesn't make any difference in this case,
## as one would want.
Class Test/newreno_rto_dup_tsh -superclass TestSuite
Test/newreno_rto_dup_tsh instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4
	set test_	newreno_rto_dup_tsh
	Agent/TCP set timestamps_ true
	Agent/TCP set bugFix_ts_ true
	set guide_ "NewReno after timeout, false dupacks, timestamp heuristic."
	$self next pktTraceFile
	Test/newreno_rto_dup_tsh instproc run {} [Test/newreno_rto_dup info instbody run]
}

#########################################################################3
# Impatient (newreno_changes1_ to 1) vs. 
# Slow-but-steady (newreno_changes1_ 0)
#########################################################################3


Class Test/impatient1 -superclass TestSuite
Test/impatient1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	impatient1
	Agent/TCP/Newreno set newreno_changes1_ 1
	set guide_ \
	"NewReno, Impatient."
	$self next pktTraceFile
}
Test/impatient1 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP set singledup_ 1
        $self setup Newreno {15 16 17 18 19 20 21 22 23 24 25 26 27 } 1 10 16
}

Class Test/slow1 -superclass TestSuite
Test/slow1 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	slow1
	Agent/TCP/Newreno set newreno_changes1_ 0
	set guide_ \
	"NewReno, Slow-but-Steady.  Impatient is better."
	Test/slow1 instproc run {} [Test/impatient1 info instbody run]
	$self next pktTraceFile
}

Class Test/impatient2 -superclass TestSuite
Test/impatient2 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	impatient2
	Agent/TCP/Newreno set newreno_changes1_ 1
	set guide_ \
	"NewReno, Impatient.  Slow-but-Steady is better."
	$self next pktTraceFile
}
Test/impatient2 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP set singledup_ 1
        $self setup Newreno {15 16 17 18 19 } 1 10 16
}

Class Test/slow2 -superclass TestSuite
Test/slow2 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	slow2
	Agent/TCP/Newreno set newreno_changes1_ 0
	set guide_ \
	"NewReno, Slow-but-Steady."
	Test/slow2 instproc run {} [Test/impatient2 info instbody run]
	$self next pktTraceFile
}

Class Test/impatient3 -superclass TestSuite
Test/impatient3 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	impatient3
	Agent/TCP/Newreno set newreno_changes1_ 1
	set guide_ \
	"NewReno, Impatient.  Slow-but-Steady is better."
	$self next pktTraceFile
}
Test/impatient3 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP set singledup_ 1
        $self setup Newreno {15 16 17 18 19 22 27 } 1 10 16
}

Class Test/slow3 -superclass TestSuite
Test/slow3 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	slow3
	Agent/TCP/Newreno set newreno_changes1_ 0
	set guide_ \
	"NewReno, Slow-but-Steady."
	Test/slow3 instproc run {} [Test/impatient3 info instbody run]
	$self next pktTraceFile
}

Class Test/impatient4 -superclass TestSuite
Test/impatient4 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	impatient4
	Agent/TCP/Newreno set newreno_changes1_ 1
	set guide_ \
	"NewReno, Impatient."
	$self next pktTraceFile
}
Test/impatient4 instproc run {} {
	global quiet; $self instvar guide_
	puts "Guide: $guide_"
	Agent/TCP/Newreno set partial_window_deflation_ 1
	Agent/TCP set singledup_ 1
        $self setup Newreno {30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55} 1 20 32
}

Class Test/slow4 -superclass TestSuite
Test/slow4 instproc init {} {
	$self instvar net_ test_ guide_
	set net_	net4b
	set test_	slow4
	Agent/TCP/Newreno set newreno_changes1_ 0
	set guide_ \
	"NewReno, Slow-but-Steady.  Impatient is better."
	Test/slow4 instproc run {} [Test/impatient4 info instbody run]
	$self next pktTraceFile
}

TestSuite runTest

