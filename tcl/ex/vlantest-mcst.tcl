source ../lan/vlan.tcl

set opt(tr)	out
set opt(namtr)	"vlantest-mcst.nam"
set opt(seed)	0
set opt(stop)	0.2
set opt(node)	3

set opt(qsize)	100y
set opt(bw)	20Mb
set opt(delay)	1ms
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Cd
set opt(chan)	Channel
set opt(tcp)	TCP/Reno
set opt(sink)	TCPSink

set opt(source)	FTP


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
	global lan node source node0 nodex nodey

	set num $opt(node)
	for {set i 0} {$i < $num} {incr i} {
		set node($i) [$ns node]
		lappend nodelist $node($i)
	}

	set lan [$ns newLan $nodelist $opt(bw) $opt(delay) \
			-llType $opt(ll) -ifqType $opt(ifq) \
			-macType $opt(mac) -chanType $opt(chan)]
	#$lan addNode $nodelist $opt(bw) $opt(delay)

	set node0 [$ns node]
	$ns duplex-link $node0 $node(1) 5Mb 2ms DropTail
	$ns duplex-link-op $node0 $node(1) orient right
	set nodex [$ns node]
	$ns duplex-link $nodex $node(2) 5Mb 2ms DropTail
	$ns duplex-link-op $nodex $node(2) orient left
	set nodey [$ns node]
	$ns duplex-link $nodey $node(0) 5Mb 2ms DropTail
	$ns duplex-link-op $nodey $node(0) orient down
}

## MAIN ##

set ns [new Simulator -multicast on]
$ns color 2 black
$ns color 1 blue
$ns color 0 red
$ns color 30 purple
$ns color 31 green

set trfd [create-trace]

create-topology

set mproto DM
set mrthandle [$ns mrtproto $mproto  {}]
set group [Node allocaddr]

set udp0 [new Agent/UDP]
$ns attach-agent $node0 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0

set rcvrx [new Agent/Null]
$ns attach-agent $nodex $rcvrx
set rcvry [new Agent/Null]
$ns attach-agent $nodey $rcvry

$udp0 set dst_ $group
$cbr0 set interval_ 0.01

$ns at 0.0 "$nodex join-group $rcvrx $group"
$ns at 0.0 "$nodey join-group $rcvry $group"
$ns at 0.1 "$cbr0 start"
$ns at $opt(stop) "finish"
$ns run
