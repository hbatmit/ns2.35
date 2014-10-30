#
# Copyright (c) Xerox Corporation 1998. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# 
# Linking this file statically or dynamically with other modules is making
# a combined work based on this file.  Thus, the terms and conditions of
# the GNU General Public License cover the whole combination.
# 
# In addition, as a special exception, the copyright holders of this file
# give you permission to combine this file with free software programs or
# libraries that are released under the GNU LGPL and with code included in
# the standard release of ns-2 under the Apache 2.0 license or under
# otherwise-compatible licenses with advertising requirements (or modified
# versions of such code, with unchanged license).  You may copy and
# distribute such a system following the terms of the GNU GPL for this
# file and the licenses of the other code concerned, provided that you
# include the source code of that other code when and as the GNU GPL
# requires distribution of source code.
# 
# Note that people who make modified versions of this file are not
# obligated to grant this special exception for their modified versions;
# it is their choice whether to do so.  The GNU General Public License
# gives permission to release a modified version without this exception;
# this exception also makes it possible to release a modified version
# which carries forward this exception.

#
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-intserv.tcl,v 1.10 2005/08/26 05:05:30 tomh Exp $
#

# 
# This test suite runs validation tests for the measurement based
# admission control algorithms that are part of the simple
# integrated services support in ns.  The integrated services support
# includes a simple signalling protocol and a scheduler that provides
# a service like controlled-load by giving priority to real-time over
# best-effort traffic.
#
# Five admission control algorithms are tested:
# 1. Measured Sum (MS)
# 2. Hoeffding Bounds (HB)
# 3. Acceptance Region Tangent at Peak (ACTP)
# 4. Acceptance Region Tangent at Origin (ACTO)
# 5. Parameter Based (Param)
#
# The first four are measurement-based, the last is parameter-based.
#
# The measurement-based algorithms each use an estimation algorithm.
# Time window for Measured Sum, exponential averaging for Hoeffding
# Bounds and Point Sample for the Acceptance region algorithms.

# See the paper "Comparison of Measurement-based Admission Control Algorithms
# for Controlled-Load Service", Sugih Jamin, Scott Shenker and Peter
# Danzig, in the Proceedings of IEEE Infocom '97, and references therein
# for a further description of these algorithms.

# To run a test:  
# ns test-suite-intserv.tcl ALG
# where ALG is one of MS, HB, ACTP, ACTO, Param

#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

set meastime 100
set simtime 300

Class TestSuite

TestSuite proc setinstance {T} {
	TestSuite set T $T
}

TestSuite proc getinstance {} {
	TestSuite set T
}

TestSuite instproc init {} {
	$self instvar ns_ net_ test_ node_ testName_ adc_ est_
	$self instvar param_ r_ hvar_ fvar_ srcno_ qmon_ f_ flow_ totflows_

	$self defaults

	set ns_ [new Simulator]
	# next line no longer needed with 32-bit addressing, Nov '99.
	# $ns_ expand-port-field-bits 16

	TestSuite setinstance $self

	set net_ 2Node

	set topo_ [new Topology/$net_ $ns_]

	foreach i [$topo_ array names node_] {
		# This would be cool, but lets try to be compatible
		# with test-suite.tcl as far as possible.
		#
		# $self instvar $i
		# set $i [$topo_ node? $i]
		#
		set node_($i) [$topo_ node? $i]
	}

	# create the link 
	$ns_ duplex-intserv-link $node_(0) $node_(1) $param_(bw) $param_(dly) SimpleIntServ SALink $adc_ $est_ CL

	# set up queue monitor
	set f_ [open temp.rands w]
	set qmon_ [$ns_ monitor-queue $node_(0) $node_(1) $f_]
	set l01 [$ns_ link $node_(0) $node_(1)]
	$l01 set qBytesEstimate_ 0
	$l01 set qPktsEstimate_ 0
	$l01 set sampleInterval_ 0

	#$ns_ at $param_(meastime) "$l01 trace-util [expr $param_(ptime)*$param_(S)] $f_"

	# create the receiver
	set r_ [new Agent/SAack]
	$ns_ attach-agent $node_(1) $r_

	set hvar_ [new RandomVariable/Exponential]
	$hvar_ set avg_ $param_(hold)

	set fvar_ [new RandomVariable/Exponential]
	$fvar_ set avg_ $param_(fint)

	set srcno_ 1

	# schedule the first flow
	set starttime  [expr $srcno_ + double([ns-random] % 10000000) / 1e7]
	$ns_ at $starttime "$self sched-source $node_(0)"

	$ns_ at $param_(meastime) "$qmon_ set bdepartures_ 0;$qmon_ set bdrops_ 0"

	$ns_ at $param_(simtime) "$self finish $f_"

	$self set flows_ 0
	$self set totflows_ 0
}

TestSuite instproc flowadmit {} {
	$self instvar flows_ totflows_
	incr flows_
	incr totflows_
}

TestSuite instproc flowdepart {} {
	$self instvar flows_
	incr flows_ -1
}

TestSuite instproc defaults {} {
	global simtime meastime
	$self instvar param_
	set param_(psize) 125
	set param_(bw) 10e6
	set param_(ptime) [expr $param_(psize)*8.0/$param_(bw)]
	set param_(dly) 1ms
	set param_(S) 5e3
	set param_(hold) 300
	set param_(fint) .4
	set param_(simtime) $simtime
	set param_(meastime) $meastime
	set param_(trace_flow) 0
	Queue/SimpleIntServ set qlimit1_ 160
	Queue/SimpleIntServ set qlimit0_ 0
	ADC set backoff_ true
	Est set period_ [expr $param_(S) * $param_(ptime)]
}

TestSuite instproc run {} {
	$self instvar ns_
	$ns_ run
}

TestSuite instproc create-source {node starttime i} {
	$self instvar ns_ r_ hvar_
        set a [new Agent/SA]
        $ns_ attach-agent $node $a

        $a set fid_ $i
 
        $ns_ connect $a $r_
 
 
        set exp1 [new Application/Traffic/Exponential]
        $exp1 set packetSize_ 125
        $exp1 set burst_time_ [expr 20.0/64]
        $exp1 set idle_time_ 325ms
        $exp1 set rate_ 64k
 
        #set up (r,b)
        $a set rate_ 64k
        $a set bucket_ 1
        $a attach-traffic $exp1
        $a set lifetime_ [$hvar_ value]
        $ns_ at $starttime "$a start"
        $a instvar trafgen_
        set trafgen_ $exp1
} 

TestSuite instproc sched-source {node} {
	$self instvar srcno_ ns_ fvar_  param_ f_
 

        $self create-source $node  [$ns_ now]  $srcno_
        if { $param_(trace_flow) } {
                puts $f_ "Flow $srcno started @ [$ns now]"
        }
        incr srcno_
 
        #generate another startime
        set starttime [expr [$ns_ now]+[$fvar_ value]]
        $ns_ at $starttime "$self sched-source $node"

}


TestSuite proc runTest {} {
	global argc argv quiet

	set test [lindex $argv 0]

	set t [new Test/$test]

	$t show-simtime

	$t run
}

TestSuite instproc show-simtime {} {
	global tcl_precision
	$self instvar ns_ qmon_ param_ f_ flows_ totflows_
	$qmon_ instvar bdepartures_ pdrops_ pdepartures_

	if {$pdepartures_ != 0} {
		set d [expr $pdrops_*1.0/($pdrops_+$pdepartures_)]
	} else {
		set d 0
	}
	if { [$ns_ now] < $param_(meastime) } {
		set time [$ns_ now]
	} else {
		set time [expr [$ns_ now] - $param_(meastime)]
	}
	if { $time == 0 } {
		set thput 0
	} else {
		set thput [expr $bdepartures_ * 8.0 / ($param_(bw) * $time)]
	}
	set oprecision [set tcl_precision]
	set tcl_precision 6
	puts $f_ "[$ns_ now] $thput $d"
	puts $f_ "Flows: $flows_ $totflows_"
	set tcl_precision $oprecision
	$ns_ at [expr [$ns_ now] + 10.0] "$self show-simtime"
}

TestSuite instproc finish { file } {
	global tcl_precision
	$self instvar qmon_ param_ f_
	$qmon_ instvar pdrops_ pdepartures_ bdepartures_

	set utlzn [expr $bdepartures_*8.0/($param_(bw)*($param_(simtime)-$param_(meastime)))]
	set d [expr $pdrops_*1.0/($pdrops_+ $pdepartures_)]

	set oprecision [set tcl_precision]
	set tcl_precision 6
	puts $f_ "Drops : $d Utilization : $utlzn"
	set tcl_precision $oprecision

	close $file
	exec rm -f $file
	exit 0
}


Class Test/MS -superclass TestSuite

Test/MS instproc init {} {
	$self instvar adc_ est_
	set adc_ MS
	set est_ TimeWindow
	$self next
}

Test/MS instproc defaults {} {
	$self next
	ADC/MS set backoff_ false
	ADC/MS set utilization_ 1.02
	Est/TimeWindow set T_ 3
}

Class Test/HB -superclass TestSuite

Test/HB instproc init {} {
	$self instvar adc_ est_
	set adc_ HB
	set est_ ExpAvg
	$self next
}

Test/HB instproc defaults {} {
	$self next
	ADC/HB set epsilon_ 0.999
	Est/ExpAvg set w_ 0.0625
}

Class Test/ACTP -superclass TestSuite

Test/ACTP instproc init {} {
	$self instvar adc_ est_
	set adc_ ACTP
	set est_ PointSample
	$self next
}

Test/ACTP instproc defaults {} {
	$self next
	$self instvar param_
	ADC/ACTP set s_ [expr 2e-7]
	set param_(S) 2.5e4
	Est set period_ [expr $param_(S) * $param_(ptime)]
}

Class Test/ACTO -superclass TestSuite

Test/ACTO instproc init {} {
	$self instvar adc_ est_
	set adc_ ACTO
	set est_ PointSample
	$self next
}

Test/ACTO instproc defaults {} {
	$self next
	$self instvar param_
	ADC/ACTO set s_ [expr 8e-8]
	set param_(S) 2.5e4
	Est set period_ [expr $param_(S) * $param_(ptime)]
}

Class Test/Param -superclass TestSuite

Test/Param instproc init {} {
	$self instvar adc_ est_
	set adc_ Param
	set est_ Null
	$self next
}

Test/Param instproc defaults {} {
	$self next
	ADC/Param set utilization_ 2.05
}

Class SkelTopology

SkelTopology instproc init ns {
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


Class Topology/2Node -superclass SkelTopology

Topology/2Node instproc init ns {

	$self next $ns
	$self instvar node_
	set node_(0) [$ns node]

	set node_(1) [$ns node]

}

#new Test/$TestName

Agent/SA instproc sched-stop { decision } {
        global hold simtime trace_flow
        $self instvar node_ lifetime_
	set T [TestSuite getinstance]

	$T instvar ns_ param_ flows_ totflows_ f_

        if { $decision == 1 } {
                set leavetime [expr [$ns_ now] + $lifetime_]
		$T flowadmit
                $ns_ at [expr [$ns_ now] + $lifetime_] "$self stop;\
                if { $param_(trace_flow) } { \
                    puts $f_ \"Flow [$self set fid_] left @ $leavetime\" \
            }; \
            $ns_ detach-agent $node_ $self; \
            delete [$self set trafgen_]; delete $self; $T flowdepart"
        } else {
                set leavetime [$ns_ now]
                $ns_ at-now "if { $param_(trace_flow) } { \
                    puts $f_ \"Flow [$self set fid_] left @ $leavetime\"\
            }; \
            $ns_ detach-agent $node_ $self; \
            delete [$self set trafgen_]; delete $self"
        }

}



TestSuite runTest


