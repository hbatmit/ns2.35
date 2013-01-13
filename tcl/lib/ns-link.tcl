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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-link.tcl,v 1.49 2009/01/15 06:14:27 tom_henderson Exp $
#

Class Link

Link set nl_ 0

Link instproc init { src dst } {
	$self next

	# Debo
	$self instvar id_
	set id_ [Link set nl_]
        Link set nl_ [expr $id_ + 1]

# puts -nonewline "Link " 
# puts " $id_  init"

        #modified for interface code
	$self instvar trace_ fromNode_ toNode_ color_ oldColor_
	set fromNode_ $src
	set toNode_   $dst
	set color_ "black"
	set oldColor_ "black"

	set trace_ ""
}

Link instproc head {} {
	$self instvar head_
	return $head_
}

Link instproc add-to-head { connector } {
	$self instvar head_
	$connector target [$head_ target]
	$head_ target $connector
}

Link instproc queue {} {
	$self instvar queue_
	return $queue_
}

Link instproc link {} {
	$self instvar link_
	return $link_
}

Link instproc src {}	{ $self set fromNode_	}
Link instproc dst {}	{ $self set toNode_	}
Link instproc cost c	{ $self set cost_ $c	}

Link instproc cost? {} {
	$self instvar cost_
	if ![info exists cost_] {
		set cost_ 1
	}
	set cost_
}

# Debo
Link instproc id {} 	{ $self set id_ }
Link instproc setid { x } { $self set id_ $x }
Link instproc bw {} { $self set bandwidth_ }

Link instproc if-label? {} {
	$self instvar iif_
	$iif_ label
}

Link instproc up { } {
	$self instvar dynamics_ dynT_
	if ![info exists dynamics_] return
	$dynamics_ set status_ 1
	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr format link-up {$src_} {$dst_}
			set ns [Simulator instance]
			$self instvar fromNode_ toNode_
			$tr ntrace "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S UP"
			$tr ntrace "v -t [$ns now] link-up [$ns now] [$fromNode_ id] [$toNode_ id]"
		}
	}
}

Link instproc down { } {
	$self instvar dynamics_ dynT_
	if ![info exists dynamics_] {
		puts stderr "$class::$proc Link $self was not declared dynamic, and cannot be taken down.  ignored"
		return
	}
	$dynamics_ set status_ 0
	$self all-connectors reset
	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr format link-down {$src_} {$dst_}
			set ns [Simulator instance]
			$self instvar fromNode_ toNode_
			$tr ntrace "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DOWN"
			$tr ntrace "v -t [$ns now] link-down [$ns now] [$fromNode_ id] [$toNode_ id]"
		}
	}
}

Link instproc up? {} {
	$self instvar dynamics_
	if [info exists dynamics_] {
		return [$dynamics_ status?]
	} else {
		return "up"
	}
}

Link instproc all-connectors op {
	foreach c [$self info vars] {
		$self instvar $c
		if ![info exists $c] continue
		if [array size $c] continue
		foreach var [$self set $c] {
			if [catch "$var info class"] {
				continue
			}
			if ![$var info class Node] { ;# $op on most everything
				catch "$var $op";# in case var isn't a connector
			}
		}
	}
}

Link instproc install-error {em} {
	$self instvar link_
	$em target [$link_ target]
	$link_ target $em
}

Class SimpleLink -superclass Link

SimpleLink instproc init { src dst bw delay q {lltype "DelayLink"} } {
	$self next $src $dst
	$self instvar link_ queue_ head_ toNode_ ttl_
	$self instvar drophead_

	set ns [Simulator instance]
	set drophead_ [new Connector]
	$drophead_ target [$ns set nullAgent_]

	set head_ [new Connector]
	$head_ set link_ $self

	#set head_ $queue_ -> replace by the following
	# xxx this is hacky
	if { [[$q info class] info heritage ErrModule] == "ErrorModule" } {
		$head_ target [$q classifier]
        } else {
                $head_ target $q
        }

	set queue_ $q
	set link_ [new $lltype]
	$link_ set bandwidth_ $bw
	$link_ set delay_ $delay
	$queue_ target $link_
	$link_ target [$dst entry]
	$queue_ drop-target $drophead_

	# XXX
	# put the ttl checker after the delay
	# so we don't have to worry about accounting
	# for ttl-drops within the trace and/or monitor
	# fabric
	#
	set ttl_ [new TTLChecker]
	$ttl_ target [$link_ target]
	$self ttl-drop-trace
	$link_ target $ttl_

	# Finally, if running a multicast simulation,
	# put the iif for the neighbor node...
	if { [$ns multicast?] } {
		$self enable-mcast $src $dst
	}
        $ns instvar srcRt_
	if [info exists srcRt_] {
        	if { $srcRt_ == 1 } {
            	$self enable-src-rt $src $dst $head_
        	}
	}

}

SimpleLink instproc enable-src-rt {src dst head} {
    $self instvar ttl_
    $src instvar src_agent_
    $ttl_ target [$dst entry]
    $src_agent_ install_slot $head [$dst id]
}


SimpleLink instproc enable-mcast {src dst} {
	$self instvar iif_ ttl_
	set iif_ [new NetworkInterface]
	$iif_ target [$ttl_ target]
	$ttl_ target $iif_

        $src add-oif [$self head]  $self
        $dst add-iif [$iif_ label] $self
}

# Debo

SimpleLink instproc bw {} { 
	$self instvar link_
	$link_ set bandwidth_ 

}

SimpleLink instproc delay {} {
        $self instvar link_
        $link_ set delay_
}

SimpleLink instproc qsize {} {
	[$self queue] set limit_
}

#
# should be called after SimpleLink::trace
#
SimpleLink instproc nam-trace { ns f } {
	$self instvar enqT_ deqT_ drpT_ rcvT_ dynT_

	#XXX 
	# we use enqT_ as a flag of whether tracing has been
	# initialized
	if [info exists enqT_] {
		$enqT_ namattach $f
		if [info exists deqT_] {
			$deqT_ namattach $f
		}
		if [info exists drpT_] {
			$drpT_ namattach $f
		}
		if [info exists rcvT_] {
			$rcvT_ namattach $f
		}
		if [info exists dynT_] {
			foreach tr $dynT_ {
				$tr namattach $f
			}
		}
	} else {
		$self trace $ns $f "nam"
	}
}

#
# Build trace objects for this link and
# update the object linkage
#
# create nam trace files if op == "nam"
#
SimpleLink instproc trace { ns f {op ""} } {

	$self instvar enqT_ deqT_ drpT_ queue_ link_ fromNode_ toNode_
	$self instvar rcvT_ ttl_ trace_
	$self instvar drophead_		;# idea stolen from CBQ and Kevin

	set trace_ $f
	set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_ $op]
	set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_ $op]
	set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_ $op]
	set rcvT_ [$ns create-trace Recv $f $fromNode_ $toNode_ $op]

	$self instvar drpT_ drophead_
	set nxt [$drophead_ target]
	$drophead_ target $drpT_
	$drpT_ target $nxt

	$queue_ drop-target $drophead_

#	$drpT_ target [$queue_ drop-target]
#	$queue_ drop-target $drpT_

	$deqT_ target [$queue_ target]
	$queue_ target $deqT_

	# head is, like the drop-head_ a special connector.
	# mess not with it.
	$self add-to-head $enqT_

	# put recv trace after ttl checking, so that only actually 
	# received packets are recorded
	$rcvT_ target [$ttl_ target]
	$ttl_ target $rcvT_

	$self instvar dynamics_
	if [info exists dynamics_] {
		$self trace-dynamics $ns $f $op
	}
}

SimpleLink instproc trace-dynamics { ns f {op ""}} {
	$self instvar dynT_ fromNode_ toNode_
	lappend dynT_ [$ns create-trace Generic $f $fromNode_ $toNode_ $op]
	$self transit-drop-trace
	$self linkfail-drop-trace
}

SimpleLink instproc ttl-drop-trace args {
	$self instvar ttl_
	if ![info exists ttl_] return
	if {[llength $args] != 0} {
		$ttl_ drop-target [lindex $args 0]
	} else {
		$self instvar drophead_
		$ttl_ drop-target $drophead_
	}
}

SimpleLink instproc transit-drop-trace args {
	$self instvar link_
	if {[llength $args] != 0} {
		$link_ drop-target [lindex $args 0]
	} else {
		$self instvar drophead_
		$link_ drop-target $drophead_
	}
}

SimpleLink instproc linkfail-drop-trace args {
	$self instvar dynamics_
	if ![info exists dynamics_] return
	if {[llength $args] != 0} {
		$dynamics_ drop-target [lindex $args 0]
	} else {
		$self instvar drophead_
		$dynamics_ drop-target $drophead_
	}
}

#
# Trace to a callback function rather than a file.
#
SimpleLink instproc trace-callback {ns cmd} {
	$self trace $ns {}
	foreach part {enqT_ deqT_ drpT_ rcvT_} {
		$self instvar $part
		set to [$self set $part]
		$to set callback_ 1
		$to proc handle a "$cmd \$a"
	}
}

#
# like init-monitor, but allows for specification of more of the items
# attach-monitors $insnoop $inqm $outsnoop $outqm $dropsnoop $dropqm
#
SimpleLink instproc attach-monitors { insnoop outsnoop dropsnoop qmon } {
	$self instvar drpT_ queue_ snoopIn_ snoopOut_ snoopDrop_
	$self instvar qMonitor_ drophead_

	set snoopIn_ $insnoop
	set snoopOut_ $outsnoop
	set snoopDrop_ $dropsnoop

	$self add-to-head $snoopIn_

	$snoopOut_ target [$queue_ target]
	$queue_ target $snoopOut_

	set nxt [$drophead_ target]
	$drophead_ target $snoopDrop_
	$snoopDrop_ target $nxt

#	if [info exists drpT_] {
#		$snoopDrop_ target [$drpT_ target]
#		$drpT_ target $snoopDrop_
#		$queue_ drop-target $drpT_
#	} else {
#		$snoopDrop_ target [[Simulator instance] set nullAgent_]
#		$queue_ drop-target $snoopDrop_
#	}

	$snoopIn_ set-monitor $qmon
	$snoopOut_ set-monitor $qmon
	$snoopDrop_ set-monitor $qmon
	set qMonitor_ $qmon
}

# 
# Added by Yun Wang, based on attach-monitors
# 
# like init-monitor, but allows for specification of more of the items
# attach-taggers $insnoop $inqm
#
SimpleLink instproc attach-taggers { insnoop qmon } {
        $self instvar drpT_ queue_ head_ snoopIn_ snoopOut_ snoopDrop_
        $self instvar qMonitor_ drophead_

        set snoopIn_ $insnoop

        $snoopIn_ target $head_
        set head_ $snoopIn_

        $snoopIn_ set-monitor $qmon

# This may cause problem when you want to insert both flow monitor and tagger.
# Yun Wang

        set qMonitor_ $qmon

}

#
# Insert objects that allow us to monitor the queue size
# of this link.  Return the name of the object that
# can be queried to determine the average queue size.
#
SimpleLink instproc init-monitor { ns qtrace sampleInterval} {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_

	set ns_ $ns
	set qtrace_ $qtrace
	set sampleInterval_ $sampleInterval
	set qMonitor_ [new QueueMonitor]

	$self attach-monitors [new SnoopQueue/In] \
		[new SnoopQueue/Out] [new SnoopQueue/Drop] $qMonitor_

	set bytesInt_ [new Integrator]
	$qMonitor_ set-bytes-integrator $bytesInt_
	set pktsInt_ [new Integrator]
	$qMonitor_ set-pkts-integrator $pktsInt_
	return $qMonitor_
}

SimpleLink instproc start-tracing { } {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_
	$self instvar fromNode_ toNode_

	if {$qtrace_ != 0} {
		$qMonitor_ trace $qtrace_
	}
	$qMonitor_ set-src-dst [$fromNode_ id] [$toNode_ id]
} 

SimpleLink instproc queue-sample-timeout { } {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_
	$self instvar fromNode_ toNode_
	
	set qavg [$self sample-queue-size]
	if {$qtrace_ != 0} {
		puts $qtrace_ "[$ns_ now] [$fromNode_ id] [$toNode_ id] $qavg"
	}
	$ns_ at [expr [$ns_ now] + $sampleInterval_] "$self queue-sample-timeout"
}

SimpleLink instproc sample-queue-size { } {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_ lastSample_

	set now [$ns_ now]
	set qBytesMonitor_ [$qMonitor_ get-bytes-integrator]
	set qPktsMonitor_ [$qMonitor_ get-pkts-integrator]

	$qBytesMonitor_ newpoint $now [$qBytesMonitor_ set lasty_]
	set bsum [$qBytesMonitor_ set sum_]

	$qPktsMonitor_ newpoint $now [$qPktsMonitor_ set lasty_]
	set psum [$qPktsMonitor_ set sum_]

	if ![info exists lastSample_] {
		set lastSample_ 0
	}
	set dur [expr $now - $lastSample_]
	if { $dur != 0 } {
		set meanBytesQ [expr $bsum / $dur]
		set meanPktsQ [expr $psum / $dur]
	} else {
		set meanBytesQ 0
		set meanPktsQ 0
	}
	$qBytesMonitor_ set sum_ 0.0
	$qPktsMonitor_ set sum_ 0.0
	set lastSample_ $now

	#return "$meanBytesQ $meanPktsQ"

	$qMonitor_ instvar pdrops_ pdepartures_ parrivals_ bdrops_ bdepartures_ barrivals_

	return "$meanBytesQ $meanPktsQ $parrivals_ $pdepartures_ $pdrops_ $barrivals_ $bdepartures_ $bdrops_"	

}	


SimpleLink instproc dynamic {} {
	$self instvar dynamics_

	if [info exists dynamics_] return
	
	set dynamics_ [new DynamicLink]
	$self add-to-head $dynamics_
	
	$self transit-drop-trace
	$self all-connectors isDynamic
}

#
# insert an "error module" BEFORE the queue
# point the em's drop-target to the drophead
#
SimpleLink instproc errormodule args {
	$self instvar errmodule_ queue_ drophead_
	if { $args == "" } {
		return $errmodule_
	}

	set em [lindex $args 0]
	set errmodule_ $em

	$self add-to-head $em

	$em drop-target $drophead_
}

#
# Insert a loss module AFTER the queue. 
#
SimpleLink instproc insert-linkloss args { 
	$self instvar link_errmodule_ queue_ drophead_ link_ 
	if { $args == "" } {
		return $link_errmodule_
	}

	set em [lindex $args 0]
	if [info exists link_errmodule_] {
		delete link_errmodule_
	}
	set link_errmodule_ $em

	$em target [$link_ target]
	$link_ target $em

	$em drop-target $drophead_
}


