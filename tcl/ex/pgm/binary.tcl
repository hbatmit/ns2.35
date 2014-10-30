# Pragmatic General Multicast (PGM), Reliable Multicast
#
# Example script to demonstrate the capabilities of the PGM implementation.
#
# This is a binary tree with one sender and several receivers at the leaf
# nodes.
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

set height 3
set nodeNum [expr 1 << $height]
set linkNum [expr $nodeNum - 1]
set rcvrNum [expr 1 << [expr $height - 1]]
puts "Height $height nodes $nodeNum rcvrs $rcvrNum"

set colors { red black white blue yellow LightBlue green magenta orange }

#
# Create multicast group
#
set group [Node allocaddr]
puts "Group addr: $group"

# Create source node.
set n(0) [$ns node]
$n(0) shape "circle"
$n(0) color "red"

#
# Create nodes
#
for {set k 1} {$k < $rcvrNum} {incr k} {
        set n($k) [$ns node]
        $n($k) shape "circle"
}

for {set k $rcvrNum} {$k < $nodeNum} {incr k} {
        set n($k) [$ns node]
        $n($k) shape "circle"
        $n($k) color "blue"
}

#set n(16) [$ns node]
#$n(16) shape "circle"
#$n(16) color "blue"

#set n(17) [$ns node]
#$n(17) shape "circle"
#$n(17) color "blue"

proc makelinks { bw delay pairs } {
        global ns n
        foreach p $pairs {
                set src $n([lindex $p 0])
                set dst $n([lindex $p 1])
                $ns duplex-link $src $dst $bw $delay DropTail
        }
}

makelinks 1.5Mb 10ms {
{ 0 1 }
{ 1 2 }
{ 1 3 }
{ 2 4 }
{ 2 5 }
{ 3 6 }
{ 3 7 }
}

#
#Set routing protocol
#
set mproto DM
set mrthandle [$ns mrtproto $mproto {}]

#
# Create loss modules - Cycle is 10,
# loss module i drops (i*5)th packet in the cycle
#
for {set i 0} {$i <= $linkNum} {incr i} {
       set loss_module($i) [new PGMErrorModel]
       # Drop second ODATA packet every 10 packets.
       $loss_module($i) drop-packet ODATA 10 2
       $loss_module($i) drop-target [$ns set nullAgent_]
}

proc insert-loss pairs {
        global ns n loss_module
        set i 0
        foreach p $pairs {
                set src $n([lindex $p 0])
                set dst $n([lindex $p 1])
                $ns lossmodel $loss_module($i) $src $dst
                incr i
        }
}

#
# Insert a loss module
#
insert-loss {
    { 2 4 }
    { 2 5 }
}

#
# Create Sender
#
set src [new Agent/PGM/Sender]
$ns attach-agent $n(0) $src
$src set dst_addr_ $group
$src set dst_port_ 0

# Attach data source to sender.
set cbr [new Application/Traffic/CBR]
$cbr attach-agent $src
$cbr set rate_ 448Kb
$cbr set packetSize_ 210
$cbr set random_ 0

#
# Create PGM receivers
#
set i $rcvrNum
for {set k 0} {$k < $rcvrNum} {incr k} {
        set rcv($k) [new Agent/PGM/Receiver]
        $ns attach-agent $n($i) $rcv($k)
        $ns at 0.01 "$n($i) join-group $rcv($k) $group"
        incr i
}

#set rcv(8) [new Agent/PGM/Receiver]
#$ns attach-agent $n(16) $rcv(8)
#$ns at 0.01 "$n(16) join-group $rcv(8) $group"

#set rcv(9) [new Agent/PGM/Receiver]
#$ns attach-agent $n(17) $rcv(9)
#$ns at 0.01 "$n(17) join-group $rcv(9) $group"

# Set Node-2 PGM Agent to be disabled, only multicast.
#set agent2 [$n(2) get-pgm]
#$agent2 set pgm_enabled_ 0

#set agent4 [$n(4) get-pgm]
#$agent4 set pgm_enabled_ 0

$ns at 0.3 "$src start-SPM"
$ns at 0.5 "$cbr start"
$ns at 1.5 "$cbr stop"
$ns at 2.0 "$src stop-SPM"
$ns at 2.0 "finish"

proc finish {} {
    global ns src rcv rcvrNum nodeNum n

    $src print-stats

# Print statistics for each PGM Agent.
    for {set k 0} {$k < $nodeNum} {incr k} {
	set pgm_agent [$n($k) get-pgm]
	$pgm_agent print-stats
    }

# Print statistics for each receiver.
    for {set k 0} {$k < $rcvrNum} {incr k} {
	$rcv($k) print-stats
    }
#$rcv(1) print-all-stats
    puts "Simulation Finished"
#       puts "Starting Nam.."
#       exec nam out.nam &
    exit 0
}

$ns run
