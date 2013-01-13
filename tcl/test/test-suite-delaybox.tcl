# test-suite-DelayBox.tcl
#
# Based on ns/tcl/test/test-suite-full.tcl
#
# Modified by Michele Weigle, UNC-Chapel Hill, Dec 2003
#

#*************************************************************************
# Tests to validate DelayBox
#
#  oneway - one-way TCP
#  oneway-asymmetric - one-way TCP and delay only on the data path
#  full - Full-TCP
#  full-asymmetric - Full-TCP and delay only on the data path
#*************************************************************************

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set SetCWRonRetransmit_ false
# The default is being changed to true.


# Agent/TCP/FullTcp set debug_ true;

source misc.tcl

TestSuite instproc finish testname {
	$self instvar ns_
	$ns_ halt
}

Class SkelTopology
Class Topology -superclass SkelTopology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/DB -superclass Topology

#
# DelayBox test topology
#
#           s1                      k1
#             \                    /
#  100Mb, 1ms  \    100Mb, 1ms    / 100Mb, 1ms
#               r1 ----------- r2 
#  100Mb, 1ms  /                 \ 100Mb, 1ms
#             /                   \
#          s2                       k2
#
Topology/DB instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns DelayBox]
    set node_(r2) [$ns DelayBox]
    set node_(k1) [$ns node]
    set node_(k2) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 100Mb 1ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 100Mb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 100Mb 1ms DropTail
    $ns duplex-link $node_(k1) $node_(r2) 100Mb 1ms DropTail
    $ns duplex-link $node_(k2) $node_(r2) 100Mb 1ms DropTail
}

TestSuite instproc setup tcptype {
    $self instvar ns_ node_ testName_

    # clear output files
    exec rm -f temp.rands

    Trace set show_tcphdr_ 1 
    Agent/TCP set minrto_ 1;     # ns-2.27 value

    set stopt 10

    # create TCP Agents
    if {$tcptype == "FullTcp"} {
        set src(0) [new Agent/TCP/FullTcp]
        set sink(0) [new Agent/TCP/FullTcp]
        set src(1) [new Agent/TCP/FullTcp]
        set sink(1) [new Agent/TCP/FullTcp]
    } elseif {$tcptype == "oneway"} {
        set src(0) [new Agent/TCP]
        set sink(0) [new Agent/TCPSink]
        set src(1) [new Agent/TCP]
        set sink(1) [new Agent/TCPSink]
    }

    # setup TCP Agents and connections
    $ns_ attach-agent $node_(s1) $src(0)
    $src(0) set fid_ 0
    $ns_ attach-agent $node_(k1) $sink(0)
    $sink(0) set fid_ 0
    $ns_ attach-agent $node_(s2) $src(1)
    $src(1) set fid_ 1
    $ns_ attach-agent $node_(k2) $sink(1)
    $sink(1) set fid_ 1

    # make the connections
    $ns_ connect $src(0) $sink(0)
    $sink(0) listen
    $ns_ connect $src(1) $sink(1)
    $sink(1) listen
    
    # schedule flows
    $ns_ at 0.7 "$src(0) advance 50"
    $ns_ at 0.8 "$src(1) advance 50"
    $ns_ at 2.0 "$src(0) close"
    $ns_ at 2.0 "$src(1) close"
    $ns_ at $stopt "$self finish $testName_"

   # setup tracing
    set trace_file [open temp.rands w]
    $ns_ trace-queue $node_(s1) $node_(r1) $trace_file
    $ns_ trace-queue $node_(r1) $node_(s1) $trace_file
    $ns_ trace-queue $node_(s2) $node_(r1) $trace_file
    $ns_ trace-queue $node_(r1) $node_(s2) $trace_file
    $ns_ trace-queue $node_(r1) $node_(r2) $trace_file
    $ns_ trace-queue $node_(r2) $node_(r1) $trace_file
    $ns_ trace-queue $node_(k1) $node_(r2) $trace_file
    $ns_ trace-queue $node_(r2) $node_(k1) $trace_file
    $ns_ trace-queue $node_(k2) $node_(r2) $trace_file
    $ns_ trace-queue $node_(r2) $node_(k2) $trace_file

    # create random variables
    set srcd_rng [new RNG];
    set src_delay [new RandomVariable/Uniform];   # delay 20-50 ms
    $src_delay set min_ 20
    $src_delay set max_ 50
    $src_delay use-rng $srcd_rng

    set srcbw_rng [new RNG];
    set src_bw [new RandomVariable/Uniform];      # bw 1-20 Mbps
    $src_bw set min_ 1
    $src_bw set max_ 20
    $src_delay use-rng $srcbw_rng

    set sinkd_rng [new RNG];
    set sink_delay [new RandomVariable/Uniform];   # delay 1-20 ms
    $sink_delay set min_ 1
    $sink_delay set max_ 20
    $sink_delay use-rng $sinkd_rng

    set sinkbw_rng [new RNG];
    set sink_bw [new RandomVariable/Constant];      # bw 100 Mbps
    $sink_bw set val_ 100
    $sink_bw use-rng $sinkbw_rng

    set loss_rng [new RNG];
    set loss_rate [new RandomVariable/Uniform];    # loss 0-1%
    $loss_rate set min_ 0
    $loss_rate set max_ 0.01
    $loss_rate use-rng $loss_rng
    
    # setup rules for DelayBoxes
    $node_(r1) add-rule [$node_(s1) id] [$node_(k1) id] $src_delay \
	    $loss_rate $src_bw
    $node_(r1) add-rule [$node_(s2) id] [$node_(k2) id] $src_delay \
	    $loss_rate $src_bw
    $node_(r2) add-rule [$node_(s1) id] [$node_(k1) id] $sink_delay \
	    $loss_rate $sink_bw
    $node_(r2) add-rule [$node_(s2) id] [$node_(k2) id] $sink_delay \
	    $loss_rate $sink_bw
}


#
# oneway
#
Class Test/oneway -superclass TestSuite
Test/oneway instproc init topo {
        global defaultRNG
	$self instvar net_ defNet_ test_
        $defaultRNG seed 9999
        set net_ $topo
	set defNet_ DB
	set test_ oneway
	$self next 0
}

Test/oneway instproc run {} {
	$self instvar ns_ 
        $self setup oneway
	$ns_ run
}

#
# oneway-asymmetric
#
Class Test/oneway-asymmetric -superclass TestSuite
Test/oneway-asymmetric instproc init topo {
        global defaultRNG
	$self instvar net_ defNet_ test_
        $defaultRNG seed 9999
        set net_ $topo
	set defNet_ DB
	set test_ oneway-asymmetric
	$self next 0
}

Test/oneway-asymmetric instproc run {} {
	$self instvar ns_ node_ 
        $self setup oneway

        $node_(r1) set-asymmetric
        $node_(r2) set-asymmetric

	$ns_ run
}

#
# full
#
Class Test/full -superclass TestSuite
Test/full instproc init topo {
        global defaultRNG
	$self instvar net_ defNet_ test_
        $defaultRNG seed 9999
        set net_ $topo
	set defNet_ DB
	set test_ full
	$self next 0
}

Test/full instproc run {} {
	$self instvar ns_
        $self setup FullTcp
	$ns_ run
}

#
# full-asymmetric
#
Class Test/full-asymmetric -superclass TestSuite
Test/full-asymmetric instproc init topo {
        global defaultRNG
	$self instvar net_ defNet_ test_
        $defaultRNG seed 9999
        set net_ $topo
	set defNet_ DB
	set test_ full-asymmetric
	$self next 0
}

Test/full-asymmetric instproc run {} {
	$self instvar ns_ node_
        $self setup FullTcp

        $node_(r1) set-asymmetric
        $node_(r2) set-asymmetric

	$ns_ run
}

TestSuite runTest
