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
# Maintainer: <kannan@isi.edu>.
#

# The API for this code is still somewhat fluid and subject to change.
# Check the documentation for details.

Class rtQueue

Simulator instproc rtmodel { dist parms args } {
	set ret ""
	if { [rtModel info subclass rtModel/$dist] != "" } {
		$self instvar  rtModel_
		set ret [eval new rtModel/$dist $self]
		eval $ret set-elements $args
		eval $ret set-parms $parms
		set trace [$self get-ns-traceall]
		if {$trace != ""} {
			$ret trace $self $trace
		}
		set trace [$self get-nam-traceall]
		if {$trace != ""} {
			$ret trace $self $trace "nam"
		}
		if [info exists rtModel_] {
			lappend rtModel_ $ret
		} else {
			set rtModel_ $ret
		}
	}
	return $ret
}

Simulator instproc rtmodel-configure {} {
    $self instvar rtq_ rtModel_
    if [info exists rtModel_] {
	set rtq_ [new rtQueue $self]
	foreach m $rtModel_ {
	    $m configure
	}
    }
}

Simulator instproc rtmodel-at {at op args} {
    set parms [list $op $at]
    eval $self rtmodel Manual [list $parms] $args
}

Simulator instproc rtmodel-delete model {
    $self instvar rtModel_
    set idx [lsearch -exact $rtModel_ $model]
    if { $idx != -1 } {
	delete $model
	set rtModel_ [lreplace $rtModel_ $idx $idx]
    }
}

#
rtQueue instproc init ns {
    $self next
    $self instvar ns_
    set ns_ $ns
}

rtQueue instproc insq-i { interval obj iproc args } {
    $self instvar rtq_ ns_
    set time [expr $interval + [$ns_ now]]
    if ![info exists rtq_($time)] {
	$ns_ at $time "$self runq $time"
    }
    lappend rtq_($time) "$obj $iproc $args"
    return $time
}

rtQueue instproc insq { at obj iproc args } {
    $self instvar rtq_ ns_
    if {[$ns_ now] >= $at} {
	puts stderr "$proc: Cannot set event in the past"
	set at ""
    } else {
	if ![info exists rtq_($at)] {
	    $ns_ at $at "$self runq $at"
	}
	lappend rtq_($at) "$obj $iproc $args"
    }
    return $at
}

rtQueue instproc delq { time obj } {
    $self instvar rtq_
    set ret ""
    set nevent ""
    if [info exists rtq_($time)] {
	foreach event $rtq_($time) {
	    if {[lindex $event 0] != $obj} {
		lappend nevent $event
	    } else {
		set ret $event
	    }
	}
	set rtq_($time) $nevent		;# XXX
    }
    return ret
}

rtQueue instproc runq { time } {
    $self instvar rtq_
    set objects ""
    foreach event $rtq_($time) {
	set obj   [lindex $event 0]
	set iproc [lindex $event 1]
	set args  [lrange $event 2 end]
	eval $obj $iproc $args
	lappend objects $obj
    }
    foreach obj $objects {
	$obj notify
    }
    unset rtq_($time)
}

#
Class rtModel

rtModel set rtq_ ""

rtModel instproc init ns {
    $self next
    $self instvar ns_ startTime_ finishTime_
    set ns_ $ns
    set startTime_ [$class set startTime_]
    set finishTime_ [$class set finishTime_]
}

rtModel instproc set-elements args {
    $self instvar ns_ links_ nodes_
    if { [llength $args] == 2 } {
	set n0 [lindex $args 0]
	set n1 [lindex $args 1]
	set n0id [$n0 id]
	set n1id [$n1 id]
	
	set nodes_($n0id) $n0
	set nodes_($n1id) $n1
	set links_($n0id:$n1id) [$ns_ link $n0 $n1]
	set links_($n1id:$n0id) [$ns_ link $n1 $n0]
    } else {
	set n0 [lindex $args 0]
	set n0id [$n0 id]
	set nodes_($n0id) $n0
	foreach nbr [$n0 set neighbor_] {
	    set n1 $nbr
	    set n1id [$n1 id]
	    
	    set nodes_($n1id) $n1
	    set links_($n0id:$n1id) [$ns_ link $n0 $n1]
	    set links_($n1id:$n0id) [$ns_ link $n1 $n0]
	}
    }
}

rtModel instproc set-parms args {
    $self instvar startTime_ upInterval_ downInterval_ finishTime_

    set cls [$self info class]
    foreach i {startTime_ upInterval_ downInterval_ finishTime_} {
	if [catch "$cls set $i" $i] {
	    set $i [$class set $i]
	}
    }

    set off "-"
    set up  "-"
    set dn  "-"
    set fin "-"

    switch [llength $args] {
	4 {
	    set off [lindex $args 0]
	    set up  [lindex $args 1]
	    set dn  [lindex $args 2]
	    set fin [lindex $args 3]
	}
	3 {
	    set off [lindex $args 0]
	    set up  [lindex $args 1]
	    set dn  [lindex $args 2]
	}
	2 {
	    set up [lindex $args 0]
	    set dn [lindex $args 1]
	}
    }
    if {$off != "-" && $off != ""} {
	set startTime_ $off
    }
    if {$up != "-" && $up != ""} {
	set upInterval_ $up
    }
    if {$dn != "-" && $dn != ""} {
	set downInterval_ $dn
    }
    if {$fin != "-" && $fin != ""} {
	set finishTime_ $fin
    }
}

rtModel instproc configure {} {
    $self instvar ns_ links_
    if { [rtModel set rtq_] == "" } {
	rtModel set rtq_ [$ns_ set rtq_]
    }

    foreach l [array names links_] {
	$links_($l) dynamic
    }
    $self set-first-event
}

rtModel instproc set-event-exact {fireTime op} {
    $self instvar ns_ finishTime_
    if {$finishTime_ != "-" && $fireTime > $finishTime_} {
	if {$op == "up"} {
	    [rtModel set rtq_] insq $finishTime_ $self $op
	}
    } else {
	[rtModel set rtq_] insq $fireTime $self $op
    }
}

rtModel instproc set-event {interval op} {
    $self instvar ns_
    $self set-event-exact [expr [$ns_ now] + $interval] $op
}

rtModel instproc set-first-event {} {
    $self instvar startTime_ upInterval_
    $self set-event [expr $startTime_ + $upInterval_] down
}

rtModel instproc up {} {
    $self instvar links_
    foreach l [array names links_] {
	$links_($l) up
    }
}

rtModel instproc down {} {
    $self instvar links_
    foreach l [array names links_] {
	$links_($l) down
    }
}

rtModel instproc notify {} {
    $self instvar nodes_ ns_
    foreach n [array names nodes_] {
	$nodes_($n) intf-changed
    }
    [$ns_ get-routelogic] notify
}

rtModel instproc trace { ns f {op ""} } {
    $self instvar links_
    foreach l [array names links_] {
	$links_($l) trace-dynamics $ns $f $op
    }
}

#
# Exponential link failure/recovery models
#

Class rtModel/Exponential -superclass rtModel

rtModel/Exponential instproc set-first-event {} {
	global rtglibRNG

	$self instvar startTime_ upInterval_
	$self set-event [expr $startTime_ + [$rtglibRNG exponential] * $upInterval_] down
}

rtModel/Exponential instproc up { } {
	global rtglibRNG

	$self next
	$self instvar upInterval_
	$self set-event [expr [$rtglibRNG exponential] * $upInterval_] down
}

rtModel/Exponential instproc down { } {
	global rtglibRNG

	$self next
	$self instvar downInterval_
	$self set-event [expr [$rtglibRNG exponential] * $downInterval_] up
}

#
# Deterministic link failure/recovery models
#

Class rtModel/Deterministic -superclass rtModel

rtModel/Deterministic instproc up { } {
	$self next
	$self instvar upInterval_
	$self set-event $upInterval_ down
}

rtModel/Deterministic instproc down { } {
	$self next
	$self instvar downInterval_
	$self set-event $downInterval_ up
}

#
# Route Dynamics instantiated through a trace file.
# Invoked through:
#
#    $ns_ rtmodel Trace $traceFile $node1 [$node2 ... ]
#

Class rtModel/Trace -superclass rtModel

rtModel/Trace instproc get-next-event {} {
    $self instvar tracef_ links_
    while {[gets $tracef_ event] >= 0} {
	set toks [split $event]
	if [info exists links_([lindex $toks 3]:[lindex $toks 4])] {
	    return $toks
	}
    }
    return ""
}

rtModel/Trace instproc set-trace-events {} {
    $self instvar ns_ nextEvent_ evq_
    
    set time [lindex $nextEvent_ 1]
    while {$nextEvent_ != ""} {
	set nextTime [lindex $nextEvent_ 1]
	if {$nextTime < $time} {
	    puts stderr "event $nextEvent_  is before current time $time. ignored."
	    continue
	}
	if {$nextTime > $time} break
	if ![info exists evq_($time)] {
	    set op [string range [lindex $nextEvent_ 2] 5 end]
	    $self set-event-exact $time $op
	    set evq_($time) 1
	}
	set nextEvent_ [$self get-next-event]
    }
}


rtModel/Trace instproc set-parms traceFile {
    $self instvar tracef_ nextEvent_
    if [catch "open $traceFile r" tracef_] {
	puts stderr "cannot open $traceFile"
    } else {
	set nextEvent_ [$self get-next-event]
	if {$nextEvent_ == ""} {
	    puts stderr "no relevant events in $traceFile"
	}
    }
}

rtModel/Trace instproc set-first-event {} {
    $self set-trace-events
}

rtModel/Trace instproc up {} {
    $self next
    $self set-trace-events
}

rtModel/Trace instproc down {} {
    $self next
    $self set-trace-events
}

#
# One-shot route dynamics events
# Invoked through:
#
#	$ns_ link-op $op $at $node1 [$node2 ...]
# or
#	$ns_ rtmodel Manual {$op $at} $node1 [$node2 ...]
#
Class rtModel/Manual -superclass rtModel

rtModel/Manual instproc set-first-event {} {
    $self instvar op_ at_
    $self set-event-exact $at_ $op_ ;# you could concievably set a finishTime_?
}

rtModel/Manual instproc set-parms {op at} {
    $self instvar op_ at_
    set op_ $op
    set at_ $at
}

rtModel/Manual instproc notify {} {
    $self next
    delete $self		;# XXX wierd code alert.
	# If needed, this could be commented out, on the assumption that
	# manual settings will be very limited, and hence not a sufficient
	# drag on memory resources.  For now, play it safe (or is it risky?)
}
