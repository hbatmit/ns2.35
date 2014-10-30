#
# pingdemo.tcl
#
# demonstration of the emulation facilities using the icmp
# echo responder (also uses the arp responder)
#
# K Fall, 10/11/98
# kfall@cs.berkeley.edu
#
# Instructions:
#	1> first compile 'nse', the ns simulator with emulation
#	2> change the 'myname' var below to the name of some machine
#		that isn't being used right now,
#		and set ifname to the relevant interface
#	3> run this script (nse pingdemo.tcl)
#	4> run a 'ping' from some host to the name chosen in step 2
#	5> ping output should indicate the various functions exercised by
#		the tests below
#

Class PingDemo

PingDemo instproc init {} {
	$self next
	$self instvar myname myaddr dotrace stoptime owdelay ifname delay_list next_delay_list
	$self instvar traceallfile tracenamfile

	#
	# These parameters are system specific.
	#
	switch [exec hostname] {
	pollytto* {
		set myname "dash-w.isi.edu"
		set ifname cnw0
		set dotrace 0
	}
	kfalls-host* {
		set myname "bit.ee.lbl.gov"
		set ifname fxp0
		set dotrace 1
	}
	default {
		puts "error: running on an unknown host; edit PingDemo::init"
		exit 1
	}
	}

	set myaddr [exec host $myname | sed "s/.*address\ //"]
	set stoptime 200.0
	set owdelay 1000ms
	set delay_list {0.5 0.1 0.01 0.001}
	set next_delay_list ""

	set traceallfile em-all.tr
	set tracenamfile em.nam
	puts ""
}

PingDemo instproc syssetup {} {
	puts "turning off IP forwarding and ICMP redirect generation."
	exec sysctl -w net.inet.ip.forwarding=0 net.inet.ip.redirect=0
}

PingDemo instproc emsetup {} {
	$self instvar myname myaddr dotrace stoptime owdelay ifname
	$self instvar traceallfile tracenamfile ns

	puts "I am $myname with IP address $myaddr."

	set ns [new Simulator]

	if { $dotrace } {
		exec rm -f $traceallfile $tracenamfile
		set allchan [open $traceallfile w]
		$ns trace-all $allchan
		set namchan [open $tracenamfile w]
		$ns namtrace-all $namchan
	}

	$ns use-scheduler RealTime

	set bpf2 [new Network/Pcap/Live]; #	used to read IP info
	$bpf2 set promisc_ true
	set dev2 [$bpf2 open readonly $ifname]

	set ipnet [new Network/IP];	 #	used to write IP pkts
	$ipnet open writeonly

	set arpagent [new ArpAgent]
	$arpagent config $ifname
	set myether [$arpagent set myether_]
	puts "Arranging to proxy ARP for $myaddr on my ethernet $myether."
	$arpagent insert $myaddr $myether publish

	#
	# try to filter out unwanted stuff like netbios pkts, dns, etc
	#

	$bpf2 filter "icmp and dst $myaddr"

	set pfa1 [new Agent/Tap]
	set pfa2 [new Agent/Tap]
	set ipa [new Agent/Tap]
	set echoagent [new Agent/PingResponder]

	$pfa2 set fid_ 0
	$ipa set fid_ 1

	$pfa2 network $bpf2
	$ipa network $ipnet

	#
	# topology creation
	#

	set node0 [$ns node]
	set node1 [$ns node]
	set node2 [$ns node]

	$ns simplex-link $node0 $node1 1Mb $owdelay DropTail
	$ns simplex-link $node1 $node0 1Mb $owdelay DropTail
	$ns simplex-link $node0 $node2 1Mb $owdelay DropTail
	$ns simplex-link $node2 $node0 1Mb $owdelay DropTail
	$ns simplex-link $node1 $node2 1Mb $owdelay DropTail
	$ns simplex-link $node2 $node1 1Mb $owdelay DropTail

	$self instvar explink1 explink2; # link to experiment with
	set explink1 [$ns link $node0 $node2]
	set explink2 [$ns link $node2 $node1]

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
}

PingDemo instproc newowdelay delay {
	$self instvar explink1 explink2 ns owdelay
	set owdelay $delay
	set lnk [$explink1 link]
	puts "[$ns now]: change 1-way delay from [$lnk set delay_] to $delay sec."
	$lnk set delay_ $delay
	set lnk [$explink2 link]
	$lnk set delay_ $delay
}

# eternally cycle through the delays in delay_list
PingDemo instproc newowdelay_cycle {} {
	$self instvar delay_list next_delay_list
	if { "$next_delay_list" == "" } {
		set next_delay_list $delay_list
	}
	set next_delay [lindex $next_delay_list 0]
	set next_delay_list [lrange $next_delay_list 1]
	$self newowdelay $next_delay
	$ns at [expr [$ns now] + 10] "$self newowdelay_cycle"
}


PingDemo instproc run {} {

	$self instvar ns myaddr owdelay ifname explink
	$self syssetup
	$self emsetup

	puts "listening for pings on addr $myaddr, 1-way link delay: $owdelay\n"

	$ns at 10.5 "$self newowdelay_cycle"

	$ns run
}

PingDemo thisdemo
thisdemo run
