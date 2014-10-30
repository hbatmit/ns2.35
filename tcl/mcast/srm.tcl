# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#
#  The copyright of this module includes the following
#  linking-with-specific-other-licenses addition:
#
#  In addition, as a special exception, the copyright holders of
#  this module give you permission to combine (via static or
#  dynamic linking) this module with free software programs or
#  libraries that are released under the GNU LGPL and with code
#  included in the standard release of ns-2 under the Apache 2.0
#  license or under otherwise-compatible licenses with advertising
#  requirements (or modified versions of such code, with unchanged
#  license).  You may copy and distribute such a system following the
#  terms of the GNU GPL for this module and the licenses of the
#  other code concerned, provided that you include the source code of
#  that other code when and as the GNU GPL requires distribution of
#  source code.
#
#  Note that people who make modified versions of this module
#  are not obligated to grant this special exception for their
#  modified versions; it is their choice whether to do so.  The GNU
#  General Public License gives permission to release a modified
#  version without this exception; this exception also makes it
#  possible to release a modified version which carries forward this
#  exception.
# 
# Source srm-debug.tcl if you want to turn on tracing of events.
# Do not insert probes directly in this code unless you are sure
# that what you want is not available there.
#
#	Author:		Kannan Varadhan	<kannan@isi.edu>
#	Version Date:	Mon Jun 30 15:51:33 PDT 1997
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/mcast/srm.tcl,v 1.17 2005/09/16 03:05:44 tomh Exp $ (USC/ISI)
#

# THis routine is a temporary hack.  It is likely to dissappear
# at some point in time.
#
Agent instproc traffic-source agent {
	$self instvar tg_
	set tg_ $agent
	$tg_ target $self
	$tg_ set agent_addr_ [$self set agent_addr_]
	$tg_ set agent_port_ [$self set agent_port_]
}

Agent/SRM set packetSize_  1024	;# assume default message size for repair is 1K
Agent/SRM set groupSize_   0
Agent/SRM set app_fid_ 0

Agent/SRM set distanceCompute_	ewma

Agent/SRM set C1_	2.0
Agent/SRM set C2_	2.0
Agent/SRM set requestFunction_	"SRM/request"
Agent/SRM set requestBackoffLimit_	5

Agent/SRM set D1_	1.0
Agent/SRM set D2_	1.0
Agent/SRM set repairFunction_	"SRM/repair"

Agent/SRM set sessionDelay_ 1.0
Agent/SRM set sessionFunction_	"SRM/session"

Class Agent/SRM/Deterministic -superclass Agent/SRM
Agent/SRM/Deterministic set C2_ 0.0
Agent/SRM/Deterministic set D2_ 0.0

Class Agent/SRM/Probabilistic -superclass Agent/SRM
Agent/SRM/Probabilistic set C1_ 0.0
Agent/SRM/Probabilistic set D1_ 0.0

Class Agent/SRM/Fixed -superclass Agent/SRM

Class SRM
Class SRM/request -superclass SRM
Class SRM/repair -superclass SRM
Class SRM/session -superclass SRM

source ../mcast/srm-adaptive.tcl

###
Agent/SRM instproc init {} {
	$self next
	$self instvar ns_ requestFunction_ repairFunction_
	set ns_ [Simulator instance]

	$ns_ create-eventtrace Event $self	

	$self init-instvar sessionDelay_
	foreach var {C1_ C2_ D1_ D2_} {
		$self init-instvar $var
	}
	$self init-instvar requestFunction_
	$self init-instvar repairFunction_
	$self init-instvar sessionFunction_
	$self init-instvar requestBackoffLimit_
	$self init-instvar distanceCompute_

	$self array set stats_ [list		\
			dup-req		-1	ave-dup-req	-1	\
			dup-rep		-1	ave-dup-rep	-1	\
			req-delay	0.0	ave-req-delay	-1	\
			rep-delay	0.0	ave-rep-delay	-1	\
			]
}

Agent/SRM instproc delete {} {
	$self instvar ns_ pending_ done_ session_ tg_
	foreach i [array names pending_] {
		$pending_($i) cancel DELETE
		delete $pending_($i)
	}
	$self cleanup
	delete $session_
	if [info exists tg_] {
		delete $tg_
	}
}

Agent/SRM instproc start {} {
	$self instvar node_ dst_addr_	;# defined in Agent base class
	set dst_addr_ [expr $dst_addr_]	; # get rid of possibly leading 0x etc.
	$self cmd start

	$node_ join-group $self $dst_addr_

	$self instvar ns_ session_ sessionFunction_
	set session_ [new $sessionFunction_ $ns_ $self]
	$session_ schedule
}

Agent/SRM instproc start-source {} {
	$self instvar tg_
	if ![info exists tg_] {
		error "No source defined for agent $self"
	}
	$tg_ start
}

Agent/SRM instproc sessionFunction f {
	$self instvar sessionFunction_
	set sessionFunction_ $f
}

Agent/SRM instproc requestFunction f {
	$self instvar requestFunction_
	set requestFunction_ $f
}

Agent/SRM instproc repairFunction f {
	$self instvar repairFunction_
	set repairFunction_ $f
}

Agent/SRM instproc groupSize? {} {
	$self set groupSize_
}

global alpha
if ![info exists alpha] {
	set alpha	0.25
}

proc ewma {ave cur} {
	if {$ave < 0} {
		return $cur
	} else {
		global alpha
		return [expr (1 - $alpha) * $ave + $alpha * $cur]
	}
}

proc instantaneous {ave cur} {
	set cur
}

Agent/SRM instproc compute-ave var {
	$self instvar stats_
	set stats_(ave-$var) [ewma $stats_(ave-$var) $stats_($var)]
}

####

Agent/SRM instproc recv {type args} {
	eval $self recv-$type $args
}

Agent/SRM instproc recv-data {sender msgid} {
	$self instvar pending_
	if ![info exists pending_($sender:$msgid)] {
		# This is a very late data of some wierd sort.
		# How did we get here?
		error "Oy vey!  How did we get here?"
	}
	if {[$pending_($sender:$msgid) set round_] == 1} {
		$pending_($sender:$msgid) cancel DATA
		$pending_($sender:$msgid) evTrace Q DATA
		delete $pending_($sender:$msgid)
		unset pending_($sender:$msgid)
	} else {
		$pending_($sender:$msgid) recv-repair
	}
}

Agent/SRM instproc mark-period period {
	$self compute-ave $period
	$self set stats_($period) 0
}

Agent/SRM instproc request {sender args} {
	# called when agent detects a lost packet
	$self instvar pending_ ns_ requestFunction_
	set newReq 0
	foreach msgid $args {
		if [info exists pending_($sender:$msgid)] {
			error "duplicate loss detection in agent"
		}
		set pending_($sender:$msgid) [new $requestFunction_ $ns_ $self]
		$pending_($sender:$msgid) set-params $sender $msgid
		$pending_($sender:$msgid) schedule
		
		if ![info exists old_($sender:$msgid)] {
			incr newReq
		}
	}
	if $newReq {
		$self mark-period dup-req
	}
}

Agent/SRM instproc update-ave {type delay} {
	$self instvar stats_
	set stats_(${type}-delay) $delay
	$self compute-ave ${type}-delay
}

Agent/SRM instproc recv-request {requestor round sender msgid} {
	# called when agent receives a control message for an old ADU
	$self instvar pending_ stats_
	if [info exists pending_($sender:$msgid)] {
		# Either a request or a repair is pending.
		# Possibly mark duplicates.
		if { $round == 1 } {
			incr stats_(dup-req) [$pending_($sender:$msgid)	\
					dup-request?]
		}
		$pending_($sender:$msgid) recv-request
	} else {
		# We have no events pending, only if the ADU is old, and
		# we have received that ADU.
		$self repair $requestor $sender $msgid
	}
}

Agent/SRM instproc repair {requestor sender msgid} {
	# called whenever agent receives a request, and packet exists
	$self instvar pending_ ns_ repairFunction_
	if [info exists pending_($sender:$msgid)] {
		error "duplicate repair detection in agent??  really??"
	}
	set pending_($sender:$msgid) [new $repairFunction_ $ns_ $self]
	$pending_($sender:$msgid) set requestor_ $requestor
	$pending_($sender:$msgid) set-params $sender $msgid
	$pending_($sender:$msgid) schedule
	$self mark-period dup-rep
}

Agent/SRM instproc recv-repair {round sender msgid} {
	$self instvar pending_ stats_
	if ![info exists pending_($sender:$msgid)] {
		# 1.  We didn't hear the request for the older ADU, or
		# 2.  This is a very late repair beyond the $3 d_{S,B}$ wait
		# What should we do?
		#if { $round == 1 } { incr stats_(dup-rep) }
		$self instvar trace_ ns_ node_ 
		if [info exists trace_] {
			# puts -nonewline $trace_ [format "%7.4f" [$ns_ now]]
			# puts $trace_ " n [$node_ id] m <$sender:$msgid> r $round X REPAIR late dup"
		}
	} else {
		if { $round == 1 } {
			incr stats_(dup-rep) [$pending_($sender:$msgid)	\
					dup-repair?]
		}
		$pending_($sender:$msgid) recv-repair
	}
}

Agent/SRM/Fixed instproc repair args {
	$self set D1_ [expr log10([$self set groupSize_])]
	$self set D2_ [expr log10([$self set groupSize_])]
	eval $self next $args
}

####

Agent/SRM instproc clear {obj s m} {
	$self instvar pending_ done_ old_ logfile_
	set done_($s:$m) $obj
	set old_($s:$m) [$obj set round_]
	if [info exists logfile_] {
		$obj dump-stats $logfile_
	}
	unset pending_($s:$m)
	if {[array size done_] > 32} {
		$self instvar ns_
		$ns_ at [expr [$ns_ now] + 0.01] "$self cleanup"
	}
}

Agent/SRM instproc round? {s m} {
	$self instvar old_
	if [info exists old_($s:$m)] {
		return $old_($s:$m)
	} else {
		return 0
	}
}

Agent/SRM instproc cleanup {} {
	# We need to do this somewhere...now seems a good time
	$self instvar done_
	if [info exists done_] {
		foreach i [array names done_] {
			delete $done_($i)
		}
		unset done_
	}
}

Agent/SRM instproc trace file {
	$self set trace_ $file
}

Agent/SRM instproc log file {
	$self set logfile_ $file
}

#
#
# The class definition for the following classes is at the
# top of this file.
# Note that the SRM event handlers are not rooted as TclObjects.
#
SRM instproc init {ns agent} {
	$self next
	$self instvar ns_ agent_ nid_ distf_
	set ns_ $ns
	set agent_ $agent
	set nid_ [[$agent_ set node_] id]
	set distf_ [$agent_ set distanceCompute_]
	if ![catch "$agent_ set trace_" traceVar] {
		$self set trace_ $traceVar
	}
	$self array set times_ [list		\
			startTime [$ns_ now] serviceTime -1 distance -1]
}

SRM instproc set-params {sender msgid} {
	$self next
	$self instvar agent_ sender_ msgid_ round_ sent_
	set sender_ $sender
	set msgid_  $msgid
	set round_  [$agent_ round? $sender_ $msgid_]
	set sent_	0
}

SRM instproc cancel {} {
	$self instvar ns_ eventID_
	if [info exists eventID_] {
		$ns_ cancel $eventID_
		unset eventID_
	}
}

SRM instproc schedule {} {
	$self instvar round_
	incr round_
}

SRM instproc distance? node {
	$self instvar agent_ times_ distf_
	set times_(distance) [$distf_ $times_(distance)	\
				[$agent_ distance? $node]]
}

SRM instproc serviceTime {} {
	$self instvar ns_ times_
	set times_(serviceTime) [expr ([$ns_ now] - $times_(startTime)) / \
			( 2 * $times_(distance))]
}

# Some routines to make SRM tracing standard
SRM instproc logpfx fp {
	$self instvar ns_ nid_ sender_ msgid_ round_
	puts -nonewline $fp [format "%7.4f" [$ns_ now]]
	puts -nonewline $fp " n $nid_ m <$sender_:$msgid_> r $round_ "
}

SRM instproc nam-event-pfx fp {
	$self instvar ns_ nid_ sender_ msgid_ round_
	puts -nonewline $fp "E "
	puts -nonewline $fp [format "%8.6f" [$ns_ now]]
	puts -nonewline $fp " n $nid_ m <$sender_:$msgid_> r $round_ "
}

SRM instproc ns-event-pfx fp {
	$self instvar ns_ nid_ sender_ msgid_ round_
	puts -nonewline $fp "E "
	puts -nonewline $fp [format "%8.6f" [$ns_ now]]
	puts -nonewline $fp " n $nid_ m <$sender_:$msgid_> r $round_ "
}

SRM instproc dump-stats fp {
	$self instvar times_ statistics_
	$self logpfx $fp
	puts -nonewline $fp "type [string range [$self info class] 4 end] "
	puts $fp "[array get times_] [array get statistics_]"
}

SRM instproc evTrace {tag type args} {
	$self instvar trace_ ns_
	$ns_ instvar eventTraceAll_ traceAllFile_ namtraceAllFile_

	if [info exists eventTraceAll_] {
		if {$eventTraceAll_ == 1} {
			if [info exists traceAllFile_] {
				$self ns-event-pfx $traceAllFile_
				puts -nonewline $traceAllFile_ "$tag $type"
				foreach elem $args {
					puts -nonewline $traceAllFile_ " $elem"
				}
				puts $traceAllFile_ {}
			}
	
#			if [info exists namtraceAllFile_] {
#				$self nam-event-pfx $namtraceAllFile_
#				puts -nonewline $namtraceAllFile_ "$tag $type"
#				foreach elem $args {
#					puts -nonewline $namtraceAllFile_ " $elem"
#				}
#				puts $namtraceAllFile_ {}
#			}
		}
	}

	if [info exists trace_] {
		$self logpfx $trace_
		puts -nonewline $trace_ "$tag $type"
		foreach elem $args {
			puts -nonewline $trace_ " $elem"
		}
		puts $trace_ {}
	}
}

####

SRM/request instproc init args {
	eval $self next $args
	$self array set statistics_ "dupRQST 0 dupREPR 0 #sent 0 backoff 0"
}

SRM/request instproc set-params args {
	eval $self next $args
	$self instvar agent_ sender_
	foreach var {C1_ C2_} {
		if ![catch "$agent_ set $var" val] {
			$self instvar $var
			set $var $val
		}
	}
	$self distance? $sender_
	$self instvar backoff_ backoffCtr_ backoffLimit_
	set backoff_ 1
	set backoffCtr_ 0
	set backoffLimit_ [$agent_ set requestBackoffLimit_]

	$self evTrace Q DETECT
}

SRM/request instproc dup-request? {} {
	$self instvar ns_ round_ ignore_
	if {$round_ == 2 && [$ns_ now] <= $ignore_} {
		return 1
	} else {
		return 0
	}
}

SRM/request instproc dup-repair? {} {
	return 0
}

SRM/request instproc backoff? {} {
	$self instvar backoff_ backoffCtr_ backoffLimit_
	set retval $backoff_
	if {[incr backoffCtr_] <= $backoffLimit_} {
		incr backoff_ $backoff_
	}
	set retval
}

SRM/request instproc compute-delay {} {
	$self instvar C1_ C2_
	set rancomp [expr $C1_ + $C2_ * [uniform 0 1]]

	$self instvar sender_ backoff_
	set dist [$self distance? $sender_]
	$self evTrace Q INTERVALS C1 $C1_ C2 $C2_ d $dist i $backoff_
	set delay [expr $rancomp * $dist]
}

SRM/request instproc schedule {} {
	$self instvar ns_ eventID_ delay_
	$self next
	set now [$ns_ now]
	set delay_ [expr [$self compute-delay] * [$self backoff?]]
	set fireTime [expr $now + $delay_]

	$self evTrace Q NTIMER at $fireTime

	set eventID_ [$ns_ at $fireTime "$self send-request"]
}

SRM/request instproc cancel type {
	$self next
	if {$type == "REQUEST" || $type == "REPAIR"} {
		$self instvar agent_ round_
		if {$round_ == 1} {
			$agent_ update-ave req [$self serviceTime]
		}
	}
}

SRM/request instproc send-request {} {
	$self instvar agent_ round_ sender_ msgid_ sent_ round_
	$self evTrace Q SENDNACK

	$agent_ send request $round_ $sender_ $msgid_

	$self instvar statistics_
	incr statistics_(#sent)
	set sent_ $round_
}

SRM/request instproc recv-request {} {
	$self instvar ns_ agent_ round_ delay_ ignore_ statistics_
	if {[info exists ignore_] && [$ns_ now] < $ignore_} {
		incr statistics_(dupRQST)
#		$self evTrace Q NACK dup
	} else {
		$self cancel REQUEST
		$self schedule          ;# or rather, reschedule-rqst 
		set ignore_ [expr [$ns_ now] + ($delay_ / 2)]
		incr statistics_(backoff)
		$self evTrace Q NACK IGNORE-BACKOFF $ignore_
	}
}

SRM/request instproc recv-repair {} {
	$self instvar ns_ agent_ sender_ msgid_ ignore_ eventID_
	if [info exists eventID_] {
		$self serviceTime
		set ignore_ [expr [$ns_ now] + 3 * [$self distance? $sender_]]
		$ns_ at $ignore_ "$agent_ clear $self $sender_ $msgid_"
		$self cancel REPAIR
		$self evTrace Q REPAIR IGNORES $ignore_
	} else {		;# we must be in the 3dS,B holdDown interval
		$self instvar statistics_
		incr statistics_(dupREPR)
#		$self evTrace Q REPAIR dup
	}
}

#
SRM/repair instproc init args {
	eval $self next $args
	$self array set statistics_ "dupRQST 0 dupREPR 0 #sent 0"
}

SRM/repair instproc set-params args {
	eval $self next $args
	$self instvar agent_ requestor_
	foreach var {D1_ D2_} {
		if ![catch "$agent_ set $var" val] {
			$self instvar $var
			set $var $val
		}
	}
	$self distance? $requestor_
	$self evTrace P NACK from $requestor_
}

SRM/repair instproc dup-request? {} {
	return 0
}

SRM/repair instproc dup-repair? {} {
	$self instvar ns_ round_
	if {$round_ == 1} {		;# because repairs do not reschedule
		return 1
	} else {
		return 0
	}
}
#
SRM/repair instproc compute-delay {} {
	$self instvar D1_ D2_
	set rancomp [expr $D1_ + $D2_ * [uniform 0 1]]

	$self instvar requestor_
	set dist [$self distance? $requestor_]
	$self evTrace P INTERVALS D1 $D1_ D2 $D2_ d $dist
	set delay [expr $rancomp * $dist]
}

SRM/repair instproc schedule {} {
	$self instvar ns_ eventID_
	$self next
	set fireTime [expr [$ns_ now] + [$self compute-delay]]

	$self evTrace P RTIMER at $fireTime

	set eventID_ [$ns_ at $fireTime "$self send-repair"]
}

SRM/repair instproc cancel type {
	$self next
	if {$type == "REQUEST" || $type == "REPAIR"} {
		$self instvar agent_ round_
		if {$round_ == 1} {
			$agent_ update-ave rep [$self serviceTime]
		}
	}
}

SRM/repair instproc send-repair {} {
	$self instvar ns_ agent_ round_ sender_ msgid_ requestor_ sent_ round_
	$self evTrace P SENDREP

	$agent_ set requestor_ $requestor_
	$agent_ send repair $round_ $sender_ $msgid_

	$self instvar statistics_
	incr statistics_(#sent)
	set sent_ $round_
}

SRM/repair instproc recv-request {} {
	# duplicate (or multiple) requests
	$self instvar statistics_
	incr statistics_(dupRQST)
#	$self evTrace P NACK dup
}

SRM/repair instproc recv-repair {} {
	$self instvar ns_ agent_ round_ sender_ msgid_ eventID_ requestor_
	if [info exists eventID_] {
		#
		# holdDown is identical to the ignore_ parameter in
		# SRM/request::recv-repair.  However, since recv-request
		# for this class is very simple, we do not need an
		# instance variable to record the current state, and
		# hence only require a simple variable.
		#
		set holdDown [expr [$ns_ now] +		\
				3 * [$self distance? $requestor_]]
		$ns_ at $holdDown "$agent_ clear $self $sender_ $msgid_"
		$self cancel REPAIR
		$self evTrace P REPAIR IGNORES $holdDown
	} else {		;# we must in the 3dS,B holdDown interval
		$self instvar statistics_
		incr statistics_(dupREPR)
#		$self evTrace P REPAIR dup
	}
}

#
SRM/session instproc init args {
	eval $self next $args
	$self instvar agent_ sessionDelay_ round_
	set sessionDelay_ [$agent_ set sessionDelay_]
	set round_ 1
	$self array set statistics_ "#sent 0"

	$self set sender_ 0
	$self set msgid_  0
}

SRM/session instproc delete {} {
	$self instvar $ns_ eventID_
	$ns_ cancel $eventID_
	$self next
}

SRM/session instproc schedule {} {
	$self instvar ns_ agent_ sessionDelay_ eventID_

	$self next

	# What is a reasonable interval to schedule session messages?

	set sessionDelay_ [$agent_ set sessionDelay_]
	set fireTime [expr $sessionDelay_ * [uniform 0.9 1.1]]
	# set fireTime [expr $sessionDelay_ * [uniform 0.9 1.1] * \
    #			(1 + log([$agent_ set groupSize_])) ]

	set eventID_ [$ns_ at [expr [$ns_ now] + $fireTime]		\
			"$self send-session"]
}

SRM/session instproc send-session {} {
	$self instvar agent_ statistics_
	$agent_ send session
	$self evTrace S SESSION
	incr statistics_(#sent)
	$self schedule
}

SRM/session instproc evTrace args {}	;# because I don't want to trace
					 # session messages.

Class SRM/session/log-scaled -superclass SRM/session
SRM/session/log-scaled instproc schedule {} {
	$self instvar ns_ agent_ sessionDelay_ eventID_

	# What is a reasonable interval to schedule session messages?
	#set fireTime [expr $sessionDelay_ * [uniform 0.9 1.1]]
	set fireTime [expr $sessionDelay_ * [uniform 0.9 1.1] * \
			(1 + log([$agent_ set groupSize_])) ]

	set eventID_ [$ns_ at [expr [$ns_ now] + $fireTime]		\
			"$self send-session"]
}
