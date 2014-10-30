# Pragmatic General Multicast (PGM), Reliable Multicast
#
# Example to demonstrate NAK reliability for Agent/PGM as well as
# Agent/PGM/Receiver.
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
   $r set nak_bo_ivl_ 30ms
   $ns attach-agent $n($i) $r
   $ns at 0.01 "$n($i) join-group $r $group"

   $n($i) color "blue"
}

# Create nodes.
for {set k 0} { $k < 8 } { incr k } {
    set n($k) [$ns node]
    $n($k) shape "circle"
    $n($k) color "green"
}

makelinks 1.5Mb 10ms {
    { 0 1 }
    { 1 2 }
    { 1 3 }
    { 3 4 }
    { 3 5 }
    { 4 6 }
    { 5 7 }
}

$ns duplex-link-op $n(0) $n(1) color "red"
$ns duplex-link-op $n(1) $n(2) color "blue"
$ns duplex-link-op $n(3) $n(5) color "blue"
$ns duplex-link-op $n(1) $n(3) color "magenta"
$ns duplex-link-op $n(4) $n(6) color "magenta"

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
create-receiver rcv2 2 $group
create-receiver rcv6 6 $group
create-receiver rcv7 7 $group

# Disable Agent/PGM on receivers.
disable-pgm { 2 6 7 }

# Disable Agent/PGM on sender.
disable-pgm { 0 }

# Create a lossy link between 0 and 1.
set loss_module1 [new PGMErrorModel]
# Drop the fifth ODATA packet going through this link.
$loss_module1 drop-packet ODATA 100 5
$loss_module1 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module1 $n(0) $n(1)

# Create a lossy link between 2 and 1.
set loss_module2 [new PGMErrorModel]
# Drop the first NAK packet going from node 2 to node 1.
$loss_module2 drop-packet NAK 50 1
$loss_module2 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module2 $n(2) $n(1)

# Create a lossy link between 5 and 3.
set loss_module3 [new PGMErrorModel]
# Drop the first NAK packet going from node 2 to node 1.
$loss_module3 drop-packet NAK 50 1
$loss_module3 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module3 $n(5) $n(3)

# Create a lossy link between 1 and 3.
set loss_module4 [new PGMErrorModel]
# Drop the first NCF packet going from node 1 to node 3.
$loss_module4 drop-packet NCF 1 0
$loss_module4 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module4 $n(1) $n(3)

# Create a lossy link between 4 and 6.
set loss_module5 [new PGMErrorModel]
# Drop the first NCF packet going from node 4 to node 6.
$loss_module5 drop-packet NCF 50 1
$loss_module5 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module5 $n(4) $n(6)

$ns at 0.2 "$src0 start-SPM"
$ns at 0.25 "$cbr0 start"
$ns at 0.5 "$cbr0 stop"
$ns at 0.55 "$src0 stop-SPM"
$ns at 0.6 "finish"

proc finish {} {

    exit 0
}

$ns run
