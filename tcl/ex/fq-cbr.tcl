#
# This file contains a preliminary cut at fair-queueing for ns
# as well as a number of stubs for Homework 3 in CS268.
#
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/fq-cbr.tcl,v 1.8 1998/09/01 17:02:48 tomh Exp $
#
# updated 8/5/98 by kfall to use hash classifier
#

set ns [new Simulator]

# override built-in link allocator
$ns proc simplex-link { n1 n2 bw delay type } {

	$self instvar link_ queueMap_ nullAgent_
	$self instvar traceAllFile_
	set sid [$n1 id]
	set did [$n2 id]
	if [info exists queueMap_($type)] {
		set type $queueMap_($type)
	}
	if { $type == "FQ" } {
		set link_($sid:$did) [new FQLink $n1 $n2 $bw $delay $nullAgent_]
	} else {
		set q [new Queue/$type]
		$q drop-target $nullAgent_
		set link_($sid:$did) [new SimpleLink $n1 $n2 $bw $delay $q]
	}

	$n1 add-neighbor $n2

	#XXX yuck
	if { $type == "RED" } {
	 	set bw [[$link_($sid:$did) set link_] set bandwidth_]
		$q set ptc_ [expr $bw / (8. * [$q set meanPacketSize_])]
	}
	if [info exists traceAllFile_] {
		$self trace-queue $n1 $n2 $traceAllFile_
	}

}

Class Classifier/Hash/Fid/FQ -superclass Classifier/Hash/Fid
     
Classifier/Hash/Fid/FQ instproc unknown-flow { src dst fid buck } {
        $self instvar fq_
        $fq_ new-flow $src $dst $fid
}    

Class FQLink -superclass Link

FQLink instproc init { src dst bw delay nullAgent } {
	$self next $src $dst
	$self instvar link_ queue_ head_ toNode_ ttl_ classifier_ \
		nactive_ drpT_ drophead_
	set drpT_ $nullAgent
	set nactive_ 0
	set queue_ [new Queue/FQ]
	set link_ [new DelayLink]
	$link_ set bandwidth_ $bw
	$link_ set delay_ $delay

	set classifier_ [new Classifier/Hash/Fid/FQ 33]
	$classifier_ set fq_ $self

	$queue_ target $link_
	$queue_ drop-target $nullAgent
	$link_ target [$toNode_ entry]

	set head_ $classifier_

	set drophead_ [new Connector]
	$drophead_ target [[Simulator instance] set nullAgent_]

	# XXX
	# put the ttl checker after the delay
	# so we don't have to worry about accounting
	# for ttl-drops within the trace and/or monitor
	# fabric
	#
	set ttl_ [new TTLChecker]
	$ttl_ target [$link_ target]
	$link_ target $ttl_

	$queue_ set secsPerByte_ [expr 8.0 / [$link_ set bandwidth_]]
}

Queue set limit_ 10

FQLink set queueManagement_ RED
FQLink set queueManagement_ DropTail

FQLink instproc new-flow { src dst fid } {
	$self instvar classifier_ nactive_ queue_ link_ drpT_
	incr nactive_

	set type [$class set queueManagement_]
	set q [new Queue/$type]

	#XXX yuck
	if { $type == "RED" } {
	 	set bw [$link_ set bandwidth_]
		$q set ptc_ [expr $bw / (8. * [$q set meanPacketSize_])]
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
# should be called after SimpleLink::trace
#
FQLink instproc nam-trace { ns f } {
	$self instvar enqT_ deqT_ drpT_ rcvT_ dynT_

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
		#XXX 
		# we use enqT_ as a flag of whether tracing has been
		# initialized
		$self trace $ns $f "nam"
	}
}

#
# Support for link tracing
# XXX only SimpleLink (and its children) can dump nam config, because Link
# doesn't have bandwidth and delay.
#
FQLink instproc dump-namconfig {} {
	# make a duplex link in nam
	$self instvar link_ attr_ fromNode_ toNode_

	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}
	if ![info exists attr_(ORIENTATION)] {
		set attr_(ORIENTATION) ""
	}

	set ns [Simulator instance]
	set bw [$link_ set bandwidth_]
	set delay [$link_ set delay_]

	$ns puts-nam-config \
		"l -t * -s [$fromNode_ id] -d [$toNode_ id] -S UP -r $bw -D $delay -o $attr_(ORIENTATION)"
}

FQLink instproc dump-nam-queueconfig {} {
	$self instvar attr_ fromNode_ toNode_

	set ns [Simulator instance]
	if [info exists attr_(QUEUE_POS)] {
		$ns puts-nam-config "q -t * -s [$fromNode_ id] -d [$toNode_ id] -a $attr_(QUEUE_POS)"
	} else {
		set attr_(QUEUE_POS) ""
	}
}

#
# Build trace objects for this link and
# update the object linkage
#
# create nam trace files if op == "nam"
#
FQLink instproc trace { ns f {op ""} } {
	$self instvar enqT_ deqT_ drpT_ queue_ link_ head_ fromNode_ toNode_
	$self instvar rcvT_ ttl_
	$self instvar drophead_		;# idea stolen from CBQ and Kevin

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

	#$enqT_ target $head_
	#set head_ $enqT_       -> replaced by the following
        if { [$head_ info class] == "networkinterface" } {
	    $enqT_ target [$head_ target]
	    $head_ target $enqT_
	    # puts "head is i/f"
        } else {
	    $enqT_ target $head_
	    set head_ $enqT_
	    # puts "head is not i/f"
	}

	# put recv trace after ttl checking, so that only actually 
	# received packets are recorded
	$rcvT_ target [$ttl_ target]
	$ttl_ target $rcvT_

	$self instvar dynamics_
	if [info exists dynamics_] {
		$self trace-dynamics $ns $f $op
	}
}

#
# Insert objects that allow us to monitor the queue size
# of this link.  Return the name of the object that
# can be queried to determine the average queue size.
#
FQLink instproc init-monitor ns {
	puts stderr "FQLink::init-monitor not implemented"
}

#Queue/RED set thresh_ 3
#Queue/RED set maxthresh_ 8

proc build_topology { ns which } {
        $ns color 1 red
        $ns color 2 white

	foreach i "0 1 2 3" {
		global n$i
		set tmp [$ns node]
		set n$i $tmp
	}
	$ns duplex-link $n0 $n2 5Mb 2ms DropTail
	$ns duplex-link $n1 $n2 5Mb 2ms DropTail
	$ns duplex-link-op $n0 $n2 orient right-down
	$ns duplex-link-op $n1 $n2 orient right-up

	if { $which == "FIFO" } {
		$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail
	} elseif { $which == "RED" } {
		$ns duplex-link $n2 $n3 1.5Mb 10ms RED
	} else {
		$ns duplex-link $n2 $n3 1.5Mb 10ms FQ
	}
	$ns duplex-link-op $n2 $n3 orient right
	$ns duplex-link-op $n2 $n3 queuePos 0.5
}

proc build_cbr { from to startTime interval } {
	global ns
	set udp0 [new Agent/UDP]
	set src [new Application/Traffic/CBR]
	$src set packet-size 800
	$src set rate [expr 800 * 8/ [$ns delay_parse $interval]] 
	set sink [new Agent/Null]
	$ns attach-agent $from $udp0
	$src attach-agent $udp0
	$ns attach-agent $to $sink
	$ns connect $udp0 $sink
	$ns at $startTime "$src start"
	return $udp0
}

set f [open out.tr w]
$ns trace-all $f

build_topology $ns FQ

set cbr1 [build_cbr $n0 $n3 0.1 4ms]
$cbr1 set class_ 1
set cbr2 [build_cbr $n1 $n3 0.11 8ms]
$cbr2 set class_ 2

#$ns at 40.0 "finish Output"
$ns at 4.0 "xfinish"

proc xfinish {} {
	global ns f
	$ns flush-trace
	close $f

	#XXX
	puts "running nam..."
	exec nam out.nam &
	exit 0
}

$ns run

