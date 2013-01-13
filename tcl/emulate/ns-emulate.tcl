#
# ns-emulate.tcl
#	defaults for nse [ns with emulation support]
#
if [TclObject is-class Network/Pcap/Live] {
	Network/Pcap/Live set snaplen_ 4096;# bpf snap len
	Network/Pcap/Live set promisc_ false;
	Network/Pcap/Live set timeout_ 0
	Network/Pcap/Live set optimize_ true;# bpf code optimizer
	Network/Pcap/Live set offset_ 0.0; # 

	Network/Pcap/File set offset_ 0.0; # ts for 1st pkt in trace file
}

if [TclObject is-class Agent/Tap] {
    Agent/Tap set maxpkt_ 1600
}

if [TclObject is-class Agent/TCPTap] {
    Agent/TCPTap set maxpkt_ 1600
}

if [TclObject is-class Agent/IcmpAgent] {
    Agent/IcmpAgent set ttl_ 254
}

if [TclObject is-class Agent/IPTap] {
    Agent/IPTap set maxpkt_ 1600
}

if [TclObject is-class ArpAgent] {

	ArpAgent set cachesize_ 10; # entries in arp cache
	ArpAgent instproc init {} {
		$self next
	}

	ArpAgent instproc config ifname {
		$self instvar net_ myether_ myip_
		set net_ [new Network/Pcap/Live]
		$net_ open readwrite $ifname
		set myether_ [$net_ linkaddr]
		set myip_ [$net_ netaddr]
		$net_ filter "arp and (not ether src $myether_) and \
			(ether dst $myether_ \
			or ether dst ff:ff:ff:ff:ff:ff)"
		$self cmd network $net_
		$self cmd myether $myether_
		$self cmd myip $myip_
	}
}

