# test-suite-tmix.tcl
#
# Based on ns/tcl/test/test-suite-packmime.tcl
# Modified by Matt Crinklaw-Vogt and Michele Weigle, 
# Old Dominion Univ, Mar 2008
#

#*************************************************************************
# Tests to validate Tmix
#
#  Lossless-orig - don't enforce losses, use original cvec format
#  Lossless-alt - don't enforce losses, use alternate cvec format
#  Lossy-alt - enforce losses, use alternate cvec format
#  Oneway-lossy-alt - use one-way agent, etc.
#  Oneway-lossless-alt
#
#*************************************************************************

source misc.tcl

# this will generate the same traffic in each direction
set INBOUND_ORIG "../ex/tmix/sample-orig.cvec"
set OUTBOUND_ORIG "../ex/tmix/sample-orig.cvec"
set INBOUND_ALT "../ex/tmix/sample-alt.cvec"
set OUTBOUND_ALT "../ex/tmix/sample-alt.cvec"

set WARMUP 4;   # time to start tracing
set END 5;      # simulation end time

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
# Tmix test topology
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
    set node_(r1) [$ns Tmix_DelayBox]
    set node_(r2) [$ns Tmix_DelayBox]

    set node_(k1) [$ns node]
    set node_(k2) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 100Mb 1ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 100Mb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 100Mb 1ms DropTail
    $ns duplex-link $node_(k1) $node_(r2) 100Mb 1ms DropTail
    $ns duplex-link $node_(k2) $node_(r2) 100Mb 1ms DropTail
}

TestSuite instproc setup_topo {} {
    $self instvar ns_ 

    remove-all-packet-headers
    add-packet-header IP TCP
    $ns_ use-scheduler Heap

    Trace set show_tcphdr_ 1 
    Agent/TCP/FullTcp set segsize_ 1460;
    Agent/TCP/FullTcp set nodelay_ true;
    Agent/TCP/FullTcp set segsperack_ 2;
}

TestSuite instproc start_tracing {} {
    $self instvar ns_ node_

    # clear output files
    exec rm -f temp.rands

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
}

#
# Lossy-alt
#
Class Test/Lossy-alt -superclass TestSuite
Test/Lossy-alt instproc init topo {
    global defaultRNG
    $self instvar net_ defNet_ test_
    $defaultRNG seed 9999
    set net_ $topo
    set defNet_ DB
    set test_ Lossy-alt
    $self next 0
}

Test/Lossy-alt instproc run {} {
    global INBOUND_ALT OUTBOUND_ALT END WARMUP
    $self instvar ns_ node_
    $self setup_topo

    # setup Tmix-DB nodes
    $node_(r1) set-cvfile $INBOUND_ALT [$node_(s1) id] [$node_(k1) id]
    $node_(r2) set-cvfile $OUTBOUND_ALT [$node_(k2) id] [$node_(s2) id]
    
    # setup Tmix nodes
    set tmix(0) [new Tmix]
    $tmix(0) set-init $node_(s1);
    $tmix(0) set-acc $node_(k1);
    $tmix(0) set-ID 7
    $tmix(0) set-cvfile $INBOUND_ALT
    $tmix(0) set-TCP Newreno
    
    set tmix(1) [new Tmix]
    $tmix(1) set-init $node_(k2);
    $tmix(1) set-acc $node_(s2);
    $tmix(1) set-ID 8
    $tmix(1) set-cvfile $OUTBOUND_ALT
    $tmix(1) set-TCP Newreno
    
    $ns_ at 0.0 "$tmix(0) start"
    $ns_ at 0.0 "$tmix(1) start"
    $ns_ at $WARMUP "$self start_tracing"
    $ns_ at $END "$tmix(0) stop"
    $ns_ at $END "$tmix(1) stop"
    $ns_ at [expr $END + 1] "$ns_ halt"
    
    $ns_ run
}

#
# Lossless-orig
#
Class Test/Lossless-orig -superclass TestSuite
Test/Lossless-orig instproc init topo {
    global defaultRNG
    $self instvar net_ defNet_ test_
    $defaultRNG seed 9999
    set net_ $topo
    set defNet_ DB
    set test_ Lossless-orig
    $self next 0
}

Test/Lossless-orig instproc run {} {
    global INBOUND_ORIG OUTBOUND_ORIG END WARMUP
    $self instvar ns_ node_
    $self setup_topo

    # setup Tmix-DB nodes
    $node_(r1) set-cvfile $INBOUND_ORIG [$node_(s1) id] [$node_(k1) id]
    $node_(r2) set-cvfile $OUTBOUND_ORIG [$node_(k2) id] [$node_(s2) id]
    $node_(r1) set-lossless
    $node_(r2) set-lossless

    # setup Tmix nodes
    set tmix(0) [new Tmix]
    $tmix(0) set-init $node_(s1);
    $tmix(0) set-acc $node_(k1);
    $tmix(0) set-ID 7
    $tmix(0) set-cvfile $INBOUND_ORIG
    $tmix(0) set-TCP Newreno

    set tmix(1) [new Tmix]
    $tmix(1) set-init $node_(k2);
    $tmix(1) set-acc $node_(s2);
    $tmix(1) set-ID 8
    $tmix(1) set-cvfile $OUTBOUND_ORIG
    $tmix(1) set-TCP Newreno

    $ns_ at 0.0 "$tmix(0) start"
    $ns_ at 0.0 "$tmix(1) start"
    $ns_ at $WARMUP "$self start_tracing"
    $ns_ at $END "$tmix(0) stop"
    $ns_ at $END "$tmix(1) stop"
    $ns_ at [expr $END + 1] "$ns_ halt"
    
    $ns_ run
}

#
# Lossless-alt
#
Class Test/Lossless-alt -superclass TestSuite
Test/Lossless-alt instproc init topo {
    global defaultRNG
    $self instvar net_ defNet_ test_
    $defaultRNG seed 9999
    set net_ $topo
    set defNet_ DB
    set test_ Lossless-alt
    $self next 0
}

Test/Lossless-alt instproc run {} {
    global INBOUND_ALT OUTBOUND_ALT END WARMUP
    $self instvar ns_ node_
    $self setup_topo

    # setup Tmix-DB nodes
    $node_(r1) set-cvfile $INBOUND_ALT [$node_(s1) id] [$node_(k1) id]
    $node_(r2) set-cvfile $OUTBOUND_ALT [$node_(k2) id] [$node_(s2) id]
    $node_(r1) set-lossless
    $node_(r2) set-lossless

    # setup Tmix nodes
    set tmix(0) [new Tmix]
    $tmix(0) set-init $node_(s1);
    $tmix(0) set-acc $node_(k1);
    $tmix(0) set-ID 7
    $tmix(0) set-cvfile $INBOUND_ALT
    $tmix(0) set-TCP Newreno

    set tmix(1) [new Tmix]
    $tmix(1) set-init $node_(k2);
    $tmix(1) set-acc $node_(s2);
    $tmix(1) set-ID 8
    $tmix(1) set-cvfile $OUTBOUND_ALT
    $tmix(1) set-TCP Newreno

    $ns_ at 0.0 "$tmix(0) start"
    $ns_ at 0.0 "$tmix(1) start"
    $ns_ at $WARMUP "$self start_tracing"
    $ns_ at $END "$tmix(0) stop"
    $ns_ at $END "$tmix(1) stop"
    $ns_ at [expr $END + 1] "$ns_ halt"
    
    $ns_ run
}

#
# Oneway-lossy-alt
#
Class Test/Oneway-lossy-alt -superclass TestSuite
Test/Oneway-lossy-alt instproc init topo {
    global defaultRNG
    $self instvar net_ defNet_ test_
    $defaultRNG seed 9999
    set net_ $topo
    set defNet_ DB
    set test_ Oneway-lossy-alt
    $self next 0
}

Test/Oneway-lossy-alt instproc run {} {
    global INBOUND_ALT OUTBOUND_ALT END WARMUP
    $self instvar ns_ node_
    $self setup_topo

    # setup Tmix-DB nodes
    $node_(r1) set-cvfile $INBOUND_ALT [$node_(s1) id] [$node_(k1) id]
    $node_(r2) set-cvfile $OUTBOUND_ALT [$node_(k2) id] [$node_(s2) id]
    
    # setup Tmix nodes
    set tmix(0) [new Tmix]
    $tmix(0) set-init $node_(s1);
    $tmix(0) set-acc $node_(k1);
    $tmix(0) set-ID 7
    $tmix(0) set-cvfile $INBOUND_ALT
    $tmix(0) set-agent-type one-way
    $tmix(0) set-TCP Newreno
    $tmix(0) set-pkt-size 500
    $tmix(0) set-sink DelAck   
    
    set tmix(1) [new Tmix]
    $tmix(1) set-init $node_(k2);
    $tmix(1) set-acc $node_(s2);
    $tmix(1) set-ID 8
    $tmix(1) set-cvfile $OUTBOUND_ALT
    $tmix(1) set-agent-type one-way
    $tmix(1) set-TCP Newreno
    $tmix(1) set-pkt-size 500
    $tmix(1) set-sink DelAck   
    
    $ns_ at 0.0 "$tmix(0) start"
    $ns_ at 0.0 "$tmix(1) start"
    $ns_ at $WARMUP "$self start_tracing"
    $ns_ at $END "$tmix(0) stop"
    $ns_ at $END "$tmix(1) stop"
    $ns_ at [expr $END + 1] "$ns_ halt"
    
    $ns_ run
}

#
# Lossless-alt
#
Class Test/Oneway-lossless-alt -superclass TestSuite
Test/Oneway-lossless-alt instproc init topo {
    global defaultRNG
    $self instvar net_ defNet_ test_
    $defaultRNG seed 9999
    set net_ $topo
    set defNet_ DB
    set test_ Oneway-lossless-alt
    $self next 0
}

Test/Oneway-lossless-alt instproc run {} {
    global INBOUND_ALT OUTBOUND_ALT END WARMUP
    $self instvar ns_ node_
    $self setup_topo

    # setup Tmix-DB nodes
    $node_(r1) set-cvfile $INBOUND_ALT [$node_(s1) id] [$node_(k1) id]
    $node_(r2) set-cvfile $OUTBOUND_ALT [$node_(k2) id] [$node_(s2) id]
    $node_(r1) set-lossless
    $node_(r2) set-lossless

    # setup Tmix nodes
    set tmix(0) [new Tmix]
    $tmix(0) set-init $node_(s1);
    $tmix(0) set-acc $node_(k1);
    $tmix(0) set-ID 7
    $tmix(0) set-cvfile $INBOUND_ALT
    $tmix(0) set-agent-type one-way
    $tmix(0) set-TCP Newreno
    $tmix(0) set-pkt-size 500
    $tmix(0) set-sink DelAck   

    set tmix(1) [new Tmix]
    $tmix(1) set-init $node_(k2);
    $tmix(1) set-acc $node_(s2);
    $tmix(1) set-ID 8
    $tmix(1) set-cvfile $OUTBOUND_ALT
    $tmix(1) set-agent-type one-way

    $ns_ at 0.0 "$tmix(0) start"
    $ns_ at 0.0 "$tmix(1) start"
    $ns_ at $WARMUP "$self start_tracing"
    $ns_ at $END "$tmix(0) stop"
    $ns_ at $END "$tmix(1) stop"
    $ns_ at [expr $END + 1] "$ns_ halt"
    
    $ns_ run
}

TestSuite runTest
