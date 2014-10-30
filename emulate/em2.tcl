# em2.tcl - test emulation
# be careful here, as the tap agents will
# send out whatever they are given
#
set dotrace 1
set stoptime 200.0

set me "10.0.1.1"
set ns [new Simulator]

if { $dotrace } {
	set allchan [open em-all.tr w]
	$ns trace-all $allchan
	set namchan [open em.nam w]
	$ns namtrace-all $namchan
}

$ns use-scheduler RealTime

#
# we want the test machine to have ip forwarding disabled, so
# check this
#

set ipforw [exec sysctl -n net.inet.ip.forwarding]
if { $ipforw } {
	puts "can not run with ip forwarding enabled"
	exit 1
}

#
# allocate a BPF type network object and a raw-IP object
#
set bpf0 [new Network/Pcap/Live]
set bpf1 [new Network/Pcap/Live]
$bpf0 set promisc_ true
$bpf1 set promisc_ true

set ipnet [new Network/IP]

set nd0 [$bpf0 open readonly fxp0]
set nd1 [$bpf1 open readonly fxp1]
$ipnet open writeonly

puts "bpf0($bpf0) on dev $nd0, bpf1($bpf1) on dev $nd1, ipnet is $ipnet"

#
# try to filter out wierd stuff like netbios pkts, arp requests, dns, etc
#
set notme "(not ip host $me)"
set notbcast "(not ether broadcast)"
set ftp "and port ftp-data"
set f0len [$bpf0 filter "(ip dst host bit) and $notme and $notbcast"]
set f1len [$bpf1 filter "(ip src host bit) and $notme and $notbcast"]

puts "filter lengths: $f0len (bpf0), $f1len (bpf1)"
puts "dev $nd0 has address [$bpf0 linkaddr]"
puts "dev $nd1 has address [$bpf1 linkaddr]"

set a0 [new Agent/Tap]
set a1 [new Agent/Tap]
set a2 [new Agent/Tap]
puts "install nets into taps..."
$a0 network $bpf0
$a1 network $bpf1
$a2 network $ipnet

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

puts "node0: $node0 (id:[$node0 id]), node1: $node1 (id:[$node1 id]), node2: $node2 (id: [$node2 id])"

$ns simplex-link $node0 $node2 8Mb 100ms DropTail
$ns simplex-link $node1 $node2 8Mb 100ms DropTail

#
# attach-agent winds up calling $node attach $agent which does
# these things:
#	append agent to list of agents in the node
#	sets target in the agent to the entry of the node
#	sets the node_ field of the agent to the node
#	if not yet created,
#		create port demuxer in node (Addr classifier),
#		put in dmux_
#		call "$self add-route $id_ $dmux_"
#			installs id<->dmux mapping in classifier_
#	allocate a port
#	set agent's port id and address
#	install port-agent mapping in dmux_
#	
#	
$ns attach-agent $node0 $a0
$ns attach-agent $node1 $a1
$ns attach-agent $node2 $a2

$ns connect $a0 $a2
$ns connect $a1 $a2

#puts "scheduling termination at t=$stoptime"
#$ns at $stoptime "exit 0"

puts "let's rock"
$ns run
