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
#source $nshome/tcl/lan/ns-lan.tcl
source $nshome/tcl/http/http.tcl

set opt(trsplit) "-"
set opt(tr)	out
set opt(namtr)	""
set opt(stop)	20
set opt(node)	3
set opt(seed)	0

set opt(bw)	1.8Mb
set opt(delay)	1ms
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Cd
set opt(chan)	Channel
set opt(tcp)	TCP/Reno
set opt(sink)	TCPSink

# number of traffic source (-1 means the number of nodes $opt(node))
set opt(telnet) 0
set opt(cbr)	0
set opt(ftp)	0
set opt(http)	-1

set opt(webModel) $nshome/tcl/http/data
set opt(maxConn) 4
set opt(phttp) 0

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop sec\] \[-seed value\] \[-node numNodes\]"
	puts "\t\[-tr tracefile\] \[-g\]"
	puts "\t\[-ll lltype\] \[-ifq qtype\] \[-mac mactype\]"
	puts "\t\[-bw $opt(bw)] \[-delay $opt(delay)\]"
	exit 1
}

proc getopt {argc argv} {
	global opt
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
	exec perl $nshome/bin/trsplit -tt r -pt tcp -c "$comment" \
			$opt(trsplit) $opt(tr) >& $opt(tr)-bw
	exec head -1 $opt(tr)-bw >@ stdout

	if [info exists opt(g)] {
		catch "exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $opt(tr).*]] &"
	}
	exit 0
}


proc create-trace {} {
	global ns opt

	if {[info exists opt(f)] || [info exists opt(g)]} {
		set opt(trsplit) "-f"
	}
	if [file exists $opt(tr)] {
		exec touch $opt(tr).
		eval exec rm -f $opt(tr) $opt(tr)-bw [glob $opt(tr).*]
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

	# Nodes:
	#  0		a router from LAN to external network
	#  1 to n	nodes on the LAN
	#  n+1		node outside of the LAN,
	set num $opt(node)
	set node(0)  [$ns node]
	for {set i 1} {$i <= $num} {incr i} {
		set node($i) [$ns node]
		lappend nodelist $node($i)
	}
	set node($i) [$ns node]
	$ns duplex-link $node(0) $node($i) 10Mb 50ms DropTail

	set lan [$ns newLan $nodelist $opt(bw) $opt(delay) -llType $opt(ll) \
			-ifqType $opt(ifq) -macType $opt(mac)]
	$lan addNode $node(0) $opt(bw) $opt(delay)

	set ifq0 [[$lan netIface $node(0)] set ifq_]
	$ifq0 set limit_ [expr $opt(node) * [$ifq0 set limit_]]
}


proc create-source {} {
	global ns opt
	global lan node source

	if {$opt(webModel) != ""} {
		create-webModel $opt(webModel)
	}
	# make node($num+1) the source of all connection
	set src $node(0)
	foreach ttype {http ftp cbr telnet} {
		set num $opt($ttype)
		if {$num < 0} {
			set num $opt(node)
		}
		for {set i 0} {$i < $num} {incr i} {
			set dst $node([expr 1 + $i % $opt(node)])
			new_$ttype $i $src $dst
		}
	}
}

proc create-webModel path {
	Http setCDF rvClientTime 0
#	Http setCDF rvClientTime $path/HttpThinkTime.cdf
	Http setCDF rvReqLen $path/HttpRequestLength.cdf
	Http setCDF rvNumImg $path/HttpConnections.cdf
	set rv [Http setCDF rvRepLen $path/HttpReplyLength.cdf]
	Http setCDF rvImgLen $rv
}

proc new_http {i server client} {
	global ns opt http webm
	set webopt "-srcType $opt(tcp) -snkType $opt(sink) -phttp $opt(phttp) -maxConn $opt(maxConn)"
	set http($i) [eval new Http $ns $client $server $webopt]
	$ns at [expr 0 + $i/1000.0] "$http($i) start"
}

proc new_cbr {i src dst} {
	global ns opt cbr
	set cbr($i) [$ns create-connection CBR $src Null $dst 0]
	$ns at [expr 0.0001 + $i/1000.0] "$cbr($i) start"
}

proc new_ftp {i src dst} {
	global ns opt ftp
	set tcp [$ns create-connection $opt(tcp) $src $opt(sink) $dst 0]
	set ftp($i) [$tcp attach-source FTP]
	$ns at [expr 0.0002 + $i/1000.0] "$ftp($i) start"
}

proc new_telnet {i src dst} {
	global ns opt telnet
	set tcp [$ns create-connection $opt(tcp) $src $opt(sink) $dst 0]
	set telnet($i) [$tcp attach-source Telnet]
	$ns at [expr 0.0003 + $i/1000.0] "$telnet($i) start"
}


## MAIN ##
getopt $argc $argv
if {$opt(seed) >= 0} { ns-random $opt(seed) }
if [info exists opt(qsize)] { Queue set limit_ $opt(qsize) }

set ns [new Simulator]
set trfd [create-trace]

create-topology
$lan trace $ns $trfd

create-source
$ns at $opt(stop) "finish"
$ns run
