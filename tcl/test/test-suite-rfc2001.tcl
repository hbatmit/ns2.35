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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-rfc2001.tcl,v 1.11 2006/01/24 23:00:07 sallyfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

# This validation test does not need to be included in "./validate", but it 
# should be kept for documentation purposes, as it is referred to in
# other documents.  

source misc.tcl
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
source topologies.tcl
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr 90 * 512 + 40]

TestSuite instproc finish file {
	global quiet wrap PERL
        exec $PERL ../../bin/set_flow_id -s all.tr | \
          $PERL ../../bin/getrc -s 2 -d 3 | \
          $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
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

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 10ms bottleneck.
# Queue-limit on bottleneck is 2 packets.
#
Class Topology/net4 -superclass NodeTopology/4nodes
Topology/net4 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid] 
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
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
	global wrap wrap1
        $self instvar ns_ node_ testName_

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
        $tcp1 set window_ 28
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 1.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0
        $self drop_pkts $list

        $self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
        $ns_ run
}

# Definition of test-suite tests


###################################################
## Three drops, Reno has a retransmit timeout.
###################################################

Class Test/reno -superclass TestSuite
Test/reno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	reno
	$self next
}
Test/reno instproc run {} {
	Agent/TCP set bugFix_ false
        $self setup Reno {14 26 28}
}

# Class Test/reno_bugfix -superclass TestSuite
# Test/reno_bugfix instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	reno_bugfix
# 	$self next
# }
# Test/reno_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
#         $self setup Reno {14 26 28}
# }

Class Test/newreno -superclass TestSuite
Test/newreno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	newreno
	$self next
}
Test/newreno instproc run {} {
	Agent/TCP set bugFix_ false
        $self setup Newreno {14 26 28}
}

# Class Test/newreno_bugfix -superclass TestSuite
# Test/newreno_bugfix instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno_bugfix
# 	$self next
# }
# Test/newreno_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
#         $self setup Newreno {14 26 28}
# }

# Class Test/newreno_A -superclass TestSuite
# Test/newreno_A instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno_A
# 	$self next
# }
# Test/newreno_A instproc run {} {
# 	Agent/TCP set bugFix_ false
# 	Agent/TCP/Newreno set newreno_changes1_ 1
#         $self setup Newreno {14 26 28}
# }

# Class Test/newreno_bugfix_A -superclass TestSuite
# Test/newreno_bugfix_A instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno_bugfix_A
# 	$self next
# }
# Test/newreno_bugfix_A instproc run {} {
# 	Agent/TCP set bugFix_ true
# 	Agent/TCP/Newreno set newreno_changes1_ 1
#         $self setup Newreno {14 26 28}
# }

###################################################
## Many drops, Reno has a retransmit timeout.
###################################################


Class Test/reno1 -superclass TestSuite
Test/reno1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	reno1
	$self next
}
Test/reno1 instproc run {} {
	Agent/TCP set bugFix_ false
        $self setup Reno {14 15 16 17 18 19 20 21 25 }
}

# Class Test/reno1_bugfix -superclass TestSuite
# Test/reno1_bugfix instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	reno1_bugfix
# 	$self next
# }
# Test/reno1_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
#         $self setup Reno {14 15 16 17 18 19 20 21 25 }
# }

Class Test/newreno1 -superclass TestSuite
Test/newreno1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	newreno1
	$self next
}
Test/newreno1 instproc run {} {
	Agent/TCP set bugFix_ false
        $self setup Newreno {14 15 16 17 18 19 20 21 25 }
}

# Class Test/newreno1_bugfix -superclass TestSuite
# Test/newreno1_bugfix instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno1_bugfix
# 	$self next
# }
# Test/newreno1_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
#         $self setup Newreno {14 15 16 17 18 19 20 21 25 }
# }

Class Test/newreno1_A -superclass TestSuite
Test/newreno1_A instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	newreno1_A
	$self next
}
Test/newreno1_A instproc run {} {
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set newreno_changes1_ 1
        $self setup Newreno {14 15 16 17 18 19 20 21 25 }
}

# Class Test/newreno1_A_bugfix -superclass TestSuite
# Test/newreno1_A_bugfix instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno1_A_bugfix
# 	$self next
# }
# Test/newreno1_A_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
# 	Agent/TCP/Newreno set newreno_changes1_ 1
#         $self setup Newreno {14 15 16 17 18 19 20 21 25 }
# }

###################################################
## Multiple fast retransmits.
###################################################

Class Test/reno2 -superclass TestSuite
Test/reno2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	reno2
	$self next
}
Test/reno2 instproc run {} {
	Agent/TCP set bugFix_ false
        $self setup Reno {24 25 26 28 31 35 40 45 46 47 48 }
}

Class Test/reno2_bugfix -superclass TestSuite
Test/reno2_bugfix instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	reno2_bugfix
	$self next
}
Test/reno2_bugfix instproc run {} {
	Agent/TCP set bugFix_ true
#        $self setup Reno {24 25 26 28 31 35 37 40 43 47 48 }
        $self setup Reno {24 25 26 28 31 35 40 45 46 47 48 }
}

Class Test/newreno2_A -superclass TestSuite
Test/newreno2_A instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	newreno2_A
	$self next
}
Test/newreno2_A instproc run {} {
	Agent/TCP set bugFix_ false
	Agent/TCP/Newreno set newreno_changes1_ 1
	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
}

Class Test/newreno2_A_bugfix -superclass TestSuite
Test/newreno2_A_bugfix instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	newreno2_A_bugfix
	$self next
}
Test/newreno2_A_bugfix instproc run {} {
	Agent/TCP set bugFix_ true
	Agent/TCP/Newreno set newreno_changes1_ 1
	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
}

# Class Test/newreno3 -superclass TestSuite
# Test/newreno3 instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno3
# 	$self next
# }
# Test/newreno3 instproc run {} {
# 	Agent/TCP set bugFix_ false
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

# Class Test/newreno3_bugfix -superclass TestSuite
# Test/newreno3_bugfix instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno3_bugfix
# 	$self next
# }
# Test/newreno3_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

# Class Test/newreno4_A -superclass TestSuite
# Test/newreno4_A instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno4_A
# 	$self next
# }
# Test/newreno4_A instproc run {} {
# 	Agent/TCP set bugFix_ false
# 	Agent/TCP/Newreno set newreno_changes1_ 1
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

# Class Test/newreno4_A_bugfix -superclass TestSuite
# Test/newreno4_A_bugfix instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net4
# 	set test_	newreno4_A_bugfix
# 	$self next
# }
# Test/newreno4_A_bugfix instproc run {} {
# 	Agent/TCP set bugFix_ true
# 	Agent/TCP/Newreno set newreno_changes1_ 1
# 	$self setup Newreno {24 25 26 28 31 35 40 45 46 47 48 }
# }

TestSuite runTest

