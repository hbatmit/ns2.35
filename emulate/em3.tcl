# em3.tcl - test emulation
# like em2, but now separate the ftp ctrl and data connections
# and mark them with different flow id's so that we can use flow-based
# dropping capabilities
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
set bpf0c [new Network/Pcap/Live]
set bpf0d [new Network/Pcap/Live]
set bpf1c [new Network/Pcap/Live]
set bpf1d [new Network/Pcap/Live]
$bpf0c set promisc_ true
$bpf0d set promisc_ true
$bpf1c set promisc_ true
$bpf1d set promisc_ true

set ipnet [new Network/IP]

set nd0c [$bpf0c open readonly fxp0]
set nd0d [$bpf0d open readonly fxp0]
set nd1c [$bpf1c open readonly fxp1]
set nd1d [$bpf1d open readonly fxp1]

$ipnet open writeonly

puts "bpf0c($bpf0c) on dev $nd0c, bpf1c($bpf1c) on dev $nd1c, ipnet is $ipnet"

#
# try to filter out wierd stuff like netbios pkts, arp requests, dns, etc
#
set notme "(not ip host $me)"
set notbcast "(not ether broadcast)"
set notftpdata "(not port ftp-data)"
set ftpctrl "(port ftp)"
set ftpdata "(port ftp-data)"

set 0cfilter "(ip dst host bit) and $notme and $notbcast and $ftpctrl"
set 0dfilter "(ip dst host bit) and $notme and $notbcast and $ftpdata"
set 1cfilter "(ip src host bit) and $notme and $notbcast and $ftpctrl"
set 1dfilter "(ip src host bit) and $notme and $notbcast and $ftpdata"

$bpf0c filter $0cfilter
$bpf0d filter $0dfilter
$bpf1c filter $1cfilter
$bpf1d filter $1dfilter

set a0c [new Agent/Tap]
set a0d [new Agent/Tap]
set a1c [new Agent/Tap]
set a1d [new Agent/Tap]

set a2 [new Agent/Tap]
puts "install nets into taps..."

$a0c set fid_ 0
$a0d set fid_ 1
$a1c set fid_ 2
$a1d set fid_ 3

$a2 set fid_ 4

$a0c network $bpf0c
$a0d network $bpf0d
$a1c network $bpf1c
$a1d network $bpf1d

$a2 network $ipnet

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

puts "node0: $node0 (id:[$node0 id]), node1: $node1 (id:[$node1 id]), node2: $node2 (id: [$node2 id])"

$ns simplex-link $node0 $node2 8Mb 10ms DropTail
$ns simplex-link $node1 $node2 8Mb 10ms DropTail

set lossylink [$ns link $node0 $node2]	; # main subnet interface
set em [new ErrorModule Fid]
set errmodel [new ErrorModel/Periodic]
$errmodel unit pkt
$errmodel set offset_ 1.0
$errmodel set period_ 10.0
$lossylink errormodule $em
$em insert $errmodel
$em bind $errmodel 1	; # drop pkts with fid 1 (ftp data)

set conn [new Connector]
$conn target [$errmodel target]
$conn drop-target [$errmodel target]
$em bind $conn 0	; # pass through fid 0 (ftp ctrl)

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
$ns attach-agent $node0 $a0c
$ns attach-agent $node0 $a0d
$ns attach-agent $node1 $a1c
$ns attach-agent $node1 $a1d

$ns attach-agent $node2 $a2

$ns connect $a0c $a2
$ns connect $a0d $a2
$ns connect $a1c $a2
$ns connect $a1d $a2

puts "here we go.."
$ns run
