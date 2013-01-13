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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/topologies.tcl,v 1.14 1998/05/09 00:35:53 sfloyd Exp $
#
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., Simulator Tests. July 1995.  
# URL ftp://ftp.ee.lbl.gov/papers/simtests.ps.Z.
#
# To run individual tests:
# ns test-suite.tcl tahoe1
# ns test-suite.tcl tahoe2
# ...
#

Class SkelTopology

SkelTopology instproc init {} {
    $self next
}

SkelTopology instproc node? n {
    $self instvar node_
    if [info exists node_($n)] {
	set ret $node_($n)
    } else {
	set ret ""
    }
    set ret
}

SkelTopology instproc add-fallback-links {ns nodelist bw delay qtype args} {
   $self instvar node_
    set n1 [lindex $nodelist 0]
    foreach n2 [lrange $nodelist 1 end] {
	if ![info exists node_($n2)] {
	    set node_($n2) [$ns node]
	}
	$ns duplex-link $node_($n1) $node_($n2) $bw $delay $qtype
	foreach opt $args {
	    set cmd [lindex $opt 0]
	    set val [lindex $opt 1]
	    if {[llength $opt] > 2} {
		set x1 [lindex $opt 2]
		set x2 [lindex $opt 3]
	    } else {
		set x1 $n1
		set x2 $n2
	    }
	    $ns $cmd $node_($x1) $node_($x2) $val
	    $ns $cmd $node_($x2) $node_($x1) $val
	}
	set n1 $n2
    }
}

SkelTopology instproc checkConfig {lclass ns} {
    if {[$lclass info instprocs config] != "" &&		\
	    [$self info class] == $lclass} {
	$self config $ns
    }
}

#

Class NodeTopology/4nodes -superclass SkelTopology

# Create a simple four node topology:
#
#	   s1
#	     \ 
#     8Mb,5ms \   0.8Mb,100ms
#	        r1 --------- k1
#     8Mb,5ms /
#	     /
#	   s2

NodeTopology/4nodes instproc init ns {
    $self next

    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 100ms bottleneck.
# Queue-limit on bottleneck is 6 packets.
#
Class Topology/net0 -superclass NodeTopology/4nodes
Topology/net0 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail 
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail 
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 6
    $ns queue-limit $node_(k1) $node_(r1) 6

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(k1) orient right
    $ns duplex-link-op $node_(r1) $node_(k1) queuePos 0
    $ns duplex-link-op $node_(k1) $node_(r1) queuePos 0

    $self checkConfig $class $ns
}

Class Topology/net0-lossy -superclass NodeTopology/4nodes
Topology/net0-lossy instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail 
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail 
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 6
    $ns queue-limit $node_(k1) $node_(r1) 6

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(k1) orient right
    $ns duplex-link-op $node_(r1) $node_(k1) queuePos 0
    $ns duplex-link-op $node_(k1) $node_(r1) queuePos 0

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $errmodel set offset_ 1.0
    $errmodel set period_ 25.0
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0

    $self checkConfig $class $ns
}

Class Topology/net0- -superclass Topology/net0
Topology/net0- instproc init ns {
    $self next $ns
    $self instvar node_ rtm_
    set rtm_(r1:k1)  [$ns rtmodel Exponential {} $node_(r1) $node_(k1)]
    set dynamicsTrace [open dyn.tr "w"]
    [$ns link $node_(r1) $node_(k1)] trace-dynamics $ns $dynamicsTrace
    [$ns link $node_(k1) $node_(r1)] trace-dynamics $ns $dynamicsTrace
    $self checkConfig $class $ns
}

Class Topology/net0-Session -superclass Topology/net0
Topology/net0-Session instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 800Kb 100ms DropTail
    $self instvar node_ rtm_
    set rtm_(r1:k1) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(k1)]
    $ns rtproto Session
    $self checkConfig $class $ns
}

Class Topology/net0-DV -superclass Topology/net0
Topology/net0-DV instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 800Kb 100ms DropTail
    $self instvar node_ rtm_
    set rtm_(r1:k1) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(k1)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net0-DVm0 -superclass Topology/net0
Topology/net0-DVm0 instproc init ns {
    Node set multiPath_ 1
    Agent/rtProto/Direct set preference_ 200	;# Cf. tcl/ex/simple-eqp1.tcl

    $self next $ns			;# now instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 800Kb 100ms DropTail {cost 2 r1 k1}
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net0-DVm1 -superclass Topology/net0
Topology/net0-DVm1 instproc init ns {
    Node set multiPath_ 1
    Agent/rtProto/Direct set preference_ 200	;# Cf. tcl/ex/simple-eqp1.tcl

    $self next $ns			;# now instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 800Kb 100ms DropTail {cost 2 r1 k1}
    $self instvar node_ rtm_
    set rtm_(r1:k1) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(k1)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

#
#
# Links1 uses 10Mb, 5ms feeders, and a 1.5Mb 100ms bottleneck.
# Queue-limit on bottleneck is 23 packets.
#

Class Topology/net1 -superclass NodeTopology/4nodes
Topology/net1 instproc init ns {
    $self next $ns
    $self instvar node_

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 5ms DropTail 
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 5ms DropTail 
    $ns duplex-link $node_(r1) $node_(k1) 1.5Mb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 23
    $ns queue-limit $node_(k1) $node_(r1) 23

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(k1) orient right
    $ns duplex-link-op $node_(r1) $node_(k1) queuePos 0
    $ns duplex-link-op $node_(k1) $node_(r1) queuePos 0

    $self checkConfig $class $ns
}

Class Topology/net1- -superclass Topology/net1
Topology/net1- instproc init ns {
    $self next $ns
    $self instvar node_ rtm_
    set rtm_(r1:k1) [$ns rtmodel Exponential {} $node_(r1) $node_(k1)]
    set dynamicsTrace [open dyn.tr "w"]
    [$ns link $node_(r1) $node_(k1)] trace-dynamics $ns $dynamicsTrace
    [$ns link $node_(k1) $node_(r1)] trace-dynamics $ns $dynamicsTrace
    $self checkConfig $class $ns
}

Class Topology/net1-Session -superclass Topology/net1
Topology/net1-Session instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 1.5Mb 100ms DropTail
    $self instvar node_ rtm_
    set rtm_(r1:k1) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(k1)]
    $ns rtproto Session
    $self checkConfig $class $ns
}

Class Topology/net1-DV -superclass Topology/net1
Topology/net1-DV instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 1.5Mb 100ms DropTail
    $self instvar node_ rtm_
    set rtm_(r1:k1) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(k1)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net1-DVm0 -superclass Topology/net1
Topology/net1-DVm0 instproc init ns {
    Node set multiPath_ 1
    Agent/rtProto/Direct set preference_ 200	;# Cf. tcl/ex/simple-eqp1.tcl

    $self next $ns			;# now instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 1.5Mb 100ms DropTail {cost 2 r1 k1}
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net1-DVm1 -superclass Topology/net1
Topology/net1-DVm1 instproc init ns {
    Node set multiPath_ 1
    Agent/rtProto/Direct set preference_ 200	;# Cf. tcl/ex/simple-eqp1.tcl

    $self next $ns			;# now instantiate entire topology
    $self add-fallback-links $ns {r1 b1 k1} 1.5Mb 100ms DropTail {cost 2 r1 k1}
    $self instvar node_ rtm_
    set rtm_(r1:k1) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(k1)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

#

Class NodeTopology/6nodes -superclass SkelTopology

#
# Create a simple six node topology:
#
#        s1                 s3
#         \                 /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 --------- r2
# 10Mb,3ms /               \ 10Mb,5ms
#         /                 \
#        s2                 s4 
#

NodeTopology/6nodes instproc init ns {
    $self next

    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]
}

Class Topology/cbq1 -superclass NodeTopology/6nodes
Topology/cbq1 instproc init ns {
	$self next $ns

	$self instvar node_
	$ns duplex-link $node_(s1) $node_(r1) 10Mb 5ms DropTail
	$ns duplex-link $node_(s2) $node_(r1) 10Mb 5ms DropTail
	$ns duplex-link $node_(s3) $node_(r1) 10Mb 5ms DropTail
	$ns duplex-link $node_(s4) $node_(r1) 10Mb 5ms DropTail
	$ns simplex-link $node_(r2) $node_(r1) 1.5Mb 5ms DropTail
	$ns queue-limit $node_(r2) $node_(r1) 20
}

Class Topology/cbq1-prr -superclass Topology/cbq1
Topology/cbq1-prr instproc init ns {
	$self next $ns

	$self instvar node_ cbqlink_
	$ns simplex-link $node_(r1) $node_(r2) 1.5Mb 5ms CBQ
	set cbqlink_ [$ns link $node_(r1) $node_(r2)]
}

Class Topology/cbq1-wrr -superclass Topology/cbq1
Topology/cbq1-wrr instproc init ns {
	$self next $ns

	$self instvar node_ cbqlink_
	$ns simplex-link $node_(r1) $node_(r2) 1.5Mb 5ms CBQ/WRR
	set cbqlink_ [$ns link $node_(r1) $node_(r2)]
}

Class Topology/net2 -superclass NodeTopology/6nodes
Topology/net2 instproc init ns {
    $self next $ns

    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail 
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail 
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED 
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail 
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail 

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up

    $self checkConfig $class $ns
}

Class Topology/net2-lossy -superclass Topology/net2
Topology/net2-lossy instproc init ns {
    $self next $ns
    $self instvar node_

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(r2)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass
}

Class Topology/net2- -superclass Topology/net2
Topology/net2- instproc init ns {
    $self next $ns
    $self instvar node_ rtm_
    set rtm_(r1:r2) [$ns rtmodel Exponential {} $node_(r1) $node_(r2)]
    set dynamicsTrace [open dyn.tr "w"]
    [$ns link $node_(r1) $node_(r2)] trace-dynamics $ns $dynamicsTrace
    [$ns link $node_(r2) $node_(r1)] trace-dynamics $ns $dynamicsTrace
    $self checkConfig $class $ns
}

Class Topology/net2-Session -superclass Topology/net2
Topology/net2-Session instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms DropTail \
	    {queue-limit 25}
    $self instvar node_ rtm_
    set rtm_(r1:r2) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(r2)]
    $ns rtproto Session
    $self checkConfig $class $ns
}

Class Topology/net2-DV -superclass Topology/net2
Topology/net2-DV instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms DropTail \
	    {queue-limit 25}
    $self instvar node_ rtm_
    set rtm_(r1:r2) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(r2)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net2-DVm0 -superclass Topology/net2
Topology/net2-DVm0 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms DropTail \
	    {queue-limit 25} {cost 2 r1 r2}
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net2-DVm1 -superclass Topology/net2
Topology/net2-DVm1 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms DropTail \
	    {queue-limit 25} {cost 2 r1 r2}
    $self instvar node_ rtm_
    set rtm_(r1:r2) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(r2)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net2RED-Session -superclass Topology/net2
Topology/net2RED-Session instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms RED \
	    {queue-limit 25}
    $self instvar node_ rtm_
    set rtm_(r1:r2) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(r2)]
    $ns rtproto Session
    $self checkConfig $class $ns
}

Class Topology/net2RED-DV -superclass Topology/net2
Topology/net2RED-DV instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms RED \
	    {queue-limit 25}
    $self instvar node_ rtm_
    set rtm_(r1:r2) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(r2)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net2RED-DVm0 -superclass Topology/net2
Topology/net2RED-DVm0 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms RED \
	    {queue-limit 25} {cost 2 r1 r2}
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net2RED-DVm1 -superclass Topology/net2
Topology/net2RED-DVm1 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    $self add-fallback-links $ns {r1 b1 r2} 1.5Mb 10ms RED \
	    {queue-limit 25} {cost 2 r1 r2}
    $self instvar node_ rtm_
    set rtm_(r1:r2) [$ns rtmodel Trace "dyn.tr" $node_(r1) $node_(r2)]
    $ns rtproto DV
    $self checkConfig $class $ns
}

#

Class NodeTopology/8nodes -superclass SkelTopology

#
# Create a simple eight node topology:
#
#        s1                 s3
#         \      4  x       /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 -.--.--.- r4
# 10Mb,3ms /               \ 10Mb,5ms
#         /                 \
#        s2                 s4 
#
# This topology is of interest when the 4 node spine is dynamic,
# and 3 alternate nodes provide fallback paths.
#

NodeTopology/8nodes instproc init ns {
    $self next
    
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]
    
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(r3) [$ns node]
    set node_(r4) [$ns node]
}

Class Topology/net3 -superclass NodeTopology/8nodes
Topology/net3 instproc init ns {
    $self next $ns
    
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail 
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail 
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED 
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(r2) $node_(r3) 1.5Mb 20ms DropTail 
    $ns duplex-link $node_(r3) $node_(r4) 1.5Mb 20ms DropTail 
    $ns duplex-link $node_(s3) $node_(r4) 10Mb 4ms DropTail 
    $ns duplex-link $node_(s4) $node_(r4) 10Mb 5ms DropTail 

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r3) orient right
    $ns duplex-link-op $node_(r3) $node_(r4) orient right
    $ns duplex-link-op $node_(s3) $node_(r4) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r4) orient left-up

    $self checkConfig $class $ns
}

Class Topology/net3- -superclass Topology/net3
Topology/net3- instproc init ns {
    $self next $ns
    $self instvar node_ rtm_
    set dynamicsTrace [open dyn.tr "w"]
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set rtm_($x1:$x2) [$ns rtmodel Exponential {5.0 1.0} $node_($x1) $node_($x2)]
	[$ns link $node_($x1) $node_($x2)] trace-dynamics $ns $dynamicsTrace
	[$ns link $node_($x2) $node_($x1)] trace-dynamics $ns $dynamicsTrace
    }
    $self checkConfig $class $ns
}

Class Topology/net3-Session -superclass Topology/net3
Topology/net3-Session instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self instvar node_ rtm_
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms DropTail \
		{queue-limit 25}
	set rtm_($x1:$x2) [$ns rtmodel Trace "dyn.tr" $node_($x1) $node_($x2)]
    }
    $ns rtproto Session
    $self checkConfig $class $ns
}

Class Topology/net3-DV -superclass Topology/net3
Topology/net3-DV instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self instvar node_ rtm_
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms DropTail \
		{queue-limit 25}
	set rtm_($x1:$x2) [$ns rtmodel Trace "dyn.tr" $node_($x1) $node_($x2)]
    }
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net3-DVm0 -superclass Topology/net3
Topology/net3-DVm0 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms DropTail \
		{queue-limit 25} [list cost 2 $x1 $x2]
    }
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net3-DVm1 -superclass Topology/net3
Topology/net3-DVm1 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    $self instvar node_ rtm_
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms DropTail \
		{queue-limit 25} [list cost 2 $x1 $x2]
	set rtm_($x1:$x2) [$ns rtmodel Trace "dyn.tr" $node_($x1) $node_($x2)]
    }
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net3RED-Session -superclass Topology/net3
Topology/net3RED-Session instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self instvar node_ rtm_
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms RED \
		{queue-limit 25}
	set rtm_($x1:$x2) [$ns rtmodel Trace "dyn.tr" $node_($x1) $node_($x2)]
    }
    $ns rtproto Session
    $self checkConfig $class $ns
}

Class Topology/net3RED-DV -superclass Topology/net3
Topology/net3RED-DV instproc init ns {
    $self next $ns			;# instantiate entire topology
    $self instvar node_ rtm_
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms RED \
		{queue-limit 25}
	set rtm_($x1:$x2) [$ns rtmodel Trace "dyn.tr" $node_($x1) $node_($x2)]
    }
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net3RED-DVm0 -superclass Topology/net3
Topology/net3RED-DVm0 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms RED \
		{queue-limit 25} [list cost 2 $x1 $x2]
    }
    $ns rtproto DV
    $self checkConfig $class $ns
}

Class Topology/net3RED-DVm1 -superclass Topology/net3
Topology/net3RED-DVm1 instproc init ns {
    Node set multiPath_ 1
    $self next $ns			;# instantiate entire topology
    $self instvar node_ rtm_
    foreach i [list {r1 b1 r2} {r2 b2 r3} {r3 b3 r4}] {
	set x1 [lindex $i 0]
	set x2 [lindex $i 2]
	set b1 [lindex $i 1]
	$self add-fallback-links $ns [list $x1 $b1 $x2] 1.5Mb 10ms RED \
		{queue-limit 25} [list cost 2 $x1 $x2]
	set rtm_($x1:$x2) [$ns rtmodel Trace "dyn.tr" $node_($x1) $node_($x2)]
    }
    $ns rtproto DV
    $self checkConfig $class $ns
}

