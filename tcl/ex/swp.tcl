source timer.tcl

global ns
set ns [new Simulator]

$ns color 0 blue
$ns color 1 red
$ns color 2 darkgreen

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

puts n0=[$n0 id]
puts n1=[$n1 id]
puts n2=[$n2 id]
puts n3=[$n3 id]

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

#$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 10Mb 2ms DropTail
$ns duplex-link $n1 $n2 10Mb 2ms DropTail
$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n2 orient right-up
$ns duplex-link-op $n1 $n2 orient right-down
$ns duplex-link-op $n2 $n3 orient right

$ns duplex-link-op $n2 $n3 queuePos 0.5

$ns queue-limit $n2 $n3 8

Class Sender -superclass {Agent/Message Timer}
Class Receiver -superclass Agent/Message

Sender instproc init {} {
	$self next
	$self reset
	$self set window_ 4
}

Sender instproc reset {} {
	$self instvar seqno_ ack_
	set seqno_ 0
	set ack_ 0
}

Sender instproc start {} {
	$self transmit
}

Sender instproc reset_timer {} {
	$self cancel
	$self sched 0.05
}

Sender instproc timeout {} {
	$self instvar seqno_ ack_
	set seqno_ $ack_
	$self transmit
}

Sender instproc cansend {} {
	$self instvar seqno_ window_ ack_
	return [expr $seqno_ < $ack_ + $window_]
}

Sender instproc sendnext {} {
	$self instvar seqno_
	$self send $seqno_
	incr seqno_
	$self reset_timer
}

Sender instproc transmit {} {
	while [$self cansend] {
		$self sendnext
	}
}

Sender instproc recv msg {
	$self instvar ack_
	set ack_ $msg
	$self transmit
}

Sender set packetSize_ 400
Receiver set packetSize_ 40

Receiver instproc init {} {
	$self next
	$self reset
}

Receiver instproc reset {} {
	$self instvar ack_
	set ack_ 0
}

Receiver instproc recv msg {
	set seqno $msg
	$self instvar ack_
	if { $seqno == $ack_ } {
		incr ack_ 
	}
	$self send $ack_
}

set sndr [new Sender]
$ns attach-agent $n0 $sndr
$sndr set fid_ 0

set rcvr [new Receiver]
$ns attach-agent $n3 $rcvr
$rcvr set fid_ 1

$ns connect $sndr $rcvr

$sndr set window_ 1
# start up - stop-and-wait
$ns at 0.0 "$sndr start"
# do pipelining
$ns at 0.2 "$sndr set window_ 6"
# cause queue overflow
$ns at 0.4 "$sndr reset; $rcvr reset; $sndr set window_ 10"
$ns at 0.6 "$ns queue-limit $n2 $n3 20"
$ns at 0.8 "$sndr set window_ 14"

$ns at 1.0 "finish"

proc finish {} {
	global nf
	close $nf
	puts "running nam..."
	exec nam out.nam &
	exit 0
}

$ns run

