# tem.tcl - test emulation
# be careful here, as the tap agents will
# send out whatever they are given
#
set dotrace 1
set stoptime 20.0

set me [exec hostname]
set ns [new Simulator]

if { $dotrace } {
	set allchan [open em-all.tr w]
	$ns trace-all $allchan
	set namchan [open em.nam w]
	$ns namtrace-all $namchan
}

$ns use-scheduler RealTime

set livenet1 [new Network/Pcap/Live]
$livenet1 set promisc_ true

set livenet2 [new Network/Pcap/Live]
$livenet2 set promisc_ true

set nd1 [$livenet1 open] ;# open some appropriate device (devname is optional)
set nd2 [$livenet2 open] ;# open some appropriate device (devname is optional)
puts "ok, got network devices $nd1 and $nd2"

set f1len [$livenet1 filter "icmp and src $me"]
set f2len [$livenet2 filter "icmp and dst $me"]

puts "filter 1 len: $f1len, filter 2 len: $f2len"
puts "link1 address: [$livenet1 linkaddr]"
puts "link2 address: [$livenet2 linkaddr]"

set a1 [new Agent/Tap]
set a2 [new Agent/Tap]
puts "install nets into taps..."
$a1 network $livenet1
#############$a2 network $livenet2

set node0 [$ns node]
set node1 [$ns node]

$ns duplex-link $node0 $node1 8Mb 5ms DropTail
#
# attach-agent sets "target" of the agents to the node entrypoint
$ns attach-agent $node0 $a1
$ns attach-agent $node1 $a2

# let's detach the things
set null [$ns set nullAgent_]
###$a1 target $null
###$a2 target $null

$ns connect $a1 $a2

puts "scheduling termination at t=$stoptime"
$ns at $stoptime "exit 0"

puts "let's rock"
$ns run
