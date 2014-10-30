puts "sourcing tcl/lan/vlan.tcl..."
source ../lan/vlan.tcl

set opt(tr)	out
set opt(namtr)	"vlantest-hier.nam"
set opt(seed)	0
set opt(stop)	.5
set opt(node)	3

set opt(qsize)	100
set opt(bw)	20Mb
set opt(delay)	1ms
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Cd
set opt(chan)	Channel
set opt(tcp)	TCP/Reno
set opt(sink)	TCPSink

set opt(app)	FTP


proc finish {} {
	global ns opt

	$ns flush-trace

	exec nam $opt(namtr) &

	exit 0
}


proc create-trace {} {
	global ns opt

	if [file exists $opt(tr)] {
		catch "exec rm -f $opt(tr) $opt(tr)-bw [glob $opt(tr).*]"
	}

	set trfd [open $opt(tr) w]
	$ns trace-all $trfd
	if {$opt(namtr) != ""} {
		$ns namtrace-all [open $opt(namtr) w]
	}
	return $trfd
}


proc create-topology {} {
	global ns opt
	global lan node source node0 nodex

	$ns set-address-format hierarchical
	set num $opt(node)

	AddrParams set domain_num_ 1
	lappend cluster_num 3
	AddrParams set cluster_num_ $cluster_num
	lappend eilastlevel [expr $num + 1] 1 1
	AddrParams set nodes_num_ $eilastlevel

	for {set i 0} {$i < $num} {incr i} {
		set node($i) [$ns node 0.0.[expr $i + 1]]
		lappend nodelist $node($i)
	}

	set lan [$ns newLan $nodelist $opt(bw) \
			$opt(delay) -llType $opt(ll) -ifqType $opt(ifq) \
			-macType $opt(mac) -chanType $opt(chan) -address "0.0.0"]
	$lan cost 2
	set node0 [$ns node "0.1.0"]
	$ns duplex-link $node0 $node(1) 20Mb 2ms DropTail
	$ns duplex-link-op $node0 $node(1) orient right
	set nodex [$ns node "0.2.0"]
	$ns duplex-link $nodex $node(2) 20Mb 2ms DropTail
	$ns duplex-link-op $nodex $node(2) orient left
}

## MAIN ##

set ns [new Simulator]
set trfd [create-trace]

create-topology

set tcp0 [$ns create-connection TCP/Reno $node0 TCPSink $nodex 0]
$tcp0 set window_ 15

set ftp0 [$tcp0 attach-app FTP]

#set udp0 [new Agent/UDP]
#$ns attach-agent $node0 $udp0
#set cbr0 [new Application/Traffic/CBR]
#$cbr0 attach-agent $udp0
#set rcvr0 [new Agent/Null]
#$ns attach-agent $nodex $rcvr0
#$udp0 set dst_ [$rcvr0 set addr_]

$ns at 0.0 "$ftp0 start"
#$ns at 0.0 "$cbr0 start"
$ns at $opt(stop) "finish"
$ns run
