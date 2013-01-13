# miptest.tcl
#
# To show mobileIP activities.
# Modified from a SUN's original example script
#
# See the comments from SUN below:
#
# kwang: some explanations on how to use this limited implementation of
#        Mobile IP :
# 1. Look into "proc create-topology" below for some necessary commands
#    to activate Mobile IP.
# 2. Reset "Simulator instvar node_factory_" to create specialized
#    nodes (Node/MIPBS and Node/MIPMH); how they'll interact with
#    HierNode or ManualRtNode is unknown.
# 3. Make the mobile links "dynamic"; initially mark the links that are
#    not between MH and HA "down" to fool the route computer
# 4. Specify the links on which BS and MH should broadcast ads and agent
#    solicitations.
# 5. Tell Agent/MIPMH which its HA is.
# 6. For configurable parameters, see the constructors of these modules
#    for the "bind" statements; beacon period can be set by the statement
#    "<name of an Agent/MIPBS or Agent/MIPMH> beacon-period <beacon period>".
# some changes are made by Ya 2/99

set opt(tr)	out
set opt(namtr)	"miptest.nam"
set opt(seed)	0
set opt(stop)	20

set opt(qsize)	100
set opt(bw)	2Mb
set opt(delay)	1ms
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Ca
set opt(chan)	Channel
set opt(tcp)	TCP/Reno
set opt(sink)	TCPSink

set opt(source)	FTP


proc Usage {} {
	global opt argv0
	puts "Usage: $argv0 \[-stop sec\] \[-seed value\] \[-node numNodes\]"
	puts "\t\[-tr tracefile\] \[-g\]"
	puts "\t\[-ll lltype\] \[-ifq qtype\] \[-mac mactype\]"
	puts "\t\[-bw $opt(bw)] \[-delay $opt(delay)\]"
	exit 1
}

proc Getopt {} {
	global opt argc argv
	if {$argc == 0} Usage
	for {set i 0} {$i < $argc} {incr i} {
		set key [lindex $argv $i]
		if ![string match {-*} $key] continue
		set key [string range $key 1 end]
		set val [lindex $argv [incr i]]
		set opt($key) $val
		if [string match {-[A-z]*} $val] {
			incr i -1
			continue
		}
		switch $key {
			ll  { set opt($key) LL/$val }
			ifq { set opt($key) Queue/$val }
			mac { set opt($key) Mac/$val }
		}
	}
}


proc finish {} {
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
	global lan node source
	$ns instvar link_

	set node(s) [$ns node]
	set node(g) [$ns node]
	Simulator set node_factory_ Node/MIPBS
	set node(ha) [$ns node]
	$node(ha) shape "hexagon"
	set node(fa) [$ns node]
	$node(fa) shape "hexagon"
	Simulator set node_factory_ Node/MIPMH
	set node(mh) [$ns node]
	$node(mh) shape "circle"

	$ns duplex-link $node(s) $node(g) 2Mb 5ms DropTail 
	$ns duplex-link-op $node(s) $node(g) orient right
	$ns duplex-link $node(g) $node(ha) 2Mb 5ms DropTail
	$ns duplex-link-op $node(g) $node(ha) orient up
	$ns duplex-link $node(g) $node(fa) 2Mb 5ms DropTail
	$ns duplex-link-op $node(g) $node(fa) orient right-up

	$ns duplex-link $node(ha) $node(mh) 2Mb 5ms DropTail 
	$ns duplex-link-op $node(ha) $node(mh) orient right
	$ns duplex-link $node(fa) $node(mh) 4Mb 5ms DropTail 
	$ns duplex-link-op $node(fa) $node(mh) orient left

	set mhid [$node(mh) id]
	set haid [$node(ha) id]
	set faid [$node(fa) id]
	$link_($mhid:$faid) dynamic
	$link_($faid:$mhid) dynamic
	$link_($mhid:$faid) down
	$link_($faid:$mhid) down
	$link_($mhid:$haid) dynamic
	$link_($haid:$mhid) dynamic

	[$node(ha) set regagent_] add-ads-bcast-link $link_($haid:$mhid)
	[$node(fa) set regagent_] add-ads-bcast-link $link_($faid:$mhid)

	[$node(mh) set regagent_] set home_agent_ $haid
	[$node(mh) set regagent_] add-sol-bcast-link $link_($mhid:$haid)
	[$node(mh) set regagent_] add-sol-bcast-link $link_($mhid:$faid)

	$ns at 0.0 "$node(ha) label HomeAgent"
	$ns at 0.0 "$node(fa) label ForeignAgent"
	$ns at 0.0 "$node(mh) label MobileHost"

	$ns at 0.0 "$node(ha) color gold"
	$ns at 0.0 "$node(mh) color gold"

	$ns rtmodel-at 0.1 down $node(mh) $node(fa)

	set swtm [expr 3.0+($opt(stop)-3.0)/4.0]
	$ns rtmodel-at $swtm down $node(mh) $node(ha)
	$ns rtmodel-at $swtm up $node(mh) $node(fa)
	$ns at $swtm "$node(ha) color black"
	$ns at $swtm "$node(fa) color gold"


	set swtm [expr 3.0+($opt(stop)-3.0)/2.0]
	$ns rtmodel-at $swtm down $node(mh) $node(fa)
	$ns rtmodel-at $swtm up $node(mh) $node(ha)
	$ns at $swtm "$node(ha) color gold"
	$ns at $swtm "$node(fa) color black"


	set swtm [expr 3.0+($opt(stop)-3.0)*3.0/4.0]
	$ns rtmodel-at $swtm down $node(mh) $node(ha)
	$ns rtmodel-at $swtm up $node(mh) $node(fa)
	$ns at $swtm "$node(ha) color black"
	$ns at $swtm "$node(fa) color gold"

#	$ns at $swtm "$link_($mhid:$faid) up"
#	$ns at $swtm "$link_($faid:$mhid) up"
#	$ns at [expr $swtm - 1.5] "$link_($mhid:$haid) down"
#	$ns at [expr $swtm - 1.5] "$link_($haid:$mhid) down"

#	set swtm [expr 3.0+($opt(stop)-3.0)/2.0]
#	$ns at $swtm "$link_($mhid:$haid) up"
#	$ns at $swtm "$link_($haid:$mhid) up"
#	$ns at [expr $swtm - 1.5] "$link_($mhid:$faid) down"
#	$ns at [expr $swtm - 1.5] "$link_($faid:$mhid) down"

#	set swtm [expr 3.0+($opt(stop)-3.0)*3.0/4.0]
#	$ns at $swtm "$link_($mhid:$faid) up"
#	$ns at $swtm "$link_($faid:$mhid) up"
#	$ns at [expr $swtm - 1.5] "$link_($mhid:$haid) down"
#	$ns at [expr $swtm - 1.5] "$link_($haid:$mhid) down"
}

proc create-source {} {
	global ns opt
	global lan node source

	set tp [$ns create-connection $opt(tcp) \
		    $node(s) $opt(sink) $node(mh) 0]
	set source [$tp attach-source $opt(source)]
	$ns at 0.0 "$source start"
}


## MAIN ##
Getopt
if {$opt(seed) >= 0} { ns-random $opt(seed) }
if [info exists opt(qsize)] { Queue set limit_ $opt(qsize) }

set ns [new Simulator]
set trfd [create-trace]

create-topology
create-source

#$lan trace $ns $trfd

$ns at $opt(stop) "finish"
$ns run
