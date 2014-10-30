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
set opt(namtr)	""
set opt(seed)	0
set opt(stop)	20
set opt(node)	3

set opt(qsize)	100
set opt(bw)	2Mb
set opt(delay)	1ms
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Ca
set opt(chan)	Channel
set opt(tcp)	TCP/Reno
set opt(sink)	TCPSink

set opt(app)	FTP


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
	global env nshome pwd
	global ns opt trfd

	$ns flush-trace
	close $trfd
	foreach key {node bw delay ll ifq mac seed} {
		lappend comment $opt($key)
	}

	set force ""
	if {[info exists opt(f)] || [info exists opt(g)]} {
		set force "-f"
	}
	exec perl $nshome/bin/trsplit -tt r -pt tcp -c '$comment' \
			$force $opt(tr) >& $opt(tr)-bw
	exec head -n 1 $opt(tr)-bw >@ stdout

	if [info exists opt(g)] {
		catch "exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $opt(tr).*]] &"
	}
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

	set num $opt(node)
	for {set i 0} {$i <= $num} {incr i} {
		set node($i) [$ns node]
		lappend nodelist $node($i)
	}

	set lan [$ns newLan $nodelist $opt(bw) $opt(delay) \
			-llType $opt(ll) -ifqType $opt(ifq) \
			-macType $opt(mac) -chanType $opt(chan)]
}

proc create-source {} {
	global ns opt
	global lan node source

	set num $opt(node)
	for {set i 1} {$i <= $num} {incr i} {
		set src 0
		set dst $i
		set tp($i) [$ns create-connection $opt(tcp) \
				$node($src) $opt(sink) $node($dst) 0]
		set source($i) [$tp($i) attach-app $opt(app)]
		$ns at [expr $i/1000.0] "$source($i) start"
	}
}


## MAIN ##
Getopt
if {$opt(seed) >= 0} { ns-random $opt(seed) }
if [info exists opt(qsize)] { Queue set limit_ $opt(qsize) }

set ns [new Simulator]
set trfd [create-trace]

create-topology
create-source

$lan trace $ns $trfd

$ns at $opt(stop) "finish"
$ns run
