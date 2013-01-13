set stoptime 200.0 ; # simulation end time (seconds)
set owdelay 1000ms ; # 1-way delay

#set myaddr "10.11.12.13"
set myaddr "128.32.130.59"
set ns [new Simulator] ; # create simulator object

$ns use-scheduler RealTime ; # specify the real-time sync'd scheduler
set bpf [new Network/Pcap/Live]; # live traffic -- read IP pkts
$bpf set promisc_ true          ; # use promiscuous mode
$bpf open readonly fxp0 ; # specify interface

set ipnet [new Network/IP]      ; # live traffic -- write IP pkts
$ipnet open writeonly

$bpf filter "icmp and dst $myaddr"; # only ICMP packets for me

set pfa [new Agent/Tap]
set ipa [new Agent/Tap]
set echoagent [new Agent/PingResponder]

$pfa network $bpf       ; # associate pf net object w/tap agent
$ipa network $ipnet     ; # associate ip net object w/tap agent

# create topology in simulator
set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]
$ns simplex-link $node0 $node2 100Mb $owdelay DropTail
$ns simplex-link $node2 $node1 100Mb $owdelay DropTail

# place agents in topology
$ns attach-agent $node0 $pfa; #        packet filter agent
$ns attach-agent $node1 $ipa; # ip agent (for sending)
$ns attach-agent $node2 $echoagent
$ns simplex-connect $pfa $echoagent
$ns simplex-connect $echoagent $ipa

puts "listening for pings on addr $myaddr..."
$ns run ; # start emulation
