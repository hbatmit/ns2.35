#
# emping.tcl
#
# test emulation facility by incorporating icmp echo reply responder
#
#
set dotrace 1
set stoptime 200.0
set owdelay 1000ms

set myaddr "10.11.12.13"
set ns [new Simulator]

if { $dotrace } {
	set allchan [open em-all.tr w]
	$ns trace-all $allchan
	set namchan [open em.nam w]
	$ns namtrace-all $namchan
}

$ns use-scheduler RealTime

#
# allocate BPF-type network objects and a raw-IP object
#
#set bpf1 [new Network/Pcap/Live]; #	used to read and write L2 info
#$bpf set promisc_ true
#set dev1 [$bpf1 open readwrite fxp0]

set bpf2 [new Network/Pcap/Live]; #	used to read IP info
$bpf2 set promisc_ true
set dev2 [$bpf2 open readonly fxp0]

set ipnet [new Network/IP];	 #	used to write IP pkts
$ipnet open writeonly

#
# try to filter out unwanted stuff like netbios pkts, dns, etc
#

$bpf2 filter "icmp and dst $myaddr"

set pfa1 [new Agent/Tap]
set pfa2 [new Agent/Tap]
set ipa [new Agent/Tap]
set echoagent [new Agent/PingResponder]

puts "install nets into taps..."

$pfa2 set fid_ 0
$ipa set fid_ 1

$pfa2 network $bpf2
$ipa network $ipnet

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

$ns simplex-link $node0 $node1 1Mb $owdelay DropTail
$ns simplex-link $node1 $node0 1Mb $owdelay DropTail
$ns simplex-link $node0 $node2 1Mb $owdelay DropTail
$ns simplex-link $node2 $node0 1Mb $owdelay DropTail
$ns simplex-link $node1 $node2 1Mb $owdelay DropTail
$ns simplex-link $node2 $node1 1Mb $owdelay DropTail

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
$ns attach-agent $node0 $pfa2; #	packet filter agent
$ns attach-agent $node1 $ipa; # ip agent (for sending)
$ns attach-agent $node2 $echoagent

$ns simplex-connect $pfa2 $echoagent
$ns simplex-connect $echoagent $ipa

puts "here we go.., listening for pings on addr $myaddr"

$ns run
