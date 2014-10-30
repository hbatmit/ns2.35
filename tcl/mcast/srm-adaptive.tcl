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
#	Author:		Kannan Varadhan	<kannan@isi.edu>
#	Version Date:	Mon Jun 30 15:51:33 PDT 1997
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/mcast/srm-adaptive.tcl,v 1.9 2005/09/16 03:05:44 tomh Exp $ (USC/ISI)
#

Agent/SRM/Adaptive set pdistance_	0.0	;# bound instance variables
Agent/SRM/Adaptive set requestor_ 0

Agent/SRM/Adaptive set C1_	2.0
Agent/SRM/Adaptive set MinC1_	0.5
Agent/SRM/Adaptive set MaxC1_	2.0
Agent/SRM/Adaptive set C2_	2.0
Agent/SRM/Adaptive set MinC2_	1.0
Agent/SRM/Adaptive set MaxC2_	1.0	;# G

Agent/SRM/Adaptive set D1_	-1	;# log10 G
Agent/SRM/Adaptive set MinD1_	0.5
Agent/SRM/Adaptive set MaxD1_	0.0	;# log10 G
Agent/SRM/Adaptive set D2_	-1	;# log10 G	XXX
Agent/SRM/Adaptive set MinD2_	1.0
Agent/SRM/Adaptive set MaxD2_	1.0	;# G

Agent/SRM/Adaptive set requestFunction_	"SRM/request/Adaptive"
Agent/SRM/Adaptive set repairFunction_	"SRM/repair/Adaptive"

Agent/SRM/Adaptive set AveDups_	1.0
Agent/SRM/Adaptive set AveDelay_	1.0

Agent/SRM/Adaptive set eps_	0.10

#Agent/SRM/Adaptive set done_	0
Agent/SRM/Adaptive instproc init args {
    # XXX These are now done in tcl/lib/ns-packet.tcl
#      if ![$class set done_] {
#  	set pm [[Simulator instance] set packetManager_]
#  	TclObject set off_asrm_ [$pm allochdr aSRM]
#  	$class set done_ 1
#      }

    eval $self next $args
    $self array set closest_ "requestor 0 repairor 0"

    $self set AveDups_	[$class set AveDups_]
    $self set AveDelay_	[$class set AveDelay_]

    foreach i [list MinC1_ MaxC1_ MinC2_ MaxC2_			\
	    MinD1_ MaxD1_ MinD2_ MaxD2_] {
	$self instvar $i
	set $i [$class set $i]
    }

    $self set eps_	[$class set eps_]
}

Agent/SRM/Adaptive instproc check-bounds args {
    set G [$self set groupSize_]
    $self set MaxC2_ $G
    $self set MaxD1_ [expr log10($G)]
    $self set MaxD2_ $G
    if {[llength $args] <= 0} {
	set args "C1_ C2_ D1_ D2_"
    }
    foreach i $args {
	$self instvar $i
	set val [$self set $i]	      ;# We do this for notational convenience
	set min [$self set Min$i]
	set max [$self set Max$i]
	if { $val < $min } {
	    set $i $min
	} elseif { $val > $max } {
	    set $i $max
	}
    }
}

Agent/SRM/Adaptive instproc recompute-request-params {} {
    $self instvar closest_ C1_ C2_ stats_ AveDups_ AveDelay_ eps_
    if {$stats_(ave-req-delay) < 0} {
	# This is the very first loss this agent is seeing.
	$self check-bounds C1_ C2_	;# adjust bounds to estimated size of G
	return
    }

    $self compute-ave dup-req
    if $closest_(requestor) {
	set C2_ [expr $C2_ - 0.1]
	set closest_(requestor)	0
    } elseif {$stats_(ave-dup-req) >= $AveDups_} {
	set C1_ [expr $C1_ + 0.1]
	set C2_ [expr $C2_ + 0.5]
    } elseif {$stats_(ave-dup-req) < [expr $AveDups_ - $eps_]} {
	if {$stats_(ave-req-delay) > $AveDelay_} {
	    set C2_ [expr $C2_ - 0.1]
	}
	if {$stats_(ave-dup-req) < 0.25} {
            set C1_ [expr $C1_ - 0.05]
	}
    } else {
	set C1_ [expr $C1_ + 0.05]
    }
    $self check-bounds C1_ C2_
}

Agent/SRM/Adaptive instproc sending-request {} {
    $self set C1_ [expr [$self set C1_] - 0.1]  ;# XXX SF's code uses other
                                                 # value, 0.05.  Why?
    $self set closest_(requestor) 1
    $self check-bounds C1_
}

Agent/SRM/Adaptive instproc recv-request {r d s m} {
    $self instvar pending_ closest_
    if { [info exists pending_($s:$m)]  && $d == 1 } {
	# Should we not set closeness, even if we get a late round message?
	set closeness [$pending_($s:$m) closest-requestor?]
	if {$closeness >= 0} {
	    set closest_(requestor) $closeness
	}
    }
    $self next $r $d $s $m
}

Agent/SRM/Adaptive instproc recompute-repair-params {} {
    $self instvar closest_ D1_ D2_ stats_ AveDups_ AveDelay_ eps_
    if {$stats_(ave-rep-delay) < 0} {
	# This is the very first repair this agent is doing.
#	if {$D1_ != -1 || $D2_ != -1} {
#	    error "invalid repair parameters <$D1_, $D2_>"
#	}
        set logG [expr log10([$self set groupSize_])]
	set D1_  $logG
	set D2_  $logG
	$self check-bounds D1_ D2_	;# adjust bounds to estimated size of G
	return
    }

    $self compute-ave dup-rep
    if $closest_(repairor) {
        set D2_ [expr $D2_ - 0.1]
	set closest_(repairor) 0
    } elseif {$stats_(ave-dup-rep) >= $AveDups_} {
        set D1_ [expr $D1_ + 0.1]
        set D2_ [expr $D2_ + 0.5]
    } elseif {$stats_(ave-dup-rep) < [expr $AveDups_ - $eps_]} {
        if {$stats_(ave-rep-delay) > $AveDelay_} {
            set D2_ [expr $D2_ - 0.1]
	}
	if {$stats_(ave-dup-rep) < 0.25} {
            set D1_ [expr $D1_ - 0.05]
	}
    } else {
        set D1_ [expr $D1_ + 0.05]
    }
    $self check-bounds D1_ D2_
}

Agent/SRM/Adaptive instproc sending-repair {} {
    $self set D1_ [expr [$self set D1_] - 0.1]  ;# XXX SF's code uses other
                                                 # value, D1 -= 0.05.  Why?
    $self set closest_(repairor) 1
    $self check-bounds D1_
}

Agent/SRM/Adaptive instproc recv-repair {d s m} {
    $self instvar pending_ closest_
    if { [info exists pending_($s:$m)] && $d == 1 } {
	# Should we not set closeness, even if we get a late round message?
	set closeness [$pending_($s:$m) closest-repairor?]
	if {$closeness >= 0} {
	    set closest_(repairor) $closeness
	}
    }
    $self next $d $s $m
}

Class SRM/request/Adaptive -superclass SRM/request
SRM/request/Adaptive instproc set-params args {
    $self instvar agent_
    $agent_ recompute-request-params
    eval $self next $args
}

SRM/request/Adaptive instproc backoff? {} {
    $self instvar backoff_ backoffCtr_ backoffLimit_
    set retval $backoff_
    if {[incr backoffCtr_] <= $backoffLimit_} {
	set backoff_ [expr $backoff_ * 3]
    }
    set retval
}

SRM/request/Adaptive instproc schedule {} {
    $self next
}

SRM/request/Adaptive instproc send-request {} {
    $self instvar agent_ round_
    if { $round_ == 1 } {
	$agent_ sending-request
    }
    $self next
}

SRM/request/Adaptive instproc closest-requestor? {} {
    $self instvar agent_ sender_ sent_ round_
    if {$sent_ == 1 && $round_ == 1} {	;# since repairs aren't rescheduled.
        if {[$agent_ set pdistance_] >			\
		[expr 1.5 * [$self distance? $sender_]]} {
            return 1
        } else {
            return 0
        }
    } else {
        return -1
    }
}

SRM/request/Adaptive instproc closest-repairor? {} {
    return -1
}

Class SRM/repair/Adaptive -superclass SRM/repair
SRM/repair/Adaptive instproc set-params args {
    $self instvar agent_
    $agent_ recompute-repair-params
    eval $self next $args
}

SRM/repair/Adaptive instproc schedule {} {
    $self next
}

SRM/repair/Adaptive instproc send-repair {} {
    $self instvar round_ agent_
    if { $round_ == 1 } {
        $agent_ sending-repair
    }
    $self next
}

SRM/repair/Adaptive instproc closest-requestor? {} {
    return -1
}

SRM/repair/Adaptive instproc closest-repairor? {} {
    $self instvar agent_ requestor_ sent_ round_
    if {$sent_ == 1 && $round_ == 1} {
        if {[$agent_ set pdistance_] >			\
		[expr 1.5 * [$self distance? $requestor_]]} {
            return 1
        } else {
            return 0
        }
    } else {
        return -1
    }
}
