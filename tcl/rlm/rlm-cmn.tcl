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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/rlm/rlm-cmn.tcl,v 1.1 1998/02/20 20:46:46 bajaj Exp $
#

Class MMG

MMG instproc addr {} {
	$self instvar addr
	return $addr
}

MMG instproc node {} {
	$self instvar node
	return $node
}

MMG instproc subscription {} {
	$self instvar subscription
	return $subscription
}

#XXX
MMG instproc state {} {
	$self instvar state
	return $state
}

Class Layer

#XXX goes in ns-lib.tcl
Agent/LossMonitor set npkts_ 0
Agent/LossMonitor set bytes_ 0
Agent/LossMonitor set nlost_ 0
Agent/LossMonitor set lastPktTime_ 0

Class LossTrace -superclass Agent/LossMonitor
LossTrace instproc log-loss {} {
	$self instvar mmg
	#XXX
	$mmg log-loss
	
	global lossTraceFile lossNode
	if [info exists lossTraceFile] {
		set id [[$mmg node] id]
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
	$self instvar lastSeqno expected_ layerNo lastTime lastPktTime_ mmg seqno_
	if [info exists lastSeqno] {
		set id [[$mmg node] id]
		set npkt [expr $seqno_ - $lastSeqno]
		if { $npkt != 0 } {
			puts $lossTraceFile \
			    "p $id $layerNo $lastTime $lastPktTime_ $npkt"
		}
		unset lastSeqno
	}
}

LossTrace instproc init {} {
	$self next
	$self instvar lastTime seqno_
	set lastTime 0
}

LossTrace set expected_ -1

Layer instproc init { ns m addr layerNo } {
	$self next
	$self instvar mon TJ mmg npkts_
	global rlm_param
	set mmg $m
	set TJ $rlm_param(init-tj)
	#XXX subscribe to all groups
	set mon [$ns create-agent [$mmg node] LossTrace 0]
	$mon set layerNo $layerNo
	$mon set mmg $mmg
	$mon set dst_ $addr
	set npkts_ 0

	global allMon
	lappend allMon $mon
}

#Layer should relax by beta and not alpha
Layer instproc relax {} {
	global rlm_param
	$self instvar TJ
	set TJ [expr $TJ * $rlm_param(beta)]
	if { $TJ <= $rlm_param(init-tj) } {
		set TJ $rlm_param(init-tj)
	}
}

Layer instproc backoff alpha {
	global rlm_param
	$self instvar TJ
	set TJ [expr $TJ * $alpha]
	if { $TJ >= $rlm_param(max) } {
		set TJ $rlm_param(max)
	}
}

Layer instproc peg-backoff L {
	$self instvar TJ
	set t [$L set TJ]    
	if { $t >= $TJ } {
		set TJ $t
	}
}

Layer instproc timer {} {
	$self instvar TJ
	return $TJ
}

#XXX get rid of this method!
Layer instproc mon { } {
	$self instvar mon
	return $mon
}

Layer instproc last-add {} {
	$self instvar add_time
	return $add_time
}

Layer instproc join-group {} {
	$self instvar mon mmg npkts_ add_time
	$mon clear
	[$mmg node] join-group $mon [$mon set dst_]
	puts stderr "[$mmg node] join-group $mon [$mon set dst_]"
	
	set npkts_ [$mon set npkts_]
	set add_time [ns-now]
}

Layer instproc leave-group {} {
	$self instvar mon mmg
	[$mmg node] leave-group $mon [$mon set dst_]
}

Layer instproc getting-pkts {} {
	$self instvar mon npkts_
	return [expr [$mon set npkts_] != $npkts_]
}

MMG instproc init { ns localNode baseGroup n } {
	$self next
	global rlm_param
	$self instvar node addr maxlevel TD TDVAR ctrl \
	    total_bits subscription state layer
	set node $localNode
	set addr $baseGroup
	set maxlevel $n
	set TD $rlm_param(init-td)
	set TDVAR $rlm_param(init-td-var)
	
	set ctrl [$ns create-agent $node Agent/Message 0]
	$ctrl proc handle msg "recv_ctrl $self \$msg"
	#XXX
	set caddr [expr $baseGroup + 0x1000]
	$node join-group $ctrl $caddr
	$ctrl set dst_ $caddr
	
	#
	# we number the subscription level starting at 1.
	# level 0 means no groups are subscribed to.
	# 
	set i 1
	while { $i <= $n } {
		set layerNo [expr $i - 1]
		set dst [expr $baseGroup + $layerNo]
		set layer($i) [new Layer $ns $self $dst $layerNo]
		$self instvar layers
		lappend layers $layer($i)
		incr i
		
	}
	
	set total_bits 0
	set state /S
	
	#
	# set the subscription level to 0 and call add_layer
	# to start out with at least one group
	#
	set subscription 0
	$self add-layer
	
	#XXX reset start state to SS (from AL) 
	set state /S
	#
	# Schedule the initial join-timer.
	#
	$self set_TJ_timer
}

#
# set up the data structures so that the $n consecutive addresses
# starting with $baseGroup will act as an MMG set at $node.
#
proc build_loss_monitors { ns node baseGroup n } {
	global rlm_param
	set mmg [new MMG $ns $node $baseGroup $n]

	global allmmg
	lappend allmmg $mmg
	
	return $mmg
}

MMG instproc total_bits {} {
	$self instvar total_bits
	return $total_bits
}

MMG instproc add-bits bits {
	$self instvar total_bits
	set total_bits [expr $total_bits + $bits]
}

MMG instproc total_bytes_delivered {} {
	$self instvar layers
        set v 0
        foreach s $layers {
                incr v [[$s mon] set bytes]
        }
        return $v
}

MMG instproc trace { trace } {
        $self instvar layers
        foreach s $layers {
		[$s mon] trace $trace
        }
}

proc rlm_init { rectFileName nlevel runtime } {
	#XXX
	global NLEVEL
	set NLEVEL $nlevel
	
	if { $rectFileName != "" } {
		global rect_file
		set rect_file [open $rectFileName "w"]
		set h [expr $nlevel * 10 + 10]
		puts $rect_file "bbox 0 0 $runtime $h"
	}
}

proc rlm_finish { } {
	global allmmg rect_file
	foreach mmg $allmmg {
		set s [$mmg subscription]
		while { $s >= 1 } {
			incr s -1
		}
	}
	if [info exists rect_file] {
		close $rect_file
	}
	#XXX
	global lossTraceFile
	if [info exists lossTraceFile] {
		global allMon
		foreach m $allMon {
			$m flush
		}
	}
}

proc rlm_debug msg {
	global rlm_debug_flag
	if $rlm_debug_flag {
		puts stderr "[format %.05f [ns-now]] $msg"
	}
}

#XXX should have debug flag on each obj
MMG instproc debug msg {
	$self instvar addr subscription state node
	rlm_debug "nd [$node id] $addr layer $subscription $state $msg"
}

MMG instproc set-state s {
	$self instvar state node
	set old $state
	set state $s
	$self debug "FSM: $old -> $s"
}

proc rect_write { f mmg x0 x1 n } {
	set addr [[$mmg node] id]
	set base [expr [$mmg addr] & 0x7fff]
	puts $f "r $addr $base $x0 $x1 $n"
}

proc rect_add { mmg n } {
	global rect_start
	set rect_start($mmg:$n) [ns-now]
}


proc rect_del { mmg n } {
    	global rect_start rect_file src_rate
	
	set mmgAddr [$mmg addr]
	set x $rect_start($mmg:$n)
	set rate $src_rate([expr $mmgAddr + $n - 1])
	set bits [expr ([ns-now] - $x) * $rate]
	$mmg add-bits $bits
	
	if [info exists rect_file] {
		rect_write $rect_file $mmg $x [ns-now] $n
	}
}

MMG instproc dumpLevel {} {
	global rlmTraceFile rates
	if [info exists rlmTraceFile] {
		$self instvar subscription node rateMap
		#XXX
		if ![info exists rateMap] {
			set s 0
			set rateMap "0"
			foreach r $rates {
				set s [expr $s + $r]
				lappend rateMap $s
			}
		}
		set r [lindex $rateMap $subscription]
		puts $rlmTraceFile "[$node id] [ns-now] $r"
	}
}

MMG instproc drop-layer {} {
	$self dumpLevel
	$self instvar subscription layer
	set n $subscription
	#
	# if we have an active layer, drop it
	#
	if { $n > 0 } {
		$self debug "DRP-LAYER $n"
		#		rect_del $self $n
		$layer($n) leave-group 
		incr n -1
		set subscription $n
	}
	$self dumpLevel
}

MMG instproc add-layer {} {
	$self dumpLevel
	$self instvar maxlevel subscription layer
	set n $subscription
	if { $n < $maxlevel } {
		$self debug "ADD-LAYER"
		incr n
		set subscription $n
		$layer($n) join-group
		rect_add $self $n
	}
	$self dumpLevel
}

MMG instproc current_layer_getting_packets {} {
	global rlm_pkts
	$self instvar subscription layer TD
	set n $subscription
	if { $n == 0 } {
		return 0
	}
	$self debug "npkts [[$layer($subscription) mon ] set npkts_]"
	if [$layer($subscription) getting-pkts] {
		return 1
	}
	#XXX hack to adjust TD for large latency case
	global add_time
	set delta [expr [ns-now] - [$layer($subscription) last-add]]
	if { $delta > $TD } {
		set TD [expr 1.2 * $delta]
	}
	return 0
}

#
# return the amount of loss across all the groups of the given mmg
#
MMG instproc mmg_loss {} {
	$self instvar layers
	set loss 0
	foreach s $layers {
		incr loss [[$s mon] set nlost_]
	}
	return $loss
}

#
# return the number of packets received across all the groups of the given mmg
#
MMG instproc mmg_pkts {} {
	$self instvar layers
	set npkts_ 0
	foreach s $layers {
		incr npkts_ [[$s mon] set npkts_]
	}
	return $npkts_
}
