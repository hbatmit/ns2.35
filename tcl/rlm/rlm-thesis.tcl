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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/rlm/rlm-thesis.tcl,v 1.1 1998/02/20 20:46:48 bajaj Exp $
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

MMG instproc neq {} {
	$self instvar equilibrium
	set n 0
	foreach v [array names equilibrium] {
		if $equilibrium($v) {
			incr n
		}
	}
	return $n
}

MMG instproc check-equilibrium {} {
	global equilibrium rlm_param NLEVEL
	$self instvar equilibrium layer
	if ![info exists equilibrium] {
		set equilibrium 0
	}
	# see if the next higher-level is maxed out
	set n [expr [$self subscription] + 1]
	if { $n >= $NLEVEL || [$layer($n) timer] >= $rlm_param(max) } {
		set eq 1
	} else {
		set eq 0
	}
	if { $equilibrium != $eq } {
		set equilibrium $eq
		$self debug "EQ $eq [$self neq]"
	}
}

MMG instproc backoff-one { n alpha } {
	$self debug "BACKOFF $n by $alpha"
	$self instvar layer
	$layer($n) backoff $alpha
}

MMG instproc backoff n {
	$self debug "BACKOFF $n"
	global rlm_param
	$self instvar maxlevel layer
	set alpha $rlm_param(alpha)
	set L $layer($n)
	$L backoff $alpha
	incr n
	while { $n <= $maxlevel } {
		$layer($n) peg-backoff $L
		incr n
	}
	$self check-equilibrium
}

MMG instproc highest_level_pending {} {
	$self instvar maxlevel
	set m ""
	set n 0
	incr n
	while { $n <= $maxlevel } {
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
		$self debug "H-THRESH $nloss $npkts [expr double($nloss) / ($nloss + $npkts)]"
		if { [expr double($nloss) / ($nloss + $npkts)] > 0.25 } {
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
	
	$self instvar state subscription pending_ts
	if { $state == "/M" } {
		if [$self exceed_loss_thresh] {
			cancel_timer TD $self
			$self drop-layer
			$self check-equilibrium
			$self enter_D
		}
		return
	}
	if { $state == "/S" } {
		cancel_timer TD $self
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
			if { $n == $subscription } {
				set ts $pending_ts($subscription)
				$self rlm_update_D [expr [ns-now] - $ts]
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
			if { $n == [expr $subscription + 1] } {
				cancel_timer TJ $self
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
	if { $state == "/H" || $state == "/D" } {
		return
	}
	puts stderr "rlm state machine botched"
	exit -1
}

MMG instproc relax_TJ {} {
	$self instvar subscription layer
	if { $subscription > 0 } {
		$layer($subscription) relax
		$self check-equilibrium
	}
}

MMG instproc trigger_TD {} {
	$self instvar state
	if { $state == "/H" } {
		$self enter_M
		return
	}
	if { $state == "/D" || $state == "/M" } {
		$self set-state /S
		$self set_TD_timer_conservative
		return
	}
	if { $state == "/S" } {
		$self relax_TJ
		$self set_TD_timer_conservative
		return
	}
	puts stderr "trigger_TD: rlm state machine botched $state)"
	exit -1
}

MMG instproc set_TJ_timer {} {
	global rlm_param
	$self instvar subscription layer
	set n [expr $subscription + 1]
	if ![info exists layer($n)] {
		#
		# no timer -- means we're maximally subscribed
		#
		return
	}
	set I [$layer($n) timer]
	set d [expr $I / 2.0 + [trunc_exponential $I]]
	$self debug "TJ $d"
	set_timer TJ $self $d
}

MMG instproc set_TD_timer_conservative {} {
	$self instvar TD TDVAR
	set delay [expr $TD + 1.5 * $TDVAR]
	set_timer TD $self $delay
}

MMG instproc set_TD_timer_wait {} {
	$self instvar TD TDVAR
	#XXX factor of 2?
	$self instvar subscription
	set k [expr $subscription / 2. + 1.5]
	#	set k 2
	set_timer TD $self [expr $TD + $k * $TDVAR]
}

#
# Return true iff the time given by $ts is recent enough
# such that any action taken since then is likely to influence the
# present or future
#
MMG instproc is-recent { ts } {
	$self instvar TD TDVAR
	set ts [expr $ts + ($TD + 2 * $TDVAR)]
	if { $ts > [ns-now] } {
		return 1
	}
	return 0
}

MMG instproc level_pending n {
	$self instvar pending_ts
	if { [info exists pending_ts($n)] && \
		 [$self is-recent $pending_ts($n)] } {
		return 1
	}
	return 0
}

MMG instproc level_recently_joined n {
	$self instvar join_ts
	if { [info exists join_ts($n)] && \
		 [$self is-recent $join_ts($n)] } {
		return 1
	}
	return 0
}

MMG instproc pending_inferior_jexps {} {
	set n 0
	$self instvar subscription
	while { $n <= $subscription } { 
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
#MINE : changed this so that at least the base layer is always subscribed to
MMG instproc trigger_TJ {} {
	$self debug "trigger-TJ"
	$self instvar state ctrl subscription
	if { ($state == "/S" && ![$self pending_inferior_jexps] && \
		  [$self current_layer_getting_packets])  } {
		$self add-layer
		$self check-equilibrium
		set msg "add $subscription"
		$ctrl send $msg
		#XXX loop back message
		$self local-join
	}
	$self set_TJ_timer
}

MMG instproc our_level_recently_added {} {
	$self instvar subscription layer
	return [$self is-recent [$layer($subscription) last-add]]
}

proc recv_ctrl { mmg msg } {
	$mmg recv-ctrl $msg
}

MMG instproc recv-ctrl msg {
	$self instvar join_ts pending_ts subscription
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
	set join_ts($level) [ns-now]
	if { $level > $subscription } {
		set pending_ts($level) [ns-now]
	}
}

MMG instproc local-join {} {
	$self instvar subscription pending_ts join_ts
	set join_ts($subscription) [ns-now]
	set pending_ts($subscription) [ns-now]
}
