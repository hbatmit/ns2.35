#!/bin/sh
# the next line finds ns \
nshome=`dirname $0`; [ ! -x $nshome/ns ] && [ -x ../../ns ] && nshome=../..
# the next line starts ns \
export nshome; exec $nshome/ns "$0" "$@"

if [info exists env(nshome)] {
	set nshome $env(nshome)
} elseif [file executable ../../ns] {
	set nshome ../..
} elseif {[file executable ./ns] || [file executable ./ns.exe]} {
	set nshome "[pwd]"
} else {
	puts "$argv0 cannot find ns directory"
	exit 1
}
set env(PATH) "$nshome/bin:$env(PATH)"

set opt(tr)	out
set opt(namtr)	"lantest.nam"
set opt(seed)	0
set opt(stop)	5 
set opt(node)	8

set opt(qsize)	100
set opt(bw)	10Mb
set opt(delay)	1ms
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/802_3
set opt(chan)	Channel
set opt(tcp)	TCP/Reno
set opt(sink)	TCPSink

set opt(app)	FTP


proc finish {} {
	global env nshome pwd
	global ns opt trfd

	$ns flush-trace
	close $trfd

	exec ../../../nam-1/nam lantest.nam

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
	global lan node source node0

	set num $opt(node)
	for {set i 0} {$i < $num} {incr i} {
		set node($i) [$ns node]
		lappend nodelist $node($i)
	}

	set lan [$ns newLan $nodelist $opt(bw) $opt(delay) \
			-llType $opt(ll) -ifqType $opt(ifq) \
			-macType $opt(mac) -chanType $opt(chan)]

	set node0 [$ns node]
	$ns duplex-link $node0 $node(0) 2Mb 2ms DropTail

	$ns duplex-link-op $node0 $node(0) orient right

}

## MAIN ##

set ns [new Simulator]
set trfd [create-trace]

create-topology

set tcp0 [$ns create-connection TCP/Reno $node0 TCPSink $node(7) 0]
$tcp0 set window_ 15

set ftp0 [$tcp0 attach-app FTP]

$ns at 0.0 "$ftp0 start"

$ns at $opt(stop) "finish"
$ns run
