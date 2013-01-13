# Pragmatic General Multicast (PGM), Reliable Multicast
#
# Example to demonstrate Constrained NAK forwarding, NAK elmination,
# and NAK suppression.
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

makelinks 1.5Mb 10ms {
    { 0 1 }
    { 1 2 }
    { 1 3 }
    { 2 4 }
    { 3 8 }
    { 2 5 }
    { 2 6 }
    { 2 7 }
    { 3 9 }
    { 3 10 }
    { 3 11 }
}

$ns duplex-link-op $n(0) $n(1) color "red"

$ns duplex-link-op $n(0) $n(1) orient down
$ns duplex-link-op $n(1) $n(2) orient left-down
$ns duplex-link-op $n(1) $n(3) orient right-down
$ns duplex-link-op $n(2) $n(4) orient left
$ns duplex-link-op $n(3) $n(8) orient left

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
create-receiver rcv5 5 $group
create-receiver rcv6 6 $group
create-receiver rcv7 7 $group
create-receiver rcv8 8 $group
create-receiver rcv9 9 $group
create-receiver rcv10 10 $group
create-receiver rcv11 11 $group

# Disable Agent/PGM on receivers.
disable-pgm { 4 5 6 7 8 9 10 11 }

# Disable Agent/PGM on sender.
disable-pgm { 0 }

# Disable Agent/PGM on Node 2.
disable-pgm { 2 }
$n(2) color "black"

# Create a lossy link between 0 and 1.
set loss_module [new PGMErrorModel]
# Drop the third ODATA packet going on this link.
$loss_module drop-packet ODATA 50 3
$loss_module drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module $n(0) $n(1)

$ns at 0.2 "$src0 start-SPM"
$ns at 0.25 "$cbr0 start"
$ns at 0.4 "$cbr0 stop"
$ns at 0.45 "$src0 stop-SPM"
$ns at 0.5 "finish"

proc finish {} {

    exit 0
}

$ns run
