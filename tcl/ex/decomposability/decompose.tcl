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

source sender-app.tcl
source logging-app2.tcl
source stats.tcl

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
        if { $qsize < $opt(maxq) } {
          set qsize $opt(maxq);
        }
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
    global ns opt node_array app_src recvapp tp linuxcc f
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
	$tcpsrc set fid_ [expr $i%256]
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

        set app_src($i) [ $tcpsrc attach-app $opt(app) ]
        $app_src($i) setup_and_start $i $tcpsrc
        set recvapp($i) [new LoggingApp $i]
        $recvapp($i) attach-agent $tcpsink
        $ns at 0.0 "$recvapp($i) start"
        incr i
    }
}

proc finish {} {
    global ns opt stats app_src recvapp linuxcc
    global f
    puts "size [array size app_src]"    
    for {set i 0} {$i < [array size app_src]} {incr i} {
        set sapp $app_src($i)
        $sapp dumpstats 
        set rcdbytes [$recvapp($i) set nbytes_]
        set rcd_nrtt [$recvapp($i) set nrtt_]
        if { $rcd_nrtt > 0 } {
            set rcd_avgrtt [expr 1000.0*[$recvapp($i) set cumrtt_] / $rcd_nrtt ]
        } else {
            set rcd_avgrtt 0.0
        }
        if {$i == 0} {
            if { [info exists linuxcc] } {
                puts "Results for $opt(tcp)/$linuxcc $opt(gw) $opt(sink) over $opt(simtime) seconds:"
            } else {
                puts "Results for $opt(tcp) $opt(gw) $opt(sink) over $opt(simtime) seconds:"
            }
        }

        [$sapp set stats_] showstats $rcdbytes $rcd_avgrtt
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

if { ![info exists opt(reset)] } {
    set opt(reset) true;    # reset TCP connection on end of ON period
}

global defaultRNG
$defaultRNG seed $opt(seed)

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
