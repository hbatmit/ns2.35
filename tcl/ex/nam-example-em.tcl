#
# example of new ns support for nam trace, adapted from Kannan's srm2.tcl
#
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/nam-example-em.tcl,v 1.3 1999/09/09 03:29:44 salehi Exp $
#

if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
	set prog $argv0
}

ns-random 1
set ns [new Simulator -multicast on]
Node expandaddr

#$ns trace-all [open out.tr w]
$ns namtrace-all [open out.nam w]

# define color index
$ns color 0 red
$ns color 1 blue
$ns color 2 chocolate
$ns color 3 red
$ns color 4 brown
$ns color 5 tan
$ns color 6 gold
$ns color 7 black

# create node with given shape and color
set n(0) [$ns node]
$n(0) color "red" 
$n(0) shape "square"
set n(1) [$ns node]
$n(1) color "blue" 
$n(1) shape "circle"
set n(2) [$ns node]
$n(2) color "blue" 
$n(2) shape "circle"

# create links and layout
$ns duplex-link $n(0) $n(1) 1.5Mb 10ms DropTail
$ns duplex-link-op $n(0) $n(1) orient right-down
$ns duplex-link $n(0) $n(2) 1.5Mb 10ms DropTail
$ns duplex-link-op $n(0) $n(2) orient left-down

# set routing
set group [Node allocaddr]
set cmc [$ns mrtproto CtrMcast {}]
$ns at 0.3 "$cmc switch-treetype $group"

# set group members
set udp0 [new Agent/UDP]
$ns attach-agent $n(0) $udp0
$udp0 set dst_addr_ $group
$udp0 set dst_port_ 0

# set traffic source
set packetSize 800
set s [new Application/Traffic/CBR]
$s set packetSize_ $packetSize
$s set interval_ 0.5
$s attach-agent $udp0

set rcvr0 [new Agent/LossMonitor]
$ns attach-agent $n(1) $rcvr0 
set rcvr1 [new Agent/LossMonitor]
$ns attach-agent $n(2) $rcvr1

set em0 [new SelectErrorModel]
$em0 drop-packet 2 2 1 ;# Drop 1 CBR out of 2
$ns lossmodel $em0 $n(0) $n(1)

$ns at 1.0 "$n(0) join-group $udp0 $group"
$ns at 1.0 "$n(1) join-group $rcvr0 $group"
$ns at 1.5 "$n(2) join-group $rcvr1 $group"

$ns at 1.0 "$s start"

$ns at 0.1 "$n(0) label CBR"
$ns at 0.1 "$n(1) label rcvr0"
$ns at 0.1 "$n(2) label rcvr1"

$ns at 10.0 "finish"

proc finish {} {
    global s 
    $s stop

    global ns 
    $ns flush-trace		;# NB>  Did not really close out.tr...:-)

    global argv0
    if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
    } else {
	set prog $argv0
    }

    puts "running nam..."
    exec nam -f dynamic-nam.conf out.nam &
    exit 0
}

$ns at 0.0 "$ns set-animation-rate 0.1ms"
$ns run
