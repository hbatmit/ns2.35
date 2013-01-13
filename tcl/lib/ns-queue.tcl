#
# Copyright (c) 1996-1997 Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-queue.tcl,v 1.28 2005/03/21 18:51:30 haldar Exp $
#

#
# This file contains auxillary support for CBQ and CBQ links, and
# some embryonic stuff for Queue Monitors. -KF
#

# Also contains some support for queue tracing and 
# procs for some queues like FQ and RED/PD
#

#
# CBQ
#

#
# set up the baseline CBQ or CBQ/WRR object
#	baseline object contains only an empty classifier
#	and the scheduler (CBQ or CBQ/WRR) object itself
#
# After initialized, the structure is as follows:
#
#
#	head_-> (classifier)
#	queue_-> (cbq) ==> link_
#	drophead_ -> (connector) ==> nullAgent
#
# == is data flow
# -- is a pointer reference
#
Class CBQLink -superclass SimpleLink
CBQLink instproc init { src dst bw delay q cl {lltype "DelayLink"} } {
        $self next $src $dst $bw $delay $q $lltype ; # SimpleLink ctor
        $self instvar head_ queue_ link_
	$self instvar  classifier_	; # not found in a SimpleLink
#	$self instvar  drophead_ ; # not found in a SimpleLink

	$queue_ link $link_ ; # queue_ set by SimpleLink ctor, CBQ needs $link_
        set classifier_ $cl
	$head_ target $classifier_

#	set drophead_ [new Connector]
#	$drophead_ target [[Simulator instance] set nullAgent_]
	#
	# the following is merely to inject 'algorithm_' in the
	# $queue_ class' name space.  It is not used in ns-2, but
	# is needed here for the compat code to recognize that
	# 'set algorithm_ foo' commands should work
	#
	set defalg [Queue/CBQ set algorithm_]
	$queue_ set algorithm_ $defalg
	# this part actually sets the default
	$queue_ algorithm $defalg
}

#
# set up a trace.  Calling the SimpleLink version is harmless, but doesn't
# do exactly what we need
#

#CBQLink instproc trace { ns f } {
#	$self next $ns $f
#	$self instvar drpT_ drophead_
#	set nxt [$drophead_ target]
#	$drophead_ target $drpT_
#	$drpT_ target $nxt
#}

#
# set up monitors.  Once again, the base version is close, but not
# exactly what's needed
#
#CBQLink instproc attach-monitors { isnoop osnoop dsnoop qmon } {
#	$self next $isnoop $osnoop $dsnoop $qmon
#	$self instvar drophead_
#	set nxt [$drophead_ target]
#	$drophead_ target $dsnoop
#	$dsnoop target $nxt
#}

CBQLink instproc classifier {} {
	$self instvar classifier_
	return $classifier_
}

#
#
# for flow-id based classification, bind c
# CBQClass to a given flow id # (or range)
#
# OTcl usage:
# 	bind $cbqclass id
#    		or
# 	bind $cbqclass idstart idend
#
# these use flow id's as id's
#
CBQLink instproc bind args {
	# this is to perform '$cbqlink bind $cbqclass id'
	# and '$cbqlink insert $cbqclass bind $cbqclass idstart idend'

	$self instvar classifier_
	set nargs [llength $args]
	set cbqcl [lindex $args 0]
	set a [lindex $args 1]
	if { $nargs == 3 } {
		set b [lindex $args 2]
	} else {
		set b $a
	}
	# bind the class to the flow id's [a..b]
	while { $a <= $b } {
		# first install the class to get its slot number
		# use the flow id as the hash bucket
		set slot [$classifier_ installNext $cbqcl]
		$classifier_ set-hash auto 0 0 $a $slot
		incr a
	}
}

#
# insert the class into the link
# each class will have an associated queue
# we must create a set of snoop qs with an associated qmon
# which cbq uses to monitor demand.  The qmon may either be
# given, or defaults to just a QueueMonitor type.
#
# Otcl usage:
#	insert $cbqclass
#	insert $cbqclass $qmon
#
# the two different usages are used to make backward compat with
# ns-1 easier, since in ns-2, insert was in c++
#
# general idea:
#  pkt--> Classifier --> CBQClass --> snoopin --> qdisc --> snoopout --> CBQ
#
CBQLink instproc insert args {
	# queue_ refers to the cbq object
	$self instvar queue_ drophead_ link_
	set nargs [llength $args]
	set cbqcl [lindex $args 0]
	set qdisc [$cbqcl qdisc]
	if { $nargs == 1 } {
		set qmon [new QueueMonitor]
	} else {
		set qmon [lindex $args 1]
	}

	# qdisc can be null for internal classes

	if { $qmon == "" } {
		error "CBQ requires a q-monitor for class $cbqcl"
	}
	if { $qdisc != "" } {
		# create in, out, and drop snoop queues
		# and attach them to the same monitor
		# this is used by CBQ to assess demand
		# (we don't need bytes/pkt integrator or stats here)
		set in [new SnoopQueue/In]
		set out [new SnoopQueue/Out]
		set drop [new SnoopQueue/Drop]
		$in set-monitor $qmon
		$out set-monitor $qmon
		$drop set-monitor $qmon

		# output of cbqclass -> snoopy inq
		$in target $qdisc
		$cbqcl target $in

		# drop from qdisc -> snoopy dropq
		# snoopy dropq's target is overall cbq drop target
		$qdisc drop-target $drop
		$drop target $drophead_

		# output of queue -> snoopy outq
		# output of snoopy outq is cbq
		$qdisc target $out
		$out target $queue_
		# tell this class about its new queue monitor
		$cbqcl qmon $qmon
	}


	$cbqcl instvar maxidle_

	if { $maxidle_ == "auto" } {
		$cbqcl automaxidle [$link_ set bandwidth_] \
			[$queue_ set maxpkt_]
		set maxidle_ [$cbqcl set maxidle_]
	}
	$cbqcl maxidle $maxidle_

	# tell cbq about this class
	# (this also tells the cbqclass about the cbq)
	$queue_ insert-class $cbqcl
}
#
# procedures on a cbq class
#

CBQClass instproc init {} {
	$self next
	$self instvar automaxidle_gain_
	set automaxidle_gain_ [$class set automaxidle_gain_]
}

CBQClass instproc automaxidle { linkbw maxpkt } {
	$self instvar automaxidle_gain_ maxidle_
	$self instvar priority_


	set allot [$self allot]


	set g $automaxidle_gain_
	set n [expr 8 * $priority_]

	if { $g == 0 || $allot == 0 || $linkbw == 0 } {
		set maxidle_ 0.0
		return
	}
	set gTOn [expr pow($g, $n)]
	set first [expr ((1/$allot) - 1) * (1-$gTOn) / $gTOn ]
	set second [expr (1 - $g)]
	set t [expr ($maxpkt * 8.0)/$linkbw]
	if { $first > $second } {
		set maxidle_ [expr $t * $first]
	} else {
		set maxidle_ [expr $t * $second]
	}
	return $maxidle_
}


CBQClass instproc setparams { parent okborrow allot maxidle prio level xdelay } {

        $self allot $allot
	$self parent $parent

	$self set okborrow_ $okborrow
        $self set maxidle_ $maxidle
        $self set priority_ $prio
        $self set level_ $level
        $self set extradelay_ $xdelay

        return $self
}

#
# insert a queue into a CBQ class:
#	arrange for the queue to initally be blocked
#	and for it to block when resumed
#	(provides flow control on the queue)

CBQClass instproc install-queue q {
	$q set blocked_ true
	$q set unblock_on_resume_ false
	$self qdisc $q
}

#
# QueueMonitor
#

QueueMonitor instproc reset {} {
	$self instvar size_ pkts_
	$self instvar parrivals_ barrivals_
	$self instvar pdepartures_ bdepartures_
	$self instvar pdrops_ bdrops_

	# don't reset size_ and pkts_ here
	# because they are not cumulative measurements
	# the way the following values are
	set parrivals_ 0
	set barrivals_ 0
	set pdepartures_ 0
	set bdepartures_ 0
	set pdrops_ 0
	set bdrops_ 0

	set bint [$self get-bytes-integrator]
	if { $bint != "" } {
		$bint reset
	}

	set pint [$self get-pkts-integrator]
	if { $pint != "" } {
		$pint reset
	}

	set samp [$self get-delay-samples]
	if { $samp != "" } {
		$samp reset
	}
}

QueueMonitor/ED instproc reset {} {
	$self next
	$self instvar epdrops_ ebdrops_ mon_epdrops_ mon_ebdrops_
	set epdrops_ 0
	set ebdrops_ 0
	set mon_epdrops_ 0
	set mon_ebdrops_ 0
}

Class AckReconsClass -superclass Agent

AckReconsControllerClass instproc demux { src dst } {
	$self instvar reconslist_ queue_
	set addr $src:$dst
	if { ![info exists reconslist_($addr)] } {
		set recons [new Agent/AckReconsClass $src $dst]
		$recons target $queue_
		set reconslist_($addr) $recons
	}
	# return an ack reconstructor object
	return $reconslist_($addr)
}

# Calculate number and spacing of acks to be sent

# deltaAckThresh_ = threshold after which reconstructor kicks in
# ackInterArr_ = estimate of arrival rate of acks ("counting process")
# ackSpacing_ = duration in time between acks sent by reconstructor
# delack_ = generate an ack at least every delack_ acks at reconstructor

Agent/AckReconsClass instproc spacing { ack } {
	$self instvar ackInterArr_ ackSpacing_ delack_ \
			lastAck_ lastRealAck_ lastRealTime_ adaptive_ size_
	global ns
	
	set deltaTime [expr [$ns now] - $lastRealTime_]
	set deltaAck [expr $ack - $lastAck_]
	if {$adaptive_} {
		set bw [expr $deltaAck*$size_/$deltaTime]
		set ackSpacing_ $ackInterArr_
		if { $deltaAck > 0 } {
#			set ackSpacing_ [expr $ackInterArr_*$delack_/$deltaAck]
		}
	} else {
		set deltaT [expr $deltaTime / ($deltaAck/$delack_ +1)]
		set ackSpacing_ $deltaT
	}
}

# Estimate rate at which acks are arriving
Agent/AckReconsClass instproc ackbw {ack time} {
	$self instvar ackInterArr_ lastRealTime_ lastRealAck_ alpha_

	set sample [expr $time - $lastRealTime_]
	# EWMA
	set ackInterArr_ [expr $alpha_*$sample + (1-$alpha_)*$ackInterArr_]
}

Class Classifier/Hash/Fid/FQ -superclass Classifier/Hash/Fid

Classifier/Hash/Fid/FQ instproc unknown-flow { src dst fid } {
	$self instvar fq_
	$fq_ new-flow $src $dst $fid
}

Class FQLink -superclass SimpleLink

FQLink instproc init { src dst bw delay q } {
	$self next $src $dst $bw $delay $q
	$self instvar link_ queue_ head_ toNode_ ttl_ classifier_ \
		nactive_ 
	$self instvar drophead_		;# idea stolen from CBQ and Kevin

	set nactive_ 0

	set classifier_ [new Classifier/Hash/Fid/FQ 33]
	$classifier_ set fq_ $self

	#$self add-to-head $classifier_
	$head_ target $classifier_

	# XXX
	# put the ttl checker after the delay
	# so we don't have to worry about accounting
	# for ttl-drops within the trace and/or monitor
	# fabric
	#

	$queue_ set secsPerByte_ [expr 8.0 / [$link_ set bandwidth_]]
}
FQLink instproc new-flow { src dst fid } {
	$self instvar classifier_ nactive_ queue_ link_ drpT_
	incr nactive_

	set type [$class set queueManagement_]
	set q [new Queue/$type]

	#XXX yuck
	if { $type == "RED" } {
	 	set bw [$link_ set bandwidth_]
		$q set ptc_ [expr $bw / (8. * [$q set mean_pktsize_])]
	}
	$q drop-target $drpT_

	set slot [$classifier_ installNext $q]
	$classifier_ set-hash auto $src $dst $fid $slot
	$q target $queue_
	$queue_ install $fid $q
}
#XXX ask Kannan why this isn't in otcl base class.
FQLink instproc up? { } {
	return up
}

#
#Added by ratul for RedPDQueue
#
Queue/RED/PD instproc makeflowmon { link {cltype "SrcDestFid"} {cslots 29}} {

    set flowmon [new QueueMonitor/ED/Flowmon]
    set cl [new Classifier/Hash/$cltype $cslots]
    
    $cl proc unknown-flow { src dst fid } {
	set nflow [new QueueMonitor/ED/Flow/RedPD]
	set slot [$self installNext $nflow]
	#     	puts "New Flow : $nflow at slot $slot"
	$self set-hash auto $src $dst $fid $slot
	#     	puts "Installed It\n";
    }
    
    $cl proc no-slot slotnum {
	puts stderr "classifier $self, no-slot for slotnum $slotnum"
    }
    
    $flowmon classifier $cl
    $self attach-flowmon $flowmon
    
    set isnoop [new SnoopQueue/In]
    set osnoop [new SnoopQueue/Out]
    set dsnoop [new SnoopQueue/Drop]
    set edsnoop [new SnoopQueue/EDrop]
    
    $link attach-monitors $isnoop $osnoop $dsnoop $flowmon
    $edsnoop set-monitor $flowmon
    $self early-drop-target $edsnoop 
    set ns [Simulator instance]
    $edsnoop target [$ns set nullAgent_]
	
    # $edsnoop target [$dsnoop target]
    # $edsnoop drop-target [$dsnoop drop-target]
 
    $self drop-target $dsnoop
    
    return $flowmon

}	

#############################################################
# Stuff below has been added to enable queue specific tracing
#
#Blame me if anything is broken below - ratul
##########################################################

#
# attach-nam-traces: Only conventional trace objects are understood by nam currently. 
# do not attach fancy trace objects here. nam will crash in this case.
#
Queue instproc attach-nam-traces {src dst file} {
    
	#valid only if the default trace type in attach-traces is Drop, Enque, Deque.
	#see comment above.
	#this function should be different for different queue when needed.
	
	$self attach-traces $src $dst $file "nam"
}

#
# Dummy function for all the queues that don't implement attach-traces
#
Queue instproc attach-traces {src dst file {op ""}} {
	#Do nothing here
}

#
# Added to be able to trace the edrop events 
#
Queue/RED instproc attach-traces {src dst file {op ""}} {
	
        set ns [Simulator instance]
	set type [$self trace-type]
	
	#nam does not understand anything else yet. 
	if {$op == "nam"} {
		set type "Drop"
	}
	
	#puts "In attach-trace: $type"

	set newtrace [$ns create-trace $type $file $src $dst $op]
	set oldTrace [$self edrop-trace]

	#    puts "oldTrace - $oldTrace"
	if {$oldTrace!=0} {
		#	puts "exists"
		$newtrace target $oldTrace
	} else {
		#	puts "Does not exist"
		$newtrace target [$ns set nullAgent_]
	}
	
	$self edrop-trace $newtrace
}

#
# Added to be able to trace the mon_edrop and edrop events
#
Queue/RED/PD instproc attach-traces {src dst file {op ""}} {

	$self next $src $dst $file $op
	
	set ns [Simulator instance]
	set type [$self mon-trace-type]
	
	#nam does not understand anything else yet
	if {$op == "nam"} {
		set type "Drop"
	}
	
	set medtrace [$ns create-trace $type $file $src $dst $op]
	
	set oldTrace [$self mon-edrop-trace]
	if {$oldTrace!=0} {
		puts "exists"
		$medtrace target $oldTrace
	} else {
		#	puts "Does not exist"
		$medtrace target [$ns set nullAgent_]
	}
	
	$self mon-edrop-trace $medtrace
	
}

Delayer instproc init {} {
    $self next
}

Simulator instproc insert-delayer {src dst delayer} {
    set link [$self link $src $dst]
    $link insert-delayer $delayer
}

SimpleLink instproc insert-delayer args  {
    $self instvar delayer_ queue_
    if {[info exists delayer_]} {
        puts stderr "Delayer already inserted"
    } else {
        set delayer_ [lindex $args 0]
	$delayer_ target [$queue_ target]
        $queue_ target $delayer_
    }
}

# XCP queue consists of underlying virtual queues, for xcp, tcp and other flows. 
Queue/XCP instproc init {} {
	$self next
	$self create-vqueues
}

Queue/XCP instproc create-vqueues {} {
	$self instvar vq_ limit_

	# create virtual queues for xcp/tcp flows
	$self set-xcpQ [set vq_(0) [new Queue/DropTail/XCPQ]]
	$self set-tcpQ [set vq_(1) [new Queue/RED]]
	$self set-otherQ [set vq_(2) [new Queue/RED]]
}

Queue/XCP instproc link {del} {
	$self instvar vq_ 

	# XXX yuck
	#link info to RED queue
	$vq_(1) link $del
	$vq_(2) link $del

}

Queue/XCP instproc queue-limit { limit } {
	$self instvar vq_
	
	$vq_(0) set limit_ $limit
	$vq_(1) set limit_ $limit
	$vq_(2) set limit_ $limit
}

Queue/XCP instproc reset {} {
	$self instvar vq_

	$vq_(0) reset
	$vq_(1) reset
	$vq_(2) reset
}
