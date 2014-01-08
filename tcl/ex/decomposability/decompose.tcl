# Experiments with on-off sources that transmit data for a certain
# "on" time and then are silent for a certain "off" time. The on and
# off times come from exponential distributions at specifiable rates.
# During the "on" period, the data isn't sent at a constant bit rate
# as in the existing exponential on-off traffic model in
# tools/expoo.cc but is instead sent according to the underlying
# transport (agent) protocol, such as TCP.  The "off" period is the
# same as in that traffic model.
# Anirudh: This code's been refactored now to read:
# 1. A topology file.
# 2. A source-destination pairs file. 

#!/bin/sh
# the next line finds ns \
nshome=`dirname $0`; [ ! -x $nshome/ns ] && [ -x ../../../ns ] && nshome=../../..
# the next line starts ns \
export nshome; exec $nshome/ns "$0" "$@"

if [info exists env(nshome)] {
	set nshome $env(nshome)
} elseif [file executable ../../../ns] {
	set nshome ../../..
} elseif {[file executable ./ns] || [file executable ./ns.exe]} {
	set nshome "[pwd]"
} else {
	puts "$argv0 cannot find ns directory"
	exit 1
}
set env(PATH) "$nshome/bin:$env(PATH)"

proc Getopt {} {
    global opt argc argv
    for {set i 1} {$i < $argc} {incr i} {
        set key [lindex $argv $i]
        if ![string match {-*} $key] continue
        set key [string range $key 1 end]
        set val [lindex $argv [incr i]]
        set opt($key) $val
        if [string match {-[A-z]*} $val] {
            incr i -1
            continue
        }
    }
}

# Create topology from file
proc create-topology {topology_file} {
    global ns opt node_array
    set topo_fd [open $topology_file r]
    while {[gets $topo_fd line] >= 0} {
        set fields [regexp -inline -all -- {\S+} $line]
        set src [lindex $fields 0]
        set dst [lindex $fields 1]
        set bw  [lindex $fields 2]
        set delay [lindex $fields 3]
        if { ! [info exists node_array($src)] } {
            set node_array($src) [$ns node]
        }
        if { ! [info exists node_array($dst)] } {
            set node_array($dst) [$ns node]
        }
        $ns duplex-link $node_array($src) $node_array($dst) ${bw}Mb ${delay}ms $opt(gw)
        # Set qsize to 5 times the bandwidth-RTT product measured in packets
        set qsize [expr (5 * ${bw} * 2 * ${delay} * 1000.0) / (8 * ($opt(pktsize) + $opt(hdrsize)))]

        puts "Queue size is $qsize"
        $ns queue-limit $node_array($src) $node_array($dst) $qsize
        $ns queue-limit $node_array($dst) $node_array($src) $qsize

        if { $opt(gw) == "XCP" } {
            # Hari: not clear why the XCP code doesn't do this automatically
            set lnk [$ns link $node_array($src) $node_array($dst)]
            set q [$lnk queue]
            $q set-link-capacity [ [$lnk set link_] set bandwidth_ ]
            set rlnk [$ns link $node_array($dst) $node_array($src)]
            set rq [$rlnk queue]
            $rq set-link-capacity [ [$rlnk set link_] set bandwidth_ ]
        }
    }
}

proc create-sources-destinations {sdpairs_file} {
    global ns opt node_array app_src tp linuxcc f
    if { [string range $opt(tcp) 0 9] == "TCP/Linux/"} {
        set linuxcc [ string range $opt(tcp) 10 [string length $opt(tcp)] ]
        set opt(tcp) "TCP/Linux"
    }
    set sdpairs_fd [open $sdpairs_file r]
    set i 0
    while {[gets $sdpairs_fd line] >= 0} {
        set fields [regexp -inline -all -- {\S+} $line]
        set src [lindex $fields 0]
        set dst [lindex $fields 1]
        assert [info exists node_array($src)];
        assert [info exists node_array($dst)];
        set tp($i) [$ns create-connection-list $opt(tcp) $node_array($src) $opt(sink) $node_array($dst) $i]
        set tcpsrc  [lindex $tp($i) 0]
        set tcpsink [lindex $tp($i) 1]
	if { $opt(tcp) == "TCP/Rational" } {
		# Turn on modified timestamps just for RemyCC
		$tcpsink set ts_echo_bugfix_ 0
	}

        if { [info exists linuxcc] } { 
            $ns at 0.0 "$tcpsrc select_ca $linuxcc"
            $ns at 0.0 "$tcpsrc set_ca_default_param linux debug_level 2"
        }

        if { [string first "Rational" $opt(tcp)] != -1 } {
            if { $opt(tracewhisk) == "all" || $opt(tracewhisk) == $i } {
                $tcpsrc set tracewhisk_ 1
                puts "tracing ON for connection $i: $opt(tracewhisk)"
            } else {
                $tcpsrc set tracewhisk_ 0
                puts "tracing OFF for connection $i: $opt(tracewhisk)"
            }
        }
	$tcpsrc set fid_ 0
        $tcpsrc set window_ $opt(rcvwin)
        $tcpsrc set packetSize_ $opt(pktsize)
        $tcpsrc set syn_ 0
        $tcpsrc set delay_growth_ 0

        if { [info exists opt(tr)] } {
            $tcpsrc trace cwnd_
            $tcpsrc trace rtt_
            $tcpsrc trace maxseq_
            $tcpsrc trace ack_
            if { $opt(tcp) == "TCP/Rational" } {
                $tcpsrc trace _intersend_time
            }
            $tcpsrc attach $f
        }

        set app_src($i) [new Application/OnOff $opt(ontype) $i $opt(pktsize) $opt(hdrsize) $opt(run) $opt(onavg) $opt(offavg) $tcpsrc [string equal $opt(tcp) "TCP/Rational"]]
        $app_src($i) attach-agent $tcpsrc
        incr i
    }
}

proc finish {} {
    global ns opt app_src
    global f
    puts "size [array size app_src]"
    for {set i 0} {$i < [array size app_src]} {incr i} {
      $app_src($i) stats
    }

    if { [info exists f] } {
        $ns flush-trace
        close $f
    }
    exit 0
}

## MAIN ##
source decomposeconf/workloads.tcl

if {[llength $argv] < 2} {
    puts "Usage: <topology-file> <sd-file>";
    exit 1;
}
set topology_file [lindex $argv 0]
set sdpairs_file  [lindex $argv 1]

Getopt

global defaultRNG

set ns [new Simulator]

RandomVariable/Pareto set shape_ 0.5

if { [info exists opt(tr)] } {
    # if we don't set up tracing early, trace output isn't created!!
    set f [open $opt(tr).tr w]
    $ns trace-all $f
}

set flowfile flowcdf-allman-icsi.tcl

if { $opt(ontype) == "flowcdf" } {
    source $flowfile
}

create-topology $topology_file
create-sources-destinations $sdpairs_file

$ns at $opt(simtime) "finish"
$ns run
