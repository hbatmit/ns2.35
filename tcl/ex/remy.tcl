# Experiments with on-off sources that transmit data for a certain
# "on" time and then are silent for a certain "off" time. The on and
# off times come from exponential distributions at specifiable rates.
# During the "on" period, the data isn't sent at a constant bit rate
# as in the existing exponential on-off traffic model in
# tools/expoo.cc but is instead sent according to the underlying
# transport (agent) protocol, such as TCP.  The "off" period is the
# same as in that traffic model.

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

# source, sink, and app types
set opt(nsrc) 2;                # number of sources in experiment
set opt(tcp) TCP/Reno
set opt(sink) TCPSink
set opt(app) FTP
set opt(pktsize) 1460

# topology parameters
set opt(gw) DropTail;           # queueing at bottleneck
set opt(bneck) 1Mb;             # bottleneck bandwidth (for some topos)
set opt(maxq) 1000;             # max queue length at bottleneck
set opt(delay) 50ms;            # total one-way delay in topology


# random on-off times for sources
set opt(seed) 0
set opt(onrand) Exponential
set opt(offrand) Exponential
set opt(onavg) 20.0;              # mean on and off time
set opt(offavg) 20.0;              # mean on and off time

# simulator parameters
set opt(simtime) 1000.0;        # total simulated time
set opt(tr) remyout;            # output trace in opt(tr).out

# utility and scoring
set opt(alpha) 1.0

proc Usage {} {
    global opt argv0
    puts "Usage: $argv0 \[-simtime seconds\] \[-seed value\] \[-nsrc numSources\]"
    puts "\t\[-tr tracefile\]"
    puts "\t\[-bw $opt(bneck)] \[-delay $opt(delay)\]"
    exit 1
}

proc Getopt {} {
    global opt argc argv
#    if {$argc == 0} Usage
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
            qmax { set opt($key) Queue/$val }
        }
    }
}

#
# Create a simple dumbbell topology.
#
proc create-dumbbell-topology {bneckbw delay} {
    global ns opt s gw d 
    for {set i 0} {$i < $opt(nsrc)} {incr i} {
#        $ns duplex-link $s($i) $gw 10Mb 1ms DropTail
#        $ns duplex-link $gw $d $bneckbw $delay DropTail
        $ns duplex-link $s($i) $gw 10Mb 1ms $opt(gw)
        $ns duplex-link $gw $d $bneckbw $delay $opt(gw)
    }
}

proc finish {} {
    global ns opt recvapp ontime
    global f

    puts "ConnID\tMbits\tMbits/s\tAvgSRTT\tOn%\tUtility"
    for {set i 0} {$i < $opt(nsrc)} {incr i} {
        if { $ontime($i) > 0.0 } {
#            puts [$recvapp($i) results]
            set totalbytes [expr [lindex [$recvapp($i) results] 0] ]
            if { [lindex [$recvapp($i) results] 2] > 0 } {
                set avgrtt [expr [lindex [$recvapp($i) results] 1] / [lindex [$recvapp($i) results] 2] ]
            } else {
                set avgrtt 0
            }
            set throughput [expr 8 * [lindex [$recvapp($i) results] 0] / $ontime($i) ]
#            set utility [expr log($throughput) - [expr $opt(alpha)*log($delay)]]
#            set U [expr $throughput/$opt(bneck)]
#            set D [expr $delay/(2*$opt(delay) + 2)]
#            set utility [expr log($U) - [expr $opt(alpha)*log($D)]
            set utility [expr log($throughput) - [expr $opt(alpha)*log($avgrtt)]]
            puts [ format "%d\t%.2f\t%.2f\t%.1f\t%.0f\t%.2f" $i [expr 8.0*$totalbytes/1000000] [expr $throughput/1000000.0] $avgrtt [expr 100.0*$ontime($i)/$opt(simtime)] $utility ]
        }
    }

#    $ns flush-trace
#    close $f
    exit 0
}

Class LoggingApp -superclass Application

LoggingApp instproc init {id} {
    $self set connid_ $id
    $self set nbytes_ 0
    $self set cumsrtt_ 0.0
    $self set numsamples_ 0
    eval $self next
}

LoggingApp instproc recv {bytes} {
    $self instvar nbytes_ connid_ cumsrtt_ numsamples_
    global ns tp

#    puts "[$ns now]: CWND $connid_: [ [lindex $tp($connid_) 0] set cwnd_ ]"
#    puts "[$ns now]: SRTT $connid_: [ [lindex $tp($connid_) 0] set srtt_ ]"
    set nbytes_ [expr $nbytes_ + $bytes]
    set cumsrtt_ [expr [ [lindex $tp($connid_) 0] set srtt_ ]  + $cumsrtt_]
    set numsamples_ [expr $numsamples_ + 1]
    return nbytes_
}

LoggingApp instproc results { } {
    $self instvar nbytes_ cumsrtt_ numsamples_
    return [list $nbytes_ $cumsrtt_ $numsamples_]
}

#remove-all-packet-headers;      # removes all except common header
#add-packet-header IP TCP 

## MAIN ##

Getopt
if { $opt(seed) >= 0 } {
    ns-random $opt(seed)
}
if [info exists opt(maxq)] {
    Queue set limit_ $opt(maxq)
}

set ns [new Simulator]

# if we don't set up tracing early, trace output isn't created!!
#set f [open opt(tr).tr w]
#$ns trace-all $f

# create sources, destinations, gateways
for {set i 0} {$i < $opt(nsrc)} {incr i} {
    set s($i) [$ns node]
}
set d [$ns node];               # destination for all the TCPs
set gw [$ns node];              # bottleneck router

create-dumbbell-topology $opt(bneck) $opt(delay)
set numsrc $opt(nsrc)

for {set i 0} {$i < $numsrc} {incr i} {
    set tp($i) [$ns create-connection-list $opt(tcp) $s($i) $opt(sink) $d 0]
    [lindex $tp($i) 0] set window_ 130
    [lindex $tp($i) 0] set packetSize_ $opt(pktsize)
    set src($i) [ [lindex $tp($i) 0] attach-app $opt(app) ]
    set recvapp($i) [new LoggingApp $i]
    $recvapp($i) attach-agent [lindex $tp($i) 1]
    $ns at 0.0 "$recvapp($i) start"
}

for {set i 0} {$i < $numsrc} {incr i} {
    set on_ranvar($i) [new RandomVariable/$opt(onrand)]
    $on_ranvar($i) set avg_ $opt(onavg)
    set off_ranvar($i) [new RandomVariable/$opt(offrand)]
    $off_ranvar($i) set avg_ $opt(offavg)
}

for {set i 0} {$i < $numsrc} {incr i} {
    set state($i) OFF
    set curtime($i) 0.0
    set ontime($i) 0.0;         # total time spent in the "on" phase
    if {$i % 2 == 1} {
        $ns at 0.0 "$src($i) start"
        set state($i) ON
#        puts "$curtime($i): Turning on $i"
    }
        
    while {$curtime($i) < $opt(simtime)} {
        if {$state($i) == "OFF"} {
            set r [$off_ranvar($i) value]
        } else {
            set r [$on_ranvar($i) value]
        }
        set nexttime [expr $curtime($i) + $r]
        set lastepoch [expr $nexttime - $curtime($i)]
        set curtime($i) $nexttime
        if { $state($i) == "OFF" } {
            $ns at $curtime($i) "$src($i) start"
#            puts "$curtime($i): Turning on $i"
            set state($i) ON
        } else {
            set ontime($i) [expr $ontime($i) + $lastepoch]
            $ns at $curtime($i) "$src($i) stop"
#            puts "$curtime($i): Turning off $i"
            set state($i) OFF
        }
    }
}

puts "Results for $opt(tcp) over $opt(simtime) seconds:"
$ns at $opt(simtime) "finish"

$ns run
