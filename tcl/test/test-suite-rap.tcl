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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-rap.tcl,v 1.7 2004/10/18 19:42:18 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

source misc_simple.tcl
# FOR UPDATING GLOBAL DEFAULTS:

Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr 90 * 512 + 40]

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
	$self setTopo

	set fid 1
        # Set up connection
    	if {$tcptype == "RAP"} {
      		set conn1 [$ns_ create-connection RAP $node_(s1) \
          	RAP $node_(k1) $fid]
    	} 
        $ns_ at 1.0 "$conn1 start"

        $self drop_pkts $list

        #$self traceQueues $node_(r1) [$self openTrace 8.0 $testName_]
	$ns_ at 8.0 "$self cleanupAll $testName_"
        $ns_ run
}

# Definition of test-suite tests

###################################################
## One drop
###################################################

Class Test/onedrop_rap -superclass TestSuite
Test/onedrop_rap instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	onedrop_rap
	$self next pktTraceFile
}
Test/onedrop_rap instproc run {} {
        $self setup RAP {14}
}

###################################################
## Two drops
###################################################

Class Test/twodrops_rap -superclass TestSuite
Test/twodrops_rap instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	twodrops_rap
	$self next pktTraceFile
}
Test/twodrops_rap instproc run {} {
        $self setup RAP {14 28}
}


###################################################
## Three drops
###################################################

Class Test/threedrops_rap -superclass TestSuite
Test/threedrops_rap instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	threedrops_rap
	$self next pktTraceFile
}
Test/threedrops_rap instproc run {} {
        $self setup RAP {14 26 28}
}

###################################################
## Four drops
###################################################

Class Test/fourdrops_rap -superclass TestSuite
Test/fourdrops_rap instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	fourdrops_rap
	$self next pktTraceFile
}
Test/fourdrops_rap instproc run {} {
        $self setup RAP {14 24 26 28}
}

###################################################
## Different parameters
###################################################

Class Test/diff_decrease_rap -superclass TestSuite
Test/diff_decrease_rap instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	diff_decrease_rap
	Agent/RAP set beta_ 0.875
	$self next pktTraceFile
}
Test/diff_decrease_rap instproc run {} {
        $self setup RAP {14}
}

###################################################
## Different parameters
###################################################

Class Test/diff_increase_rap -superclass TestSuite
Test/diff_increase_rap instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	diff_increase_rap
	Agent/RAP set alpha_ 0.2
	$self next pktTraceFile
}
Test/diff_increase_rap instproc run {} {
        $self setup RAP {14}
}


TestSuite runTest

