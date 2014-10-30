set stoptime 200.0 ; # simulation end time (seconds)
set owdelay 10ms ; # 1-way delay

set virtaddr "128.32.130.59"
#set website "169.229.60.105"
set website "128.32.130.33"
set ns [new Simulator] ; # create simulator object

$ns use-scheduler RealTime ; # specify the real-time sync'd scheduler
set bpf1 [new Network/Pcap/Live]; # live traffic -- read IP pkts
set bpf2 [new Network/Pcap/Live]; # live traffic -- read IP pkts
$bpf1 set promisc_ true          ; # use promiscuous mode
$bpf2 set promisc_ true          ; # use promiscuous mode
$bpf1 open readonly fxp0 ; # specify interface
$bpf2 open readonly fxp0 ; # specify interface

set ipnet [new Network/IP]      ; # live traffic -- write IP pkts
$ipnet open writeonly
set myrealaddr [$bpf1 netaddr]

$bpf1 filter "tcp and src $myrealaddr and dst $virtaddr"; #
$bpf2 filter "tcp and dst $virtaddr and not src $myrealaddr";

set pfa1 [new Agent/Tap]
set pfa2 [new Agent/Tap]
set ipa [new Agent/Tap]

set outgoingNAT [new Agent/NatAgent/TCPSrcDest]
set incomingNAT [new Agent/NatAgent/TCPSrcDest]

$outgoingNAT source $virtaddr
$outgoingNAT destination $website

$incomingNAT source $virtaddr
$incomingNAT destination $myrealaddr

$pfa1 network $bpf1       ; # associate pf net object w/tap agent
$pfa2 network $bpf2       ; # associate pf net object w/tap agent
$ipa network $ipnet     ; # associate ip net object w/tap agent

# create topology in simulator
set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]
$ns simplex-link $node0 $node2 100Mb $owdelay DropTail
$ns simplex-link $node2 $node1 100Mb $owdelay DropTail

# place agents in topology
$ns attach-agent $node0 $pfa1; #        packet filter agent
$ns attach-agent $node0 $pfa2; #        packet filter agent
$ns attach-agent $node1 $ipa; # ip agent (for sending)
$ns attach-agent $node2 $outgoingNAT
$ns attach-agent $node2 $incomingNAT
$ns simplex-connect $pfa1 $outgoingNAT
$ns simplex-connect $pfa2 $incomingNAT
$ns simplex-connect $outgoingNAT $ipa
$ns simplex-connect $incomingNAT $ipa

puts "[$outgoingNAT set dst_]"

$ns gen-map
$ns run ; # start emulation
