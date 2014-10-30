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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/rlm/rlm.tcl,v 1.1 1998/05/18 22:34:03 aswan Exp $
#

#
# exponential factor for backing off join-timer
#
set rlm_param(alpha) 4
set rlm_param(alpha) 2
set rlm_param(beta) 0.75
set rlm_param(init-tj) 1.5
set rlm_param(init-tj) 10
set rlm_param(init-tj) 5
set rlm_param(init-td) 5
#XXX
#set rlm_param(init-td) 10
set rlm_param(init-td-var) 2
set rlm_param(max) 600
#XXX
set rlm_param(max) 60
set rlm_param(g1) 0.25
set rlm_param(g2) 0.25

#XXX
#set rlm_param(target-exp-time) 5
#puts stderr "rlm: scaling min-rate by M=$M"
#set rlm_param(max) [expr $rlm_param(target-exp-time) * 60 * $M]

#XXX
#puts stderr "rlm: scaling alpha by M=$M"
#set rlm_param(alpha) [expr $rlm_param(alpha) * $M]

#
# The MMG (Multiple Multicast Groups) class implements the RLM
# protocol (Receiver-driven Layered Multicast).  See
# <a href=http://www.cs.berkeley.edu/~mccanne/phd-work/>McCanne's
# thesis</a> for a detailed description of RLM.<p>
#
# This class implements only the basic protocol machinery, it
# does not know anything about either ns or mash.  MMG is an
# abstract class -- you should not instantiate it directly.
# Instead, to use RLM a subclass needs to be created that
# actually joins and leaves groups, makes upcalls on packet
# losses, etc...<p>
#
# Two such subclasses are implemented at the moment, one for
# ns and one for mash.  Note that since all code in the MMG
# base class is shared between ns and mash, you should not
# change anything in this file without being certain that the
# changes will work properly in both ns and mash.<p>
#
# See documentation for the appropriate subclass (i.e., MMG/ns
# or MMG/mash) for details about RLM in different environments.
Class MMG

MMG instproc init { levels } {
	$self next

	$self instvar debug_ env_ maxlevel_
	set debug_ 0
	set env_ [lindex [split [$self info class] /] 1]
	set maxlevel_ $levels

	#XXX
	global rlm_debug_flag
	if [info exists rlm_debug_flag] {
		set debug_ $rlm_debug_flag
	}

	$self instvar TD TDVAR state_ subscription_
	#XXX
	global rlm_param
	set TD $rlm_param(init-td)
	set TDVAR $rlm_param(init-td-var)
	set state_ /S
	
	#
	# we number the subscription level starting at 1.
	# level 0 means no groups are subscribed to.
	# 
	$self instvar layer_ layers_
	set i 1
	while { $i <= $maxlevel_ } {
		set layer_($i) [$self create-layer [expr $i - 1]]
		lappend layers_ $layer_($i)
		incr i
	}
	
	#
	# set the subscription level to 0 and call add_layer
	# to start out with at least one group
	#
	set subscription_ 0
	$self add-layer
	
	set state_ /S

	#
	# Schedule the initial join-timer.
	#
	$self set_TJ_timer
}

MMG instproc set-state s {
	$self instvar state_
	set old $state_
	set state_ $s
	$self debug "FSM: $old -> $s"
}

MMG instproc drop-layer {} {
	$self dumpLevel
	$self instvar subscription_ layer_
	set n $subscription_

	#
	# if we have an active layer, drop it
	#
	if { $n > 0 } {
		$self debug "DRP-LAYER $n"
		$layer_($n) leave-group 
		incr n -1
		set subscription_ $n
	}
	$self dumpLevel
}

MMG instproc add-layer {} {
	$self dumpLevel
	$self instvar maxlevel_ subscription_ layer_
	set n $subscription_
	if { $n < $maxlevel_ } {
		$self debug "ADD-LAYER"
		incr n
		set subscription_ $n
		$layer_($n) join-group
	}
	$self dumpLevel
}

MMG instproc current_layer_getting_packets {} {
	$self instvar subscription_ layer_ TD
	set n $subscription_
	if { $n == 0 } {
		return 0
	}

	set l $layer_($subscription_)
	$self debug "npkts [$l npkts]"
	if [$l getting-pkts] {
		return 1
	}

	#XXX hack to adjust TD for large latency case
	set delta [expr [$self now] - [$l last-add]]
	if { $delta > $TD } {
		set TD [expr 1.2 * $delta]
	}
	return 0
}

#
# return the amount of loss across all the groups of the given mmg
#
MMG instproc mmg_loss {} {
	$self instvar layers_
	set loss 0
	foreach l $layers_ {
		incr loss [$l nlost]
	}
	return $loss
}

#
# return the number of packets received across all the groups of the given mmg
#
MMG instproc mmg_pkts {} {
	$self instvar layers_
	set npkts 0
	foreach l $layers_ {
		incr npkts [$l npkts]
	}
	return $npkts
}

#XXX what is this for?
# deleted some code that didn't seem to be used...
MMG instproc check-equilibrium {} {
	global rlm_param
	$self instvar subscription_ maxlevel_ layer_

	# see if the next higher-level is maxed out
	set n [expr $subscription_ + 1]
	if { $n >= $maxlevel_ || [$layer_($n) timer] >= $rlm_param(max) } {
		set eq 1
	} else {
		set eq 0
	}

	$self debug "EQ $eq"
}

MMG instproc backoff-one { n alpha } {
	$self debug "BACKOFF $n by $alpha"
	$self instvar layer_
	$layer_($n) backoff $alpha
}

MMG instproc backoff n {
	$self debug "BACKOFF $n"
	global rlm_param
	$self instvar maxlevel_ layer_
	set alpha $rlm_param(alpha)
	set L $layer_($n)
	$L backoff $alpha
	incr n
	while { $n <= $maxlevel_ } {
		$layer_($n) peg-backoff $L
		incr n
	}
	$self check-equilibrium
}

MMG instproc highest_level_pending {} {
	$self instvar maxlevel_
	set m ""
	set n 0
	incr n
	while { $n <= $maxlevel_ } {
		if [$self level_pending $n] {
			set m $n
		}
		incr n
	}
	return $m
}

MMG instproc rlm_update_D  D {
	#
	# update detection time estimate
	#
	global rlm_param
	$self instvar TD TDVAR
	
	set v [expr abs($D - $TD)]
	set TD [expr $TD * (1 - $rlm_param(g1)) \
				+ $rlm_param(g1) * $D]
	set TDVAR [expr $TDVAR * (1 - $rlm_param(g2)) \
		       + $rlm_param(g2) * $v]
}

MMG instproc exceed_loss_thresh {} {
	$self instvar h_npkts h_nlost
	set npkts [expr [$self mmg_pkts] - $h_npkts]
	if { $npkts >= 10 } {
		set nloss [expr [$self mmg_loss] - $h_nlost]
		#XXX 0.4
		set loss [expr double($nloss) / ($nloss + $npkts)]
		$self debug "H-THRESH $nloss $npkts $loss"
		if { $loss > 0.25 } {
			return 1
		}
	}
	return 0
}

MMG instproc enter_M {} {
	$self set-state /M
	$self set_TD_timer_wait
	$self instvar h_npkts h_nlost
	set h_npkts [$self mmg_pkts]
	set h_nlost [$self mmg_loss]
}

MMG instproc enter_D {} {
	$self set-state /D
	$self set_TD_timer_conservative
}

MMG instproc enter_H {} {
	$self set_TD_timer_conservative
	$self set-state /H
}

MMG instproc log-loss {} {
	$self debug "LOSS [$self mmg_loss]"
	
	$self instvar state_ subscription_ pending_ts_
	if { $state_ == "/M" } {
		if [$self exceed_loss_thresh] {
			$self cancel_timer TD
			$self drop-layer
			$self check-equilibrium
			$self enter_D
		}
		return
	}
	if { $state_ == "/S" } {
		$self cancel_timer TD
		set n [$self highest_level_pending]
		if { $n != "" } {
			#
			# there is a join-experiment in progress --
			# back off the join-experiment rate for the
			# layer that was doing the experiment
			# if we're at that layer, drop it, and
			# update the detection time estimator.
			#
			$self backoff $n
			if { $n == $subscription_ } {
				set ts $pending_ts_($subscription_)
				$self rlm_update_D [expr [$self now] - $ts]
				$self drop-layer
				$self check-equilibrium
				$self enter_D
				return
			}
			#
			# If we're at the level just below the experimental
			# layer that cause a problem, reset our join timer.
			# The logic is that we just effectively ran an
			# experiment, so we might as well reset our timer.
			# This improves the scalability of the algorithm
			# by limiting the frequency of experiments.
			#
			if { $n == [expr $subscription_ + 1] } {
				$self cancel_timer TJ
				$self set_TJ_timer
			}
		}
		if [$self our_level_recently_added] {
			$self enter_M
			return
		}
		$self enter_H
		return
	}
	if { $state_ == "/H" || $state_ == "/D" } {
		return
	}
	puts stderr "rlm state machine botched"
	exit -1
}

MMG instproc relax_TJ {} {
	$self instvar subscription_ layer_
	if { $subscription_ > 0 } {
		$layer_($subscription_) relax
		$self check-equilibrium
	}
}

MMG instproc trigger_TD {} {
	$self instvar state_
	if { $state_ == "/H" } {
		$self enter_M
		return
	}
	if { $state_ == "/D" || $state_ == "/M" } {
		$self set-state /S
		$self set_TD_timer_conservative
		return
	}
	if { $state_ == "/S" } {
		$self relax_TJ
		$self set_TD_timer_conservative
		return
	}
	puts stderr "trigger_TD: rlm state machine botched $state)"
	exit -1
}

MMG instproc set_TJ_timer {} {
	global rlm_param
	$self instvar subscription_ layer_
	set n [expr $subscription_ + 1]
	if ![info exists layer_($n)] {
		#
		# no timer -- means we're maximally subscribed
		#
		return
	}
	set I [$layer_($n) timer]
	set d [expr $I / 2.0 + [trunc_exponential $I]]
	$self debug "TJ $d"
	$self set_timer TJ $d
}

MMG instproc set_TD_timer_conservative {} {
	$self instvar TD TDVAR
	set delay [expr $TD + 1.5 * $TDVAR]
	$self set_timer TD $delay
}

MMG instproc set_TD_timer_wait {} {
	$self instvar TD TDVAR
	#XXX factor of 2?
	$self instvar subscription_
	set k [expr $subscription_ / 2. + 1.5]
	#	set k 2
	$self set_timer TD [expr $TD + $k * $TDVAR]
}

#
# Return true iff the time given by $ts is recent enough
# such that any action taken since then is likely to influence the
# present or future
#
MMG instproc is-recent { ts } {
	$self instvar TD TDVAR
	set ts [expr $ts + ($TD + 2 * $TDVAR)]
	if { $ts > [$self now] } {
		return 1
	}
	return 0
}

MMG instproc level_pending n {
	$self instvar pending_ts_
	if { [info exists pending_ts_($n)] && \
		 [$self is-recent $pending_ts_($n)] } {
		return 1
	}
	return 0
}

MMG instproc level_recently_joined n {
	$self instvar join_ts_
	if { [info exists join_ts_($n)] && \
		 [$self is-recent $join_ts_($n)] } {
		return 1
	}
	return 0
}

MMG instproc pending_inferior_jexps {} {
	set n 0
	$self instvar subscription_
	while { $n <= $subscription_ } { 
		if [$self level_recently_joined $n] {
			return 1
		}
		incr n
	}
	$self debug "NO-PEND-INF"
	return 0
}

#
# join the next higher layer when in /S
#
MMG instproc trigger_TJ {} {
	$self debug "trigger-TJ"
	$self instvar state_ ctrl_ subscription_
	if { ($state_ == "/S" && ![$self pending_inferior_jexps] && \
		  [$self current_layer_getting_packets])  } {
		$self add-layer
		$self check-equilibrium
		set msg "add $subscription_"
		$ctrl_ send $msg
		#XXX loop back message
		$self local-join
	}
	$self set_TJ_timer
}

MMG instproc our_level_recently_added {} {
	$self instvar subscription_ layer_
	return [$self is-recent [$layer_($subscription_) last-add]]
}


MMG instproc recv-ctrl msg {
	$self instvar join_ts_ pending_ts_ subscription_
	$self debug "X-JOIN $msg"
	set what [lindex $msg 0]
	if { $what != "add" } {
		#puts RECV/$msg
		return
	}
	set level [lindex $msg 1]
	#
	#XXX
	# only set the join-ts if the outside J.E. is greater
	# than our level.  if not, then we do not want to falsely
	# increase the ts of our levels.XXX say this better.
	#
	set join_ts_($level) [$self now]
	if { $level > $subscription_ } {
		set pending_ts_($level) [$self now]
	}
}

MMG instproc local-join {} {
	$self instvar subscription_ pending_ts_ join_ts_
	set join_ts_($subscription_) [$self now]
	set pending_ts_($subscription_) [$self now]
}

MMG instproc debug { msg } {
	$self instvar debug_ subscription_ state_
	if {$debug_} {
		puts stderr "[gettimeofday] layer $subscription_ $state_ $msg"
	}
}

#XXX
MMG instproc dumpLevel {} {
#	global rlmTraceFile rates
#	if [info exists rlmTraceFile] {
#		$self instvar subscription node rateMap
#		#XXX
#		if ![info exists rateMap] {
#			set s 0
#			set rateMap "0"
#			foreach r $rates {
#				set s [expr $s + $r]
#				lappend rateMap $s
#			}
#		}
#		set r [lindex $rateMap $subscription]
#		puts $rlmTraceFile "[$node id] [ns-now] $r"
#	}
}



Class Layer

Layer instproc init { mmg } {
	$self next

	$self instvar mmg_ TJ npkts_
	global rlm_param
	set mmg_ $mmg
	set TJ $rlm_param(init-tj)
	set npkts_ 0
	# loss trace created in constructor of derived class
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

Layer instproc last-add {} {
	$self instvar add_time_
	return $add_time_
}

Layer instproc join-group {} {
	$self instvar npkts_ add_time_ mmg_
	set npkts_ [$self npkts]
	set add_time_ [$mmg_ now]
	# derived class actually joins group
}

Layer instproc leave-group {} {
	# derived class actually leaves group
}

Layer instproc getting-pkts {} {
	$self instvar npkts_
	return [expr [$self npkts] != $npkts_]
}
