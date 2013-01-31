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

source timer.tcl

set conffile [lindex $argv 0]
#set conffile remyconf/vz4gdown.tcl
#set conffile remyconf/equisource.tcl

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

Class LoggingApp -superclass {Application Timer}

LoggingApp instproc init {id} {
    $self set srcid_ $id
    $self set nbytes_ 0
    $self set cumrtt_ 0.0
    $self set numsamples_ 0
    $self set u_ [new RandomVariable/Uniform]
    $self settype
    $self next
}

LoggingApp instproc settype { } {
    $self instvar endtime_ maxbytes_
    global opt
    if { $opt(ontype) == "bytes" } {
        $self set endtime_ $opt(simtime)
        $self set maxbytes_ 0        
    } else {
        $self set maxbytes_ "infinity"; # not byte-limited
        $self set endtime_ 0
    }
}

# called at the start of the simulation for the first run
LoggingApp instproc go { starttime } {
    $self instvar maxbytes_ endtime_ laststart_ srcid_ state_ u_
    global ns opt src on_ranvar flowcdf

    set laststart_ $starttime
    $ns at $starttime "$src($srcid_) start"    
    if { $starttime >= [$ns now] } {
        set state_ ON
        if { $opt(ontype) == "bytes" } {
            set maxbytes_ [$on_ranvar($srcid_) value]; # in bytes
        } elseif  { $opt(ontype) == "time" } {
            set endtime_ [$on_ranvar($srcid_) value]; # in time
        } else {
            $u_ set min_ 0.0
            $u_ set max_ 1.0
            set r [$u_ value]
            set idx [expr int(100000*$r)]
            if { $idx > [llength $flowcdf] } {
                set idx [expr [llength $flowcdf] - 1]
            }
            set maxbytes_ [expr 40 + [lindex $flowcdf $idx]]
#            puts "Flow len $maxbytes_"
        }
        # puts "$starttime: Turning on $srcid_ for $maxbytes_ bytes $endtime_ sec"

    } else {
        $self sched [expr $starttime - [$ns now]]
        set state_ OFF
    }
}

LoggingApp instproc timeout {} {
    $self go $laststart_
}

LoggingApp instproc recv { bytes } {
    # there's one of these objects for each src/dest pair 
    $self instvar nbytes_ srcid_ cumrtt_ numsamples_ maxbytes_ endtime_ laststart_ state_ u_
    global ns opt src tp on_ranvar off_ranvar stats flowcdf

    if { $state_ == OFF } {
        if { [$ns now] >= $laststart_ } {
            set state_ ON
        }
    }

    if { $state_ == ON } {
        set nbytes_ [expr $nbytes_ + $bytes]
        set tcp_sender [lindex $tp($srcid_) 0]
        set rtt_ [expr [$tcp_sender set rtt_] * [$tcp_sender set tcpTick_]]
        if {$rtt_ > 0.0} {
            set cumrtt_ [expr $rtt_  + $cumrtt_]
            set numsamples_ [expr $numsamples_ + 1]
        }
        set ontime [expr [$ns now] - $laststart_]
        if { $nbytes_ >= $maxbytes_ || $ontime >= $endtime_ || $opt(simtime) <= [$ns now]} {
#            puts "[$ns now]: Turning off $srcid_"
            $ns at [$ns now] "$src($srcid_) stop"
            $stats($srcid_) update $nbytes_ $ontime $cumrtt_ $numsamples_
            set nbytes_ 0
            set state_ OFF
            set nexttime [expr [$ns now] + [$off_ranvar($srcid_) value]]; # stay off until nexttime
            set laststart_ $nexttime
            puts "off [expr $nexttime - [$ns now]]"
            if { $nexttime < $opt(simtime) } { 
                # set up for next on period
                if { $opt(ontype) == "bytes" } {
                    set maxbytes_ [$on_ranvar($srcid_) value]; # in bytes
                } elseif  { $opt(ontype) == "time" } {
                    set endtime_ [$on_ranvar($srcid_) value]; # in time
                } else {
                    set r [$u_ value]
                    set maxbytes_ [expr 40 + [ lindex $flowcdf [expr int(100000*$r)]]]
#                    puts "Flow len $maxbytes_"
                }
                $ns at $nexttime "$src($srcid_) start"; # schedule next start
#                puts "$nexttime: Turning on $srcid_ for $maxbytes_ bytes $endtime_ s"
            }
        }
        return nbytes_
    }
}

LoggingApp instproc results { } {
    $self instvar nbytes_ cumrtt_ numsamples_
    return [list $nbytes_ $cumrtt_ $numsamples_]
}

Class StatCollector 

StatCollector instproc init {id ctype} {
    $self set srcid_ $id
    $self set ctype_ $ctype;    # type of congestion control / tcp
    $self set numbytes_ 0
    $self set ontime_ 0.0;     # total time connection was in ON state
    $self set cumrtt_ 0.0
    $self set nsamples_ 0
    $self set nconns_ 0
}

StatCollector instproc update {newbytes newtime cumrtt nsamples} {
    global ns opt
    $self instvar srcid_ numbytes_ ontime_ cumrtt_ nsamples_ nconns_
    incr numbytes_ $newbytes
    set ontime_ [expr $ontime_ + $newtime]
    set cumrtt_ $cumrtt
    set nsamples_ $nsamples
    incr nconns_
#    puts "[$ns now]: updating stats for $srcid_: $newbytes $newtime $cumrtt $nsamples"
#    puts "[$ns now]: \tTO: $numbytes_ $ontime_ $cumrtt_ $nsamples_"
    if { $opt(partialresults) } {
        showstats False
    }
}

StatCollector instproc results { } {
    $self instvar numbytes_ ontime_ cumrtt_ nsamples_ nconns_
    return [list $numbytes_ $ontime_ $cumrtt_ $nsamples_ $nconns_]
}

#
# Create a simple dumbbell topology.
#
proc create-dumbbell-topology {bneckbw delay} {
    global ns opt s gw d accessrate accessdelay nshome
    for {set i 0} {$i < $opt(nsrc)} {incr i} {
#        $ns duplex-link $s($i) $gw 10Mb 1ms DropTail
#        $ns duplex-link $gw $d $bneckbw $delay DropTail
        $ns duplex-link $s($i) $gw $accessrate($i) $accessdelay($i) $opt(gw)
        $ns queue-limit $s($i) $gw $opt(maxq)
        $ns queue-limit $gw $s($i) $opt(maxq)
        if { $opt(gw) == "XCP" } {
            # not clear why the XCP code doesn't do this automatically
            set lnk [$ns link $s($i) $gw]
            set q [$lnk queue]
            $q set-link-capacity [ [$lnk set link_] set bandwidth_ ]
            set rlnk [$ns link $gw $s($i)]
            set rq [$rlnk queue]
            $rq set-link-capacity [ [$rlnk set link_] set bandwidth_ ]
        }
    }
    if { $opt(link) == "trace" } {
        $ns simplex-link $d $gw [ bw_parse $bneckbw ] $delay $opt(gw)
#        [ [ $ns link $d $gw ] link ] trace-file "$nshome/link/tracedata/uplink-verizon4g.pps"
        source $nshome/link/trace.tcl
        $ns simplex-link $gw $d [ bw_parse $bneckbw ] $delay $opt(gw)
        [ [ $ns link $gw $d ] link ] trace-file $opt(linktrace)
    } else {
        $ns duplex-link $gw $d $bneckbw $delay $opt(gw)
    }
    $ns queue-limit $gw $d $opt(maxq)
    $ns queue-limit $d $gw $opt(maxq)    
    if { $opt(gw) == "XCP" } {
        # not clear why the XCP code doesn't do this automatically
        set lnk [$ns link $gw $d]
        set q [$lnk queue]
        $q set-link-capacity [ [$lnk set link_] set bandwidth_ ]
        set rlnk [$ns link $d $gw]
        set rq [$rlnk queue]
        $rq set-link-capacity [ [$rlnk set link_] set bandwidth_ ]
    }
}

proc create-sources-sinks {} {
    global ns opt s d src recvapp tp

    set numsrc $opt(nsrc)
    if { [string range $opt(tcp) 0 9] == "TCP/Linux/"} {
        set linuxcc [ string range $opt(tcp) 10 [string length $opt(tcp)] ]
        set opt(tcp) "TCP/Linux"
    }

    for {set i 0} {$i < $numsrc} {incr i} {
        set tp($i) [$ns create-connection-list $opt(tcp) $s($i) $opt(sink) $d $i]
        set tcpsrc [lindex $tp($i) 0]
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
        $tcpsrc set window_ $opt(rcvwin)
        $tcpsrc set packetSize_ $opt(pktsize)
#        set src($i) [ $tcpsrc attach-app $opt(app) ]
        set src($i) [ $tcpsrc attach-source $opt(app) ]
        set recvapp($i) [new LoggingApp $i]
        $recvapp($i) attach-agent $tcpsink
        $ns at 0.0 "$recvapp($i) start"
    }
}

proc showstats {final} {
    global ns opt stats

    for {set i 0} {$i < $opt(nsrc)} {incr i} {
        set res [$stats($i) results]
        set totalbytes [lindex $res 0]
        set totaltime [lindex $res 1]
        set totalrtt [lindex $res 2]
        set nsamples [lindex $res 3]
        set nconns [lindex $res 4]

        if { $nsamples > 0.0 } {
            set avgrtt [expr 1000*$totalrtt/$nsamples]
        } else {
            set avgrtt 0.0
        }
        if { $totaltime > 0.0 } {
            set throughput [expr 8.0 * $totalbytes / $totaltime]
            set utility [expr log($throughput) - [expr $opt(alpha)*log($avgrtt)]]
            if { $final == True } {
                puts [ format "FINAL\t%d\t%d\t%.3f\t%.1f\t%.4f\t%.2f\t%d" $i $totalbytes [expr $throughput/1000000.0] $avgrtt [expr 100.0*$totaltime/$opt(simtime)] $utility $nconns ]
            } else {
                puts [ format "-\t%d\t%d\t%.3f\t%.1f\t%.4f\t%.2f\t%d" $i $totalbytes [expr $throughput/1000000.0] $avgrtt [expr 100.0*$totaltime/$opt(simtime)] $utility $nconns]
            }
        }
    }
}

proc finish {} {
    global ns opt stats
    global f
    showstats True

#    $ns flush-trace
#    close $f
    exit 0
}


## MAIN ##

source $conffile
puts "Reading params from $conffile"

Getopt

set_access_params $opt(nsrc)

if { $opt(gw) == "XCP" } {
    remove-all-packet-headers       ; # removes all except common
    add-packet-header Flags IP TCP XCP ; # hdrs reqd for validation
}
    
if { $opt(seed) >= 0 } {
    ns-random $opt(seed)
}

set ns [new Simulator]

Queue set limit_ $opt(maxq)
#RandomVariable/Pareto set shape_ 0.5

# if we don't set up tracing early, trace output isn't created!!
#set f [open opt(tr).tr w]
#$ns trace-all $f

set flowfile flowcdf-allman-icsi.tcl

# create sources, destinations, gateways
for {set i 0} {$i < $opt(nsrc)} {incr i} {
    set s($i) [$ns node]
}
set d [$ns node];               # destination for all the TCPs
set gw [$ns node];              # bottleneck router

create-dumbbell-topology $opt(bneck) $opt(delay)
create-sources-sinks

for {set i 0} {$i < $opt(nsrc)} {incr i} {
    set on_ranvar($i) [new RandomVariable/$opt(onrand)]
    if { $opt(ontype) == "time" } {
        $on_ranvar($i) set avg_ $opt(onavg)
    } elseif { $opt(ontype) == "bytes" } {
            $on_ranvar($i) set avg_ $opt(avgbytes)
    } elseif { $opt(ontype) == "flowcdf" } {
        source $flowfile
    }
    set off_ranvar($i) [new RandomVariable/$opt(offrand)]
    $off_ranvar($i) set avg_ $opt(offavg)
    set stats($i) [new StatCollector $i $opt(tcp)]
}

for {set i 0} {$i < $opt(nsrc)} {incr i} {
    if { [expr $i % 2] == 1 } {
        # start only the odd-numbered connections immediately
        $recvapp($i) go 0.0
    } else {
        $recvapp($i) go [$off_ranvar($i) value]
    }
}

if { [info exists linuxcc] } {
    puts "Results for $opt(tcp)/$linuxcc $opt(gw) $opt(sink) over $opt(simtime) seconds:"
} else {
    puts "Results for $opt(tcp) $opt(gw) $opt(sink) over $opt(simtime) seconds:"
}

puts "\tSrcID\tBytes\tMbits/s\tAvgRTT\tOn%\tUtility\tNumConns"

$ns at $opt(simtime) "finish"

$ns run

