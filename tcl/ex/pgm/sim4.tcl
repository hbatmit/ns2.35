# Pragmatic General Multicast (PGM), Reliable Multicast
#
# Example to demonstrate multiple senders, and that an Agent/PGM can also
# act as an Agent/PGM/Receiver.
#
# This is a random topology with two senders and several receivers in
# two multicast groups.  The topology can be found in the file r10-0.tcl.
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

# Used to provide backward compatability with older version of ns.
Simulator instproc duplex-link-of-interfaces {args} {
  # Extract arguments.
  set n1 [lindex $args 0]
  set n2 [lindex $args 1]
  set BW [lindex $args 2]
  set delay [lindex $args 3]
  set queue [lindex $args 4]

  # Execute correct link instruction.
  $self duplex-link $n1 $n2 $BW $delay $queue
}

source r10-0.tcl

set verbose 0

# Create the network topology.
create-nodes ns n
create-links ns n 1.5Mb

for {set k 0} { $k < 10 } { incr k } {
    $n($k) color "green"
}

# Create two multicast groups.
set group1 [Node allocaddr]
puts "Group1 addr: $group1"

set group2 [Node allocaddr]
puts "Group2 addr: $group2"


#
#Set routing protocol
#
set mproto DM
set mrthandle [$ns mrtproto $mproto {}]

set loss_module1 [new PGMErrorModel]
$loss_module1 drop-packet ODATA 10 2
$loss_module1 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module1 $n(0) $n(9)

$ns duplex-link-op $n(0) $n(9) color "red"

set loss_module2 [new PGMErrorModel]
# Drop the second ODATA packet that crosses this link every 10th cycle.
$loss_module2 drop-packet ODATA 10 2
$loss_module2 drop-target [$ns set nullAgent_]
$ns lossmodel $loss_module2 $n(2) $n(6)

$ns duplex-link-op $n(2) $n(6) color "red"

#
# Create Sender
#
set src1 [new Agent/PGM/Sender]
$ns attach-agent $n(2) $src1
$src1 set dst_addr_ $group1
$src1 set dst_port_ 0

$src1 set rdata_delay_ 40ms
$src1 set spm_interval_ 75ms

$n(2) color "magenta"

# Attach data source to sender.
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $src1
$cbr1 set rate_ 448Kb
$cbr1 set packetSize_ 210
$cbr1 set random_ 0

set src2 [new Agent/PGM/Sender]
$ns attach-agent $n(8) $src2
$src2 set dst_addr_ $group2
$src2 set dst_port_ 0

$src2 set rdata_delay_ 40ms
$src2 set spm_interval_ 75ms

$n(8) color "magenta"

# Attach data source to sender.
set cbr2 [new Application/Traffic/CBR]
$cbr2 attach-agent $src2
$cbr2 set rate_ 448Kb
$cbr2 set packetSize_ 210
$cbr2 set random_ 0

proc create-receiver {rcv i group} {
   upvar $rcv r

   global ns n

   set r [new Agent/PGM/Receiver]
   $ns attach-agent $n($i) $r
   $ns at 0.01 "$n($i) join-group $r $group"
   $n($i) color "blue"
}

#
# Create PGM receivers
#
create-receiver rcv1 1 $group1
create-receiver rcv5 5 $group1
create-receiver rcv7 7 $group1

create-receiver rcv3 3 $group2
create-receiver rcv4 4 $group2
create-receiver rcv6 6 $group2

$n(3) shape "square"
$n(4) shape "square"
$n(6) shape "square"

$ns at 0.3 "$src1 start-SPM"
$ns at 0.4 "$src2 start-SPM"
$ns at 0.5 "$cbr1 start"
$ns at 0.6 "$cbr2 start"
$ns at 0.9 "$cbr1 stop"
$ns at 1.0 "$cbr2 stop"
$ns at 1.1 "$src1 stop-SPM"
$ns at 1.2 "$src2 stop-SPM"
$ns at 1.5 "finish"

proc finish {} {
    global ns src1 src2 rcv1 rcv5 rcv7 rcv3 rcv4 rcv6 n

    puts "Simulation Finished"

    puts "src1:"
    $src1 print-stats

    puts "agent9:"
    set pgm_agent [$n(9) get-pgm]
    $pgm_agent print-stats
    
    puts "rcv1:"
    $rcv1 print-stats
    puts "rcv5:"
    $rcv5 print-stats
    puts "rcv7:"
    $rcv7 print-stats

    puts "src2:"
    $src2 print-stats
    puts "rcv3:"
    $rcv3 print-stats
    puts "rcv4:"
    $rcv4 print-stats
    puts "rcv6:"
    $rcv6 print-stats

#       puts "Starting Nam.."
#       exec nam out.nam &
    exit 0
}

$ns run
