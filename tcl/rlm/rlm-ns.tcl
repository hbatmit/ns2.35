#
# Copyright (c) 1996 Regents of the University of California.
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
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/rlm/rlm-ns.tcl,v 1.1 1998/05/18 22:34:03 aswan Exp $

#XXX goes in ns-lib.tcl
Agent/LossMonitor set npkts_ 0
Agent/LossMonitor set bytes_ 0
Agent/LossMonitor set nlost_ 0
Agent/LossMonitor set lastPktTime_ 0

Class LossTrace -superclass Agent/LossMonitor
LossTrace set expected_ -1

LossTrace instproc init {} {
	$self next
	$self instvar lastTime
	set lastTime 0
}

LossTrace instproc log-loss {} {
	$self instvar mmg_
	$mmg_ log-loss
	
	global lossTraceFile lossNode
	if [info exists lossTraceFile] {
		set id [[$mmg_ node] id]
		if { [info exists lossNode] && $lossNode != $id } {
			return
		}
		#
		# compute intervals of arrived and lost pkts
		# this code assumes no pkt reordering 
		# (which is true in ns as of 11/96, but this
		# will change in the future)
		#
		set f $lossTraceFile
		$self instvar layerNo seqno_ expected_ lastPktTime_ \
		    lastSeqno lastTime
		if [info exists lastSeqno] {
			set npkt [expr $expected_ - $lastSeqno]
			puts $f "p $id $layerNo $lastTime $lastPktTime_ $npkt"
			set lastTime $lastPktTime_
		}
		set lost [expr $seqno_ - $expected_]
		set t [ns-now]
		puts $f "d $id $layerNo $lastPktTime_ $t $lost"
		set lastSeqno $seqno_
		set lastTime $t
	}
}

LossTrace instproc flush {} {
        global lossTraceFile
	$self instvar lastSeqno expected_ layerNo lastTime \
	    lastPktTime_ mmg_ seqno_
	if [info exists lastSeqno] {
		set id [[$mmg_ node] id]
		set npkt [expr $seqno_ - $lastSeqno]
		if { $npkt != 0 } {
			puts $lossTraceFile \
			    "p $id $layerNo $lastTime $lastPktTime_ $npkt"
		}
		unset lastSeqno
	}
}



Class Layer/ns -superclass Layer

Layer/ns instproc init {ns mmg addr layerNo} {
	$self next $mmg

	$self instvar ns_ addr_ mon_
	set ns_ $ns
	set addr_ $addr
	set mon_ [$ns_ create-agent [$mmg node] LossTrace 0]
	$mon_ set layerNo $layerNo
	$mon_ set mmg_ $mmg
	$mon_ set dst_ $addr
}

Layer/ns instproc join-group {} {
	$self instvar mon_ mmg_ addr_
	$mon_ clear
	[$mmg_ node] join-group $mon_ $addr_
	$self next
}

Layer/ns instproc leave-group {} {
	$self instvar mon_ mmg_ addr_
	[$mmg_ node] leave-group $mon_ $addr_
	$self next
}

Layer/ns instproc npkts {} {
	$self instvar mon_
	return [$mon_ set npkts_]
}

Layer/ns instproc nlost {} {
	$self instvar mon_
	return [$mon_ set nlost_]
}

#XXX get rid of this method!
Layer/ns instproc mon {} {
	$self instvar mon_
	return $mon_
}

#
# This class serves as an interface between the MMG class which
# implements the RLM protocol machinery, and the objects in ns
# that are involved in the RLm protocol (i.e., Node objects
# join/leave multicast groups, LossMonitor objects report packet
# loss, etc...).<p>
#
# See tcl/ex/test-rlm.tcl for an example of how to create a
# simulation script that uses RLM
#
Class MMG/ns -superclass MMG

MMG/ns instproc init {ns localNode caddr addrs} {
	$self instvar ns_ node_ addrs_
	set ns_ $ns
	set node_ $localNode
	set addrs_ $addrs

	$self next [llength $addrs]

	$self instvar ctrl_
	set ctrl_ [$ns create-agent $node_ Agent/Message 0]
	$ctrl_ set dst_ $caddr
	$ctrl_ proc handle msg "$self recv-ctrl \$msg"
	$node_ join-group $ctrl_ $caddr
}

MMG/ns instproc create-layer {layerNo} {
	$self instvar ns_ addrs_
	return [new Layer/ns $ns_ $self [lindex $addrs_ $layerNo] $layerNo]
}

MMG/ns instproc now {} {
	$self instvar ns_
	return [$ns_ now]
}

MMG/ns instproc set_timer {which delay} {
	$self instvar ns_ timers_
	if [info exists timers_($which)] {
		puts "timer botched ($which)"
		exit 1
	}
	set time [expr [$ns_ now] + $delay]
	set timers_($which) [$ns_ at $time "$self trigger_timer $which"]
}

MMG/ns instproc trigger_timer {which} {
	$self instvar timers_
	unset timers_($which)
	$self trigger_$which
}

MMG/ns instproc cancel_timer {which} {
	$self instvar ns_ timers_
	if [info exists timers_($which)] {
		#XXX does this cancel the timer?
		$ns_ at $timers_($which)
		unset timers_($which)
	}
}


#######


MMG/ns instproc node {} {
	$self instvar node_
	return $node_
}

MMG/ns instproc debug { msg } {
	$self instvar debug_
	if {!$debug_} { return }

	$self instvar subscription_ state_ node_
	set time [format %.05f [ns-now]]
	puts stderr "$time node [$node_ id] layer $subscription_ $state_ $msg"
}

MMG/ns instproc trace { trace } {
        $self instvar layers_
        foreach s $layers_ {
		[$s mon] trace $trace
        }
}


MMG/ns instproc total_bytes_delivered {} {
	$self instvar layers_
        set v 0
        foreach s $layers_ {
                incr v [[$s mon] set bytes]
        }
        return $v
}

