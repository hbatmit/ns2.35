# Pragmatic General Multicast (PGM), Reliable Multicast
#
# Example to demonstrate constrained RDATA forwarding and NAK anticipation.
#
# Ryan S. Barnett, 2001
# rbarnett@catarina.usc.edu

set ns [new Simulator -multicast on]
$ns node-config -PGM ON
$ns namtrace-all [open out.nam w]

$ns color 0 red
$ns color 1 black
$ns color 2 white
$ns color 3 blue
$ns color 4 yellow
$ns color 5 LightBlue
$ns color 6 green
$ns color 7 magenta
$ns color 8 orange

set colors { red black white blue yellow LightBlue green magenta orange }

set group [Node allocaddr]
puts "Group addr: $group"

proc makelinks { bw delay pairs } {
        global ns n
        foreach p $pairs {
                set src $n([lindex $p 0])
                set dst $n([lindex $p 1])
                $ns duplex-link $src $dst $bw $delay DropTail
        }
}

proc disable-pgm { pairs } {
    global n

    foreach p $pairs {
	set agent [$n($p) get-pgm]
	$agent set pgm_enabled_ 0
    }
}

proc create-receiver {rcv i group} {
   upvar $rcv r

   global ns n

   set r [new Agent/PGM/Receiver]
   $r set nak_bo_ivl_ 100ms
   $ns attach-agent $n($i) $r
   $ns at 0.01 "$n($i) join-group $r $group"

   $n($i) color "blue"
}

# Create nodes.
for {set k 0} { $k < 12 } { incr k } {
    set n($k) [$ns node]
    $n($k) shape "circle"
    $n($k) color "green"
}

# Create topology.
#lappend lanlist1 $n(2)
#lappend lanlist1 $n(5)
#lappend lanlist1 $n(6)
#lappend lanlist1 $n(7)

#set lan1 [$ns newLan $lanlist1 10Mb 2ms]

#lappend lanlist2 $n(3)
#lappend lanlist2 $n(9)
#lappend lanlist2 $n(10)
#lappend lanlist2 $n(11)

#set lan2 [$ns newLan $lanlist2 10Mb 2ms]

makelinks 1.5Mb 10ms {
    { 0 1 }
    { 1 2 }
    { 1 3 }
    { 2 4 }
    { 2 5 }
    { 3 6 }
    { 3 7 }
    { 5 8 }
    { 5 9 }
    { 7 10 }
    { 7 11 }
}

$ns duplex-link-op $n(2) $n(5) color "red"
$ns duplex-link-op $n(1) $n(3) color "red"

# Set routing protocol.
set mproto DM
set mrthandle [$ns mrtproto $mproto {}]

# Create sender.
set src0 [new Agent/PGM/Sender]
$ns attach-agent $n(0) $src0
$src0 set dst_addr_ $group
$src0 set dst_port_ 0
$src0 set set rdata_delay_ 40ms

set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $src0
$cbr0 set rate_ 448Kb
$cbr0 set packetSize_ 210
$cbr0 set random_ 0

$n(0) color "magenta"

# Create receivers.
create-receiver rcv4 4 $group
create-receiver rcv6 6 $group
create-receiver rcv8 8 $group
create-receiver rcv9 9 $group
create-receiver rcv10 10 $group
create-receiver rcv11 11 $group

$rcv6 set nak_bo_ivl_ 10ms
$rcv8 set nak_bo_ivl_ 30ms
$rcv9 set nak_bo_ivl_ 30ms

# Disable Agent/PGM on receivers.
disable-pgm { 4 6 8 9 10 11 }

# Disable Agent/PGM on sender.
disable-pgm { 0 }

# Disable Agent/PGM on Node 3.
disable-pgm { 3 }
$n(3) color "black"

# Create a lossy link between 2 and 5.
set loss_module1 [new PGMErrorModel]
# Drop the third ODATA packet going through this link.
$loss_module1 drop-packet ODATA 50 3
$loss_module1 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module1 $n(2) $n(5)

# Create a lossy link between 1 and 3.
set loss_module2 [new PGMErrorModel]
# Drop the fifth ODATA packet going through this link.
$loss_module2 drop-packet ODATA 50 5
$loss_module2 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module2 $n(1) $n(3)

$ns at 0.2 "$src0 start-SPM"
$ns at 0.25 "$cbr0 start"
$ns at 0.5 "$cbr0 stop"
$ns at 0.55 "$src0 stop-SPM"
$ns at 0.7 "finish"

proc finish {} {

    exit 0
}

$ns run
